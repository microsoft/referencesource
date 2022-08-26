#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

ScriptScope::ScriptScope()
{
}

ScriptScope ^ScriptScope::Empty::get()
{
    if(m_empty == nullptr)
        m_empty = gcnew ScriptScope();

    return m_empty;
}

System::Type ^ScriptScope::FindVariable(System::String ^name)
{
    return nullptr;
}
