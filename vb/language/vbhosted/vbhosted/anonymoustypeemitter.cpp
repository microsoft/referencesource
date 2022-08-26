#include "stdAfx.h"

using namespace System::Reflection;
using namespace System::Reflection::Emit;
using namespace Microsoft::Compiler::VisualBasic;

static AnonymousTypeEmitter::AnonymousTypeEmitter()
{
    StringType = System::String::typeid;
    ObjectType = System::Object::typeid;
    StringBuilderType = System::Text::StringBuilder::typeid;
}

AnonymousTypeEmitter::AnonymousTypeEmitter(SymbolMap^ symbolMap, ModuleBuilder^ moduleBuilder) : 
    TypeEmitter(symbolMap, moduleBuilder)
{
    m_types = gcnew System::Collections::Generic::Dictionary<System::String^,System::Type^>();
}

bool AnonymousTypeEmitter::TryGetType(BCSYM_NamedRoot* pSymbol, System::Type^% type)
{
    ASSERT(pSymbol, "[AnonymousTypeEmitter::TryGetType] 'pSymbol' parameter is null");

    System::String^ name = gcnew System::String(pSymbol->GetEmittedName());
    return m_types->TryGetValue(name, type);
}

System::Type^ AnonymousTypeEmitter::EmitType(BCSYM_NamedRoot* pAnonymousType)
{
    ASSERT(pAnonymousType, "[AnonymousTypeEmitter::EmitType] 'pAnonymousType' parameter is null");
    ASSERT(pAnonymousType->IsAnonymousType(), "[AnonymousTypeEmitter::EmitType] 'pAnonymousType' parameter was not an Anonymous Type");

    //Assumption there is a 1 to 1 mapping between Generic Parameters and Properties field combinations

    TypeBuilder^ typeBuilder = DefineType(pAnonymousType->PNamedRoot());
    System::Type^ type = nullptr;

    m_types->Add(typeBuilder->Name, typeBuilder);

    //If there is a key need to define Equals and GetHashCode methods
    bool hasKey = false;

    int paramCount = pAnonymousType->GetGenericParamCount();
    ASSERT(paramCount > 0, "[AnonymousTypeEmitter::EmitType] AnonymousType must be generic.");
    array<PropertyAndField^>^ properties = gcnew array<PropertyAndField^>(paramCount);
    
    //Search for Properties
    GenericParameter* pParam = pAnonymousType->GetFirstGenericParam();
    for (int ind = 0 ; ind < paramCount; ind++)
    {
        ASSERT(pParam, "[AnonymousTypeEmitter::EmitType] the number of generic params was smaller than reported");
        PropertyAndField^ current = FindPropertyAndField(pAnonymousType, pParam, typeBuilder, hasKey);
        ASSERT(current, "[AnonymousTypeEmitter::EmitType] PropertyAndField was not found");
        properties[ind] = current;

        EmitProperty(current);

        pParam = pParam->GetNextParam();
    }
    ASSERT(pParam == NULL, "[AnonymousTypeEmitter::EmitType] the number of generic params was larger than reported");

    //Will cache methods to later define
    ConstructorBuilder^ ctorBuilder = nullptr;
    MethodBuilder^ toStringBuilder = nullptr;
    MethodBuilder^ equalsObjBuilder = nullptr;
    MethodBuilder^ equalsTypeBuilder = nullptr;
    MethodBuilder^ getHashBuilder = nullptr;

    FindMethods(pAnonymousType, typeBuilder, ctorBuilder, toStringBuilder, equalsObjBuilder, equalsTypeBuilder, getHashBuilder);

    ASSERT(ctorBuilder, "[AnonymousTypeEmitter::EmitType] Could not find the ctor");
    ASSERT(toStringBuilder, "[AnonymousTypeEmitter::EmitType] Could not find the ToString function");

    EmitCtor(properties, ctorBuilder);
    EmitToString(properties, toStringBuilder);
    
    if(hasKey)
    {
        //There is a key member so need to define additional methods
        ASSERT(equalsTypeBuilder, "[AnonymousTypeEmitter::EmitType] Could not find the Equals(Anonymous) function");
        ASSERT(equalsObjBuilder, "[AnonymousTypeEmitter::EmitType] Could not find the Equals(Object) function");
        ASSERT(getHashBuilder, "[AnonymousTypeEmitter::EmitType] Could not find the GetHashCode function");

        //When a key is used the Anonymous type implements the interface System::IEquatable<Anonymous Type>
        //Get the interface to implement
        System::Type^ genericIEquatable = System::IEquatable::typeid;
        System::Type^ concreteIEquatable = genericIEquatable->MakeGenericType(typeBuilder);
        //Implement it
        typeBuilder->AddInterfaceImplementation(concreteIEquatable);

        EmitEqualsTyped(properties, equalsTypeBuilder);
        EmitEqualsObj(typeBuilder, equalsObjBuilder, equalsTypeBuilder);
        EmitGetHashCode(properties, getHashBuilder);
    }else
    {
        ASSERT(equalsTypeBuilder == nullptr, "[AnonymousTypeEmitter::EmitType] Should not have found the Equals(Anonymous) function");
        ASSERT(equalsObjBuilder == nullptr, "[AnonymousTypeEmitter::EmitType] Should not have found the Equals(Object) function");
        ASSERT(getHashBuilder == nullptr, "[AnonymousTypeEmitter::EmitType] Should not have found the GetHashCode function");

    }

    type = typeBuilder->CreateType();
    m_types[typeBuilder->Name] = type;
    return type;
}

