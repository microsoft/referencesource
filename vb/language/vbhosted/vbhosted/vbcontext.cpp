#include "stdAfx.h"

// Compiler error numbers.
enum VbcErrorIds
{
    ERR_NoError,
#define ERRORDEF(num, level, name, strid) name,
#include "CommandLineErrors.inc"
#undef ERRORDEF
    ERR_COUNT
};

VbContext::VbContext(gcroot<Microsoft::Compiler::VisualBasic::CompilerContext^> compilerContext)
{
    m_compilerContext = compilerContext;
}

VbContext::~VbContext()
{
}

//VbContext
STDMETHODIMP VbContext::SetAssemblyRefs(/*[in]*/ gcroot<System::Collections::Generic::IList<System::Reflection::Assembly ^> ^> referenceAssemblies, /*[in]*/ Compiler *pCompiler, /*[in]*/ IVbCompilerProject *pVbCompilerProject, /*[in]*/ ErrorTable *pErrorTable)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pCompiler, "[VbContext::SetAssemblyRefs] 'pCompiler' parameter is null");
    VerifyInPtr(pVbCompilerProject, "[VbContext::SetAssemblyRefs] 'pVbCompilerProject' parameter is null");
    VerifyInPtr(pErrorTable, "[VbContext::SetAssemblyRefs] 'pErrorTable' parameter is null");
    
    HRESULT hr = S_OK;
    int count = 0;
    STRING *strLocation = NULL;
    STRING *strName = NULL;

    System::Collections::Generic::HashSet<System::String^>^ defaultAssemblies;

    AllProjectsIterator projIterator(pCompiler);

    if(referenceAssemblies)
    {
        count = referenceAssemblies->Count;
    }

    defaultAssemblies = gcnew System::Collections::Generic::HashSet<System::String^>();

    while(CompilerProject* project = projIterator.Next())
    {
        if(project->IsMetaData())
        {
            ASSERT(project->GetAssemblyIdentity(), "[VbContext::SetAssemblyRefs] the default assembly ref did not have an identity");
            ASSERT(project->GetAssemblyIdentity() && project->GetAssemblyIdentity()->GetAssemblyIdentityString(), "[VbContext::SetAssemblyRefs] the default assembly ref did not have an identity string");
            defaultAssemblies->Add(gcnew System::String(project->GetAssemblyIdentity()->GetAssemblyIdentityString()));
        }
    }

    for(long index = 0; index < count; ++index)
    {
        gcroot<System::Reflection::Assembly ^> assembly = referenceAssemblies->default[index];

        if(!assembly)
        {
            ASSERT(false, "[VbContext::SetAssemblyRefs] IList<Import ^>::default[index] returned unexpected null reference");
            continue;
        }

        if(!assembly->IsDynamic && !System::String::IsNullOrEmpty(assembly->Location))
        {
            if(!defaultAssemblies->Contains(assembly->FullName))
            {
                pin_ptr<const wchar_t> pLocation = PtrToStringChars(assembly->Location);
                // While the wchar_t type is equivalent to WCHAR, the cast to LPCOLESTR is necessary because the
                // pin_ptr<> class separates the pointer-ness of the reference from the type of the reference.
                // Consequently, we remind the compiler that "pLocation" is a pointer to WCHAR and not a scalar 
                // WCHAR reference.
                strLocation = pCompiler->AddString((LPCOLESTR) pLocation);

                hr = pVbCompilerProject->AddMetaDataReference(strLocation, TRUE);
                if (FAILED(hr) && ERRIDOFHR(hr) == ERRID_DuplicateReference2)
                {
                    ASSERT(assembly->GetName(), "[VbContext::SetAssemblyRefs] assembly did not have a name.");
                    System::String^ name = assembly->GetName()->Name;
                    pin_ptr<const wchar_t> pName = PtrToStringChars(name);
                    strName = pCompiler->AddString((LPCOLESTR) pName);
                    pErrorTable->CreateError(ERRID_DuplicateReference2, NULL, strName, strLocation);
                }
                else
                {
                    IfFailGo(hr);
                }
            }
        }
    }

Error:

    return hr;
}

