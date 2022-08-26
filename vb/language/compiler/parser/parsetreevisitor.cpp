//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of ParseTreeVisitor
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void ParseTreeVisitor::Visit(_In_opt_ ParseTree::Statement * pStatement)
{
    if (!pStatement)
    {
        return;
    }

    switch (pStatement->Opcode)
    {
        case ParseTree::Statement::CCConst:
            Visit(pStatement->AsNamedValue()->Value);
            break;

        case ParseTree::Statement::CCIf:
        case ParseTree::Statement::CCElseIf:
            Visit(pStatement->AsExpression()->Operand);
            break;

        case ParseTree::Statement::Region:
            Visit(pStatement->AsRegion()->Title);
            break;

        case ParseTree::Statement::EnumeratorWithValue:
            Visit(pStatement->AsEnumeratorWithValue()->Value);
            break;

        case ParseTree::Statement::VariableDeclaration:
            VisitVariableDeclarationList(pStatement->AsVariableDeclaration()->Declarations);
            break;

        case ParseTree::Statement::BlockIf:
        case ParseTree::Statement::SyncLock:
        case ParseTree::Statement::LineIf:
        case ParseTree::Statement::ElseIf:
        case ParseTree::Statement::Select:
        case ParseTree::Statement::While:
        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
        case ParseTree::Statement::With:
            Visit(pStatement->AsExpressionBlock()->Operand);
            break;

        case ParseTree::Statement::EndLoopUntil:
        case ParseTree::Statement::EndLoopWhile:
        case ParseTree::Statement::Return:
        case ParseTree::Statement::Yield:
        case ParseTree::Statement::Await:
        case ParseTree::Statement::Error:
        case ParseTree::Statement::Throw:
            Visit(pStatement->AsExpression()->Operand);
            break;

        case ParseTree::Statement::Case:
            VisitCaseList(pStatement->AsCase()->Cases);
            break;

        case ParseTree::Statement::Catch:
            Visit(pStatement->AsCatch()->WhenClause);
            break;

        case ParseTree::Statement::ForFromTo:
            Visit(pStatement->AsForFromTo()->InitialValue);
            Visit(pStatement->AsForFromTo()->FinalValue);
            Visit(pStatement->AsForFromTo()->IncrementValue);
            Visit(pStatement->AsFor()->ControlVariableDeclaration);
            Visit(pStatement->AsFor()->ControlVariable);
            break;

        case ParseTree::Statement::ForEachIn:
            Visit(pStatement->AsForEachIn()->Collection);
            Visit(pStatement->AsFor()->ControlVariableDeclaration);
            Visit(pStatement->AsFor()->ControlVariable);
            break;

        case ParseTree::Statement::Using:
            Visit(pStatement->AsUsing()->ControlExpression);
            Visit(pStatement->AsUsing()->ControlVariableDeclaration);
            break;

        case ParseTree::Statement::EndNext:
            VisitExpressionList(pStatement->AsEndNext()->Variables);
            break;

        case ParseTree::Statement::Call:
            Visit(pStatement->AsCall()->Target);
            VisitParenthesizedArgumentList(&pStatement->AsCall()->Arguments);
            break;

        case ParseTree::Statement::RaiseEvent:
            VisitParenthesizedArgumentList(&pStatement->AsRaiseEvent()->Arguments);
            break;

        case ParseTree::Statement::Assign:
        case ParseTree::Statement::AssignPlus:
        case ParseTree::Statement::AssignMinus:
        case ParseTree::Statement::AssignMultiply:
        case ParseTree::Statement::AssignDivide:
        case ParseTree::Statement::AssignPower:
        case ParseTree::Statement::AssignIntegralDivide:
        case ParseTree::Statement::AssignConcatenate:
        case ParseTree::Statement::AssignShiftLeft:
        case ParseTree::Statement::AssignShiftRight:
            Visit(pStatement->AsAssignment()->Source);
            Visit(pStatement->AsAssignment()->Target);
            break;

        case ParseTree::Statement::AssignMid:
            Visit(pStatement->AsAssignMid()->Source);
            Visit(pStatement->AsAssignMid()->Start);
            Visit(pStatement->AsAssignMid()->Length);
            Visit(pStatement->AsAssignMid()->Target);
            break;

        case ParseTree::Statement::Erase:
            VisitExpressionList(pStatement->AsErase()->Arrays);
            break;

        case ParseTree::Statement::Redim:
            VisitExpressionList(pStatement->AsRedim()->Redims);
            break;

        case ParseTree::Statement::AddHandler:
            Visit(pStatement->AsHandler()->Event);
            Visit(pStatement->AsHandler()->Delegate);
            break;

        case ParseTree::Statement::RemoveHandler:
            Visit(pStatement->AsHandler()->Event);
            Visit(pStatement->AsHandler()->Delegate);
            break;

        case ParseTree::Statement::SyntaxError:
        case ParseTree::Statement::File:
        case ParseTree::Statement::EndInvalid:
        case ParseTree::Statement::EndUnknown:
        case ParseTree::Statement::CCElse:
        case ParseTree::Statement::CCEndIf:
        case ParseTree::Statement::EndRegion:
        case ParseTree::Statement::Enum:
        case ParseTree::Statement::Class:
        case ParseTree::Statement::Module:
        case ParseTree::Statement::Structure:
        case ParseTree::Statement::Interface:
        case ParseTree::Statement::Namespace:
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::FunctionDeclaration:
        case ParseTree::Statement::OperatorDeclaration:
        case ParseTree::Statement::AddHandlerDeclaration:
        case ParseTree::Statement::RemoveHandlerDeclaration:
        case ParseTree::Statement::RaiseEventDeclaration:
        case ParseTree::Statement::PropertyGet:
        case ParseTree::Statement::PropertySet:
        case ParseTree::Statement::DelegateProcedureDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
        case ParseTree::Statement::DelegateFunctionDeclaration:
        case ParseTree::Statement::EventDeclaration:
        case ParseTree::Statement::BlockEventDeclaration:
        case ParseTree::Statement::Resume:
        case ParseTree::Statement::ForeignProcedureDeclaration:
        case ParseTree::Statement::ForeignFunctionDeclaration:
        case ParseTree::Statement::Property:
        case ParseTree::Statement::Enumerator:
        case ParseTree::Statement::Implements:
        case ParseTree::Statement::Inherits:
        case ParseTree::Statement::Imports:
        case ParseTree::Statement::OptionUnknown:
        case ParseTree::Statement::OptionInvalid:
        case ParseTree::Statement::OptionCompareNone:
        case ParseTree::Statement::OptionCompareText:
        case ParseTree::Statement::OptionCompareBinary:
        case ParseTree::Statement::OptionExplicitOn:
        case ParseTree::Statement::OptionExplicitOff:
        case ParseTree::Statement::OptionStrictOn:
        case ParseTree::Statement::OptionStrictOff:
        case ParseTree::Statement::OptionInferOn:
        case ParseTree::Statement::OptionInferOff:
        case ParseTree::Statement::Attribute:
        case ParseTree::Statement::ProcedureBody:
        case ParseTree::Statement::PropertyGetBody:
        case ParseTree::Statement::PropertySetBody:
        case ParseTree::Statement::FunctionBody:
        case ParseTree::Statement::OperatorBody:
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::LineElse:
        case ParseTree::Statement::CaseElse:
        case ParseTree::Statement::Try:
        case ParseTree::Statement::Finally:
        case ParseTree::Statement::DoForever:
        case ParseTree::Statement::DoWhileBottomTest:
        case ParseTree::Statement::DoUntilBottomTest:
        case ParseTree::Statement::EndIf:
        case ParseTree::Statement::EndWith:
        case ParseTree::Statement::EndUsing:
        case ParseTree::Statement::EndSelect:
        case ParseTree::Statement::EndStructure:
        case ParseTree::Statement::EndEnum:
        case ParseTree::Statement::EndInterface:
        case ParseTree::Statement::EndClass:
        case ParseTree::Statement::EndModule:
        case ParseTree::Statement::EndNamespace:
        case ParseTree::Statement::EndSub:
        case ParseTree::Statement::EndFunction:
        case ParseTree::Statement::EndOperator:
        case ParseTree::Statement::EndGet:
        case ParseTree::Statement::EndSet:
        case ParseTree::Statement::EndProperty:
        case ParseTree::Statement::EndEvent:
        case ParseTree::Statement::EndAddHandler:
        case ParseTree::Statement::EndRemoveHandler:
        case ParseTree::Statement::EndRaiseEvent:
        case ParseTree::Statement::EndWhile:
        case ParseTree::Statement::EndLoop:
        case ParseTree::Statement::EndTry:
        case ParseTree::Statement::EndSyncLock:
        case ParseTree::Statement::Label:
        case ParseTree::Statement::Goto:
        case ParseTree::Statement::OnError:
        case ParseTree::Statement::Stop:
        case ParseTree::Statement::End:
        case ParseTree::Statement::ContinueDo:
        case ParseTree::Statement::ContinueFor:
        case ParseTree::Statement::ContinueWhile:
        case ParseTree::Statement::ExitUnknown:
        case ParseTree::Statement::ExitDo:
        case ParseTree::Statement::ExitFor:
        case ParseTree::Statement::ExitWhile:
        case ParseTree::Statement::ExitSelect:
        case ParseTree::Statement::ExitSub:
        case ParseTree::Statement::ExitFunction:
        case ParseTree::Statement::ExitOperator:
        case ParseTree::Statement::ExitProperty:
        case ParseTree::Statement::ExitTry:
        case ParseTree::Statement::CommentBlock:
        case ParseTree::Statement::EndCommentBlock:
        case ParseTree::Statement::Empty:
        case ParseTree::Statement::LambdaBody:
            break;

        default:
            VSFAIL("You must add visiting code for any new parse tree nodes to ParseTreeVisitor");
            break;
    }
}

