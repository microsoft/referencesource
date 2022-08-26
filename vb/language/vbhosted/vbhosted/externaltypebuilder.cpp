#include "stdafx.h"

using namespace System::Reflection;

ExternalTypeBuilder::ExternalTypeBuilder(Compiler* pCompiler, CompilerFile* pCompilerFile, Symbols* pSymbols, VbContext* pContext, ExternalSymbolFactory* pFactory)
    : m_pCompiler(pCompiler), 
    m_pCompilerFile(pCompilerFile),
    m_pSymbols(pSymbols),
    m_pFactory(pFactory),
    m_pVbContext(pContext)
{
    ASSERT(pCompiler, "[ExternalTypeBuilder::ExternalTypeBuilder] 'pCompiler' parameter is null");
    ASSERT(pCompilerFile, "[ExternalTypeBuilder::ExternalTypeBuilder] 'pCompilerFile' parameter is null");
    ASSERT(pSymbols, "[ExternalTypeBuilder::ExternalTypeBuilder] 'pSymbols' parameter is null");
    ASSERT(pFactory, "[ExternalTypeBuilder::ExternalTypeBuilder] 'pFactory' parameter is null");
    ASSERT(pContext, "[ExternalTypeBuilder::ExternalTypeBuilder] 'pContext' parameter is null");
}

ExternalTypeBuilder::~ExternalTypeBuilder()
{
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetNamespaceForName(const STRING* Name, BCSYM_Namespace *pParentNamespace)
{
    HRESULT hr = S_OK;
    BCSYM_Namespace* pNS = NULL;
    STRING* pstrQualifiedName = NULL;

    VerifyInPtrRetNull(Name, "[ExternalTypeBuilder::GetNamespaceForName] 'Name' parameter is NULL");
    VerifyInPtrRetNull(pParentNamespace, "[ExternalTypeBuilder::GetNamespaceForName] 'pParentNamespace' parameter is NULL");
    
    //First, need to build the full name to use.
    //This feels really ---- because in all likelihood we have this as a complete string somewhere else.
    // Unfortunately, at this point in the callstack we only have the current name, and the parent we're looking up in.
    if (StringPool::IsNullOrEmpty(pParentNamespace->GetQualifiedEmittedName()))
    {
        //If pParentNamespace->GetQualifiedName() is Null or Empty, we should be in the unnamed namespace
        // Which means that we should NOT be concatenating or adding "."
        pstrQualifiedName = m_pCompiler->AddString(Name);
    }
    else
    {
        const int nNames = 2;
        const WCHAR* pNames[2];
        pNames[0] = pParentNamespace->GetQualifiedEmittedName();
        pNames[1] = Name;

        pstrQualifiedName = m_pCompiler->ConcatStringsArray(nNames, pNames, WIDE("."));
    }

    BOOL bExists = FALSE;

    IfFailGo(m_pVbContext->NamespaceExists(CComBSTR(pstrQualifiedName), &bExists));
    if (bExists == TRUE)
    {
        pNS = m_pCompiler->GetNamespace(
            pstrQualifiedName,
            m_pCompilerFile,
            NULL
            );

        ASSERT(pNS, "[ExternalTypeBuilder::GetNamespaceForName] 'pNS' variable not created by m_pCompiler->GetNamespace");
        IfFalseGo(pNS, E_UNEXPECTED);
        ASSERT(pNS->GetHashRaw(), "[ExternalTypeBuilder::GetNamespaceForName] 'pNS' variable has no HashTable");
        IfFalseGo(pNS->GetHashRaw(), E_UNEXPECTED);
        pNS->GetHashRaw()->SetExternalSymbolSource(m_pFactory->GetTypeFactory(pNS));
        pNS->GetHashRaw()->SetIsHostedDynamicHash(true);
		pNS->GetNamespaceRing()->SetContainsHostedDynamicHash(true);

    }

Error:
    return ((hr == S_OK) && pNS) ? pNS->PNamedRoot() : NULL;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetTypeForName(const STRING* Name, BCSYM_Namespace* pParentNamespace)
{
    HRESULT hr = S_OK;
    BCSYM_NamedRoot* pSymbol = NULL;

    VerifyInPtrRetNull(Name, "[ExternalTypeBuilder::GetTypeForName] 'Name' parameter is NULL");
    VerifyInPtrRetNull(pParentNamespace, "[ExternalTypeBuilder::GetTypeForName] 'pParentNamespace' parameter is NULL");

    gcroot<array<System::Type^,1>^> types;
    gcroot<System::Type^> type;

    IfFailGo(m_pVbContext->FindTypes(CComBSTR(Name), CComBSTR(pParentNamespace->GetQualifiedEmittedName()), types));
    if (types && types->Length)
    {
        for (int index = 0; index < types->Length; index++)
        {
            type = types[index];
            //We do not support ITypeScope returning Nested Types, but we're not going 
            //to prevent other types being returned.
            if (type && !type->IsNested && type->IsPublic)
            {
                //We do not support ITypeScope returning types that don't match what we asked for,
                //but we're not going to prevent other types being returned.
                if (ValidateType(Name, pParentNamespace->GetQualifiedEmittedName(), type))
                {
                    BCSYM_NamedRoot* pNamedRoot = NULL;
                    IfFailGo(m_pFactory->GetTypeScopeType(type, &pNamedRoot));
                    ASSERT(pNamedRoot, "[ExternalTypeBuilder::GetTypeForName] 'pNamedRoot' should be defined in success case");
                    // Only containers can have bases and implements. Similarly, only External Symbols need special handling.
                    if (pNamedRoot->IsContainer() && pNamedRoot->GetExternalSymbol())
                    {
                        ASSERT(pNamedRoot->IsInterface() || pNamedRoot->IsClass(), "Not interface, not Class");
                        // IfFailGo will go back to Error, and route through to the Failed Case, below.
                        IfFailGo(EnsureBaseAndImplementsLoaded(pNamedRoot->PContainer()));
                    }
                    
                    pSymbol = pNamedRoot;
                    IfFalseGo(pSymbol, E_INVALIDARG);
                }
            }
        }
    }
    
Error:

    return pSymbol;
}

HRESULT ExternalTypeBuilder::SetupChildrenForType(BCSYM_Container* pTypeSymbol)
{
    gcroot<System::Type^> pType;
    HRESULT hr = S_OK;
    SymbolList symbolList;
    
    VerifyInPtr(pTypeSymbol, "[ExternalTypeBuilder::SetupChildrenForType] 'pTypeSymbol' parameter must not be NULL");
    ASSERT(pTypeSymbol->GetExternalSymbol(), "[ExternalTypeBuilder::SetupChildrenForType] 'pTypeSymbol' has no external symbol");

    IfFalseGo(pTypeSymbol->GetExternalSymbol(), E_UNEXPECTED);

    Symbols::SetChildrenLoading(pTypeSymbol, true);

    if (pTypeSymbol->IsClass() || pTypeSymbol->IsInterface() || pTypeSymbol->IsStruct())
    {
        Vtypes pvtUnderlyingType;

        IfFailGo(ConvertCOMTypeToSystemType(pTypeSymbol->GetExternalSymbol(), pType));
        IfFailGo(GetChildrenForType(pTypeSymbol, pType, &symbolList, &pvtUnderlyingType));

        if (pType->IsEnum)
        {
            //Set enum's underlying type
            pTypeSymbol->PClass()->SetVtype(pvtUnderlyingType);
        }
        else
        {
            Bindable::BindMetaDataContainer(pTypeSymbol, NULL);
        }
    
    }

        // We're fully loaded.
    Symbols::SetChildrenLoaded(pTypeSymbol);

    // We're no longer loading.  Don't bother resetting this if we get an
    // error, we're already horked.
    //
    Symbols::SetChildrenLoading(pTypeSymbol, false);
Error:
    return hr;
}

HRESULT ExternalTypeBuilder::EnsureBaseAndImplementsLoaded(BCSYM_Container* pTypeSymbol)
{
    VerifyInPtr(pTypeSymbol, "[ExternalTypeBuilder::EnsureBaseAndImplementsLoaded] 'pTypeSymbol' parameter must not be NULL");
    ASSERT(pTypeSymbol->GetExternalSymbol(), "[ExternalTypeBuilder::EnsureBaseAndImplementsLoaded] 'pTypeSymbol' has no external symbols");

    gcroot<System::Type^> pType;
    HRESULT hr = S_OK;

    IfFalseGo(pTypeSymbol->GetExternalSymbol(), E_UNEXPECTED);

    IfTrueRet(pTypeSymbol->AreBaseAndImplementsLoaded(), S_OK);
    IfTrueRet(pTypeSymbol->AreBaseAndImplementsLoading(), S_FALSE);
    
    Symbols::SetBaseAndImplementsLoading(pTypeSymbol, true);

    if (pTypeSymbol->IsClass() || pTypeSymbol->IsInterface())
    {
        IfFailGo(ConvertCOMTypeToSystemType(pTypeSymbol->GetExternalSymbol(), pType));
        IfFailGo(GetNestedTypesForType(pTypeSymbol, pType));
        IfFailGo(GetBaseAndImplementsForType(pTypeSymbol, pType));
    }

Error:
    // As per metaimport, even if there's an error, we should mark the base and implements as being loaded.
    ASSERT(SUCCEEDED(hr), "[ExternalTypeBuilder::EnsureBaseAndImplementsLoaded] 'pTypeSymbol' has not had Base and Implements loaded.");
    Symbols::SetBaseAndImplementsLoaded(pTypeSymbol);
    Symbols::SetBaseAndImplementsLoading(pTypeSymbol, false);

    return hr;
}

HRESULT ExternalTypeBuilder::GetBaseAndImplementsForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType)
{
    VerifyInPtr(pContainer, "[ExternalTypeBuilder::GetBaseAndImplementsForType] 'pContainer' parameter must not be null");
    VerifyInCLRPtr(pType, "[ExternalTypeBuilder::GetBaseAndImplementsForType] 'pType' parameter must not be null");

    HRESULT hr = S_OK;
    BCSYM* pBaseSymbol = NULL;
    BCSYM_Implements* pImplList = NULL;

    ASSERT(pContainer->IsClass() || pContainer->IsInterface(), "[ExternalTypeBuilder::GetBaseAndImplementsForType] 'pContainer' should either be an interface or class");

    if (pContainer->IsClass())
    {
        pBaseSymbol = GetBaseForType(pContainer, pType);

        if (pBaseSymbol)
        {
            //MetaImport tests the BaseSymbol to see if it is Delegate or MultiCastDelegate.
            // However, it is not possible to derive from a implemented delegate; each class must
            // inherit directly from Delegate or Multicast delegate.
            // If this type exists and it's base is a delegate, it's base class must either 
            // be Delegate or multicast delegate. Foregoing more complicated and costly check.
            if (pBaseSymbol->IsDelegate())
            {
                pContainer->PClass()->SetIsDelegate(true);
            }
            else if (pBaseSymbol->IsEnum())
            {
                pContainer->PClass()->SetIsEnum(true);
            }
            else if (pBaseSymbol->IsStruct())
            {
                pContainer->PClass()->SetIsStruct(true);
            }

            Symbols::SetBaseClass(pContainer->PClass(), pBaseSymbol);
        }
    }

    if (pContainer->IsInterface() || (pContainer->IsClass() && !pContainer->IsEnum()))
    {
        pImplList = GetImplementsForType(pContainer, pType);
        if (pImplList)
            Symbols::SetImplements(pContainer, pImplList);
    }

    return hr;
}

HRESULT ExternalTypeBuilder::GetNestedTypesForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType)
{
    VerifyInPtr(pContainer, "[ExternalTypeBuilder::GetNestedTypesForType] 'pContainer' parameter must not be null");
    VerifyInCLRPtr(pType, "[ExternalTypeBuilder::GetNestedTypesForType] 'pType' parameter must not be null");

    HRESULT hr = S_OK;
#if DEBUG
    BCSYM* pSymbol = NULL;
#endif

    gcroot<array<System::Reflection::MemberInfo^>^> members = pType->GetNestedTypes();
    IfFalseGo(members != nullptr && members->Length > 0, S_OK);

    for (int i = 0; i < members->Length; i++)
    {
        gcroot<System::Reflection::MemberInfo^> member = members[i];
#if DEBUG
        pSymbol =
#endif
        GetSymbolForTypeNestedType(pContainer, member);
        ASSERT(pSymbol == NULL, 
            "[ExternalTypeBuilder::GetNestedTypesForType] 'pSymbol' variable should be NULL; GetSymbolForTypeNestedType should always return NULL");
    }

Error:
    return hr;
}

BCSYM_NamedRoot* ExternalTypeBuilder::MakeSymbolForType(gcroot<System::Type^> pType)
{
    HRESULT hr = S_OK;
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::MakeSymbolForType] 'pType' parameter is null");
    
    BCSYM_NamedRoot* pSymbol = NULL;

    if (pType->IsClass || pType->IsValueType || pType->IsEnum)
    {
        BCSYM_Class* pClass = NULL;
        pClass = MakeClassForType(pType);
        ASSERT(pClass->IsContainer(),
            "[ExternalTypeBuilder::MakeSymbolForType] 'pSymbol' Class did not return a Container");
    
        IfFailGo(LoadCustomAttributes(static_cast<MemberInfo^>(static_cast<System::Type^>(pType)), pClass));

        pSymbol = pClass;
    }
    else if (pType->IsInterface)
    {
        BCSYM_Interface* pInterface = NULL;
        pInterface = MakeInterfaceForType(pType);
        ASSERT(pInterface->IsInterface(),
            "[ExternalTypeBuilder::MakeSymbolForType] 'pSymbol' Interface did not return an Interface");

        IfFailGo(LoadCustomAttributes(static_cast<MemberInfo^>(static_cast<System::Type^>(pType)), pInterface));

        pSymbol = pInterface;
    }
    else
        VSFAIL("[ExternalTypeBuilder::MakeSymbolForType] 'pType' is not a Class. No other types supported yet.");

Error:
    return pSymbol;
}


