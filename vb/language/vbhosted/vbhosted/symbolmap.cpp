#include "stdAfx.h"

using namespace System::Reflection;
using namespace Microsoft::Compiler::VisualBasic;

SymbolMap::~SymbolMap()
{
    //Check to make sure that we only create the assembly and module builders if we actually had to emit a type
    if(m_assemblyBuilder != nullptr)
    {
        ASSERT(m_moduleBuilder != nullptr, "[SymbolMap::~SymbolMap] should not have created an Assembly Builder if we didn't need a Module Builder.");
        ASSERT(m_moduleBuilder != nullptr && m_moduleBuilder->GetTypes()->Length > 0, "[SymbolMap::~SymbolMap] should not have created an Assembly Builder if we didn't need a Type Builder.");
    }
}

System::Type^ SymbolMap::GetType(BCSYM* pSymbol)
{
    return GetType(pSymbol, NULL);
}

System::Type^ SymbolMap::GetType(BCSYM* pSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pSymbol,"[SymbolMap::GetType] 'pSymbol' parameter is null");
    IfFalseThrow(pSymbol != NULL)

    System::Type^ type = nullptr;

    if(pSymbol->IsAnonymousType())
    {
        type = GetAnonymousType(pSymbol->PGenericTypeBinding());
    }
    else if(pSymbol->IsAnonymousDelegate())
    {
        type = GetAnonymousDelegate(pSymbol);
    }
    else if(pSymbol->IsContainer() &&
            pSymbol->PContainer()->GetSourceFile() &&
            pSymbol->PContainer()->GetSourceFile()->IsSolutionExtension() &&
            pSymbol->PContainer()->GetSourceFile()->GetSolutionExtension()->m_extension == SolutionExtension::XmlHelperExtension)
    {
        return Microsoft::VisualBasic::CompilerServices::InternalXmlHelper::typeid;
    }
    else if(pSymbol->IsNamedRoot())
    {
        if(pSymbol->IsGenericTypeBinding())
        {
            type = GetBoundGenericType(pSymbol->PGenericTypeBinding(), pBindingContext);
        }
        else if(pSymbol->IsGenericParam())
        {
            type = GetGenericParamType(pSymbol->PGenericParam(), pBindingContext);
        }
        else
        {
            type = GetBasicType(pSymbol->PNamedRoot());
        }
    }
    else if(pSymbol->IsArrayType())
    {
        type = GetArrayType(pSymbol->PArrayType(), pBindingContext);
    }
    else if(pSymbol->IsPointerType())
    {
        type = GetType(pSymbol->PPointerType()->GetRoot(), pBindingContext);
        if(type != nullptr)
        {
            type = type->MakeByRefType();
        }
    }
    else if(pSymbol->IsVoidType())
    {
        return System::Void::typeid;
    }
    else
    {
        IfFalseThrow(false);
    }

    return type;
}

AssemblyBuilder^ SymbolMap::DynamicAssemblyBuilder::get()
{
    if(m_assemblyBuilder == nullptr)
    {
        AssemblyName^ assemblyNameObj = gcnew AssemblyName("$HostedHelperAssembly$");
        m_assemblyBuilder = System::AppDomain::CurrentDomain->DefineDynamicAssembly(assemblyNameObj, AssemblyBuilderAccess::RunAndCollect);
    }

    return m_assemblyBuilder;
}

ModuleBuilder^ SymbolMap::DynamicModuleBuilder::get()
{
    if(m_moduleBuilder == nullptr)
    {
        m_moduleBuilder = DynamicAssemblyBuilder->DefineDynamicModule("$HostedHelperAssembly$");
    }

    return m_moduleBuilder;
}

AnonymousTypeEmitter^ SymbolMap::DynamicAnonymousTypeEmitter::get()
{
    if(m_anonymousTypeEmitter == nullptr)
    {
        m_anonymousTypeEmitter = gcnew AnonymousTypeEmitter(this, DynamicModuleBuilder);
    }

    return m_anonymousTypeEmitter;
}

AnonymousDelegateEmitter^ SymbolMap::DynamicAnonymousDelegateEmitter::get()
{
    if(m_anonymousDelegateEmitter == nullptr)
    {
        m_anonymousDelegateEmitter = gcnew AnonymousDelegateEmitter(this, DynamicModuleBuilder);
    }

    return m_anonymousDelegateEmitter;
}

