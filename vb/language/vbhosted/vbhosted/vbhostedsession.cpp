#include "stdafx.h"

VBHostedSession::VBHostedSession(BSTR sourceCode, IVbCompiler* pCompiler, IVbCompilerHost* pCompilerHost, IVbCompilerProject* pVbCompilerProject, 
        CompilerProject *pExternalCompilerProject, VbContext* pContext, gcroot<System::Type ^> TargetType)
    :
    m_spVbCompiler(pCompiler),
    m_spVbCompilerHost(pCompilerHost),
    m_spVbCompilerProject(pVbCompilerProject),
    m_pContext(pContext),
    m_SourceCode(sourceCode),
    m_ExprTargetType(TargetType),
    m_pSourceFile(NULL),
    m_pExternalCompilerFile(NULL),
    m_pUnnamedNamespace(NULL),
    m_pCompilerProject(NULL),
    m_pExternalCompilerProject(pExternalCompilerProject),
    m_Allocator()
{
    ASSERT(m_spVbCompiler, "[VBHostedSession::VBHostedSession] error allocating smart pointer to VbCompiler");
    ASSERT(m_spVbCompilerHost, "[VBHostedSession::VBHostedSession] error allocating smart pointer to VbCompilerHost");

    if (m_spVbCompilerProject)
    {
        m_pCompilerProject = (CompilerProject*)m_spVbCompilerProject.p;
    }

    if (m_spVbCompiler)
    {
        m_pCompiler = (Compiler*)m_spVbCompiler.p;
    }

    SetupExternalCompilerProject();
}

VBHostedSession::~VBHostedSession()
{
    if (m_pSourceFile)
    {
        //The dtor for SourceFile removes it from the corresponding project
        delete m_pSourceFile;
    }

    m_pCompilerProject->DeleteAllImports();
    m_pCompilerProject->GetImportsCache()->Clear();

    m_pExternalCompilerProject->ClearLookupCaches();
    m_pExternalCompilerProject->RemoveNamespaces();

    if (m_pExternalCompilerFile)
    {
        //The dtor for CompilerFile removes it from the corresponding project
        delete m_pExternalCompilerFile;
    }

    m_pCompiler = NULL;
    m_pCompilerProject = NULL;
    m_pExternalCompilerProject = NULL;
    m_pSourceFile = NULL;
    m_pExternalCompilerFile = NULL;
}

STDMETHODIMP VBHostedSession::Parse(ErrorTable* pErrorTable, ParseTree::ParseTreeNode** pParsed, bool isExpression)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pErrorTable, "[VBHostedSession::Parse] 'pErrorTable' parameter is null");
    VerifyOutPtr(pParsed, "[VBHostedSession::Parse] 'pParsed' parameter is null");

    ASSERT(m_pCompiler, "[VBHostedSession::Parse] 'm_pCompiler' is null");
    ASSERT(m_SourceCode, "[VBHostedSession::Parse] 'm_SourceCode' is null");
    ASSERT(SysStringLen(m_SourceCode) > 0, "[VBHostedSession::Parse] 'm_SourceCode' is zero length string");

    VB_ENTRY();

    // Create the Scanner
    Scanner scanner
    (
        m_pCompiler,
        m_SourceCode,
        ::SysStringLen(m_SourceCode),
        0,
        0,
        0
    );

    // Create the Parser
    CompilerHost *pDefaultCompilerHost = m_pCompiler->GetDefaultCompilerHost();

    Parser parser
    (
        m_Allocator.GetNorlsAllocator(),
        m_pCompiler,
        pDefaultCompilerHost,
        false,
        LANGUAGE_CURRENT
    );

    if (isExpression)
    {
        IfFailGo(parser.ParseOneExpressionForHostedCompiler
            (
                &scanner, 
                pErrorTable, 
                (ParseTree::Expression**)pParsed, 
                NULL
            ));
    }
    else
    {
        IfFailGo(parser.ParseMethodBody
            (
                &scanner,
                pErrorTable,
                NULL,
                NULL,
                ParseTree::Statement::ProcedureBody,
                (ParseTree::MethodBodyStatement**)pParsed
            ));
    }

    VB_EXIT_LABEL();
}

