//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements helpers for processing parsetree statements.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

bool ParseTreeHelpers::GetEnumIntializerReplaceLocation(
    ParseTree::EnumeratorStatement *pEnumerator,
    Location *pLoc)
{
    AssertIfNull(pEnumerator);
    AssertIfNull(pLoc);

    pLoc->m_lBegLine = pEnumerator->Name.TextSpan.m_lEndLine;
    pLoc->m_lBegColumn = pEnumerator->Name.TextSpan.m_lEndColumn + 1;
    pLoc->m_lEndLine = pEnumerator->TextSpan.m_lEndLine;
    pLoc->m_lEndColumn = pEnumerator->TextSpan.m_lEndColumn;

    return true;
}

bool
ParseTreeHelpers::GetParameterIntializerReplaceLocation
(
    ParseTree::Parameter *pParameter,
    Location *pLoc
)
{
    AssertIfNull(pParameter);
    AssertIfNull(pLoc);

    if (pParameter->Type)
    {
        pLoc->m_lBegLine = pParameter->Type->TextSpan.m_lEndLine;
        pLoc->m_lBegColumn = pParameter->Type->TextSpan.m_lEndColumn + 1;
    }
    else
    {
        pLoc->m_lBegLine = pParameter->Name->TextSpan.m_lEndLine;
        pLoc->m_lBegColumn = pParameter->Name->TextSpan.m_lEndColumn + 1;
    }

    pLoc->m_lEndLine = pParameter->TextSpan.m_lEndLine;
    pLoc->m_lEndColumn = pParameter->TextSpan.m_lEndColumn;

    return true;
}

bool
ParseTreeHelpers::GetParameterSpecifiersReplaceLocation
(
    ParseTree::Parameter *pParameter,
    Location *pLoc
)
{
    AssertIfNull(pParameter);
    AssertIfNull(pLoc);

    if (pParameter->Specifiers)
    {
        pLoc->m_lBegLine = pParameter->Specifiers->TextSpan.m_lBegLine;
        pLoc->m_lBegColumn = pParameter->Specifiers->TextSpan.m_lBegColumn;
    }
    else if (pParameter->Attributes)
    {
        pLoc->m_lBegLine = pParameter->Attributes->TextSpan.m_lEndLine;
        pLoc->m_lBegColumn = pParameter->Attributes->TextSpan.m_lEndColumn + 1;
    }
    else
    {
        pLoc->m_lBegLine = pParameter->TextSpan.m_lBegLine;
        pLoc->m_lBegColumn = pParameter->TextSpan.m_lBegColumn;
    }

    pLoc->m_lEndLine = pParameter->Name->TextSpan.m_lBegLine;
    pLoc->m_lEndColumn = pParameter->Name->TextSpan.m_lBegColumn - 1;

    return true;
}

bool
ParseTreeHelpers::GetVariableIntializerReplaceLocation
(
    ParseTree::VariableDeclaration *pVariable,
    Location *pLoc
)
{
    AssertIfNull(pVariable);
    AssertIfNull(pLoc);

    if (pVariable->Type)
    {
        pLoc->m_lBegLine = pVariable->Type->TextSpan.m_lEndLine;
        pLoc->m_lBegColumn = pVariable->Type->TextSpan.m_lEndColumn + 1;
    }
    else
    {
        pLoc->m_lBegLine = pVariable->Variables->TextSpan.m_lEndLine;
        pLoc->m_lBegColumn = pVariable->Variables->TextSpan.m_lEndColumn + 1;
    }

    pLoc->m_lEndLine = pVariable->TextSpan.m_lEndLine;
    pLoc->m_lEndColumn = pVariable->TextSpan.m_lEndColumn;

    return true;
}

bool
ParseTreeHelpers::GetWholeStatementLocation
(
    ParseTree::Statement *pPreviousStatement, //[in] Can be null. Previous statement.
    ParseTree::Statement *pCurrentStatement, // [in] Cannot be null. Current statement.
    Location *pLoc, //[out]. Will contain the location.
    bool lookAtBlockSpanForUnterminatedBlocks
)
{
    AssertIfNull(pCurrentStatement);
    AssertIfNull(pLoc);

    TextSpan * pBodyTextSpan = NULL;

    if (pLoc &&
        pCurrentStatement &&
        !pCurrentStatement->HasSyntaxError)
    {

        ParseTree::Statement *pEndStatement = pCurrentStatement;

        switch (pCurrentStatement->Opcode)
        {
            case ParseTree::Statement::Structure:
            case ParseTree::Statement::Interface:
            case ParseTree::Statement::Class:
            case ParseTree::Statement::Module:
            case ParseTree::Statement::Enum:
            case ParseTree::Statement::Namespace:
            case ParseTree::Statement::Property:
            case ParseTree::Statement::AutoProperty:
            case ParseTree::Statement::BlockEventDeclaration:
            {
                ParseTree::BlockStatement *pBlock = pCurrentStatement->AsBlock();

                if (pBlock->HasProperTermination && pBlock->TerminatingConstruct)
                {
                    pEndStatement = pBlock->TerminatingConstruct;
                }
                else if (lookAtBlockSpanForUnterminatedBlocks)
                {
                    *pLoc = pBlock->BodyTextSpan;
                    pLoc->m_lBegLine = pCurrentStatement->TextSpan.m_lBegLine;
                    pLoc->m_lBegColumn = pCurrentStatement->TextSpan.m_lBegColumn;
                    return true;
                }

                break;
            }

            case ParseTree::Statement::CommentBlock:
            {
                *pLoc = pCurrentStatement->AsCommentBlock()->BodyTextSpan;
                return true;
            }

            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
            case ParseTree::Statement::AddHandlerDeclaration:
            case ParseTree::Statement::RemoveHandlerDeclaration:
            case ParseTree::Statement::RaiseEventDeclaration:
            {
                ParseTree::MethodDefinitionStatement *pMethodDef = pCurrentStatement->AsMethodDefinition();

                if (pMethodDef->HasProperTermination &&
                    pMethodDef->TerminatingConstruct &&
                    !pMethodDef->TerminatingConstruct->HasSyntaxError)
                {
                    pEndStatement = pMethodDef->TerminatingConstruct;
                }

                break;
            }

            case ParseTree::Statement::DelegateProcedureDeclaration:
            case ParseTree::Statement::DelegateFunctionDeclaration:
            case ParseTree::Statement::ForeignProcedureDeclaration:
            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignFunctionNone:
            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::Enumerator:
            case ParseTree::Statement::EnumeratorWithValue:
            case ParseTree::Statement::Implements:
            case ParseTree::Statement::Inherits:
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
            case ParseTree::Statement::VariableDeclaration:
                // Nothing special to do for these
                break;

            default:
                VSFAIL("Unknown Opcode");
                return false;
        }

        // Calculate the start location.
        if (pCurrentStatement->IsFirstOnLine)
        {
            // Expand until the beginning of the line.
            pLoc->m_lBegLine = pCurrentStatement->TextSpan.m_lBegLine;
            pLoc->m_lBegColumn = 0;
        }
        else
        {
            AssertIfNull(pPreviousStatement);

            ParseTree::StatementList *pPreviousList = pPreviousStatement->ContainingList;

            // Expand until the previous colon.
            pLoc->m_lBegLine = pPreviousList->Element->TextSpan.m_lBegLine + pPreviousList->Colon.Line;
            pLoc->m_lBegColumn = pPreviousList->Colon.Column + 1;
        }

        // Calculate the end location.
        ParseTree::StatementList *pEndStatementList = pEndStatement->ContainingList;

        if (HasPunctuator(pEndStatementList->Colon))
        {
            // Expand until the next colon.
            pLoc->m_lEndLine = pEndStatementList->Element->TextSpan.m_lBegLine + pEndStatementList->Colon.Line;
            pLoc->m_lEndColumn = pEndStatementList->Colon.Column;
        }
        else
        {
            // Use the normal span.
            pLoc->m_lEndLine = pEndStatementList->Element->TextSpan.m_lEndLine;
            pLoc->m_lEndColumn = pEndStatementList->Element->TextSpan.m_lEndColumn;
        }

        return true;
    }

    return false;
}

ParseTree::Statement *
ParseTreeHelpers::GetStatementContainingLocation
(
    ParseTree::BlockStatement *pCurrentBlock, // [In] Can be NULL.
    Location *pLoc, //[In] Cannot be NULL
    bool lookAtBlockSpanForUnterminatedBlocks,
    ParseTree::StatementList **ppCachedResult
)
{
    if (pCurrentBlock)
    {
        ParseTree::StatementList *pStatementList = pCurrentBlock->Children;
        ParseTree::Statement *pPreviousStatement = pCurrentBlock;

        if (ppCachedResult && *ppCachedResult && (*ppCachedResult)->Element->TextSpan.EndsBefore(pLoc))
        {
            // Dev10 86336
            // XmlDoc processing calls this function for each comment in a source file.  Since
            // XmlDoc comments are processed in lexical order, we can cache the current statement
            // for the next comment to avoid starting from the first child of pCurrentBlock.  This
            // changes the algorithm from O(m*n) to O(m+n).

            pStatementList = *ppCachedResult;
            while (pStatementList && pStatementList->Element->GetParent()->BodyTextSpan.EndsBefore(pLoc))
            {
                pStatementList = pStatementList->Element->GetParent()->ContainingList;
            }

            VSASSERT(pStatementList, "We should always have a containing StatementList unless pLoc is invalid");
            if (pStatementList)
            {
                pPreviousStatement = pStatementList->PreviousInBlock ? 
                                     pStatementList->PreviousInBlock->Element :
                                     pStatementList->Element->GetParent();
            }
        }

        while (pStatementList)
        {
            ParseTree::Statement *pCurrentStatement = pStatementList->Element;

            if (pCurrentStatement &&
                !pCurrentStatement->HasSyntaxError)
            {
                bool checkCurrentStatement = false;
                bool checkBlockStatementsChildren = false;
                switch (pCurrentStatement->Opcode)
                {
                case ParseTree::Statement::Structure:
                case ParseTree::Statement::Interface:
                case ParseTree::Statement::Class:
                case ParseTree::Statement::Module:
                case ParseTree::Statement::Enum:
                case ParseTree::Statement::Namespace:
                case ParseTree::Statement::Property:
                case ParseTree::Statement::AutoProperty:
                case ParseTree::Statement::BlockEventDeclaration:
                case ParseTree::Statement::CommentBlock:
                    checkCurrentStatement = true;
                    checkBlockStatementsChildren = true;
                    break;
                case ParseTree::Statement::ProcedureDeclaration:
                case ParseTree::Statement::FunctionDeclaration:
                case ParseTree::Statement::ConstructorDeclaration:
                case ParseTree::Statement::OperatorDeclaration:
                case ParseTree::Statement::PropertyGet:
                case ParseTree::Statement::PropertySet:
                case ParseTree::Statement::AddHandlerDeclaration:
                case ParseTree::Statement::RemoveHandlerDeclaration:
                case ParseTree::Statement::RaiseEventDeclaration:
                case ParseTree::Statement::DelegateProcedureDeclaration:
                case ParseTree::Statement::DelegateFunctionDeclaration:
                case ParseTree::Statement::ForeignProcedureDeclaration:
                case ParseTree::Statement::ForeignFunctionDeclaration:
                case ParseTree::Statement::ForeignFunctionNone:
                case ParseTree::Statement::EventDeclaration:
                case ParseTree::Statement::Enumerator:
                case ParseTree::Statement::EnumeratorWithValue:
                case ParseTree::Statement::Implements:
                case ParseTree::Statement::Inherits:
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
                case ParseTree::Statement::VariableDeclaration:
                    checkCurrentStatement = true;
                    break;
                }

                Location locTmp;
                if (checkCurrentStatement &&
                    GetWholeStatementLocation(pPreviousStatement, pCurrentStatement, &locTmp, lookAtBlockSpanForUnterminatedBlocks))
                {
                    if (locTmp.ContainsInclusive(pLoc))
                    {
                        ParseTree::Statement *pBestMatch = NULL;
                        if (checkBlockStatementsChildren)
                        {
                            pBestMatch = GetStatementContainingLocation(pCurrentStatement->AsBlock(), pLoc, lookAtBlockSpanForUnterminatedBlocks);
                        }

                        if (pBestMatch == NULL)
                        {
                            pBestMatch = pCurrentStatement;
                        }

                        if (ppCachedResult)
                        {
                            *ppCachedResult = pBestMatch->ContainingList; 
                        }
                        return pBestMatch;
                    }
                    else if (locTmp.StartsAfter(pLoc))
                    {
                        return NULL;
                    }
                }
            }
            pPreviousStatement = pStatementList->Element;
            pStatementList = pStatementList->NextInBlock;
        }
    }

    return NULL;
}

//============================================================================
// Can the given statement be cast to an assignment statement
//============================================================================
bool
ParseTreeHelpers::IsAssignment
(
    ParseTree::Statement *pStatement
)
{
    bool fCorrectType = false;
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
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
                fCorrectType = true;
                break;

            default:
                break;
        }
    }
    return fCorrectType;
}

//============================================================================
// Can the given statement be cast to a method signature
//============================================================================
bool
ParseTreeHelpers::IsMethodSignature
(
    ParseTree::Statement *pStatement
)
{
    bool fCorrectType = false;
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
            case ParseTree::Statement::AddHandlerDeclaration:
            case ParseTree::Statement::RemoveHandlerDeclaration:
            case ParseTree::Statement::RaiseEventDeclaration:
            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:
            case ParseTree::Statement::ForeignProcedureDeclaration:
            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignFunctionNone:
            case ParseTree::Statement::DelegateProcedureDeclaration:
            case ParseTree::Statement::DelegateFunctionDeclaration:
                fCorrectType = true;
                break;

            default:
                break;
        }
    }
    return fCorrectType;
}

//============================================================================
// Can the given statement be cast to a method declaration
//============================================================================
bool
ParseTreeHelpers::IsMethodDeclaration
(
    ParseTree::Statement *pStatement
)
{
    bool fCorrectType = false;
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
            case ParseTree::Statement::AddHandlerDeclaration:
            case ParseTree::Statement::RemoveHandlerDeclaration:
            case ParseTree::Statement::RaiseEventDeclaration:
            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:
            case ParseTree::Statement::DelegateProcedureDeclaration:
            case ParseTree::Statement::DelegateFunctionDeclaration:
                fCorrectType = true;
                break;

            default:
                break;
        }
    }
    return fCorrectType;
}

//============================================================================
// Can the given statement be cast to a type statement
//============================================================================
bool
ParseTreeHelpers::IsTypeStatement
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::Class:
            case ParseTree::Statement::Enum:
            case ParseTree::Statement::Interface:
            case ParseTree::Statement::Structure:
            case ParseTree::Statement::Module:
                return true;
        }
    }

    return false;
}

//============================================================================
// Can the given statement be cast to an enumerator statement
//============================================================================
bool
ParseTreeHelpers::IsEnumeratorStatement
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::Enumerator:
            case ParseTree::Statement::EnumeratorWithValue:
                return true;
        }
    }

    return false;
}

//============================================================================
// Can the given statement be cast to a variable declaration statement
//============================================================================
bool
ParseTreeHelpers::IsVariableDeclarationStatement
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::VariableDeclaration:
                return true;
        }
    }

    return false;
}