System::Type^ SymbolMap::GetAnonymousType(BCSYM_GenericTypeBinding * pSymbol)
{
    ASSERT(pSymbol,"[SymbolMap::GetAnonymousType] 'pSymbol' parameter is null");
    ASSERT(pSymbol->IsAnonymousType(),"[SymbolMap::GetAnonymousType] 'pSymbol' is not an Anonymous Type");

    BCSYM_NamedRoot* pNamed = pSymbol->PNamedRoot();
    System::Type^ typeDef = nullptr;

    if(!DynamicAnonymousTypeEmitter->TryGetType(pNamed, typeDef))
    {
        typeDef = DynamicAnonymousTypeEmitter->EmitType(pNamed);
    }

    ASSERT(typeDef != nullptr, "[SymbolMap::GetAnonymousType] Could not load Anonymous Type");
    return MakeConcrete(typeDef, nullptr, pSymbol, NULL);
}

System::Type^ SymbolMap::GetAnonymousDelegate(BCSYM * pSymbol)
{
    ASSERT(pSymbol,"[SymbolMap::GetAnonymousDelegate] 'pSymbol' parameter is null");
    ASSERT(pSymbol->IsAnonymousDelegate(),"[SymbolMap::GetAnonymousDelegate] 'pSymbol' is not an Anonymous Delegate");

    BCSYM_NamedRoot* pNamed = pSymbol->PNamedRoot();
    System::Type^ typeDef = nullptr;

    if(!DynamicAnonymousDelegateEmitter->TryGetType(pNamed, typeDef))
    {
        typeDef = DynamicAnonymousDelegateEmitter->EmitType(pNamed);
    }

    ASSERT(typeDef != nullptr, "[SymbolMap::GetAnonymousDelegate] Could not load Anonymous Delegate");
    if(pSymbol->IsGenericTypeBinding())
    {
        return MakeConcrete(typeDef, nullptr, pSymbol->PGenericTypeBinding(), NULL);
    }
    else
    {
        return typeDef;
    }
}

System::Type^ SymbolMap::GetBasicType(BCSYM_NamedRoot* pSymbol)
{
    ASSERT(pSymbol,"[SymbolMap::GetBasicType] 'pSymbol' parameter is null");

    BCSYM_NamedRoot* pParentSymbol = pSymbol->GetParent();

    System::Type^ type = nullptr;
    Assembly^ assembly = nullptr;
    System::String^ typeName = nullptr;

    if (pSymbol->GetExternalSymbol())
    {
        gcroot<System::Type^> typeRoot;
        if (SUCCEEDED(ConvertCOMTypeToSystemType(pSymbol->GetExternalSymbol(), typeRoot)))
        {
            ASSERT(typeRoot, "[SymbolMap::GetType] ConvertCOMTypeToSystemType SUCCEEDED, but returned nullptr");
            return typeRoot;
        }
        else
        {
            ASSERT(false, "[SymbolMap::GetType] ConvertCOMTypeToSystemType failed for pSymbol->GetExternalSymbol()");
            IfFalseThrow(pSymbol != NULL)
        }
    }
    else
    {
        assembly = GetAssembly(pSymbol);

        if(assembly != nullptr)
        {
            //Is this a nested type?
            if(pParentSymbol && pParentSymbol->IsType())
            {
                //Get the parent
                System::Type^ parentType = GetType(pParentSymbol, NULL);
                if(parentType != nullptr)
                {
                    //Get the nested type
                    typeName = gcnew System::String(pSymbol->GetEmittedName());
                    type = parentType->GetNestedType(typeName);
                }
            }
            else
            {
                //Get the type
                typeName = gcnew System::String(pSymbol->GetQualifiedEmittedName());
                type = assembly->GetType(typeName, false, true);
            }
        }
    }

    return type;
}

System::Type^ SymbolMap::GetBoundGenericType(BCSYM_GenericTypeBinding* pSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pSymbol,"[SymbolMap::GetBoundGenericType] 'pSymbol' parameter is null");

    System::Type^ type = nullptr;
    System::Type^ parentType = nullptr;
    array<System::Type^>^ parentArguments = nullptr;
    BCSYM_GenericTypeBinding* parentBinding = pSymbol->GetParentBinding();
    //Need to get the actual named root of the symbol to access its emitted name
    BCSYM_NamedRoot* pNamed = pSymbol->PNamedRoot();

    //Is this a nested type of another bound generic Type?
    if(parentBinding)
    {
        //Get the concrete parent type
        parentType = GetType(parentBinding, pBindingContext);
    }

    if(parentType != nullptr)
    {
        //Get nested type
        System::String^ typeName = gcnew System::String(pNamed->GetEmittedName());
        type = parentType->GetNestedType(typeName);

        //Cache the parent's arguments for copying to the nested type
        parentArguments = parentType->GetGenericArguments();
    }
    else
    {
        //Get the type
        type = GetBasicType(pNamed);
    }

    if(type != nullptr)
    {   
        type = MakeConcrete(type, parentArguments, pSymbol, pBindingContext);
    }

    return type;
}
System::Type^ SymbolMap::MakeConcrete(System::Type^ typeDef, array<System::Type^>^ parentArguments, BCSYM_GenericTypeBinding* pSymbol, GenericBinding* pBindingContext)
{
    array<System::Type^>^ typeArguments = typeDef->GetGenericArguments();

    //Copy the parent arguments, if any
    int offset = 0;
    if(parentArguments != nullptr)
    {
        offset = parentArguments->Length;
    }
    for(int ind = 0; ind < offset; ind++)
    {
        typeArguments[ind] = parentArguments[ind];
    }

    //Get the generic arguments for this type
    for(int ind = offset; ind < typeArguments->Length; ind++)
    {
        BCSYM* arg = pSymbol->GetArgument(ind - offset);
        System::Type^ typeArg = GetType(arg, pBindingContext);
        if(typeArg != nullptr)
            typeArguments[ind] = typeArg;
    }

    //Make the type concrete
    return typeDef->MakeGenericType(typeArguments);
}

