#include "stdAfx.h"

using namespace System::Reflection;
using namespace System::Reflection::Emit;
using namespace Microsoft::Compiler::VisualBasic;

/*









*/

static TypeEmitter::TypeEmitter()
{
    VoidType = System::Void::typeid;
}

TypeEmitter::TypeEmitter(SymbolMap^ symbolMap, ModuleBuilder^ moduleBuilder) :
    m_moduleBuilder(moduleBuilder),
    m_symbolMap(symbolMap)
{
    ASSERT(symbolMap, "[TypeEmitter::TypeEmitter`1] 'symbolMap' parameter is null");
    ASSERT(moduleBuilder, "[TypeEmitter::TypeEmitter`1] 'moduleBuilder' parameter is null");
}

ModuleBuilder^ TypeEmitter::DynamicModuleBuilder::get()
{
    return m_moduleBuilder;
}

TypeBuilder^ TypeEmitter::DefineType(BCSYM_NamedRoot* pType)
{
    ASSERT(pType, "[TypeEmitter::DefineType] 'pType' parameter is null");
    //

    System::String^ name = gcnew System::String(pType->GetEmittedName());
    TypeAttributes attributes = GetTypeAttributes(pType);
    TypeBuilder^ typeBuilder = nullptr;
    if (pType->IsClass() && pType->PClass()->GetBaseClass())
    {
        System::Type^ baseType = m_symbolMap->GetType(pType->PClass()->GetBaseClass());
        ASSERT(baseType, "[TypeEmitter::DefineType] could not load the base type.");
        typeBuilder = DynamicModuleBuilder->DefineType(name, attributes, baseType);
    }
    else
    {
        typeBuilder = DynamicModuleBuilder->DefineType(name, attributes);
    }

    if (pType->IsGeneric())
    {
        //
        int genericParamCount = pType->GetGenericParamCount();
        array<System::String^>^ names = gcnew array<System::String^>(genericParamCount);

        //Get the name sof the generic params
        BCSYM_GenericParam* genericParam = pType->GetFirstGenericParam();
        for (int index = 0 ; index < genericParamCount; index++)
        {
            ASSERT(genericParam, "[TypeEmitter::DefineType] the number of generic params was smaller than reported");
            names[index] = gcnew System::String(genericParam->GetEmittedName());
            genericParam = genericParam->GetNextParam();
        }
        ASSERT(genericParam == NULL, "[TypeEmitter::DefineType] the number of generic params was larger than reported");

        //Make the type builder generic
        array<GenericTypeParameterBuilder^>^ genericParams = typeBuilder->DefineGenericParameters(names);

        //Set the metadata for each of the params
        genericParam = pType->GetFirstGenericParam();
        for (int index = 0 ; index < genericParamCount; index++)
        {
            ASSERT(genericParam, "[TypeEmitter::DefineType] the number of generic params was smaller than reported");
            DefineGenericTypeParameter(typeBuilder, genericParams[index], genericParam);
            genericParam = genericParam->GetNextParam();
        }
        ASSERT(genericParam == NULL, "[TypeEmitter::DefineType] the number of generic params was larger than reported");

    }
    
    return typeBuilder;
}
void TypeEmitter::DefineGenericTypeParameter(TypeBuilder^ typeBuilder, GenericTypeParameterBuilder^ paramBuilder, BCSYM_GenericParam* pParam)
{
    ASSERT(typeBuilder, "[TypeEmitter::DefineGenericTypeParameter] 'typeBuilder' parameter is null");
    ASSERT(paramBuilder, "[TypeEmitter::DefineGenericTypeParameter] 'paramBuilder' parameter is null");
    ASSERT(pParam, "[TypeEmitter::DefineGenericTypeParameter] 'pParam' parameter is null");

    //Set the attributes
    paramBuilder->SetGenericParameterAttributes(GetGenericParameterAttributes(pParam));
    //Get the list of interface constraints and the type constraint if present
    //Other constraints such as New() are handled in the attributes
    System::Collections::Generic::List<System::Type^>^ interfaces = gcnew System::Collections::Generic::List<System::Type^>();
    for (BCSYM_GenericConstraint* pConstraint = pParam->GetConstraints(); pConstraint; pConstraint = pConstraint->Next())
    {
        if (pConstraint->IsGenericTypeConstraint())
        {
            BCSYM_GenericTypeConstraint* pTypeConstraint = pConstraint->PGenericTypeConstraint();
            BCSYM* pType = pType = pTypeConstraint->GetType();
            System::Type^ type = GetType(typeBuilder, pType);
            ASSERT(type, "[TypeEmitter::DefineGenericTypeParameter] could not load type constraint's type");
            if(pType->IsInterface())
            {
                interfaces->Add(type);
            }
            else
            {
                paramBuilder->SetBaseTypeConstraint(type);
            }
        }
    }
    //If interface constraints were found set them
    if(interfaces->Count > 0)
    {
        paramBuilder->SetInterfaceConstraints(interfaces->ToArray());
    }
}