void ParseTreeVisitor::Visit(_In_opt_ ParseTree::Expression * pExpression)
{
    if (!pExpression)
    {
        return;
    }

    switch (pExpression->Opcode)
    {
        case ParseTree::Expression::Plus:
        case ParseTree::Expression::Minus:
        case ParseTree::Expression::Multiply:
        case ParseTree::Expression::Divide:
        case ParseTree::Expression::Power:
        case ParseTree::Expression::IntegralDivide:
        case ParseTree::Expression::Concatenate:
        case ParseTree::Expression::ShiftLeft:
        case ParseTree::Expression::ShiftRight:
        case ParseTree::Expression::Modulus:
        case ParseTree::Expression::Or:
        case ParseTree::Expression::OrElse:
        case ParseTree::Expression::Xor:
        case ParseTree::Expression::And:
        case ParseTree::Expression::AndAlso:
        case ParseTree::Expression::Like:
        case ParseTree::Expression::Is:
        case ParseTree::Expression::IsNot:
        case ParseTree::Expression::Equal:
        case ParseTree::Expression::NotEqual:
        case ParseTree::Expression::Less:
        case ParseTree::Expression::LessEqual:
        case ParseTree::Expression::GreaterEqual:
        case ParseTree::Expression::Greater:
        case ParseTree::Expression::Equals:
            Visit(pExpression->AsBinary()->Left);
            Visit(pExpression->AsBinary()->Right);
            break;

        case ParseTree::Expression::Not:
        case ParseTree::Expression::Await:
        case ParseTree::Expression::AddressOf:
        case ParseTree::Expression::Negate:
        case ParseTree::Expression::UnaryPlus:
        case ParseTree::Expression::Parenthesized:
        case ParseTree::Expression::CastBoolean:
        case ParseTree::Expression::CastCharacter:
        case ParseTree::Expression::CastDate:
        case ParseTree::Expression::CastDouble:
        case ParseTree::Expression::CastSignedByte:
        case ParseTree::Expression::CastByte:
        case ParseTree::Expression::CastShort:
        case ParseTree::Expression::CastUnsignedShort:
        case ParseTree::Expression::CastInteger:
        case ParseTree::Expression::CastUnsignedInteger:
        case ParseTree::Expression::CastLong:
        case ParseTree::Expression::CastUnsignedLong:
        case ParseTree::Expression::CastDecimal:
        case ParseTree::Expression::CastSingle:
        case ParseTree::Expression::CastString:
        case ParseTree::Expression::CastObject:
        case ParseTree::Expression::XmlEmbedded:
            Visit(pExpression->AsUnary()->Operand);
            break;

        case ParseTree::Expression::Conversion:
        case ParseTree::Expression::DirectCast:
        case ParseTree::Expression::TryCast:
        case ParseTree::Expression::IsType:
            Visit(pExpression->AsTypeValue()->Value);
            break;

        case ParseTree::Expression::BangQualified:
        case ParseTree::Expression::DotQualified:
        case ParseTree::Expression::XmlElementsQualified:
        case ParseTree::Expression::XmlAttributeQualified:
        case ParseTree::Expression::XmlDescendantsQualified:
            Visit(pExpression->AsQualified()->Base);
            Visit(pExpression->AsQualified()->Name);
            break;

        case ParseTree::Expression::GenericQualified:
            Visit(pExpression->AsGenericQualified()->Base);
            break;

        case ParseTree::Expression::CallOrIndex:
            VisitParenthesizedArgumentList(&pExpression->AsCallOrIndex()->Arguments);
            Visit(pExpression->AsCallOrIndex()->Target);
            break;

        case ParseTree::Expression::IIf:
            VisitParenthesizedArgumentList(&pExpression->AsIIf()->Arguments);
            break;

        case ParseTree::Expression::New:
            VisitParenthesizedArgumentList(&pExpression->AsNew()->Arguments);
            VisitType(pExpression->AsNew()->InstanceType);
            break;

        case ParseTree::Expression::ArrayInitializer:
            VisitBracedInitializerList(pExpression->AsArrayInitializer()->Elements);
            break;

        case ParseTree::Expression::NewArrayInitializer:
            VisitType(pExpression->AsNewArrayInitializer()->ArrayType);
            VisitBracedInitializerList(pExpression->AsArrayInitializer()->Elements);
            break;

        case ParseTree::Expression::NewObjectInitializer:
            VisitBracedInitializerList(pExpression->AsNewObjectInitializer()->InitialValues);
            Visit(pExpression->AsNewObjectInitializer()->NewExpression);
            break;

        case ParseTree::Expression::Deferred:
            Visit(pExpression->AsDeferred()->Value);
            break;

        case ParseTree::Expression::CrossJoin:
            Visit(pExpression->AsCrossJoin()->Source);
            Visit(pExpression->AsCrossJoin()->JoinTo);
            break;

        case ParseTree::Expression::Let:
        case ParseTree::Expression::From: {
            ParseTree::FromList * pFromItemList = pExpression->AsFrom()->FromItems;
            while (pFromItemList)
            {
                VisitFromItem(pFromItemList->Element);
                pFromItemList = pFromItemList->Next;
            }
            break;
        }

        case ParseTree::Expression::Select:
            Visit(pExpression->AsSelect()->Source);
            VisitInitializerList(pExpression->AsSelect()->Projection);
            break;

        case ParseTree::Expression::Distinct:
            Visit(pExpression->AsDistinct()->Source);
            break;

        case ParseTree::Expression::Where:
            Visit(pExpression->AsWhere()->Source);
            Visit(pExpression->AsWhere()->Predicate);
            break;

        case ParseTree::Expression::OrderBy:
        {
            Visit(pExpression->AsOrderBy()->Source);

            ParseTree::OrderByList * pOrderByList = pExpression->AsOrderBy()->OrderByItems;
            while (pOrderByList)
            {
                Visit(pOrderByList->Element->OrderExpression);
                pOrderByList = pOrderByList->Next;
            }
            break;
        }

        case ParseTree::Expression::Aggregation:
            Visit(pExpression->AsAggregation()->Argument);
            break;

        case ParseTree::Expression::GroupBy:
            Visit(pExpression->AsGroupBy()->Source);
            VisitInitializerList(pExpression->AsGroupBy()->Element);
            VisitInitializerList(pExpression->AsGroupBy()->Key);
            VisitInitializerList(pExpression->AsGroupBy()->Projection);
            break;

        case ParseTree::Expression::Aggregate:
            Visit(pExpression->AsAggregate()->Source);
            Visit(pExpression->AsAggregate()->AggSource);
            VisitInitializerList(pExpression->AsAggregate()->Projection);
            break;

        case ParseTree::Expression::InnerJoin:
            Visit(pExpression->AsInnerJoin()->Source);
            Visit(pExpression->AsInnerJoin()->JoinTo);
            Visit(pExpression->AsInnerJoin()->Predicate);
            break;

        case ParseTree::Expression::GroupJoin:
            Visit(pExpression->AsGroupJoin()->Source);
            Visit(pExpression->AsGroupJoin()->JoinTo);
            Visit(pExpression->AsGroupJoin()->Predicate);
            VisitInitializerList(pExpression->AsGroupJoin()->Projection);
            break;

        case ParseTree::Expression::LinqSource:
            Visit(pExpression->AsLinqSource()->Source);
            VisitVariableDeclaration(pExpression->AsLinqSource()->ControlVariableDeclaration);
            break;

        case ParseTree::Expression::TakeWhile:
        case ParseTree::Expression::SkipWhile:
            Visit(pExpression->AsWhile()->Source);
            Visit(pExpression->AsWhile()->Predicate);
            break;

        case ParseTree::Expression::Take:
        case ParseTree::Expression::Skip:
            Visit(pExpression->AsSkipTake()->Source);
            Visit(pExpression->AsSkipTake()->Count);
            break;

        case ParseTree::Expression::XmlDocument:
            VisitExpressionList(pExpression->AsXmlDocument()->Attributes);
            VisitExpressionList(pExpression->AsXmlDocument()->Content);
            break;

        case ParseTree::Expression::XmlElement:
            Visit(pExpression->AsXmlElement()->Name);
            VisitExpressionList(pExpression->AsXmlElement()->Attributes);
            VisitExpressionList(pExpression->AsXmlElement()->Content);
            break;

        case ParseTree::Expression::XmlAttribute:
            Visit(pExpression->AsXmlAttribute()->Name);
            Visit(pExpression->AsXmlAttribute()->Value);
            break;

        case ParseTree::Expression::XmlAttributeValueList:
        case ParseTree::Expression::XmlCData:
        case ParseTree::Expression::XmlComment:
            VisitExpressionList(pExpression->AsXml()->Content);
            break;

        case ParseTree::Expression::XmlPI:
            Visit(pExpression->AsXmlPI()->Name);
            break;

        case ParseTree::Expression::Lambda:
            VisitParameterList(pExpression->AsLambda()->Parameters);
            if (pExpression->AsLambda()->IsStatementLambda)
            {
                Visit(pExpression->AsLambda()->GetStatementLambdaBody());
            }
            else
            {
                Visit(pExpression->AsLambda()->GetSingleLineLambdaExpression());
            }
            break;

        case ParseTree::Expression::SyntaxError:
        case ParseTree::Expression::Me:
        case ParseTree::Expression::MyBase:
        case ParseTree::Expression::MyClass:
        case ParseTree::Expression::GlobalNameSpace:
        case ParseTree::Expression::Name:
        case ParseTree::Expression::CharacterLiteral:
        case ParseTree::Expression::IntegralLiteral:
        case ParseTree::Expression::BooleanLiteral:
        case ParseTree::Expression::DecimalLiteral:
        case ParseTree::Expression::FloatingLiteral:
        case ParseTree::Expression::DateLiteral:
        case ParseTree::Expression::StringLiteral:
        case ParseTree::Expression::GetXmlNamespace:
        case ParseTree::Expression::Nothing:
        case ParseTree::Expression::XmlName:
        case ParseTree::Expression::XmlCharData:
        case ParseTree::Expression::XmlReference:
        case ParseTree::Expression::TypeReference:
        case ParseTree::Expression::GetType:
        case ParseTree::Expression::GroupRef:
            break;

        default:
            VSFAIL("You must add visiting code for any new parse tree nodes to ParseTreeVisitor");
            break;
    }
}