System::Type^ SymbolMap::GetGenericParamType(BCSYM_GenericParam* pSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pSymbol, "[SymbolMap::GetGenericParamType] 'pSymbol' parameter is null");
    if(pBindingContext)
    {
        BCSYM* arg = pBindingContext->GetCorrespondingArgument(pSymbol);
        ASSERT(arg, "arg");
        return GetType(arg, pBindingContext);
    }
    else if(pSymbol->GetParent())
    {
        //For the type emitters need to get the parent type and get the type parameter from that.
        System::Type^ parentType = nullptr;

        if(pSymbol->GetParent()->IsAnonymousType())
        {
            DynamicAnonymousTypeEmitter->TryGetType(pSymbol->GetParent()->PNamedRoot(), parentType);
        }
        else if(pSymbol->GetParent()->IsAnonymousDelegate())
        {
            DynamicAnonymousDelegateEmitter->TryGetType(pSymbol->GetParent()->PNamedRoot(), parentType);
        }

        if(parentType)
        {
            array<System::Type^>^ params = parentType->GetGenericArguments();
            ASSERT((UINT32)params->Length > pSymbol->GetPosition(), "[SymbolMap::GetGenericParamType] 'pSymbol' was not in it's parent's list");
            if((UINT32)params->Length > pSymbol->GetPosition())
            {
                return params[pSymbol->GetPosition()];
            }
        }
    }

    return nullptr;
}

System::Type^ SymbolMap::GetArrayType(BCSYM_ArrayType* pSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pSymbol,"[SymbolMap::GetArrayType] 'pSymbol' parameter is null");

    System::Type^ type = nullptr;
    System::Type^ rootType = nullptr;
    int rank = 0;
    BCSYM* pRoot = pSymbol->GetRoot();

    ASSERT(pRoot, "[SymbolMap::GetArrayType] 'pSymbol' parameter has no root type");

    //Get the array's root type
    rootType = GetType(pRoot, pBindingContext);
    if(rootType != nullptr)
    {
        rank = (int)pSymbol->GetRank();
        //There is a difference between a vector and a 1-dimensional multidimensional array, we need a vector
        if(rank == 1)
        {
            type = rootType->MakeArrayType();
        }
        else
        {
            type = rootType->MakeArrayType(rank);
        }
    }

    return type;
}

Assembly^ SymbolMap::GetAssembly(BCSYM_NamedRoot* pSymbol)
{  
    ASSERT(pSymbol,"[SymbolMap::GetAssembly] 'pSymbol' parameter is null");

    CompilerProject* pProj = NULL;
    AssemblyIdentity* pIdent = NULL;
    System::String^ identStr = nullptr;
    array<Assembly^>^ assemblies = nullptr;
    Assembly^ assembly = nullptr;
    bool found = false;

    pProj = pSymbol->GetContainingProject();
    ASSERT(pProj, "[SymbolMap::GetAssembly] 'pSymbol' parameter has no containing project");
    if(pProj->IsMetaData())
    {
        pIdent = pProj->GetAssemblyIdentity();
        ASSERT(pIdent, "[SymbolMap::GetAssembly] project has no AssemblyIdentity");
        identStr = gcnew System::String(pIdent->GetAssemblyIdentityString());

        assemblies = System::AppDomain::CurrentDomain->GetAssemblies();
        for(int i = 0 ; !found && i < assemblies->Length; i++)
        {
            assembly = assemblies[i];
            found = assembly->FullName->Equals(identStr, System::StringComparison::OrdinalIgnoreCase);
        }
        ASSERT(!found || assembly != nullptr, "[SymbolMap::GetAssembly] found assembly was nullptr");

        if(!found)
        {
            // If the VB runtime is required, then load it on demand.
            if (pProj->IsDefaultVBRuntime())
            {
                assembly = System::AppDomain::CurrentDomain->Load(identStr);
                ThrowIfFalse(assembly != nullptr);
            }
            else
            {
                ASSERT(false, "[SymbolMap::GetAssembly] assembly not found");
                assembly = nullptr;
            }
        }
    }

    return assembly;
}