FieldBuilder^ TypeEmitter::DefineField(TypeBuilder^ typeBuilder, BCSYM_Variable* pField)
{
    ASSERT(typeBuilder, "[TypeEmitter::DefineField] 'typeBuilder' parameter is null");
    ASSERT(pField, "[TypeEmitter::DefineField] 'pField' parameter is null");

    System::String^ name = gcnew System::String(pField->GetEmittedName());
    System::Type^ type = GetType(typeBuilder, pField->GetType());
    FieldAttributes attributes = GetFieldAttributes(pField);

    return typeBuilder->DefineField(name, type, attributes); 
}

PropertyBuilder^ TypeEmitter::DefineProperty(TypeBuilder^ typeBuilder, BCSYM_Property* pProperty)
{
    ASSERT(typeBuilder, "[TypeEmitter::DefineProperty] 'typeBuilder' parameter is null");
    ASSERT(pProperty, "[TypeEmitter::DefineProperty] 'pProperty' parameter is null");

    System::String^ name = gcnew System::String(pProperty->GetEmittedName());
    System::Type^ type = GetType(typeBuilder, pProperty->GetType());
    PropertyAttributes attributes = GetPropertyAttributes(pProperty);
    array<System::Type^>^ parameterTypes = GetParameterTypes(typeBuilder, pProperty);
    BCSYM_Proc* accessorSymbol;
    MethodBuilder^ accessorDefenition;

    PropertyBuilder^ propertyBuilder = typeBuilder->DefineProperty(name, attributes, type, parameterTypes);
    if(pProperty->GetProperty())
    {
        accessorSymbol = pProperty->GetProperty();
        accessorDefenition = DefineMethod(typeBuilder, accessorSymbol);
        propertyBuilder->SetGetMethod(accessorDefenition);
    }
    if(pProperty->SetProperty())
    {
        accessorSymbol = pProperty->SetProperty();
        accessorDefenition = DefineMethod(typeBuilder, accessorSymbol);
        propertyBuilder->SetSetMethod(accessorDefenition);
    }

    return propertyBuilder;
}

MethodBuilder^ TypeEmitter::DefineMethod(TypeBuilder^ typeBuilder, BCSYM_Proc* pProc)
{
    ASSERT(typeBuilder, "[TypeEmitter::DefineMethod] 'typeBuilder' parameter is null");
    ASSERT(pProc, "[TypeEmitter::DefineMethod] 'pProc' parameter is null");

    System::String^ name = gcnew System::String(pProc->GetEmittedName());
    System::Type^ type = GetType(typeBuilder, pProc->GetType());
    MethodAttributes attributes = GetMethodAttributes(pProc);
    array<System::Type^>^ parameterTypes = GetParameterTypes(typeBuilder, pProc);

    return typeBuilder->DefineMethod(name, attributes, type, parameterTypes);
}

ConstructorBuilder^ TypeEmitter::DefineConstructor(TypeBuilder^ typeBuilder, BCSYM_Proc* pProc)
{
    ASSERT(typeBuilder, "[TypeEmitter::DefineConstructor] 'typeBuilder' parameter is null");
    ASSERT(pProc, "[TypeEmitter::DefineConstructor] 'pProc' parameter is null");

    MethodAttributes attributes = GetMethodAttributes(pProc);
    array<System::Type^>^ parameterTypes = GetParameterTypes(typeBuilder, pProc);

    //
    ConstructorBuilder^ ctorBuilder = typeBuilder->DefineConstructor(attributes, CallingConventions::Standard, parameterTypes);
    return ctorBuilder;
}