STDMETHODIMP VbContext::SetImports(/*[in]*/ IVbCompilerProject *pVbCompilerProject)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbCompilerProject, "[VbContext::SetImports] 'pVbCompilerProject' parameter is null");

    HRESULT hr = S_OK;

    gcroot<System::Collections::Generic::IList<Microsoft::Compiler::VisualBasic::Import ^> ^> imports =
        m_compilerContext->ImportScope->GetImports();

    if(imports)
    {
        long count = imports->Count;

        for(long index = 0; index < count; ++index)
        {
            gcroot<Microsoft::Compiler::VisualBasic::Import ^> import = imports->default[index];

            if(!import)
            {
                ASSERT(false, "[VbContext::SetImports] IList<Import ^>::default[index] returned unexpected null reference");
                continue;
            }

            CComBSTR importExpression;

            if(!System::String::IsNullOrEmpty(import->Alias))
            {
                pin_ptr<const wchar_t> pAlias = PtrToStringChars(import->Alias);

                // While the wchar_t type is equivalent to WCHAR, the cast to LPCOLESTR is necessary because the
                // pin_ptr<> class separates the pointer-ness of the reference from the type of the reference.
                // Consequently, we remind the compiler that "pAlias" is a pointer to WCHAR and not a scalar 
                // WCHAR reference.
                IfFailGo(importExpression.Append((LPCOLESTR) pAlias, import->Alias->Length));
                IfFailGo(importExpression.Append(L'='));
            }

            pin_ptr<const wchar_t> pImport = PtrToStringChars(import->ImportedEntity);

            // While the wchar_t type is equivalent to WCHAR, the cast to LPCOLESTR is necessary because the
            // pin_ptr<> class separates the pointer-ness of the reference from the type of the reference.
            // Consequently, we remind the compiler that "pImport" is a pointer to WCHAR and not a scalar 
            // WCHAR reference.
            IfFailGo(importExpression.Append((LPCOLESTR) pImport, import->ImportedEntity->Length));

            IfFailGo(pVbCompilerProject->AddImport(importExpression));
        }
    }

Error:

    return hr;
}

STDMETHODIMP VbContext::NamespaceExists(/*[in]*/ BSTR name, /*[out,retval]*/ BOOL *pRetVal)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(name, "[VbContext::NamespaceExists] 'name' parameter is null");
    VerifyParamCond(SysStringLen(name) > 0, E_INVALIDARG, "[VbContext::NamespaceExists] 'name' parameter is zero length string");
    VerifyOutPtr(pRetVal, "[VbContext::NamespaceExists] 'pRetVal' parameter is null");

    HRESULT hr = S_OK;

    *pRetVal = FALSE;

    if(m_compilerContext->TypeScope->NamespaceExists(gcnew System::String(name)))
    {
        *pRetVal = TRUE;
    }

    return hr;
}

STDMETHODIMP VbContext::FindTypes(/*[in]*/ BSTR name, /*[in]*/ BSTR prefix, /*[out, retval]*/ gcroot<array<System::Type^,1>^>& pTypes)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(name, "[VbContext::GetTypeRef] 'name' parameter is null");
    VerifyParamCond(SysStringLen(name) > 0, E_INVALIDARG, "[VbContext::GetTypeRef] 'name' parameter is zero length string");
    VerifyInPtr(prefix, "[VbContext::GetTypeRef] 'prefix' parameter is null");
    // It should be legal to pass in prefix where prefix == ""

    HRESULT hr = S_OK;
    pTypes = m_compilerContext->TypeScope->FindTypes(gcnew System::String(name), gcnew System::String(prefix));

    return hr;
}

STDMETHODIMP VbContext::FindVariable(/*[in]*/ BSTR name, /*[out, retval]*/ gcroot<System::Type^>& pType)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(name, "[VbContext::GetVariable] 'name' parameter is null");
    VerifyParamCond(SysStringLen(name) > 0, E_INVALIDARG, "[VbContext::GetVariable] 'name' parameter is zero length string");
    
    HRESULT hr = S_OK;

    pType = m_compilerContext->ScriptScope->FindVariable(gcnew System::String(name));

    return hr;
}

STDMETHODIMP VbContext::SetOptions(/*[in]*/ IVbCompilerProject *pVbCompilerProject)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbCompilerProject, "[VbContext::SetOptions] 'pVbCompilerProject' parameter is null");

    gcroot<Microsoft::Compiler::VisualBasic::CompilerOptions ^> compilerOptions = m_compilerContext->Options;

    return SetOptions(pVbCompilerProject, compilerOptions);
}

STDMETHODIMP VbContext::SetDefaultOptions(/*[in]*/ IVbCompilerProject *pVbCompilerProject)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbCompilerProject, "[VbContext::SetOptions] 'pVbCompilerProject' parameter is null");

    gcroot<Microsoft::Compiler::VisualBasic::CompilerOptions ^> compilerOptions = gcnew Microsoft::Compiler::VisualBasic::CompilerOptions();

    return SetOptions(pVbCompilerProject, compilerOptions);
}