MethodInfo^ SymbolMap::GetMethod(BCSYM_Proc * pMethodSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pMethodSymbol, "[SymbolMap::GetMethod] 'pMethodSymbol' parameter is null");
    IfFalseThrow(pMethodSymbol != NULL)

    MethodInfo^ rv = nullptr;

    if(pMethodSymbol->GetExternalSymbol())
    {
        gcroot<MethodInfo^> methodRoot;
        if (SUCCEEDED(ConvertCOMToManagedObject<MethodInfo>(pMethodSymbol->GetExternalSymbol(), methodRoot)))
        {
            ASSERT(methodRoot, "[SymbolMap::GetMethod] ConvertCOMToManagedObject SUCCEEDED, but returned nullptr");
        }
        else
        {
            ASSERT(false, "[SymbolMap::GetMethod] ConvertCOMToManagedObject failed for pMethodSymbol->GetExternalSymbol()");
            return nullptr;
        }
        
        if(pBindingContext)
        {
            //Something is generic, may need to get the type
            bool genericMethod = false;
            //Method's Type is Generic, will need to get it
            BCSYM* pTypeSymbol = NULL;

            if(pBindingContext->IsGenericTypeBinding())
            {
                //Normal method of a bound generic Type
                pTypeSymbol = pBindingContext->PGenericTypeBinding();
            }
            else
            {
                //Generic method
                genericMethod = true;
                GenericTypeBinding* pParentBinding = pBindingContext->GetParentBinding();
                if(pParentBinding)
                {
                    //Generic method of a bound generic Type
                    pTypeSymbol = pParentBinding;
                }
                else
                {
                    //Generic method of a non generic Type
                    //Don't need the type
                    rv = methodRoot;
                }
            }

            if(pTypeSymbol)
            {
                //Don't need binding context b/c pTypeSymbol is it
                System::Type^ type = GetType(pTypeSymbol, NULL);
                rv = (MethodInfo^)MethodBase::GetMethodFromHandle(methodRoot->MethodHandle, type->TypeHandle);
            }

            if(genericMethod)
            {
                //Get the generic paramaters to make the method concrete
                array<System::Type^>^ genericParams = gcnew array<System::Type^>(pMethodSymbol->GetGenericParamCount());
                BCSYM_GenericParam* param = pMethodSymbol->GetFirstGenericParam();
                for(int paramInd = 0; param; paramInd++)
                {
                    genericParams[paramInd] = GetType(param, pBindingContext);

                    param = param->GetNextParam();
                }
                //Get the concrete method
                rv = rv->MakeGenericMethod(genericParams);
            }
            return rv;
        }
        else
        {
            return methodRoot;
        }
    }
    else
    {
        BindingFlags flags;
        //Get the Methods's declaring type
        BCSYM* pTypeSymbol = NULL;
        if(pBindingContext)
        {
            //Something is generic
            if(pBindingContext->IsGenericTypeBinding())
            {
                //Normal method of a bound generic Type
                pTypeSymbol = pBindingContext->PGenericTypeBinding();
            }
            else
            {
                //Generic method
                GenericTypeBinding* pParentBinding = pBindingContext->GetParentBinding();
                if(pParentBinding)
                {
                    //Generic method of a bound generic Type
                    pTypeSymbol = pParentBinding;
                }
                else
                {
                    //Generic method of a non generic Type
                    pTypeSymbol = pMethodSymbol->GetParent();
                }
            }
        }
        else
        {
            //Normal method of a non generic Type
            pTypeSymbol = pMethodSymbol->GetParent();
        }
        //Don't need binding context b/c in generic cases pTypeSymbol is it
        System::Type^ type = GetType(pTypeSymbol, NULL);
        //Get the method's name
        System::String^ methodName = gcnew System::String(pMethodSymbol->GetEmittedName());

        //Is this a static method?
        if(pMethodSymbol->IsShared())
        {
            flags = BindingFlags::Public | BindingFlags::Static;
        }
        else
        {
            flags = BindingFlags::Public | BindingFlags::Instance;
        }

        //Is this a non generic method?
        if(!pMethodSymbol->IsGeneric())
        {
            //Get the parameter types
            array<System::Type^>^ paramTypes = GetParameterTypes(pMethodSymbol, pBindingContext);

            try
            {
                //Get the method by access and signature
                rv = type->GetMethod(methodName, flags, nullptr, paramTypes, nullptr);
            }catch(AmbiguousMatchException^) {} //will be handled by our dissambiguation code bellow
        }

        //Was this a generic method or ambiguous?
        if(rv == nullptr)
        {
            //Get the unbound methods
            array<MethodInfo^>^ methods = (type->IsGenericType ? type->GetGenericTypeDefinition() : type)->GetMethods(flags);
            MethodBase^ found = FindMethod(pMethodSymbol,
                methodName,
                methods,    //The unbound methods.
                type);      //Needed to make the methods concrete.
            ASSERT(found, "[SymbolMap::GetMethod] could not find the method");
            ASSERT(found->MemberType == MemberTypes::Method, "[SymbolMap::GetMethod] method found was not a method");
            rv = (MethodInfo^)found;
            ASSERT(rv, "[SymbolMap::GetMethod] method found could not be cast to MethodInfo");

            //Generic Methods need to be made concrete
            int genericParameterCount = pMethodSymbol->GetGenericParamCount();
            if(genericParameterCount > 0)
            {
                //Get the generic paramaters to make the method concrete
                array<System::Type^>^ genericParams = gcnew array<System::Type^>(genericParameterCount);
                BCSYM_GenericParam* param = pMethodSymbol->GetFirstGenericParam();
                for(int paramInd = 0; paramInd < genericParameterCount; paramInd++)
                {
                    genericParams[paramInd] = GetType(param, pBindingContext);

                    param = param->GetNextParam();
                }
                //Get the concrete method
                rv = rv->MakeGenericMethod(genericParams);
            }
        }

        return rv;
    }
}

