#include "stdafx.h"

using namespace Microsoft::Compiler::VisualBasic;

CompilerResults::CompilerResults() :
    m_codeBlock(nullptr)
{
    m_errors = gcnew System::Collections::Generic::List<Error^>();
    m_warnings = gcnew System::Collections::Generic::List<Warning^>();
}

void CompilerResults::AddError(int errorCode, System::String ^description, Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation)
{
    Error ^error = gcnew Error(errorCode, description, sourceLocation);
    m_errors->Add(error);
}

void CompilerResults::AddWarning(int warningCode, System::String ^description, Microsoft::Compiler::VisualBasic::SourceLocation ^sourceLocation)
{
    Warning ^warning = gcnew Warning(warningCode, description, sourceLocation);
    m_warnings->Add(warning);
}

void CompilerResults::SetCodeBlock(System::Linq::Expressions::LambdaExpression ^codeBlock)
{
    m_codeBlock = codeBlock;
}

System::Linq::Expressions::LambdaExpression ^CompilerResults::CodeBlock::get()
{
    return m_codeBlock;
}

System::Collections::Generic::IList<Microsoft::Compiler::VisualBasic::Error ^> ^CompilerResults::Errors::get()
{
    return m_errors;
}

System::Collections::Generic::IList<Microsoft::Compiler::VisualBasic::Warning ^> ^CompilerResults::Warnings::get()
{
    return m_warnings;
}
