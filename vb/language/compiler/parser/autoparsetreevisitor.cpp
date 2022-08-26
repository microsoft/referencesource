#include "Stdafx.h"

void AutoParseTreeVisitor::Default(void * pNode)
{
    Assume(false, L"Unexpected node type! Please add aupport for any new parse tree nodes to the AutoParseTreeVisitor class!");
}

void AutoParseTreeVisitor::VisitParseTreeNodeBase
(
    ParseTree::ParseTreeNode * pParseTreeNode
)
{
    //do nothing
}

void AutoParseTreeVisitor::VisitExpressionBase
(
    ParseTree::Expression * pExpr
)
{
    VisitParseTreeNodeBase(pExpr);
}

void AutoParseTreeVisitor::VisitNameBase
(
    ParseTree::Name * pName
)
{
    VisitParseTreeNodeBase(pName);
}

void AutoParseTreeVisitor::VisitTypeBase
(
    ParseTree::Type * pType
)
{
    VisitParseTreeNodeBase(pType);
}

void AutoParseTreeVisitor::VisitConstraintBase
(
    ParseTree::Constraint * pConstraint
)
{
    VisitParseTreeNodeBase( pConstraint);
}

void AutoParseTreeVisitor::VisitStatementBase
(
    ParseTree::Statement * pStatement
)
{
    VisitParseTreeNodeBase(pStatement);
    
    ThrowIfNull(pStatement);

    if (pStatement->Comments)
    {
        VisitCommentList(pStatement->Comments);
    }
}

void AutoParseTreeVisitor::VisitCaseBase
(
    ParseTree::Case * pCase
)
{
    //do nothing
}

void AutoParseTreeVisitor::VisitImportDirectiveBase
(
    ParseTree::ImportDirective * pImportDirective
)
{
    VisitParseTreeNodeBase(pImportDirective);
}


void AutoParseTreeVisitor::VisitSyntaxErrorStatement
(
    ParseTree::Statement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEmptyStatement
(
    ParseTree::Statement * pStatement
)
{
    VisitStatementBase(pStatement);
}


void AutoParseTreeVisitor::VisitCCConstStatement
(
    ParseTree::CCConstStatement* pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitStatementBase(pStatement);
    
    if (pStatement->Value)
    {
        VisitExpression(pStatement->Value);
    }
}

void AutoParseTreeVisitor::VisitCCElseStatement
(
    ParseTree::CCElseStatement * pStatement
)
{
    VisitCCBranchStatement(pStatement);
}

void AutoParseTreeVisitor::VisitCCEndIfStatement
(
    ParseTree::CCEndStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExpressionStatementBase
(
    ParseTree::ExpressionStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);
    
    if (pStatement->Operand)
    {
        VisitExpression(pStatement->Operand);
    }
}

void AutoParseTreeVisitor::VisitCCBranchStatement
(
    ParseTree::CCBranchStatement* pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitCCIfStatement
(
    ParseTree::CCIfStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitCCBranchStatement(pStatement);

    if (pStatement->TerminatingConstruct)
    {
        VisitStatement(pStatement->TerminatingConstruct);
    }
}

void AutoParseTreeVisitor::VisitCCElseIfStatement
(
    ParseTree::CCIfStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitCCBranchStatement(pStatement);

    if (pStatement->TerminatingConstruct)
    {
        VisitStatement(pStatement->TerminatingConstruct);
    }
}

void AutoParseTreeVisitor::VisitBlockStatementBase
(
    ParseTree::BlockStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Children)
    {
        VisitStatementList(pStatement->Children);
    }
}

void AutoParseTreeVisitor::VisitRegionStatement
(
    ParseTree::RegionStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitBlockStatementBase(pStatement);

    if (pStatement->Title)
    {
        VisitExpression(pStatement->Title);
    }
}

void AutoParseTreeVisitor::VisitTypeStatementBase
(
    ParseTree::TypeStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitBlockStatementBase(pStatement);

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList(pStatement->Attributes);
    }

    if (pStatement->Specifiers)
    {
        VisitSpecifierList(pStatement->Specifiers);
    }

    if (pStatement->GenericParameters)
    {
        VisitGenericParameterList(pStatement->GenericParameters);
    }
}