//Based off of src\vb\Language\Compiler\CodeGen\PEBuilder.cpp PEBuilder::GetTypeDefFlags
TypeAttributes TypeEmitter::GetTypeAttributes(BCSYM_NamedRoot* pType)
{
    ASSERT(pType, "[TypeEmitter::GetTypeAttributes] 'pType' parameter is null");

    TypeAttributes attributes = (TypeAttributes)0;

    if (pType->GetPWellKnownAttrVals()->GetComImportData())
    {
        attributes = attributes | TypeAttributes::Import;
    }
    //
    /*if (TypeHelpers::IsNoPIAType(pType, m_pCompiler))
    {
        attributes = attributes | TypeAttributes::Import;
    }*/

    // Gather the information.
    if(pType->IsClass())
    {
        BCSYM_Class *pClass = pType->PClass();

        if (pClass->IsEnum())
        {
            attributes = attributes | TypeAttributes::Sealed;
        }
        else if (pClass->IsStruct())
        {
            attributes = attributes | TypeAttributes::Sealed;
            // The user may specify a struct layout using a pseudo-custom attribute.
            // If the user did not specify a custom struct layout, use this symbol's default layout.
            __int16 layoutFlags;
            if (pClass->GetPWellKnownAttrVals()->GetStructLayoutData(&layoutFlags))
            {
                AssertIfFalse((TypeAttributes)0 == ((TypeAttributes)layoutFlags & ~TypeAttributes::LayoutMask));
                if ((TypeAttributes)layoutFlags != TypeAttributes::LayoutMask)
                {
                    attributes = attributes | ((TypeAttributes)layoutFlags & TypeAttributes::LayoutMask);
                }
                else
                {
                    attributes = attributes | TypeAttributes::SequentialLayout;
                }
            }
            else
            {
                attributes = attributes | TypeAttributes::SequentialLayout;
            }
        }
        else
        {
            // flags
            if (pClass->IsNotInheritable() || pClass->IsStdModule())
            {
                attributes = attributes | TypeAttributes::Sealed;
            }

            if ( !pClass->AreMustOverridesSatisfied() || pClass->IsMustInherit())
            {
                attributes = attributes | TypeAttributes::Abstract;
            }
        }

        // 


        //// If the .cctor is synthetic, then we're only initializing fields,
        //// so it's ok not to run it until the first shared field is accessed
        //if (SharedConstructor && SharedConstructor->IsSyntheticMethod())
        //{
        //    attributes = attributes | TypeAttributes::BeforeFieldInit;
        //}
    }
    else if(pType->IsInterface())
    {
        // flags
        attributes = attributes | TypeAttributes::Interface | TypeAttributes::Abstract;
    }
    else
    {
        ASSERT(false, "[TypeEmitter::GetTypeAttributes] What other kinds of containers do we have?");
    }

    // Shared flags.
    if (pType->GetContainer() && pType->GetContainer()->IsClass())
    {
        // Set the access flags.
        switch(pType->GetAccess())
        {
        case ACCESS_Public:
            attributes = attributes | TypeAttributes::NestedPublic;
            break;

        case ACCESS_Protected:
            attributes = attributes | TypeAttributes::NestedFamily;
            break;

        case ACCESS_Private:
            attributes = attributes | TypeAttributes::NestedPrivate;
            break;

        case ACCESS_Friend:
            attributes = attributes | TypeAttributes::NestedAssembly;
            break;

        case ACCESS_ProtectedFriend:
            attributes = attributes | TypeAttributes::NestedFamORAssem;
            break;

        default:
            ASSERT(false, "[TypeEmitter::GetTypeAttributes] Unexpected access.");
        }
    }
    else
    {
        // Set the access flags.
        switch(pType->GetAccess())
        {
        case ACCESS_Public:
            attributes = attributes | TypeAttributes::Public;
            break;

        case ACCESS_Protected:
            ASSERT(false, "[TypeEmitter::GetTypeAttributes] Can't have a non-nested protected type.");
            attributes = attributes | TypeAttributes::Public;
            break;

        case ACCESS_Private:
            attributes = attributes | TypeAttributes::NotPublic;
            break;

        case ACCESS_Friend:
            attributes = attributes | TypeAttributes::NotPublic;
            break;

        case ACCESS_ProtectedFriend:
            ASSERT(false, "[TypeEmitter::GetTypeAttributes] Can't have a non-nested protected type.");
            attributes = attributes | TypeAttributes::Public;
            break;

        default:
            ASSERT(false, "[TypeEmitter::GetTypeAttributes] Unexpected access.");
        }
    }

    return attributes;
}

