#include "stdafx.h"

VbHostedCompiler::VbHostedCompiler(gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^>^> referenceAssemblies)
    : 
    m_fInit(false),
    m_referenceAssemblies(gcnew System::Collections::Generic::List<System::Reflection::Assembly ^>(referenceAssemblies)),
    m_ErrorTable()
{
}

VbHostedCompiler::~VbHostedCompiler()
{
    if(m_spVbCompilerProject)
    {
        m_spVbCompilerProject->Disconnect();
    }

    if (m_spVbExternalCompilerProject)
    {
        m_spVbExternalCompilerProject->Disconnect();
    }
}

STDMETHODIMP VbHostedCompiler::InitCompiler()
{
    HRESULT hr = S_OK;
    if (!m_fInit)
    {
        ASSERT(!m_spVbCompiler, "[VbHostedCompiler::InitCompiler] 'm_spVbCompiler' member is not NULL");
        ASSERT(!m_spVbCompilerHost, "[VbHostedCompiler::InitCompiler] 'm_spVbCompilerHost' member is not NULL");
        ASSERT(!m_spVbCompilerProject, "[VbHostedCompiler::InitCompiler] 'm_spVbCompilerProject' member is not NULL");
        ASSERT(!m_pCompilerProject, "[VbHostedCompiler::InitCompiler] 'm_pCompilerProject' member is not NULL");

        IfFailGo(VbCompilerHost::Create(&m_spVbCompilerHost));
        IfFailGo(GetCompiler());
        IfFailGo(SetupCompilerProject(&m_ErrorTable));
        IfFailGo(CreateExternalCompilerProject());
        m_fInit = true;
    }

Error:
    if (FAILED(hr))
    {
        m_spVbCompilerHost = NULL;
        m_spVbCompiler = NULL;
    }
    return hr;
}

STDMETHODIMP VbHostedCompiler::CompileExpression
    (
        BSTR Expression, 
        VbContext* pContext, 
        gcroot<System::Type ^> TargetType,
        VbParsed *pParsed
    )
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(Expression, "[VbHostedCompiler::CompileExpression] 'Expression' parameter is null");
    VerifyParamCond(SysStringLen(Expression) > 0, E_INVALIDARG, "[VbHostedCompiler::CompileExpression] 'Expression' parameter is zero length string");
    VerifyInPtr(pContext, "[VbHostedCompiler::CompileExpression] 'pContext' parameter is null");
    VerifyInPtr(pParsed, "[VbHostedCompiler::CompileExpression] 'pParsed' parameter is null");

    VB_ENTRY();

    IfFailGo(InitCompiler());

    pParsed->SetSharedErrorTable(&m_ErrorTable);

    if (!m_ErrorTable.HasErrors())
    {
        VBHostedSession session(Expression, m_spVbCompiler, m_spVbCompilerHost, m_spVbCompilerProject, m_pExternalCompilerProject, pContext, TargetType);
        pParsed->GetErrorTable()->Init(m_pCompiler, m_pCompilerProject, NULL);
        IfFailGo(session.CompileExpression(pParsed));
    }

    g_pvbNorlsManager->GetPageHeap().ShrinkUnusedResources();

    VB_EXIT_LABEL();
}

STDMETHODIMP VbHostedCompiler::CompileStatements
    (
        BSTR Statements, 
        VbContext* pContext, 
        VbParsed *pParsed
    )
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(Statements, "[VbHostedCompiler::CompileStatements] 'Statements' parameter is null");
    VerifyParamCond(SysStringLen(Statements) > 0, E_INVALIDARG, "[VbHostedCompiler::CompileStatements] 'Statements' parameter is zero length string");
    VerifyInPtr(pContext, "[VbHostedCompiler::CompileStatements] 'pContext' parameter is null");
    VerifyInPtr(pParsed, "[VbHostedCompiler::CompileStatements] 'pParsed' parameter is null");

    VB_ENTRY();

    IfFailGo(InitCompiler());

    pParsed->SetSharedErrorTable(&m_ErrorTable);

    if (!m_ErrorTable.HasErrors())
    {
        VBHostedSession session(Statements, m_spVbCompiler, m_spVbCompilerHost, m_spVbCompilerProject, m_pExternalCompilerProject, pContext, nullptr);
        pParsed->GetErrorTable()->Init(m_pCompiler, m_pCompilerProject, NULL);
        IfFailGo(session.CompileStatements(pParsed));
    }

    VB_EXIT_LABEL();
}

STDMETHODIMP VbHostedCompiler::GetCompiler()
{

    HRESULT hr = S_OK;
    
    CompilerHost *pCompilerHost = NULL; // Don't need to manage lifetime of pCompilerHost as it is controlled by pCompiler.

    // Create the compiler, this will have references to the default libraries 
    IfFailGo(VBCreateBasicCompiler(false, DelayLoadUICallback, m_spVbCompilerHost, &m_spVbCompiler));

    m_pCompiler = (Compiler *)((IVbCompiler*)m_spVbCompiler);

    //"Compile" the mscorlib project
    if(m_pCompiler->FindCompilerHost(m_spVbCompilerHost, &pCompilerHost) && pCompilerHost && pCompilerHost->GetComPlusProject())
    {
        IfTrueGo(pCompilerHost->GetComPlusProject()->Compile(), E_FAIL);
    }
    else
    {
        ASSERT(pCompilerHost, "Could not get Compiler Host");
        ASSERT(!pCompilerHost, "Could not get mscorlib's project");
        IfFailGo(E_FAIL);
    }
    
Error:

    return hr;
}

