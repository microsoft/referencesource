#include "stdafx.h"

using namespace Microsoft::Compiler::VisualBasic;

CompilerContext ^CompilerContext::Empty::get()
{
    if(m_empty == nullptr)
    {
        m_empty = gcnew CompilerContext(nullptr, nullptr, nullptr, nullptr);
    }

    return m_empty;
}

CompilerContext::CompilerContext(IScriptScope ^scriptScope, 
                                 ITypeScope ^typeScope, 
                                 IImportScope ^importScope, 
                                 CompilerOptions ^options)
{
    m_scriptScope = scriptScope;
    m_typeScope = typeScope;
    m_importScope = importScope;
    m_options = options;
}

IImportScope ^CompilerContext::ImportScope::get()
{
    if(m_importScope == nullptr)
    {
        m_importScope = Microsoft::Compiler::VisualBasic::ImportScope::Empty;
    }

    return m_importScope;
}

IScriptScope ^CompilerContext::ScriptScope::get()
{
    if(m_scriptScope == nullptr)
    {
        m_scriptScope = Microsoft::Compiler::VisualBasic::ScriptScope::Empty;
    }

    return m_scriptScope;
}

ITypeScope ^CompilerContext::TypeScope::get()
{
    if(m_typeScope == nullptr)
    {
        m_typeScope = Microsoft::Compiler::VisualBasic::TypeScope::Empty;
    }

    return m_typeScope;
}

CompilerOptions ^CompilerContext::Options::get()
{
    if(m_options == nullptr)
    {
        m_options = gcnew CompilerOptions();
    }

    return m_options;
}