//**************************************************************************************************************
// GetSymbolForType should only be used to get the symbol for a type to be used in contexts where a type is referred
// to (rather than defined). Eg: parameter type, return type, field type, base type, interface implements type, etc.
// To get the symbol corresponding to the definition for a type, the GetSymbolForTypeDefinition function needs to be
// used.
//
// Note that for generics, this function always (even for generic definitions) returns the instantiated generic i.e.
// a generic binding. This is required due to bug Dev10 698789.
//
//**************************************************************************************************************
BCSYM* ExternalTypeBuilder::GetSymbolForType(gcroot<System::Type^> pType, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethod)
{
    BCSYM* pSymbol = NULL;
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetSymbolForType] 'pType' parameter is null");

    if (pContainer && pType->IsGenericParameter)
    {
        pSymbol = GetSymbolForTypeDefinition(pType, pContainer, pMethod);
        ASSERT(pSymbol, "[ExternalTypeBuilder::GetSymbolForType] 'pSymbol' variable is not valid");
        IfFalseGo(pSymbol, E_UNEXPECTED);
    }
    else
    {
        if (pType->IsByRef || pType->IsArray)
        {

            mscorlib::_TypePtr pTypeCom;

            BCSYM* ptyp = GetSymbolForType(pType->GetElementType(), pContainer, pMethod);
            IfFalseGo(ptyp, E_INVALIDARG);
            if (ptyp->IsBad()) return ptyp;

            if (pType->IsArray)
            {
                int cDims = pType->GetArrayRank();
                pSymbol = m_pSymbols->GetArrayType(cDims, ptyp);
            }
            else
            {
                ASSERT(pType->IsByRef, "[ExternalTypeBuilder::GetSymbolForType] 'pType' is Not a By Ref, but also not an array");
                //VB Doesn't support Pointers.
                IfTrueGo(pType->IsPointer, E_UNEXPECTED);
                pSymbol = m_pSymbols->MakePtrType(ptyp);
            }

            IfFailGo(ConvertSystemTypeToCom(pType, &pTypeCom));
            pSymbol->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pTypeCom));
        }
        else if (pType->IsGenericType)
        {
            BCSYM* pGenericDefinition = NULL;
            gcroot<array<System::Type^,1>^> arguments = pType->GetGenericArguments();
            int ArgumentCount = arguments->Length;
            BCSYM **Arguments;
            StackAllocTypeArgumentsIfPossible(Arguments, ArgumentCount, m_pSymbols->GetNorlsAllocator(), (*m_pSymbols));

            for (int ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                Arguments[ArgumentIndex] = GetSymbolForType(arguments[ArgumentIndex], pContainer, pMethod);
                ASSERT(Arguments[ArgumentIndex], "[ExternalTypeBuilder::GetSymbolForType] 'Arguments' List is not valid");
                IfFalseGo(Arguments[ArgumentIndex], E_UNEXPECTED);
                if (Arguments[ArgumentIndex]->IsBad()) return Arguments[ArgumentIndex];
            }

            pGenericDefinition = GetSymbolForTypeDefinition(pType->GetGenericTypeDefinition(), pContainer, NULL);
            ASSERT(pGenericDefinition, "[ExternalTypeBuilder::GetSymbolForType] 'pGenericDefinition' variable is not valid");
            IfFalseGo(pGenericDefinition, E_UNEXPECTED);
            if (pGenericDefinition->IsBad()) return pGenericDefinition;

            pSymbol = BuildGenericBinding(pGenericDefinition, ArgumentCount, Arguments, ArgumentsStackAllocated);
        }
        else
        {
            pSymbol = GetSymbolForTypeDefinition(pType, pContainer, pMethod);
            ASSERT(pSymbol, "[ExternalTypeBuilder::GetSymbolForType] 'pSymbol' returned for GetSymbolForTypeDefinition is null");
            IfFalseGo(pSymbol, E_UNEXPECTED);
        }
    }

Error:
    if (FAILED(hr))
    {
        ASSERT(!pSymbol, "[ExternalTypeBuilder::GetSymbolForType] 'pSymbol'");
        pSymbol = Symbols::GetGenericBadNamedRoot();
    }

    return pSymbol;
}

//**************************************************************************************************************
// GetSymbolForTypeDefinition should only be used to get the symbol corresponding to the definition of a type.
// Hence for example, for generics, only types corresponding to generic definitions can be passed in.
//
// In order to get the symbol for a type to be used in contexts where a type is referred to (rather than defined),
// the GetSymbolForType function should be used. Example of such contexts: parameter type, return type, field type,
// base type, interface implements type, etc.
//
//**************************************************************************************************************
BCSYM* ExternalTypeBuilder::GetSymbolForTypeDefinition(gcroot<System::Type^> pType, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethod)
{
    BCSYM* pSymbol = NULL;
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pType' parameter is null");
    
    // Only type definitions are allowed.
    VerifyParamCondRetNull(!pType->IsByRef && !pType->IsArray,
        "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pType' is not a valid type definition");

    // Generic instantiations are not allowed.
    VerifyParamCondRetNull(!pType->IsGenericType || pType->IsGenericTypeDefinition,
        "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pType' parameter is a generic instantiation - not allowed");

    if (pContainer && pType->IsGenericParameter)
    {
        //System::Type::DeclaringMethod can be used to find out what Parameter source to use.
        // cf. Remarks on http://msdn.microsoft.com/en-us/library/system.type.genericparameterposition.aspx
        if (pType->DeclaringMethod)
        {
            pSymbol = GetSymbolForGenericMethod(pMethod, pType);
        }
        else
        {
            pSymbol = GetSymbolForGenericType(pContainer, pType);
        }
    }
    // pType->IsNested should usually be handled via m_pFactory->GetType. However,
    //  IScriptScope can return a NestedType, in which case all the parent types need to
    //  be constructed.
    else if (pType->IsNested && !pContainer && !pMethod)
    {
        BCSYM_NamedRoot* pNR = NULL;
        BCSYM* pTypeContainerSymbol = NULL;
        BCSYM_Container* pTypeContainer = NULL;

        // Recursive call will get the parent type until the type isn't nested.
        pTypeContainerSymbol = GetSymbolForTypeDefinition(pType->DeclaringType, NULL, NULL);
        ASSERT(pTypeContainerSymbol,
            "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pTypeContainer' Symbol for DeclaringType is NULL");
        IfFalseGo(pTypeContainerSymbol, E_UNEXPECTED);
        if (pTypeContainerSymbol->IsBad()) return pTypeContainerSymbol;
        ASSERT(pTypeContainerSymbol->IsContainer(),
            "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pTypeContainer' Symbol for DeclaringType is not container");
        IfFalseGo(pTypeContainerSymbol->IsContainer(), E_UNEXPECTED);
        
        pTypeContainer = pTypeContainerSymbol->PContainer();
        //Ensure is a no-op if already loaded; it should never be hit. If this assert is hit, add a call to
        // EnsureBaseAndImplementsLoaded.
        ASSERT(pTypeContainer->AreBaseAndImplementsLoaded() || pTypeContainer->AreBaseAndImplementsLoading(), 
            "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pTypeContainer' variable: Type not initialized properly.");
        
        if (pTypeContainerSymbol->GetExternalSymbol())
        {
            // Now the cache from RuntimeTypeHandle should have the cached entry for this nested type
            // This only works for ITypeScope-provided types, but is decidedly more performant than the
            // Symbol Lookup via Name.
            IfFailGo(m_pFactory->GetType(pType, &pNR));
        }
        else
        {
            // When loaded from a reference assembly, the type will NOT be guaranteed to be in the
            // Factory cache. Instead, look up the BCSYM in the Container.
            IfFailGo(m_pFactory->GetNestedTypeFromReferencedType(pType, pTypeContainer, &pNR));
        }

        pSymbol = pNR;
        ASSERT(pSymbol != NULL, "[ExternalTypeBuilder::GetSymbolForTypeDefinition] Nested Type lookup succeeded but pSymbol == NULL");

    }
    else
    {
        BCSYM_NamedRoot* pNR = NULL;
        IfFailGo(m_pFactory->GetType(pType, &pNR));
        pSymbol = pNR;
        ASSERT(pSymbol != NULL, "[ExternalTypeBuilder::GetSymbolForTypeDefinition] Succeeded but pSymbol == NULL");
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pSymbol, "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pSymbol' should be defined in success case");
        // Only containers can have bases and implements. Similarly, only External Symbols need special handling.
        if (pSymbol->IsContainer() && pSymbol->GetExternalSymbol())
        {
            ASSERT(pSymbol->IsInterface() || pSymbol->IsClass(), "Not interface, not Class");
            // IfFailGo will go back to Error, and route through to the Failed Case, below.
            IfFailGo(EnsureBaseAndImplementsLoaded(pSymbol->PContainer()));
        }
    }

Error:
    if (FAILED(hr))
    {
        ASSERT(!pSymbol, "[ExternalTypeBuilder::GetSymbolForTypeDefinition] 'pSymbol'");

        // Getting only the name and not the fullname in order to avoid showing users
        // the managed names for generics and nested types.
        //
        // The full name could potentially be shown correctly without too much work for
        // non-nested types. But this would require concatenating the namespace and
        // the typename in order to include it in the GenericBadNamedRoot so that the
        // error is reported correctly. But this could result in bloating the compiler
        // string pool which could have adverse perf affects when the same compiler
        // instance is re-used. This bloat could possibly be avoided by modifying
        // BCSYM_NamedRoot::ReportError. But at this point in time, in order to avoid
        // more code churn than necessary and given that this is a pathological scenario
        // anyway, sticking with the simpler solution of just displaying the unqualified
        // type name.

        pin_ptr<const wchar_t> strManagedTypeName = PtrToStringChars(pType->Name);

        STRING *pTypeName = m_pCompiler->AddString(strManagedTypeName);
        
        if (pType->IsGenericTypeDefinition)
        {
            // unmanagle the name for generics

            pTypeName = GetActualTypeNameFromEmittedTypeName(
                pTypeName,
                -1, // any arity
                m_pCompiler,
                NULL);
        }

        pSymbol = m_pSymbols->GetBadNamedRoot(
            pTypeName,
            NULL,
            DECLF_Public,
            BINDSPACE_IgnoreSymbol,
            ERRID_UndefinedType1,
            NULL,
            NULL);
    }

    return pSymbol;
}

BCSYM* ExternalTypeBuilder::GetSymbolForGenericType(BCSYM_Container* pContainer, gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pContainer, "[ExternalTypeBuilder::GetSymbolForGenericType] 'pContainer' parameter is null");
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetSymbolForGenericType] 'pType' parameter is null");

    int ParamMetaDataPosition = pType->GenericParameterPosition;

    while (pContainer && pContainer->IsType())
    {
        for (BCSYM_GenericParam *Param = pContainer->GetFirstGenericParam(); Param; Param = Param->GetNextParam())
        {
            if (Param->GetMetaDataPosition() == ParamMetaDataPosition)
            {
                return Param;
            }
        }

        pContainer = pContainer->GetContainer();
    }

    return NULL;
}

BCSYM* ExternalTypeBuilder::GetSymbolForGenericMethod(BCSYM_MethodDecl* pMethodDecl, gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pMethodDecl, "[ExternalTypeBuilder::GetGenericMethodTypeParam] 'pContainer' parameter is null");
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetSymbolForGenericType] 'pType' parameter is null");

    int ParamMetaDataPosition = pType->GenericParameterPosition;

    for (BCSYM_GenericParam *Param = pMethodDecl->GetFirstGenericParam(); Param; Param = Param->GetNextParam())
    {
        if (Param->GetMetaDataPosition() == ParamMetaDataPosition)
        {
            return Param;
        }
    }

    VSFAIL("Failed to find a type parameter during metadata importation.");
    return NULL;
}

BCSYM_Class* ExternalTypeBuilder::MakeClassForType(gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::MakeClassForType] 'pType' parameter is null");

    HRESULT hr = S_OK;
    STRING* pstrNamespaceName = NULL;
    STRING* pstrTypeName = NULL;

    gcroot<System::String^> FullName;
    DECLFLAGS DeclFlags = 0;
    BCSYM_Class* pClass = NULL;
    pin_ptr<const wchar_t> strFullName;
    mscorlib::_TypePtr pTypeCom;

    DeclFlags = DeclFlagsForType(pType);

    ASSERT(ShouldImportSymbol(DeclFlags), "[ExternalTypeBuilder::MakeClassForType] Passed in Type is marked non-public. Still pulling in.");

    FullName = pType->IsNested ? pType->Name : pType->FullName;

    strFullName = PtrToStringChars(FullName);
    SplitTypeName(m_pCompiler, strFullName, &pstrNamespaceName, &pstrTypeName);

    pClass = m_pSymbols->AllocClass(false);
    ASSERT(pClass, "[ExternalTypeBuilder::MakeClassForType] 'pClass' variable is null; AllocClass failed without throwing");
    IfFalseGo(pClass, E_UNEXPECTED);
    m_pSymbols->GetClass(NULL,
           pstrTypeName,
           pstrTypeName,
           pstrNamespaceName,
           m_pCompilerFile, //CompilerFile* pfile
           NULL, // BCSYM *ptypBase - retrieved above.
           NULL, // BCSYM_Implements *----lList
           DeclFlags,
           t_bad, // Underlying type for enums (unused otherwise).
           NULL, // BCSYM_Variable *pvarMe
           NULL, // SymbolList *psymlistChildren
           NULL, // SymbolList *psymlistUnbindableChildren
           NULL, // BCSYM_GenericParam *pGenericParameters
           NULL, // SymbolList *psymList - add class to this list.
           pClass->PClass());

    SetupGenericParameters(pClass, pType);
    SetupGenericParameterTypeConstraints(pClass->GetFirstGenericParam());

    if(pType->IsEnum)
    {
        //Mark as an enum if appropriate
        pClass->SetIsEnum(true);
    }
    // A struct is a Value Type and no NOT an Enum
    else if (pType->IsValueType)
    {
        pClass->SetIsStruct(true);
    }

    IfFailGo(ConvertSystemTypeToCom(pType, &pTypeCom));

    pClass->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pTypeCom));
    pClass->SetChildrenNotLoaded(true);
    pClass->SetBaseAndImplementsNotLoaded(true);

Error:
    return pClass;
}

BCSYM_Interface* ExternalTypeBuilder::MakeInterfaceForType(gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::MakeClassForType] 'pType' parameter is null");

    HRESULT hr = S_OK;
    STRING* pstrNamespaceName = NULL;
    STRING* pstrTypeName = NULL;

    gcroot<System::String^> FullName;
    DECLFLAGS DeclFlags = 0;
    BCSYM_Interface* pInterface = NULL;
    pin_ptr<const wchar_t> strFullName;
    mscorlib::_TypePtr pTypeCom;

    FullName = pType->FullName;
    DeclFlags = DeclFlagsForType(pType);

    ASSERT(ShouldImportSymbol(DeclFlags), "[ExternalTypeBuilder::MakeInterfaceForType] Passed in Type is marked non-public. Still pulling in.");

    strFullName = PtrToStringChars(FullName);
    ASSERT(strFullName, "[ExternalTypeBuilder::MakeInterfaceForType] 'strFullName' variable is null");
    IfFalseGo(strFullName, E_UNEXPECTED);
    SplitTypeName(m_pCompiler, strFullName, &pstrNamespaceName, &pstrTypeName);

    pInterface = m_pSymbols->GetInterface(
        NULL, // Location* location
        pstrTypeName,
        pstrTypeName,
        pstrNamespaceName,
        m_pCompilerFile,
        DeclFlags,
        NULL, // BCSYM_Implements *----lList
        NULL, // SymbolList* psymlistChildren
        NULL, // SymbolList* psymlistUnBindableChildren
        NULL, // BCSYM_GenericParam* pGenericParameters
        NULL // SymbolList* psymlist
        //NULL // BCSYM_Interface* pAllocatedMemory = 0
        );

    SetupGenericParameters(pInterface, pType);
    SetupGenericParameterTypeConstraints(pInterface->GetFirstGenericParam());

    IfFailGo(ConvertSystemTypeToCom(pType, &pTypeCom));

    pInterface->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pTypeCom));
    pInterface->SetChildrenNotLoaded(true);
    pInterface->SetBaseAndImplementsNotLoaded(true);

