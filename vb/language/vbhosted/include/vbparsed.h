#pragma once

#import "mscorlib.tlb" raw_interfaces_only raw_native_types named_guids rename("_Module", "_ReflectionModule") rename("ReportEvent", "_ReflectionReportEvent") rename("value", "_value")
#include "vcclr.h"
#include "Errors.h"
#include "CompilerResults.h"

using System::Linq::Expressions::LambdaExpression;

class VbParsed
#if FV_DEADBEEF
    : public Deadbeef<VbParsed> // Must be last base class!
#endif
{
public:
    VbParsed();
    virtual ~VbParsed();

    //IVbParsed implementation
    gcroot<LambdaExpression^>* GetCodeBlock();
    ErrorTable* GetErrorTable();
    void SetSharedErrorTable(ErrorTable* pSharedErrorTable);

    STDMETHODIMP CopyErrorsToResults(gcroot<Microsoft::Compiler::VisualBasic::CompilerResults^> Result);

private:
    ErrorTable m_ErrorTable;
    gcroot<LambdaExpression^> *m_CodeBlock;
    ErrorTable* m_pSharedErrorTable;

    void ProcessErrors(ErrorTable* pErrorTable, gcroot<Microsoft::Compiler::VisualBasic::CompilerResults^> Result);

    // Do not want to support copy semantics for this class, so 
    // declare as private and don't implement...
    VbParsed(const VbParsed &source);
    VbParsed &operator=(const VbParsed &source);
};