STDMETHODIMP VbHostedCompiler::SetupCompilerProject(ErrorTable *pErrorTable)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pErrorTable, "[VBHostedSession::SetupCompilerProject] 'pErrorTable' parameter is null");

    ASSERT(m_spVbCompilerHost, "[VBHostedSession::SetupCompilerProject] 'm_spVbCompilerHost' is null");
    ASSERT(m_spVbCompiler, "[VBHostedSession::SetupCompilerProject] 'm_spVbCompiler' is null");

    HRESULT hr = S_OK;
 
    IfFailGo(CreateCompilerProject());
    IfFailGo(InitializeCompilerProject(pErrorTable));

    // Ensure that all the projects (in particular mscorlib and the VB runtime
    // have been compiled at least to CS_Bound).
    //
    IfFailGo(m_pCompiler->CompileToBound(
        m_pCompilerProject->GetCompilerHost(),
        NULL, // ULONG *pcWarnings
        NULL, // ULONG *pcErrors,
        NULL // VbError **ppErrors
        ));

Error:

    return hr;
}

STDMETHODIMP VbHostedCompiler::CreateCompilerProject()
{
    ASSERT(m_spVbCompilerHost, "[VBHostedSession::CreateCompilerProject] 'm_spVbCompilerHost' is null");
    ASSERT(m_spVbCompiler, "[VBHostedSession::CreateCompilerProject] 'm_spVbCompiler' is null");
    ASSERT(!m_spVbCompilerProject, "[VBHostedSession::CreateCompilerProject] 'm_spVbCompilerProject' is not null");
    ASSERT(!m_pCompilerProject, "[VBHostedSession::CreateCompilerProject] 'm_pCompilerProject' is not null");

    HRESULT hr = S_OK;

    // Add System.Core.dll as it is required for DLR tree conversion
    //
    {
        // Register the compiler host to get access to the underlying "CompierHost"
        // object before the project is created so that System.Core can also be
        // added to the list of standard libraries and thus found using the
        // FX search paths and added as a reference to the project when it
        // is created.
        //
        IfFailGo(m_pCompiler->RegisterVbCompilerHost(m_spVbCompilerHost));

        CompilerHost *CompilerHost = NULL;
        IfFalseGo(m_pCompiler->FindCompilerHost(m_spVbCompilerHost, &CompilerHost), E_UNEXPECTED);
        ThrowIfNull(CompilerHost);

        DynamicArray<STRING *> *StdLibs = CompilerHost->GetStandardLibraries();
        ThrowIfNull(StdLibs);
        STRING **StdLibsArray = StdLibs->Array();

        // Add to the list only if not already present.

        STRING *SystemCoreDLL = m_pCompiler->AddString(L"System.Core.dll");

		unsigned i;
        for(i = 0; i < StdLibs->Count(); i++)
        {
            if (StringPool::IsEqual(StdLibsArray[i] , SystemCoreDLL)) break;
        }

        if (i == StdLibs->Count())
        {
            StdLibs->AddElement(SystemCoreDLL);
        }
    }


    // Create our default project.

    IfFailGo(m_spVbCompiler->CreateProject
        (
            L"vbhost",
            m_spVbCompiler,
            NULL,
            m_spVbCompilerHost,
            &m_spVbCompilerProject
        ));
    
    m_pCompilerProject = (CompilerProject*)m_spVbCompilerProject.p;

Error:

   return hr;
}

STDMETHODIMP VbHostedCompiler::InitializeCompilerProject(ErrorTable *pErrorTable)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pErrorTable, "[VBHostedSession::InitializeCompilerProject] 'pErrorTable' parameter is null");

    ASSERT(m_pCompiler, "[VBHostedSession::InitializeCompilerProject] 'm_pCompiler' is null");
    ASSERT(m_spVbCompilerProject, "[VBHostedSession::InitializeCompilerProject] 'm_spVbCompilerProject' is null");

    HRESULT hr = S_OK;
   
    // Must initialize errortable before calling SetAssemblyRefs so that failures encountered when processing
    // the assembly references are captured correctly.
    pErrorTable->Init(m_pCompiler, m_pCompilerProject, NULL);

    m_pCompilerProject->AddStandardLibraries();

    IfFailGo(VbContext::SetAssemblyRefs(m_referenceAssemblies, m_pCompiler, m_spVbCompilerProject, pErrorTable));

    //Need to set the options to something so that the project will be compiled to bound.
    IfFailGo(VbContext::SetDefaultOptions(m_spVbCompilerProject));

Error:

    return hr;
}

STDMETHODIMP VbHostedCompiler::CreateExternalCompilerProject()
{
    ASSERT(m_spVbCompilerHost, "[VBHostedSession::CreateExternalCompilerProject] 'm_spVbCompilerHost' is null");
    ASSERT(m_spVbCompiler, "[VBHostedSession::CreateExternalCompilerProject] 'm_spVbCompiler' is null");
    ASSERT(!m_spVbExternalCompilerProject, "[VBHostedSession::CreateExternalCompilerProject] 'm_spVbExternalCompilerProject' is not null");
    ASSERT(!m_pExternalCompilerProject, "[VBHostedSession::CreateExternalCompilerProject] 'm_pExternalCompilerProject' is not null");

    HRESULT hr = S_OK;

    //Create the Type Scope Project

    IfFailGo(m_pCompilerProject->AddTypeScopeReference(&m_pExternalCompilerProject));
    m_spVbExternalCompilerProject = m_pExternalCompilerProject;

Error:

   return hr;
}