Error:
    return pInterface;
}

BCSYM* ExternalTypeBuilder::GetBaseForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pContainer, "[ExternalTypeBuilder::GetBaseForType] 'pContainer' parameter is invalid");
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetBaseForType] 'pType' parameter is invalid");
    BCSYM* pSymbol = NULL;
    gcroot<System::Type^> pBaseType;
    
    pBaseType = pType->BaseType;
    ASSERT(pBaseType, "[ExternalTypeBuilder::GetBaseForType] 'pBaseType' retrieval failed (should always retrieve valid value)");

    pSymbol = GetSymbolForType(pBaseType, pContainer, NULL);
    
    return pSymbol;
}

BCSYM_Implements* ExternalTypeBuilder::GetImplementsForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType)
{
    VerifyParamCondRetNull(pContainer, "[ExternalTypeBuilder::GetImplementsForType] 'pContainer' parameter is invalid");
    VerifyParamCondRetNull(pType, "[ExternalTypeBuilder::GetImplementsForType] 'pType' parameter is invalid");
    BCSYM_Implements* pList = NULL;

    // GetInterfaces() returns all interfaces defined directly on this class or inherited from the base class.
    // This currently provides the right behavior. Otherwise will need to use GetInterfaceMap to establish
    // where the interface implementation methods are actually defined -- 
    // InterfaceMapping.TargetMethods[0].DeclaringType == pType
    gcroot<array<System::Type^>^> interfaces = pType->GetInterfaces();
    ASSERT(interfaces, "[ExternalTypeBuilder::GetImplementsForType] System::Type::GetInterfaces returned null; expected to return empty array");

    if (interfaces == nullptr || interfaces->Length <= 0)
        return NULL;

    for (int i = 0; i < interfaces->Length; i++)
    {
        BCSYM* pSymbol = NULL;
        gcroot<System::Type^> pInterface;
        
        pInterface = interfaces[i];
        ASSERT(pInterface, "[ExternalTypeBuilder::GetImplementsForType] 'interfaces' array returned null member");
        ASSERT(pInterface->IsInterface, "[ExternalTypeBuilder::GetImplementsForType] 'interface' variable is NOT an interface");

        pSymbol = GetSymbolForType(pInterface, pContainer, NULL);
        ASSERT(pSymbol && pSymbol->IsInterface(), "[ExternalTypeBuilder::GetImplementsForType] 'pSymbol' variable: pInterface could not be mapped to BCSYM*");
        if (pSymbol && pSymbol->IsInterface())
        {
            BCSYM_NamedRoot* pNR = pSymbol->PNamedRoot();

            //Passing in pSymbol in order to pick up generic bindings, but need to use the NamedRoot in order to get the name.
            m_pSymbols->GetImplements(
                NULL, // Location* ploc
                pNR->GetName(),
                pSymbol,
                &pList
                );
        }
    }

    return pList;    
}

HRESULT ExternalTypeBuilder::GetChildrenForType(BCSYM_Container* pContainer, gcroot<System::Type^> pType, SymbolList* pSymbolList, Vtypes* pvtUnderlyingType)
{
    HRESULT hr = S_OK;
    WCHAR* pwszDefaultMember = NULL;

    VerifyInPtr(pContainer, "[ExternalTypeBuilder::GetChildrenForType] 'pContainer' parameter is null");
    VerifyInCLRPtr(pType, "[ExternalTypeBuilder::GetChildrenForType] 'pType' parameter is null");
    VerifyInPtr(pSymbolList, "[ExternalTypeBuilder::GetChildrenForType] 'pSymbolList' parameter is null");
    VerifyOutPtr(pvtUnderlyingType, "[ExternalTypeBuilder::GetChildrenForType] 'pvtUnderlyingType' parameter is null");

    //Flags: All public methods (static or instance methods) that are declared explicitly on this class.
    // Inherited members should NOT be returned in this list.
    BindingFlags bindingFlags = BindingFlags::Public | BindingFlags::Static | BindingFlags::Instance | BindingFlags::DeclaredOnly;
    gcroot<array<MemberInfo^,1>^> members;

    *pvtUnderlyingType = t_i4;

    HandleSymHashTable dhtAccessors(m_pFactory->GetHostedAllocator()->GetNorlsAllocator());

    members = pType->GetMethods(bindingFlags);
    IfFailGo(HandleTypeChildrenArray(members, pContainer, dhtAccessors, pSymbolList, pvtUnderlyingType, NULL));

    members = pType->GetFields(bindingFlags);
    IfFailGo(HandleTypeChildrenArray(members, pContainer, dhtAccessors, pSymbolList, pvtUnderlyingType, NULL));

    pContainer->GetPWellKnownAttrVals()->GetDefaultMemberData(&pwszDefaultMember);
    members = pType->GetProperties(bindingFlags);
    IfFailGo(HandleTypeChildrenArray(members, pContainer, dhtAccessors, pSymbolList, pvtUnderlyingType, pwszDefaultMember));

    members = pType->GetConstructors(bindingFlags);
    IfFailGo(HandleTypeChildrenArray(members, pContainer, dhtAccessors, pSymbolList, pvtUnderlyingType, NULL));

    members = pType->GetEvents(bindingFlags);
    IfFailGo(HandleTypeChildrenArray(members, pContainer, dhtAccessors, pSymbolList, pvtUnderlyingType, NULL));

    IfFailGo(FinishChildren(pContainer, pType, pSymbolList));

Error:
    return hr;
}

HRESULT ExternalTypeBuilder::FinishChildren(BCSYM_Container* pContainer, gcroot<System::Type^> pType, SymbolList * pSymbolList)
{
    HRESULT hr = S_OK;

    VerifyInPtr(pContainer, "[ExternalTypeBuilder::GetChildrenForType] 'pContainer' parameter is null");
    VerifyInCLRPtr(pType, "[ExternalTypeBuilder::GetChildrenForType] 'pType' parameter is null");
    VerifyInPtr(pSymbolList, "[ExternalTypeBuilder::GetChildrenForType] 'pSymbolList' parameter is null");

    DynamicArray<BCSYM_MethodDecl*> DeferredOperators;
    SymbolList UnBindableSymList;
    SymbolList BindableSymList;
    SymbolList BadSymList;
    BCSYM_NamedRoot* pnamedCurrent = NULL;
    BCSYM_NamedRoot* pnamedNext = NULL;

    for (pnamedCurrent = pSymbolList->GetFirst(); pnamedCurrent; pnamedCurrent = pnamedNext)
    {
        pnamedNext = pnamedCurrent->GetNextInSymbolList();

        if (pnamedCurrent->IsSpecialName() &&
            pnamedCurrent->IsMethodDecl() &&
            CompareNoCaseN(STRING_CONST(m_pCompiler, OperatorPrefix),
                pnamedCurrent->GetName(),
                StringPool::StringLength(STRING_CONST(m_pCompiler, OperatorPrefix))) == 0)
        {
            BCSYM_MethodDecl* CurrentMethod = pnamedCurrent->PMethodDecl();

            //An operator must be shared AND have a return type (ie, be a function)
            if (CurrentMethod->IsShared() && CurrentMethod->GetRawType())
            {
                UserDefinedOperators Operator = MapToUserDefinedOperator(
                    CurrentMethod->GetName(), CurrentMethod->GetParameterCount(), 
                    m_pCompiler);

                if (Operator != OperatorUNDEF)
                {
                    if (OperatorMAXVALID < Operator && Operator < OperatorMAX)
                    {
                        DeferredOperators.AddElement(CurrentMethod);
                    }
                    else
                    {
                        UnBindableSymList.AddToFront(MetaImport::CreateOperatorSymbol(
                            pContainer,
                            Operator,
                            CurrentMethod,
                            m_pSymbols,
                            m_pCompiler
                            ));
                    }
                }
            }
        }

        if (pnamedCurrent->GetBindingSpace() == BINDSPACE_IgnoreSymbol)
        {
            UnBindableSymList.AddToFront(pnamedCurrent);
        }
        else if (pnamedCurrent->IsGenericBadNamedRoot())
        {
            BadSymList.AddToEnd(pnamedCurrent);
        }
        else
        {
            BindableSymList.AddToFront(pnamedCurrent);
        }
    }

    BCSYM_Container* pCurrentContainer, *pNextContainer;

    for (pCurrentContainer = *pContainer->GetNestedTypeList();
        pCurrentContainer;
        pCurrentContainer = pNextContainer)
    {
        pNextContainer = pCurrentContainer->GetNextInSymbolList()->PContainer ();
        BindableSymList.AddToFront(pCurrentContainer);
    }

    BCSYM_Hash *ChildrenHash = m_pSymbols->GetHashTable(
        pContainer->PNamedRoot()->GetNameSpace(),
        pContainer,
        true,
        BindableSymList.GetCount() + BadSymList.GetCount(),
        &BadSymList
        );

    Symbols::AddSymbolListToHash(ChildrenHash, &BindableSymList, true);

    BCSYM_Hash *UnBindableChildrenHash = m_pSymbols->GetHashTable(
        pContainer->PNamedRoot()->GetNameSpace(),
        pContainer,
        true,
        0,
        &UnBindableSymList
        );

    pContainer->SetHashes(ChildrenHash, UnBindableChildrenHash);

    for (ULONG iMember = 0; iMember < DeferredOperators.Count(); iMember++)
    {
        BCSYM_MethodDecl* Deferred = DeferredOperators.Element(iMember);
        UserDefinedOperators Mapped;

        switch(MapToUserDefinedOperator(Deferred->GetName(), Deferred->GetParameterCount(),
            m_pCompiler))
        {
        case OperatorNot2:
            Mapped = OperatorNot;
            break;
        case OperatorOr2:
            Mapped = OperatorOr;
            break;
        case OperatorAnd2:
            Mapped = OperatorAnd;
            break;
        case OperatorShiftLeft2:
            Mapped = OperatorShiftLeft;
            break;
        case OperatorShiftRight2:
            Mapped = OperatorShiftRight;
            break;
        default:
            VSFAIL("[ExternalTypebuilder::FinishChildren] Unknown operator mapping");
            continue;
        }

        BCSYM_NamedRoot* ExistingOperator = UnBindableChildrenHash->SimpleBind(
            m_pCompiler->OperatorToString(Mapped));

        while (ExistingOperator)
        {
            if (ExistingOperator->IsUserDefinedOperator())
            {
                unsigned CompareFlags =
                    BCSYM::CompareProcs(
                        ExistingOperator->PUserDefinedOperator()->GetOperatorMethod(),
                        (GenericBinding*)NULL,
                        Deferred,
                        (GenericBinding*)NULL,
                        NULL);

                if (!(CompareFlags & EQ_Shape))
                {
                    goto continuefor;
                }
            }

            ExistingOperator = ExistingOperator->GetNextBound();
        }

        Symbols::AddSymbolToHash(
            UnBindableChildrenHash,
            MetaImport::CreateOperatorSymbol(
                pContainer,
                Mapped,
                Deferred,
                m_pSymbols,
                m_pCompiler
                ),
            true,
            false,
            false);

continuefor:;
    }

    return hr;
}

HRESULT ExternalTypeBuilder::HandleTypeChildrenArray(gcroot<array<MemberInfo^,1>^> members, BCSYM_Container* pContainer, 
                                                     HandleSymHashTable& dhtAccessors, SymbolList* pSymbolList, Vtypes* pvtUnderlyingType,
                                                     const WCHAR* pwszDefaultProperty)
{
    HRESULT hr = S_OK;
    bool FirstDefaultProperty = true;

    VerifyInPtr(pContainer, "[ExternalTypeBuilder::HandleTypeChildrenArray] 'pContainer' parameter is invalid.");
    VerifyInPtr(pSymbolList, "[ExternalTypeBuilder::HandleTypeChildrenArray] 'pSymbolList' parameter is invalid.");
    VerifyOutPtr(pvtUnderlyingType, "[ExternalTypeBuilder::HandleTypeChildrenArray] 'pvtUnderlyingType' parameter is invalid.");

    IfFalseGo(members != nullptr && members->Length > 0, S_OK);

    for (int i = 0; i < members->Length; i++)
    {
        BCSYM_NamedRoot* pSymbol = NULL;
        gcroot<MemberInfo^> member = members[i];
        pSymbol = GetSymbolForTypeMember(pContainer, member, dhtAccessors, pvtUnderlyingType, pwszDefaultProperty, FirstDefaultProperty);
        //pSymbol = NULL is valid if the member is unrecognized, or NestedType
        if (pSymbol != NULL)
            pSymbolList->AddToFront(pSymbol);
    }

Error:
    return hr;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeMember(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& htAccessors, Vtypes* pvtUnderlyingType, const WCHAR* pwszDefaultProperty, bool& FirstDefaultProperty)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeMember] 'pMember' parameter is null");
    
    MemberTypes mt = pMember->MemberType;
#if DEBUG
    gcroot<System::String^> strName = pMember->Name;
