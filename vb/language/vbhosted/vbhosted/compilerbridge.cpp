#include "stdAfx.h"

using namespace Microsoft::Compiler::VisualBasic;

#define HOSTED_COMPILER_EXCEPTION L"Hosted Compiler experienced an internal error."

CompilerBridge::CompilerBridge(gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^> ^> referenceAssemblies) :
    m_VbHostedCompiler(referenceAssemblies),
    m_fInvalid(false)
{
}

CompilerBridge::~CompilerBridge()
{
}

void CompilerBridge::CompileExpression(gcroot<System::String ^> expression, 
                                       gcroot<CompilerContext ^> context,
                                       gcroot<System::Type ^> targetType,
                                       gcroot<CompilerResults ^> results)
{
    HRESULT hr = S_OK;
    CComBSTR bstrExpression;
    VbContext Context(context);
    VbParsed Parsed;
    pin_ptr<const wchar_t> pExpression = nullptr;

    if (m_fInvalid)
        throw gcnew System::InvalidOperationException();

    pExpression = PtrToStringChars(expression);

    try
    {
        // While the wchar_t type is equivalent to WCHAR, the cast to LPCOLESTR is necessary because the
        // pin_ptr<> class separates the pointer-ness of the reference from the type of the reference.
        // Consequently, we remind the compiler that "pExpression" is a pointer to WCHAR and not a scalar 
        // WCHAR reference.
        IfFailGoto(bstrExpression.Append((LPCOLESTR) pExpression, expression->Length), Exit);
        ASSERT(SUCCEEDED(hr), "memory allocation error copying expression to BSTR");

        IfFailGoto(m_VbHostedCompiler.CompileExpression(bstrExpression, &Context, targetType, &Parsed), Exit);

        // translate pResults to results
        IfFailGoto(Parsed.CopyErrorsToResults(results), Exit);
        results->SetCodeBlock(*Parsed.GetCodeBlock());
    }
    catch (System::Exception^)
    {
        m_fInvalid = true;
        throw;
    }

Exit:
    if (FAILED(hr))
    {
        ASSERT(SUCCEEDED(hr), "internal error attempting to compile expression");

        //
        gcroot<System::Exception^> comException = System::Runtime::InteropServices::Marshal::GetExceptionForHR(hr);
        m_fInvalid = true;
        throw gcnew System::ApplicationException(gcnew System::String(HOSTED_COMPILER_EXCEPTION), comException);
    }
}

void CompilerBridge::CompileStatements(gcroot<System::String ^> statements, 
                                       gcroot<CompilerContext ^> context,
                                       gcroot<CompilerResults ^> results)
{
    HRESULT hr = S_OK;
    CComBSTR bstrStatements;
    VbContext Context(context);
    VbParsed Parsed;
    pin_ptr<const wchar_t> pStatements = nullptr;

    if (m_fInvalid)
        throw gcnew System::InvalidOperationException();

    pStatements = PtrToStringChars(statements);

    try
    {
        // While the wchar_t type is equivalent to WCHAR, the cast to LPCOLESTR is necessary because the
        // pin_ptr<> class separates the pointer-ness of the reference from the type of the reference.
        // Consequently, we remind the compiler that "pStatements" is a pointer to WCHAR and not a scalar 
        // WCHAR reference.
        IfFailGoto(bstrStatements.Append((LPCOLESTR) pStatements, statements->Length), Exit);
        ASSERT(SUCCEEDED(hr), "memory allocation error copying statements to BSTR");

        IfFailGoto(m_VbHostedCompiler.CompileStatements(bstrStatements, &Context, &Parsed), Exit);

        // translate pResults to results
        IfFailGoto(Parsed.CopyErrorsToResults(results), Exit);
        results->SetCodeBlock(*Parsed.GetCodeBlock());
    }
    catch (System::Exception^)
    {
        m_fInvalid = true;
        throw;
    }

Exit:
    if (FAILED(hr))
    {
        ASSERT(SUCCEEDED(hr), "internal error attempting to compile expression");

        //
        gcroot<System::Exception^> comException = System::Runtime::InteropServices::Marshal::GetExceptionForHR(hr);
        m_fInvalid = true;
        throw gcnew System::ApplicationException(gcnew System::String(HOSTED_COMPILER_EXCEPTION), comException);
    }
}