ConstructorInfo^ SymbolMap::GetConstructor(BCSYM_Proc * pConstuctorSymbol, GenericTypeBinding* pBindingContext)
{
    ASSERT(pConstuctorSymbol, "[SymbolMap::GetConstructor] 'pConstuctorSymbol' parameter is null");
    IfFalseThrow(pConstuctorSymbol != NULL)

    if(pConstuctorSymbol->GetExternalSymbol())
    {
        gcroot<ConstructorInfo^> constructorRoot;
        if (SUCCEEDED(ConvertCOMToManagedObject<ConstructorInfo>(pConstuctorSymbol->GetExternalSymbol(), constructorRoot)))
        {
            ASSERT(constructorRoot, "[SymbolMap::GetConstructor] ConvertCOMToManagedObject SUCCEEDED, but returned nullptr");
        }
        else
        {
            ASSERT(false, "[SymbolMap::GetConstructor] ConvertCOMToManagedObject failed for pConstuctorSymbol->GetExternalSymbol()");
            return nullptr;
        }
        
        if(pBindingContext)
        {
            //Constructor's Type is Generic, will need to get it
            //Get the Constuctor's declaring type
            BCSYM* pTypeSymbol = pBindingContext ? pBindingContext : pConstuctorSymbol->GetParent();
            //Don't need binding context b/c in generic cases pTypeSymbol is it
            System::Type^ type = GetType(pTypeSymbol, NULL);
            return (ConstructorInfo^)MethodBase::GetMethodFromHandle(constructorRoot->MethodHandle, type->TypeHandle);
        }
        else
        {
            return constructorRoot;
        }
    }
    else
    {
        //Get the Constuctor's declaring type
        BCSYM* pTypeSymbol = pBindingContext ? pBindingContext : pConstuctorSymbol->GetParent();
        //Don't need binding context b/c in generic cases pTypeSymbol is it
        System::Type^ type = GetType(pTypeSymbol, NULL);

        //Get the parameter types
        array<System::Type^>^ paramTypes = GetParameterTypes(pConstuctorSymbol, pBindingContext);

        try
        {
            //Try to get the constructor by its parameters
            return type->GetConstructor(paramTypes);
        }
        catch(AmbiguousMatchException^)
        {
            //For generic cases the parameters may be ambiguous, use the cutsom find method.
            //Get the unbound constructors 
            array<ConstructorInfo^>^ constructors = (type->IsGenericType ? type->GetGenericTypeDefinition() : type)->GetConstructors(); 
            MethodBase^ found = FindMethod(pConstuctorSymbol,
                nullptr,      //Name is not needed for finding constructors.
                constructors, //The unbound constructors 
                type);        //Needed to make the constructor concrete.
            ASSERT(found, "[SymbolMap::GetConstructor] could not find the constructor");
            ASSERT(found->MemberType == MemberTypes::Constructor, "[SymbolMap::GetConstructor] constructor found was not a constructor");
            ConstructorInfo^ rv = (ConstructorInfo^)found;
            ASSERT(rv, "[SymbolMap::GetConstructor] constructor found could not be cast to ConstructorInfo");
            return rv;
        }
    }
}