#endif

    switch (mt)
    {
    case System::Reflection::MemberTypes::Method:
    case System::Reflection::MemberTypes::Constructor:
        return GetSymbolForTypeMethod(pContainer, pMember, htAccessors);
    case System::Reflection::MemberTypes::Field:
        return GetSymbolForTypeField(pContainer, pMember, pvtUnderlyingType);
    case System::Reflection::MemberTypes::Property:
        return GetSymbolForTypeProperty(pContainer, pMember, htAccessors, pwszDefaultProperty, FirstDefaultProperty);
    case System::Reflection::MemberTypes::Event:
        return GetSymbolForTypeEvent(pContainer, pMember, htAccessors);
    default:
        VSFAIL("[ExternalTypeBuilder::GetSymbolForTypeMember] No current support for importing this MemberType");
    }

    return NULL;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeMethod(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& htAccessors)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeMethod] 'pMember' parameter is null");
    
    DECLFLAGS DeclFlag = 0;
    gcroot<MethodBase^> pMethodBase;
    gcroot<MethodInfo^> pMethod;

    BCSYM_NamedRoot* pSymbol = NULL;
    BCSYM_MethodDecl* pMethodDecl = NULL;
    BCSYM* pReturnType = NULL;
    BCSYM_Param* pParamList = NULL;
    pin_ptr<const wchar_t> pChMethodName = nullptr;
    STRING* pstrName = NULL;
    System::Reflection::MethodAttributes attributes;
    IUnknown* pUnkMethod = NULL;

    pMethodBase = dynamic_cast<System::Reflection::MethodBase^>(static_cast<MemberInfo^>(pMember));
    ASSERT(pMethodBase, "[ExternalTypeBuilder::GetSymbolForTypeMethod] a MemberInfo^ cannot be cast to MethodBase, despite MemberType");

    DeclFlag = DeclFlagsForMethod(pMethodBase, &attributes);
    IfFalseGo(ShouldImportSymbol(DeclFlag), S_OK);

    ASSERT(pMethodBase->Name, "[ExternalTypeBuilder::GetSymbolForTypeMethod] pMethodBase->Name is nullptr");
    pChMethodName = PtrToStringChars(pMethodBase->Name);
    ASSERT(pChMethodName, "[ExternalTypeBuilder::GetSymbolForTypeMethod] PtrToStringChars(pMethodBase->Name) returned nullptr unexpectedly");
    pstrName = m_pCompiler->AddString(pChMethodName);
    ASSERT(pstrName, "[ExternalTypeBuilder::GetSymbolForTypeMethod] Compiler::AddString returned null unexpectedly");

    if ((attributes & MethodAttributes::SpecialName) == MethodAttributes::SpecialName)
    {
        //Taken from MetaImport::LoadMethodDef, MetaImport.cpp ln. 3613
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, Constructor), pstrName) ||
            StringPool::IsEqual(STRING_CONST(m_pCompiler, SharedConstructor), pstrName))
        {
            // Constructor
            DeclFlag |= DECLF_Constructor | DECLF_SpecialName;
        }
        // Mark that it's got a special name unless it's a let, which
        // really isn't special at all.
        else if (CompareNoCaseN(STRING_CONST(m_pCompiler, LetPrefix), pstrName, StringPool::StringLength(STRING_CONST(m_pCompiler, LetPrefix))) != 0)
        {
            DeclFlag |= DECLF_SpecialName;
        }
    }

    pMethodDecl = m_pSymbols->AllocMethodDecl(false);

    pMethod = dynamic_cast<MethodInfo^>(static_cast<MethodBase^>(pMethodBase));
    if (pMethod)
    {
        SetupGenericParameters(pMethodDecl, pMethod);
        SetupGenericParameterTypeConstraints(pMethodDecl->GetFirstGenericParam());
    }

    IfFailGo(GetParametersForTypeMethod(pMethodBase, &pReturnType, &pParamList, pContainer, pMethodDecl));
    
    ASSERT(pMethodDecl, "[ExternalTypeBuilder::GetSymbolForTypeMethod] AllocMethodDecl returned null without throwing");
    m_pSymbols->GetProc(NULL,
          pstrName,
          pstrName,
          (CodeBlockLocation*) NULL,
          (CodeBlockLocation*) NULL,
          DeclFlag,
          pReturnType,
          pParamList,
          NULL, //BCSYM* pparamReturn
          NULL, //STRING* pstrLibName
          NULL, //STRING* pstrAliasName
          SYNTH_None,
          NULL, //BCSYM_GenericParam* pGenericParameters
          NULL, // SymbolList* psymbolList
          pMethodDecl->PProc());

    IfFailGo(ConvertSystemMethodBaseToCom(pMethodBase, &pUnkMethod));
    ASSERT(pUnkMethod, "[ExternalTypeBuilder::GetSymbolForTypeMethod] ConvertManagedObjectToCOM succeeded, but returned NULL");
    pMethodDecl->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pUnkMethod));

    if (DeclFlag & DECLF_SpecialName)
    {
        gcroot<System::RuntimeMethodHandle^> handle = pMethodBase->MethodHandle;
        __int64 handleCode = handle->Value.ToInt64();
        htAccessors.SetValue(handleCode, pMethodDecl);
    }

    IfFailGo(LoadCustomAttributes(pMember, pMethodDecl));

    pSymbol = pMethodDecl;

Error:
    return pSymbol;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeField(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, Vtypes* pvtUnderlyingType)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeField] 'pMember' parameter is null");

    gcroot<System::Reflection::FieldInfo^> pField;
    DECLFLAGS DeclFlag = 0;
    BCSYM_NamedRoot* pSymbol = NULL;
    BCSYM_NamedRoot* pFieldDecl = NULL;
    pin_ptr<const wchar_t> pChFieldName = nullptr;
    STRING* pstrName = NULL;
    IUnknown* pUnkField = NULL;
    BCSYM* pFieldType = NULL;
    VARIABLEKIND kind = VAR_Member;
    BCSYM_Expression* pExpression = NULL;
    
    pField = dynamic_cast<System::Reflection::FieldInfo^>(static_cast<MemberInfo^>(pMember));
    ASSERT(pField, "[ExternalTypeBuilder::GetSymbolForTypeField] a MemberInfo^ cannot be cast to FieldInfo, despite MemberType");

    DeclFlag = DeclFlagsForField(pField);
    IfFalseGo(ShouldImportSymbol(DeclFlag), S_OK);

    ASSERT(pField->Name, "[ExternalTypeBuilder::GetSymbolForTypeField] pField->Name is nullptr");
    pChFieldName = PtrToStringChars(pField->Name);
    ASSERT(pChFieldName, "[ExternalTypeBuilder::GetSymbolForTypeField] PtrToStringChars(pField->Name) returned nullptr unexpectedly");
    pstrName = m_pCompiler->AddString(pChFieldName);
    ASSERT(pstrName, "[ExternalTypeBuilder::GetSymbolForTypeField] Compiler::AddString returned null unexpectedly");

    ASSERT(pField->FieldType, "[ExternalTypeBuilder::GetSymbolForTypeField] FieldInfo::FieldType is not defined");
    pFieldType = GetSymbolForType(pField->FieldType, pContainer, NULL);

    // Handle special fields
    if (DeclFlag & DECLF_SpecialName && pContainer->IsEnum())
    {
        // Value field
        ASSERT(StringPool::IsEqual(pstrName, STRING_CONST(m_pCompiler, EnumValueMember)), "[ExternalTypeBuilder::GetSymbolForTypeField] Bad enum member.");

        *pvtUnderlyingType = pFieldType->GetVtype();
    }

    if(pField->IsLiteral)
    {
        kind = VAR_Const;
        pExpression = GetSymbolForConstantExpression(pField->GetValue(nullptr), pField->FieldType);
        pFieldDecl = m_pSymbols->AllocVariable(false, true);
    }
    else
    {
        pFieldDecl = m_pSymbols->AllocVariable(false, false);
    }

    
    ASSERT(pFieldDecl, "[ExternalTypeBuilder::GetSymbolForTypeField] AllocVariable returned NULL without throwing");
    m_pSymbols->GetVariable(
        NULL, //const Location *ploc
        pstrName,
        pstrName,
        DeclFlag,
        kind,
        pFieldType,
        pExpression,
        NULL, //SymbolList* psymbolList
        pFieldDecl->PVariable());

    IfFailGo((ConvertManagedObjectToCOM<System::Reflection::FieldInfo, IUnknown>(pField, &pUnkField)));
    ASSERT(pUnkField, "[ExternalTypeBuilder::GetSymbolForTypeField] ConvertManagedObjectToCOM succeeded, but returned NULL");
    pFieldDecl->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pUnkField));

    IfFailGo(LoadCustomAttributes(pMember, pFieldDecl));

    pSymbol = pFieldDecl;

Error:
    return pSymbol;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeNestedType(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] 'pMember' parameter is null");

    gcroot<System::Type^> pNestedType;
    DECLFLAGS DeclFlag = 0;
    BCSYM* pSymbol = NULL;
    pin_ptr<const wchar_t> pChNamespace = nullptr;
    STRING* pstrNamespace = NULL;
    IUnknown* pUnkNT = NULL;
    BCSYM_Container** ppNestedTypeList = NULL;
    BCSYM_Container* pSymbolContainer = NULL;
    
    pNestedType = dynamic_cast<System::Type^>(static_cast<MemberInfo^>(pMember));
    ASSERT(pNestedType, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] a MemberInfo^ cannot be cast to Type^, despite MemberType");

    DeclFlag = DeclFlagsForType(pNestedType);
    IfFalseGo(ShouldImportSymbol(DeclFlag), S_OK);

    pSymbol = this->GetSymbolForTypeDefinition(pNestedType, pContainer, NULL);
    ASSERT(pSymbol, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] 'pSymbol' returned for GetSymbolForTypeDefinition is null");
    IfFalseGo(pSymbol, E_UNEXPECTED);

    ASSERT(pSymbol->GetExternalSymbol(), "[ExternalTypeBuilder::GetSymbolForTypeNestedType] 'pSymbol' returned for GetSymbolForTypeDefinition has no external link");
    ASSERT(pSymbol->IsContainer(), "[ExternalTypeBuilder::GetSymbolForTypeNestedType] 'pSymbol' returned for GetSymbolForTypeDefinition is not Container");
    IfFalseGo(pSymbol->IsContainer(), E_UNEXPECTED);
    
    pSymbolContainer = pSymbol->PContainer();

    if (pNestedType->Namespace && pNestedType->Namespace->Length > 0)
    {
        //The namespace of a NestedType isn't set properly.
        ASSERT(pSymbol->PNamedRoot(), "[ExternalTypeBuilder::GetSymbolForTypeNestedType] 'pSymbol' is a Container, but is not a NamedRoot");
        ASSERT(pNestedType->Namespace, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] pNestedType->Namespace is nullptr");
        pChNamespace = PtrToStringChars(pNestedType->Namespace);
        ASSERT(pChNamespace, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] PtrToStringChars(pNestedType->Namespace) returned nullptr unexpectedly");
        pstrNamespace = m_pCompiler->AddString(pChNamespace);
        ASSERT(pstrNamespace, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] Compiler::AddString returned null unexpectedly");
        pSymbolContainer->SetNameSpace(pstrNamespace);        
    }
    
    ppNestedTypeList = pContainer->GetNestedTypeList();
    
    Symbols::SetNext(pSymbolContainer, *ppNestedTypeList);
    *ppNestedTypeList = pSymbolContainer;

    Symbols::SetParent(pSymbolContainer, pContainer);

Error:
    //Nested Types should NOT be added to the standard symbol list.
    ASSERT(SUCCEEDED(hr) ? pSymbol : !pSymbol, "[ExternalTypeBuilder::GetSymbolForTypeNestedType] Method success state does not correspond to Symbol.");
    return NULL;
}

BCSYM_Proc* ExternalTypeBuilder::GetSymbolForAccessor(gcroot<System::Reflection::MethodInfo^> pMethod, HandleSymHashTable& htAccessors)
{
    BCSYM_NamedRoot* pSymbol = NULL;
    gcroot<System::RuntimeMethodHandle^> handle;
    __int64 handleCode = 0;

    VerifyParamCondRetNull(pMethod, "[ExternalTypeBuilder::GetSymbolForAccessor] 'pMethod' parameter is null");

    handle = pMethod->MethodHandle;
    handleCode = handle->Value.ToInt64();
    pSymbol = htAccessors.GetValueOrDefault(handleCode, NULL);

    ASSERT(!pSymbol || pSymbol->IsProc(), "[ExternalTypeBuilder::GetSymbolForAccessor] 'pSymbol' variable should either be NULL or a Proc");
    return pSymbol ? (pSymbol->IsProc() ? pSymbol->PProc() : NULL) : NULL;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeProperty(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& htAccessors, const WCHAR* pwszDefaultProperty, bool& FirstDefaultProperty)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeProperty] 'pMember' parameter is null");

    gcroot<System::Reflection::PropertyInfo^> pProperty;
    DECLFLAGS DeclFlag = 0;
    BCSYM_NamedRoot* pSymbol = NULL;
    BCSYM_NamedRoot* pPropDecl = NULL;
    pin_ptr<const wchar_t> pChPropName = nullptr;
    STRING* pstrName = NULL;
    IUnknown* pUnkProp = NULL;

    gcroot<System::Reflection::MethodInfo^> pSetMethod;
    gcroot<System::Reflection::MethodInfo^> pGetMethod;
    BCSYM_Proc* pSetProc = NULL;
    BCSYM_Proc* pGetProc = NULL;
    BCSYM_Param* pFirstParameter = NULL;

    BCSYM* pPropType = NULL;

    //Setting to both values. Turning off as Set or Get is encountered.
    DECLFLAGS PropBlockFlag = DECLF_ReadOnly | DECLF_WriteOnly;
    
    pProperty = dynamic_cast<System::Reflection::PropertyInfo^>(static_cast<MemberInfo^>(pMember));
    ASSERT(pProperty, "[ExternalTypeBuilder::GetSymbolForTypeProperty] a MemberInfo^ cannot be cast to PropertyInfo, despite MemberType");

    ASSERT(pProperty->Name, "[ExternalTypeBuilder::GetSymbolForTypeProperty] pField->Name is nullptr");
    pChPropName = PtrToStringChars(pProperty->Name);
    ASSERT(pChPropName, "[ExternalTypeBuilder::GetSymbolForTypeProperty] PtrToStringChars(pField->Name) returned nullptr unexpectedly");
    pstrName = m_pCompiler->AddString(pChPropName);
    ASSERT(pstrName, "[ExternalTypeBuilder::GetSymbolForTypeProperty] Compiler::AddString returned null unexpectedly");

    ASSERT(pProperty->PropertyType, "[ExternalTypeBuilder::GetSymbolForTypeProperty] PropertyInfo::PropertyType is not defined");
    pPropType = GetSymbolForType(pProperty->PropertyType, pContainer, NULL);

    pSetMethod = pProperty->GetSetMethod();
    pGetMethod = pProperty->GetGetMethod();

    if (pSetMethod)
    {
        pSetProc = GetSymbolForAccessor(pSetMethod, htAccessors);
        ASSERT(pSetProc, "[ExternalTypeBuilder::GetSymbolForTypeProperty] 'pSetProc' variable is NULL when 'pSetMethod' token is valid");
        Symbols::SetIsPropertySet(pSetProc);
        //cf. MetaImport::GetProperties: Property declaration flags defaults to the Get if present, over the Set flags. Hence,
        // DeclFlag is set here and then overwritten by pGetProc below if appropriate.
        DeclFlag = pSetProc->GetDeclFlags();
        pFirstParameter = pSetProc->GetFirstParam();
        PropBlockFlag &= ~(DECLF_ReadOnly);
    }
    
    if (pGetMethod)
    {
        pGetProc = GetSymbolForAccessor(pGetMethod, htAccessors);
        ASSERT(pGetProc, "[ExternalTypeBuilder::GetSymbolForTypeProperty] 'pGetProc' variable is NULL when 'pGetMethod' token is valid");
        Symbols::SetIsPropertyGet(pGetProc);
        //cf. MetaImport::GetProperties: Property declaration flags defaults to the Get if present, over the Set flags. Hence,
        // Overwrite previously set DeclFlag.
        DeclFlag = pGetProc->GetDeclFlags();
        pFirstParameter = pGetProc->GetFirstParam();
        PropBlockFlag &= ~(DECLF_ReadOnly);
    }

    ASSERT(pSetMethod || pGetMethod, "[ExternalTypeBuilder::GetSymbolForTypeProperty] Neither pSetMethod or pGetMethod is available");
    IfTrueGo(!pGetProc && !pSetProc, S_OK);

    //Turn off PropGet, PropSet, and SpecialName flags.
    DeclFlag &= ~(DECLF_PropGet | DECLF_PropSet | DECLF_SpecialName);
    //The property is a function, and has a return value.
    DeclFlag |= DECLF_HasRetval | DECLF_Function;

    if (pGetProc && pSetProc)
    {
        //Modified from MetaImport::GetProperties
        DeclFlag &= ~ DECLF_AccessFlags;
        if (pSetProc->GetAccess() < pGetProc->GetAccess())
        {
            DeclFlag |= pGetProc->GetDeclFlags() & DECLF_AccessFlags;
        }
        else
        {
            DeclFlag |= pSetProc->GetDeclFlags() & DECLF_AccessFlags;
        }
    }

    pPropDecl = m_pSymbols->AllocProperty(false); 
    ASSERT(pPropDecl, "[ExternalTypeBuilder::GetSymbolForTypeProperty] AllocProperty returned NULL without throwing");

    m_pSymbols->GetProc(
        NULL, // Location* location
        pstrName,
        pstrName,
        NULL, // CodeBlockLocation *pCodeBlock
        NULL, // CodeBlockLocation *pProcBlock
        DeclFlag,
        pPropType,
        pFirstParameter,
        NULL, // BCSYM_Param* pparamReturn
        NULL, // STRING* pstrLibName
        NULL, // STRING* pstrAliasName
        SYNTH_None,
        NULL, // BCSYM* pGenericParams
        NULL, // SymbolList* psymbollist
        pPropDecl->PProc()
        );
        

    m_pSymbols->GetProperty(
        NULL, // Location *Location
        pstrName,
        PropBlockFlag, // DECLFLAGS PropertyFlags -- different properties than the GetProc above.
        pGetProc, // BCSYM_Proc* GetProperty
        pSetProc, // BCSYM_Proc* SetProperty
        pPropDecl->PProperty(),
        NULL
        );

    Symbols::MakeUnbindable(pGetProc);
    Symbols::MakeUnbindable(pSetProc);

    IfFailGo((ConvertManagedObjectToCOM<System::Reflection::PropertyInfo, IUnknown>(pProperty, &pUnkProp)));
    ASSERT(pUnkProp, "[ExternalTypeBuilder::GetSymbolForTypeProperty] ConvertManagedObjectToCOM succeeded, but returned NULL");
    pPropDecl->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pUnkProp));

    IfFailGo(LoadCustomAttributes(pMember, pPropDecl));

    // Mark this as being the default, if necessary.
    if (pwszDefaultProperty && CompareNoCase(pstrName, pwszDefaultProperty) == 0)
    {
        Symbols::SetIsDefault(pPropDecl->PProperty());

        // Only point to the first one -- the rest will be overloads
        if (FirstDefaultProperty)
        {
            // we should tell the container that it has a default
            Symbols::SetDefaultProperty(pContainer, pPropDecl->PProperty());
            FirstDefaultProperty = false;
        }
    }

    pSymbol = pPropDecl;