AnonymousTypeEmitter::PropertyAndField^ AnonymousTypeEmitter::FindPropertyAndField(BCSYM_NamedRoot* pAnonymousType, GenericParameter* pParam, TypeBuilder^ typeBuilder, bool &hasKey)
{
    ASSERT(pAnonymousType, "[AnonymousTypeEmitter::FindMethods] 'pAnonymousType' parameter is null");
    ASSERT(typeBuilder, "[AnonymousTypeEmitter::FindMethods] 'typeBuilder' parameter is null");
    ASSERT(pParam, "[AnonymousTypeEmitter::FindPropertyAndField] 'pParam' parameter is null");

    if(!pAnonymousType || !typeBuilder || !pParam)
    {
        return nullptr;
    }

    BCITER_CHILD members(pAnonymousType);
    PropertyAndField^ result = gcnew PropertyAndField();

    //Find property and field for this Generic Param
    while (BCSYM_NamedRoot* pMember = members.GetNext())
    {
        ASSERT(pMember, "[AnonymousTypeEmitter::FindPropertyAndField] the number of members was smaller than reported");
        if (pMember->IsVariable())
        {
            BCSYM_Variable *pField = pMember->PVariable();
            if (pField->GetType() == pParam)
            {
                //Found the field
                result->Field = DefineField(typeBuilder, pField);
                if(result->Property != nullptr)
                {
                    //Already found the property so break
                    break;
                }
            }
        }
        else if (pMember->IsProperty())
        {
            BCSYM_Property *pProperty = pMember->PProperty();
            if (pProperty->GetType() == pParam)
            {
                //Found the property
                result->Property = DefineProperty(typeBuilder, pProperty);
                if(pProperty->IsReadOnly())
                {
                    hasKey = true;
                }
                if(result->Field != nullptr)
                {
                    //Already found the field so break
                    break;
                }
            }
        }
    }

    ASSERT(result->Field, "[AnonymousTypeEmitter::FindPropertyAndField] Could not find a field");
    ASSERT(result->Property, "[AnonymousTypeEmitter::FindPropertyAndField] Could not find a property");

    return result;
}