STDMETHODIMP VBHostedSession::ParseExpression(ErrorTable* pErrorTable, ParseTree::Expression** pParsed)
{
    return Parse(pErrorTable, (ParseTree::ParseTreeNode**)pParsed, true);
}

STDMETHODIMP VBHostedSession::ParseStatements(ErrorTable* pErrorTable, ParseTree::MethodBodyStatement** pParsed)
{
    return Parse(pErrorTable, (ParseTree::ParseTreeNode**)pParsed, false);
}

STDMETHODIMP VBHostedSession::Analyze(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock, bool IsExpression)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(Parsed, "[VBHostedSession::Analyze] 'Parsed' parameter is null");
    VerifyInPtr(pErrorTable, "[VBHostedSession::Analyze] 'pErrorTable' parameter is null");
    VerifyOutPtr(CodeBlock, "[VBHostedSession::Analyze] 'CodeBlock' parameter is null");

    ASSERT(m_pCompiler, "[VBHostedSession::Analyze] 'm_pCompiler' is null");
    ASSERT(m_pCompilerProject, "[VBHostedSession::Analyze] 'm_pCompilerProject' is null");

    VB_ENTRY();
    
    ExternalSymbolFactory symFactory(&m_Allocator, m_pContext, m_pCompiler, GetExternalCompilerFile());
    
    TransientSymbolStore trasientsStore(m_pCompiler, m_Allocator.GetNorlsAllocator());
    BCSYM_Class * ExtClass;
    BCSYM_Hash * ExtClassMethodHash;
    BCSYM *TargetTypeSymbol = NULL;

    CompilerHost *CompilerHost = m_pCompilerProject->GetCompilerHost();

    Semantics semantics
    (
        m_Allocator.GetNorlsAllocator(),
        pErrorTable,
        m_pCompiler,
        CompilerHost,
        GetSourceFile(),
        &trasientsStore,
        false
    );

    BCSYM_Hash *externalUnnamedScope = m_pExternalUnnamedNamespace->GetHash();
    externalUnnamedScope->SetExternalSymbolSource(symFactory.GetTypeFactory(m_pExternalUnnamedNamespace));
    externalUnnamedScope->SetIsHostedDynamicHash(true);
	m_pExternalUnnamedNamespace->GetNamespaceRing()->SetContainsHostedDynamicHash(true);

    BCSYM_Hash *unnamedScope = m_pUnnamedNamespace->GetHash();
    unnamedScope->SetIsHostedDynamicHash(true);
	m_pUnnamedNamespace->GetNamespaceRing()->SetContainsHostedDynamicHash(true);

    ExtClass = CreateExternalClass(semantics, &symFactory);
    ExtClassMethodHash = CreateExternalClassMethodHash(semantics, ExtClass);
    symFactory.Initialize(ExtClass);

    // The following step needs to be done after there's a source file and also
    // after the type scope and script scope are tied to the unnamed namespace via
    // the external symbol factory.
    //
    ResolveImports(pErrorTable);

    // Ensure that all the projects (in particular Type Scope's have been compiled at least to CS_Bound).
    IfFailGo(m_pCompiler->CompileToBound(
        m_pCompilerProject->GetCompilerHost(),
        NULL, // ULONG *pcWarnings
        NULL, // ULONG *pcErrors,
        NULL // VbError **ppErrors
        ));

    if (static_cast<System::Type ^>(m_ExprTargetType) != nullptr)
    {
        // Target type of "Void" should have caused an exception earlier itself.
        ASSERT(!System::Void::typeid->Equals(m_ExprTargetType), "[VBHostedSession::Analyze] 'Void' target type unexpected");

        TargetTypeSymbol = symFactory.GetSymbolForType(m_ExprTargetType);
        IfFalseGo(TargetTypeSymbol, E_UNEXPECTED);

         if (TargetTypeSymbol->IsBad())
         {
             IfFalseGo(TargetTypeSymbol->IsNamedRoot() && TargetTypeSymbol->PNamedRoot()->GetErrid() != 0, E_UNEXPECTED);

             // Report error with type including if type is not found.
             TargetTypeSymbol->PNamedRoot()->ReportError(m_pCompiler, pErrorTable, NULL);

             // Return S_OK to indicate a compilation without any unexpected internal
             // compiler errors.
             //
             return S_OK;
         }
    }

    if (IsExpression)
    {
        // 



        ILTree::Expression *BoundTree = semantics.InterpretExpressionForHostedCompiler(
            (ParseTree::Expression*)Parsed,
            ExtClassMethodHash,
            TargetTypeSymbol
            );

        if (BoundTree != NULL && !pErrorTable->HasErrors())
        {
            // 

            DLRTreeETGenerator ExprTreeGenerator(m_pCompiler, m_Allocator.GetNorlsAllocator());
            VBHostedExpressionTreeSemantics ExprTreeSemantics(&semantics, &ExprTreeGenerator, m_Allocator.GetNorlsAllocator());

            try
            {
                DLRExpressionTree *ExprTree = ExprTreeSemantics.ConvertLambdaToExpressionTree(
                    BoundTree,
                    ExprForceRValue,
                    CompilerHost->GetFXSymbolProvider()->GetExpressionType()
                    );

                // Check for errors that could occur during ConvertLambda
                //
                if (!pErrorTable->HasErrors())
                {
                    Expression^ ManagedExprTree = ExprTreeGenerator.GetManagedExpressionTree(ExprTree);
                    *CodeBlock = Expression::Lambda(ManagedExprTree, (array<ParameterExpression^>^)nullptr);
                }
            }
            catch (BadNodeException&)
            {
                // Tree conversion had to be aborted due to an unconvertable tree.
                // The error has already been reported.

                ASSERT(pErrorTable->HasErrors(), "[VBHostedSession::Analyze] Should not throw a BadNodeException without having an error.");
            }
        }
    }
    else
    {
        // 


        // 



        semantics.InterpretStatementsForHostedCompiler(
            (ParseTree::ExecutableBlockStatement*)Parsed,
            ExtClass->GetHash()
            );
    }
    VB_EXIT_LABEL();
}

