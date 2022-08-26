#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

ImportScope::ImportScope()
{
}

ImportScope ^ImportScope::Empty::get()
{
    if(m_empty == nullptr)
        m_empty = gcnew ImportScope();

    return m_empty;
}

System::Collections::Generic::IList<Import ^> ^ImportScope::GetImports()
{
    return nullptr;
}