//============================================================================
// Can the given statement be cast to an ArrayType?
//============================================================================
bool
ParseTreeHelpers::IsArrayType
(
    ParseTree::Type *pType
)
{
    if (pType)
    {
        switch (pType->Opcode)
        {
            case ParseTree::Type::ArrayWithSizes:
            case ParseTree::Type::ArrayWithoutSizes:
                return true;
        }
    }

    return false;
}


//============================================================================
// Is the type opcode represent an intrinsic type?
//============================================================================
bool
ParseTreeHelpers::IsIntrinsicType
(
    ParseTree::Type *pType
)
{
    if (pType)
    {
        switch (pType->Opcode)
        {
            case ParseTree::Type::Short:
            case ParseTree::Type::UnsignedShort:
            case ParseTree::Type::Integer:
            case ParseTree::Type::UnsignedInteger:
            case ParseTree::Type::Long:
            case ParseTree::Type::UnsignedLong:
            case ParseTree::Type::Decimal:
            case ParseTree::Type::Single:
            case ParseTree::Type::Double:
            case ParseTree::Type::SignedByte:
            case ParseTree::Type::Byte:
            case ParseTree::Type::Boolean:
            case ParseTree::Type::Char:
            case ParseTree::Type::Date:
            case ParseTree::Type::String:
            case ParseTree::Type::Object:
                return true;
        }
    }

    return false;
}

//=============================================================================
// Is the given statement a partial type?
//=============================================================================
bool
ParseTreeHelpers::IsPartialType
(
    ParseTree::Statement *pStatement
)
{
    return pStatement &&
           (pStatement->Opcode == ParseTree::Statement::Class ||
            pStatement->Opcode == ParseTree::Statement::Structure) &&
            pStatement->AsType()->Specifiers &&
            pStatement->AsType()->Specifiers->HasSpecifier(ParseTree::Specifier::Partial);
}

//=============================================================================
// Can the given statement type have a return value?
//=============================================================================
bool
ParseTreeHelpers::CanHaveReturnType
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignFunctionNone:
            case ParseTree::Statement::DelegateFunctionDeclaration:
                return true;

            default:
                break;
        }
    }

    return false;
}

//=============================================================================
// Is the given statement an operator declaration?
//
//=============================================================================
bool
ParseTreeHelpers::IsConstructorDeclaration
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::ConstructorDeclaration:
                return true;
        }
    }

    return false;
}

//=============================================================================
// Is the given statement an operator declaration?
//
//=============================================================================
bool
ParseTreeHelpers::IsOperatorDeclaration
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::OperatorDeclaration:
                return true;
        }
    }

    return false;
}

//=============================================================================
// Is the given statement an event declaration?
//
// Note that this doesn't allow you to cast it to any specific type,
// because they don't share a common base.
//=============================================================================
bool
ParseTreeHelpers::IsEventDeclaration
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        return IsEventDeclaration(pStatement->Opcode);
    }

    return false;
}

//=============================================================================
// Is the given opcode an event declaration?
//
// Note that this doesn't allow you to cast it to any specific type,
// because they don't share a common base.
//=============================================================================
bool
ParseTreeHelpers::IsEventDeclaration
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::EventDeclaration:
        case ParseTree::Statement::BlockEventDeclaration:
            return true;
    }

    return false;
}

//=============================================================================
// Is the given opcode a property accessor?
//
//=============================================================================
bool
ParseTreeHelpers::IsPropertyAccessor
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
                return true;
        }
    }

    return false;
}

//=============================================================================
// Is the given opcode an event accessor?
//
//=============================================================================
bool
ParseTreeHelpers::IsEventAccessor
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        return IsEventAccessor(pStatement->Opcode);
    }

    return false;
}

//=============================================================================
// Is the given opcode an event accessor?
//
//=============================================================================
bool
ParseTreeHelpers::IsEventAccessor
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::AddHandlerDeclaration:
        case ParseTree::Statement::RemoveHandlerDeclaration:
        case ParseTree::Statement::RaiseEventDeclaration:
            return true;
    }

    return false;
}

//============================================================================
// Is this statement not in a method body?
//============================================================================

bool
ParseTreeHelpers::IsDeclareContext
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::Module:
        case ParseTree::Statement::Class:
        case ParseTree::Statement::Structure:
        case ParseTree::Statement::Enum:
        case ParseTree::Statement::File:
        case ParseTree::Statement::Property:
        case ParseTree::Statement::AutoProperty:
        case ParseTree::Statement::BlockEventDeclaration:
        case ParseTree::Statement::Interface:
        case ParseTree::Statement::Namespace:
        case ParseTree::Statement::CommentBlock:
        case ParseTree::Statement::Region:
            return true;

        default:
            return false;
    }
}

//============================================================================
// Can the given statement be cast to a method body statement
//============================================================================
bool
ParseTreeHelpers::IsMethodBody
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::ProcedureBody:
            case ParseTree::Statement::PropertyGetBody:
            case ParseTree::Statement::PropertySetBody:
            case ParseTree::Statement::FunctionBody:
            case ParseTree::Statement::OperatorBody:
            case ParseTree::Statement::AddHandlerBody:
            case ParseTree::Statement::RemoveHandlerBody:
            case ParseTree::Statement::RaiseEventBody:
                return true;
        }
    }

    return false;
}

//=============================================================================
// Is the given statement within a method?
//=============================================================================
bool
ParseTreeHelpers::IsWithinMethod
(
    ParseTree::Statement *pStatement,
    ParseTree::MethodBodyStatement **ppContainingMethodBody
)
{
    if (pStatement)
    {
        pStatement = pStatement->GetParent();
        while (pStatement && !IsDeclareContext(pStatement->Opcode))
        {
            if (IsMethodBody(pStatement))
            {
                if (ppContainingMethodBody)
                {
                    *ppContainingMethodBody = pStatement->AsMethodBody();
                }

                return true;
            }

            pStatement = pStatement->GetParent();
        }
    }

    return false;
}

//=============================================================================
// Is the given statement within a multiline lambda? If so, return the LambdaBody. 
// if fUltimateParent == TRUE, then will keep going up until we're out of any nested lambdas.
//=============================================================================
ParseTree::LambdaBodyStatement *
ParseTreeHelpers::IsWithinMultilineLambda
(
    _In_ ParseTree::Statement *pStatement,
    _In_ bool fUltimateParent
)
{
    if (pStatement)
    {
        while (pStatement)
        {
            if (pStatement->Opcode == ParseTree::Statement::LambdaBody)
            {
                if (fUltimateParent)    // recur to handle nested
                {
                    ParseTree::LambdaBodyStatement *pParentLambdaStatement = IsWithinMultilineLambda(pStatement->GetParent(), fUltimateParent);
                    if (pParentLambdaStatement)
                    {
                        return pParentLambdaStatement;
                    }
                }
                return pStatement->AsLambdaBody();
            }

            pStatement = pStatement->GetParent();
        }
    }
    return false;
}

//============================================================================
// Does the given opcode represent a constant expression
//============================================================================
bool
ParseTreeHelpers::IsConstantExpression
(
    ParseTree::Expression::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Expression::IntegralLiteral:
        case ParseTree::Expression::CharacterLiteral:
        case ParseTree::Expression::BooleanLiteral:
        case ParseTree::Expression::DecimalLiteral:
        case ParseTree::Expression::FloatingLiteral:
        case ParseTree::Expression::DateLiteral:
        case ParseTree::Expression::StringLiteral:
        case ParseTree::Expression::Nothing:
            return true;
    }

    return false;
}

//============================================================================
// Does the given opcode represent an operator assignment
//============================================================================
bool
ParseTreeHelpers::IsOperatorAssignment
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::AssignPlus:
        case ParseTree::Statement::AssignMinus:
        case ParseTree::Statement::AssignMultiply:
        case ParseTree::Statement::AssignDivide:
        case ParseTree::Statement::AssignPower:
        case ParseTree::Statement::AssignIntegralDivide:
        case ParseTree::Statement::AssignConcatenate:
        case ParseTree::Statement::AssignShiftLeft:
        case ParseTree::Statement::AssignShiftRight:
            return true;
    }

    return false;
}

//============================================================================
// Does the given opcode represent a function declaration (ie ... Function ...
//============================================================================
bool
ParseTreeHelpers::IsFunctionDeclaration
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::FunctionDeclaration:
        case ParseTree::Statement::DelegateFunctionDeclaration:
        case ParseTree::Statement::ForeignFunctionDeclaration:
            return true;
    }

    return false;
}

//============================================================================
// Does the given opcode represent a function declaration (ie ... Sub ...
//============================================================================
bool
ParseTreeHelpers::IsProcedureDeclaration
(
    ParseTree::Statement::Opcodes Opcode
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::DelegateProcedureDeclaration:
        case ParseTree::Statement::ForeignProcedureDeclaration:
            return true;
    }

    return false;
}




//============================================================================
// Retreive the punctuator for the relevant method kind
//============================================================================
void
ParseTreeHelpers::GetMethodKindPunctuator
(
    ParseTree::Statement *pStatement,
    ParseTree::PunctuatorLocation *plocPunctuator
)
{
    VSASSERT(pStatement && IsMethodSignature(pStatement), "Given statement is not a method!");
    VSASSERT(plocPunctuator, "Argument plocPunctuator cannot be NULL!");

    switch (pStatement->Opcode)
    {
        case ParseTree::Statement::PropertyGet:
        case ParseTree::Statement::PropertySet:
        case ParseTree::Statement::AddHandlerDeclaration:
        case ParseTree::Statement::RemoveHandlerDeclaration:
        case ParseTree::Statement::RaiseEventDeclaration:
        case ParseTree::Statement::EventDeclaration:
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::FunctionDeclaration:
        case ParseTree::Statement::OperatorDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
            *plocPunctuator = pStatement->AsMethodSignature()->MethodKind;
            break;

        case ParseTree::Statement::ForeignProcedureDeclaration:
        case ParseTree::Statement::ForeignFunctionDeclaration:
        case ParseTree::Statement::ForeignFunctionNone:
            *plocPunctuator = pStatement->AsForeignMethodDeclaration()->Declare;
            break;

        case ParseTree::Statement::DelegateProcedureDeclaration:
        case ParseTree::Statement::DelegateFunctionDeclaration:
            *plocPunctuator = pStatement->AsDelegateDeclaration()->Delegate;
            break;

        default:
            VSFAIL("Should not get here!");
            plocPunctuator->Column = 0;
            plocPunctuator->Line = 0;
            break;
    }
}

//============================================================================
// Retreive the punctuator for the relevant method kind (not enclosing)
//============================================================================
void
ParseTreeHelpers::GetMethodKindLocation
(
    ParseTree::Statement *pStatement,
    Location *pLoc,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    
    VSASSERT(pStatement && IsMethodSignature(pStatement), "Given statement is not a method!");
    VSASSERT(pLoc, "Argument pLoc cannot be NULL!");
    pLoc->Invalidate();

    ParseTree::PunctuatorLocation puncMethodKind;
    size_t lWidth = 0;
    switch (pStatement->Opcode)
    {
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkSUB));
            break;

        case ParseTree::Statement::FunctionDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkFUNCTION));
            break;

        case ParseTree::Statement::PropertyGet:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkGET));
            break;

        case ParseTree::Statement::PropertySet:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkSET));
            break;

        case ParseTree::Statement::AddHandlerDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkADDHANDLER));
            break;

        case ParseTree::Statement::RemoveHandlerDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkREMOVEHANDLER));
            break;

        case ParseTree::Statement::RaiseEventDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkRAISEEVENT));
            break;

        case ParseTree::Statement::EventDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkEVENT));
            break;

        case ParseTree::Statement::OperatorDeclaration:
            puncMethodKind = pStatement->AsMethodSignature()->MethodKind;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkOPERATOR));
            break;

        case ParseTree::Statement::ForeignProcedureDeclaration:
            puncMethodKind = pStatement->AsForeignMethodDeclaration()->Declare;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkSUB));
            break;

        case ParseTree::Statement::ForeignFunctionDeclaration:
            puncMethodKind = pStatement->AsForeignMethodDeclaration()->Declare;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkFUNCTION));
            break;
        case ParseTree::Statement::DelegateProcedureDeclaration:
            puncMethodKind = pStatement->AsDelegateDeclaration()->Delegate;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkSUB));
            break;

        case ParseTree::Statement::DelegateFunctionDeclaration:
            puncMethodKind = pStatement->AsDelegateDeclaration()->Delegate;
            lWidth = StringPool::StringLength(pCompiler->TokenToString(tkFUNCTION));
            break;

        case ParseTree::Statement::ForeignFunctionNone:
        default:
            puncMethodKind.Column = 0;
            puncMethodKind.Line = 0;
            lWidth = 0;
            break;
    }

    if (HasPunctuator(puncMethodKind) && lWidth > 0)
    {
        pLoc->m_lBegLine = pStatement->TextSpan.m_lBegLine + puncMethodKind.Line;
        pLoc->m_lBegColumn = puncMethodKind.Column;
        pLoc->m_lEndLine = pLoc->m_lBegLine;
        pLoc->m_lEndColumn = VBMath::Convert<long>(pLoc->m_lBegColumn + lWidth - 1);
    }
}

//============================================================================
// Return the location of parse tree element (sub, function, custom, etc.)
// after the attribute and specifier lists
//============================================================================
void
ParseTreeHelpers::GetLocationAfterAttributesAndSpecifiers
(
    ParseTree::Statement *pStatement,
    Location *pLoc
)
{
    VSASSERT(pStatement && (IsMethodSignature(pStatement) || pStatement->Opcode == ParseTree::Statement::BlockEventDeclaration),
             "Given statement is not a method!");
    VSASSERT(pLoc, "Argument pLoc cannot be NULL!");

    switch (pStatement->Opcode)
    {
        case ParseTree::Statement::PropertyGet:  // punctuator: tkGET
        case ParseTree::Statement::PropertySet:  // punctuator: tkSET
        case ParseTree::Statement::AddHandlerDeclaration: // punctuator: tkADDHANDLER
        case ParseTree::Statement::RemoveHandlerDeclaration: // punctuator: tkREMOVEHANDLER
        case ParseTree::Statement::RaiseEventDeclaration: // punctuator:tkRAISEEVENT 
        case ParseTree::Statement::EventDeclaration: // punctuator: tkEVENT
        case ParseTree::Statement::ProcedureDeclaration:  // punctuator: tkSUB
        case ParseTree::Statement::FunctionDeclaration: // punctuator: tkFUNCTION 
        case ParseTree::Statement::OperatorDeclaration: // punctuator tkOPERATOR
        case ParseTree::Statement::ConstructorDeclaration: //punctuator: tkNEW
            GetPunctuatorLocation(&pStatement->TextSpan, &pStatement->AsMethodSignature()->MethodKind, pLoc, 0, true/* fAllowColumn0 = true*/);
            break;

        case ParseTree::Statement::BlockEventDeclaration:
            // Note:  This method returns the location after the specifier list in the parse tree,
            //        which the location of the Custom 'specifier' for block event declarations
            *pLoc = pStatement->AsBlockEventDeclaration()->EventSignature->BlockEventModifier;
            break;

        case ParseTree::Statement::ForeignProcedureDeclaration:
        case ParseTree::Statement::ForeignFunctionDeclaration:
        case ParseTree::Statement::ForeignFunctionNone:
            GetPunctuatorLocation(&pStatement->TextSpan, &pStatement->AsForeignMethodDeclaration()->Declare, pLoc);
            break;

        case ParseTree::Statement::DelegateProcedureDeclaration:
        case ParseTree::Statement::DelegateFunctionDeclaration:
            GetPunctuatorLocation(&pStatement->TextSpan, &pStatement->AsDelegateDeclaration()->Delegate, pLoc);
            break;

        default:
            VSFAIL("Should not get here!");
            pLoc->Invalidate();
            break;
    }
}