STDMETHODIMP VbContext::SetOptions(/*[in]*/ IVbCompilerProject *pVbCompilerProject, /*[in]*/ gcroot<Microsoft::Compiler::VisualBasic::CompilerOptions ^> compilerOptions)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbCompilerProject, "[VbContext::SetOptions] 'pVbCompilerProject' parameter is null");
    VerifyInCLRPtr(compilerOptions, "[VbContext::SetOptions] 'compilerOptions' parameter is null");

    HRESULT hr = S_OK;
    
    CComBSTR ExeName;
    VbCompilerOptions options;
    VBHeapPtr<VbCompilerWarningItemLevel> warningsLevelTable;
    WARNING_LEVEL warningLevel = WARN_None;

    gcroot<array<int> ^> ignoreWarnings = compilerOptions->IgnoreWarnings;
    int cIgnoreWarnings = ignoreWarnings->Length;
    gcroot<array<int> ^> treatWarningsAsErrors = compilerOptions->TreatWarningsAsErrors;
    int cTreatWarningsAsErrors = treatWarningsAsErrors->Length;
    int cWarnItems = cIgnoreWarnings + cTreatWarningsAsErrors;

    IfFailGo(ExeName.Append(L"vbhost.exe"));

    switch(compilerOptions->WarningLevel)
    {
        case Microsoft::Compiler::VisualBasic::OptionWarningLevelSetting::None:
        {
            warningLevel = WARN_None;
            break;
        }
        case Microsoft::Compiler::VisualBasic::OptionWarningLevelSetting::Regular:
        {
            warningLevel = WARN_Regular;
            break;
        }
        case Microsoft::Compiler::VisualBasic::OptionWarningLevelSetting::AsError:
        {
            warningLevel = WARN_AsError; 
            break;
        }
    }

    if(cWarnItems > 0)
    {
        int ii = cIgnoreWarnings;

        warningsLevelTable.Allocate(cWarnItems);

        for(int i = 0; i < cIgnoreWarnings; ++i)
        {
            warningsLevelTable[i].dwWrnId = ignoreWarnings[i];
            warningsLevelTable[i].WarningLevel = WARN_None;
        }

        for(int i = 0; i < cTreatWarningsAsErrors; ++i, ++ii)
        {
            warningsLevelTable[ii].dwWrnId = treatWarningsAsErrors[i];
            warningsLevelTable[ii].WarningLevel = WARN_AsError;
        }
    }

    ::ZeroMemory(&options, sizeof(options));

    options.OutputType = OUTPUT_Library;
    options.WarningLevel = warningLevel;
    options.bOptionStrictOff = (compilerOptions->OptionStrict == Microsoft::Compiler::VisualBasic::OptionStrictSetting::Off);
    options.bOptionInferOff = !compilerOptions->OptionInfer;
    options.bOptionCompareText = (compilerOptions->OptionCompare == Microsoft::Compiler::VisualBasic::OptionCompareSetting::Text);
    // We will always require that variables be declared by way of the ScriptScope.
    // We will not support inferred [variant] object types.
    // This means the explicit option is ON by default (and so we set OFF to false).
    options.bOptionExplicitOff = false;
    options.bRemoveIntChecks = compilerOptions->RemoveIntChecks;
    options.bNoStandardLibs = true;
    options.wszExeName = ExeName;
    options.cWarnItems = cWarnItems;
    options.warningsLevelTable = warningsLevelTable;
    options.bHighEntropyVA = false;

    //Disable My
    options.wszCondComp = L"_MYTYPE=\"Empty\"";
    IfFailGo(pVbCompilerProject->SetCompilerOptions(&options));

Error:

    return hr;
}

STDMETHODIMP VbContext::SetOptions(/*[in]*/ SourceFile *pSourceFile)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pSourceFile, "[VbContext::SetOptions] 'pSourceFile' parameter is null");

    gcroot<Microsoft::Compiler::VisualBasic::CompilerOptions ^> compilerOptions = m_compilerContext->Options;

    if( /*!options.bOptionStrictOff*/ compilerOptions->OptionStrict == Microsoft::Compiler::VisualBasic::OptionStrictSetting::On)
    {
        pSourceFile->CombineOptionFlags(OPTION_OptionStrict);
    }
    else
    {
        pSourceFile->StripOptionFlags(OPTION_OptionStrict);
    }
    
    if( /*!options.bOptionInferOff*/ compilerOptions->OptionInfer)
    {
        pSourceFile->CombineOptionFlags(OPTION_OptionInfer);
    }
    else
    {
        pSourceFile->StripOptionFlags(OPTION_OptionInfer);
    }

    if( /*options.bOptionCompareText*/ compilerOptions->OptionCompare == Microsoft::Compiler::VisualBasic::OptionCompareSetting::Text)
    {
        pSourceFile->CombineOptionFlags(OPTION_OptionText);
    }
    else
    {
        pSourceFile->StripOptionFlags(OPTION_OptionText);
    }

    // We will always require that variables be declared by way of the ScriptScope.
    // We will not support inferred [variant] object types.
    pSourceFile->CombineOptionFlags(OPTION_OptionExplicit);

    return S_OK;
}