STDMETHODIMP VBHostedSession::AnalyzeExpression(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock)
{
    VerifyOutPtr(CodeBlock, "[VBHostedSession::AnalyzeExpression] 'CodeBlock' parameter is null");

    return Analyze(Parsed, pErrorTable, CodeBlock, true);
}

STDMETHODIMP VBHostedSession::AnalyzeStatement(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock)
{
    VerifyOutPtr(CodeBlock, "[VBHostedSession::AnalyzeStatement] 'CodeBlock' parameter is null");

    return Analyze(Parsed, pErrorTable, CodeBlock, false);
}

//Allocator throws; no errors returned to be propagated.
BCSYM_Class* VBHostedSession::CreateExternalClass(Semantics& semantics, ExternalSymbolFactory* SymFactory)
{
    ASSERT(SymFactory, "[VBHostedSession::CreateExternalClass] 'SymFactory' parameter is null");
    
    if (SymFactory == NULL)
        return NULL;
    
    BCSYM_Class* ExternalClass = NULL;
    
    ExternalClass = semantics.GetSymbols()->AllocClass(false);
    STRING* pstrName = m_pCompiler->AddString(L"$ExternalClass");

    DECLFLAGS DeclFlags = 0;
    DeclFlags = DECLF_Hidden | DECLF_NotInheritable;

    semantics.GetSymbols()->GetClass
        (
            NULL,// Location (unused)
            pstrName,
            pstrName,
            m_pUnnamedNamespace->GetName(),
            GetSourceFile(),
            semantics.GetFXSymbolProvider()->GetObjectType(),
            NULL, //BCSYM_Implements* ----List
            DeclFlags,
            t_bad, // Vtypes UnderlyingType
            NULL,//BCSYM_Variable* pvarMe
            NULL,//SymbolList* psymListChildren
            NULL,//SymbolList* psymListUnBindableChildren
            NULL,//BCSYM_GenericParam* pGenericParameters
            NULL,// SymbolList* psymList
            ExternalClass
        );

    ExternalClass->SetHash(semantics.GetSymbols()->GetHashTable(ExternalClass->GetNameSpace(), ExternalClass, true, 0, NULL));

    BCSYM_Hash* externalUnnamedScope = ExternalClass->GetHash();
    externalUnnamedScope->SetExternalSymbolSource(SymFactory->GetVariableFactory());
    externalUnnamedScope->SetIsHostedDynamicHash(true);
	GetSourceFile()->GetUnnamedNamespace()->GetNamespaceRing()->SetContainsHostedDynamicHash(true);
    semantics.GetSymbols()->AddSymbolToHash(GetSourceFile()->GetUnnamedNamespace()->GetHash(), ExternalClass, true, false, true);

    // Set the binding status for this class as "done" since this will only contain fields
    // which are pulled in incrementally and there is no other binding required.
    //
    // This will also ensure that there are no spurious asserts and also memory leaks
    // due to unnecessarily creating Bindable class instance which are then not freed.
    //
    ExternalClass->SetBindingDone(true);

    return ExternalClass;
}