//=============================================================================
// Insert the end construct for the given parse tree opcode into the buffer
//=============================================================================
bool
ParseTreeHelpers::GetEndConstructFromOpcode
(
    ParseTree::Statement::Opcodes opcode,
    StringBuffer *psbEndConstruct,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    
    VSASSERT(psbEndConstruct, "Argument psbEndConstruct cannot be NULL!");

    tokens tkType = MapOpcodeToEndTokenType(opcode);
    switch (opcode)
    {
        case ParseTree::Statement::Structure:
        case ParseTree::Statement::Module:
        case ParseTree::Statement::Namespace:
        case ParseTree::Statement::Class:
        case ParseTree::Statement::Interface:
        case ParseTree::Statement::Enum:
        case ParseTree::Statement::BlockIf:
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::ElseIf:
        case ParseTree::Statement::While:
        case ParseTree::Statement::With:
        case ParseTree::Statement::SyncLock:
        case ParseTree::Statement::Property:
        case ParseTree::Statement::AutoProperty:
        case ParseTree::Statement::BlockEventDeclaration:
        case ParseTree::Statement::Try:
        case ParseTree::Statement::Catch:
        case ParseTree::Statement::Finally:
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
        case ParseTree::Statement::FunctionDeclaration:
        case ParseTree::Statement::OperatorDeclaration:
        case ParseTree::Statement::PropertySet:
        case ParseTree::Statement::PropertyGet:
        case ParseTree::Statement::AddHandlerDeclaration:
        case ParseTree::Statement::RemoveHandlerDeclaration:
        case ParseTree::Statement::RaiseEventDeclaration:
        case ParseTree::Statement::Using:
        case ParseTree::Statement::ProcedureBody:
        case ParseTree::Statement::PropertyGetBody:
        case ParseTree::Statement::PropertySetBody:
        case ParseTree::Statement::AddHandlerBody:
        case ParseTree::Statement::RemoveHandlerBody:
        case ParseTree::Statement::RaiseEventBody:
        case ParseTree::Statement::FunctionBody:
        case ParseTree::Statement::OperatorBody:
            psbEndConstruct->AppendSTRING(pCompiler->TokenToString(tkEND));
            psbEndConstruct->AppendChar(L' ');
            break;
        default:
            break;
    }

    if (tkType != tkNone)
    {
        psbEndConstruct->AppendSTRING(pCompiler->TokenToString(tkType));
        return true;
    }

    return false;
}

//=============================================================================
// Convert the parse tree opcode to its token
//=============================================================================
tokens
ParseTreeHelpers::MapOpcodeToTokenType
(
    ParseTree::Statement::Opcodes opcode,
    _In_opt_ ParseTree::LambdaExpression *pLambda
)
{
    tokens tkType = tkNone;
    switch (opcode)
    {
        case ParseTree::Statement::Structure:
            tkType = tkSTRUCTURE;
            break;
        case ParseTree::Statement::Module:
            tkType = tkMODULE;
            break;
        case ParseTree::Statement::Namespace:
            tkType = tkNAMESPACE;
            break;
        case ParseTree::Statement::Class:
            tkType = tkCLASS;
            break;
        case ParseTree::Statement::Interface:
            tkType = tkINTERFACE;
            break;
        case ParseTree::Statement::Enum:
            tkType = tkENUM;
            break;
        case ParseTree::Statement::BlockIf:
            tkType = tkIF;
            break;
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::ElseIf:
            tkType = tkELSE;
            break;
        case ParseTree::Statement::While:
            tkType = tkWHILE;
            break;
        case ParseTree::Statement::With:
            tkType = tkWITH;
            break;
        case ParseTree::Statement::SyncLock:
            tkType = tkSYNCLOCK;
            break;
        case ParseTree::Statement::Property:
        case ParseTree::Statement::AutoProperty:
            tkType = tkPROPERTY;
            break;
        case ParseTree::Statement::Try:
            tkType = tkTRY;
            break;
        case ParseTree::Statement::Catch:
            tkType = tkCATCH;
            break;
        case ParseTree::Statement::Finally:
            tkType = tkFINALLY;
            break;
        case ParseTree::Statement::ForFromTo:
        case ParseTree::Statement::ForEachIn:
            tkType = tkFOR;
            break;
        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
        case ParseTree::Statement::DoWhileBottomTest:
        case ParseTree::Statement::DoUntilBottomTest:
        case ParseTree::Statement::DoForever:
            tkType = tkDO;
            break;
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
            tkType = tkSUB;
            break;
        case ParseTree::Statement::FunctionDeclaration:
            tkType = tkFUNCTION;
            break;
        case ParseTree::Statement::OperatorDeclaration:
            tkType = tkOPERATOR;
            break;
        case ParseTree::Statement::PropertySet:
            tkType = tkSET;
            break;
        case ParseTree::Statement::PropertyGet:
            tkType = tkGET;
            break;
        case ParseTree::Statement::AddHandlerDeclaration:
            tkType = tkADDHANDLER;
            break;
        case ParseTree::Statement::RemoveHandlerDeclaration:
            tkType = tkREMOVEHANDLER;
            break;
        case ParseTree::Statement::RaiseEventDeclaration:
            tkType = tkRAISEEVENT;
            break;
        case ParseTree::Statement::Using:
            tkType = tkUSING;
            break;
        case ParseTree::Statement::DelegateProcedureDeclaration:
        case ParseTree::Statement::DelegateFunctionDeclaration:
            tkType = tkDELEGATE;
            break;
        case ParseTree::Statement::EventDeclaration:
        case ParseTree::Statement::BlockEventDeclaration:
            tkType = tkEVENT;
            break;
        case ParseTree::Statement::ForeignProcedureDeclaration:
        case ParseTree::Statement::ForeignFunctionDeclaration:
        case ParseTree::Statement::ForeignFunctionNone:
            tkType = tkDECLARE;
            break;
        case ParseTree::Statement::LambdaBody:
        {
            // Need the lambda expression
            ThrowIfNull(pLambda);
            if (pLambda->MethodFlags & DECLF_Function)
            {
                tkType = tkFUNCTION;
            }
            else
            {
                tkType = tkSUB;
            }
            break;
        }
        default:
            break;
    }
    return tkType;
}

//=============================================================================
// Convert the parse tree opcode to the token for its end construct
//=============================================================================
tokens
ParseTreeHelpers::MapOpcodeToEndTokenType
(
    ParseTree::Statement::Opcodes opcode
)
{
    tokens tkType = tkNone;
    switch (opcode)
    {
        case ParseTree::Statement::Structure:
            tkType = tkSTRUCTURE;
            break;
        case ParseTree::Statement::Module:
            tkType = tkMODULE;
            break;
        case ParseTree::Statement::Namespace:
            tkType = tkNAMESPACE;
            break;
        case ParseTree::Statement::Class:
            tkType = tkCLASS;
            break;
        case ParseTree::Statement::Interface:
            tkType = tkINTERFACE;
            break;
        case ParseTree::Statement::Enum:
            tkType = tkENUM;
            break;
        case ParseTree::Statement::BlockIf:
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::ElseIf:
            tkType = tkIF;
            break;
        case ParseTree::Statement::While:
            tkType = tkWHILE;
            break;
        case ParseTree::Statement::With:
            tkType = tkWITH;
            break;
        case ParseTree::Statement::SyncLock:
            tkType = tkSYNCLOCK;
            break;
        case ParseTree::Statement::Property:
        case ParseTree::Statement::AutoProperty:
            tkType = tkPROPERTY;
            break;
        case ParseTree::Statement::BlockEventDeclaration:
            tkType = tkEVENT;
            break;
        case ParseTree::Statement::Try:
        case ParseTree::Statement::Catch:
        case ParseTree::Statement::Finally:
            tkType = tkTRY;
            break;
        case ParseTree::Statement::ForFromTo:
        case ParseTree::Statement::ForEachIn:
            tkType = tkNEXT;
            break;
        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
        case ParseTree::Statement::DoWhileBottomTest:
        case ParseTree::Statement::DoUntilBottomTest:
        case ParseTree::Statement::DoForever:
            tkType = tkLOOP;
            break;
        case ParseTree::Statement::ProcedureBody:
        case ParseTree::Statement::ProcedureDeclaration:
        case ParseTree::Statement::ConstructorDeclaration:
            tkType = tkSUB;
            break;
        case ParseTree::Statement::FunctionBody:
        case ParseTree::Statement::FunctionDeclaration:
            tkType = tkFUNCTION;
            break;
        case ParseTree::Statement::OperatorBody:
        case ParseTree::Statement::OperatorDeclaration:
            tkType = tkOPERATOR;
            break;
        case ParseTree::Statement::PropertySetBody:
        case ParseTree::Statement::PropertySet:
            tkType = tkSET;
            break;
        case ParseTree::Statement::PropertyGetBody:
        case ParseTree::Statement::PropertyGet:
            tkType = tkGET;
            break;
        case ParseTree::Statement::AddHandlerBody:
        case ParseTree::Statement::AddHandlerDeclaration:
            tkType = tkADDHANDLER;
            break;
        case ParseTree::Statement::RemoveHandlerBody:
        case ParseTree::Statement::RemoveHandlerDeclaration:
            tkType = tkREMOVEHANDLER;
            break;
        case ParseTree::Statement::RaiseEventBody:
        case ParseTree::Statement::RaiseEventDeclaration:
            tkType = tkRAISEEVENT;
            break;
        case ParseTree::Statement::Using:
            tkType = tkUSING;
            break;
        case ParseTree::Statement::Region:
            tkType = tkREGION;
            break;
        default:
            break;
    }
    return tkType;
}

//=============================================================================
// Convert the specifier opcode to its string representation
// The opcode is assumed be to equal to exactly one specifier type
//=============================================================================
STRING *
ParseTreeHelpers::MapSpecifierOpcodeToSTRING
(
    ParseTree::Specifier::Specifiers opcode,        // [in] Specifier opcode
    Compiler *pCompiler                             // [in] Compiler that contains the string table, cannot be NULL
)
{
    STRING *pstrSpecifier = NULL;

    switch (opcode)
    {
        case ParseTree::Specifier::Private:
            pstrSpecifier = pCompiler->TokenToString(tkPRIVATE);
            break;
        case ParseTree::Specifier::Protected:
            pstrSpecifier = pCompiler->TokenToString(tkPROTECTED);
            break;
        case ParseTree::Specifier::Friend:
            pstrSpecifier = pCompiler->TokenToString(tkFRIEND);
            break;
        case ParseTree::Specifier::ProtectedFriend:
            pstrSpecifier = pCompiler->ConcatStrings(pCompiler->TokenToString(tkPROTECTED),
                                                     L" ",
                                                     pCompiler->TokenToString(tkFRIEND));
            break;
        case ParseTree::Specifier::Public:
            pstrSpecifier = pCompiler->TokenToString(tkPUBLIC);
            break;
        case ParseTree::Specifier::NotOverridable:
            pstrSpecifier = pCompiler->TokenToString(tkNOTOVERRIDABLE);
            break;
        case ParseTree::Specifier::Overridable:
            pstrSpecifier = pCompiler->TokenToString(tkOVERRIDABLE);
            break;
        case ParseTree::Specifier::MustOverride:
            pstrSpecifier = pCompiler->TokenToString(tkMUSTOVERRIDE);
            break;
        case ParseTree::Specifier::Dim:
            pstrSpecifier = pCompiler->TokenToString(tkDIM);
            break;
        case ParseTree::Specifier::Const:
            pstrSpecifier = pCompiler->TokenToString(tkCONST);
            break;
        case ParseTree::Specifier::MustInherit:
            pstrSpecifier = pCompiler->TokenToString(tkMUSTINHERIT);
            break;
        case ParseTree::Specifier::NotInheritable:
            pstrSpecifier = pCompiler->TokenToString(tkNOTINHERITABLE);
            break;
        case ParseTree::Specifier::Overloads:
            pstrSpecifier = pCompiler->TokenToString(tkOVERLOADS);
            break;
        case ParseTree::Specifier::Overrides:
            pstrSpecifier = pCompiler->TokenToString(tkOVERRIDES);
            break;
        case ParseTree::Specifier::Default:
            pstrSpecifier = pCompiler->TokenToString(tkDEFAULT);
            break;
        case ParseTree::Specifier::Static:
            pstrSpecifier = pCompiler->TokenToString(tkSTATIC);
            break;
        case ParseTree::Specifier::Shared:
            pstrSpecifier = pCompiler->TokenToString(tkSHARED);
            break;
        case ParseTree::Specifier::Shadows:
            pstrSpecifier = pCompiler->TokenToString(tkSHADOWS);
            break;
        case ParseTree::Specifier::ReadOnly:
            pstrSpecifier = pCompiler->TokenToString(tkREADONLY);
            break;
        case ParseTree::Specifier::WriteOnly:
            pstrSpecifier = pCompiler->TokenToString(tkWRITEONLY);
            break;
        case ParseTree::Specifier::WithEvents:
            pstrSpecifier = pCompiler->TokenToString(tkWITHEVENTS);
            break;
        case ParseTree::Specifier::Widening:
            pstrSpecifier = pCompiler->TokenToString(tkWIDENING);
            break;
        case ParseTree::Specifier::Narrowing:
            pstrSpecifier = pCompiler->TokenToString(tkNARROWING);
            break;
        case ParseTree::Specifier::Partial:
            pstrSpecifier = pCompiler->TokenToString(tkPARTIAL);
            break;
        case ParseTree::Specifier::Custom:
            pstrSpecifier = pCompiler->TokenToString(tkCUSTOM);
            break;
        case ParseTree::Specifier::Iterator:
            pstrSpecifier = pCompiler->TokenToString(tkITERATOR);
            break;
        case ParseTree::Specifier::Async:
            pstrSpecifier = pCompiler->TokenToString(tkASYNC);
            break;
        default:
            break;
    }

    return pstrSpecifier;
}