Error:
    return pSymbol;
}

BCSYM_NamedRoot* ExternalTypeBuilder::GetSymbolForTypeEvent(BCSYM_Container* pContainer, gcroot<System::Reflection::MemberInfo^> pMember, HandleSymHashTable& htAccessors)
{
    HRESULT hr = S_OK;

    VerifyParamCondRetNull(pMember, "[ExternalTypeBuilder::GetSymbolForTypeProperty] 'pMember' parameter is null");

    gcroot<System::Reflection::EventInfo^> pEvent;
    DECLFLAGS DeclFlag = 0;
    BCSYM_NamedRoot* pSymbol = NULL;
    BCSYM_EventDecl* pEventDecl = NULL;
    pin_ptr<const wchar_t> pChEventName = nullptr;
    STRING* pstrName = NULL;
    IUnknown* pUnkEvent = NULL;

    gcroot<System::Reflection::MethodInfo^> pAddMethod;
    gcroot<System::Reflection::MethodInfo^> pRemoveMethod;
    gcroot<System::Reflection::MethodInfo^> pRaiseMethod;
    BCSYM_Proc* pAddProc = NULL;
    BCSYM_Proc* pRemoveProc = NULL;
    BCSYM_Proc* pRaiseProc = NULL;
    BCSYM_Param* pFirstParameter = NULL;

    BCSYM* pDelegateType = NULL;
    BCSYM_Class* pDelegateClass = NULL;
    BCSYM_NamedRoot* pInvokeMethod = NULL;
    
    pEvent = dynamic_cast<System::Reflection::EventInfo^>(static_cast<MemberInfo^>(pMember));
    ASSERT(pEvent, "[ExternalTypeBuilder::GetSymbolForTypeEvent] a MemberInfo^ cannot be cast to EventInfo, despite MemberType");

    ASSERT(pEvent->Name, "[ExternalTypeBuilder::GetSymbolForTypeEvent] pEvent->Name is nullptr");
    pChEventName = PtrToStringChars(pEvent->Name);
    ASSERT(pChEventName, "[ExternalTypeBuilder::GetSymbolForTypeEvent] PtrToStringChars(pEvent->Name) returned nullptr unexpectedly");
    pstrName = m_pCompiler->AddString(pChEventName);
    ASSERT(pstrName, "[ExternalTypeBuilder::GetSymbolForTypeEvent] Compiler::AddString returned null unexpectedly");

    ASSERT(pEvent->EventHandlerType, "[ExternalTypeBuilder::GetSymbolForTypeEvent] EventInfo::EventHandlerType is not defined");
    pDelegateType = GetSymbolForType(pEvent->EventHandlerType, pContainer, NULL);

    ASSERT(pDelegateType, "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pDelegateType' variable: DelegateType is not defined");
    IfFalseGo(pDelegateType, S_OK);

    pAddMethod = pEvent->GetAddMethod();
    pRemoveMethod = pEvent->GetRemoveMethod();
    pRaiseMethod = pEvent->GetRaiseMethod();

    if (pAddMethod)
    {
        pAddProc = GetSymbolForAccessor(pAddMethod, htAccessors);
        ASSERT(pAddProc, "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pAddProc' variable is NULL when 'pAddMethod' token is valid");
        ASSERT(pAddProc->IsProc(), "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pAddProc' variable is not PProc");
    }
    
    if (pRemoveMethod)
    {
        pRemoveProc = GetSymbolForAccessor(pRemoveMethod, htAccessors);
        ASSERT(pRemoveProc, "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pRemoveProc' variable is NULL when 'pRemoveMethod' token is valid");
        ASSERT(pRemoveProc->IsProc(), "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pRemoveProc' variable is not PProc");
    }

    if (pRaiseMethod)
    {
        pRaiseProc = GetSymbolForAccessor(pRaiseMethod, htAccessors);
        ASSERT(pRaiseProc, "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pRaiseProc' variable is NULL when 'pRemoveMethod' token is valid");
        ASSERT(pRaiseProc->IsProc(), "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pRaiseProc' variable is not PProc");
    }

    ASSERT(pAddProc && pRemoveProc, "[ExternalTypeBuilder::GetSymbolForTypeEvent] Either pAddProc or pRemoveProc is available");
    IfTrueGo(!pAddProc || !pRemoveProc, S_OK);

    ASSERT(pDelegateType->IsClass(), "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pDelegateType' variable: DelegateType is NOT a class");
    IfFalseGo(pDelegateType->IsClass(), S_OK);
    
    pDelegateClass = pDelegateType->PClass();
    pDelegateClass->EnsureChildrenLoaded();

    pInvokeMethod = GetInvokeFromDelegate(pDelegateClass, m_pCompiler);
    ASSERT(pInvokeMethod, "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pInvokeMethod' variable: Could not get InvokeMethod from Delegate");
    IfFalseGo(pInvokeMethod, S_OK);

    ASSERT(pAddProc->GetAccess() == pRemoveProc->GetAccess(), "[ExternalTypeBuilder::GetSymbolForTypeEvent] 'pAddProc' and 'pRemoveProc' have different Access restrictions");

    DeclFlag = DECLF_Function;
    //Get access flags from pAddProc
    DeclFlag |= pAddProc->GetDeclFlags() & (DECLF_AccessFlags | DECLF_ShadowsKeywordUsed);
    //Get Shared (static) from pAddProc
    DeclFlag |= pAddProc->PProc()->IsShared() ? DECLF_Shared : 0;

    pEventDecl = m_pSymbols->AllocEventDecl(false); 
    ASSERT(pEventDecl, "[ExternalTypeBuilder::GetSymbolForTypeEvent] AllocEventDecl returned NULL without throwing");

    if (!pDelegateType->IsGenericBinding())
        pFirstParameter = pInvokeMethod->PProc()->GetFirstParam();

    m_pSymbols->GetProc(
        NULL, // Location* location
        pstrName,
        pstrName,
        NULL, // CodeBlockLocation *pCodeBlock
        NULL, // CodeBlockLocation *pProcBlock
        DeclFlag,
        pInvokeMethod->PProc()->GetType(),
        pFirstParameter,
        NULL, // BCSYM_Param* pparamReturn
        NULL, // STRING* pstrLibName
        NULL, // STRING* pstrAliasName
        SYNTH_None,
        NULL, // BCSYM* pGenericParams
        NULL, // SymbolList* psymbollist
        pEventDecl->PProc()
        );

    pEventDecl->SetDelegate(pDelegateType);
    pEventDecl->SetProcAdd(pAddProc);
    pEventDecl->SetProcRemove(pRemoveProc);
    pEventDecl->SetProcFire(pRaiseProc);

    if (pDelegateType->IsGenericBinding())
    {
        Bindable::CopyInvokeParamsToEvent(
            pEventDecl,
            pDelegateType,
            pInvokeMethod->PProc(),
            m_pFactory->GetHostedAllocator()->GetNorlsAllocator(),
            m_pSymbols
            );
    }

    Symbols::MakeUnbindable(pRemoveProc);
    Symbols::MakeUnbindable(pAddProc);
    Symbols::MakeUnbindable(pRaiseProc);

    pAddProc->SetEventThatCreatedSymbol(pEventDecl);
    pRemoveProc->SetEventThatCreatedSymbol(pEventDecl);

    if (pRaiseProc)
    {
        pRaiseProc->SetEventThatCreatedSymbol(pEventDecl);
    }

    IfFailGo((ConvertManagedObjectToCOM<System::Reflection::EventInfo, IUnknown>(pEvent, &pUnkEvent)));
    ASSERT(pUnkEvent, "[ExternalTypeBuilder::GetSymbolForTypeEvent] ConvertManagedObjectToCOM succeeded, but returned NULL");
    pEventDecl->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pUnkEvent));

    IfFailGo(LoadCustomAttributes(pMember, pEventDecl));

    pSymbol = pEventDecl;

Error:
    return pSymbol;
}

HRESULT ExternalTypeBuilder::GetParametersForTypeMethod(gcroot<System::Reflection::MethodBase^> pMethodBase, BCSYM** ppReturnType, BCSYM_Param** ppParamList, BCSYM_Container* pContainer, BCSYM_MethodDecl* pMethodDecl)
{
    HRESULT hr = S_OK;
    gcroot<MethodInfo^> pMethod;
    int index = 0;
    BCSYM_Param *pParamFirst =  NULL;
    BCSYM_Param *pParamLast = NULL;

    VerifyInCLRPtr(pMethodBase, "[ExternalTypeBuilder::GetParametersForTypeMethod] 'pMethod' parameter is null");
    VerifyOutPtr(ppReturnType, "[ExternalTypeBuilder::GetParametersForTypeMethod] 'ppReturnType' parameter is null");
    VerifyOutPtr(ppParamList, "[ExternalTypeBuilder::GetParametersForTypeMethod] 'ppParamList' parameter is null");

    *ppReturnType = NULL;
    *ppParamList = NULL;

    pMethod = dynamic_cast<MethodInfo^>(static_cast<MethodBase^>(pMethodBase));
    if (pMethod)
    {
        if (System::Void::typeid->Equals(pMethod->ReturnType))
        {
            // For void returning methods, need to make the return type NULL rather than
            // leaving it as System.Void. This is the same way metaimport deals with void
            // return types.
            *ppReturnType = NULL;
        }
        else
        {
            *ppReturnType = GetSymbolForType(pMethod->ReturnType, pContainer, pMethodDecl);
        }
    }
    else
    {
#if DEBUG
        gcroot<ConstructorInfo^> ctor = dynamic_cast<ConstructorInfo^>(static_cast<MethodBase^>(pMethodBase));
        ASSERT(ctor, "[ExternalTypeBuilder::GetParametersForTypeMethod] Assumption is a MethodBase that is not a MethodInfo must be a ctor.");
#endif
        *ppReturnType = NULL;
    }

    gcroot<array<ParameterInfo^, 1>^> params = pMethodBase->GetParameters();
    for (index = 0; index < params->Length; index++)
    {
        gcroot<ParameterInfo^> param = params[index];
        ParameterAttributes paramAttr = param->Attributes;
        gcroot<System::Object^> defaultValue = param->DefaultValue;
        gcroot<System::Type^> paramType = param->ParameterType;
        pin_ptr<const wchar_t> pChParamName = PtrToStringChars(param->Name);
        STRING* pstrName = m_pCompiler->AddString(pChParamName);
        BCSYM* ptyp = GetSymbolForType(paramType, pContainer, pMethodDecl);
        PARAMFLAGS ParamFlags = 0;
        BCSYM_Expression* pConstExpr = NULL;

        if (ptyp->IsPointerType())
        {
            ParamFlags |= PARAMF_ByRef;
        }

        if(param->IsOptional)
        {
            ParamFlags |= PARAMF_Optional;
        }

        int checkFlags = ((int)ParameterAttributes::HasDefault) | ((int)ParameterAttributes::Optional);

        if((((int)param->Attributes) & checkFlags) != 0)
        {
            // For ref types (i.e. byref parameters), need to get the actual non-ref type
            //
            gcroot<System::Type^> TypeOfDefaultValue = param->ParameterType;
            if (TypeOfDefaultValue->IsByRef)
            {
                TypeOfDefaultValue = TypeOfDefaultValue->GetElementType();
            }

            pConstExpr = GetSymbolForConstantExpression(param->DefaultValue, TypeOfDefaultValue);
        }

        BCSYM_Param* paramSymbol = m_pSymbols->GetParam(
            NULL, // Location *ploc
            pstrName, //STRING* pstrName
            ptyp, // BCSYM* ptyp
            ParamFlags, //PARAMFLAGS
            pConstExpr, //BCSYM_Expression* pconstrexpr
            &pParamFirst, // BCSYM_Param **ppparamFirst
            &pParamLast, //BCSYM_Param **ppparamLast
            false // bool isReturnType
            );

        IfFailGo(LoadCustomAttributes(param, paramSymbol));

    }

    *ppParamList = pParamFirst;

Error:
    return hr;
}

void ExternalTypeBuilder::SetupGenericParameters(BCSYM_NamedRoot* pTypeSymbol, gcroot<System::Type^> pType)
{
    ASSERT(pTypeSymbol, "[ExternalTypeBuilder::SetupGenericParameters] 'pTypeSymbol' parameter is NULL");
    ASSERT(pType, "[ExternalTypeBuilder::SetupGenericParameters] 'pType' parameter is NULL");

    if (pTypeSymbol == NULL || !pType)
        return;

    gcroot<System::Type^> pParent;
    gcroot<array<System::Type^,1>^> GenericArguments;
    gcroot<array<System::Type^,1>^> ParentGenericArguments;

    GenericArguments = pType->GetGenericArguments();

    pParent = pType->DeclaringType;
    if (pParent && (pParent->IsClass || pParent->IsInterface || pParent->IsValueType))
    {
        ParentGenericArguments = pParent->GetGenericArguments();
    }

    SetupGenericParameters(pTypeSymbol, GenericArguments, ParentGenericArguments, false);
}