void AnonymousTypeEmitter::FindMethods(BCSYM_NamedRoot* pAnonymousType, TypeBuilder^ typeBuilder, ConstructorBuilder^% ctorBuilder, MethodBuilder^% toStringBuilder, MethodBuilder^% equalsObjBuilder, MethodBuilder^% equalsTypeBuilder, MethodBuilder^% getHashBuilder)
{
    ASSERT(pAnonymousType, "[AnonymousTypeEmitter::FindMethods] 'pAnonymousType' parameter is null");
    ASSERT(typeBuilder, "[AnonymousTypeEmitter::FindMethods] 'typeBuilder' parameter is null");

    //Cache method names
    ::Compiler* compiler = pAnonymousType->GetCompiler();
    STRING* toStringName = compiler ->AddString(L"ToString");
    STRING* equalsName = compiler ->AddString(L"Equals");
    STRING* getHashName = compiler ->AddString(L"GetHashCode");
    STRING* ctorName = STRING_CONST(compiler, Constructor);
    BCSYM_NamedRoot* pMember = NULL;
    BCITER_CHILD members(pAnonymousType);

    //Find the methods
    while (pMember = members.GetNext())
    {
        ASSERT(pMember, "[AnonymousTypeEmitter::EmitType] the number of members was smaller than reported");
        if (pMember->IsProc())
        {
            BCSYM_Proc* pProc = pMember->PProc();
            if(StringPool::IsEqual(pProc->GetEmittedName(), toStringName))
            {
                toStringBuilder = DefineMethod(typeBuilder, pProc);
            }
            else if(StringPool::IsEqual(pProc->GetEmittedName(), ctorName))
            {
                ctorBuilder = this->DefineConstructor(typeBuilder, pProc);
            }
            else if(StringPool::IsEqual(pProc->GetEmittedName(), getHashName))
            {
                getHashBuilder = this->DefineMethod(typeBuilder, pProc);
            }
            else if(StringPool::IsEqual(pProc->GetEmittedName(), equalsName))
            {
                BCSYM_Param* param = pProc->GetFirstParam();
                if(StringPool::IsEqual(param->GetType()->PNamedRoot()->GetEmittedName(), pAnonymousType->GetEmittedName()))
                {
                    equalsTypeBuilder = this->DefineMethod(typeBuilder, pProc);
                }
                else
                {
                    equalsObjBuilder = this->DefineMethod(typeBuilder, pProc);
                }
            }
        }
    }
    ASSERT(pMember == NULL, "[AnonymousTypeEmitter::EmitType] the number of members was larger than reported");
}
void AnonymousTypeEmitter::EmitProperty(PropertyAndField^ prop)
{
    ASSERT(prop, "[AnonymousTypeEmitter::EmitProperty] 'prop' parameter is null");

    //Emit IL for getter
    MethodBuilder^ getter = (MethodBuilder^)prop->Property->GetGetMethod(false);
    ASSERT(getter, "[AnonymousTypeEmitter::EmitType] Property did not have a getter");
    ILGenerator^ mthdIL = getter->GetILGenerator();
    //Return Me.[field]
    mthdIL->Emit(OpCodes::Ldarg_0);
    mthdIL->Emit(OpCodes::Ldfld, prop->Field);
    mthdIL->Emit(OpCodes::Ret);

    //Emit IL for setter
    MethodBuilder^ setter = (MethodBuilder^)prop->Property->GetSetMethod(false);
    if(setter != nullptr)
    {
        mthdIL = setter->GetILGenerator();
        //Me.[field] = value
        mthdIL->Emit(OpCodes::Ldarg_0);
        mthdIL->Emit(OpCodes::Ldarg_1);
        mthdIL->Emit(OpCodes::Stfld, prop->Field);
        //Return
        mthdIL->Emit(OpCodes::Ret);
    }
}

void AnonymousTypeEmitter::EmitCtor(array<PropertyAndField^>^ properties, ConstructorBuilder^ method)
{
    ASSERT(properties, "[AnonymousTypeEmitter::EmitCtor] 'properties' parameter is null");
    ASSERT(method, "[AnonymousTypeEmitter::EmitCtor] 'method' parameter is null");

    ILGenerator^ mthdIL = method->GetILGenerator();
    //Public Sub New(...)
    
    //MyBase()
    mthdIL->Emit(OpCodes::Ldarg_0);
    mthdIL->Emit(OpCodes::Call, ObjectType->GetConstructor(System::Type::EmptyTypes));

    for(int ind = 0; ind < properties->Length; ind++)
    {
        //Me.[fieldX] = [argX]
        mthdIL->Emit(OpCodes::Ldarg_0);
        mthdIL->Emit(OpCodes::Ldarg, ind+1);
        mthdIL->Emit(OpCodes::Stfld, properties[ind]->Field);
    }
    //End Sub
    mthdIL->Emit(OpCodes::Ret);
}