void ParseTreeVisitor::VisitParameter(_In_opt_ ParseTree::Parameter * pParam)
{
    if (pParam && pParam->Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))
    {
        Visit(pParam->AsOptional()->DefaultValue);
    }
}

void ParseTreeVisitor::VisitInitializer(_In_opt_ ParseTree::Initializer * pInit)
{
    if (pInit)
    {
        switch (pInit->Opcode)
        {
            case ParseTree::Initializer::Expression:
                Visit(pInit->AsExpression()->Value);
                break;

            case ParseTree::Initializer::Deferred:
                VisitInitializer(pInit->AsDeferred()->Value);
                break;

            case ParseTree::Initializer::Assignment:
                VisitInitializer(pInit->AsAssignment()->Initializer);
                break;
        }
    }
}

void ParseTreeVisitor::VisitType(_In_opt_ ParseTree::Type * pType)
{
    if (pType)
    {
        switch (pType->Opcode)
        {
            case ParseTree::Type::ArrayWithoutSizes:
                VisitType(pType->AsArray()->ElementType);
                break;

            case ParseTree::Type::ArrayWithSizes:
            {
                ParseTree::ArrayDimList * pArrayDimList = pType->AsArrayWithSizes()->Dims;
                while (pArrayDimList)
                {
                    Visit(pArrayDimList->Element->lowerBound);
                    Visit(pArrayDimList->Element->upperBound);
                    pArrayDimList = pArrayDimList->Next;
                }

                VisitType(pType->AsArray()->ElementType);
                break;
            }

            case ParseTree::Type::Nullable:
                VisitType(pType->AsNullable()->ElementType);
                break;
        }
    }
}

