#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

TypeScope::TypeScope()
{
}

TypeScope ^TypeScope::Empty::get()
{
    if(m_empty == nullptr)
        m_empty = gcnew TypeScope();

    return m_empty;
}

bool TypeScope::NamespaceExists(System::String ^ns)
{
    return false;
}
                
array<System::Type^,1> ^TypeScope::FindTypes(System::String ^typeName, System::String ^nsPrefix)
{
    return nullptr;
}