void AnonymousTypeEmitter::EmitToString(array<PropertyAndField^>^ properties, MethodBuilder^ method)
{
    ASSERT(properties, "[AnonymousTypeEmitter::EmitToString] 'properties' parameter is null");
    ASSERT(method, "[AnonymousTypeEmitter::EmitToString] 'method' parameter is null");

    ILGenerator^ mthdIL = method->GetILGenerator();

    //cache append methods
    MethodInfo^ appendString = StringBuilderType->GetMethod("Append", gcnew array<System::Type^>(1) { StringType } );
    MethodInfo^ appendFormat = StringBuilderType->GetMethod("AppendFormat", gcnew array<System::Type^>(3) { StringType, ObjectType, ObjectType } );

    //Public Overrides Function ToString() As String
    //    Dim builder As New StringBuilder
    
    mthdIL->Emit(OpCodes::Newobj, StringBuilderType->GetConstructor(System::Type::EmptyTypes));
    
    //builder.Append("{ ")
    mthdIL->Emit(OpCodes::Ldstr, "{ ");
    mthdIL->EmitCall(OpCodes::Callvirt, appendString, nullptr);

    PropertyAndField^ prop = nullptr;
    FieldBuilder^ field = nullptr;
    
	int ind;
    for(ind = 0 ; ind < properties->Length - 1; ind++)
    {
        prop = properties[ind];
        field = properties[ind]->Field;
        
        //builder.AppendFormat("{0} = {1}, ", "[PropertyX's Name]", Me.[fieldX])
        mthdIL->Emit(OpCodes::Ldstr, "{0} = {1}, ");                  //"{0} = {1}, "
        mthdIL->Emit(OpCodes::Ldstr, prop->Property->Name);         //[PropertyX's Name]
        mthdIL->Emit(OpCodes::Ldarg_0);                             //Me.
        mthdIL->Emit(OpCodes::Ldfld, field);                        //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);               //Make that an object reference
        mthdIL->EmitCall(OpCodes::Callvirt, appendFormat, nullptr); //{0}.AppendFormat({1},{2},{3})
    }

    prop = properties[properties->Length - 1];
    field = properties[ind]->Field;
    
    //builder.AppendFormat("{0} = {1} ", "[PropertyX's Name]", Me.[fieldX])
    mthdIL->Emit(OpCodes::Ldstr, "{0} = {1} ");                  //"{0} = {1} "
    mthdIL->Emit(OpCodes::Ldstr, prop->Property->Name);         //[PropertyX's Name]
    mthdIL->Emit(OpCodes::Ldarg_0);                             //Me.
    mthdIL->Emit(OpCodes::Ldfld, field);                        //[fieldX]
    mthdIL->Emit(OpCodes::Box, field->FieldType);               //Make that an object reference
    mthdIL->EmitCall(OpCodes::Callvirt, appendFormat, nullptr); //{0}.AppendFormat({1},{2},{3})

    //builder.Append("}")
    mthdIL->Emit(OpCodes::Ldstr, "}");
    mthdIL->EmitCall(OpCodes::Callvirt, appendString, nullptr);

    //Return builder.ToString
    mthdIL->EmitCall(OpCodes::Callvirt, StringBuilderType->GetMethod("ToString", System::Type::EmptyTypes), nullptr);
    mthdIL->Emit(OpCodes::Ret);
    //End Function
}

void AnonymousTypeEmitter::EmitEqualsObj(TypeBuilder^ typeBuilder, MethodBuilder^ method, MethodBuilder^ typedEqualsMethoed)
{
    ASSERT(typeBuilder, "[AnonymousTypeEmitter::EmitEqualsObj] 'typeBuilder' parameter is null");
    ASSERT(method, "[AnonymousTypeEmitter::EmitEqualsObj] 'method' parameter is null");
    ASSERT(typedEqualsMethoed, "[AnonymousTypeEmitter::EmitEqualsObj] 'typedEqualsMethoed' parameter is null");

    ILGenerator^ mthdIL = method->GetILGenerator();

    //Return Me.Equals(TryCast(obj,[AnonymousType]))
    mthdIL->Emit(OpCodes::Ldarg_0);
    mthdIL->Emit(OpCodes::Ldarg_1);
    mthdIL->Emit(OpCodes::Isinst, typeBuilder);
    mthdIL->EmitCall(OpCodes::Call, typedEqualsMethoed, nullptr);
    mthdIL->Emit(OpCodes::Ret);
}