MethodBase^ SymbolMap::FindMethod(BCSYM_Proc * pProcSymbol, System::String^ name, array<MethodBase^>^ methods, System::Type^ type)
{
    //Consider: Performance: using GetMember(name) instead may be faster.

    //Cache the number of generic parameters for filtering
    int genericParameterCount = pProcSymbol->GetGenericParamCount();
    //Cache the number of parameters for filtering
    int parameterCount = pProcSymbol->GetParameterCount();
    //Cache genericness for filtering
    bool isGeneric = pProcSymbol->IsGeneric();

    //Search through the methods for a method w/ a signature that matches

    for(int ind = 0 ; ind < methods->Length; ind++)
    {
        MethodBase^ testInfo = methods[ind];
        //For methods filter by the name first
        if(testInfo->MemberType != MemberTypes::Method || System::String::Equals(testInfo->Name, name, System::StringComparison::Ordinal))
        {
            //Get the open parameter list of the method checked
            array<ParameterInfo^>^ testParams = testInfo->GetParameters();
            if( testInfo->IsGenericMethod  == (genericParameterCount > 0) && //Does the genericness match?
                (genericParameterCount == 0 || testInfo->GetGenericArguments()->Length == genericParameterCount) && //Does it have the right number of generic parameters?
                testParams->Length == parameterCount)                       //Does it have the right number of parameters?
            {
                //Compare signatures

                //Methods require some additional checks
                if(testInfo->MemberType == MemberTypes::Method)
                {
                    //Compare Method Name and return type
                    //Return types need to be matched because overloads of conversion operator
                    //methods (op_Explicit, op_Implicit) can differ only by return type too.
                    MethodInfo^ methodInfo = (MethodInfo^)testInfo;
                    if(!(pProcSymbol->GetType() == NULL ?
                            System::Void::typeid->Equals(methodInfo->ReturnType) :
                            Equals(pProcSymbol->GetType(), methodInfo->ReturnType)
                        ))
                    {
                        continue;
                    }
                }
                else if(pProcSymbol->GetType() != NULL)
                {
                    //Constructors can not have a return type
                    continue;
                }

                //Compare parameter types
                BCSYM_Param *pParam = pProcSymbol->GetFirstParam();
                bool match = true;
                for(int paramInd = 0; paramInd < testParams->Length && match; paramInd++)
                {
                    //Are the parameters of the same type (includes byref check)?
                    match = Equals(pParam->GetType(), testParams[paramInd]->ParameterType);

                    pParam = pParam->GetNext();
                }

                //Does it match?
                if(match)
                {
                    if(type->IsGenericType)
                    {
                        //Get the implementation of the method for the Concrete Type.
                        return MethodBase::GetMethodFromHandle(testInfo->MethodHandle, type->TypeHandle);
                    }
                    else
                    {
                        return testInfo;
                    }
                }
            }
        }
    }

    return nullptr;
}

array<System::Type^>^ SymbolMap::GetParameterTypes(BCSYM_Proc * pMethodSymbol, GenericBinding* pBindingContext)
{
    ASSERT(pMethodSymbol, "[SymbolMap::GetParameterTypes] 'pMethodSymbol' parameter is null");

    int parameterCount = pMethodSymbol->GetParameterCount();
    array<System::Type^>^ paramTypes = gcnew array<System::Type^>(parameterCount);

    BCSYM_Param* pParam = pMethodSymbol->GetFirstParam();
    for (int ind = 0; ind  < parameterCount; ind++)
    {
        paramTypes[ind] = GetType(pParam->GetType(), pBindingContext);
        pParam = pParam->GetNext();
    }

    ASSERT(pParam == NULL, "[SymbolMap::GetParameterTypes] unmanged parameter list was longer than its reported length");

    return paramTypes;
}