//Allocator throws; no errors returned to be propagated.
BCSYM_Hash* VBHostedSession::CreateExternalClassMethodHash(Semantics& semantics, BCSYM_Class* ExternalClass)
{
    ASSERT(ExternalClass, "[VBHostedSession::CreateExternalClassMethodHash] 'ExternalClass' parameter is null");

    // To get the correct semantic analysis for name resolution in the hosted expression compiler,
    // we need to construct an artificial namespace environment.  Effectively, we will compile
    // expressions as if they were contained in the method body of a void, parameterless method, as in:
    //
    //     Class $ExternalClass
    //         Sub $ExternalClassMethod()
    //             <expressions are evaluated as if they were in-line here>
    //         End Sub
    //     End Class
    //
    // Consequently, we will construct a new method called "$ExternalClassMethod" and add it to the
    // "ExternalClass" hash table.
    //
    BCSYM_Hash* externalClassMethodHash = NULL;
    BCSYM_MethodDecl* externalClassMethod = semantics.GetSymbols()->AllocMethodDecl(false);
    STRING* externalClassMethodName = m_pCompiler->AddString(L"$ExternalClassMethod");
    BCSYM_Variable* dummyReturnVariable = NULL;
    DECLFLAGS externalClassMethodFlags = DECLF_Function | DECLF_Private | DECLF_Shared;

    // Define a new method with name "$ExternalClassMethod".
    semantics.GetSymbols()->GetProc
    (
        NULL, // Location* ploc
        externalClassMethodName,
        externalClassMethodName,
        NULL, // CodeBlockLocation* pCodeBlock
        NULL, // CodeBlockLocation* pProcBlock
        externalClassMethodFlags,
        NULL, // BCSYM* ptyp
        NULL, // BCSYM_Param* pparamList
        NULL, // BCSYM_Param* pparamReturn
        NULL, // STRING* pstrLibName
        NULL, // STRING* pstrAliasName
        SYNTH_None,
        NULL, // BCSYM_GenericParam* pGenericParmeters
        NULL, // SymbolList* psymbolList
        externalClassMethod->PProc()
    );

    // Construct the scope in externalClassMethodScope.
    Declared::MakeLocalsFromParams
    (
        m_pCompiler,
        m_Allocator.GetNorlsAllocator(),
        externalClassMethod->PProc(),
        NULL, // ErrorTable* ErrorLog
        true,
        &dummyReturnVariable,
        &externalClassMethodHash
    );

    // Add the method to the class's hash.
    Symbols::AddSymbolToHash
    (
        ExternalClass->GetHash(), 
        externalClassMethod->PProc(), 
        true, 
        true, 
        false
    );

    return externalClassMethodHash;
}

SourceFile* VBHostedSession::GetSourceFile()
{
    ASSERT(m_pSourceFile, "[VBHostedSession::GetSourceFile] 'm_pSourceFile' is null");
    return m_pSourceFile;
}