void AutoParseTreeVisitor::VisitStructureStatement
(
    ParseTree::TypeStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    VisitTypeStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEnumStatement
(
    ParseTree::EnumTypeStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitTypeStatementBase(pStatement);

    if (pStatement->UnderlyingRepresentation)
    {
        VisitType(pStatement->UnderlyingRepresentation);
    }
}

void AutoParseTreeVisitor::VisitInterfaceStatement
(
    ParseTree::TypeStatement * pStatement
)
{
    VisitTypeStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitClassStatement
(
    ParseTree::TypeStatement * pStatement
)
{
    VisitTypeStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitModuleStatement
(
    ParseTree::TypeStatement * pStatement
)
{
    VisitTypeStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitNamespaceStatement
(
    ParseTree::NamespaceStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitBlockStatementBase(pStatement);

    if (pStatement->Name)
    {
        VisitName(pStatement->Name);
    }
}

void AutoParseTreeVisitor::VisitMethodSignatureStatementBase
(
    ParseTree::MethodSignatureStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Specifiers)
    {
        VisitSpecifierList(pStatement->Specifiers);
    }

    if (pStatement->Parameters)
    {
        VisitParameterList(pStatement->Parameters);
    }

    if (pStatement->ReturnType)
    {
        VisitType(pStatement->ReturnType);
    }

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList( pStatement->Attributes);
    }

    if (pStatement->ReturnTypeAttributes)
    {
        VisitAttributeSpecifierList(pStatement->ReturnTypeAttributes);
    }
}

void AutoParseTreeVisitor::VisitMethodDeclarationStatementBase
(
    ParseTree::MethodDeclarationStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitMethodSignatureStatementBase(pStatement);

    if (pStatement->Handles)
    {
        VisitNameList(pStatement->Handles);
    }

    if (pStatement->Implements)
    {
        VisitNameList(pStatement->Implements);
    }

    if (pStatement->GenericParameters)
    {
        VisitGenericParameterList(pStatement->GenericParameters);
    }
}

void AutoParseTreeVisitor::VisitMethodDefinitionStatementBase
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitMethodDeclarationStatementBase(pStatement);

    if (pStatement->Body)
    {
        VisitStatement(pStatement->Body);
    }

    if (pStatement->TerminatingConstruct)
    {
        VisitStatement(pStatement->TerminatingConstruct);
    }

    if (pStatement->StaticLocalDeclarations)
    {
        VisitStatementList(pStatement->StaticLocalDeclarations);
    }
}

void AutoParseTreeVisitor::VisitProcedureDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitFunctionDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitConstructorDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOperatorDeclarationStatement
(
    ParseTree::OperatorDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitDelegateProcedureDeclarationStatement
(
    ParseTree::DelegateDeclarationStatement * pStatement
)
{
    VisitMethodDeclarationStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitDelegateFunctionDeclarationStatement
(
    ParseTree::DelegateDeclarationStatement * pStatement
)
{
    VisitMethodDeclarationStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEventDeclarationStatement
(
    ParseTree::EventDeclarationStatement * pStatement
)
{
    VisitMethodDeclarationStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitBlockEventDeclarationStatement
(
    ParseTree::BlockEventDeclarationStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitBlockStatementBase(pStatement);

    if (pStatement->EventSignature)
    {
        VisitStatement(pStatement->EventSignature);
    }
 }

void AutoParseTreeVisitor::VisitAddHandlerDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
 }

void AutoParseTreeVisitor::VisitRemoveHandlerDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
 }

void AutoParseTreeVisitor::VisitRaiseEventDeclarationStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitForeignMethodDeclarationStatementBase
(
    ParseTree::ForeignMethodDeclarationStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitMethodSignatureStatementBase(pStatement);

    if (pStatement->Library)
    {
        VisitExpression(pStatement->Library);
    }

    if (pStatement->Alias)
    {
        VisitExpression(pStatement->Alias);
    }
    
}

void AutoParseTreeVisitor::VisitForeignProcedureDeclarationStatement
(
    ParseTree::ForeignMethodDeclarationStatement * pStatement
)
{
    VisitForeignMethodDeclarationStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitForeignFunctionDeclarationStatement
(
    ParseTree::ForeignMethodDeclarationStatement * pStatement
)
{
    VisitForeignMethodDeclarationStatementBase( pStatement);    
}

void AutoParseTreeVisitor::VisitForeignFunctionNoneStatement
(
    ParseTree::ForeignMethodDeclarationStatement * pStatement
)
{
    VisitForeignMethodDeclarationStatementBase(pStatement);    
}

void AutoParseTreeVisitor::VisitPropertyStatementBase
(
    ParseTree::PropertyStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitBlockStatementBase(pStatement);

    if (pStatement->Specifiers)
    {
        VisitSpecifierList( pStatement->Specifiers);
    }

    if (pStatement->Parameters)
    {
        VisitParameterList(pStatement->Parameters);
    }

    if (pStatement->PropertyType)
    {
        VisitType(pStatement->PropertyType);
    }

    if (pStatement->Implements)
    {
        VisitNameList(pStatement->Implements);
    }

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList(pStatement->Attributes);
    }

    if (pStatement->PropertyTypeAttributes)
    {
        VisitAttributeSpecifierList(pStatement->PropertyTypeAttributes);
    }    
}    

void AutoParseTreeVisitor::VisitPropertyStatement
(
    ParseTree::PropertyStatement * pStatement
)
{
    VisitPropertyStatementBase(pStatement);
}


void AutoParseTreeVisitor::VisitAutoPropertyStatement
(
    ParseTree::AutoPropertyStatement * pStatement
)
{
    VisitPropertyStatementBase(pStatement);

    if (pStatement->pAutoPropertyDeclaration)
    {
        VisitAutoPropertyInit(pStatement->pAutoPropertyDeclaration);
    }
}

void AutoParseTreeVisitor::VisitPropertyGetStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitPropertySetStatement
(
    ParseTree::MethodDefinitionStatement * pStatement
)
{
    VisitMethodDefinitionStatementBase( pStatement);
}


void AutoParseTreeVisitor::VisitEnumeratorStatementBase
(
    ParseTree::EnumeratorStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList( pStatement->Attributes);
    }
    
}

void AutoParseTreeVisitor::VisitEnumeratorStatement
(
    ParseTree::EnumeratorStatement * pStatement
)
{
    VisitEnumeratorStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEnumeratorWithValueStatement
(
    ParseTree::EnumeratorWithValueStatement * pStatement
)
{

    VisitEnumeratorStatementBase( pStatement);

    if (pStatement->Value)
    {
        VisitExpression(pStatement->Value);
    }
}

void AutoParseTreeVisitor::VisitVariableDeclarationStatement
(
    ParseTree::VariableDeclarationStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitStatementBase(pStatement);

    if (pStatement->Specifiers)
    {
        VisitSpecifierList( pStatement->Specifiers);
    }

    if (pStatement->Declarations)
    {
        VisitVariableDeclarationList(pStatement->Declarations);
    }

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList(pStatement->Attributes);
    }
}

void AutoParseTreeVisitor::VisitTypeListStatementBase
(
    ParseTree::TypeListStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Types)
    {
        VisitTypeList(pStatement->Types);
    }
}

void AutoParseTreeVisitor::VisitImplementsStatement
(
    ParseTree::TypeListStatement * pStatement
)
{
    VisitTypeListStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitInheritsStatement
(
    ParseTree::TypeListStatement * pStatement
)
{
    VisitTypeListStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitImportsStatement
(
    ParseTree::ImportsStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Imports)
    {
        VisitImportDirectiveList( pStatement->Imports);
    }
}

void AutoParseTreeVisitor::VisitOptionUnknownStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionInvalidStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionCompareNoneStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionCompareTextStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionCompareBinaryStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionExplicitOnStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionExplicitOffStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionStrictOnStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionStrictOffStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionInferOnStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOptionInferOffStatement
(
    ParseTree::OptionStatement * pStatement
)
{
    VisitStatementBase(pStatement);
}



void AutoParseTreeVisitor::VisitAttributeStatement
(
    ParseTree::AttributeStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    VisitStatementBase( pStatement);

    if (pStatement->Attributes)
    {
        VisitAttributeSpecifierList( pStatement->Attributes);
    }
}

void AutoParseTreeVisitor::VisitFileStatement
(
    ParseTree::FileBlockStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitBlockStatementBase(pStatement);

    if (pStatement->SourceDirectives)
    {
        VisitExternalSourceDirective(pStatement->SourceDirectives);
    }    
}

void AutoParseTreeVisitor::VisitExecutableBlockStatementBase
(
    ParseTree::ExecutableBlockStatement * pStatement
)
{
    VisitBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitProcedureBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitPropertyGetBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase( pStatement);    
}

void AutoParseTreeVisitor::VisitPropertySetBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitFunctionBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOperatorBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAddHandlerBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitRemoveHandlerBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitRaiseEventBodyStatement
(
    ParseTree::MethodBodyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitLambdaBodyStatement
(
    ParseTree::LambdaBodyStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitExecutableBlockStatementBase( pStatement);

    if (pStatement->pDefinedLabels)
    {
        VisitStatementList(pStatement->pDefinedLabels);
    }

    if (pStatement->pLastLabelLinked)
    {
        VisitStatementList(pStatement->pLastLabelLinked);
    }

    if (pStatement->pReturnType)
    {
        VisitTypeBase(pStatement->pReturnType);
    }

    if (pStatement->pReturnTypeAttributes)
    {
        VisitAttributeSpecifierList( pStatement->pReturnTypeAttributes);
    }
}

void AutoParseTreeVisitor::VisitHiddenBlockStatement
( 
    ParseTree::HiddenBlockStatement * pStatement
)
{
    VisitBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitCommentBlockStatement
(
    ParseTree::CommentBlockStatement * pStatement
)
{
    VisitBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExpressionBlockStatementBase
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitExecutableBlockStatementBase(pStatement);

    if (pStatement->Operand)
    {
        VisitExpression(pStatement->Operand);
    }
}

void AutoParseTreeVisitor::VisitBlockIfStatement
(
    ParseTree::IfStatement * pStatement
)
{
    VisitExpressionBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitLineIfStatement
(
    ParseTree::IfStatement * pStatement
)
{
    VisitExpressionBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitElseIfStatement
(
    ParseTree::ElseIfStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitBlockElseStatement
(
    ParseTree::ElseStatement * pStatement
)
{
    VisitExecutableBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitLineElseStatement
(
    ParseTree::ElseStatement * pStatement
)
{
    VisitExecutableBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitSelectStatement
(
    ParseTree::SelectStatement * pStatement
)
{
    VisitExpressionBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitCaseStatement
(
    ParseTree::CaseStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitExecutableBlockStatementBase(pStatement);

    if (pStatement->Cases)
    {
        VisitCaseList(pStatement->Cases);
    }
}

void AutoParseTreeVisitor::VisitCaseElseStatement
(
    ParseTree::ExecutableBlockStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitTryStatement
(
    ParseTree::ExecutableBlockStatement * pStatement
)
{
    VisitExecutableBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitCatchStatement
(
    ParseTree::CatchStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitExecutableBlockStatementBase(pStatement);

    if (pStatement->Type)
    {
        VisitType(pStatement->Type);
    }

    if (pStatement->WhenClause)
    {
        VisitExpression(pStatement->WhenClause);
    }   
}

void AutoParseTreeVisitor::VisitFinallyStatement
(
    ParseTree::FinallyStatement * pStatement
)
{
    VisitExecutableBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitForStatementBase
(
    ParseTree::ForStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    VisitExecutableBlockStatementBase(pStatement);

    if (pStatement->ControlVariable)
    {
        VisitExpression(pStatement->ControlVariable);
    }

    if (pStatement->NextVariable)
    {
        VisitExpression(pStatement->NextVariable);
    }

    if (pStatement->ControlVariableDeclaration)
    {
        VisitStatement(pStatement->ControlVariableDeclaration);
    }
}

void AutoParseTreeVisitor::VisitForFromToStatement
(
    ParseTree::ForFromToStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitForStatementBase(pStatement);

    if (pStatement->InitialValue)
    {
        VisitExpression(pStatement->InitialValue);
    }

    if (pStatement->FinalValue)
    {
        VisitExpression(pStatement->FinalValue);
    }

    if (pStatement->IncrementValue)
    {
        VisitExpression(pStatement->IncrementValue);
    }
}

void AutoParseTreeVisitor::VisitForEachInStatement
(
    ParseTree::ForEachInStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitForStatementBase(pStatement);

    if (pStatement->Collection)
    {
        VisitExpression(pStatement->Collection);
    }
}

void AutoParseTreeVisitor::VisitWhileStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitDoWhileTopTestStatement
(
    ParseTree::TopTestDoStatement * pStatement
)
{
    VisitExpressionBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitDoUntilTopTestStatement
(
    ParseTree::TopTestDoStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitDoWhileBottomTestStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitDoUntilBottomTestStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitDoForeverStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitUsingStatement
(
    ParseTree::UsingStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitExecutableBlockStatementBase(pStatement);

    if (pStatement->ControlExpression)
    {
        VisitExpression(pStatement->ControlExpression);
    }

    if (pStatement->ControlVariableDeclaration)
    {
        VisitStatement(pStatement->ControlVariableDeclaration);
    }
}

void AutoParseTreeVisitor::VisitWithStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndNextStatement
(
    ParseTree::EndNextStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Variables)
    {
        VisitExpressionList(pStatement->Variables);
    }
}

void AutoParseTreeVisitor::VisitEndLoopWhileStatement
(
    ParseTree::BottomTestLoopStatement * pStatement
)
{
    VisitExpressionStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitEndLoopUntilStatement
(
    ParseTree::BottomTestLoopStatement * pStatement
)
{
    VisitExpressionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitReturnStatement
(
    ParseTree::ExpressionStatement * pStatement
)
{
    VisitExpressionStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitYieldStatement
(
    ParseTree::ExpressionStatement * pStatement
)
{
    VisitExpressionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAwaitStatement
(
    ParseTree::ExpressionStatement * pStatement
)
{
    VisitExpressionStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitCallStatement
(
    ParseTree::CallStatement * pStatement
)
{
    ThrowIfNull(pStatement);
    
    VisitStatementBase(pStatement);

    if (pStatement->Target)
    {
        VisitExpression(pStatement->Target);
    }

    VisitParenthesizedArgumentList(&pStatement->Arguments);
}

void AutoParseTreeVisitor::VisitRaiseEventStatement
(
    ParseTree::RaiseEventStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);
    
    VisitParenthesizedArgumentList(&pStatement->Arguments);
}

void AutoParseTreeVisitor::VisitAssignStatementBase
(
    ParseTree::AssignmentStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Target)
    {
        VisitExpression(pStatement->Target);
    }

    if (pStatement->Source)
    {
        VisitExpression(pStatement->Source);
    }    
    
}

void AutoParseTreeVisitor::VisitAssignStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignPlusStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignMinusStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignMultiplyStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignDivideStatement
(

    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignPowerStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignIntegralDivideStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignConcatenateStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignShiftLeftStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignShiftRightStatement
(
    ParseTree::AssignmentStatement * pStatement
)
{
    VisitAssignStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitAssignMidStatement
(
    ParseTree::AssignMidStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Target)
    {
        VisitExpression(pStatement->Target);
    }

    if (pStatement->Start)
    {
        VisitExpression(pStatement->Start);
    }

    if (pStatement->Length)
    {
        VisitExpression(pStatement->Length);
    }

    if (pStatement->Source)
    {
        VisitExpression(pStatement->Source);
    }
}

void AutoParseTreeVisitor::VisitEraseStatement
(
    ParseTree::EraseStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Arrays)
    {
        VisitExpressionList(pStatement->Arrays);
    }
}

void AutoParseTreeVisitor::VisitErrorStatement
(
    ParseTree::ExpressionStatement * pStatement
)
{
    VisitExpressionStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitThrowStatement
(
    ParseTree::ExpressionStatement * pStatement
)
{
    VisitExpressionStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitRedimStatement
(
    ParseTree::RedimStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Redims)
    {
        VisitExpressionList( pStatement->Redims);
    }
}

void AutoParseTreeVisitor::VisitHandlerStatementBase
(
    ParseTree::HandlerStatement * pStatement
)
{
    ThrowIfNull(pStatement);

    VisitStatementBase(pStatement);

    if (pStatement->Event)
    {
        VisitExpression(pStatement->Event);
    }

    if (pStatement->Delegate)
    {
        VisitExpression(pStatement->Delegate);
    }
}    

void AutoParseTreeVisitor::VisitAddHandlerStatement
(
    ParseTree::HandlerStatement * pStatement
)
{
    VisitHandlerStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitRemoveHandlerStatement
(
    ParseTree::HandlerStatement * pStatement
)
{
    VisitHandlerStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitSyncLockStatement
(
    ParseTree::ExpressionBlockStatement * pStatement
)
{
    VisitExpressionBlockStatementBase( pStatement);
}

void AutoParseTreeVisitor::VisitEndIfStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndUsingStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndWithStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndSelectStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndStructureStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndEnumStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndInterfaceStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndClassStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndModuleStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndNamespaceStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndSubStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndFunctionStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndGetStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndSetStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndPropertyStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndOperatorStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndEventStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndAddHandlerStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndRemoveHandlerStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndRaiseEventStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndWhileStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndLoopStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndTryStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndSyncLockStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndRegionStatement
(
    ParseTree::CCEndStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndCommentBlockStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndUnknownStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndInvalidStatement
(
    ParseTree::EndBlockStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitLabelStatement
(
    ParseTree::LabelReferenceStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitGotoStatement
(
    ParseTree::LabelReferenceStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitOnErrorStatement
(
    ParseTree::OnErrorStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitResumeStatement
(
    ParseTree::ResumeStatement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitStopStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitEndStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitContinueDoStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitContinueForStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitContinueWhileStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitContinueUnknownStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitContinueInvalidStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitDoStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitForStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitSubStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitFunctionStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitOperatorStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitPropertyStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitTryStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitSelectStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitWhileStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitUnknownStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitExitInvalidStatement
(
    ParseTree::Statement *pStatement
)
{
    VisitStatementBase(pStatement);
}

void AutoParseTreeVisitor::VisitSyntaxErrorExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNameExpression
(
    ParseTree::NameExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitMeExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitMyBaseExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitMyClassExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGlobalNameSpaceExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitIntegralLiteralExpression
(
    ParseTree::IntegralLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCharacterLiteralExpression
(
    ParseTree::CharacterLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitBooleanLiteralExpression
(
    ParseTree::BooleanLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitDecimalLiteralExpression
(
    ParseTree::DecimalLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitFloatingLiteralExpression
(
    ParseTree::FloatingLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitDateLiteralExpression
(
    ParseTree::DateLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitStringLiteralExpression
(
    ParseTree::StringLiteralExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNothingExpression
(
    ParseTree::Expression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlNameExpression
(
    ParseTree::XmlNameExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlCharDataExpression
(
    ParseTree::XmlCharDataExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlReferenceExpression
(
    ParseTree::XmlReferenceExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGetXmlNamespaceExpression
(
    ParseTree::GetXmlNamespaceExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitAlreadyBoundSymbolExpression
(
    ParseTree::AlreadyBoundSymbolExpression *pExpr
)
{
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitUnaryExpressionBase
(
    ParseTree::UnaryExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Operand)
    {
        VisitExpression(pExpr->Operand);
    }
}

void AutoParseTreeVisitor::VisitParenthesizedExpression
(
    ParseTree::ParenthesizedExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCallOrIndexExpression
(
    ParseTree::CallOrIndexExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Target)
    {
        VisitExpression(pExpr->Target);
    }

    VisitParenthesizedArgumentList( &pExpr->Arguments);
}

void AutoParseTreeVisitor::VisitQualifiedExpressionBase
(
    ParseTree::QualifiedExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Base)
    {
        VisitExpression(pExpr->Base);
    }

    if (pExpr->Name)
    {
        VisitExpression(pExpr->Name);
    }
    
}

void AutoParseTreeVisitor::VisitDotQualifiedExpression
(
    ParseTree::QualifiedExpression * pExpr
)
{
    VisitQualifiedExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitBangQualifiedExpression
(
    ParseTree::QualifiedExpression * pExpr
)
{
    VisitQualifiedExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlElementsQualifiedExpression
(
    ParseTree::QualifiedExpression * pExpr
)
{
    VisitQualifiedExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlAttributeQualifiedExpression
(
    ParseTree::QualifiedExpression * pExpr
)
{
    VisitQualifiedExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlDescendantsQualifiedExpression
(
    ParseTree::QualifiedExpression * pExpr
)
{
    VisitQualifiedExpressionBase( pExpr);
}

void AutoParseTreeVisitor::VisitGenericQualifiedExpression
(
    ParseTree::GenericQualifiedExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Base)
    {
        VisitExpression(pExpr->Base);
    }

    VisitGenericArguments(&pExpr->Arguments);
}

void AutoParseTreeVisitor::VisitCastBooleanExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastCharacterExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastDateExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastDoubleExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastSignedByteExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastByteExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastShortExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastUnsignedShortExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastIntegerExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastUnsignedIntegerExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastLongExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastUnsignedLongExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastDecimalExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastSingleExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastStringExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitCastObjectExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitTypeValueExpressionBase
(
    ParseTree::TypeValueExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Value)
    {
        VisitExpression(pExpr->Value);
    }

    if (pExpr->TargetType)
    {
        VisitType(pExpr->TargetType);
    }
}

void AutoParseTreeVisitor::VisitConversionExpression
(
    ParseTree::ConversionExpression* pExpr
)
{
    VisitTypeValueExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitDirectCastExpression
(
    ParseTree::ConversionExpression * pExpr
)
{
    VisitTypeValueExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitTryCastExpression
(
    ParseTree::ConversionExpression * pExpr
)
{
    VisitTypeValueExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNegateExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNotExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitUnaryPlusExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitAddressOfExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitAwaitExpression
(
    ParseTree::UnaryExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitBinaryExpressionBase
(
    ParseTree::BinaryExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Left)
    {
        VisitExpression(pExpr->Left);
    }

    if (pExpr->Right)
    {
        VisitExpression(pExpr->Right);
    }
}

void AutoParseTreeVisitor::VisitPlusExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitMinusExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitMultiplyExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitDivideExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitPowerExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitIntegralDivideExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitConcatenateExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitShiftLeftExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitShiftRightExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitModulusExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitOrExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitOrElseExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXorExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitAndExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitAndAlsoExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitLikeExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitIsExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitIsNotExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitEqualExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNotEqualExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitLessExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitLessEqualExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGreaterEqualExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGreaterExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlExpressionBase
(
    ParseTree::XmlExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Content)
    {
        VisitExpressionList(pExpr->Content);
    }
}

void AutoParseTreeVisitor::VisitXmlDocumentExpression
(
    ParseTree::XmlDocumentExpression * pExpr
)
{
    VisitXmlExpressionBase(pExpr);

    if (pExpr->Attributes)
    {
        VisitExpressionList(pExpr->Attributes);
    }
}

void AutoParseTreeVisitor::VisitXmlElementExpression
(
    ParseTree::XmlElementExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitXmlExpressionBase(pExpr);

    if (pExpr->Name)
    {
        VisitExpression(pExpr->Name);
    }

    if (pExpr->Attributes)
    {
        VisitExpressionList(pExpr->Attributes);
    }

    
}

void AutoParseTreeVisitor::VisitXmlAttributeExpression
(
    ParseTree::XmlAttributeExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Name)
    {
        VisitExpression(pExpr->Name);
    }

    if (pExpr->Value)
    {
        VisitExpression(pExpr->Value);
    }
}

void AutoParseTreeVisitor::VisitXmlAttributeValueListExpression
(
    ParseTree::XmlExpression * pExpr
)
{
    VisitXmlExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlCDataExpression
(
    ParseTree::XmlExpression * pExpr
)
{
    VisitXmlExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlPIExpression
(
    ParseTree::XmlPIExpression * pExpr
)
{
    ThrowIfNull(pExpr);
    
    VisitXmlExpressionBase(pExpr);

    if (pExpr->Name)
    {
        VisitExpression(pExpr->Name);
    }
}

void AutoParseTreeVisitor::VisitXmlCommentExpression
(
    ParseTree::XmlExpression * pExpr
)
{
    VisitXmlExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitXmlEmbeddedExpression
(
    ParseTree::XmlEmbeddedExpression * pExpr
)
{
    VisitUnaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitFromExpressionBase
(
    ParseTree::FromExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);
    
    if (pExpr->FromItems)
    {
        VisitFromList(pExpr->FromItems);
    }
}

void AutoParseTreeVisitor::VisitFromExpression
(
    ParseTree::FromExpression * pExpr
)
{
    VisitFromExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitLetExpression
(
    ParseTree::FromExpression * pExpr
)
{
    VisitFromExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitLinqOperatorExpressionBase
(
    ParseTree::LinqOperatorExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Source)
    {
        VisitExpression(pExpr->Source);
    }
}

void AutoParseTreeVisitor::VisitAggregateExpression
(
    ParseTree::AggregateExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->AggSource)
    {
        VisitExpression(pExpr->AggSource);
    }

    if (pExpr->Projection)
    {
        VisitInitializerList(pExpr->Projection);
    }
    
}

void AutoParseTreeVisitor::VisitQueryAggregateGroupExpression
(
    ParseTree::QueryAggregateGroupExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Group)
    {
        VisitExpression(pExpr->Group);
    }
}

void AutoParseTreeVisitor::VisitCrossJoinExpression
(
    ParseTree::CrossJoinExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->JoinTo)
    {
        VisitExpression(pExpr->JoinTo);
    }
}

void AutoParseTreeVisitor::VisitFilterExpressionBase
(
    ParseTree::FilterExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->Predicate)
    {
        VisitExpression(pExpr->Predicate);
    }
}

void AutoParseTreeVisitor::VisitWhereExpression
(
    ParseTree::WhereExpression * pExpr
)
{
    VisitFilterExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitSelectExpression
(
    ParseTree::SelectExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->Projection)
    {
        VisitInitializerList(pExpr->Projection);
    }
}

void AutoParseTreeVisitor::VisitGroupRefExpression
(
    ParseTree::Expression *pExpr
)
{
    ThrowIfNull(pExpr);
    VisitExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGroupByExpression
(
    ParseTree::GroupByExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->Element)
    {
        VisitInitializerList(pExpr->Element);
    }

    if  (pExpr->Key)
    {
        VisitInitializerList(pExpr->Key);
    }

    if (pExpr->Projection)
    {
        VisitInitializerList(pExpr->Projection);
    }
}

void AutoParseTreeVisitor::VisitAggregationExpression
(
    ParseTree::AggregationExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);


    if (pExpr->Argument)
    {
        VisitExpression(pExpr->Argument);
    }
}

void AutoParseTreeVisitor::VisitQueryOperatorCallExpression
(
    ParseTree::QueryOperatorCallExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->operatorCall)
    {
        VisitExpression(pExpr->operatorCall);
    }
}

void AutoParseTreeVisitor::VisitDistinctExpression
(
    ParseTree::DistinctExpression * pExpr
)
{
    VisitLinqOperatorExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitOrderByExpression
(
    ParseTree::OrderByExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->OrderByItems)
    {
        VisitOrderByList(pExpr->OrderByItems);
    }
}

void AutoParseTreeVisitor::VisitLinqSourceExpression
(
    ParseTree::LinqSourceExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->ControlVariableDeclaration)
    {
        
        VisitVariableDeclaration(pExpr->ControlVariableDeclaration);
    }

    if (pExpr->Source)
    {
        VisitExpression(pExpr->Source);
    }
}

void AutoParseTreeVisitor::VisitInnerJoinExpressionBase
(
    ParseTree::InnerJoinExpression * pExpr
)
{
    ThrowIfNull(pExpr);
    
    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->JoinTo)
    {
        VisitExpression(pExpr->JoinTo);
    }

    if (pExpr->Predicate)
    {
        VisitExpression(pExpr->Predicate);
    }    
}

void AutoParseTreeVisitor::VisitInnerJoinExpression
(
    ParseTree::InnerJoinExpression * pExpr
)
{
    VisitInnerJoinExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitGroupJoinExpression
(
    ParseTree::GroupJoinExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitInnerJoinExpressionBase(pExpr);

    if (pExpr->Projection)
    {
        VisitInitializerList(pExpr->Projection);
    }
}

void AutoParseTreeVisitor::VisitEqualsExpression
(
    ParseTree::BinaryExpression * pExpr
)
{
    VisitBinaryExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitTakeWhileExpression
(
    ParseTree::WhileExpression * pExpr
)
{
    VisitFilterExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitSkipWhileExpression
(
    ParseTree::WhileExpression * pExpr
)
{
    VisitFilterExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitSkipTakeExpressionBase
(
    ParseTree::SkipTakeExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitLinqOperatorExpressionBase(pExpr);

    if (pExpr->Count)
    {
        VisitExpression(pExpr->Count);
    }
}

void AutoParseTreeVisitor::VisitTakeExpression
(
    ParseTree::SkipTakeExpression * pExpr
)
{
    VisitSkipTakeExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitSkipExpression
(
    ParseTree::SkipTakeExpression * pExpr
)
{
    VisitSkipTakeExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitImplicitConversionExpression
(
    ParseTree::ImplicitConversionExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Value)
    {
        VisitExpression(pExpr->Value);
    }
}

void AutoParseTreeVisitor::VisitIsTypeExpression
(
    ParseTree::TypeValueExpression * pExpr
)
{
    VisitTypeValueExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitTypeReferenceExpression
(
    ParseTree::TypeReferenceExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->ReferencedType)
    {
        VisitType(pExpr->ReferencedType);
    }
}

void AutoParseTreeVisitor::VisitNewExpression
(
    ParseTree::NewExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->InstanceType)
    {
        VisitType(pExpr->InstanceType);
    }

     VisitParenthesizedArgumentList(&pExpr->Arguments);
}

void AutoParseTreeVisitor::VisitArrayInitializerExpressionBase
(
    ParseTree::ArrayInitializerExpression *pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Elements)
    {
        VisitBracedInitializerList(pExpr->Elements);
    }
}

void AutoParseTreeVisitor::VisitArrayInitializerExpression
(
    ParseTree::ArrayInitializerExpression * pExpr
)
{
    VisitArrayInitializerExpressionBase(pExpr);
}

void AutoParseTreeVisitor::VisitNewArrayInitializerExpression
(
    ParseTree::NewArrayInitializerExpression * pExpr
)
{
    ThrowIfNull(pExpr);
    
    VisitArrayInitializerExpressionBase(pExpr);

    if (pExpr->ArrayType)
    {
        VisitType(pExpr->ArrayType);
    }
}

void AutoParseTreeVisitor::VisitObjectInitializerExpressionBase
(
    ParseTree::ObjectInitializerExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->InitialValues)
    {
        VisitBracedInitializerList(pExpr->InitialValues);
    }
}

void AutoParseTreeVisitor::VisitNewObjectInitializerExpression
(
    ParseTree::NewObjectInitializerExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitObjectInitializerExpressionBase(pExpr);

    if (pExpr->NewExpression)
    {
        VisitExpression(pExpr->NewExpression);
    }
}

void AutoParseTreeVisitor::VisitGetTypeExpression
(
    ParseTree::GetTypeExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->TypeToGet)
    {
        VisitType(pExpr->TypeToGet);
    }
        
}

void AutoParseTreeVisitor::VisitLambdaExpression
(
    ParseTree::LambdaExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Parameters)
    {
        VisitParameterList(pExpr->Parameters);
    }

    if (!pExpr->IsStatementLambda  && pExpr->GetSingleLineLambdaExpression())
    {
        VisitExpression(pExpr->GetSingleLineLambdaExpression());
    }
    else if (pExpr->IsStatementLambda && pExpr->GetStatementLambdaBody())
    {
        VisitStatement(pExpr->GetStatementLambdaBody());
    }
}

void AutoParseTreeVisitor::VisitIIfExpression
(
    ParseTree::IIfExpression* pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    VisitParenthesizedArgumentList(&pExpr->Arguments);
}

void AutoParseTreeVisitor::VisitCollectionInitializerExpression
(
    ParseTree::CollectionInitializerExpression * pExpr
)
{
    ThrowIfNull(pExpr);
    
    VisitExpressionBase(pExpr);

    if (pExpr->Initializer)
    {
        VisitBracedInitializerList(pExpr->Initializer);
    }

    if (pExpr->NewExpression)
    {
        VisitNewExpression(pExpr->NewExpression);
    }
}

void AutoParseTreeVisitor::VisitAlreadyBoundExpression
(
    ParseTree::AlreadyBoundExpression * pExpr
)
{
    VisitExpressionBase(pExpr);
}


void AutoParseTreeVisitor::VisitDeferredExpression
(
    ParseTree::DeferredExpression * pExpr
)
{
    ThrowIfNull(pExpr);

    VisitExpressionBase(pExpr);

    if (pExpr->Value)
    {
        VisitExpression(pExpr->Value);
    }
}

void AutoParseTreeVisitor::VisitSimpleNameBase
(
    ParseTree::SimpleName *pName
)
{
    VisitNameBase(pName);
}

void AutoParseTreeVisitor::VisitSimpleName
(
    ParseTree::SimpleName * pName
)
{
    VisitSimpleNameBase(pName);
}

void AutoParseTreeVisitor::VisitSimpleWithArgumentsName
(
    ParseTree::SimpleWithArgumentsName * pName
)
{
    ThrowIfNull(pName);
    
    VisitSimpleNameBase(pName);
    VisitGenericArguments(&pName->Arguments);
}

void AutoParseTreeVisitor::VisitQualifiedNameBase
(
    ParseTree::QualifiedName *pName
)
{
    ThrowIfNull(pName);

    VisitNameBase(pName);

    if (pName->Base)
    {
        VisitName(pName->Base);
    }
}

void AutoParseTreeVisitor::VisitQualifiedName
(
    ParseTree::QualifiedName * pName
)
{
    VisitQualifiedNameBase(pName);
}

void AutoParseTreeVisitor::VisitQualifiedWithArgumentsName
(
    ParseTree::QualifiedWithArgumentsName * pName
)
{
    ThrowIfNull(pName);

    VisitQualifiedNameBase(pName);

    VisitGenericArguments(&pName->Arguments);
}

void AutoParseTreeVisitor::VisitGlobalNameSpaceName
(
    ParseTree::Name *pName
)
{
    VisitNameBase(pName);
}

void AutoParseTreeVisitor::VisitNamedType
(
    ParseTree::NamedType * pType
)
{
    ThrowIfNull(pType);

    VisitTypeBase(pType);

    if (pType->TypeName)
    {
        VisitName(pType->TypeName);
    }
}

void AutoParseTreeVisitor::VisitArrayTypeBase
(
    ParseTree::ArrayType * pType
)
{
    ThrowIfNull(pType);

    VisitTypeBase(pType);
    
    if (pType->ElementType)
    {
        VisitType(pType->ElementType);
    }
}

void AutoParseTreeVisitor::VisitArrayWithoutSizesType
(
    ParseTree::ArrayType * pType
)
{
    VisitArrayTypeBase(pType);
}

void AutoParseTreeVisitor::VisitArrayWithSizesType
(
    ParseTree::ArrayWithSizesType * pType
)
{
    ThrowIfNull(pType);

    VisitArrayTypeBase(pType);

    if (pType->Dims)
    {
        VisitArrayDimList(pType->Dims);
    }
}

void AutoParseTreeVisitor::VisitObjectType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitStringType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitCharType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitDateType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitDoubleType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitSingleType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitDecimalType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitUnsignedLongType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitLongType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitUnsignedIntegerType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitIntegerType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitUnsignedShortType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitShortType
(
    ParseTree::Type *pType
)
{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitByteType
(
    ParseTree::Type *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitSignedByteType
(
    ParseTree::Type *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitBooleanType
(
    ParseTree::Type *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitSyntaxErrorType
(
    ParseTree::Type *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitAlreadyBoundType
(
    ParseTree::AlreadyBoundType *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitAlreadyBoundDelayCalculatedType
(
    ParseTree::AlreadyBoundDelayCalculatedType *pType
)

{
    VisitTypeBase(pType);
}

void AutoParseTreeVisitor::VisitNullableType
(
    ParseTree::NullableType * pType
)
{
    ThrowIfNull(pType);

    VisitTypeBase(pType);

    if (pType->ElementType)
    {
        VisitType(pType->ElementType);
    }
}

void AutoParseTreeVisitor::VisitAttributeList
(
    ParseTree::AttributeList * pAttributeList
)
{
    VisitList(pAttributeList, &AutoParseTreeVisitor::VisitAttribute);
}

void AutoParseTreeVisitor::VisitSpecifierList
(
    ParseTree::SpecifierList * pSpecifierList
)
{
    VisitList(pSpecifierList, &AutoParseTreeVisitor::VisitSpecifier);
}

void AutoParseTreeVisitor::VisitAttributeSpecifierList
(
    ParseTree::AttributeSpecifierList * pAttributeSpecifierList
)
{
    VisitList(pAttributeSpecifierList, &AutoParseTreeVisitor::VisitAttributeSpecifier);
}

void AutoParseTreeVisitor::VisitConstraintList
(
    ParseTree::ConstraintList * pConstraintList
)
{
    VisitList(pConstraintList, &AutoParseTreeVisitor::VisitConstraint);
}

void AutoParseTreeVisitor::VisitGenericParameterList
(
    ParseTree::GenericParameterList * pGenericParameterList
)
{
    VisitList(pGenericParameterList, &AutoParseTreeVisitor::VisitGenericParameter);
}

void AutoParseTreeVisitor::VisitParameterSpecifierList
(
    ParseTree::ParameterSpecifierList * pParameterSpecifierList
)
{
    VisitList(pParameterSpecifierList, &AutoParseTreeVisitor::VisitParameterSpecifier);
}

void AutoParseTreeVisitor::VisitParameterList
(
    ParseTree::ParameterList * pParameterList
)
{
    VisitList(pParameterList, &AutoParseTreeVisitor::VisitParameter);
}

void AutoParseTreeVisitor::VisitDeclaratorList
(
    ParseTree::DeclaratorList * pDeclaratorList
)
{
    VisitList(pDeclaratorList, &AutoParseTreeVisitor::VisitDeclarator);
    
}

void AutoParseTreeVisitor::VisitVariableDeclarationList
(
    ParseTree::VariableDeclarationList * pVariableDeclarationList
)
{
    VisitList(pVariableDeclarationList, &AutoParseTreeVisitor::VisitVariableDeclaration);
}

void AutoParseTreeVisitor::VisitImportDirectiveList
(
    ParseTree::ImportDirectiveList * pImportDirectiveList
)
{
    VisitList(pImportDirectiveList, &AutoParseTreeVisitor::VisitImportDirective);
}

void AutoParseTreeVisitor::VisitCaseList
(
    ParseTree::CaseList * pCaseList
)
{
    VisitList(pCaseList, &AutoParseTreeVisitor::VisitCase);
}

void AutoParseTreeVisitor::VisitCommentList
(
    ParseTree::CommentList * pCommentList
)
{
    VisitList(pCommentList, &AutoParseTreeVisitor::VisitComment);
}

void AutoParseTreeVisitor::VisitNameList
(
    ParseTree::NameList * pNameList
)
{
    VisitList(pNameList, &AutoParseTreeVisitor::VisitName);
}

void AutoParseTreeVisitor::VisitTypeList
(
    ParseTree::TypeList * pTypeList
)
{
    VisitList(pTypeList, &AutoParseTreeVisitor::VisitType);
}

void AutoParseTreeVisitor::VisitArgumentList
(
    ParseTree::ArgumentList * pArgumentList
)
{
    VisitList(pArgumentList, &AutoParseTreeVisitor::VisitArgument);
}

void AutoParseTreeVisitor::VisitExpressionList
(
    ParseTree::ExpressionList * pExpressionList
)
{
    VisitList(pExpressionList, &AutoParseTreeVisitor::VisitExpression);
}

void AutoParseTreeVisitor::VisitArrayDimList
(
    ParseTree::ArrayDimList * pArrayDimList
)
{
    VisitList(pArrayDimList, &AutoParseTreeVisitor::VisitArrayDim);
}

void AutoParseTreeVisitor::VisitFromList
(
    ParseTree::FromList * pFromList
)
{
    VisitList(pFromList, &AutoParseTreeVisitor::VisitFromItem);
}

void AutoParseTreeVisitor::VisitOrderByList
(
    ParseTree::OrderByList * pOrderByList
)
{
    VisitList(pOrderByList, &AutoParseTreeVisitor::VisitOrderByItem);
}

void AutoParseTreeVisitor::VisitInitializerList
(
    ParseTree::InitializerList * pInitializerList
)
{
    VisitList(pInitializerList, &AutoParseTreeVisitor::VisitInitializer);
}

void AutoParseTreeVisitor::VisitStatementList
(
    ParseTree::StatementList * pStatementList
)
{
    while (pStatementList)
    {
        VisitStatement(pStatementList->Element);

        pStatementList = pStatementList->NextInBlock;
    }
}


void AutoParseTreeVisitor::VisitParenthesizedArgumentList
(
    ParseTree::ParenthesizedArgumentList * pArgumentList
)
{
    ThrowIfNull(pArgumentList);
    
    VisitParseTreeNodeBase(pArgumentList);

    if (pArgumentList->Values)
    {
        VisitArgumentList(pArgumentList->Values);
    }
}

void AutoParseTreeVisitor::VisitAttribute
(
    ParseTree::Attribute * pAttribute
)
{
    ThrowIfNull(pAttribute);

    VisitParseTreeNodeBase(pAttribute);

    if (pAttribute->Name)
    {
        VisitName(pAttribute->Name);
    }

    VisitParenthesizedArgumentList(&pAttribute->Arguments);

    if (pAttribute->DeferredRepresentationAsCall)
    {
        VisitExpression(pAttribute->DeferredRepresentationAsCall);
    }
}

void AutoParseTreeVisitor::VisitArgument
(
    ParseTree::Argument * pArgument
)
{
    ThrowIfNull(pArgument);

    VisitParseTreeNodeBase(pArgument);

    if (pArgument->Value)
    {
        VisitExpression(pArgument->Value);
    }

    if (pArgument->lowerBound)
    {
        VisitExpression(pArgument->lowerBound);
    }
}

void AutoParseTreeVisitor::VisitBracedInitializerList
(
    ParseTree::BracedInitializerList * pInitializerList
)
{
    ThrowIfNull(pInitializerList);

    VisitParseTreeNodeBase(pInitializerList);

    if (pInitializerList->InitialValues)
    {
        VisitInitializerList(pInitializerList->InitialValues);
    }
}


void AutoParseTreeVisitor::VisitExternalSourceDirective
(
    ParseTree::ExternalSourceDirective * pExternalSourceDirective
)
{
    while (pExternalSourceDirective)
    {
        pExternalSourceDirective = pExternalSourceDirective->Next;
    }
}

void AutoParseTreeVisitor::VisitVariableDeclarationBase
(
    ParseTree::VariableDeclaration * pVarDecl
)
{
    ThrowIfNull(pVarDecl);

    VisitParseTreeNodeBase(pVarDecl);

    if (pVarDecl->Type)
    {
        VisitType(pVarDecl->Type);
    }

    if (pVarDecl->Variables)
    {
        VisitDeclaratorList( pVarDecl->Variables);
    }
}

void AutoParseTreeVisitor::VisitNoInitializerVariableDeclaration
(
    ParseTree::VariableDeclaration * pVarDecl
)
{
    VisitVariableDeclarationBase(pVarDecl);
}

void AutoParseTreeVisitor::VisitWithInitializerVariableDeclaration
(
    ParseTree::InitializerVariableDeclaration * pVarDecl
)
{
    ThrowIfNull(pVarDecl);

    VisitVariableDeclarationBase(pVarDecl);

    if (pVarDecl->InitialValue)
    {
        VisitInitializer(pVarDecl->InitialValue);
    }    
}

void AutoParseTreeVisitor::VisitWithNewVariableDeclaration
(
    ParseTree::NewVariableDeclaration * pVarDecl
)
{
    ThrowIfNull(pVarDecl);

    VisitVariableDeclarationBase(pVarDecl);
    VisitParenthesizedArgumentList(&pVarDecl->Arguments);

    if (pVarDecl->ObjectInitializer)
    {
        VisitObjectInitializerList(pVarDecl->ObjectInitializer);
    }

    if (pVarDecl->CollectionInitializer)
    {
        VisitBracedInitializerList( pVarDecl->CollectionInitializer);
    }

    //ILC:undone
    //      Figure out if we need to do anything with the "DeferredInitializerText" field.
}

void AutoParseTreeVisitor::VisitGenericArguments
(
    ParseTree::GenericArguments * pGenericArguments
)
{
    ThrowIfNull(pGenericArguments);


    if (pGenericArguments->Arguments)
    {
        VisitTypeList(pGenericArguments->Arguments);
    }

}

void AutoParseTreeVisitor::VisitAttributeSpecifier
(
    ParseTree::AttributeSpecifier * pAttributeSpecifier
)
{
    ThrowIfNull(pAttributeSpecifier);


    if (pAttributeSpecifier->Values)
    {
        VisitAttributeList(pAttributeSpecifier->Values);
    }
}

void AutoParseTreeVisitor::VisitGenericParameter
(
    ParseTree::GenericParameter * pParam
)
{
    ThrowIfNull(pParam);


    if (pParam->Constraints)
    {
        VisitConstraintList(pParam->Constraints);
    }
        
}

void AutoParseTreeVisitor::VisitParameter
(
    ParseTree::Parameter * pParam
)
{
    ThrowIfNull(pParam);

    if (pParam->Name)
    {
        VisitDeclarator(pParam->Name);
    }

    if (pParam->Type)
    {
        VisitType(pParam->Type);
    }

    if (pParam->Specifiers)
    {
        VisitParameterSpecifierList( pParam->Specifiers);
    }

    if (pParam->Attributes)
    {
        VisitAttributeSpecifierList(pParam->Attributes);
    }

    if (pParam->Specifiers && pParam->Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))
    {
        ParseTree::OptionalParameter * pAsOptional = pParam->AsOptional();

        if (pAsOptional->DefaultValue)
        {
            VisitExpression(pAsOptional->DefaultValue);
        }
    }
}

void AutoParseTreeVisitor::VisitDeclarator
(
    ParseTree::Declarator * pDeclarator
)
{
    ThrowIfNull(pDeclarator);

    VisitParseTreeNodeBase(pDeclarator);

    if (pDeclarator->ArrayInfo)
    {
        VisitType(pDeclarator->ArrayInfo);
    }
}

void AutoParseTreeVisitor::VisitComment
(
    ParseTree::Comment * pComment
)
{
    //do nothing
}

void AutoParseTreeVisitor::VisitArrayDim
(
    ParseTree::ArrayDim * pArrayDim
)
{
    ThrowIfNull(pArrayDim);

    VisitParseTreeNodeBase( pArrayDim);

    if (pArrayDim->lowerBound)
    {
        VisitExpression(pArrayDim->lowerBound);
    }
}

void AutoParseTreeVisitor::VisitFromItem
(
    ParseTree::FromItem * pFromItem
)
{
    ThrowIfNull(pFromItem);
    
    if (pFromItem->ControlVariableDeclaration)
    {
        VisitVariableDeclaration(pFromItem->ControlVariableDeclaration);
    }

    if (pFromItem->Source)
    {
        VisitExpression(pFromItem->Source);
    }
}

void AutoParseTreeVisitor::VisitOrderByItem
(
    ParseTree::OrderByItem * pOrderByItem
)
{
    ThrowIfNull(pOrderByItem);

    if (pOrderByItem->OrderExpression)
    {
        VisitExpression(pOrderByItem->OrderExpression);
    }
}

void AutoParseTreeVisitor::VisitObjectInitializerList
(
    ParseTree::ObjectInitializerList * pInitList
)
{
    ThrowIfNull(pInitList);

    if (pInitList->BracedInitializerList)
    {
        VisitBracedInitializerList(pInitList->BracedInitializerList);
    }
}

void AutoParseTreeVisitor::VisitNewConstraint
(
    ParseTree::Constraint *pConstraint
)
{
    VisitConstraintBase(pConstraint);
}

void AutoParseTreeVisitor::VisitClassConstraint
(
    ParseTree::Constraint *pConstraint
)
{
    VisitConstraintBase(pConstraint);
}

void AutoParseTreeVisitor::VisitStructConstraint
(
    ParseTree::Constraint *pConstraint
)
{
    VisitConstraintBase(pConstraint);
}

void AutoParseTreeVisitor::VisitTypeConstraint
(
    ParseTree::TypeConstraint * pConstraint
)
{
    ThrowIfNull(pConstraint);

    VisitConstraintBase(pConstraint);

    if (pConstraint->Type)
    {
        VisitType(pConstraint->Type);
    }
}

void AutoParseTreeVisitor::VisitNamespaceImportDirectiveBase
(
    ParseTree::NamespaceImportDirective * pDirective
)
{
    ThrowIfNull(pDirective);

    VisitImportDirectiveBase( pDirective);
    
    if (pDirective->ImportedName)
    {
        VisitName(pDirective->ImportedName);
    }
}

void AutoParseTreeVisitor::VisitNamespaceImportDirective
(
    ParseTree::NamespaceImportDirective * pDirective
)
{
    VisitNamespaceImportDirectiveBase(pDirective);
}

void AutoParseTreeVisitor::VisitAliasImportDirective
(
    ParseTree::AliasImportDirective * pDirective
)
{
    VisitNamespaceImportDirectiveBase(pDirective);
}

void AutoParseTreeVisitor::VisitXmlNamespaceImportDirective
(
    ParseTree::XmlNamespaceImportDirective * pDirective
)
{
    ThrowIfNull(pDirective);
    
    VisitImportDirectiveBase( pDirective);

    if (pDirective->NamespaceDeclaration)
    {
        VisitExpression(pDirective->NamespaceDeclaration);
    }    
}

void AutoParseTreeVisitor::VisitSyntaxErrorCase
(
    ParseTree::Case * pCase
)
{
    VisitCaseBase(pCase);
}

void AutoParseTreeVisitor::VisitRelationalCase
(
    ParseTree::RelationalCase * pCase
)
{
    ThrowIfNull(pCase);
    
    VisitCaseBase(pCase);
    
    if (pCase->Value)
    {
        VisitExpression(pCase->Value);
    }
        
}

void AutoParseTreeVisitor::VisitValueCase
(
    ParseTree::ValueCase * pCase
)
{
    ThrowIfNull(pCase);
    
    VisitCaseBase(pCase);
    
    if (pCase->Value)
    {
        VisitExpression(pCase->Value);
    }
    
}

void AutoParseTreeVisitor::VisitRangeCase
(
    ParseTree::RangeCase * pCase
)
{
    ThrowIfNull(pCase);
    
    VisitCaseBase(pCase);
    
    if (pCase->High)
    {
        VisitExpression(pCase->High);
    }

    if (pCase->High)
    {
        VisitExpression(pCase->High);
    }
    
}


void AutoParseTreeVisitor::VisitInitializerBase
(
    ParseTree::Initializer * pInitializer
)
{
    VisitParseTreeNodeBase(pInitializer);
}

void AutoParseTreeVisitor::VisitExpressionInitializer
(
    ParseTree::ExpressionInitializer * pInitializer
)
{
    ThrowIfNull(pInitializer);

    VisitInitializerBase(pInitializer);

    if (pInitializer->Value)
    {
        VisitExpression(pInitializer->Value);
    }
}

void AutoParseTreeVisitor::VisitDeferredInitializer
(
    ParseTree::DeferredInitializer * pInitializer
)
{
    ThrowIfNull(pInitializer);

    VisitInitializerBase(pInitializer);

    if (pInitializer->Value)
    {
        VisitInitializer(pInitializer->Value);
    }    
}

void AutoParseTreeVisitor::VisitAssignmentInitializer
(
    ParseTree::AssignmentInitializer * pInitializer
)
{
    ThrowIfNull(pInitializer);

    VisitInitializerBase(pInitializer);

    if (pInitializer->Initializer)
    {
        VisitInitializer(pInitializer->Initializer);
    }
}

void AutoParseTreeVisitor::VisitAutoPropertyInitBase
(
    ParseTree::AutoPropertyInitialization * pInit
)
{
    VisitParseTreeNodeBase(pInit);
}

void AutoParseTreeVisitor::VisitWithInitializerAutoPropInit
(
    ParseTree::InitializerAutoPropertyDeclaration * pInit
)
{
    ThrowIfNull(pInit);
    VisitAutoPropertyInitBase(pInit);

    if (pInit->InitialValue)
    {
        VisitInitializer(pInit->InitialValue);
    }
}

void AutoParseTreeVisitor::VisitWithNewAutoPropInit 
(
    ParseTree::NewAutoPropertyDeclaration * pInit
)
{
    ThrowIfNull(pInit);
    VisitAutoPropertyInitBase(pInit);

    VisitParenthesizedArgumentList( &pInit->Arguments);

    if (pInit->ObjectInitializer)
    {
        VisitObjectInitializerList( pInit->ObjectInitializer);
    }

    if (pInit->CollectionInitializer)
    {
        VisitBracedInitializerList( pInit->CollectionInitializer);
    }
    
}

void AutoParseTreeVisitor::VisitSpecifier
(
    ParseTree::Specifier * pSpecifier
)
{
    //do nothing
}

void AutoParseTreeVisitor::VisitParameterSpecifier
(
    ParseTree::ParameterSpecifier * pSpecifier
)
{
    //do nothing
}