//=============================================================================
// Returns an index that can be used to determine specifier ordering
// in formatted code
//=============================================================================
long
ParseTreeHelpers::MapSpecifierOpcodeToIndex
(
    ParseTree::Specifier::Specifiers opcode
)
{
    long lIndex = 0;
    switch (opcode)
    {
        case ParseTree::Specifier::Partial:
            lIndex = 5;
            break;
        case ParseTree::Specifier::Default:
            lIndex = 10;
            break;
        case ParseTree::Specifier::Private:
            lIndex = 20;
            break;
        case ParseTree::Specifier::Protected:
            lIndex = 30;
            break;
        case ParseTree::Specifier::Public:
            lIndex = 40;
            break;
        case ParseTree::Specifier::Friend:
            lIndex = 50;
            break;
        case ParseTree::Specifier::NotOverridable:
            lIndex = 60;
            break;
        case ParseTree::Specifier::Overridable:
            lIndex = 70;
            break;
        case ParseTree::Specifier::MustOverride:
            lIndex = 80;
            break;
        case ParseTree::Specifier::Overloads:
            lIndex = 90;
            break;
        case ParseTree::Specifier::Overrides:
            lIndex = 100;
            break;
        case ParseTree::Specifier::MustInherit:
            lIndex = 110;
            break;
        case ParseTree::Specifier::NotInheritable:
            lIndex = 120;
            break;
        case ParseTree::Specifier::Static:
            lIndex = 130;
            break;
        case ParseTree::Specifier::Shared:
            lIndex = 140;
            break;
        case ParseTree::Specifier::Shadows:
            lIndex = 150;
            break;
        case ParseTree::Specifier::ReadOnly:
            lIndex = 160;
            break;
        case ParseTree::Specifier::WriteOnly:
            lIndex = 170;
            break;
        case ParseTree::Specifier::Dim:
            lIndex = 180;
            break;
        case ParseTree::Specifier::Const:
            lIndex = 190;
            break;
        case ParseTree::Specifier::WithEvents:
            lIndex = 200;
            break;
        case ParseTree::Specifier::Widening:
            lIndex = 210;
            break;
        case ParseTree::Specifier::Narrowing:
            lIndex = 220;
            break;
        case ParseTree::Specifier::Custom:
            lIndex = 230;
            break;
        default:
            break;
    }

    return lIndex;
}

//=============================================================================
// Map the access to its specifier opcode
//=============================================================================
ParseTree::Specifier::Specifiers
ParseTreeHelpers::MapAccessToSpecifierOpcode
(
    ACCESS access
)
{
    ParseTree::Specifier::Specifiers opcode = (ParseTree::Specifier::Specifiers) 0;

    switch (access)
    {
        case ACCESS_Private:
        case ACCESS_IntersectionProtectedFriend:
            opcode = ParseTree::Specifier::Private;
            break;

        case ACCESS_Protected:
            opcode = ParseTree::Specifier::Protected;
            break;

        case ACCESS_ProtectedFriend:
            opcode = ParseTree::Specifier::ProtectedFriend;
            break;

        case ACCESS_Friend:
            opcode = ParseTree::Specifier::Friend;
            break;

        case ACCESS_Public:
            opcode = ParseTree::Specifier::Public;
            break;

        default:
            break;
    };

    return opcode;
}

//=============================================================================
// Convert the parameter specifier opcode to its string representation
//=============================================================================
STRING *
ParseTreeHelpers::MapParamSpecifierOpcodeToSTRING
(
    ParseTree::ParameterSpecifier::Specifiers opcode,     // [in] Specifier opcode
    Compiler *pCompiler                                   // [in] Compiler that contains the string table, cannot be NULL
)
{
    STRING *pstrSpecifier = NULL;

    switch (opcode)
    {
        case ParseTree::ParameterSpecifier::Optional:
            pstrSpecifier = pCompiler->TokenToString(tkOPTIONAL);
            break;
        case ParseTree::ParameterSpecifier::ByRef:
            pstrSpecifier = pCompiler->TokenToString(tkBYREF);
            break;
        case ParseTree::ParameterSpecifier::ByVal:
            pstrSpecifier = pCompiler->TokenToString(tkBYVAL);
            break;
        case ParseTree::ParameterSpecifier::ParamArray:
            pstrSpecifier = pCompiler->TokenToString(tkPARAMARRAY);
            break;
        default:
            break;
    }

    return pstrSpecifier;
}

//=============================================================================
// Returns an index that can be used to determine parameter specifier ordering
// in formatted code
//=============================================================================
long
ParseTreeHelpers::MapParamSpecifierOpcodeToIndex
(
    ParseTree::ParameterSpecifier::Specifiers opcode
)
{
    long lIndex = 0;
    switch (opcode)
    {
        case ParseTree::ParameterSpecifier::Optional:
            lIndex = 10;
            break;
        case ParseTree::ParameterSpecifier::ByRef:
            lIndex = 20;
            break;
        case ParseTree::ParameterSpecifier::ByVal:
            lIndex = 30;
            break;
        case ParseTree::ParameterSpecifier::ParamArray:
            lIndex = 40;
            break;
        default:
            break;
    }

    return lIndex;
}

//=============================================================================
// Obtain token type from specifier opcode.
//=============================================================================
tokens
ParseTreeHelpers::MapToTokenType
(
    ParseTree::Specifier::Specifiers specifierOpcode
)
{
    switch (specifierOpcode)
    {
        case ParseTree::Specifier::Private:
            return tkPRIVATE;
        case ParseTree::Specifier::Protected:
            return tkPROTECTED;
        case ParseTree::Specifier::Friend:
            return tkFRIEND;
        case ParseTree::Specifier::Public:
            return tkPUBLIC;

        case ParseTree::Specifier::NotOverridable:
            return tkNOTOVERRIDABLE;
        case ParseTree::Specifier::Overridable:
            return tkOVERRIDABLE;
        case ParseTree::Specifier::MustOverride:
            return tkMUSTOVERRIDE;

        case ParseTree::Specifier::Dim:
            return tkDIM;
        case ParseTree::Specifier::Const:
            return tkCONST;

        case ParseTree::Specifier::MustInherit:
            return tkMUSTINHERIT;
        case ParseTree::Specifier::NotInheritable:
            return tkNOTINHERITABLE;
        case ParseTree::Specifier::Overloads:
            return tkOVERLOADS;
        case ParseTree::Specifier::Overrides:
            return tkOVERRIDES;
        case ParseTree::Specifier::Default:
            return tkDEFAULT;
        case ParseTree::Specifier::Static:
            return tkSTATIC;
        case ParseTree::Specifier::Shared:
            return tkSHARED;
        case ParseTree::Specifier::Shadows:
            return tkSHADOWS;

        case ParseTree::Specifier::ReadOnly:
            return tkREADONLY;
        case ParseTree::Specifier::WriteOnly:
            return tkWRITEONLY;

        case ParseTree::Specifier::WithEvents:
            return tkWITHEVENTS;

        case ParseTree::Specifier::Partial:
            return tkPARTIAL;

        case ParseTree::Specifier::Widening:
            return tkWIDENING;
        case ParseTree::Specifier::Narrowing:
            return tkNARROWING;

        case ParseTree::Specifier::Custom:
            return tkCUSTOM;

        default:
            VSFAIL("Unexpected specifier kind.");
            return tkNone;
    }
}

//=============================================================================
// Obtain token type from foreign method declaration string mode.
//=============================================================================
tokens
ParseTreeHelpers::MapToTokenType
(
    ParseTree::ForeignMethodDeclarationStatement::StringModes stringMode
)
{
    switch (stringMode)
    {
        case ParseTree::ForeignMethodDeclarationStatement::None:
            return tkNone;
        case ParseTree::ForeignMethodDeclarationStatement::ANSI:
            return tkANSI;
        case ParseTree::ForeignMethodDeclarationStatement::Unicode:
            return tkUNICODE;
        case ParseTree::ForeignMethodDeclarationStatement::Auto:
            return tkAUTO;
        default:
            VSFAIL("Unexpected Declare StringMode value.");
            return tkNone;
    }
}

//=============================================================================
// Obtain the event's method signature declaration, if any
//=============================================================================
ParseTree::MethodSignatureStatement *
ParseTreeHelpers::GetEventDeclaration
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::EventDeclaration:
                return pStatement->AsMethodSignature();

            case ParseTree::Statement::BlockEventDeclaration:
                return pStatement->AsBlockEventDeclaration()->EventSignature;
        }
    }

    return NULL;
}

//=============================================================================
// Obtain the statement's parameter list, if any
//=============================================================================
ParseTree::ParameterList *
ParseTreeHelpers::GetParameters
(
    ParseTree::Statement *pStatement
)
{
    ParseTree::ParameterList *pParamList = NULL;

    if (IsMethodSignature(pStatement))
    {
        pParamList = pStatement->AsMethodSignature()->Parameters;
    }
    else if (pStatement->Opcode == ParseTree::Statement::Property ||
             pStatement->Opcode == ParseTree::Statement::AutoProperty)
    {
        pParamList = pStatement->AsProperty()->Parameters;
    }
    else if (pStatement->Opcode == ParseTree::Statement::BlockEventDeclaration)
    {
        pParamList = GetParameters(pStatement->AsBlockEventDeclaration()->EventSignature);
    }

    return pParamList;
}

//=============================================================================
// Obtain the statement's generic parameter list, if any
//=============================================================================
ParseTree::GenericParameterList *
ParseTreeHelpers::GetGenericParameters
(
    ParseTree::Statement *pStatement
)
{
    ParseTree::GenericParameterList *pGenericParamList = NULL;

    if (IsMethodDeclaration(pStatement))
    {
        pGenericParamList = pStatement->AsMethodDeclaration()->GenericParameters;
    }
    else if (IsTypeStatement(pStatement))
    {
        pGenericParamList = pStatement->AsType()->GenericParameters;
    }

    return pGenericParamList;
}


//=============================================================================
// Get the generic arguments out of a type.
//=============================================================================
ParseTree::GenericArguments *
ParseTreeHelpers::GetGenericArguments
(
    ParseTree::Type *pPossibleGenericType
)
{
    return GetGenericArguments(GetTypeName(pPossibleGenericType));
}

//=============================================================================
// Get the generic arguments out of a name.
//=============================================================================
ParseTree::GenericArguments *
ParseTreeHelpers::GetGenericArguments
(
    ParseTree::Name *pPossibleGenericName
)
{
    if (pPossibleGenericName)
    {
        switch (pPossibleGenericName->Opcode)
        {
            case ParseTree::Name::SimpleWithArguments:
                return  &pPossibleGenericName->AsSimpleWithArguments()->Arguments;

            case ParseTree::Name::QualifiedWithArguments:
                return &pPossibleGenericName->AsQualifiedWithArguments()->Arguments;
        }
    }

    return NULL;
}

//=============================================================================
// Get the number of generic arguments in the call target expression
//=============================================================================
int
ParseTreeHelpers::GetGenericArgumentCountInCallTarget
(
    ParseTree::CallOrIndexExpression *pCallExpression
)
{
    int iCount = 0;

    if (pCallExpression &&
        pCallExpression->Target &&
        pCallExpression->Target->Opcode == ParseTree::Expression::GenericQualified)
    {
        ParseTree::TypeList *pTypes = pCallExpression->Target->AsGenericQualified()->Arguments.Arguments;

        while (pTypes)
        {
            iCount++;
            pTypes = pTypes->Next;
        }
    }

    return iCount;
}

//=============================================================================
// Get count of generic arguments
//=============================================================================
int
ParseTreeHelpers::GetGenericArgumentCount
(
    ParseTree::GenericArguments *pGenericArguments
)
{
    int iArity = 0;
    if (pGenericArguments)
    {
        ParseTree::TypeList *pTypes = pGenericArguments->Arguments;

        while (pTypes)
        {
            iArity++;
            pTypes = pTypes->Next;
        }
    }

    return iArity;
}

//=============================================================================
// Get the arity of the statement if it has generic parameters
//=============================================================================
int
ParseTreeHelpers::GetArity
(
    ParseTree::Statement *pStatement
)
{
    ParseTree::GenericParameterList *pGenericParameters = GetGenericParameters(pStatement);


    int iArity = 0;
    while (pGenericParameters)
    {
        iArity++;
        pGenericParameters = pGenericParameters->Next;
    }

    return iArity;
}

//=============================================================================
// Get the arity of the name if it has generic parameters
//=============================================================================
int
ParseTreeHelpers::GetArity
(
    ParseTree::Name *pName
)
{
    return GetGenericArgumentCount(GetGenericArguments(pName));
}

//=============================================================================
// Get the arity of the type if it has generic parameters
//=============================================================================
int
ParseTreeHelpers::GetTypeArity
(
    ParseTree::Type *pType
)
{
    return GetGenericArgumentCount(GetGenericArguments(pType));
}

//=============================================================================
// Get the name out of a type.
//=============================================================================
ParseTree::Name *
ParseTreeHelpers::GetTypeName
(
    ParseTree::Type *pPossibleNamedType
)
{
    if (pPossibleNamedType &&
        pPossibleNamedType->Opcode == ParseTree::Type::Named)
    {
        return pPossibleNamedType->AsNamed()->TypeName;
    }

    return NULL;
}

//=============================================================================
// Get the name out of a type.
//=============================================================================
bool
ParseTreeHelpers::DoesTypeNameReferToGenericArguments
(
    ParseTree::Type *pType
)
{
    bool fFound = false;

    ParseTree::Name *pName = GetTypeName(pType);

    while (pName && !fFound)
    {
       switch (pName->Opcode)
       {
            case ParseTree::Name::SimpleWithArguments:
            case ParseTree::Name::QualifiedWithArguments:
                fFound = true;
                break;

            case ParseTree::Name::Qualified:
                pName = pName->AsQualified()->Base;
                break;

            default:
                pName = NULL;
                break;
       }
    }

    return fFound;
}

//=============================================================================
// Find the first implements statement in the given type
//=============================================================================
ParseTree::TypeListStatement *
ParseTreeHelpers::FindFirstImplements
(
    ParseTree::TypeStatement *pType
)
{
    return FindFirstMatchingTypeList(pType, ParseTree::Statement::Implements);
}

//=============================================================================
// Find the first inherits statement in the given type
//=============================================================================
ParseTree::TypeListStatement *
ParseTreeHelpers::FindFirstInherits
(
    ParseTree::TypeStatement *pType
)
{
    return FindFirstMatchingTypeList(pType, ParseTree::Statement::Inherits);
}

//=============================================================================
// Find the first matching typelist statement in the given type
//=============================================================================
ParseTree::TypeListStatement *
ParseTreeHelpers::FindFirstMatchingTypeList
(
    ParseTree::TypeStatement *pType,
    ParseTree::Statement::Opcodes Opcode
)
{
    ParseTree::TypeListStatement *pTypeListStatement = NULL;

    if (pType && pType->Children)
    {
        ParseTree::StatementList *pChildren = pType->Children;

        while (pChildren && pChildren->Element)
        {
            if (!pChildren->Element->HasSyntaxError &&
                pChildren->Element->Opcode == Opcode)
            {
                pTypeListStatement = pChildren->Element->AsTypeList();
                break;
            }

            pChildren = pChildren->NextInBlock;
        }
    }

    return pTypeListStatement;
}

//=============================================================================
// Find the next matching typelist statement after the given one
//=============================================================================
ParseTree::TypeListStatement *
ParseTreeHelpers::FindNextMatchingTypeList
(
    ParseTree::Statement *pStatement,
    ParseTree::Statement::Opcodes Opcode
)
{
    ParseTree::TypeListStatement *pTypeListStatement = NULL;

    if (pStatement && pStatement->ContainingList)
    {
        ParseTree::StatementList *pChildren = pStatement->ContainingList->NextInBlock;

        while (pChildren && pChildren->Element)
        {
            if (!pChildren->Element->HasSyntaxError &&
                pChildren->Element->Opcode == Opcode)
            {
                pTypeListStatement = pChildren->Element->AsTypeList();
                break;
            }

            pChildren = pChildren->NextInBlock;
        }
    }

    return pTypeListStatement;
}

