#include "StdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

Warning::Warning(int warningCode,
                 System::String ^description,
                 Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation)
{
    ASSERT(!System::String::IsNullOrEmpty(description), "description is null or empty");

    if(System::String::IsNullOrEmpty(description))
        throw gcnew System::ArgumentNullException("description");

    if(sourceLocation == nullptr)
        sourceLocation = gcnew Microsoft::Compiler::VisualBasic::SourceLocation(0, 0, 0, 0);

    m_warningCode = warningCode;
    m_description = description;
    m_sourceLocation = sourceLocation;
}

int Warning::WarningCode::get()
{
    return m_warningCode;
}

System::String ^Warning::Description::get()
{
    return m_description;
}

Microsoft::Compiler::VisualBasic::SourceLocation ^Warning::SourceLocation::get()
{
    return m_sourceLocation;
}