void AnonymousTypeEmitter::EmitEqualsTyped(array<PropertyAndField^>^ properties, MethodBuilder^ method)
{
    ASSERT(properties, "[AnonymousTypeEmitter::EmitEqualsTyped] 'properties' parameter is null");
    ASSERT(method, "[AnonymousTypeEmitter::EmitEqualsTyped] 'method' parameter is null");

    MethodInfo^ equalsMethod  = ObjectType->GetMethod("Equals", BindingFlags::Instance | BindingFlags::Public);
    ILGenerator^ mthdIL = method->GetILGenerator();

    //<DebuggerNonUserCode> _ 
    //Public Function Equals(ByVal Him As [AnonymousType]) As Boolean Implements IEquatable(Of VB$AnonymousType_1(Of T0, T1, T2)).Equals

    Label endIfNullHim = mthdIL->DefineLabel();
    
    //If (val Is Nothing) Then
    mthdIL->Emit(OpCodes::Ldarg_1);
    mthdIL->Emit(OpCodes::Brtrue_S, endIfNullHim);
    //Return False
    mthdIL->Emit(OpCodes::Ldc_I4_0);
    mthdIL->Emit(OpCodes::Ret);
    //End If
    mthdIL->MarkLabel(endIfNullHim);

    for each (PropertyAndField^ prop in properties)
    {
        FieldInfo^ field = prop->Field;
        //Skip read/write fields
        if(prop->Property->CanWrite)
        {
            continue;
        }
        
        //If (Me.[fieldX] Is Nothing) Then
        Label elseMyFieldNull = mthdIL->DefineLabel();
        mthdIL->Emit(OpCodes::Ldarg_0);                     //Me.
        mthdIL->Emit(OpCodes::Ldfld, field);                //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);       //Make that an object reference
        mthdIL->Emit(OpCodes::Brtrue_S, elseMyFieldNull);   //If the top of the stack (i.e. Me.[FieldX]) is not (null or false) goto the label elseMyFieldNull

        //If (Not Him.[fieldX] Is Nothing) Then
        Label endIfMyFieldNull = mthdIL->DefineLabel();
        mthdIL->Emit(OpCodes::Ldarg_1);                     //Him.
        mthdIL->Emit(OpCodes::Ldfld, field);                //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);       //Make that an object reference
        mthdIL->Emit(OpCodes::Brfalse_S, endIfMyFieldNull); //If the top of the stack (i.e. Him.[FieldX]) is (null or false) goto the label endIfMyFieldNull

        //Return False
        mthdIL->Emit(OpCodes::Ldc_I4_0);
        mthdIL->Emit(OpCodes::Ret);

        //End If
        //Else
        mthdIL->MarkLabel(elseMyFieldNull);
        
        //If (Him.[fieldX] Is Nothing) Then
        mthdIL->Emit(OpCodes::Ldarg_1);                     //Him.
        mthdIL->Emit(OpCodes::Ldfld, field);                //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);       //Make that an object reference
        mthdIL->Emit(OpCodes::Brtrue_S, endIfMyFieldNull);  //If the top of the stack (i.e. Him.[FieldX]) is not (null or false) goto the label elseMyFieldNull

        //Return False
        mthdIL->Emit(OpCodes::Ldc_I4_0);
        mthdIL->Emit(OpCodes::Ret);

        //End If
        //End If
        mthdIL->MarkLabel(endIfMyFieldNull);

        Label endIfEquals = mthdIL->DefineLabel();
        //If (((Not Me.[fieldX] Is Nothing) AndAlso (Not Him.[fieldX] Is Nothing)) AndAlso Not Me.[fieldX].Equals(Him.[fieldX])) Then
        mthdIL->Emit(OpCodes::Ldarg_0);                 //Me.
        mthdIL->Emit(OpCodes::Ldfld, field);            //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);   //Make that an object reference
        mthdIL->Emit(OpCodes::Brfalse_S, endIfEquals);  //If the top of the stack (i.e. Me.[FieldX]) is (null or false) goto the label endIfEquals

        mthdIL->Emit(OpCodes::Ldarg_1);                 //Him.
        mthdIL->Emit(OpCodes::Ldfld, field);            //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);   //Make that an object reference
        mthdIL->Emit(OpCodes::Brfalse_S, endIfEquals);  //If the top of the stack (i.e. Him.[FieldX]) is (null or false) goto the label endIfEquals

        mthdIL->Emit(OpCodes::Ldarg_0);                 //Me.
        mthdIL->Emit(OpCodes::Ldfld, field);            //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);   //Make that an object reference

        mthdIL->Emit(OpCodes::Ldarg_1);                 //Him.
        mthdIL->Emit(OpCodes::Ldfld, field);            //[fieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType);   //Make that an object reference

        mthdIL->EmitCall(OpCodes::Callvirt, equalsMethod, nullptr);  //{0}.Equals({1})
        mthdIL->Emit(OpCodes::Brtrue_S, endIfEquals);                //If the top of the stack (i.e. the call to Equals) is true goto the label endIfEquals

        //Return False
        mthdIL->Emit(OpCodes::Ldc_I4_0);
        mthdIL->Emit(OpCodes::Ret);

        //End If
        mthdIL->MarkLabel(endIfEquals);
    }

    //Return True
    mthdIL->Emit(OpCodes::Ldc_I4_1);
    mthdIL->Emit(OpCodes::Ret);
    //End Function
}