//=============================================================================
// Find the first property set of the given property
//=============================================================================
ParseTree::MethodDefinitionStatement *
ParseTreeHelpers::FindFirstPropertySet
(
    ParseTree::PropertyStatement *pProperty
)
{
    ParseTree::MethodDefinitionStatement *pPropertySet = NULL;

    if (pProperty && pProperty->Children)
    {
        ParseTree::StatementList *pChildren = pProperty->Children;
        while (pChildren && pChildren->Element)
        {
            if (!pChildren->Element->HasSyntaxError &&
                pChildren->Element->Opcode == ParseTree::Statement::PropertySet)
            {
                pPropertySet = pChildren->Element->AsMethodDefinition();
                break;
            }
            pChildren = pChildren->NextInBlock;
        }
    }

    return pPropertySet;
}

//=============================================================================
// Does the given property statement have a Get?
//=============================================================================
bool
ParseTreeHelpers::HasPropertyGet
(
    ParseTree::PropertyStatement *pProperty
)
{
    if (pProperty && pProperty->Children)
    {
        ParseTree::StatementList *pChildren = pProperty->Children;
        while (pChildren && pChildren->Element)
        {
            if (pChildren->Element->Opcode == ParseTree::Statement::PropertyGet)
            {
                return true;
            }
            pChildren = pChildren->NextInBlock;
        }
    }

    return false;
}

//=============================================================================
// Does the given property statement have a Set?
//=============================================================================
bool
ParseTreeHelpers::HasPropertySet
(
    ParseTree::PropertyStatement *pProperty
)
{
    if (pProperty && pProperty->Children)
    {
        ParseTree::StatementList *pChildren = pProperty->Children;
        while (pChildren && pChildren->Element)
        {
            if (pChildren->Element->Opcode == ParseTree::Statement::PropertySet)
            {
                return true;
            }
            pChildren = pChildren->NextInBlock;
        }
    }

    return false;
}

ParseTree::SpecifierList *
ParseTreeHelpers::GetSpecifierList
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::Structure:
            case ParseTree::Statement::Interface:
            case ParseTree::Statement::Class:
            case ParseTree::Statement::Module:
            case ParseTree::Statement::Enum:
                return pStatement->AsType()->Specifiers;

            case ParseTree::Statement::Property:
            case ParseTree::Statement::AutoProperty:
                return pStatement->AsProperty()->Specifiers;

            case ParseTree::Statement::BlockEventDeclaration:
                return pStatement->AsBlockEventDeclaration()->EventSignature->Specifiers;

            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
            case ParseTree::Statement::AddHandlerDeclaration:
            case ParseTree::Statement::RemoveHandlerDeclaration:
            case ParseTree::Statement::RaiseEventDeclaration:
            case ParseTree::Statement::DelegateProcedureDeclaration:
            case ParseTree::Statement::DelegateFunctionDeclaration:
            case ParseTree::Statement::ForeignProcedureDeclaration:
            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignFunctionNone:
            case ParseTree::Statement::EventDeclaration:
                return pStatement->AsMethodSignature()->Specifiers;

            case ParseTree::Statement::VariableDeclaration:
                return pStatement->AsVariableDeclaration()->Specifiers;
        }
    }

    return NULL;
}

ParseTree::Statement *
ParseTreeHelpers::GetPreviousNonEmptyInBlock
(
    ParseTree::Statement *pStatement
)
{
    if (pStatement)
    {
        ParseTree::StatementList *pStatementList = pStatement->ContainingList->PreviousInBlock;

        // Skip over empty statements.
        while (
            pStatementList &&
            pStatementList->Element->Opcode == ParseTree::Statement::Empty)
        {
            pStatementList = pStatementList->PreviousInBlock;
        }

        if (pStatementList)
        {
            return pStatementList->Element;
        }
    }

    return NULL;
}

ParseTree::StatementList *
ParseTreeHelpers::SkipOverPlacebos
(
    ParseTree::StatementList *pStatementList
)
{
    // Skip over placebo statements.
    while (
        pStatementList &&
        pStatementList->Element->IsPlacebo())
    {
        pStatementList = pStatementList->NextInBlock;
    }

    return pStatementList;
}

tokens
ParseTreeHelpers::GetOperatorTokenFromOperatorAssignment
(
    ParseTree::Statement::Opcodes opcode
)
{
    tokens tk = tkNone;

    switch (opcode)
    {
        case ParseTree::Statement::AssignPlus:
            tk = tkPlus;
            break;
        case ParseTree::Statement::AssignMinus:
            tk = tkMinus;
            break;
        case ParseTree::Statement::AssignMultiply:
            tk = tkMult;
            break;
        case ParseTree::Statement::AssignDivide:
            tk = tkDiv;
            break;
        case ParseTree::Statement::AssignPower:
            tk = tkPwr;
            break;
        case ParseTree::Statement::AssignIntegralDivide:
            tk = tkIDiv;
            break;
        case ParseTree::Statement::AssignConcatenate:
            tk = tkConcat;
            break;
        case ParseTree::Statement::AssignShiftLeft:
            tk = tkShiftLeft;
            break;
        case ParseTree::Statement::AssignShiftRight:
            tk = tkShiftRight;
            break;

        default:
            break;
    }

    return tk;
}

//=============================================================================
// Could this expression be part of an identifier
//=============================================================================
bool
ParseTreeHelpers::IsExprPartOfIdentifier
(
    ParseTree::Expression *pExpression
)
{
    bool fPartOfName = false;

    if (pExpression)
    {
        switch (pExpression->Opcode)
        {
            case ParseTree::Expression::Name:
            case ParseTree::Expression::Me:
            case ParseTree::Expression::MyBase:
            case ParseTree::Expression::MyClass:
            case ParseTree::Expression::GlobalNameSpace:
            case ParseTree::Expression::BooleanLiteral:
            case ParseTree::Expression::Nothing:
                fPartOfName = true;
                break;
            default:
                break;
        }
    }

    return fPartOfName;
}

//=============================================================================
// Get the last statement of the given block (children of the block only)
//=============================================================================
ParseTree::StatementList *
ParseTreeHelpers::GetLastStatementInBlock
(
    ParseTree::BlockStatement *pBlock
)
{
    VSASSERT(pBlock, "pBlock should not be NULL!");

    ParseTree::StatementList *pLastStatement = NULL;
    if (pBlock->Children)
    {
        pLastStatement = pBlock->Children;
        while (pLastStatement &&
               pLastStatement->NextInBlock &&
               pLastStatement->NextInBlock->Element &&
               pLastStatement->NextInBlock->Element->GetParent() == pBlock)
        {
            pLastStatement = pLastStatement->NextInBlock;
        }
    }

    return pLastStatement;
}


//=============================================================================
// Get the real location of the punctuator. Returns true if the punctuator exists, false otherwise.
//=============================================================================
bool
ParseTreeHelpers::GetPunctuatorLocation
(
    const Location *pBaseLocation,
    const ParseTree::PunctuatorLocation *pPunctuator,
    Location *pRealLocation,
    int PunctuatorLength,    // if you want the location to be set to the entire punctuator, pass in the length
    bool fAllowColumn0 //= false      // allow a PunctLoc of Line=0, col = 0 to be valid
)
{
    AssertIfNull(pBaseLocation);
    AssertIfNull(pPunctuator);
    AssertIfNull(pRealLocation);

    if (pPunctuator->Line || pPunctuator->Column || fAllowColumn0 )
    {
        pRealLocation->SetLocation(
            pBaseLocation->m_lBegLine + pPunctuator->Line,
            pPunctuator->Column,
            pBaseLocation->m_lBegLine + pPunctuator->Line,
            pPunctuator->Column + PunctuatorLength
            );

        return true;
    }
    else
    {
        pRealLocation->Invalidate();
    }

    return false;
}

bool
ParseTreeHelpers::GetStringFromType
(
    ParseTree::Type *pType,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pSb);

    if (pType)
    {
        switch (pType->Opcode)
        {
            case ParseTree::Type::SyntaxError:
                return false;

            case ParseTree::Type::Boolean:
                pSb->AppendSTRING(pCompiler->TokenToString(tkBOOLEAN));
                return true;

            case ParseTree::Type::SignedByte:
                pSb->AppendSTRING(pCompiler->TokenToString(tkSBYTE));
                return true;

            case ParseTree::Type::Byte:
                pSb->AppendSTRING(pCompiler->TokenToString(tkBYTE));
                return true;

            case ParseTree::Type::Short:
                pSb->AppendSTRING(pCompiler->TokenToString(tkSHORT));
                return true;

            case ParseTree::Type::UnsignedShort:
                pSb->AppendSTRING(pCompiler->TokenToString(tkUSHORT));
                return true;

            case ParseTree::Type::Integer:
                pSb->AppendSTRING(pCompiler->TokenToString(tkINTEGER));
                return true;

            case ParseTree::Type::UnsignedInteger:
                pSb->AppendSTRING(pCompiler->TokenToString(tkUINTEGER));
                return true;

            case ParseTree::Type::Long:
                pSb->AppendSTRING(pCompiler->TokenToString(tkLONG));
                return true;

            case ParseTree::Type::UnsignedLong:
                pSb->AppendSTRING(pCompiler->TokenToString(tkULONG));
                return true;

            case ParseTree::Type::Decimal:
                pSb->AppendSTRING(pCompiler->TokenToString(tkDECIMAL));
                return true;

            case ParseTree::Type::Single:
                pSb->AppendSTRING(pCompiler->TokenToString(tkSINGLE));
                return true;

            case ParseTree::Type::Double:
                pSb->AppendSTRING(pCompiler->TokenToString(tkDOUBLE));
                return true;

            case ParseTree::Type::Date:
                pSb->AppendSTRING(pCompiler->TokenToString(tkDATE));
                return true;

            case ParseTree::Type::Char:
                pSb->AppendSTRING(pCompiler->TokenToString(tkCHAR));
                return true;

            case ParseTree::Type::String:
                pSb->AppendSTRING(pCompiler->TokenToString(tkSTRING));
                return true;

            case ParseTree::Type::Object:
                pSb->AppendSTRING(pCompiler->TokenToString(tkOBJECT));
                return true;

            case ParseTree::Type::Named:
                return 
                    GetStringFromName
                    (
                        pType->AsNamed()->TypeName,
                        IncludeGenericParameters,
                        IncludeArrayBounds,
                        pSb,
                        pCompiler
                    );

            case ParseTree::Type::ArrayWithoutSizes:
            case ParseTree::Type::ArrayWithSizes:
                return 
                    GetStringFromArrayType
                    (
                        pType->AsArray(),
                        IncludeGenericParameters,
                        IncludeArrayBounds,
                        pSb,
                        pCompiler
                    );

            case ParseTree::Type::Nullable:
            {
                bool fResult = GetStringFromType(pType->AsNullable()->ElementType, IncludeGenericParameters, IncludeArrayBounds, pSb, pCompiler);
                if (fResult)
                {
                    pSb->AppendChar(L'?');
                }
                return fResult;
            }

            default:
                VSFAIL("Invalid Opcode");
        }
    }

    return false;
}

bool
ParseTreeHelpers::GetStringFromArrayType
(
    ParseTree::ArrayType *pArrayType,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pArrayType);

    if (pArrayType)
    {
        ParseTree::Type *pTempType = pArrayType;

        while (IsArrayType(pTempType))
        {
            pTempType = pTempType->AsArray()->ElementType;
        }

        if (!pTempType || GetStringFromType(pTempType, IncludeGenericParameters, IncludeArrayBounds, pSb, pCompiler))
        {
            if (IncludeArrayBounds)
            {
                pTempType = pArrayType;

                while (IsArrayType(pTempType))
                {
                    pSb->AppendSTRING(pCompiler->TokenToString(tkLParen));

                    unsigned cRank = pTempType->AsArray()->Rank - 1;

                    while (cRank--)
                    {
                        pSb->AppendSTRING(pCompiler->TokenToString(tkComma));
                    }

                    pSb->AppendSTRING(pCompiler->TokenToString(tkRParen));


                    pTempType = pTempType->AsArray()->ElementType;
                }
            }

            return true;
        }
    }

    return false;
}


// 



bool
ParseTreeHelpers::GetStringFromIdentifierDescriptor
(
    ParseTree::IdentifierDescriptor *pIdentifier,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb
)
{
    AssertIfNull(pSb);

    if (pIdentifier &&
        !pIdentifier->IsBad)
    {
        if (pIdentifier->IsBracketed)
        {
            pSb->AppendChar(L'[');
        }

        pSb->AppendSTRING(pIdentifier->Name);

        if (pIdentifier->IsBracketed)
        {
            pSb->AppendChar(L']');
        }

        return true;
    }

    return false;
}

bool
ParseTreeHelpers::GetStringFromGenericArguments
(
    ParseTree::GenericArguments *pGenericArguments,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pSb);

    bool IsValid = true;

    if (pGenericArguments)
    {
        pSb->AppendSTRING(pCompiler->TokenToString(tkLParen));
        pSb->AppendSTRING(pCompiler->TokenToString(tkOF));
        pSb->AppendChar(UCH_SPACE);

        ParseTree::TypeList *pFirstElement = pGenericArguments->Arguments;
        ParseTree::TypeList *pTypeList = pGenericArguments->Arguments;

        while (pTypeList)
        {
            if (pTypeList != pFirstElement)
            {
                pSb->AppendSTRING(pCompiler->TokenToString(tkComma));
                pSb->AppendChar(UCH_SPACE);
            }

            if (pGenericArguments->Opcode == ParseTree::GenericArguments::WithTypes)
            {
                if (!GetStringFromType(pTypeList->Element, IncludeGenericParameters, IncludeArrayBounds, pSb, pCompiler))
                {
                    IsValid = false;
                }
            }

            pTypeList = pTypeList->Next;
        }

        pSb->AppendSTRING(pCompiler->TokenToString(tkRParen));
    }

    return IsValid;
}

bool
ParseTreeHelpers::GetStringFromGenericParameters
(
    ParseTree::GenericParameterList *pGenericParameters,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pSb);

    bool IsValid = true;

    if (pGenericParameters && IncludeGenericParameters)
    {
        pSb->AppendSTRING(pCompiler->TokenToString(tkLParen));
        pSb->AppendSTRING(pCompiler->TokenToString(tkOF));
        pSb->AppendChar(UCH_SPACE);

        ParseTree::GenericParameterList *pFirstParameter = pGenericParameters;

        while (pGenericParameters)
        {
            if (pGenericParameters != pFirstParameter)
            {
                pSb->AppendSTRING(pCompiler->TokenToString(tkComma));
                pSb->AppendChar(UCH_SPACE);
            }

            GetStringFromIdentifierDescriptor(
                &pGenericParameters->Element->Name,
                IncludeGenericParameters,
                IncludeArrayBounds,
                pSb);

            pGenericParameters = pGenericParameters->Next;
        }

        pSb->AppendSTRING(pCompiler->TokenToString(tkRParen));
    }

    return IsValid;
}