STDMETHODIMP VBHostedSession::PrepareSourceFile()
{
    ASSERT(m_pCompilerProject, "[VBHostedSession::GetSourceFile] 'm_pCompilerProject' is null");
    ASSERT(m_SourceCode, "[VBHostedSession::GetSourceFile] 'm_SourceCode' is null");
    ASSERT(SysStringLen(m_SourceCode) > 0, "[VBHostedSession::GetSourceFile] 'm_SourceCode' is zero length string");
    ASSERT(!m_pSourceFile, "[VBHostedSession::GetSourceFile] 'm_pSourceFile' is not null");
    ASSERT(m_pCompiler, "[VBHostedSession::GetSourceFile] 'm_pCompiler' is null");
    
    HRESULT hr = S_OK;
    
    // 




    // Create the semantics
    IfFailGo(m_pCompilerProject->AddBufferHelper
        (
            L" ",   // Needs to be on-zero length, else results in a compile error when reading the text file
            1 * sizeof(WCHAR),  // length of the string in bytes
            L"RandomFileName", 
            VSITEMID_NIL, 
            FALSE, 
            TRUE, 
            &m_pSourceFile
        ));

    m_pUnnamedNamespace = m_pCompiler->GetUnnamedNamespace(m_pSourceFile);

    m_pSourceFile->SetCompState(CS_Bound);
    m_pContext->SetOptions(m_pSourceFile);

Error:

    return hr;
}

CompilerFile* VBHostedSession::GetExternalCompilerFile()
{
    ASSERT(m_pExternalCompilerFile, "[VBHostedSession::GetExternalCompilerFile] 'm_pExternalCompilerFile' is null");
    return m_pExternalCompilerFile;
}

STDMETHODIMP VBHostedSession::CompileExpression (/*[in,out]*/ VbParsed *pVbParsed)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbParsed, "[VBHostedSession::CompileExpression] 'pVbParsed' parameter is null");

    ASSERT(m_pCompiler, "[VBHostedSession::CompileExpression] 'm_pCompiler' is null");

    HRESULT hr = S_OK;

    //Allocated on NorlsAllocator. No delete necessary. 
    //Cleaned up when the NorlsAllocator class is destroyed.
    ParseTree::Expression* expression = NULL;
    
    // Setting these after the project is setup so that the imports are not unnecessarily
    // bound before the scriptscope and typescope contexts are set up.
    //
    IfFailGo(m_pContext->SetImports(m_spVbCompilerProject));

    // Add a source file to the project.
    //
    // The source file is required to be passed into semantics, etc.
    // as the source file context.
    //
    IfFailGo(PrepareSourceFile());

    IfFailGo(m_pContext->SetOptions(m_spVbCompilerProject));

    IfTrueGo(pVbParsed->GetErrorTable()->HasErrors(), S_FALSE);

    IfFailGo(ParseExpression(pVbParsed->GetErrorTable(), &expression));
    IfTrueGo(pVbParsed->GetErrorTable()->HasErrors(), S_FALSE);
    IfFailGo(AnalyzeExpression(expression, pVbParsed->GetErrorTable(), pVbParsed->GetCodeBlock()));
    
Error:

    return hr;
}

STDMETHODIMP VBHostedSession::CompileStatements(/*[in,out]*/ VbParsed *pVbParsed)
{
    // Asserts are NOT necessary since the Verify* macros all invoke VSFAIL which is a VSASSERT wrapper.
    VerifyInPtr(pVbParsed, "[VBHostedSession::CompileStatements] 'pVbParsed' parameter is null");

    ASSERT(m_pCompiler, "[VBHostedSession::CompileStatements] 'm_pCompiler' is null");

    HRESULT hr = S_OK;

    //Allocated on NorlsAllocator. No delete necessary. 
    //Cleaned up when the NorlsAllocator class is destroyed.
    ParseTree::MethodBodyStatement* statement = NULL;

    // Setting these after the project is setup so that the imports are not unnecessarily
    // bound before the scriptscope and typescope contexts are set up.
    //
    IfFailGo(m_pContext->SetImports(m_spVbCompilerProject));

    // Add a source file to the project.
    //
    // The source file is required to be passed into semantics, etc.
    // as the source file context.
    //
    IfFailGo(PrepareSourceFile());

    IfFailGo(m_pContext->SetOptions(m_spVbCompilerProject));

    IfFailGo(ParseStatements(pVbParsed->GetErrorTable(), &statement));
    IfFailGo(AnalyzeStatement(statement, pVbParsed->GetErrorTable(), pVbParsed->GetCodeBlock()));

Error:

    return hr;
}