void ParseTreeVisitor::VisitFromItem(_In_ ParseTree::FromItem * pFromItem)
{
    Visit(pFromItem->Source);
    VisitVariableDeclaration(pFromItem->ControlVariableDeclaration);
}

void ParseTreeVisitor::VisitVariableDeclaration(_In_opt_ ParseTree::VariableDeclaration * pVarDecl)
{
    if (pVarDecl)
    {
        VisitDeclaratorList(pVarDecl->Variables);

        switch (pVarDecl->Opcode)
        {
            case ParseTree::VariableDeclaration::WithInitializer:
                VisitInitializer(pVarDecl->AsInitializer()->InitialValue);
                break;

            case ParseTree::VariableDeclaration::WithNew:
                VisitParenthesizedArgumentList(&pVarDecl->AsNew()->Arguments);
                if (pVarDecl->AsNew()->ObjectInitializer)
                {
                    VisitBracedInitializerList(pVarDecl->AsNew()->ObjectInitializer->BracedInitializerList);
                }
                break;
        }
    }
}

void ParseTreeVisitor::VisitCaseList(ParseTree::CaseList * pCaseList)
{
    while (pCaseList)
    {
        switch (pCaseList->Element->Opcode)
        {
            case ParseTree::Case::Relational:
                Visit(pCaseList->Element->AsRelational()->Value);
                break;

            case ParseTree::Case::Value:
                Visit(pCaseList->Element->AsValue()->Value);
                break;

            case ParseTree::Case::Range:
                Visit(pCaseList->Element->AsRange()->Low);
                Visit(pCaseList->Element->AsRange()->High);
                break;
        }
        pCaseList = pCaseList->Next;
    }
}

