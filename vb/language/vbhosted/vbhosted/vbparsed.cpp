#include "stdafx.h"

VbParsed::VbParsed()
    :
    m_ErrorTable(),
    m_pSharedErrorTable(NULL)
{
    m_CodeBlock = new gcroot<LambdaExpression^>();
}

VbParsed::~VbParsed()
{
    // The Shared Error Table's memory is owned by the VBHostedCompiler. This is just a pointer to it.
    m_pSharedErrorTable = NULL;
    delete m_CodeBlock;
}

gcroot<LambdaExpression ^>* VbParsed::GetCodeBlock()
{
    return m_CodeBlock;
}

ErrorTable* VbParsed::GetErrorTable()
{
    return &m_ErrorTable;
}

void VbParsed::SetSharedErrorTable(ErrorTable* pSharedErrorTable)
{
    ASSERT(pSharedErrorTable, "[VbParsed::SetSharedErrorTable] 'pSharedErrorTable' parameter must not be null");
    ASSERT(!m_pSharedErrorTable, "[VbParsed::SetSharedErrorTable] 'm_pSharedErrorTable' member already initialized");

    if (pSharedErrorTable)
        m_pSharedErrorTable = pSharedErrorTable;
}

STDMETHODIMP VbParsed::CopyErrorsToResults(gcroot<Microsoft::Compiler::VisualBasic::CompilerResults^> Result)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyParamCond(Result, E_INVALIDARG, "[VbParsed::CopyErrorsToResults] 'Result' parameter is null");

    if (m_pSharedErrorTable && (m_pSharedErrorTable->HasErrors() || m_pSharedErrorTable->HasWarnings()))
    {
        ProcessErrors(m_pSharedErrorTable, Result);
    }

    if (m_ErrorTable.HasErrors() || m_ErrorTable.HasWarnings())
    {
        ProcessErrors(&m_ErrorTable, Result);
    }

    return S_OK;
}

void VbParsed::ProcessErrors(ErrorTable* pErrorTable, gcroot<Microsoft::Compiler::VisualBasic::CompilerResults^> Result)
{
    ASSERT(pErrorTable, "[VbParsed::ProcessErrors] 'pErrorTable' parameter must not be NULL");
    if (!pErrorTable)
        return;

    ErrorIterator errors(pErrorTable);
    CompileError* CurrentError = NULL;

    while (CurrentError = errors.Next())
    {
        //Warnings that are treated as errors are simply stored in the m_errors construct within the ErrorTable structure.
        //When extracted from our pErrorTable instance, we use the ErrorIterator.  Unfortunately, the recording of a warning 
        //as an error does not distinguish it as an error but rather as a warning stored in the error array.  So, when we 
        //transfer information from the ErrorTable to the CompilerResults instance that will be returned to the caller, we 
        //recategorize the warning as a warning based on the warning's self-identification that it is a warning.  The 
        //information that a warning was recorded as an error is lost.  However, the warning is recorded with a VS_TASKPRIORITY 
        //of TP_HIGH.  This is sufficient to distinguish a warning recorded as an error and so we now check whether an error 
        //is self-identified as an error or whether an error is recorded with a priority of TP_HIGH.  If neither is true, 
        //the error that self-identifies as a warning is indeed a warning.
        if (CurrentError->IsError() || CurrentError->m_Priority == TP_HIGH)
        {
            Result->AddError(CurrentError->m_errid, 
                gcnew System::String(CurrentError->m_wszMessage), 
                gcnew Microsoft::Compiler::VisualBasic::SourceLocation(CurrentError->m_loc.m_lBegColumn, 
                CurrentError->m_loc.m_lEndColumn, 
                CurrentError->m_loc.m_lBegLine, 
                CurrentError->m_loc.m_lEndLine));
        }
        else if (CurrentError->IsWarning())
        {
            Result->AddWarning(CurrentError->m_errid, 
                gcnew System::String(CurrentError->m_wszMessage), 
                gcnew Microsoft::Compiler::VisualBasic::SourceLocation(CurrentError->m_loc.m_lBegColumn, 
                CurrentError->m_loc.m_lEndColumn, 
                CurrentError->m_loc.m_lBegLine, 
                CurrentError->m_loc.m_lEndLine));
        } 
        else
        {
            ASSERT(false, "[VbParsed::CopyErrorsToResults] errortable has unexpected comment entry");
        }
    }


}
