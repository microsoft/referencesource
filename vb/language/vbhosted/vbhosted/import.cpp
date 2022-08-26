#include "StdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

Import::Import(System::String ^importedEntity)
{
    if(System::String::IsNullOrEmpty(importedEntity))
        throw gcnew System::ArgumentNullException("importedEntity");

    m_alias = System::String::Empty;
    m_importedEntity = importedEntity;
}

Import::Import(System::String ^alias, 
               System::String ^importedEntity)
{
    if(System::String::IsNullOrEmpty(alias))
        throw gcnew System::ArgumentNullException("alias");
    if(System::String::IsNullOrEmpty(importedEntity))
        throw gcnew System::ArgumentNullException("importedEntity");

    m_alias = alias;
    m_importedEntity = importedEntity;
}

System::String ^Import::Alias::get()
{
    return m_alias;
}

System::String ^Import::ImportedEntity::get()
{
    return m_importedEntity;
}