void ParseTreeVisitor::VisitExpressionList(ParseTree::ExpressionList * pExprList)
{
    while (pExprList)
    {
        Visit(pExprList->Element);
        pExprList = pExprList->Next;
    }
}

void ParseTreeVisitor::VisitArgumentList(ParseTree::ArgumentList * pArgList)
{
    while (pArgList)
    {
        Visit(pArgList->Element->Value);
        Visit(pArgList->Element->lowerBound);
        pArgList = pArgList->Next;
    }
}

void ParseTreeVisitor::VisitParenthesizedArgumentList(_In_opt_ ParseTree::ParenthesizedArgumentList * pParenArgList)
{
    if (pParenArgList)
    {
        VisitArgumentList(pParenArgList->Values);
    }
}

void ParseTreeVisitor::VisitInitializerList(ParseTree::InitializerList * pInitList)
{
    while (pInitList)
    {
        VisitInitializer(pInitList->Element);
        pInitList = pInitList->Next;
    }
}

void ParseTreeVisitor::VisitBracedInitializerList(_In_opt_ ParseTree::BracedInitializerList * pBracedInitList)
{
    if (pBracedInitList)
    {
        VisitInitializerList(pBracedInitList->InitialValues);
    }
}

void ParseTreeVisitor::VisitParameterList(ParseTree::ParameterList * pParamList)
{
    while (pParamList)
    {
        VisitParameter(pParamList->Element);
        pParamList = pParamList->Next;
    }
}

void ParseTreeVisitor::VisitDeclaratorList(ParseTree::DeclaratorList * pDeclList)
{
    while (pDeclList)
    {
        VisitType(pDeclList->Element->ArrayInfo);
        pDeclList = pDeclList->Next;
    }
}

void ParseTreeVisitor::VisitVariableDeclarationList(ParseTree::VariableDeclarationList * pVarDeclList)
{
    while (pVarDeclList)
    {
        VisitVariableDeclaration(pVarDeclList->Element);
        pVarDeclList = pVarDeclList->Next;
    }
}