STDMETHODIMP VBHostedSession::SetupExternalCompilerProject()
{
    ASSERT(m_spVbCompilerHost, "[VBHostedSession::SetupExternalCompilerProject] 'm_spVbCompilerHost' is null");
    ASSERT(m_spVbCompiler, "[VBHostedSession::SetupExternalCompilerProject] 'm_spVbExternalCompiler' is null");
    ASSERT(m_pExternalCompilerProject, "[VBHostedSession::SetupExternalCompilerProject] 'm_pExternalCompilerProject' is null");

    HRESULT hr = S_OK;

    IfFailGo(m_pExternalCompilerProject->AddTypeScopeFile());
    IfFailGo(InitializeExternalCompilerProject());

Error:

    return hr;
}

STDMETHODIMP VBHostedSession::InitializeExternalCompilerProject()
{
    ASSERT(m_pCompiler, "[VBHostedSession::InitializeExternalCompilerProject] 'm_pCompiler' is null");
    ASSERT(m_pCompilerProject, "[VBHostedSession::InitializeExternalCompilerProject] 'm_pCompilerProject' is null");
    ASSERT(m_pExternalCompilerProject, "[VBHostedSession::InitializeExternalCompilerProject] 'm_pExternalCompilerProject' is null");

    HRESULT hr = S_OK;
    CompilerFile* pFile = NULL;

    // Get the Compiler File and Unnamed Namespace from the project.

    FileIterator iterator(m_pExternalCompilerProject);
    pFile = iterator.Next();
    ASSERT(pFile, "[VBHostedSession::InitializeExternalCompilerProject] Could not get Compiler File from External Project");
    ASSERT(!iterator.Next(), "[VBHostedSession::InitializeExternalCompilerProject] External Project has more than one file");
    IfFalseGo(pFile, E_UNEXPECTED);
    m_pExternalCompilerFile = pFile;

    //Need to promote the CompState to ExternalCompilerProject's CompState in order
    // to avoid issues when the proect has already been compiled to bound -- 
    // decompilation is not supported in the command line compiler.
    m_pExternalCompilerFile->SetCompState(m_pExternalCompilerProject->GetCompState());
    m_pExternalUnnamedNamespace = m_pCompiler->GetUnnamedNamespace(m_pExternalCompilerFile);
    ASSERT(m_pExternalCompilerFile, "[VBHostedSession::InitializeExternalCompilerProject] Could not get m_pExternalCompilerFile");
    ASSERT(m_pExternalUnnamedNamespace, "[VBHostedSession::InitializeExternalCompilerProject] Could not get m_pExternalUnnamedNamespace");

Error:

    return hr;
}

void VBHostedSession::ResolveImports(ErrorTable *pErrorTable)
{
    CompilerProject::ProjectLevelImportsList *Imports = m_pCompilerProject->GetProjectLevelImportsList();

    // Parse the imports
    Imports->ReBuildAllImportTargets();

    ErrorTable *ImportsErrorLog = Imports->GetImportsErrorLog();

    if (Imports->GetAllImportedTargets())
    {
        // Resolve imports

        Bindable::ResolveImportsList(
            Imports->GetAllImportedTargets(),           // List of project level imports
            NULL,                                       // No associated source file
            m_pCompilerProject,                         // Associated project
            m_Allocator.GetNorlsAllocator(),            // Allocator
            NULL,                                       // No associated line marker table
            ImportsErrorLog,                            // use the special imports error table to put any imports errors in
            m_pCompiler,                                // The current compiler instance
            NULL                                        // No compilation cache - same as core compiler for this scenario
            );

        // For any generic type import targets, ensure that the type arguments satisfy the generic type's
        // constraints

        Symbols SymbolFactory(m_pCompiler, m_Allocator.GetNorlsAllocator(), NULL);

        for(ImportedTarget *Import = Imports->GetAllImportedTargets();
            Import;
            Import = Import->m_pNext)
        {
            Bindable::CheckGenericConstraintsForImportsTarget(
                Import,
                ImportsErrorLog,                        // use the special imports error table to put any imports errors in
                m_pCompilerProject->GetCompilerHost(),
                m_pCompiler,
                &SymbolFactory,
                NULL                                    // No compilation cache - same as core compiler for this scenario
                );
        }
    }

    // Propagate the errors/warnings occuring during parsing and binding of the imports.
    pErrorTable->MoveNoStepErrors(ImportsErrorLog);
}
