#include "StdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

Error::Error(int errorCode,
             System::String ^description,
             Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation)
{
    ASSERT(!System::String::IsNullOrEmpty(description), "description is null or empty");

    if(System::String::IsNullOrEmpty(description))
        throw gcnew System::ArgumentNullException("description");

    if(sourceLocation == nullptr)
        sourceLocation = gcnew Microsoft::Compiler::VisualBasic::SourceLocation(0, 0, 0, 0);

    m_errorCode = errorCode;
    m_description = description;
    m_sourceLocation = sourceLocation;
}

int Error::ErrorCode::get()
{
    return m_errorCode;
}

System::String ^Error::Description::get()
{
    return m_description;
}

Microsoft::Compiler::VisualBasic::SourceLocation ^Error::SourceLocation::get()
{
    return m_sourceLocation;
}