//Based off of src\vb\Language\Compiler\CodeGen\MetaEmit.cpp MetaEmit::EmitGenericParam
GenericParameterAttributes TypeEmitter::GetGenericParameterAttributes(BCSYM_GenericParam* pParam)
{
    ASSERT(pParam, "[TypeEmitter::GetGenericParameterAttributes] 'pParam' parameter is null");

    BCSYM_GenericConstraint* pConstraint;
    GenericParameterAttributes attributes = GenericParameterAttributes::None;

    switch (pParam->GetVariance())
    {
        case Variance_In:
            attributes = attributes | GenericParameterAttributes::Contravariant;
            break;
        case Variance_Out:
            attributes = attributes | GenericParameterAttributes::Covariant;
            break;
        case Variance_None:
            //No Op
            break;
        default:
            ASSERT(false, "[TypeEmitter::GetGenericParameterAttributes] Unexpected variance in meta-emit");
    }

    for (pConstraint = pParam->GetConstraints(); pConstraint; pConstraint = pConstraint->Next())
    {
        if (pConstraint->IsNewConstraint())
        {
            attributes = attributes | GenericParameterAttributes::DefaultConstructorConstraint;
        }
        else if (pConstraint->IsReferenceConstraint())
        {
            attributes = attributes | GenericParameterAttributes::ReferenceTypeConstraint;
        }
        else if (pConstraint->IsValueConstraint())
        {
            attributes = attributes | GenericParameterAttributes::NotNullableValueTypeConstraint;
        }
    }

    return attributes;
}

//Based off of src\vb\Language\Compiler\CodeGen\MetaEmit.cpp MetaEmit::DefineField
FieldAttributes TypeEmitter::GetFieldAttributes(BCSYM_Variable* pField)
{
    ASSERT(pField, "[TypeEmitter::GetFieldAttributes] 'pField' parameter is null");

    FieldAttributes attributes = (FieldAttributes)0;

    if (pField->GetVarkind() == VAR_Const)
    {
        if (TypeHelpers::IsDecimalType(pField->GetType()) ||
            TypeHelpers::IsDateType(pField->GetType()))
        {
            // Decimal and date constant fields are special because there is no
            // way to express their value in metadata. So other languages
            // don't barf, we make them initonly rather than literal
            attributes = attributes | FieldAttributes::InitOnly;
        }
        else
        {
            attributes = attributes | FieldAttributes::Literal;
        }

        attributes = attributes | FieldAttributes::Static;
    }
    else
    {
        if (pField->IsShared())
        {
            attributes = attributes | FieldAttributes::Static;
        }
    }

    if (pField->IsReadOnly())
    {
        attributes = attributes | FieldAttributes::InitOnly;
    }

    switch(pField->GetAccess())
    {
    case ACCESS_Public:
        attributes = attributes | FieldAttributes::Public;
        break;

    case ACCESS_Protected:
        attributes = attributes | FieldAttributes::Family;
        break;

    case ACCESS_Private:
        attributes = attributes | FieldAttributes::Private;
        break;

    case ACCESS_Friend:
        attributes = attributes | FieldAttributes::Assembly;
        break;

    case ACCESS_ProtectedFriend:
        attributes = attributes | FieldAttributes::FamORAssem;
        break;

    default:
        ASSERT(false, "[TypeEmitter::GetGenericParameterAttributes] Unexpected access.");
    }
    return attributes;
}

PropertyAttributes TypeEmitter::GetPropertyAttributes(BCSYM_Property* pProperty)
{
    ASSERT(pProperty, "[TypeEmitter::GetPropertyAttributes] 'pProperty' parameter is null");

    if(pProperty->IsSpecialName())
    {
        return PropertyAttributes::SpecialName;
    }
    else
    {
        return PropertyAttributes::None;
    }
}

