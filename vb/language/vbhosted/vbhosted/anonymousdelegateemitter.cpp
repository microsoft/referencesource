#include "stdAfx.h"

using namespace System::Reflection;
using namespace System::Reflection::Emit;
using namespace Microsoft::Compiler::VisualBasic;

AnonymousDelegateEmitter::AnonymousDelegateEmitter(SymbolMap^ symbolMap, ModuleBuilder^ moduleBuilder) : 
    TypeEmitter(symbolMap, moduleBuilder)
{
    m_types = gcnew System::Collections::Generic::Dictionary<System::String^,System::Type^>();
}

bool AnonymousDelegateEmitter::TryGetType(BCSYM_NamedRoot* pSymbol, System::Type^% type)
{
    ASSERT(pSymbol, "[AnonymousTypeEmitter::TryGetType] 'pSymbol' parameter is null");

    System::String^ name = gcnew System::String(pSymbol->GetEmittedName());
    return m_types->TryGetValue(name, type);
}

System::Type^ AnonymousDelegateEmitter::EmitType(BCSYM_NamedRoot* pAnonymousDelegate)
{
    ASSERT(pAnonymousDelegate, "[AnonymousDelegateEmitter::EmitType] 'pAnonymousDelegate' parameter is null");
    ASSERT(pAnonymousDelegate->IsAnonymousDelegate(), "[AnonymousDelegateEmitter::EmitType] 'pAnonymousDelegate' parameter was not an Anonymous Delegate");

    TypeBuilder^ typeBuilder = DefineType(pAnonymousDelegate->PNamedRoot());
    m_types->Add(typeBuilder->Name, typeBuilder);

    System::Type^ type = nullptr;
    ::Compiler* compiler = pAnonymousDelegate->GetCompiler();
    STRING* ctorName = STRING_CONST(compiler, Constructor);
    BCSYM_NamedRoot* pMember = NULL;
    BCITER_CHILD members(pAnonymousDelegate);

    //Emit the methods
    while (pMember = members.GetNext())
    {
        if (pMember->IsProc())
        {
            BCSYM_Proc* pProc = pMember->PProc();
            if(StringPool::IsEqual(pProc->GetEmittedName(), ctorName))
            {
                ConstructorBuilder^ ctorBuilder = DefineConstructor(typeBuilder, pProc);
                ctorBuilder->SetImplementationFlags(MethodImplAttributes::Runtime);
            }
            else
            {
                MethodBuilder^ methodBuilder = DefineMethod(typeBuilder, pProc);
                methodBuilder->SetImplementationFlags(MethodImplAttributes::Runtime);
            }
        }
        else
        {
            ASSERT(pAnonymousDelegate, "[AnonymousDelegateEmitter::EmitType] unexpected member");
        }
    }

    type = typeBuilder->CreateType();
    m_types[typeBuilder->Name] = type;
    return type;
}

