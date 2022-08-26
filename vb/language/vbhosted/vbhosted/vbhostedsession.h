#pragma once

#include "VBContext.h"
#include "VBParsed.h"
#include "..\..\Compiler\Parser\ParseTrees.h"
#include "..\..\Compiler\SourceFile.h"
#include "VBHostedAllocator.h"
#include "ExternalSymbolFactory.h"

using System::Linq::Expressions::Expression;
using System::Linq::Expressions::LambdaExpression;
using System::Linq::Expressions::ParameterExpression;

class VBHostedSession
#if FV_DEADBEEF
    : public Deadbeef<VBHostedSession> // Must be last base class!
#endif
{
public:
    VBHostedSession(BSTR sourceCode, IVbCompiler* pCompiler, IVbCompilerHost* pCompilerHost, IVbCompilerProject* pVbCompilerProject, 
        CompilerProject *pExternalCompilerProject, VbContext* pContext, gcroot<System::Type ^> TargetType);
    virtual ~VBHostedSession();

    STDMETHODIMP CompileExpression(VbParsed *pVbParsed);
    STDMETHODIMP CompileStatements(VbParsed *pVbParsed);

private:

    STDMETHODIMP Parse(ErrorTable* pErrorTable, ParseTree::ParseTreeNode** pParsed, bool isExpression);
    STDMETHODIMP ParseExpression(ErrorTable* pErrorTable, ParseTree::Expression** pParsed);
    STDMETHODIMP ParseStatements(ErrorTable* pErrorTable, ParseTree::MethodBodyStatement** pParsed);

    STDMETHODIMP Analyze(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock, bool IsExpression);
    STDMETHODIMP AnalyzeExpression(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock);
    STDMETHODIMP AnalyzeStatement(ParseTree::ParseTreeNode* Parsed, ErrorTable* pErrorTable, gcroot<LambdaExpression^> *CodeBlock);

    BCSYM_Class* CreateExternalClass(Semantics& semantics, ExternalSymbolFactory* SymFactory);
    BCSYM_Hash* CreateExternalClassMethodHash(Semantics& semantics, BCSYM_Class* ExternalClass);

    SourceFile* GetSourceFile();
    STDMETHODIMP PrepareSourceFile();
    
    CompilerFile* GetExternalCompilerFile();

    // CreateExternalCompilerProject creates a new compiler Project which acts
    // as the context for Types from Type Scope.
    STDMETHODIMP InitializeExternalCompilerProject();
    STDMETHODIMP SetupExternalCompilerProject();

    void ResolveImports(ErrorTable *pErrorTable);

    //Ref counted data members
    CComPtr<IVbCompiler> m_spVbCompiler;
    CComPtr<IVbCompilerHost> m_spVbCompilerHost;
    CComPtr<IVbCompilerProject> m_spVbCompilerProject;

    //Internal non COM members
    SourceFile* m_pSourceFile;
    CompilerFile* m_pExternalCompilerFile;
    
    //Helper accessors. Sometimes need the internal classes, not the interfaces
    Compiler *m_pCompiler;
    CompilerProject *m_pCompilerProject;
    CompilerProject *m_pExternalCompilerProject;
    BCSYM_Namespace* m_pUnnamedNamespace;
    BCSYM_Namespace* m_pExternalUnnamedNamespace;

    //Context pointer. Not an actual COM interface.
    VbContext* m_pContext;

    //Our copy of the source code
    CComBSTR m_SourceCode;

    //Type context (in terms of conversion/casting) that the expression needs to
    //be compiled in. This is not applicable when compiling statements.
    gcroot<System::Type ^> m_ExprTargetType;

    //Stack allocated internal private allocator
    VBHostedAllocator m_Allocator;
};