void ExternalTypeBuilder::SetupGenericParameters(BCSYM_NamedRoot* pMethodSymbol, gcroot<System::Reflection::MethodInfo^> pMethod)
{
    ASSERT(pMethodSymbol, "[ExternalTypeBuilder::SetupGenericParameters] 'pMethodSymbol' parameter is NULL");
    ASSERT(pMethod, "[ExternalTypeBuilder::SetupGenericParameters] 'pMethod' parameter is NULL");

    if (pMethodSymbol == NULL || !pMethod)
        return;

    gcroot<array<System::Type^,1>^> GenericArguments;
    
    GenericArguments = pMethod->GetGenericArguments();

    //Methods never retrieve their parent generic arguments
    SetupGenericParameters(pMethodSymbol, GenericArguments, nullptr, true);
}

void ExternalTypeBuilder::SetupGenericParameters(BCSYM_NamedRoot* pSymbol, gcroot<array<System::Type^,1>^> GenericArguments, 
                                                 gcroot<array<System::Type^,1>^> ParentGenericArguments, bool isMethod)
{
    ASSERT(pSymbol, "[ExternalTypeBuilder::SetupGenericParameters] 'pSymbol' parameter is NULL");
    ASSERT(GenericArguments, "[ExternalTypeBuilder::SetupGenericParameters] 'pType' parameter is NULL");

    if (pSymbol == NULL || !GenericArguments)
        return;

    unsigned long TypeParamCount = 0;
    unsigned long ContainerTypeParamCount = 0;
    bool GenericsViolateCLS = false;
    BCSYM_NamedRoot* pContainer = NULL;

    BCSYM_GenericParam* FirstGenericParam = NULL;
    BCSYM_GenericParam** GenericParamTarget = &FirstGenericParam;
    BCSYM_GenericParam* ContainerTypeGenericParam = NULL;

    TypeParamCount = GenericArguments->Length;

    // Get all generic restrictions from the container of this symbol.
    //  This happens for Nested Types or Generic Methods.
    // Verification that the parent exists and is a class/interface is done in the
    // Type/Method specific helper.
    if (pSymbol->IsContainer() && ParentGenericArguments)
    {
        ContainerTypeParamCount = ParentGenericArguments->Length;
    }
    
    if (TypeParamCount < ContainerTypeParamCount)
    {
        GenericsViolateCLS = true;
    }

    //Check to make sure the first n Type Parameters are the same for the current symbol
    // and the parent type (if present)
    for (unsigned TypeParamIndex = 0; 
        TypeParamIndex < min(TypeParamCount, ContainerTypeParamCount); 
        TypeParamIndex++)
    {
        gcroot<System::Type^> GenericArgument = GenericArguments[TypeParamIndex];
        gcroot<System::Type^> ParentGenericArgument = ParentGenericArguments[TypeParamIndex];
        
        System::Reflection::GenericParameterAttributes GenericAttr = GenericArgument->GenericParameterAttributes;
        System::Reflection::GenericParameterAttributes ParentAttr = ParentGenericArgument->GenericParameterAttributes;

        GenericsViolateCLS |= GenericAttr != ParentAttr;
    }

    ASSERT(!GenericsViolateCLS, "[ExternalTypeBuilder::SetupGenericParameters] 'GenericsViolateCLS': imported generic parameters do not match parent generic parameters");

    // Setup actual generic parameters for this symbol
    for (unsigned TypeParamIndex = ContainerTypeParamCount; TypeParamIndex < TypeParamCount; TypeParamIndex++)
    {
        Variance_Kind Variance;
        gcroot<System::Type^> GenericArgument = GenericArguments[TypeParamIndex];
        pin_ptr<const wchar_t> Name;
        BCSYM_GenericParam* Param = NULL;
        GenericParameterAttributes attributes;
        mscorlib::_TypePtr pGenericArgumentCom;

        Name = PtrToStringChars(GenericArgument->Name);
        attributes = GenericArgument->GenericParameterAttributes;

        switch (attributes & GenericParameterAttributes::VarianceMask)
        {
        case GenericParameterAttributes::Covariant:
            Variance = Variance_Out;
            break;
        case GenericParameterAttributes::Contravariant:
            Variance = Variance_In;
            break;
        case GenericParameterAttributes::None:
            Variance = Variance_None;
            break;
        default:
            Variance = Variance_None;
            // See comment in MetaImport.cpp, MetaImport::SetupGenericParameters for explanation regarding default
        }

        Param = m_pSymbols->GetGenericParam(
            NULL, // Location* Location
            m_pCompiler->AddString(Name),
            TypeParamIndex - ContainerTypeParamCount,
            isMethod, //bool IsGenericMethodParam
            Variance
            );

        Param->SetMetaDataPosition(TypeParamIndex);
  
        //SetupGenericParameterNonTypeConstraints (static helper equivalent)
        SetupGenericParameterNonTypeConstraints(Param, attributes);

        //Need to store the System::Type/mscorlib::_Type so that SetupGenericParameterTypeConstraints
        // can use it.

        if (SUCCEEDED(ConvertSystemTypeToCom(GenericArgument, &pGenericArgumentCom)))
            Param->SetExternalSymbol(m_pFactory->GetHostedAllocator()->AddToReleaseList(pGenericArgumentCom));
#if DEBUG
        else
            VSFAIL("GenericArgument cannot be converted to COM::mscorlib::_Type");
#endif

        *GenericParamTarget = Param;
        GenericParamTarget = Param->GetNextParamTarget();
        
    }

    if (FirstGenericParam != NULL)
    {
        m_pSymbols->SetGenericParams(FirstGenericParam, pSymbol);
    }

    //Change the name! From "GenericClass1" to "GenericClass1`1", for instance.
    if (pSymbol->IsType() && TypeParamCount > ContainerTypeParamCount)
    {
        unsigned GenericParamCountForThisType = TypeParamCount - ContainerTypeParamCount;

        pSymbol->SetName(
            GetActualTypeNameFromEmittedTypeName(
                pSymbol->GetEmittedName(),
                GenericParamCountForThisType,
                m_pCompiler
                )
            );
    }
}

void ExternalTypeBuilder::SetupGenericParameterTypeConstraints(BCSYM_GenericParam* ListOfGenericParameters)
{
    if (ListOfGenericParameters == NULL)
        return;

    for (BCSYM_GenericParam* Param = ListOfGenericParameters; Param; Param = Param->GetNextParam())
    {
        gcroot<System::Type^> pParameterType;

        if (SUCCEEDED(ConvertCOMTypeToSystemType(Param->GetExternalSymbol(), pParameterType)))
        {
            unsigned long ConstraintCount = 0;
            gcroot<array<System::Type^,1>^> Constraints;
            Constraints = pParameterType->GetGenericParameterConstraints();

            if (!Constraints || Constraints->Length == 0)
            {
                continue;
            }

            BCSYM_GenericConstraint* ConstraintSymbols = NULL;
            BCSYM_GenericConstraint** ConstraintTarget = &ConstraintSymbols;

            bool IsValueConstraintSet = Param->HasValueConstraint();

            for (unsigned ConstraintIndex = 0; ConstraintIndex < ConstraintCount; ConstraintIndex++)
            {
                BCSYM* ConstraintType = NULL;
                gcroot<System::Type^> Constraint = Constraints[ConstraintIndex];

                ConstraintType = GetSymbolForType(Constraint, NULL, NULL);
                ASSERT(ConstraintType, "[ExternalTypeBuilder::SetupGenericParameterTypeConstraints] 'ConstraintType' type not found");

                if (!IsValueConstraintSet || 
                    ConstraintType != m_pCompiler->GetDefaultCompilerHost()->GetFXSymbolProvider()->GetType(FX::ValueTypeType))
                {
                    *ConstraintTarget = m_pSymbols->GetGenericTypeConstraint(
                        NULL, //Location* Location
                        ConstraintType
                        );

                    ASSERT(!ConstraintType->IsBad(), "[ExternalTypeBuilder::SetupGenericParameterTypeConstraints] 'ConstraintType' is bad");

                    ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
                }
            }

            *ConstraintTarget = Param->GetConstraints();
            Param->SetConstraints(ConstraintSymbols);

        }
    }
}