bool SymbolMap::Equals(BCSYM* pSymbol, System::Type^ type)
{
    ASSERT(pSymbol, "[SymbolMap::Equals] 'pSymbol' parameter is null");
    ASSERT(pSymbol->IsType(), "[SymbolMap::Equals] 'pSymbol' parameter is not a type");
    ASSERT(type != nullptr, "[SymbolMap::Equals] 'type' parameter is not a type");

    IfFalseThrow(pSymbol != NULL);
    IfFalseThrow(type != nullptr);

    bool rv = false;

    if(pSymbol->IsNamedRoot())
    {
        //pSymbol is a Type
        if(pSymbol->IsGenericParam())
        {
            //pSymbol is a generic parameter
            rv = EqualsGenericParamType(pSymbol->PGenericParam(), type);
        }
        else if(pSymbol->IsGenericTypeBinding())
        {
            //pSymbol is a bound generic type
            rv = EqualsBoundGenericType(pSymbol->PGenericTypeBinding(), type);
        }
        else if(!type->IsGenericParameter)
        {
            //pSymbol is a basic type and type is comparable (Generic Parameters can not be compared b/c they do not have Full Names)
            rv = EqualsBasicType(pSymbol->PNamedRoot(), type);
        }
        else
        {
            //pSymbol is a basic type but type is a generic parameter
            rv = false;
        }
    }
    else if(pSymbol->IsArrayType())
    {
        //pSymbol is an Array
        rv = EqualsArrayType(pSymbol->PArrayType(), type);
    }
    else if(pSymbol->IsPointerType())
    {
        //pSymbol is ByRef
        //Make sure the type is also ByRef and that they have the same root Type
        rv = type->IsByRef && Equals(pSymbol->PPointerType()->GetRoot(), type->GetElementType());
    }
    else
    {
        //Unkown case
        IfFalseThrow(false);
    }

    return rv;
}

bool SymbolMap::EqualsBasicType(BCSYM_NamedRoot* pSymbol, System::Type^ type)
{
    ASSERT(pSymbol, "[SymbolMap::EqualsBasicType] 'pSymbol' parameter is null");
    ASSERT(pSymbol->IsType(), "[SymbolMap::EqualsBasicType] 'pSymbol' parameter is not a type");
    ASSERT(type != nullptr, "[SymbolMap::EqualsBasicType] 'type' parameter is not a type");
    ASSERT(!type->IsGenericParameter, "[SymbolMap::EqualsBasicType] 'type' parameter must not be a Generic Parameter");
    IfTrueThrow(type->IsGenericParameter);

    bool rv = false;
    CompilerProject* pProj = NULL;
    AssemblyIdentity* pIdent = NULL;
    System::String^ identStr = nullptr;
    
    pProj = pSymbol->GetContainingProject();
    ASSERT(pProj, "[SymbolMap::EqualsBasicType] 'pSymbol' parameter has no containing project");
    if(pProj->IsMetaData())
    {
        pIdent = pProj->GetAssemblyIdentity();
        ASSERT(pIdent, "[SymbolMap::EqualsBasicType] project has no AssemblyIdentity");
        identStr = gcnew System::String(pIdent->GetAssemblyIdentityString());
        rv = type->Assembly->FullName->Equals(identStr); //Do the assemblies match
    }

    //Should check in more depth if the assembly identity matches or if a host-provided symbol.
    if(rv || pSymbol->GetExternalSymbol())
    {
        BCSYM_NamedRoot* pParentSymbol = pSymbol->GetParent();
        if(pParentSymbol && pParentSymbol->IsType())
        {
            //Nested Type
            rv = type->DeclaringType != nullptr && //Is type also a Nested Type?
                Equals(pParentSymbol, type->DeclaringType) && //Are they nested from the same type?
                type->Name->Equals(gcnew System::String(pSymbol->GetEmittedName())); //Are they the same nested type?
        }
        else
        {
            //Not a nested type
            //Are they the same type?
            rv = type->FullName->Equals(gcnew System::String(pSymbol->GetQualifiedEmittedName()));
        }
    }

    return rv;
}

bool SymbolMap::EqualsBoundGenericType(BCSYM_GenericTypeBinding* pSymbol, System::Type^ type)
{
    ASSERT(pSymbol, "[SymbolMap::EqualsBoundGenericType] 'pSymbol' parameter is null");
    ASSERT(type != nullptr, "[SymbolMap::EqualsBoundGenericType] 'type' parameter is not a type");

    //Is type also a generic type?
    bool rv = type->IsGenericType;
    if(rv)
    {
        int offset = 0;

        BCSYM_GenericTypeBinding* pParentBinding = pSymbol->GetParentBinding();
        if(pParentBinding)
        {
            //Nested Type of a genric type
            System::Type^ parent = type->DeclaringType;
            if(parent == nullptr)
            {
                //pSymbol is a nested type but type is not
                rv = false;
            }
            else
            {
                //Offset is used to check just the current level's params in this context, parents are handled by the recursive call bellow
                offset = parent->GetGenericArguments()->Length;
                //Make parent concrete
                array<System::Type^>^ types = gcnew array<System::Type^>(offset);
                System::Array::ConstrainedCopy(type->GetGenericArguments(), 0, types, 0, offset);
                parent = parent->MakeGenericType(types);

                rv = Equals(pParentBinding, parent) && //Are they nested from the same type?
                    type->Name->Equals(gcnew System::String(pSymbol->PNamedRoot()->GetEmittedName())); //Are they the same nested type?
            }
        }
        else
        {
            //No need for special parent handeling
            //Are their type definitions the same?
            rv = EqualsBasicType(pSymbol->PNamedRoot(), type->GetGenericTypeDefinition());
        }

        if(rv)
        {
            //Compare the generic parameters
            array<System::Type^>^ typeArguments = type->GetGenericArguments();
            rv = typeArguments->Length == pSymbol->GetGenericParamCount() + offset;
            if(rv)
            {
                for(int argInd = offset; argInd < typeArguments->Length && rv; argInd++)
                {
                    rv = Equals(pSymbol->GetArgument(argInd - offset), typeArguments[argInd]);
                }
            }
        }
    }
    return rv;
}