//Based off of src\vb\Language\Compiler\CodeGen\MetaEmit.cpp MetaEmit::DefineMethod
MethodAttributes TypeEmitter::GetMethodAttributes(BCSYM_Proc* pProc)
{
    ASSERT(pProc, "[TypeEmitter::GetMethodAttributes] 'pProc' parameter is null");

    MethodAttributes attributes = (MethodAttributes)0;

    if (pProc->IsMustOverrideKeywordUsed())
    {
        attributes = attributes | MethodAttributes::Abstract;
    }

    if (pProc->IsShared())
    {
        attributes = attributes | MethodAttributes::Static;
    }

    if ( pProc->IsVirtual() )
    {
        attributes = attributes | MethodAttributes::Virtual;

        if (pProc->CheckAccessOnOverride())
        {
            // Disallow users from overriding this method if they lack the accessbility to do so.
            attributes = attributes | MethodAttributes::CheckAccessOnOverride;
        }

        // If there is a shadowing method between an overriding method and the
        // method it overrides, we have to emit it as a newslot and then emit
        // an explicit methodimpl. If we don't, we'll end up overriding the shadowed
        // method.
        if ((!pProc->IsOverridesKeywordUsed() &&
             !pProc->IsNotOverridableKeywordUsed() &&
             !pProc->IsOverrides()) ||
            (pProc->IsOverrides() &&
             pProc->OverridesRequiresMethodImpl()))
        {
            VSASSERT(!pProc->IsShared(), "How did a Shared method get marked as Overriding another?");
            attributes = attributes | MethodAttributes::NewSlot;
        }
    }

    if (pProc->IsNotOverridableKeywordUsed() ||
        ((pProc->IsImplementing() ||
            pProc->IsImplementingComClassInterface()) &&
         !pProc->IsOverridesKeywordUsed() &&
         !pProc->IsMustOverrideKeywordUsed() &&
         !pProc->IsOverridableKeywordUsed() &&
         !pProc->IsNotOverridableKeywordUsed()))
    {
        attributes = attributes | MethodAttributes::Final;
    }

    if (pProc->IsSpecialName())
    {
        attributes = attributes | MethodAttributes::SpecialName;
    }

    if (pProc->IsOverloadsKeywordUsed() && !pProc->IsShadowing())
    {
        attributes = attributes | MethodAttributes::HideBySig;
    }

    // Declare.
    if (pProc->IsDllDeclare())
    {
        attributes = attributes | MethodAttributes::Static | MethodAttributes::PinvokeImpl;
    }

    // Set the access.
    switch(pProc->GetAccess())
    {
    case ACCESS_Public:
        attributes = attributes | MethodAttributes::Public;
        break;

    case ACCESS_Protected:
        attributes = attributes | MethodAttributes::Family;
        break;

    case ACCESS_Private:
        attributes = attributes | MethodAttributes::Private;
        break;

    case ACCESS_Friend:
        attributes = attributes | MethodAttributes::Assembly;
        break;

    case ACCESS_ProtectedFriend:
        attributes = attributes | MethodAttributes::FamORAssem;
        break;

    case ACCESS_CompilerControlled:
        attributes = attributes | MethodAttributes::PrivateScope;
        break;

    default:
        ASSERT(false, "[TypeEmitter::GetGenericParameterAttributes] Unexpected access.");
    }

    return attributes;
}

System::Type^ TypeEmitter::GetType(TypeBuilder^ typeBuilder, BCSYM * pSymbol)
{
    ASSERT(typeBuilder, "[TypeEmitter::GetType] 'typeBuilder' parameter is null");

    if(pSymbol == NULL)
    {
        //compiler denotes void as NULL
        return VoidType;
    }
    return m_symbolMap->GetType(pSymbol);
}

array<System::Type^>^ TypeEmitter::GetParameterTypes(TypeBuilder^ typeBuilder, BCSYM_Proc * pProc)
{
    ASSERT(typeBuilder, "[TypeEmitter::GetParameterTypes] 'typeBuilder' parameter is null");
    ASSERT(pProc, "[TypeEmitter::GetParameterTypes] 'pProc' parameter is null");

    int parameterCount = pProc->GetParameterCount();
    array<System::Type^>^ paramTypes = gcnew array<System::Type^>(parameterCount);

    BCSYM_Param* pParam = pProc->GetFirstParam();
    for (int ind = 0; ind  < parameterCount; ind++)
    {
        ASSERT(pParam, "[TypeEmitter::GetParameterTypes] unmanged parameter list was shorter than its reported length");
        paramTypes[ind] = GetType(typeBuilder, pParam->GetType());
        pParam = pParam->GetNext();
    }
    ASSERT(pParam == NULL, "[TypeEmitter::GetParameterTypes] unmanged parameter list was longer than its reported length");

    return paramTypes;
}