bool
ParseTreeHelpers::GetStringFromName
(
    ParseTree::Name *pName,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pSb);

    if (pName)
    {
        switch (pName->Opcode)
        {
            case ParseTree::Name::Simple:
            case ParseTree::Name::SimpleWithArguments:
                if (GetStringFromIdentifierDescriptor(&pName->AsSimple()->ID, IncludeGenericParameters, IncludeArrayBounds, pSb))
                {
                    if (IncludeGenericParameters)
                    {
                        return 
                            GetStringFromGenericArguments
                            (
                                GetGenericArguments(pName),
                                IncludeGenericParameters,
                                IncludeArrayBounds,
                                pSb,
                                pCompiler
                            );
                    }

                    return true;
                }

                return false;

            case ParseTree::Name::Qualified:
            case ParseTree::Name::QualifiedWithArguments:
                if (GetStringFromName(pName->AsQualified()->Base, IncludeGenericParameters, IncludeArrayBounds, pSb, pCompiler))
                {
                    pSb->AppendSTRING(pCompiler->TokenToString(tkDot));

                    if (GetStringFromIdentifierDescriptor(&pName->AsQualified()->Qualifier, IncludeGenericParameters, IncludeArrayBounds, pSb))
                    {
                        if (IncludeGenericParameters)
                        {
                            return 
                                GetStringFromGenericArguments
                                (
                                    GetGenericArguments(pName),
                                    IncludeGenericParameters,
                                    IncludeArrayBounds,
                                    pSb,
                                    pCompiler
                                );
                        }

                        return true;
                    }
                }

                return false;

            case ParseTree::Name::GlobalNameSpace:
                pSb->AppendString(pCompiler->TokenToString(tkGLOBAL));
                return true;

            default:
                VSFAIL("Invalid Opcode in ParseTree::Name");
        }
    }

    return false;
}

bool
ParseTreeHelpers::GetStringFromNameWithoutQualifier
(
    ParseTree::Name *pName,
    bool IncludeGenericParameters,
    bool IncludeArrayBounds,
    StringBuffer *pSb,
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pSb);

    if (pName)
    {
        switch (pName->Opcode)
        {
            case ParseTree::Name::Simple:
            case ParseTree::Name::SimpleWithArguments:
            case ParseTree::Name::GlobalNameSpace:
                return GetStringFromName(pName, IncludeGenericParameters, IncludeArrayBounds, pSb, pCompiler);

            case ParseTree::Name::Qualified:
            case ParseTree::Name::QualifiedWithArguments:
                if (GetStringFromIdentifierDescriptor(&pName->AsQualified()->Qualifier, IncludeGenericParameters, IncludeArrayBounds, pSb))
                {
                    if (IncludeGenericParameters)
                    {
                        return 
                            GetStringFromGenericArguments
                            (
                                GetGenericArguments(pName),
                                IncludeGenericParameters,
                                IncludeArrayBounds,
                                pSb,
                                pCompiler
                            );
                    }

                    return true;
                }

                return false;

            default:
                VSFAIL("Invalid Opcode in ParseTree::Name");
        }
    }

    return false;
}

// GetFullyQualifiedName:
//
//    Will fill the string buffer with the fully qualified name of the given element.
void
ParseTreeHelpers::GetFullyQualifiedName
(
    CompilerProject *pCompilerProject,          // Project for the default namespace.
    ParseTree::Statement *pStatement,           // ParseTree statement to get the name of.
    bool IncludeGenericParameters,       // Include generic parameters.
    StringBuffer *pSbName,                       // Will contain the fully qualified name on exit.
    Compiler * pCompiler
)
{
    ThrowIfNull(pCompiler);
    AssertIfNull(pCompilerProject);
    AssertIfNull(pStatement);
    AssertIfNull(pSbName);

    if (pStatement && pStatement->Opcode != ParseTree::Statement::File)
    {

        AssertIfNull(pStatement->GetParent());

        // Recursively collect all container names
        GetFullyQualifiedName
        (
            pCompilerProject,
            pStatement->GetParent(),
            IncludeGenericParameters,
            pSbName,
            pCompiler
        );

        switch (pStatement->Opcode)
        {
            case ParseTree::Statement::Structure:
            case ParseTree::Statement::Interface:
            case ParseTree::Statement::Class:
            case ParseTree::Statement::Module:
            case ParseTree::Statement::Enum:
                if (pSbName->GetStringLength() > 0)
                {
                    pSbName->AppendChar(L'.');
                }

                GetStringFromIdentifierDescriptor(
                    &pStatement->AsType()->Name,
                    IncludeGenericParameters,
                    false,
                    pSbName);

                GetStringFromGenericParameters
                (
                    pStatement->AsType()->GenericParameters,
                    IncludeGenericParameters,
                    false,
                    pSbName,
                    pCompiler
                );

                break;

            case ParseTree::Statement::Namespace:
                if (pSbName->GetStringLength() > 0)
                {
                    pSbName->AppendChar(L'.');
                }

                GetStringFromName
                (
                    pStatement->AsNamespace()->Name,
                    IncludeGenericParameters,
                    false,
                    pSbName,
                    pCompiler
                );
                break;

            case ParseTree::Statement::BlockEventDeclaration:
                pStatement = pStatement->AsBlockEventDeclaration()->EventSignature;
                __fallthrough; //fall through
            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:
            case ParseTree::Statement::OperatorDeclaration:
            case ParseTree::Statement::DelegateProcedureDeclaration:
            case ParseTree::Statement::DelegateFunctionDeclaration:
            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::ForeignProcedureDeclaration:
            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignFunctionNone:
                if (pSbName->GetStringLength() > 0)
                {
                    pSbName->AppendChar(L'.');
                }

                GetStringFromIdentifierDescriptor(
                    &pStatement->AsMethodSignature()->Name,
                    IncludeGenericParameters,
                    false,
                    pSbName);

                GetStringFromGenericParameters
                (
                    GetGenericParameters(pStatement),
                    IncludeGenericParameters,
                    false,
                    pSbName,
                    pCompiler
                );
                break;

            case ParseTree::Statement::PropertyGet:
            case ParseTree::Statement::PropertySet:
                // Do nothing. The recursive call to parent got the information we required.
                break;

            case ParseTree::Statement::Property:
            case ParseTree::Statement::AutoProperty:
                if (pSbName->GetStringLength() > 0)
                {
                    pSbName->AppendChar(L'.');
                }

                GetStringFromIdentifierDescriptor(
                    &pStatement->AsProperty()->Name,
                    IncludeGenericParameters,
                    false,
                    pSbName);
                break;

            case ParseTree::Statement::AddHandlerDeclaration:
            case ParseTree::Statement::RemoveHandlerDeclaration:
            case ParseTree::Statement::RaiseEventDeclaration:
                // Do nothing. The recursive call to parent got the information we required.
                break;


            case ParseTree::Statement::Enumerator:
            case ParseTree::Statement::EnumeratorWithValue:
                if (pSbName->GetStringLength() > 0)
                {
                    pSbName->AppendChar(L'.');
                }

                GetStringFromIdentifierDescriptor(
                    &pStatement->AsEnumerator()->Name,
                    IncludeGenericParameters,
                    false,
                    pSbName);
                break;
        }
    }
    else if (pCompilerProject && pCompilerProject->GetDefaultNamespace())
    {
        pSbName->AppendSTRING(pCompilerProject->GetDefaultNamespace());
    }
}


ParseTree::Statement *
ParseTreeHelpers::GetStatementOnOrAfterLocation
(
    ParseTree::BlockStatement *pCurrentBlock, // [In] Can be NULL.
    Location *pLoc //[In] Cannot be NULL
)
{
    AssertIfNull(pLoc);
    AssertIfFalse(pLoc->m_lBegLine == pLoc->m_lEndLine);
    AssertIfFalse(pLoc->m_lBegColumn == pLoc->m_lEndColumn);

    if (pCurrentBlock)
    {
        ParseTree::StatementList *pStatementList = pCurrentBlock->Children;

        while (pStatementList)
        {
            ParseTree::Statement *pCurrentStatement = pStatementList->Element;

            if (pCurrentStatement)
            {
                // If the current statement is on or after the given location, return it.
                if (pLoc->StartsOnOrBefore(&pCurrentStatement->TextSpan))
                {
                    // Return the actual comment statement, not the block.
                    if (pCurrentStatement->Opcode == ParseTree::Statement::CommentBlock)
                    {
                        return pCurrentStatement->AsBlock()->Children->Element;
                    }

                    return pCurrentStatement;
                }

                switch (pCurrentStatement->Opcode)
                {
                    // Check for blocks.
                    case ParseTree::Statement::Structure:
                    case ParseTree::Statement::Interface:
                    case ParseTree::Statement::Class:
                    case ParseTree::Statement::Module:
                    case ParseTree::Statement::Enum:
                    case ParseTree::Statement::Namespace:
                    case ParseTree::Statement::Property:
                    case ParseTree::Statement::AutoProperty:
                    case ParseTree::Statement::BlockEventDeclaration:
                    {
                        // If the location is within the bounds of this block, recurse.
                        if (pLoc->StartsAfter(&pCurrentStatement->TextSpan) &&
                            (pStatementList->NextInBlock == NULL || pLoc->EndsBefore(&pStatementList->NextInBlock->Element->TextSpan)))
                        {
                            return GetStatementOnOrAfterLocation(pCurrentStatement->AsBlock(), pLoc);
                        }

                        break;
                    }

                    case ParseTree::Statement::CommentBlock:
                    {
                        if (pLoc->StartsAfter(&pCurrentStatement->TextSpan) &&
                            (pStatementList->NextInBlock == NULL ||pCurrentStatement->AsCommentBlock()->BodyTextSpan.ContainsInclusive(pLoc)))
                        {
                            return GetStatementOnOrAfterLocation(pCurrentStatement->AsBlock(), pLoc);
                        }

                        break;
                    }
                }
            }

            pStatementList = pStatementList->NextInBlock;
        }
    }

    return NULL;
}

ParseTree::Statement *
ParseTreeHelpers::GetStatementOnOrBeforeLocation
(
    ParseTree::BlockStatement *pCurrentBlock, // [In] Can be NULL.
    Location *pLoc //[In] Cannot be NULL
)
{
    AssertIfNull(pLoc);
    AssertIfFalse(pLoc->m_lBegLine == pLoc->m_lEndLine);
    AssertIfFalse(pLoc->m_lBegColumn == pLoc->m_lEndColumn);

    if (pCurrentBlock)
    {
        ParseTree::StatementList *pStatementList = pCurrentBlock->Children;
        ParseTree::Statement *pPreviousStatement = NULL;

        while (pStatementList)
        {
            ParseTree::Statement *pCurrentStatement = pStatementList->Element;

            if (pCurrentStatement)
            {
                // If the current statement contains the given location, return it.
                if (pCurrentStatement->TextSpan.ContainsInclusive(pLoc))
                {
                    pPreviousStatement = pCurrentStatement;
                    break;
                }
                // If the current statement is after the given location, return the previous statement.
                else if (pLoc->StartsBefore(&pCurrentStatement->TextSpan))
                {
                    break;
                }

                switch (pCurrentStatement->Opcode)
                {
                    // Check for blocks.
                    case ParseTree::Statement::Structure:
                    case ParseTree::Statement::Interface:
                    case ParseTree::Statement::Class:
                    case ParseTree::Statement::Module:
                    case ParseTree::Statement::Enum:
                    case ParseTree::Statement::Namespace:
                    case ParseTree::Statement::Property:
                    case ParseTree::Statement::AutoProperty:
                    case ParseTree::Statement::BlockEventDeclaration:
                    {
                        // If the location is within the bounds of this block, recurse.
                        if (pLoc->StartsAfter(&pCurrentStatement->TextSpan) &&
                            (pStatementList->NextInBlock == NULL || pLoc->EndsBefore(&pStatementList->NextInBlock->Element->TextSpan)))
                        {
                            return GetStatementOnOrBeforeLocation(pCurrentStatement->AsBlock(), pLoc);
                        }

                        break;
                    }

                    case ParseTree::Statement::CommentBlock:
                    {
                        if (pLoc->StartsAfter(&pCurrentStatement->TextSpan) &&
                            (pStatementList->NextInBlock == NULL ||pCurrentStatement->AsCommentBlock()->BodyTextSpan.ContainsInclusive(pLoc)))
                        {
                            return GetStatementOnOrBeforeLocation(pCurrentStatement->AsBlock(), pLoc);
                        }

                        break;
                    }
                }
            }

            pStatementList = pStatementList->NextInBlock;
            pPreviousStatement = pCurrentStatement;
        }

        if (pPreviousStatement && pPreviousStatement->Opcode == ParseTree::Statement::CommentBlock)
        {
            // Return the actual comment statement, not the block.
            return pPreviousStatement->AsBlock()->Children->Element;
        }

        return pPreviousStatement;
    }

    return NULL;
}