void AnonymousTypeEmitter::EmitGetHashCode(array<PropertyAndField^>^ properties, MethodBuilder^ method)
{
    ASSERT(properties, "[AnonymousTypeEmitter::EmitGetHashCode] 'properties' parameter is null");
    ASSERT(method, "[AnonymousTypeEmitter::EmitGetHashCode] 'method' parameter is null");

    //Cache GetHashCode method
    MethodInfo^ getHashCodeMethod = ObjectType->GetMethod("GetHashCode", System::Type::EmptyTypes);

    ILGenerator^ mthdIL = method->GetILGenerator();

    //Public Overrides Function GetHashCode() As Integer
    //Dim hash As Integer = [Seed]
    mthdIL->Emit(OpCodes::Ldc_I4, method->GetHashCode()); //Use method->GetHashCode() as seed so that each Ananymous type will most likley have a different seed.

    for(int ind = 0; ind < properties->Length; ind++)
    {
        FieldBuilder^ field = properties[ind]->Field;
        //Skip read/write fields
        if(properties[ind]->Property->CanWrite)
        {
            continue;
        }
        //hash = (hash * 31)
        mthdIL->Emit(OpCodes::Ldc_I4, 31);
        mthdIL->Emit(OpCodes::Mul);

        //If (Not Me.[FieldX] Is Nothing) Then
        Label endIf = mthdIL->DefineLabel();
        mthdIL->Emit(OpCodes::Ldarg_0);               //Me.
        mthdIL->Emit(OpCodes::Ldfld, field);          //[FieldX]
        mthdIL->Emit(OpCodes::Box, field->FieldType); //Make that an object reference
        mthdIL->Emit(OpCodes::Brfalse_S, endIf);      //If the top of the stack (i.e. Me.[FieldX]) is null or false goto the label endIf

        //hash = (hash + CType(Me.[FieldX], [Field's Type]).GetHashCode())
        mthdIL->Emit(OpCodes::Ldarg_0);                                     //Me.
        mthdIL->Emit(OpCodes::Ldflda, field);                               //[FieldX]
        mthdIL->Emit(OpCodes::Constrained, field->FieldType);               //([Field's Type])
        mthdIL->EmitCall(OpCodes::Callvirt, getHashCodeMethod, nullptr);    //.GetHashCode()
        mthdIL->Emit(OpCodes::Add);

        //End If
        mthdIL->MarkLabel(endIf);
    }

    //Return hash
    mthdIL->Emit(OpCodes::Ret);
    //End Function
}