void ExternalTypeBuilder::SetupGenericParameterNonTypeConstraints(BCSYM_GenericParam* Parameter, GenericParameterAttributes GenericParamFlags)
{
    ASSERT(Parameter, "[ExternalTypeBuilder::SetupGenericParameterNonTypeConstraints] 'Parameter' parameter is NULL");
    if (Parameter == NULL)
        return;

    if ((GenericParamFlags & GenericParameterAttributes::SpecialConstraintMask) == GenericParameterAttributes::None)
    {
        return;
    }

    BCSYM_GenericConstraint* Constraints = NULL;
    BCSYM_GenericConstraint** ConstraintTarget = &Constraints;

    if (((GenericParamFlags & GenericParameterAttributes::DefaultConstructorConstraint) != GenericParameterAttributes::None)
        && 
        ((GenericParamFlags & GenericParameterAttributes::NotNullableValueTypeConstraint)!= GenericParameterAttributes::None))
    {
        *ConstraintTarget = m_pSymbols->GetGenericNonTypeConstraint(
            NULL,
            BCSYM_GenericNonTypeConstraint::ConstraintKind_New
            );
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    if ((GenericParamFlags & GenericParameterAttributes::ReferenceTypeConstraint) != GenericParameterAttributes::None)
    {
        *ConstraintTarget = m_pSymbols->GetGenericNonTypeConstraint(
            NULL,
            BCSYM_GenericNonTypeConstraint::ConstraintKind_Ref
            );
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    if ((GenericParamFlags & GenericParameterAttributes::NotNullableValueTypeConstraint) != GenericParameterAttributes::None)
    {
        *ConstraintTarget = m_pSymbols->GetGenericNonTypeConstraint(
            NULL,
            BCSYM_GenericNonTypeConstraint::ConstraintKind_New
            );
        ConstraintTarget = (*ConstraintTarget)->GetNextConstraintTarget();
    }

    *ConstraintTarget = Parameter->GetConstraints();
    Parameter->SetConstraints(Constraints);
}

BCSYM* ExternalTypeBuilder::BuildGenericBinding(BCSYM* Generic, unsigned long ArgumentCount, BCSYM** Arguments, bool AllocAndCopyArgumentsToNewList)
{
    VerifyInPtrRetNull(Generic, "[ExternalTypeBuilder::BuildGenericBinding] 'Generic' parameter is NULL");
    VerifyInPtrRetNull(Arguments, "[ExternalTypeBuilder::BuildGenericBinding] 'Arguments' parameter is NULL");

    unsigned long StartIndex = 0;
    
    return BuildGenericBindingHelper(
        Generic,
        ArgumentCount,
        Arguments,
        StartIndex,
        AllocAndCopyArgumentsToNewList
        );
}

BCSYM* ExternalTypeBuilder::BuildGenericBindingHelper(BCSYM* Generic, unsigned long ArgumentCount, BCSYM** Arguments, unsigned long &StartIndex, bool AllocAndCopyArgumentsToNewList)
{
    VerifyInPtrRetNull(Generic, "[ExternalTypeBuilder::BuildGenericBindingHelper] 'Generic' parameter is NULL");
    VerifyInPtrRetNull(Arguments, "[ExternalTypeBuilder::BuildGenericBindingHelper] 'Arguments' parameter is NULL");

    BCSYM_GenericTypeBinding* ParentBinding = NULL;
    BCSYM_GenericTypeBinding* Result = NULL;
    unsigned GenericParamCount = Generic->GetGenericParamCount();

    ASSERT(ArgumentCount != 0, "[ExternalTypeBuilder::BuildGenericBindingHelper] Generic with zero type arguments unexpected");
    ASSERT(GenericParamCount <= ArgumentCount, "[ExternalTypeBuilder::BuildGenericBindingHelper] Generic instantiation with more type arguments than type parameters unexpected.");

    if (GenericParamCount < ArgumentCount)
    {
        ParentBinding =
            BuildGenericBindingHelper(
                Generic->PNamedRoot()->GetContainer(),
                ArgumentCount - GenericParamCount,
                Arguments,
                StartIndex,
                AllocAndCopyArgumentsToNewList
                )->PGenericTypeBinding();
    }

    ASSERT(GenericParamCount <= ArgumentCount - StartIndex, "Generic instantiation import lost.");

    Result = m_pSymbols->GetGenericBinding(
        false,
        Generic->PNamedRoot(),
        GenericParamCount > 0 ? Arguments + StartIndex : NULL,
        GenericParamCount,
        ParentBinding,
        AllocAndCopyArgumentsToNewList)->PGenericTypeBinding();

    StartIndex += GenericParamCount;

    return Result;
}

DECLFLAGS ExternalTypeBuilder::DeclFlagsForMethod(gcroot<System::Reflection::MethodBase^> pMethodBase, System::Reflection::MethodAttributes* pAttributes)
{
    DECLFLAGS DeclFlag = 0;
    
    VerifyParamCond(pMethodBase, 0, "[ExternalTypeBuilder::DeclFlagsForMethod] 'pMethod' parameter is null");

    *pAttributes = pMethodBase->Attributes;
    DeclFlag = MetaImport::MapMethodFlagsToDeclFlags((ULONG)*pAttributes, false);

    return DeclFlag;
}

DECLFLAGS ExternalTypeBuilder::DeclFlagsForType(gcroot<System::Type^> pType)
{
    DECLFLAGS DeclFlag = 0;
    System::Reflection::TypeAttributes attributes;

    VerifyParamCond(pType, 0, "[ExternalTypeBuilder::DeclFlagsForType] 'pType' parameter is null");

    attributes = pType->Attributes;
    DeclFlag = MetaImport::MapTypeFlagsToDeclFlags((ULONG)attributes, false);

    return DeclFlag;
}

DECLFLAGS ExternalTypeBuilder::DeclFlagsForField(gcroot<System::Reflection::FieldInfo^> pField)
{
    DECLFLAGS DeclFlag = 0;
    System::Reflection::FieldAttributes attributes;

    VerifyParamCond(pField, 0, "[ExternalTypeBuilder::DeclFlagsForField] 'pField' parameter is null");

    attributes = pField->Attributes;
    DeclFlag = MetaImport::MapFieldFlagsToDeclFlags((ULONG)attributes, false);

    return DeclFlag;
}

bool ExternalTypeBuilder::ShouldImportSymbol(DECLFLAGS DeclFlags)
{
    return !(((DeclFlags & DECLF_Private) == DECLF_Private) || ((DeclFlags & DECLF_Friend) == DECLF_Friend));
}

void VBHostIntegration::ImportTypeChildren(BCSYM_Container* pContainer)
{
    ASSERT(pContainer && pContainer->GetExternalSymbol(), "[VBHostIntegration::ImportTypeChildren] 'pContainer' parameter must be valid");
    if (!pContainer || !pContainer->GetExternalSymbol())
        return;

    BCSYM_Namespace* pNS = pContainer->GetContainingNamespace();
    BCSYM_Hash* pNSHash = pNS->GetHashRaw();

    ASSERT(pNSHash && pNSHash->GetExternalSymbolSource(), "[VBHostIntegration::ImportTypeChildren] 'pNSHash' namespace not initiated");
    if (!pNSHash || !pNSHash->GetExternalSymbolSource())
        return;

    pNSHash->GetExternalSymbolSource()->ImportTypeChildren(pContainer);

}

BCSYM_Expression* ExternalTypeBuilder::GetSymbolForConstantExpression(gcroot<System::Object^> value, gcroot<System::Type^> valueType)
{
    VerifyInPtrRetNull(valueType, "[ExternalTypeBuilder::GetSymbolForConstantExpression] 'valueType' parameter is null");

    ConstantValue Value;
    LoadConstantValue(value, valueType, &Value);

    return m_pSymbols->GetFixedExpression(&Value);
}

void ExternalTypeBuilder::LoadConstantValue (gcroot<System::Object^> value, gcroot<System::Type^> valueType, ConstantValue* Value)
{
    ASSERT(valueType, "[ExternalTypeBuilder::GetSymbolForConstantExpression] 'valueType' parameter is null");
    ASSERT(Value, "[ExternalTypeBuilder::GetSymbolForConstantExpression] 'Value' parameter is null");

    if(!valueType || !Value)
    {
        return;
    }

    if(valueType->IsEnum)
    {
        FieldInfo^ field =  valueType->GetField(gcnew System::String(STRING_CONST(m_pCompiler, EnumValueMember)));
        ASSERT(field, "[ExternalTypeBuilder::GetConstantExpression] there must be a field named value__");
        valueType = field->FieldType;
        if(!valueType)
        {
            Value->TypeCode = t_bad;
            return;
        }
    }

    if(System::Boolean::typeid->Equals(valueType))
    {
        Value->TypeCode = t_bool;
        Value->Integral = safe_cast<USE_UNALIGNED __int8>(safe_cast<System::Boolean>(static_cast<System::Object^>(value))); // Note bools in COM+ are 1 byte
    }
    else if(System::SByte::typeid->Equals(valueType))
    {
        Value->TypeCode = t_i1;
        Value->Integral = safe_cast<USE_UNALIGNED __int8>(safe_cast<System::SByte>(static_cast<System::Object^>(value)));
    }
    else if(System::Byte::typeid->Equals(valueType))
    {
        Value->TypeCode = t_ui1;
        Value->Integral = safe_cast<USE_UNALIGNED unsigned __int8>(safe_cast<System::Byte>(static_cast<System::Object^>(value)));
    }
    else if(System::Int16::typeid->Equals(valueType))
    {
        Value->TypeCode = t_i2;
        Value->Integral = safe_cast<USE_UNALIGNED __int16>(safe_cast<System::Int16>(static_cast<System::Object^>(value)));
    }
    else if(System::UInt16::typeid->Equals(valueType))
    {
        Value->TypeCode = t_ui2;
        Value->Integral = safe_cast<USE_UNALIGNED unsigned __int16>(safe_cast<System::UInt16>(static_cast<System::Object^>(value)));
    }
    else if(System::Int32::typeid->Equals(valueType))
    {
        Value->TypeCode = t_i4;
        Value->Integral = safe_cast<USE_UNALIGNED __int32>(safe_cast<System::Int32>(static_cast<System::Object^>(value)));
    }
    else if(System::UInt32::typeid->Equals(valueType))
    {
        Value->TypeCode = t_ui4;
        Value->Integral = safe_cast<USE_UNALIGNED unsigned __int32>(safe_cast<System::UInt32>(static_cast<System::Object^>(value)));
    }
    else if(System::Int64::typeid->Equals(valueType))
    {
        Value->TypeCode = t_i8;
        Value->Integral = safe_cast<USE_UNALIGNED __int64>(safe_cast<System::Int64>(static_cast<System::Object^>(value)));
    }
    else if(System::DateTime::typeid->Equals(valueType))
    {
        Value->TypeCode = t_date;
        Value->Integral = safe_cast<USE_UNALIGNED __int64>(safe_cast<System::DateTime>(static_cast<System::Object^>(value)).Ticks);
    }
    else if(System::UInt64::typeid->Equals(valueType))
    {
        Value->TypeCode = t_ui8;
        Value->Integral = safe_cast<USE_UNALIGNED unsigned __int64>(safe_cast<System::UInt64>(static_cast<System::Object^>(value)));
    }
    else if(System::Single::typeid->Equals(valueType))
    {
        Value->TypeCode = t_single;
        Value->Single = safe_cast<USE_UNALIGNED float>(safe_cast<System::Single>(static_cast<System::Object^>(value)));
    }
    else if(System::Double::typeid->Equals(valueType))
    {
        Value->TypeCode = t_double;
        Value->Double = safe_cast<USE_UNALIGNED double>(safe_cast<System::Double>(static_cast<System::Object^>(value)));
    }
    else if(System::Char::typeid->Equals(valueType))
    {
        Value->TypeCode = t_char;
        ASSERT(value, "[ExternalTypeBuilder::GetSymbolForConstantExpression] 'value' param has type charecter but is null");
        gcroot<System::String^> str = value->ToString();
        unsigned int length = str->Length;
        pin_ptr<const wchar_t> buffer = PtrToStringChars(str);
        Value->Integral = *buffer;
    }
    else if(System::String::typeid->Equals(valueType))
    {
        Value->TypeCode = t_string;
        if(value)
        {
            gcroot<System::String^> str = value->ToString();
            Value->String.LengthInCharacters = str->Length;
            if ((Value->String.LengthInCharacters + 1 < 1)  ||
                (!VBMath::TryMultiply((Value->String.LengthInCharacters + 1), sizeof(WCHAR))) ||
                (Value->String.LengthInCharacters >= (Value->String.LengthInCharacters + 1)))
            {
                VSFAIL("[ExternalTypeBuilder::GetSymbolForConstantExpression] Required size exceeds boundaries.");
                Value->TypeCode = t_bad;
                return;
            }
            pin_ptr<const wchar_t> buffer = PtrToStringChars(str);
            ASSERT(buffer, "[ExternalTypeBuilder::GetSymbolForConstantExpression] could not pin string");

            WCHAR * pTmp = (WCHAR *)m_pSymbols->GetNorlsAllocator()->Alloc((Value->String.LengthInCharacters + 1) * sizeof(WCHAR));

            if(Value->String.LengthInCharacters)
            {
                memcpy(pTmp, buffer, Value->String.LengthInCharacters * sizeof(WCHAR));
            }
            
            pTmp[Value->String.LengthInCharacters] = '\0';
            Value->String.Spelling = pTmp;
        }
    }
    else if(System::Decimal::typeid->Equals(valueType))
    {
        System::Decimal decimal = safe_cast<System::Decimal>(static_cast<System::Object^>(value));
        Value->TypeCode = t_decimal;
        gcroot<array<int>^> values = System::Decimal::GetBits(decimal);
        Value->Decimal.Lo32 = values[0];
        Value->Decimal.Mid32 = values[1];
        Value->Decimal.Hi32 = values[2];
        int temp = values[3];
        Value->Decimal.scale= (temp & 0x00FF0000) >> 16;
        Value->Decimal.sign = (temp & 0x80000000) >> 31;
    }
    else
    {
        Value->TypeCode = t_ref;
        ASSERT(!value || System::DBNull::Value->Equals(value), "[ExternalTypeBuilder::GetConstantExpression] Can only handle NULL case in import.");
        ASSERT(Value->Integral == 0, "[ExternalTypeBuilder::GetConstantExpression] ConstantValue's CTOR should init to zero.");
    }
}

HRESULT ExternalTypeBuilder::LoadCustomAttributes(gcroot<System::Reflection::MemberInfo^> pMember, BCSYM* pSym)
{
    HRESULT hr = S_OK;
    VerifyInPtr(pSym, "[ExternalTypeBuilder::LoadCustomAttributes MemberInfo] 'pSym' parameter is null");
    ASSERT(pSym->IsNamedRoot(), "[ExternalTypeBuilder::LoadCustomAttributes MemberInfo] parameter 'pSym' was not a named root");
    System::Collections::Generic::IList<CustomAttributeData^>^ customAttributes = CustomAttributeData::GetCustomAttributes(pMember);
    IfFailGo(LoadCustomAttributes(customAttributes, pSym));
    
Error:
    return hr;
}

HRESULT ExternalTypeBuilder::LoadCustomAttributes(gcroot<System::Reflection::ParameterInfo^> pParam, BCSYM* pSym)
{
    HRESULT hr = S_OK;
    VerifyInPtr(pSym, "[ExternalTypeBuilder::LoadCustomAttributes ParameterInfo] 'pSym' parameter is null");
    ASSERT(pSym->IsParam(), "[ExternalTypeBuilder::LoadCustomAttributes ParameterInfo] parameter 'pSym' was not a param");
    gcroot<System::Collections::Generic::IList<CustomAttributeData^>^> customAttributes = CustomAttributeData::GetCustomAttributes(pParam);
    IfFailGo(LoadCustomAttributes(customAttributes, pSym));
    
Error:
    return hr;
}

//See Attributes.h for thw list of well known attributes
HRESULT ExternalTypeBuilder::LoadCustomAttributes(gcroot<System::Collections::Generic::IList<CustomAttributeData^>^> customAttributes, BCSYM* pSym)
{
    HRESULT hr = S_OK;
    WellKnownAttrVals *pAttrVals;

    IfFalseGo(customAttributes, S_OK);
    VerifyInPtr(pSym, "[ExternalTypeBuilder::LoadCustomAttributes IList] 'pSym' parameter is null");

    // Need to allocate an OUT AttrVals if we don't already have one
    if (pSym->GetPAttrVals() == NULL)
    {
        // Since this AttrVals will be hung off a symbol, allocate from the symbol allocator.
        const bool AllocForWellKnownAttributesToo = true;
        pSym->AllocAttrVals(m_pSymbols->GetNorlsAllocator(), CS_MAX, NULL, NULL, AllocForWellKnownAttributesToo);
    }

    pAttrVals = pSym->GetPWellKnownAttrVals();
    ASSERT(pAttrVals, "[ExternalTypeBuilder::LoadCustomAttributes IList] Should have well known attributes");

    for each(CustomAttributeData^ attribute in safe_cast<System::Collections::Generic::IList<CustomAttributeData^>^>(customAttributes))
    {
        gcroot<System::Type^> attributeType = attribute->Constructor->DeclaringType;
        if(System::AttributeUsageAttribute::typeid->Equals(attributeType))
        {
            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::StructLayoutAttribute] AttributeUsageAttribute has wrong number of arguments for constructor");
            CorAttributeTargets attrTargetsValidOn = safe_cast<CorAttributeTargets>(safe_cast<int>(attribute->ConstructorArguments[0].Value));
            bool fAllowMultiple = false;
            bool fInherited = false;
            for(int ind = 0; ind < attribute->NamedArguments->Count; ind++)
            {
                CustomAttributeNamedArgument argument = attribute->NamedArguments[ind];
                if(!argument.MemberInfo && argument.MemberInfo->Name)
                {
                    ASSERT(false, "[ExternalTypeBuilder::LoadCustomAttributes IList] AttributeUsageAttribute's named argument must have a name");
                    IfFailGo(E_UNEXPECTED);
                }

                if(argument.MemberInfo->Name->Equals(L"AllowMultiple"))
                {
                    fAllowMultiple = safe_cast<bool>(argument.TypedValue.Value);
                }
                else if(argument.MemberInfo->Name->Equals(L"Inherited"))
                {
                    fInherited  = safe_cast<bool>(argument.TypedValue.Value);
                }
            }
            pAttrVals->SetAttributeUsageData(attrTargetsValidOn, fAllowMultiple, fInherited);

        }
        else if(System::ObsoleteAttribute::typeid->Equals(attributeType))
        {

            WCHAR* pwszMessage = NULL;
            bool fError = false;

            if(attribute->ConstructorArguments->Count > 0)
            {
                gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
                pwszMessage = GetBuffer(str);
            }
            if(attribute->ConstructorArguments->Count == 2)
            {
                fError = safe_cast<bool>(attribute->ConstructorArguments[1].Value);
            }

            pAttrVals->SetObsoleteData(pwszMessage, fError);

        }
        else if(System::Diagnostics::ConditionalAttribute::typeid->Equals(attributeType))
        {

            WCHAR* pwszConditional = NULL;
            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::StructLayoutAttribute] ConditionalAttribute has wrong number of arguments for constructor");

            gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
            pwszConditional = GetBuffer(str);
            pwszConditional = m_pCompiler->AddString(pwszConditional);

            pAttrVals->AddConditionalData(pwszConditional);

        }
        else if(System::Diagnostics::DebuggableAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDebuggableData();
        }
        else if(System::Diagnostics::DebuggerBrowsableAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] DebuggerBrowsableAttribute has wrong number of arguments for constructor");
            pAttrVals->SetDebuggerBrowsableData(safe_cast<DebuggerBrowsableStateEnum>(safe_cast<int>(attribute->ConstructorArguments[0].Value)));

        }
        else if(System::Diagnostics::DebuggerDisplayAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDebuggerDisplayData();
        }
        else if(System::Diagnostics::DebuggerHiddenAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDebuggerHiddenData();
        }
        else if(System::CLSCompliantAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] CLSCompliantAttribute has wrong number of arguments for constructor");
            bool fCompliant = safe_cast<bool>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetCLSCompliantData(fCompliant, false /* fInherited */);

        }
        else if(System::ComponentModel::DesignerSerializationVisibilityAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] DesignerSerializationVisibilityAttribute has wrong number of arguments for constructor");
            int DesignerSerializationVisibilityType = safe_cast<int>(attribute->ConstructorArguments[0].Value);
            const int DESIGNERSERIALIZATIONVISIBILITYTYPE_CONTENT = 2;
            bool fPersistContents;
            if (DesignerSerializationVisibilityType == DESIGNERSERIALIZATIONVISIBILITYTYPE_CONTENT)
            {
                fPersistContents = true;
            }
            else
            {
                fPersistContents = false;
            }
            pAttrVals->SetPersistContentsData(fPersistContents);

        }
        else if(System::Reflection::DefaultMemberAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] DefaultMemberAttribute has wrong number of arguments for constructor");
            gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
            WCHAR* pwszDefaultMember = GetBuffer(str);
            pAttrVals->SetDefaultMemberData(pwszDefaultMember);

        }
        else if(System::ComponentModel::DesignerCategoryAttribute::typeid->Equals(attributeType))
        {

            WCHAR* pwszCategory = NULL;
            if(attribute->ConstructorArguments->Count > 0)
            {
                gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
                pwszCategory = GetBuffer(str);
            }
            pAttrVals->SetDesignerCategoryData(pwszCategory);

        }
        //don't load StandardModuleAttribute
        //don't load ExtensionAttribute
        else if(Microsoft::VisualBasic::CompilerServices::DesignerGeneratedAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDesignerGeneratedData();
        }
        else if(Microsoft::VisualBasic::CompilerServices::OptionCompareAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetOptionCompareData();
        }
        else if(System::ParamArrayAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetParamArrayData();
        }
        else if(System::Runtime::CompilerServices::DecimalConstantAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 5, "[ExternalTypeBuilder::LoadCustomAttributes IList] DecimalConstantAttribute has wrong number of arguments for constructor");
            DECIMAL decimalValue;
            decimalValue.scale = safe_cast<BYTE>(attribute->ConstructorArguments[0].Value);
            decimalValue.sign = safe_cast<BYTE>(attribute->ConstructorArguments[1].Value);
            if(System::Int32::typeid->Equals(attribute->ConstructorArguments[2].ArgumentType))
            {
                decimalValue.Hi32 = safe_cast<int>(attribute->ConstructorArguments[2].Value);
                decimalValue.Mid32 = safe_cast<int>(attribute->ConstructorArguments[3].Value);
                decimalValue.Lo32 = safe_cast<int>(attribute->ConstructorArguments[4].Value);
            }
            else
            {
                decimalValue.Hi32 = safe_cast<unsigned int>(attribute->ConstructorArguments[2].Value);
                decimalValue.Mid32 = safe_cast<unsigned int>(attribute->ConstructorArguments[3].Value);
                decimalValue.Lo32 = safe_cast<unsigned int>(attribute->ConstructorArguments[4].Value);
            }
            pAttrVals->SetDecimalConstantData(decimalValue);

        }
        else if(System::Runtime::CompilerServices::DateTimeConstantAttribute::typeid->Equals(attributeType))
        {

            __int64 dateTimeValue = safe_cast<__int64>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetDateTimeConstantData(dateTimeValue);

        }
        else if(System::Runtime::CompilerServices::IDispatchConstantAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetIDispatchConstantData();
        }
        else if(System::Runtime::CompilerServices::IUnknownConstantAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetIUnknownConstantData();
        }
        else if(System::ComponentModel::EditorBrowsableAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetEditorBrowsableData(0 /* editorBrowsableState */);
        }
        else if(System::Runtime::InteropServices::DllImportAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDllImportData();
        }
        else if(System::Runtime::InteropServices::DllImportAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetDllImportData();
        }
        else if(System::STAThreadAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetSTAThreadData();
        }
        else if(System::MTAThreadAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetMTAThreadData();
        }
        else if(System::Runtime::CompilerServices::AccessedThroughPropertyAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] AccessedThroughPropertyAttribute has wrong number of arguments for constructor");
            gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
            WCHAR* pwszProperty = GetBuffer(str);
            pAttrVals->SetAccessedThroughPropertyData(pwszProperty);

        }
        //CompilationRelaxationsAttribute is handled only for assemblies.
        //don't load MyGroupCollectionAttribute
        //don't load HideModuleNameAttribute
        else if(System::Runtime::CompilerServices::RuntimeCompatibilityAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetRuntimeCompatibilityData();
        }
        else if(System::Runtime::InteropServices::MarshalAsAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetMarshalAsData();
        }
        else if(System::Runtime::InteropServices::TypeLibTypeAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] TypeLibTypeAttribute has wrong number of arguments for constructor");
            short typeLibType;
            if(System::Int16::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
            {
                typeLibType = safe_cast<short>(attribute->ConstructorArguments[0].Value);
            }
            else
            {
                typeLibType = safe_cast<short>(safe_cast<System::Runtime::InteropServices::TypeLibTypeFlags>(attribute->ConstructorArguments[0].Value));
            }
            pAttrVals->SetTypeLibTypeData(typeLibType);

        }
        else if(System::Runtime::InteropServices::InterfaceTypeAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] InterfaceTypeAttribute has wrong number of arguments for constructor");
            long interfaceType;
            if(System::Int16::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
            {
                interfaceType = safe_cast<long>(safe_cast<short>(attribute->ConstructorArguments[0].Value));
            }
            else
            {
                interfaceType = safe_cast<long>(safe_cast<System::Runtime::InteropServices::ComInterfaceType>(attribute->ConstructorArguments[0].Value));
            }
            pAttrVals->SetInterfaceTypeData(interfaceType);

        }
        else if(System::Runtime::InteropServices::ClassInterfaceAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] ClassInterfaceAttribute has wrong number of arguments for constructor");
            short classInterfaceType;
            if(System::Int16::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
            {
                classInterfaceType = safe_cast<short>(attribute->ConstructorArguments[0].Value);
            }
            else
            {
                classInterfaceType = safe_cast<short>(safe_cast<System::Runtime::InteropServices::ClassInterfaceType>(attribute->ConstructorArguments[0].Value));
            }
            pAttrVals->SetClassInterfaceData(classInterfaceType);

        }
        else if(System::Runtime::InteropServices::TypeLibFuncAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] TypeLibFuncAttribute has wrong number of arguments for constructor");
            short typeLibFunc;
            if(System::Int16::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
            {
                typeLibFunc = safe_cast<short>(attribute->ConstructorArguments[0].Value);
            }
            else
            {
                typeLibFunc = safe_cast<short>(safe_cast<System::Runtime::InteropServices::TypeLibFuncFlags>(attribute->ConstructorArguments[0].Value));
            }
            pAttrVals->SetTypeLibFuncData(typeLibFunc);

        }
        else if(System::Runtime::InteropServices::ComVisibleAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] ComVisibleAttribute has wrong number of arguments for constructor");
            bool fCOMVisible = safe_cast<bool>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetCOMVisibleData(fCOMVisible);

        }
        else if(System::Runtime::InteropServices::GuidAttribute::typeid->Equals(attributeType))
        {
            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] GuidAttribute has wrong number of arguments for constructor");
            gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
            WCHAR* pwszGuid = GetBuffer(str);
            pAttrVals->SetGuidData(pwszGuid);
        }
        else if(System::Runtime::InteropServices::TypeIdentifierAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetTypeIdentifierData();
        }
        else if(System::Runtime::InteropServices::ComEventInterfaceAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 2, "[ExternalTypeBuilder::LoadCustomAttributes IList] ComEventInterfaceAttribute has wrong number of arguments for constructor");
            WellKnownAttrVals::ComEventInterfaceData data;
            gcroot<System::String^> str = safe_cast<System::Type^>(attribute->ConstructorArguments[0].Value)->FullName;
            data.m_SourceInterface = GetBuffer(str);
            str = safe_cast<System::Type^>(attribute->ConstructorArguments[1].Value)->FullName;
            data.m_EventProvider = GetBuffer(str);
            pAttrVals->SetComEventInterfaceData(data);

        }
        else if(System::Runtime::InteropServices::ComImportAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetComImportData();
        }
        else if(System::Runtime::InteropServices::ComSourceInterfacesAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetComSourceInterfacesData();
        }
        else if(Microsoft::VisualBasic::ComClassAttribute::typeid->Equals(attributeType))
        {

            WellKnownAttrVals::ComClassData data;
            gcroot<System::String^> str;
            if(attribute->ConstructorArguments->Count > 0)
            {
                str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
                data.m_ClassId = GetBuffer(str);
            }
            if(attribute->ConstructorArguments->Count > 1)
            {
                str = safe_cast<System::String^>(attribute->ConstructorArguments[1].Value);
                data.m_InterfaceId = GetBuffer(str);
            }
            if(attribute->ConstructorArguments->Count == 3)
            {
                str = safe_cast<System::String^>(attribute->ConstructorArguments[2].Value);
                data.m_EventId = GetBuffer(str);
            }
            if(attribute->NamedArguments->Count == 1)
            {
                data.m_fShadows = safe_cast<bool>(attribute->NamedArguments[0].TypedValue.Value);
            }
            pAttrVals->SetComClassData(data);

        }
        else if(System::Runtime::InteropServices::DispIdAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] DispIdAttribute has wrong number of arguments for constructor");
            int dispId = safe_cast<int>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetDispIdData(dispId);

        }
        else if(System::Runtime::InteropServices::BestFitMappingAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] BestFitMappingAttribute has wrong number of arguments for constructor");
            bool bestFitMappingData = safe_cast<bool>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetBestFitMappingData(bestFitMappingData, false /* fThrow */);

        }
        else if(System::Runtime::InteropServices::FieldOffsetAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] FieldOffsetAttribute has wrong number of arguments for constructor");
            long fieldOffsetData = safe_cast<long>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetFieldOffsetData(fieldOffsetData);

        }
        else if(System::Runtime::InteropServices::LCIDConversionAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] LCIDConversionAttribute has wrong number of arguments for constructor");
            long lcidConversionData = safe_cast<long>(attribute->ConstructorArguments[0].Value);
            pAttrVals->SetLCIDConversionData(lcidConversionData);

        }
        else if(System::Runtime::InteropServices::PreserveSigAttribute::typeid->Equals(attributeType))
        {
            ASSERT(pSym->IsProc(), "[ExternalTypeBuilder::LoadCustomAttributes IList] PreserveSigAttribute is only applicable to methods");
            if (pSym->IsProc())
            {
                pSym->PProc()->SetPreserveSig(true);
            }
        }
        else if(System::Runtime::InteropServices::InAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetInData();
        }
        else if(System::Runtime::InteropServices::OutAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetOutData();
        }
        else if(System::Runtime::InteropServices::UnmanagedFunctionPointerAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] UnmanagedFunctionPointerAttribute has wrong number of arguments for constructor");
            short unmanagedFunctionPointerData = safe_cast<short>(safe_cast<System::Runtime::InteropServices::CallingConvention>(attribute->ConstructorArguments[0].Value));
            pAttrVals->SetUnmanagedFunctionPointerData(unmanagedFunctionPointerData);

        }
        else if(System::Runtime::InteropServices::CoClassAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] CoClassAttribute has wrong number of arguments for constructor");
            gcroot<System::String^> str = safe_cast<System::Type^>(attribute->ConstructorArguments[0].Value)->FullName;
            WCHAR* pwszCoClassTypeName = GetBuffer(str);
            pAttrVals->SetCoClassData(pwszCoClassTypeName);

        }
        else if(System::ComponentModel::CategoryAttribute::typeid->Equals(attributeType))
        {

            // The default category is represented as a NULL string.
            WCHAR* pwszCategoryName = NULL;

            if(attribute->ConstructorArguments->Count == 1)
            {
                gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
                pwszCategoryName = GetBuffer(str);
            }
            pAttrVals->SetCategoryData(pwszCategoryName);

        }		
        else if(System::ComponentModel::DefaultEventAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::LoadCustomAttributes IList] DefaultEventAttribute has wrong number of arguments for constructor");
            gcroot<System::String^> str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
            WCHAR* pwszDefaultEvent = GetBuffer(str);
            BCSYM_Class* pPartialClassOnWhichApplied = NULL;
            if (pSym->IsClass())
            {
                pPartialClassOnWhichApplied = pSym->PClass();
            }
            pAttrVals->SetDefaultEventData(pwszDefaultEvent, pPartialClassOnWhichApplied);

        }
        
        else if(System::ComponentModel::Design::HelpKeywordAttribute::typeid->Equals(attributeType))
        {

            WCHAR* pwszHelpKeyword = NULL;
            if(attribute->ConstructorArguments->Count == 1)
            {
                gcroot<System::String^> str;
                if(System::String::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
                {
                    str = safe_cast<System::String^>(attribute->ConstructorArguments[0].Value);
                }
                else
                {
                    str = safe_cast<System::Type^>(attribute->ConstructorArguments[0].Value)->FullName;
                }
                pwszHelpKeyword = GetBuffer(str);
            }
            pAttrVals->SetHelpKeywordData(pwszHelpKeyword);

        }
        else if(System::NonSerializedAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetNonSerializedData();
        }
        else if(System::SerializableAttribute::typeid->Equals(attributeType))
        {
            pAttrVals->SetSerializableData();
        }
        //don't load WebMethodAttribute
        //don't load WebServiceAttribute
        else if(System::Runtime::InteropServices::StructLayoutAttribute::typeid->Equals(attributeType))
        {

            ASSERT(attribute->ConstructorArguments->Count == 1, "[ExternalTypeBuilder::StructLayoutAttribute] DefaultEventAttribute has wrong number of arguments for constructor");
            short structLayoutData;
            if(System::Int16::typeid->Equals(attribute->ConstructorArguments[0].ArgumentType))
            {
                structLayoutData = safe_cast<short>(attribute->ConstructorArguments[0].Value);
            }
            else
            {
                structLayoutData = safe_cast<short>(safe_cast<System::Runtime::InteropServices::LayoutKind>(attribute->ConstructorArguments[0].Value));
            }
            pAttrVals->SetStructLayoutData(structLayoutData);
        }
    }