STRING *
ParseTreeHelpers::ExtractDeclarationName
(
    Compiler *pCompiler,
    ParseTree::Statement *pStatement,
    Location *pLoc,
    Location *pLocFoundName,
    bool fIncludeBrackets
)
{
    STRING *pstrName = NULL;

    if (pLoc && pStatement && !pStatement->HasSyntaxError)
    {
        ParseTree::ParameterList *pParameters = ParseTreeHelpers::GetParameters(pStatement);
        if (pParameters)
        {
            pstrName = ExtractDeclarationNameFromParams(pCompiler, pParameters, pLoc, pLocFoundName, fIncludeBrackets);
        }

        if (!pstrName)
        {
            switch (pStatement->Opcode)
            {
                case ParseTree::Statement::Imports:
                    {
                        ParseTree::ImportDirectiveList *pImportsList = pStatement->AsImports()->Imports;

                        while (pImportsList)
                        {
                            if (pImportsList->Element->Mode == ParseTree::ImportDirective::Alias &&
                                pImportsList->Element->AsAlias()->Alias.TextSpan.ContainsExtended(pLoc))
                            {
                                pstrName = ExtractDeclarationFromID(pCompiler, &pImportsList->Element->AsAlias()->Alias, pLoc, pLocFoundName, fIncludeBrackets);
                                break;
                            }

                            pImportsList = pImportsList->Next;
                        }
                    }
                    break;

                case ParseTree::Statement::Enum:
                case ParseTree::Statement::Structure:
                case ParseTree::Statement::Interface:
                case ParseTree::Statement::Class:
                case ParseTree::Statement::Module:
                    if (pStatement->AsType()->Name.TextSpan.ContainsExtended(pLoc))
                    {
                        pstrName = ExtractDeclarationFromID(pCompiler, &pStatement->AsType()->Name, pLoc, pLocFoundName, fIncludeBrackets);
                    }
                    else
                    {
                        pstrName = ExtractDeclarationNameFromGenericParams(
                            pCompiler,
                            pStatement->AsType()->GenericParameters,
                            pLoc,
                            pLocFoundName,
                            fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::Namespace:
                    if (pStatement->AsNamespace()->Name &&
                        pStatement->AsNamespace()->Name->TextSpan.ContainsExtended(pLoc) &&
                        pStatement->AsNamespace()->Name->Opcode == ParseTree::Name::Simple)
                    {
                        pstrName = ExtractDeclarationFromID(
                            pCompiler,
                            &pStatement->AsNamespace()->Name->AsSimple()->ID,
                            pLoc,
                            pLocFoundName,
                            fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::ProcedureDeclaration:
                case ParseTree::Statement::FunctionDeclaration:
                case ParseTree::Statement::DelegateProcedureDeclaration:
                case ParseTree::Statement::DelegateFunctionDeclaration:
                    pstrName = ExtractDeclarationNameFromGenericParams(
                            pCompiler,
                            pStatement->AsMethodDeclaration()->GenericParameters,
                            pLoc,
                            pLocFoundName,
                            fIncludeBrackets);

                    if (pstrName)
                    {
                        break;
                    }

                    // fall through
                    __fallthrough;
                case ParseTree::Statement::ForeignProcedureDeclaration:
                case ParseTree::Statement::ForeignFunctionDeclaration:
                case ParseTree::Statement::EventDeclaration:
                case ParseTree::Statement::BlockEventDeclaration:
                    {
                        ParseTree::MethodSignatureStatement *pSignature = NULL;
                        if (pStatement->Opcode == ParseTree::Statement::BlockEventDeclaration)
                        {
                            pSignature = pStatement->AsBlockEventDeclaration()->EventSignature;
                        }
                        else
                        {
                            pSignature = pStatement->AsMethodSignature();
                        }

                        if (pSignature->Name.TextSpan.ContainsExtended(pLoc))
                        {
                            pstrName = ExtractDeclarationFromID(pCompiler, &pSignature->Name, pLoc, pLocFoundName, fIncludeBrackets);
                        }
                    }
                    break;

                case ParseTree::Statement::Property:
                case ParseTree::Statement::AutoProperty:
                    if (pStatement->AsProperty()->Name.TextSpan.ContainsExtended(pLoc))
                    {
                        pstrName = ExtractDeclarationFromID(pCompiler, &pStatement->AsProperty()->Name, pLoc, pLocFoundName, fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::Enumerator:
                case ParseTree::Statement::EnumeratorWithValue:
                    if (pStatement->AsEnumerator()->Name.TextSpan.ContainsExtended(pLoc))
                    {
                        pstrName = ExtractDeclarationFromID(pCompiler, &pStatement->AsEnumerator()->Name, pLoc, pLocFoundName, fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::VariableDeclaration:
                    {
                        // For a variable declaration, find the relevant declarator
                        ParseTree::VariableDeclarationList *pDeclarations = pStatement->AsVariableDeclaration()->Declarations;

                        while (pDeclarations &&
                            pDeclarations->Element &&
                            !pDeclarations->Element->TextSpan.ContainsExtended(pLoc))
                        {
                            pDeclarations = pDeclarations->Next;
                        }

                        ParseTree::DeclaratorList *pDeclarators =
                            (pDeclarations && pDeclarations->Element) ?
                                pDeclarations->Element->Variables :
                                NULL;

                        while (pDeclarators &&
                               pDeclarators->Element &&
                               !pDeclarators->Element->TextSpan.ContainsExtended(pLoc))
                        {
                            pDeclarators = pDeclarators->Next;
                        }

                        if (pDeclarators && pDeclarators->Element && pDeclarators->Element->Name.TextSpan.ContainsExtended(pLoc))
                        {
                            pstrName = ExtractDeclarationFromID(pCompiler, &pDeclarators->Element->Name, pLoc, pLocFoundName, fIncludeBrackets);
                        }
                    }
                    break;

                case ParseTree::Statement::ForFromTo:
                case ParseTree::Statement::ForEachIn:
                    if (pStatement->AsFor()->ControlVariableDeclaration &&
                        pStatement->AsFor()->ControlVariableDeclaration->TextSpan.ContainsExtended(pLoc))
                    {
                        pstrName = ExtractDeclarationName(
                                pCompiler,
                                pStatement->AsFor()->ControlVariableDeclaration,
                                pLoc,
                                pLocFoundName,
                                fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::Using:
                    if (pStatement->AsUsing()->ControlVariableDeclaration &&
                        pStatement->AsUsing()->ControlVariableDeclaration->TextSpan.ContainsExtended(pLoc))
                    {
                        pstrName = ExtractDeclarationName(
                                pCompiler,
                                pStatement->AsUsing()->ControlVariableDeclaration,
                                pLoc,
                                pLocFoundName,
                                fIncludeBrackets);
                    }
                    break;

                case ParseTree::Statement::Catch:
                    if (pStatement->AsCatch()->Name.Name && pStatement->AsCatch()->Type)
                    {
                        pstrName = ExtractDeclarationFromID(pCompiler, &pStatement->AsCatch()->Name, pLoc, pLocFoundName, fIncludeBrackets);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    return pstrName;
}

STRING *
ParseTreeHelpers::ExtractDeclarationNameFromGenericParams
(
    Compiler *pCompiler,
    ParseTree::GenericParameterList *pList,
    Location *pLoc,
    Location *pLocFoundName,
    bool fIncludeBrackets
)
{
    if (pLoc && pList && pList->TextSpan.ContainsExtended(pLoc))
    {
        while (pList &&
               pList->Element &&
               !pList->Element->Name.TextSpan.ContainsExtended(pLoc))
        {
            pList = pList->Next;
        }

        if (pList && pList->Element)
        {
            return ExtractDeclarationFromID(pCompiler, &pList->Element->Name, pLoc, pLocFoundName, fIncludeBrackets);
        }
    }

    return NULL;
}

STRING *
ParseTreeHelpers::ExtractDeclarationNameFromParams
(
    Compiler *pCompiler,
    ParseTree::ParameterList *pList,
    Location *pLoc,
    Location *pLocFoundName,
    bool fIncludeBrackets
)
{
    if (pLoc && pList && pList->TextSpan.ContainsExtended(pLoc))
    {
        while (pList &&
               pList->Element &&
               (!pList->Element->Name ||
                !pList->Element->Name->Name.TextSpan.ContainsExtended(pLoc)))
        {
            pList = pList->Next;
        }

        if (pList && pList->Element)
        {
            return ExtractDeclarationFromID(pCompiler, &pList->Element->Name->Name, pLoc, pLocFoundName, fIncludeBrackets);
        }
    }

    return NULL;
}

STRING *
ParseTreeHelpers::ExtractDeclarationFromID
(
    Compiler *pCompiler,
    ParseTree::IdentifierDescriptor *pID,
    Location *pLoc,
    Location *pLocFoundName,
    bool fIncludeBrackets
)
{
    if (pLoc && pID && pID->TextSpan.ContainsExtended(pLoc))
    {
        if (pLocFoundName)
        {
            *pLocFoundName = pID->TextSpan;

            // Remove the type character from the location
            if (pID->TypeCharacter != chType_NONE)
            {
                pLocFoundName->m_lEndColumn = VBMath::Convert<long>(max(0, pLocFoundName->m_lEndColumn - wcslen(WszTypeChar(pID->TypeCharacter))));
            }
        }

        if (fIncludeBrackets && pID->IsBracketed && pCompiler)
        {
            return pCompiler->ConcatStrings(L"[", pID->Name, L"]");
        }
        else
        {
            return pID->Name;
        }
    }

    return NULL;
}

//=============================================================================
// IsInferredDeclaration
// Is Option Infer On and does the given statement represent an inferred type
// i.e. no explicit type in the declaration.
// Note: Using statement looks like a variable declaration at this point.
//=============================================================================
bool
ParseTreeHelpers::IsInferredDeclaration
(
    ParseTree::Statement *pStatement,
    SourceFile           *pFile,
    Location             *pLoc
)
{
    if (!pStatement ||
        !pFile ||
        !(pFile->GetOptionFlags() & OPTION_OptionInfer))
    {
        return false;
    }

    if (pStatement->Opcode == ParseTree::Statement::VariableDeclaration)
    {
        // For a variable declaration, find the relevant declarator
        ParseTree::VariableDeclarationList *pDeclarations = pStatement->AsVariableDeclaration()->Declarations;

        while (pDeclarations &&
            pDeclarations->Element &&
            !pDeclarations->Element->TextSpan.ContainsExtended(pLoc))
        {
            pDeclarations = pDeclarations->Next;
        }

        //Initialized Variables with no explicit type are considered to be inferred.
        if (pDeclarations &&
            pDeclarations->Element &&
            pDeclarations->Element->Opcode == ParseTree::VariableDeclaration::WithInitializer &&
            !pDeclarations->Element->Type)
        {
            return true;
        }
    }
    else if (pStatement->Opcode == ParseTree::Statement::ForFromTo ||
             pStatement->Opcode == ParseTree::Statement::ForEachIn)
    {
        ParseTree::ForStatement *pForStatement = pStatement->AsFor();
        if (pForStatement && !pForStatement->ControlVariableDeclaration)
        {
            return true;
        }
    }

    return false;
}

bool
ParseTreeHelpers::IsPrefixDotExpression
(
    ParseTree::Expression *pExpression
)
{
    ParseTree::Expression *pLeftMostExpression = pExpression;

    while (pExpression)
    {
        pLeftMostExpression = pExpression;

        switch (pExpression->Opcode)
        {
            case ParseTree::Expression::DotQualified:
            case ParseTree::Expression::BangQualified:
            case ParseTree::Expression::XmlElementsQualified:
            case ParseTree::Expression::XmlAttributeQualified:
            case ParseTree::Expression::XmlDescendantsQualified:
                pExpression = pExpression->AsQualified()->Base;
                break;

            case ParseTree::Expression::GenericQualified:
                pExpression = pExpression->AsGenericQualified()->Base;
                break;

            case ParseTree::Expression::Parenthesized:
                pExpression = pExpression->AsUnary()->Operand;
                break;

            case ParseTree::Expression::CallOrIndex:
                pExpression = pExpression->AsCallOrIndex()->Target;
                break;

            default:
                pExpression = NULL;
                break;
        }
    }

    return pLeftMostExpression &&
           pLeftMostExpression->Opcode == ParseTree::Expression::DotQualified;
}

//============================================================================
// Get the attributes or specifiers given statement if any
//============================================================================
bool
ParseTreeHelpers::GetAttributesAndSpecifiers
(
    ParseTree::Statement *pStatement,
    ParseTree::AttributeSpecifierList **ppAttributes,
    Location *plocBeforeAttributes,
    Location *plocAfterAttributes,
    ParseTree::SpecifierList **ppSpecifiers,
    Location *plocBeforeSpecifiers,
    Location *plocAfterSpecifiers,
    Location *plocAfterAttributesAndSpecifiers
)
{
    ParseTree::SpecifierList *pSpecifiers = NULL;
    ParseTree::AttributeSpecifierList *pAttributes = NULL;
    Location locAfter;
    locAfter.Invalidate();

    bool fCanHaveAttributesOrSpecifiers = false;
    if (pStatement)
    {
        if (IsMethodSignature(pStatement) || pStatement->Opcode == ParseTree::Statement::BlockEventDeclaration)
        {
            ParseTree::MethodSignatureStatement *pMethodSignature =
                pStatement->Opcode == ParseTree::Statement::BlockEventDeclaration ?
                    pStatement->AsBlockEventDeclaration()->EventSignature :
                    pStatement->AsMethodSignature();

            pSpecifiers = pMethodSignature->Specifiers;
            pAttributes = pMethodSignature->Attributes;

            GetLocationAfterAttributesAndSpecifiers(pStatement, &locAfter);
            fCanHaveAttributesOrSpecifiers = true;
        }
        else if (pStatement->Opcode == ParseTree::Statement::Property ||
                 pStatement->Opcode == ParseTree::Statement::AutoProperty)
        {
            ParseTree::PropertyStatement *pProperty = pStatement->AsProperty();

            pSpecifiers = pProperty->Specifiers;
            pAttributes = pProperty->Attributes;
            locAfter.SetLocation(pProperty->TextSpan.m_lBegLine + pProperty->Property.Line,
                                 pProperty->Property.Column);
            fCanHaveAttributesOrSpecifiers = true;
        }
        else if (IsTypeStatement(pStatement))
        {
            pSpecifiers = pStatement->AsType()->Specifiers;
            pAttributes = pStatement->AsType()->Attributes;
            locAfter.SetLocation(pStatement->AsType()->TextSpan.m_lBegLine + pStatement->AsType()->TypeKeyword.Line,
                                 pStatement->AsType()->TypeKeyword.Column);
            fCanHaveAttributesOrSpecifiers = true;
        }
        else if (pStatement->Opcode == ParseTree::Statement::VariableDeclaration)
        {
            ParseTree::VariableDeclarationStatement *pVariableDeclaration = pStatement->AsVariableDeclaration();

            pSpecifiers = pVariableDeclaration->Specifiers;
            pAttributes = pVariableDeclaration->Attributes;

            if (pSpecifiers)
            {
                locAfter.SetLocation(pSpecifiers->TextSpan.m_lEndLine, pSpecifiers->TextSpan.m_lEndColumn + 1);
            }
            else if (pAttributes)
            {
                locAfter.SetLocation(pAttributes->TextSpan.m_lEndLine, pAttributes->TextSpan.m_lEndColumn + 1);
            }

            if (pVariableDeclaration->Declarations &&
                pVariableDeclaration->Declarations->TextSpan.StartsAfter(&locAfter))
            {
                locAfter.SetLocation(pVariableDeclaration->Declarations->TextSpan.m_lBegLine,
                                     pVariableDeclaration->Declarations->TextSpan.m_lBegColumn);
            }

            fCanHaveAttributesOrSpecifiers = true;
        }
    }

    if (fCanHaveAttributesOrSpecifiers)
    {
        if (ppAttributes)
        {
            *ppAttributes = pAttributes;
        }

        if (plocBeforeAttributes)
        {
            // There is no location before attributes
            plocBeforeAttributes->Invalidate();
        }

        if (plocAfterAttributes)
        {
            // A specifier list may be after the attribute list
            if (pSpecifiers)
            {
                plocAfterAttributes->SetLocation(pSpecifiers->TextSpan.m_lBegLine,
                                                 pSpecifiers->TextSpan.m_lBegColumn);
            }
            else
            {
                *plocAfterAttributes = locAfter;
            }
        }

        if (ppSpecifiers)
        {
            *ppSpecifiers = pSpecifiers;
        }

        if (plocBeforeSpecifiers)
        {
            // An attribute list may lie before the specifier list
            if (pAttributes)
            {
                plocBeforeSpecifiers->SetLocation(pAttributes->TextSpan.m_lEndLine,
                                                  pAttributes->TextSpan.m_lEndColumn);
            }
            else
            {
                plocBeforeSpecifiers->Invalidate();
            }
        }

#pragma warning (push)
#pragma warning (disable:6001) // Location is initialized above
        if (plocAfterSpecifiers)
        {
            *plocAfterSpecifiers = locAfter;
        }

        if (plocAfterAttributesAndSpecifiers)
        {
            *plocAfterAttributesAndSpecifiers = locAfter;
        }
#pragma warning (pop)
    }

    return fCanHaveAttributesOrSpecifiers;
}

//============================================================================
// Get the specifiers for a given statement if any
//============================================================================
bool
ParseTreeHelpers::GetSpecifiers
(
    ParseTree::Statement *pStatement,                       // [in]  Can be NULL
    DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,    // [out] Receives the specifier list in array form, can be NULL
    Location *plocBefore,                                   // [out] Receives the location before the specifier list, can be NULL
    Location *plocAfter                                     // [out] Receives the location after the specifier list, can be NULL
)
{
    ParseTree::SpecifierList *pSpecifiers = NULL;
    if (GetAttributesAndSpecifiers(pStatement, NULL, NULL, NULL, &pSpecifiers,  plocBefore, plocAfter))
    {
        if (pdaSpecifiers)
        {
            while (pSpecifiers && pSpecifiers->Element)
            {
                pdaSpecifiers->AddElement(pSpecifiers->Element);
                pSpecifiers = pSpecifiers->Next;
            }
        }

        return true;
    }

    return false;
}

//============================================================================
// Does the specifier list have the given specifier?
// For protected friend, multiple specifiers can be returned.
//============================================================================
bool
ParseTreeHelpers::HasSpecifier
(
    DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,    // [in]  Specifier array, cannot be NULL
    ParseTree::Specifier::Specifiers Opcode,                // [in]  Specifier to look for
    ULONG *pulFoundSpecifier1,                              // [out] Receives the first specifier index, can be NULL
    ULONG *pulFoundSpecifier2                               // [out] Receives the second specifier index, can be NULL
)
{
    VSASSERT(pdaSpecifiers, "Argument pdaSpecifiers cannot be NULL!");

    bool FoundSpecifier = false;

    if (Opcode == ParseTree::Specifier::ProtectedFriend)
    {
        FoundSpecifier = GetFirstMatchingSpecifier(pdaSpecifiers, ParseTree::Specifier::Protected, pulFoundSpecifier1) &&
                         GetFirstMatchingSpecifier(pdaSpecifiers, ParseTree::Specifier::Friend, pulFoundSpecifier2);

        if (FoundSpecifier && pulFoundSpecifier1 && pulFoundSpecifier2 && *pulFoundSpecifier2 < *pulFoundSpecifier1)
        {
            ULONG ulTemp = *pulFoundSpecifier2;
            *pulFoundSpecifier2 = *pulFoundSpecifier1;
            *pulFoundSpecifier1 = ulTemp;
        }
    }
    else
    {
        FoundSpecifier = GetFirstMatchingSpecifier(pdaSpecifiers, Opcode, pulFoundSpecifier1);
    }

    return FoundSpecifier;
}

//============================================================================
// This returns the index of the first specifier that matches the opcode
//============================================================================
bool
ParseTreeHelpers::GetFirstMatchingSpecifier
(
    DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,    // [in]  Specifier array, cannot be NULL
    ParseTree::Specifier::Specifiers Opcode,                // [in]  Specifier(s) to look for
    ULONG *pulFoundSpecifier                                // [out] Receives the index of the found specifier, can be NULL
)
{
    VSASSERT(pdaSpecifiers, "Argument pdaSpecifiers cannot be NULL!");

    bool FoundSpecifier = false;

    for (ULONG i = 0; i < pdaSpecifiers->Count(); i++)
    {
        if (pdaSpecifiers->Element(i)->Opcode & Opcode)
        {
            FoundSpecifier = true;
            if (pulFoundSpecifier)
            {
                *pulFoundSpecifier = i;
            }

            break;
        }
    }

    return FoundSpecifier;
}

//============================================================================
// This returns the indices of all specifiers that matches the opcode
//============================================================================
bool
ParseTreeHelpers::GetAllMatchingSpecifiers
(
    DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,    // [in]  Specifier array, cannot be NULL
    ParseTree::Specifier::Specifiers Opcode,                // [in]  Specifier(s) to look for
    DynamicArray<ULONG> *pdaFoundSpecifiers                 // [out] Receives the indices of the found specifiers, can be NULL
)
{
    VSASSERT(pdaSpecifiers, "Argument pdaSpecifiers cannot be NULL!");

    bool FoundSpecifier = false;

    for (ULONG i = 0; i < pdaSpecifiers->Count(); i++)
    {
        if (pdaSpecifiers->Element(i)->Opcode & Opcode)
        {
            FoundSpecifier = true;
            if (pdaFoundSpecifiers)
            {
                pdaFoundSpecifiers->AddElement(i);
            }
        }
    }

    return FoundSpecifier;
}

void
ParseTreeHelpers::GetLocationOfObjectInitializer
(
    ParseTree::VariableDeclaration *pVariableDeclaration,
    Location *pLoc
)
{
    AssertIfNull(pVariableDeclaration);
    AssertIfNull(pVariableDeclaration->Type);
    AssertIfFalse(pVariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew);
    AssertIfNull(pVariableDeclaration->AsNew()->ObjectInitializer);
    AssertIfNull(pVariableDeclaration->AsNew()->ObjectInitializer->BracedInitializerList);
    AssertIfNull(pLoc);

    ParseTree::NewVariableDeclaration *pNewVariableDeclaration = pVariableDeclaration->AsNew();

    pLoc->m_lBegLine = pNewVariableDeclaration->TextSpan.m_lBegLine + pNewVariableDeclaration->New.Line;
    pLoc->m_lBegColumn = pNewVariableDeclaration->New.Column;
    pLoc->m_lEndLine = pNewVariableDeclaration->ObjectInitializer->BracedInitializerList->TextSpan.m_lEndLine;
    pLoc->m_lEndColumn = pNewVariableDeclaration->ObjectInitializer->BracedInitializerList->TextSpan.m_lEndColumn;
}

void
ParseTreeHelpers::GetLocationOfObjectInitializer
(
    _In_ ParseTree::AutoPropertyStatement *pAutoPropertyStatement,
    _Out_ Location *pLoc
)
{
    AssertIfNull(pAutoPropertyStatement);
    AssertIfNull(pLoc);

    ParseTree::AutoPropertyInitialization *pDeclaration = 
        pAutoPropertyStatement->pAutoPropertyDeclaration;

    AssertIfNull(pDeclaration);
    AssertIfNull(pAutoPropertyStatement->PropertyType);
    AssertIfFalse(pDeclaration->Opcode == ParseTree::AutoPropertyInitialization::WithNew);
    AssertIfNull(pDeclaration->AsNew()->ObjectInitializer);
    AssertIfNull(pDeclaration->AsNew()->ObjectInitializer->BracedInitializerList);

    ParseTree::NewAutoPropertyDeclaration *pNewDeclaration = pDeclaration->AsNew();

    pLoc->m_lBegLine = pNewDeclaration->TextSpan.m_lBegLine + pNewDeclaration->New.Line;
    pLoc->m_lBegColumn = pNewDeclaration->New.Column;

    pLoc->m_lEndLine = 
        pNewDeclaration->ObjectInitializer->BracedInitializerList->TextSpan.m_lEndLine;

    pLoc->m_lEndColumn = 
      pNewDeclaration->ObjectInitializer->BracedInitializerList->TextSpan.m_lEndColumn;
}

//===============================================================
// GetRegionParent
// Returns the first non Region parent of the given Region statement
// Returns unmodified input if input is not a Region or EndRegion
//===============================================================
ParseTree::Statement *ParseTreeHelpers::GetRegionParent(ParseTree::Statement *pStatement)
{
    while (pStatement && (pStatement->Opcode == ParseTree::Statement::Region ||
                            pStatement->Opcode == ParseTree::Statement::EndRegion))
    {
            pStatement = pStatement->GetParent();
    }

    return pStatement;
}

ParseTree::Initializer *
ParseTreeHelpers::DigThroughDeferredInitializer
(
    ParseTree::Initializer *pInitializer
)
{
    while (pInitializer && pInitializer->Opcode == ParseTree::Initializer::Deferred)
    {
        pInitializer = pInitializer->AsDeferred()->Value;
    }

    return pInitializer;
}

ParseTree::Expression *
ParseTreeHelpers::DigThroughDeferredExpression
(
    ParseTree::Expression *pExpression
)
{
    while (pExpression && pExpression->Opcode == ParseTree::Expression::Deferred)
    {
        pExpression = pExpression->AsDeferred()->Value;
    }

    return pExpression;
}


bool
ParseTreeHelpers::CanBePartialMethodImplementation
(
    ParseTree::Statement *pStatement
)
{
    if (!pStatement || pStatement->Opcode != ParseTree::Statement::ProcedureDeclaration)
        return false;

    ParseTree::MethodDefinitionStatement *pMethod = pStatement->AsMethodDefinition();
    return
        !pMethod->Implements && // Implements is not allowed on partial methods.
        !pMethod->ReturnType && // Must be a Sub.
        (
            !pMethod->Specifiers ||
            !pMethod->Specifiers->HasSpecifier(
                ParseTree::Specifier::OverrideModifiers | // Overriding is not allowed.
                ParseTree::Specifier::Overrides | // Overriding is not allowed.
                ParseTree::Specifier::Partial // Implementation must have no "Partial" keyword.
            )
        );
}

bool
ParseTreeHelpers::IsLocationAfterFunctionOrSubKeywordInLambda
(
    Location *pLocation,
    ParseTree::LambdaExpression *pLambdaExpression
)
{
    AssertIfNull(pLocation);
    AssertIfNull(pLambdaExpression);

    if (HasPunctuator(pLambdaExpression->FirstPunctuator))
    {
        Location locFunction;

        // Include the right edge
        locFunction.SetLocation(pLambdaExpression->TextSpan.m_lBegLine,
                                &pLambdaExpression->FirstPunctuator,
                                (pLambdaExpression->MethodFlags & DECLF_Function) ? 
                                    g_tkKwdNameLengths[tkFUNCTION] :
                                    g_tkKwdNameLengths[tkSUB]);

        return locFunction.EndsBefore(pLocation);
    }

    return false;
}

bool
ParseTreeHelpers::IsLocationAfterLeftParenInLambda
(
    Location *pLocation,
    ParseTree::LambdaExpression *pLambdaExpression
)
{
    AssertIfNull(pLocation);
    AssertIfNull(pLambdaExpression);

    if (HasPunctuator(pLambdaExpression->LeftParen))
    {
        Location locLParen;
        locLParen.SetLocation(pLambdaExpression->TextSpan.m_lBegLine, &pLambdaExpression->LeftParen, 1);

        return locLParen.EndsOnOrBefore(pLocation);
    }

    return false;
}

bool
ParseTreeHelpers::IsLocationAfterLambdaParameters
(
    Location *pLocation,
    ParseTree::LambdaExpression *pLambdaExpression,
    Location *pLocRParen/* = null */
)
{
    bool locationIsAfterLamabdaParameters = false;
    AssertIfNull(pLocation);
    AssertIfNull(pLambdaExpression);

    if (HasPunctuator(pLambdaExpression->RightParen))
    {
        Location locRParen;
        locRParen.SetLocation(pLambdaExpression->TextSpan.m_lBegLine, &pLambdaExpression->RightParen, 1);
        if (pLocRParen)
        {
            *pLocRParen= locRParen;
        }

        locationIsAfterLamabdaParameters = locRParen.EndsOnOrBefore(pLocation);
    }

    return locationIsAfterLamabdaParameters;
}

unsigned
ParseTreeHelpers::GetParameterCount
(
    ParseTree::ParameterList *pParameterList
)
{
    unsigned uCount = 0;

    while (pParameterList)
    {
        uCount++;
        pParameterList = pParameterList->Next;
    }

    return uCount;
}

void
ParseTreeHelpers::GetStartLocationOfQueryExpression
(
    ParseTree::Expression *pExpression,
    Location *plocStart
)
{
    AssertIfNull(pExpression);
    AssertIfNull(plocStart);

    bool fStop = false;

    while (pExpression && !fStop)
    {
        switch (pExpression->Opcode)
        {
            case ParseTree::Expression::From:
            case ParseTree::Expression::Aggregate:
            case ParseTree::Expression::LinqSource:
                fStop = true;
                break;

            case ParseTree::Expression::CrossJoin:
                pExpression = pExpression->AsCrossJoin()->Source;
                break;

            case ParseTree::Expression::Where:
                pExpression = pExpression->AsWhere()->Source;
                break;

            case ParseTree::Expression::GroupBy:
                pExpression = pExpression->AsGroupBy()->Source;
                break;

            case ParseTree::Expression::Select:
                pExpression = pExpression->AsSelect()->Source;
                break;

            case ParseTree::Expression::OrderBy:
                pExpression = pExpression->AsOrderBy()->Source;
                break;

            case ParseTree::Expression::Distinct:
                pExpression = pExpression->AsDistinct()->Source;
                break;

            case ParseTree::Expression::InnerJoin:
                pExpression = pExpression->AsInnerJoin()->Source;
                break;

            case ParseTree::Expression::GroupJoin:
                pExpression = pExpression->AsGroupJoin()->Source;
                break;

            case ParseTree::Expression::Take:
            case ParseTree::Expression::Skip:
                pExpression = pExpression->AsSkipTake()->Source;
                break;

            case ParseTree::Expression::TakeWhile:
            case ParseTree::Expression::SkipWhile:
                pExpression = pExpression->AsWhile()->Source;
                break;

            default:
                pExpression = NULL;
                break;
        }
    }

    if (pExpression)
    {
        plocStart->SetLocation(pExpression->TextSpan.m_lBegLine, pExpression->TextSpan.m_lBegColumn);
    }
    else
    {
        plocStart->Invalidate();
    }
}

bool
ParseTreeHelpers::ContainsAssemblyOrModuleAttributes
(
    ParseTree::AttributeStatement *pStatement
)
{
    if (!pStatement)
        return false;

    for (ParseTree::AttributeSpecifierList *pAttrList = pStatement->Attributes; pAttrList; pAttrList = pAttrList->Next)
    {
        for (ParseTree::AttributeList *pAttr = pAttrList->Element->Values; pAttr; pAttr = pAttr->Next)
        {
            if (pAttr->Element && (pAttr->Element->IsAssembly || pAttr->Element->IsModule))
            {
                return true;
            }
        }
    }

    return false;
}

tokens
ParseTreeHelpers::MapOptionKindToTokenType
(
    ParseTree::Statement *pOption
)
{
    if (!pOption)
    {
        return tkNone;
    }

    switch (pOption->Opcode)
    {
        case ParseTree::Statement::OptionUnknown:
        case ParseTree::Statement::OptionInvalid:
        default:
            return tkNone;

        case ParseTree::Statement::OptionCompareNone:
        case ParseTree::Statement::OptionCompareText:
        case ParseTree::Statement::OptionCompareBinary:
            return tkCOMPARE;

        case ParseTree::Statement::OptionExplicitOn:
        case ParseTree::Statement::OptionExplicitOff:
            return tkEXPLICIT;

        case ParseTree::Statement::OptionStrictOn:
        case ParseTree::Statement::OptionStrictOff:
            return tkSTRICT;

        case ParseTree::Statement::OptionInferOn:
        case ParseTree::Statement::OptionInferOff:
            return tkINFER;
    }
}

ParseTree::Declarator *
ParseTreeHelpers::VariableDeclaratorIterator::Next(ParseTree::VariableDeclaration **ppCurrentDeclaration)
{
    ParseTree::Declarator *pDeclarator = m_pCurrentDeclarator ? m_pCurrentDeclarator->Element : NULL;

    if (ppCurrentDeclaration)
    {
        *ppCurrentDeclaration = m_pCurrentDeclaration ? m_pCurrentDeclaration->Element : NULL;
    }

    if (m_pCurrentDeclarator)
    {
        m_pCurrentDeclarator = m_pCurrentDeclarator->Next;
    }
    else if (m_pCurrentDeclaration)
    {
        m_pCurrentDeclaration = m_pCurrentDeclaration->Next;
        m_pCurrentDeclarator = (m_pCurrentDeclaration && m_pCurrentDeclaration->Element) ? m_pCurrentDeclaration->Element->Variables : NULL;
    }

    return pDeclarator;
}

void
ParseTreeHelpers::VariableDeclaratorIterator::Reset()
{
    m_pCurrentDeclaration = m_pStatement->Declarations;
    m_pCurrentDeclarator = (m_pCurrentDeclaration && m_pCurrentDeclaration->Element) ? m_pCurrentDeclaration->Element->Variables : NULL;
}