bool SymbolMap::EqualsGenericParamType(BCSYM_GenericParam* pSymbol, System::Type^ type)
{
    ASSERT(pSymbol, "[SymbolMap::EqualsGenericParamType] 'pSymbol' parameter is null");
    ASSERT(type != nullptr, "[SymbolMap::EqualsGenericParamType] 'type' parameter is not a type");

    //Is type also a generic parameter?
    bool rv = type->IsGenericParameter;
    if(rv)
    {
        //Get Psymbol's Declaring Types
        BCSYM_NamedRoot* parentSymbol = pSymbol->GetParent();
        ASSERT(parentSymbol, "[SymbolMap::EqualsGenericParamType] 'pSymbol' does not have a parent type");

        //Cache position of type
        int position = type->GenericParameterPosition;

        //Do not need to check parent of Generic Parameter's of Method's as we can only be in one Method's context.
        if(parentSymbol->IsType() && type->DeclaringMethod == nullptr)
        {
            //Both parents are types
            int offset = 0;

            //Generic parameter of a Type
            System::Type^ last = type->DeclaringType;
            System::Type^ parent = last->DeclaringType;

            if(parent != nullptr)
            {
                //Generic Nested Type
                //For Generic Nested Types need to search through hierarchy to find the actual type that declared the Generic Parameter to compare with the symbol.
                offset = parent->GetGenericArguments()->Length;
                while(offset > position)
                {
                    last = parent;
                    parent = parent->DeclaringType;
                    if(parent == nullptr)
                    {
                        //last is the Declaring Type of type and is also top of the hierarchy
                        offset = 0;
                    }
                    else
                    {
                        offset = parent->GetGenericArguments()->Length;
                    }
                }
                //last is the Declaring Type of type
            }
            
            rv = pSymbol->GetPosition() + offset == position && //Do they have the same position
                 EqualsBasicType(parentSymbol, last);           //Are they from the same type
        }
        else if(parentSymbol->IsProc())
        {
            //Both parents are methods
            rv = pSymbol->GetPosition() == position;
        }
        else
        {
            //parent missmatch
            rv = false;
        }
    }
    return rv;
}

bool SymbolMap::EqualsArrayType(BCSYM_ArrayType* pSymbol, System::Type^ type)
{
    ASSERT(pSymbol, "[SymbolMap::EqualsArrayType] 'pSymbol' parameter is null");
    ASSERT(type != nullptr, "[SymbolMap::EqualsArrayType] 'type' parameter is not a type");

    return type->IsArray && //Is type also an array?
        pSymbol->GetRank() == type->GetArrayRank() && //Do they have the same rank?
        Equals(pSymbol->GetRoot(), type->GetElementType()); //Do they have the same Element Type?
}

System::Reflection::FieldInfo^ SymbolMap::GetField(BCSYM_Variable * pFieldSymbol, GenericTypeBinding* pBindingContext)
{
    ASSERT(pFieldSymbol, "[SymbolMap::GetField] 'pFieldSymbol' parameter is null");
    ASSERT(!pFieldSymbol->IsFromScriptScope(), "[SymbolMap::GetField] 'pFieldSymbol' is from Script Scope");
    IfFalseThrow(pFieldSymbol != NULL);

    // Bug 604017: linked to Dev10 604014, resolved as Duplicate.
    // Removed code for short circuiting lookup for TypeScope-provided Types.
    // Leaving code in unnecessary scope to minimize code churn.
    {
        BindingFlags flags;

        //Get the Fields's declaring type
        BCSYM* pTypeSymbol = pBindingContext ? pBindingContext : pFieldSymbol->GetParent();
        //Don't need binding context b/c in generic cases pTypeSymbol is it
        System::Type^ type = GetType(pTypeSymbol, NULL);
        //Get the field's name
        System::String^ fieldName = gcnew System::String(pFieldSymbol->GetEmittedName());

        //Is this a static field?
        if(pFieldSymbol->IsShared())
        {
            flags = BindingFlags::Public | BindingFlags::Static;
        }
        else
        {
            flags = BindingFlags::Public | BindingFlags::Instance;
        }

        return type->GetField(fieldName, flags);
    }
}
