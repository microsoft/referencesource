#pragma once

#include "VBContext.h"
#include "VBParsed.h"

class VbHostedCompiler
#if FV_DEADBEEF
    : public Deadbeef<VbHostedCompiler> // Must be last base class!
#endif
{
public:
    VbHostedCompiler(gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^>^> referenceAssemblies);
    virtual ~VbHostedCompiler();

    STDMETHODIMP CompileExpression(/*[in]*/ BSTR Expression, /*[in]*/ VbContext *pContext, /*[in]*/ gcroot<System::Type ^> TargetType, /*[in,out]*/ VbParsed *pParsed);
    STDMETHODIMP CompileStatements(/*[in]*/ BSTR Statements, /*[in]*/ VbContext *pContext, /*[in,out]*/ VbParsed *pParsed);
    
private:
    STDMETHODIMP GetCompiler();
    STDMETHODIMP InitCompiler();

    // CreateCompilerProject creates a new compiler Project which acts as the
    // context for the expression or statements being compiled.
    STDMETHODIMP CreateCompilerProject();
    STDMETHODIMP InitializeCompilerProject(ErrorTable *pErrorTable);
    STDMETHODIMP SetupCompilerProject(ErrorTable *pErrorTable);

    bool m_fInit;
    CComPtr<IVbCompiler> m_spVbCompiler;
    CComPtr<IVbCompilerHost> m_spVbCompilerHost;
    CComPtr<IVbCompilerProject> m_spVbCompilerProject;
    CComPtr<IVbCompilerProject> m_spVbExternalCompilerProject;

    // CreateExternalCompilerProject creates a new compiler Project which acts
    // as the context for Types from Type Scope.
    STDMETHODIMP CreateExternalCompilerProject();

    Compiler* m_pCompiler;
    CompilerProject* m_pCompilerProject;
    CompilerProject *m_pExternalCompilerProject;
    ErrorTable m_ErrorTable;

    gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^>^> m_referenceAssemblies;
};