Error:
    return hr;
}

WCHAR* ExternalTypeBuilder::GetBuffer(gcroot<System::String^> str)
{
    if(str)
    {
        pin_ptr<const wchar_t> buffer = PtrToStringChars(str);
        ASSERT(buffer, "[ExternalTypeBuilder::GetBuffer] could not pin string");
        int length = str->Length;
        if ((length + 1 < 1)  || (!VBMath::TryMultiply(length + 1, sizeof(WCHAR))) || (length >= (length + 1)))
        {
                VSFAIL("[ExternalTypeBuilder::GetBuffer] Required size exceeds boundaries.");
                return NULL;
        }
        WCHAR * pTmp = (WCHAR *)m_pSymbols->GetNorlsAllocator()->Alloc((length + 1) * sizeof(WCHAR));
        if(length)
        {
            memcpy(pTmp, buffer, length * sizeof(WCHAR));
        }
        
        pTmp[length] = '\0';
        return pTmp;
    }
    return NULL;
}

bool ExternalTypeBuilder::ValidateType(const STRING* requestedTypeName, const STRING* requestedNamespace, gcroot<System::Type^> typeToValidate)
{
    int length = 0;
    STRING* testName = NULL;
    STRING* testNamespace = NULL;

    //Trim off any generic anotation
    if (typeToValidate->IsGenericType)
    {
        length = typeToValidate->Name->LastIndexOf('`');
        ASSERT(length > 0, "[ExternalTypeBuilder::ValidateType] generic name was not in the expected format");
    }
    else
    {
        length = typeToValidate->Name->Length;
    }

    //Get the Type's name
    pin_ptr<const wchar_t> buffer = PtrToStringChars(typeToValidate->Name);
    ASSERT(buffer, "[ExternalTypeBuilder::ValidateType] could not pin string for type name");
    testName = m_pCompiler->AddStringWithLen(buffer, length);

    //Get the Type's Namespace
    if(typeToValidate->Namespace != nullptr)
    {
        buffer = PtrToStringChars(typeToValidate->Namespace);
        ASSERT(buffer, "[ExternalTypeBuilder::ValidateType] could not pin string for namespace");
        testNamespace = m_pCompiler->AddString(buffer);
    }
    else
    {
        testNamespace = STRING_CONST(m_pCompiler, EmptyString);
    }

    return StringPool::IsEqual(requestedTypeName, testName) && StringPool::IsEqual(requestedNamespace, testNamespace);
}
