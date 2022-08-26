//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB compiler parser.
//
//  Except for inline methods, which are defined at the top of the file, the
//  method definitions here appear in the same order as their declarations in
//  Parser.h.
//  
//  Parsing Model
//  
//  The VB grammar spec is essential for understanding the parser.
//  
//  The VB language is heavily line based. The parser's normal mode
//  of operation is therefore line based. The token stream provides
//  all of the tokens for a given logical line in one operation,
//  which gives the parser infinite lookahead within a line.
//  
//  The parser verifies the syntactic correctness of language
//  constructs made up of more than one statement (the statement
//  groups). The parser keeps track of syntactic context in order to
//  break the input program into statement groups.
//  
//  The mechanism for managing contexts is to use LinkStatement to enter a statement
//  as the last statement in the current context, EstablishContext to create a new
//  context as a child of the current context, and PopContext to establish the
//  current context's parent as the current context. A LinkStatement call should
//  not occur until all of the operands of the statement have been parsed, because
//  the parser uses the LinkStatement call as a trigger to set the HasSyntaxError flag for
//  statements that contain syntax errors. Calling LinkStatement before parsing an
//  operand will result in setting HasSyntaxError in the following statement if the
//  operand has a syntax error. LinkStatement will perform an EstablishContext if
//  the statement it is linking is a block, so a separate EstablishContext for
//  linked block statements is not necessary.
//  
//  The parsing mechanism is hand-written recursive descent, with operator
//  precedence for infix expressions. Error recovery is ad hoc, but will
//  become less so over time.
//  
//  The parser is required to produce well-formed trees for all input,
//  inserting error trees as necessary to represent malformed constructs.
//  It is a requirement that an attempt to parse a statement produce some sort
//  of statement tree; otherwise, program text can evaporate during pretty listing.
//  
//  The location information for blocks describes the statement beginning the block
//  while the block's body is being parsed, and describes the entire range of the
//  block (including its end construct) after the block has been terminated.
//  
//  All list structures in the parse trees are right heavy, i.e. follow the Lisp CAR/CDR
//  conventions. The text location info for a list element (an SX_LIST node) spans
//  from the beginning of the list element through the end of the list. The location of
//  the end of the list is not known when creating the list element (to avoid possibly
//  deep recursions in the parsing logic), so the end locations of list elements are
//  set after completing the parsing of the list. (The method FixupListEndLocations
//  accomplishes this.)
//  
//  Most parsing procedures have a boolean in/out parameter named ErrorInConstruct that
//  keeps track of whether or not the parse is reliably synchronized with the input
//  stream. A parsing procedure should generate syntax errors only if ErrorInConstruct
//  is false, and should set ErrorInConstruct true whenever a syntax error occurs, unless
//  robust error recovery is possible. ErrorInConstruct is not a reliable indicator of
//  whether or not a syntax error has occured, because successful error recovery can
//  suppress setting it.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"
#include "ParserPrivate.inl"  // Inline functions specific to parser

class SaveErrorStateGuard : public GuardBase
{
public:
    SaveErrorStateGuard(Parser *pParser) : 
      m_pParser(pParser),
      m_errorDisabled(pParser->IsErrorDisabled()),
      m_pErrorTable(pParser->m_Errors)
    {
    }

    ~SaveErrorStateGuard()
    {
        RunAction();
    }

protected:
    virtual __override 
    void DoAction()
    {
        m_pParser->EnableErrors(m_errorDisabled);
        m_pParser->m_Errors = m_pErrorTable;
    }

private:
    SaveErrorStateGuard();
    SaveErrorStateGuard(const SaveErrorStateGuard&);

    Parser *m_pParser;
    bool m_errorDisabled;
    ErrorTable *m_pErrorTable;
};


class SetupConditionalConstantsScopeForPartialParse
{
private:
    SetupConditionalConstantsScopeForPartialParse();
    SetupConditionalConstantsScopeForPartialParse(const SetupConditionalConstantsScopeForPartialParse&);

    Parser *m_pParser;
    bool save_m_DoLiteParsingOfConditionalCompilation;
    BCSYM_CCContainer* save_m_ConditionalConstantsScope;
    bool m_DoRestore;

public:

    SetupConditionalConstantsScopeForPartialParse
    (
        Parser *pParser,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants
    ) : 
      m_pParser(pParser),
      save_m_DoLiteParsingOfConditionalCompilation(pParser->m_DoLiteParsingOfConditionalCompilation),
      save_m_ConditionalConstantsScope(pParser->m_ConditionalConstantsScope),
      m_DoRestore(false)
    {
        if (pConditionalCompilationConstants)
        {
            AssertIfTrue(m_pParser->m_ConditionalConstantsScope);  // why would we want to override Parser's ConditionalConstantsScope?

            m_DoRestore = true;
            m_pParser->m_DoLiteParsingOfConditionalCompilation = true; 
            m_pParser->m_ConditionalConstantsScope = pConditionalCompilationConstants->PCCContainer();

            // Clear the parent before (possibly) setting it to a (possibly) different value.
            Symbols::ClearParent(m_pParser->m_ConditionalConstantsScope);

            if (pProjectLevelCondCompScope)
            {
                Symbols::SetParent(m_pParser->m_ConditionalConstantsScope, pProjectLevelCondCompScope->GetHash());
            }
        }
    }


    ~SetupConditionalConstantsScopeForPartialParse()
    {
        if (m_DoRestore)
        {
            AssertIfFalse(m_pParser->m_ConditionalConstantsScope != save_m_ConditionalConstantsScope);
            
            // Clear the CC scope's parent (see VS RAID 231584)
            Symbols::ClearParent(m_pParser->m_ConditionalConstantsScope);

            m_pParser->m_DoLiteParsingOfConditionalCompilation = save_m_DoLiteParsingOfConditionalCompilation; 
            m_pParser->m_ConditionalConstantsScope = save_m_ConditionalConstantsScope;
        }
    }

};




ParseTree::MethodDeclarationStatement * Parser::CreateMethodDeclaration
(
    ParseTree::Statement::Opcodes Opcode,
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::IdentifierDescriptor Ident,
    ParseTree::GenericParameterList *GenericParams,
    ParseTree::ParameterList *Params,
    ParseTree::Type *ReturnType,
    ParseTree::AttributeSpecifierList *ReturnTypeAttributes,
    ParseTree::SpecifierList *Specifiers,
    ParseTree::NameList *Implements,
    ParseTree::NameList *Handles,
    _In_ Token *StartToken,
    _In_ Token *EndToken,
    _In_opt_ Token *MethodKind,
    _In_opt_ Token *LeftParen,
    _In_opt_ Token *RightParen,
    _In_opt_ Token *As,
    _In_opt_ Token *HandlesOrImplements,
    _In_opt_ Token *Of,
    _In_opt_ Token *GenericLeftParen,
    _In_opt_ Token *GenericRightParen,
    bool IsDelegate
)
{
    ParseTree::MethodDeclarationStatement *Proc =
        IsDelegate ?
            new(m_TreeStorage) ParseTree::DelegateDeclarationStatement :
            Opcode == ParseTree::Statement::OperatorDeclaration ?
                (ParseTree::MethodDeclarationStatement *)new(m_TreeStorage) ParseTree::OperatorDefinitionStatement :
                Opcode == ParseTree::Statement::EventDeclaration ?
                    (ParseTree::MethodDeclarationStatement *)new(m_TreeStorage) ParseTree::EventDeclarationStatement :
                    (ParseTree::MethodDeclarationStatement *)new(m_TreeStorage) ParseTree::MethodDefinitionStatement;

    Proc->Opcode = Opcode;
    Proc->Attributes = Attributes;
    Proc->Name = Ident;
    Proc->Specifiers = Specifiers;
    Proc->GenericParameters = GenericParams;
    Proc->Parameters = Params;
    Proc->ReturnType = ReturnType;
    Proc->ReturnTypeAttributes = ReturnTypeAttributes;
    Proc->Handles = Handles;
    Proc->Implements = Implements;
    
    SetStmtBegLocation(Proc, StartToken);
    SetStmtEndLocation(Proc, EndToken);
    SetPunctuator(&Proc->MethodKind, StartToken, MethodKind);
    SetPunctuator(&Proc->LeftParen, StartToken, LeftParen);
    SetPunctuator(&Proc->RightParen, StartToken, RightParen);
    SetPunctuator(&Proc->As, StartToken, As);
    SetPunctuator(&Proc->HandlesOrImplements, StartToken, HandlesOrImplements);
    SetPunctuator(&Proc->GenericOf, StartToken, Of);
    SetPunctuatorIfMatches(&Proc->GenericLeftParen, StartToken, GenericLeftParen, tkLParen);
    SetPunctuatorIfMatches(&Proc->GenericRightParen, StartToken, GenericRightParen, tkRParen);

    return Proc;
}

ParseTree::Expression *
Parser::ExpressionSyntaxError
(
)
{
    ParseTree::Expression *Error = new(m_TreeStorage) ParseTree::Expression;
    Error->Opcode = ParseTree::Expression::SyntaxError;

    return Error;
}

ParseTree::IdentifierDescriptor
Parser::CreateId
(
    _In_ Token *IdToken
)
{
    VSASSERT(IdToken->m_TokenType == tkID || IdToken->IsKeyword(), "Expected identifier isn't.");

    ParseTree::IdentifierDescriptor Result;

    Result.Name = IdToken->m_Id.m_Spelling;
    Result.TypeCharacter = IdToken->m_Id.m_TypeCharacter;
    Result.IsBracketed = IdToken->m_Id.m_IsBracketed;
    Result.IsBad = false;
    Result.IsNullable = false;

    SetLocation(&Result.TextSpan, IdToken, IdToken);

    return Result;
}

ParseTree::IdentifierDescriptor
Parser::CreateXmlNameIdentifier
(
    _In_ Token *NameToken,
    bool &ErrorInConstruct
)
{
    VSASSERT(NameToken && (NameToken->m_TokenType == tkXmlNCName), "Expected tkXmlName isn't.");

    ParseTree::IdentifierDescriptor Name;
    Name.Name = NameToken->m_Id.m_Spelling;
    Name.TypeCharacter = chType_NONE;
    Name.IsBracketed = false;
    Name.IsBad = false;
    Name.IsNullable = false;
    SetLocation(&Name.TextSpan, NameToken, NameToken);

    return Name;
}

void
Parser::CreateZeroArgument
(
    ParseTree::ArgumentList **&ArgumentsTarget
)
{
    ParseTree::IntegralLiteralExpression *Value = new(m_TreeStorage) ParseTree::IntegralLiteralExpression;

    Value->Opcode = ParseTree::Expression::IntegralLiteral;
    Value->Value = 0;
    Value->Base = ParseTree::IntegralLiteralExpression::Decimal;
    Value->TypeCharacter = chType_NONE;
    SetExpressionLocation(Value, m_CurrentToken, m_CurrentToken, NULL);

    ParseTree::Argument *Argument = new(m_TreeStorage) ParseTree::Argument;
    Argument->Value = Value;
    SetLocation(&Argument->TextSpan, m_CurrentToken, m_CurrentToken);

    CreateListElement(
        ArgumentsTarget,
        Argument,
        m_CurrentToken,
        m_CurrentToken,
        m_CurrentToken);
}

void
Parser::UpdateStatementLinqFlags
(
    _Out_ ParseTree::Statement *pStatement
)
{
    AssertIfNull(pStatement);

    pStatement->ContainsAnonymousTypeInitialization = m_SeenAnonymousTypeInitialization;
    pStatement->ContainsQueryExpression = m_SeenQueryExpression;
    pStatement->ContainsLambdaExpression = m_SeenLambdaExpression;
}

Token *
Parser::GetNextXmlToken
(
)
{
    if (m_CurrentToken->m_TokenType != tkEOL)
    {
        if (m_CurrentToken->m_TokenType != tkEOF)
            m_CurrentToken = m_CurrentToken->m_Next;
    }
    else if (m_CurrentToken->m_EOL.m_NextLineAlreadyScanned)
    {
       m_CurrentToken = m_CurrentToken->m_Next;
    }

    VSASSERT(m_CurrentToken->m_TokenType != tkNone, "Unexpected tkNone token");

    return m_CurrentToken;
}


Token *
Parser::GetNextToken
(
)
{
    // If advancing past a token that follows an undisposed comment, then
    // the undisposed comment(s) belong to the current statement.

    while (m_FirstUndisposedComment)
    {
        Token *T = m_FirstUndisposedComment;

        CreateComment(T);

        if (T->m_Next->m_TokenType == tkEOL &&
            T->m_Next->m_EOL.m_NextLineAlreadyScanned)
        {
            T = T->m_Next;
        }

        m_FirstUndisposedComment =
            (T->m_Next->m_TokenType == tkREM || T->m_Next->m_TokenType == tkXMLDocComment) ?
                T->m_Next :
                NULL;
    }

    Token *Next = Following(m_CurrentToken);
    
    m_CurrentToken = Next;
    return Next;
}

inline long
Parser::TokenColumnBeg
(
    _In_ Token *Tok
)
{
    return Tok->m_StartColumn;
}

inline void
Parser::CacheStmtLocation
(
)
{
#if DEBUG
    m_CurrentStatementStartLine = m_CurrentToken->m_StartLine;
    m_CurrentStatementStartColumn = m_CurrentToken->m_StartColumn;
#endif
}

inline void
Parser::SetStmtBegLocation
(
    ParseTree::Statement *Statement,
    _In_ Token *Start
)
{
    VSASSERT(Start->m_StartLine == m_CurrentStatementStartLine, "Bad line.");
    VSASSERT(Start->m_StartColumn == m_CurrentStatementStartColumn, "Bad column.");

    Statement->TextSpan.m_lBegLine = Start->m_StartLine;
    Statement->TextSpan.m_lBegColumn = TokenColumnBeg(Start);
}

inline void
Parser::SetStmtEndLocation
(
    _In_ ParseTree::Statement *Statement,
    _In_ Token *FollowingEnd
)
{
    Token * pTmp = BackupIfImpliedNewline(FollowingEnd);

    if (pTmp != FollowingEnd)
    {
        FollowingEnd = pTmp;
    }

    Statement->TextSpan.m_lEndLine = FollowingEnd->m_Prev->m_StartLine;
    Statement->TextSpan.m_lEndColumn = TokenColumnEnd(FollowingEnd->m_Prev);

    if (Statement->IsBlock())
    {
        Statement->AsBlock()->BodyTextSpan.m_lBegLine = FollowingEnd->m_StartLine;
        Statement->AsBlock()->BodyTextSpan.m_lBegColumn = TokenColumnBeg(FollowingEnd);
    }

    // In error cases, the parser sometimes makes more than one statement tree per
    // statement, so clearing the statement location cache is unhealthy.
    // 



}

void
Parser::SetFileBlockLocation
(
    ParseTree::FileBlockStatement *File,
    _In_ Token *LastToken
)
{
    VSASSERT(LastToken->m_TokenType == tkEOF, "File ends at a token other than EOF.");

    // The file block's location is from the start of the file
    // (line 0 / column 0, which can precede the location of the
    // first token) through the end of the file (which is always the
    // location of the EOF token--not the token preceding the EOF token).
    // File block locations are set manually rather than by the utility
    // methods because they don't obey the rules of other constructs.

    File->TextSpan.m_lBegLine = 0;
    File->TextSpan.m_lBegColumn = 0;
    File->TextSpan.m_lEndLine = LastToken->m_StartLine;
    File->TextSpan.m_lEndColumn = TokenColumnEnd(LastToken);
    File->BodyTextSpan.m_lBegLine = 0;
    File->BodyTextSpan.m_lBegColumn = 0;
    File->BodyTextSpan.m_lEndLine = LastToken->m_StartLine;
    File->BodyTextSpan.m_lEndColumn = TokenColumnEnd(LastToken);
}

void Parser::SetLocationForSpanOfOneToken
(
    _Out_ Location *pLoc,     // [out] we fill out pLoc based on the pToken and End tokens
    _In_ Token *pToken        // [in] token that represents the construct (inclusive)
)
{
    if (pToken->IsEndOfLine() && pToken->m_StartColumn != 0)
    {
        // The token is at the end of the line.  Put it at the last character.
        pLoc->m_lBegLine = pToken->m_StartLine;
        pLoc->m_lBegColumn = pToken->m_StartColumn - 1;
        pLoc->m_lEndLine = pToken->m_StartLine;
        pLoc->m_lEndColumn = pToken->m_StartColumn - 1;
    }
    else // Just set the location to be the token (either will do as they are the same)
    {
        pLoc->m_lBegLine = pToken->m_StartLine;
        pLoc->m_lBegColumn = pToken->m_StartColumn;
        pLoc->m_lEndLine = pToken->m_StartLine;
        pLoc->m_lEndColumn = pToken->m_StartColumn;

        if (pToken->m_Width > 0)
        {
            pLoc->m_lEndColumn += pToken->m_Width - 1;
        }
    }
}

void
Parser::SetLocation
(
    _Out_ Location* Loc,     // [out] we fill out Loc based on the Beg and End tokens
    _In_ Token *Beg,        // [in] begin token for the construct (inclusive)
    _In_ Token *End         // [in] end token for the construct (non-inclusive)
)
{
    if (Beg == End) // Location spans only one token
    {
        SetLocationForSpanOfOneToken( Loc, Beg );
    }
    else // The location spans more than one token
    {
        Token * TokenWeBackedUpTo = BackupIfImpliedNewline(End); // If we have an implied EOL, try to backup to the last token in the offending span

        /* We are in an error state if the end token doesn't line up with where the statement should have ended
        That means the parser went past the logical end of the statement because of a line continuation but
        things didn't work out */
        if (TokenWeBackedUpTo != End)
        {
            End = TokenWeBackedUpTo;
        }

        /* It is possible for BackupIfImpliedNewline(End) to backup to the begin token.  For instance
            
            dim x = 1 + _
            EOL
            REM This is bug 444553

        In this case End will point to the REM.  Begin points to the EOL following the explicit line continuation.
        When we backup, TokenWeBackedUpTo will be on the EOL as well.
        This code treats the End token as non-inclusive as far as error reporting goes, so we need to look again at the condition
        where the Begin token and End token are the same thing so we get the right span. */
        if (Beg == End )
        {
            SetLocationForSpanOfOneToken( Loc, Beg );
        }
        else
        {
            VSASSERT( Beg->m_StartLine <= End->m_Prev->m_StartLine, "How can the start line come after the end line?  This will cause a crash later in the task window" );
            // Note the m_Prev below - the end token is non-inclusive as far as computing the span goes.
            Loc->m_lBegLine = Beg->m_StartLine;
            Loc->m_lBegColumn = Beg->m_StartColumn;
            Loc->m_lEndLine = End->m_Prev->m_StartLine;
            Loc->m_lEndColumn = End->m_Prev->m_StartColumn;
            if (Beg->m_Width > 0)
            {
                // Dev10 #687030: Avoid setting negative m_lEndColumn.
                if ( Loc->m_lEndColumn > 0 || End->m_Prev->m_Width > 0 )
                {
                    AssertIfFalse(End->m_Prev->m_Width >= 0);
                    AssertIfFalse(Loc->m_lEndColumn >= 0);
                    Loc->m_lEndColumn += End->m_Prev->m_Width - 1;
                }
            }
        }
    } // else the location spans more than one token.
}

void
Parser::SetLocationAndPunctuator
(
    _Out_ Location *TextSpan,
    _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
    _In_ Token *Beg,        // [in] begin token for the construct (inclusive)
    _In_ Token *End,         // [in] end token for the construct (non-incl)
    _In_opt_ Token *Punctuator
)
{
    SetLocation(TextSpan, Beg, End);
    SetPunctuator(PunctuatorSource, Beg, Punctuator);
}

Token *  
Parser::BackupIfImpliedNewline
(
    _In_ Token *T
)
{
    if ( T == m_FirstTokenAfterNewline )
    {
        VSASSERT(T->m_Prev->m_TokenType == tkEOL, "How can a token that follows a newline not be preceeded by an EOL Token?" );

        do
        {
            T = T->m_Prev;

            if ( T->m_Prev->m_TokenType == tkREM || T->m_Prev->m_TokenType == tkXMLDocComment )
            {
                T = T->m_Prev;
            }

        } while ( T->m_Prev->m_TokenType == tkEOL );
    }

    return T;
}

inline tokens
Parser::IdentifierAsKeyword
(
    _In_ Token *T
)
{
    return ::IdentifierAsKeyword(T, m_pStringPool);
}

inline tokens
Parser::TokenAsKeyword
(
    _In_ Token *T
)
{
    return ::TokenAsKeyword(T, m_pStringPool);
}

// Returns if Tok equals the given non-reserved token, tkNonReserved
inline bool
Parser::EqNonReservedToken
(
    _In_ Token *T,
    tokens NonReservedTokenType
)
{
    return (T->m_TokenType == tkID && IdentifierAsKeyword(T) == NonReservedTokenType);
}


static ParseTree::Expression::Opcodes
GetBinaryOperatorHelper
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        case tkIS:
            return ParseTree::Expression::Is;
        case tkISNOT:
            return ParseTree::Expression::IsNot;
        case tkLIKE:
            return ParseTree::Expression::Like;
        case tkAND:
            return ParseTree::Expression::And;
        case tkANDALSO:
            return ParseTree::Expression::AndAlso;
        case tkOR:
            return ParseTree::Expression::Or;
        case tkORELSE:
            return ParseTree::Expression::OrElse;
        case tkXOR:
            return ParseTree::Expression::Xor;
        case tkConcat:
            return ParseTree::Expression::Concatenate;
        case tkMult:
            return ParseTree::Expression::Multiply;
        case tkPlus:
            return ParseTree::Expression::Plus;
        case tkMinus:
            return ParseTree::Expression::Minus;
        case tkDiv:
            return ParseTree::Expression::Divide;
        case tkIDiv:
            return ParseTree::Expression::IntegralDivide;
        case tkMOD:
            return ParseTree::Expression::Modulus;
        case tkPwr:
            return ParseTree::Expression::Power;
        case tkLT:
            return ParseTree::Expression::Less;
        case tkLE:
            return ParseTree::Expression::LessEqual;
        case tkNE:
            return ParseTree::Expression::NotEqual;
        case tkEQ:
            return ParseTree::Expression::Equal;
        case tkGT:
            return ParseTree::Expression::Greater;
        case tkGE:
            return ParseTree::Expression::GreaterEqual;
        case tkShiftLeft:
            return ParseTree::Expression::ShiftLeft;
        case tkShiftRight:
            return ParseTree::Expression::ShiftRight;

        default:
            return ParseTree::Expression::SyntaxError;
    }
}


ParseTree::Expression::Opcodes
Parser::GetBinaryOperator
(
    _In_ Token *T,
    _Out_opt_ OperatorPrecedence *Precedence
)
{
    ParseTree::Expression::Opcodes Op =
        GetBinaryOperatorHelper(T);

    if (Op == ParseTree::Expression::SyntaxError)
    {
        return Op;
    }

    tokens TokenType = T->m_TokenType;

#if IDE
    if (Op == ParseTree::Expression::Greater &&
        T->m_Next &&
        T->m_Next->m_TokenType == tkLT)
    {
        // The pretty lister needs to convert '><' into '<>'. It does this by
        // looking for the tkNE token, so in the context of binary operators
        // we return tkNe instead of tkGT-tkLT.

        GetNextToken();

        Op = ParseTree::Expression::NotEqual;

        m_CurrentToken->m_TokenType = TokenType = tkNE;
    }
    else if (Op == ParseTree::Expression::Equal &&
             T->m_Next)
    {
        if (T->m_Next->m_TokenType == tkGT)
        {
            // The pretty lister needs to convert '=>' into '>='. Look at the next
            // token to decide.

            GetNextToken();

            Op = ParseTree::Expression::GreaterEqual;

            m_CurrentToken->m_TokenType = TokenType = tkGE;
        }
        else if (T->m_Next->m_TokenType == tkLT)
        {
            // The pretty lister needs to convert '=<' into '<='. Because
            // the scanner switches into Xml mode when '=' is followed by '<',
            // the line needs to be rescanned, forcing the scanners in a VB only
            // scanning state.

            // Calculate the correct scanner state for restarting the scan.
            ColorizerState State = m_InputStream->GetState(m_CurrentToken);

            // Throw away the old tokens scanned as XML
            m_InputStream->AbandonTokens(m_CurrentToken->m_Next);
            TokenType = m_CurrentToken->m_TokenType;

            // Scan the line with the state forced to VB
            m_CurrentToken->m_TokenType = tkNone;
            m_InputStream->GetNextLine(State, false);
            m_CurrentToken->m_TokenType = TokenType;

            GetNextToken();

            Op = ParseTree::Expression::LessEqual;

            m_CurrentToken->m_TokenType = TokenType = tkLE;
        }
    }
 #endif

    if (Precedence)
    {
        *Precedence = TokenOpPrec(TokenType);
    }

    return Op;

}


inline bool
IsBinaryOperator
(
    _In_ Token *T
)
{
    return GetBinaryOperatorHelper(T) != ParseTree::Expression::SyntaxError;
}

static ParseTree::Statement::Opcodes
GetAssignmentOperator
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        case tkEQ:
            return ParseTree::Statement::Assign;
        case tkPlusEQ:
            return ParseTree::Statement::AssignPlus;
        case tkConcatEQ:
            return ParseTree::Statement::AssignConcatenate;
        case tkMultEQ:
            return ParseTree::Statement::AssignMultiply;
        case tkMinusEQ:
            return ParseTree::Statement::AssignMinus;
        case tkDivEQ:
            return ParseTree::Statement::AssignDivide;
        case tkIDivEQ:
            return ParseTree::Statement::AssignIntegralDivide;
        case tkPwrEQ:
            return ParseTree::Statement::AssignPower;
        case tkShiftLeftEQ:
            return ParseTree::Statement::AssignShiftLeft;
        case tkShiftRightEQ:
            return ParseTree::Statement::AssignShiftRight;
        default:
            return ParseTree::Statement::SyntaxError;
    }
}

inline bool
IsAssignmentOperator
(
    _In_ Token *T
)
{
    return GetAssignmentOperator(T) != ParseTree::Statement::SyntaxError;
}

bool
ParseTree::Statement::IsBlock
(
)
{
    switch (Opcode)
    {
        case Structure:
        case Enum:
        case Interface:
        case Class:
        case Module:
        case Namespace:
        case Property:
        case BlockEventDeclaration:
        case CommentBlock:

        case File:
        case ProcedureBody:
        case PropertyGetBody:
        case PropertySetBody:
        case FunctionBody:
        case OperatorBody:
        case AddHandlerBody:
        case RemoveHandlerBody:
        case RaiseEventBody:

        case BlockIf:
        case LineIf:
        case ElseIf:
        case BlockElse:
        case LineElse:
        case Select:
        case Case:
        case CaseElse:
        case Try:
        case Catch:
        case Finally:
        case ForFromTo:
        case ForEachIn:
        case While:
        case DoWhileTopTest:
        case DoUntilTopTest:
        case DoWhileBottomTest:
        case DoUntilBottomTest:
        case DoForever:
        case With:
        case Using:
        case SyncLock:
        case Region:
        case LambdaBody:

            return true;
    }

    return false;
}

bool
ParseTree::Statement::IsContainerBlock()
{
    switch (Opcode)
    {
        case Structure:
        case Enum:
        case Interface:
        case Class:
        case Module:
        case Namespace:
        case Property:
            return true;
    }

    return false;
}

bool
ParseTree::Statement::IsExecutableBlock
(
)
{
    switch (Opcode)
    {
        case ProcedureBody:
        case PropertyGetBody:
        case PropertySetBody:
        case FunctionBody:
        case OperatorBody:
        case AddHandlerBody:
        case RemoveHandlerBody:
        case RaiseEventBody:

        case BlockIf:
        case LineIf:
        case ElseIf:
        case BlockElse:
        case LineElse:
        case Select:
        case Case:
        case CaseElse:
        case Try:
        case Catch:
        case Finally:
        case ForFromTo:
        case ForEachIn:
        case While:
        case DoWhileTopTest:
        case DoUntilTopTest:
        case DoWhileBottomTest:
        case DoUntilBottomTest:
        case DoForever:
        case With:
        case Using:
        case SyncLock:
        case LambdaBody:

            return true;
    }

    return false;
}

bool
ParseTree::Statement::IsOptionStatement()
{
    switch(Opcode)
    {
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
        return true;
    }

    return false;
}

bool
ParseTree::Statement::IsMethodDefinition
(
)
{
    switch (Opcode)
    {
        case ProcedureDeclaration:
        case FunctionDeclaration:
        case OperatorDeclaration:
        case ConstructorDeclaration:
        case PropertyGet:
        case PropertySet:
        case AddHandlerDeclaration:
        case RemoveHandlerDeclaration:
        case RaiseEventDeclaration:
            return true;
    }
    return false;
}

bool
ParseTree::Statement::IsMethodBody
(
)
{
    switch (Opcode)
    {
        case ProcedureBody:
        case FunctionBody:
        case OperatorBody:
        case PropertyGetBody:
        case PropertySetBody:
        case AddHandlerBody:
        case RemoveHandlerBody:
        case RaiseEventBody:
            return true;
    }
    return false;
}

bool
ParseTree::Statement::IsPlacebo
(
)
{
    if (IsConditionalCompilationBranch() )
    {
        return true;
    }

    switch (Opcode)
    {
        case ParseTree::Statement::Empty:
        case ParseTree::Statement::CCConst:
        case ParseTree::Statement::CCEndIf:
        case ParseTree::Statement::Region:
        case ParseTree::Statement::EndRegion:
        case ParseTree::Statement::CommentBlock:
            return true;
    }

    return false;
}

bool
ParseTree::Statement::IsConditionalCompilationBranch
(
)
{
    switch (Opcode)
    {
        case ParseTree::Statement::CCIf:
        case ParseTree::Statement::CCElseIf:
        case ParseTree::Statement::CCElse:
            return true;
    }

    return false;
}

bool
ParseTree::Statement::IsEndConstruct
(
)
{
    switch (Opcode)
    {
        case EndIf:
        case EndUsing:
        case EndWith:
        case EndSelect:
        case EndStructure:
        case EndEnum:
        case EndInterface:
        case EndClass:
        case EndModule:
        case EndNamespace:
        case EndSub:
        case EndFunction:
        case EndOperator:
        case EndGet:
        case EndSet:
        case EndProperty:
        case EndAddHandler:
        case EndRemoveHandler:
        case EndRaiseEvent:
        case EndEvent:
        case EndNext:
        case EndLoop:
        case EndWhile:
        case EndLoopWhile:
        case EndLoopUntil:
        case EndTry:
        case EndSyncLock:
        case EndRegion:
        case EndCommentBlock:
        case EndUnknown:
        case EndInvalid:
            return true;
    }
    return false;
}

ParseTree::BlockStatement *
ParseTree::Statement::GetRawParent()
{
    return this->Parent;
}

ParseTree::BlockStatement *
ParseTree::Statement::GetParent()
{
    ParseTree::BlockStatement *pParent = this->Parent;
    
    // Skip over hidden blocks.
    while( pParent && pParent->Opcode == ParseTree::Statement::HiddenBlock)
    {
        pParent = pParent->Parent;
    }
    
    return pParent;
}

void
ParseTree::Statement::SetParent(ParseTree::BlockStatement *pValue)
{
    this->Parent = pValue;
}

ParseTree::BlockStatement **
ParseTree::Statement::GetRawParentAddress()
{
    return &Parent; 
}

bool
ParseTree::Expression::IsQueryExpression
(
)
{
    switch (Opcode)
    {
        case Where:
        case OrderBy:
        case From:
        case CrossJoin:
        case Select:
        case Distinct:
            return true;
    }
    return false;
}

bool
ParseTree::Expression::ContainsArgumentList
(
)
{
    switch (Opcode)
    {
        case IIf:
        case CallOrIndex:
        case New:
            return true;
    }
    return false;
}
        
ParseTree::ArgumentList*
ParseTree::Expression::GetArgumentList
(
)
{
    VSASSERT(this->ContainsArgumentList(), "This Expression does not contain an ArgumentList");

    ParseTree::ArgumentList *pArgList = NULL;
            
    switch(this->Opcode)
    {
        case ParseTree::Expression::CallOrIndex:
            pArgList = this->AsCallOrIndex()->Arguments.Values;
            break;
        case ParseTree::Expression::New:
            pArgList = this->AsNew()->Arguments.Values;
            break;
        case ParseTree::Expression::IIf:
            pArgList = this->AsIIf()->Arguments.Values;
            break;

    }

    return pArgList;
}

/*****************************************************************************
 ;GetDelimitedNameCount

 Abstract: Determine how many dot delimited qualifers are in a name.  e.g.:
   Return: foo - returns 1
               foo.bar - returns 2
               another.foo.bar - returns 3, etc.
               If a name is bad, 0 is returned.
*****************************************************************************/
unsigned ParseTree::Name::GetDelimitedNameCount()
{
    ParseTree::Name *Name = this;

    unsigned DotDelimitedNamesCount = 1; // The number of dot-delimited names is the number of dots plus one.

    /* The representation for a.b.c is:

                                  QualifiedName C
                                      |
                                  QualifiedName B
                                      |
                                  SimpleName A
    */

    while (Name->IsQualified())
    {
        DotDelimitedNamesCount++;
        // Work down to the simple name.
        Name = Name->AsQualified()->Base;
    }

    VSASSERT(
        Name->IsSimple() || Name->Opcode == ParseTree::Name::GlobalNameSpace,
        "Expected a Simple name or GlobalNameSpace at this point.");

    if (Name->Opcode == ParseTree::Name::GlobalNameSpace)
    {
        // "GlobalNameSpace.a.b.c" - the keyword "GlobalNameSpace" is not counted.
        DotDelimitedNamesCount -= 1;
    }

    return DotDelimitedNamesCount;
}

ParseTree::SpecifierList *
ParseTree::SpecifierList::HasSpecifier
(
    unsigned TargetSpecifiers
)
{
    for (ParseTree::SpecifierList *Specifier = this; Specifier; Specifier = Specifier->Next)
    {
        if (Specifier->Element->Opcode & TargetSpecifiers)
        {
            return Specifier;
        }
    }

    return NULL;
}

ParseTree::ParameterSpecifierList *
ParseTree::ParameterSpecifierList::HasSpecifier
(
    unsigned TargetSpecifiers
)
{
    for (ParseTree::ParameterSpecifierList *Specifier = this; Specifier; Specifier = Specifier->Next)
    {
        if (Specifier->Element->Opcode & TargetSpecifiers)
        {
            return Specifier;
        }
    }

    return NULL;
}

inline ParseTree::Expression *
Parser::CreateBoolConst
(
    bool Value
)
{
    ParseTree::BooleanLiteralExpression *Result = new(m_TreeStorage) ParseTree::BooleanLiteralExpression;
    Result->Opcode = ParseTree::Expression::BooleanLiteral;
    Result->Value = Value;
    return Result;
}

// Produce an error message if the current token is not the expected TokenType.

inline bool
Parser::VerifyExpectedToken
(
    tokens TokenType,
    _Inout_ bool &ErrorInConstruct,
    bool EatNewLineAfterToken,
    ParseTree::ParseTreeNode * pNode
)
{    
    ThrowIfFalse(! EatNewLineAfterToken || pNode);
    
    if (m_CurrentToken->m_TokenType == TokenType)
    {
        GetNextToken();
        if (EatNewLineAfterToken)
        {
            EatNewLine();
        }
        return true;
    }
    else
    {
        return HandleUnexpectedToken(TokenType, ErrorInConstruct);
    }
}

// Produce an error message if the current token is not the expected TokenType.

inline bool
Parser::VerifyExpectedXmlToken
(
    tokens TokenType,
    _Inout_ bool &ErrorInConstruct
)
{
    if (m_CurrentToken->m_TokenType == TokenType)
    {
        GetNextXmlToken();
        return true;
    }
    else
    {
        return HandleUnexpectedToken(TokenType, ErrorInConstruct);
    }
}

// *********************************************************************
// *********************************************************************
//
// Public entry points to the parser ...
//
// *********************************************************************
// *********************************************************************

Parser::Parser
(
    NorlsAllocator *TreeStorage,
    Compiler *TheCompiler,
    CompilerHost *TheCompilerHost,
    bool IsXMLDocEnabled,
    LANGVERSION compilingLanguageVersion,
    bool InterpretationUnderDebugger,
    bool InterpretationUnderIntelliSense,
    bool InitializingFields
) :
    m_TreeStorage(*TreeStorage),
    m_IsXMLDocEnabled(IsXMLDocEnabled),
    m_EvaluatingConditionCompilationExpression(false)
#if LINQ_INTELLISENSE
    ,m_IsSelectExpression(false)
#endif LINQ_INTELLISENSE
    ,m_XmlRoots(NULL)
    ,m_XmlRootsEnd(&m_XmlRoots)
    ,m_SeenAnonymousTypeInitialization(false)
    ,m_SeenQueryExpression(false)
    ,m_SeenLambdaExpression(false)
#if IDE
    ,m_SeenXmlLiterals(false)
#endif
    ,m_CompilingLanguageVersion(compilingLanguageVersion)
    ,m_ForceMethodDeclLineState(false)
    ,m_pFirstMultilineLambdaBody(NULL)
    ,m_AllowGlobalNamespace(true)
{
    m_Conditionals.Init(TreeStorage);

    m_Compiler = TheCompiler;
    m_CompilerHost = TheCompilerHost;
    m_pStringPool = m_Compiler ? m_Compiler->GetStringPool() : NULL;

    m_LastInContexts = m_ScratchLastInContexts;
    m_MaxContextDepth = sizeof(m_ScratchLastInContexts) / sizeof(ParseTree::StatementList *); 
    m_ContextIndex = -1;

    // Setting AllowStatementsSpanningLineBreaks true allows newlines to
    // occur within statements without explict line continuation.
    // m_AllowStatementsSpanningLineBreaks = true;

    m_CommentListTarget = &m_CommentsForCurrentStatement;

    m_InterpretationUnderDebugger = InterpretationUnderDebugger;

    m_InterpretationUnderIntelliSense = InterpretationUnderIntelliSense;

    m_InitializingFields = InitializingFields;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDecls
*
* Purpose:
*     Build the parse tree for the declarations in a module. The
*     decls include the function definitions but no function bodies.
*
**********************************************************************/

HRESULT
Parser::ParseDecls
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    SourceFile *InputFile,
    _Deref_out_ ParseTree::FileBlockStatement **Result,
    _Inout_ NorlsAllocator *ConditionalCompilationSymbolsStorage,
    BCSYM_Container *ProjectLevelCondCompScope,
    _Out_opt_ BCSYM_Container **ConditionalCompilationConstants,
    _In_ LineMarkerTable *LineMarkerTableForConditionals
)
{
    TIMEBLOCK(TIME_ParserDecls);

    m_Conditionals.Init(&m_TreeStorage);


    // Set up the context for conditional compilation symbols.
    m_ConditionalCompilationSymbolsSource =
        new(*ConditionalCompilationSymbolsStorage)
        Symbols(m_Compiler, ConditionalCompilationSymbolsStorage, LineMarkerTableForConditionals);

    m_ConditionalConstantsScope = m_ConditionalCompilationSymbolsSource->AllocCCContainer(false, InputFile);

    // Clear the parent before (possibly) setting it to a (possibly) different value.
    Symbols::ClearParent(m_ConditionalConstantsScope);

    if (ProjectLevelCondCompScope)
    {
        Symbols::SetParent(m_ConditionalConstantsScope, ProjectLevelCondCompScope->GetHash());
    }

    m_ConditionalCompilationSymbolsSource->
        GetConditionalCompilationScope(m_ConditionalConstantsScope);

    VB_ENTRY();
    ParseTree::FileBlockStatement *File = NULL;

    InitParserFromScannerStream(InputStream, Errors);

    File = new(m_TreeStorage) ParseTree::FileBlockStatement;
    File->Opcode = ParseTree::Statement::File;
    File->IsFirstOnLine = true;
    EstablishContext(File);

    // No explicit termination occurs for the file statement, so
    // its termination is always implicitly OK.
    File->HasProperTermination = true;

    m_ExternalSourceDirectiveTarget = &File->SourceDirectives;

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);
    CacheStmtLocation();

    while (!m_CurrentToken->IsEndOfParse())
    {
        ParseLine(&Parser::ParseDeclarationStatement);
        GetTokensForNextLine(false);
    }

    RecoverFromMissingConditionalEnds();
    RecoverFromMissingEnds(File);

    VSASSERT(
        m_Context == File,
        "ParseDecls: context must end up as source file.");

    SetFileBlockLocation(File, m_CurrentToken);

    *Result = File;
    if (ConditionalCompilationConstants != NULL)
    {
        // 




        *ConditionalCompilationConstants = m_ConditionalConstantsScope;
    }

    if (m_CurrentExternalSourceDirective)
    {
        // The location info here is bogus, but that's probably OK since external
        // source directives are generated only by tools.
        bool ErrorInExternalSourceNesting = false;
        ReportSyntaxError(ERRID_ExpectedEndExternalSource, m_CurrentToken, ErrorInExternalSourceNesting);
    }

    m_ExternalSourceDirectiveTarget = NULL;

    // Clear the CC scope's parent (see VS RAID 231584)
    Symbols::ClearParent(m_ConditionalConstantsScope);

    VB_EXIT();
}

/*********************************************************************
*
* Function:
*     Parser::ParseMethodBody
*
* Purpose:
*     Given the tree for the proc def, build the parse trees for the
*     function body. The parse trees are added to the given proc def
*     node.
*
**********************************************************************/

HRESULT
Parser::ParseMethodBody
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    BCSYM_Container *ProjectLevelCondCompScope,
    BCSYM_Container *ConditionalCompilationConstants,
    ParseTree::Statement::Opcodes MethodBodyKind,
    _Out_opt_ ParseTree::MethodBodyStatement **Result
)
{
    TIMEBLOCK(TIME_ParserMethodBody); 

    bool MethodIsEmpty = true;

    m_Conditionals.Init(&m_TreeStorage);
    m_ConditionalConstantsScope = NULL;

    if (ConditionalCompilationConstants)
    {
        m_ConditionalConstantsScope = ConditionalCompilationConstants->PCCContainer();

        // Clear the parent before (possibly) setting it to a (possibly) different value.
        Symbols::ClearParent(m_ConditionalConstantsScope);

        if (ProjectLevelCondCompScope)
        {
            Symbols::SetParent(m_ConditionalConstantsScope, ProjectLevelCondCompScope->GetHash());
        }
    }

    VB_ENTRY();
    m_ParsingMethodBody = true;
    switch (MethodBodyKind)
    {
        case ParseTree::Statement::ProcedureBody:
            m_ExpectedExitOpcode = ParseTree::Statement::ExitSub;
            break;
        case ParseTree::Statement::FunctionBody:
            m_ExpectedExitOpcode = ParseTree::Statement::ExitFunction;
            break;
        case ParseTree::Statement::OperatorBody:
            m_ExpectedExitOpcode = ParseTree::Statement::ExitOperator;
            break;
        case ParseTree::Statement::PropertyGetBody:
        case ParseTree::Statement::PropertySetBody:
            m_ExpectedExitOpcode = ParseTree::Statement::ExitProperty;
            break;
        case ParseTree::Statement::AddHandlerBody:
        case ParseTree::Statement::RemoveHandlerBody:
        case ParseTree::Statement::RaiseEventBody:
            m_ExpectedExitOpcode = ParseTree::Statement::SyntaxError;
            break;
    }

    InitParserFromScannerStream(InputStream, Errors);

    // Create a method body if necessary.
    ParseTree::MethodBodyStatement *Proc = new(m_TreeStorage) ParseTree::MethodBodyStatement;
    Proc->Opcode = MethodBodyKind;
    Proc->IsFirstOnLine = true;

    // No explicit termination occurs for the method body statement, so
    // its termination is always implicitly OK.
    Proc->HasProperTermination = true;

    // Set it as our context.
    EstablishContext(Proc);
    m_Procedure = Proc;

    // Get the first line
    m_CurrentToken = NULL;
    GetTokensForNextLine(true);
    CacheStmtLocation();
    SetStmtBegLocation(Proc, m_CurrentToken);

    // Parse the body
    while (!m_CurrentToken->IsEndOfParse())
    {
        // Comments and preprocessor directives don't imply that there are contents in the function 
        if (m_CurrentToken->m_TokenType != tkREM && 
            m_CurrentToken->m_TokenType != tkXMLDocComment && 
            m_CurrentToken->m_TokenType != tkSharp &&
            m_CurrentToken->m_TokenType != tkEOL)
        {
            MethodIsEmpty = false;
        }

        ParseLine(&Parser::ParseStatementInMethodBody);

        // Get the next line
        GetTokensForNextLine(true);
    }

    // Dev10 #709157: previously, the token tkEOL was only returned for lines ending in a colon.
    // The scanner now (as of Dev10 621395) produces EOL for all lines, which is why it's
    // ignored in the test for MethodIsEmpty, above. This added test causes a method containing
    // only a label to be nonempty.
    if (MethodIsEmpty && m_LastLabel != NULL)
    {
        MethodIsEmpty = false;
    }

    // If there were any missing conditional ends, they would have been detected
    // during the declaration parse. If the method body ends within a conditional
    // section, attempting to detect missing conditional ends here can produce
    // extraneous errors.
    // RecoverFromMissingConditionalEnds();
    RecoverFromMissingEnds(Proc);

    VSASSERT(
        m_Context == Proc,
        "ParseMethodBody: context must end up as input proc.");

    VSASSERT(m_CurrentToken->m_TokenType == tkEOF, "Method body ends at a token other than EOF.");

    // The location information for a method body is different from other block
    // statements, in that it encompasses the entire block. Therefore, the body
    // location information is also different from other blocks--it is the same
    // as the method body statement--and so must be set manually.

    // The last token of the method body is typically not the EOF token
    // (because its location is the first character of text following the
    // method body), but if the method body is empty there's no other option.

    Token *LastTokenOfMethod =
        (m_CurrentToken->m_StartLine != Proc->TextSpan.m_lBegLine ||
         TokenColumnBeg(m_CurrentToken) != Proc->TextSpan.m_lBegColumn) ?
            m_CurrentToken->m_Prev :
            m_CurrentToken;

    Proc->TextSpan.m_lEndLine = LastTokenOfMethod->m_StartLine;
    Proc->TextSpan.m_lEndColumn = TokenColumnEnd(LastTokenOfMethod);
    Proc->BodyTextSpan.m_lBegLine = Proc->TextSpan.m_lBegLine;
    Proc->BodyTextSpan.m_lBegColumn = Proc->TextSpan.m_lBegColumn;
    Proc->BodyTextSpan.m_lEndLine = Proc->TextSpan.m_lEndLine;
    Proc->BodyTextSpan.m_lEndColumn = Proc->TextSpan.m_lEndColumn;
    Proc->IsEmpty = MethodIsEmpty;

    if (Result != NULL)
    {
        *Result = Proc;
    }

    m_LastLabel = NULL;
    m_Procedure = NULL;

    m_ParsingMethodBody = false;

    if (m_ConditionalConstantsScope)
    {
        // Clear the CC scope's parent (see VS RAID 231584)
        Symbols::ClearParent(m_ConditionalConstantsScope);
    }
    VB_EXIT();
}

/*********************************************************************
*
* Function:
*     Parser::ParseOneExpression
*
* Purpose:
*     Given the tree for the given expression
*
**********************************************************************/
HRESULT
Parser::ParseOneExpression
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _Deref_out_ ParseTree::Expression **Result,
    _Out_opt_ bool* ErrorInConstructRet,
    _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
    _In_opt_ BCSYM_Container *pConditionalCompilationConstants
)
{
    VB_ENTRY();

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Dev10 #789970 Set up proper conditional compilation scope
    SetupConditionalConstantsScopeForPartialParse ccs(this, pProjectLevelCondCompScope, pConditionalCompilationConstants); 

    // Get the first (and only) line.
    // Set previous token to tkEQ so that the scanner will allow Xml literal at start of scan
    m_CurrentToken = GetNextLine(tkEQ);

    bool ErrorInConstruct = false;
    *Result = ParseExpression(ErrorInConstruct);
    if (ErrorInConstructRet)
    {
        *ErrorInConstructRet = ErrorInConstruct;
    }

    if (!m_CurrentToken->IsEndOfLine())
    {
        if (m_Errors)
        {
            ReportSyntaxError(ERRID_ExpectedEndOfExpression, m_CurrentToken, ErrorInConstruct);
        }
    }

    return NOERROR;

    VB_EXIT();
}

#if HOSTED

/*********************************************************************
*
* Function:
*     Parser::ParseOneExpressionForHostedCompiler
*
* Purpose:
*    Creates a tree for an individual expression and also ensures
*    that the parsing of the expression has consumed the entire
*    input stream. If the stream has not ended (besides EOL) after
*    the expression has been parsed, then a parse error is reported.
*
**********************************************************************/
HRESULT
Parser::ParseOneExpressionForHostedCompiler
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _Deref_out_ ParseTree::Expression **Result,
    _Out_opt_ bool* ErrorInConstructRet
)
{
    VB_ENTRY();

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Get the first (and only) line.
    // Set previous token to tkEQ so that the scanner will allow Xml literal at start of scan
    m_CurrentToken = GetNextLine(tkEQ);

    bool ErrorInConstruct = false;
    *Result = ParseExpression(ErrorInConstruct);
    if (ErrorInConstructRet)
    {
        *ErrorInConstructRet = ErrorInConstruct;
    }

    // Ensure that there is no other text following the parsed expression, either on the same
    // line as the expression or on any subsequent lines. Report an error if any text found.

    while (!m_CurrentToken->IsEndOfParse())
    {
        if (m_CurrentToken->m_TokenType != tkEOL)
        {
            // if (m_Errors) - this check which is in ParseOneExpression is not needed.
            // This is handled more centrally by AddError. Additionally, not optimizing
            // by adding this check above the while loop since (m_Error == NULL) is the
            // uncommon case.

            Location ErrorLoc = m_CurrentToken->GetLocation();

            // Note that the ReportSyntaxError overload that takes a location arg is used,
            // rather than the ones that take Tokens for the following reasons:
            //
            // 1. When ":" in on a different line from the user expression, the Token based
            // overloads report the error at the beginning of the line containing the ":"
            // and not on the ":" itself.
            //
            //    eg: "a + b
            //               :" - here the token based overloads report the error at the
            //                      beginning of the second line, rather than on ":".
            //
            //
            // 2. When ":" is on the same line as the user expression, the Token based
            // overloads report the error at the end of the expression rather than on
            // the ":" itself.
            //
            //    eg: "a + b :" - here the token based overloads report the error on "b"
            //                      rather than on ":".
            //
            // The desired behavior for the hosted compiler is to report the error on the
            // ":" itself.
            //
            ReportSyntaxError(ERRID_ExpectedEndOfExpression, &ErrorLoc, ErrorInConstruct);
            break;
        }

        m_CurrentToken = GetNextLine();
    }

    VB_EXIT();
}

#endif HOSTED

ParseTree::Initializer *
Parser::ParseOneInitializer
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _In_opt_ BCSYM_Container *pProjectLevelCondCompScope, 
    _In_opt_ BCSYM_Container *pConditionalCompilationConstants 
)
{

    InitParserFromScannerStream(InputStream, Errors);

    BackupValue<bool> backupParsingMethodBody(&m_ParsingMethodBody);
    m_ParsingMethodBody = true; // Act as if we are parsing method body: don't create symbols for conditional compilation, etc.

    m_Conditionals.Init(&m_TreeStorage);

    SetupConditionalConstantsScopeForPartialParse ccs(this, pProjectLevelCondCompScope, pConditionalCompilationConstants); 

    // Get the first (and only) line.
    // Set previous token to tkEQ so that the scanner will allow Xml literal at start of scan
    m_CurrentToken = GetNextLine(tkEQ);

    bool ErrorInConstruct = false;
    ParseTree::Initializer * pRet =  ParseInitializer(ErrorInConstruct, false);

    return pRet;
}

ParseTree::ParenthesizedArgumentList
Parser::ParseParenthesizedArguments
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors
)
{
    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Get the first (and only) line.
    m_CurrentToken = GetNextLine();

    bool ErrorInConstruct = false;
    return ParseParenthesizedArguments(ErrorInConstruct);
}

void
Parser::ParseParenthesizedArgumentsAndObjectInitializer
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _Out_ ParseTree::ParenthesizedArgumentList *&ArgumentList,
    _Deref_out_ ParseTree::ObjectInitializerList *&ObjectInitializer,
    _Deref_out_ ParseTree::BracedInitializerList * & CollectionInitializer,
    _Out_ ParseTree::PunctuatorLocation & FromLocation,
    _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
    _In_opt_ BCSYM_Container *pConditionalCompilationConstants
)
{
    ArgumentList = NULL;
    ObjectInitializer = NULL;

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Dev10 #789970 Set up proper conditional compilation scope
    SetupConditionalConstantsScopeForPartialParse ccs(this, pProjectLevelCondCompScope, pConditionalCompilationConstants); 

    // Get the first (and only) line.
    Token * start = m_CurrentToken = GetNextLine();

    bool ErrorInConstruct = false;

    if (tkLParen == m_CurrentToken->m_TokenType)
    {
        ArgumentList = new(m_TreeStorage) ParseTree::ParenthesizedArgumentList;
        *ArgumentList = ParseParenthesizedArguments(ErrorInConstruct);
    }

    if (tkWITH == m_CurrentToken->m_TokenType)
    {
        ObjectInitializer = ParseObjectInitializerList(ErrorInConstruct);
    }
    else if (tkFROM == TokenAsKeyword(m_CurrentToken))
    {
        Token * FromToken = m_CurrentToken;

        SetPunctuator(&FromLocation, start, FromToken);
        
        GetNextToken(); // get off FROM
        EatNewLine(); // allow implicit line continuation after FROM (dev10_508839)
        
        CollectionInitializer = ParseInitializerList(
                ErrorInConstruct,
                true, // AllowExpressionInitializers
                false // AllowAssignmentInitializers
            );
    }
}

ParseTree::Expression *
Parser::ParseParenthesizedExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    ParseTree::Expression *Term;

    // "(" expr ")"
    GetNextToken();

    Term = new(m_TreeStorage) ParseTree::ParenthesizedExpression;
    Term->Opcode = ParseTree::Expression::Parenthesized;

    EatNewLine();
    
    Term->AsUnary()->Operand = ParseExpression(ErrorInConstruct);

    EatNewLineIfFollowedBy(tkRParen);
    
    if (VerifyExpectedToken(tkRParen, ErrorInConstruct) == false)   // if the right paren is missing
    {
        Term->AsParenthesized()->IsRightParenMissing= true;
    }
    SetExpressionLocation(Term, Start, m_CurrentToken, NULL);

    return Term;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDeclsAndMethodBodies
*
* Purpose:
*     Build the parse tree for the declarations in a module including
*     the function bodies.
*
**********************************************************************/

HRESULT
Parser::ParseDeclsAndMethodBodies
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _In_ LineMarkerTable *LineMarkers,
    _Out_ ParseTree::FileBlockStatement **Result,
    _Inout_ NorlsAllocator *ConditionalCompilationSymbolsStorage,
    BCSYM_Container *ProjectLevelCondCompScope,
    _Out_ BCSYM_Container **ConditionalCompilationConstants
)

{
    VB_ENTRY();

    m_Conditionals.Init(&m_TreeStorage);

    // Set up the context for conditional compilation symbols.
    m_ConditionalCompilationSymbolsSource =
        new(*ConditionalCompilationSymbolsStorage)
        Symbols(m_Compiler, ConditionalCompilationSymbolsStorage, LineMarkers);

    m_ConditionalConstantsScope = m_ConditionalCompilationSymbolsSource->AllocCCContainer(false, ProjectLevelCondCompScope->GetCompilerFile()); // Microsoft 

    VSASSERT(ProjectLevelCondCompScope, "");
    Symbols::SetParent(m_ConditionalConstantsScope, ProjectLevelCondCompScope);

    m_ConditionalCompilationSymbolsSource->
        GetConditionalCompilationScope(m_ConditionalConstantsScope);

    m_IsDeclAndMethodBodiesParse = true;

    InitParserFromScannerStream(InputStream, Errors);

    ParseTree::FileBlockStatement *File = new(m_TreeStorage) ParseTree::FileBlockStatement;
    File->Opcode = ParseTree::Statement::File;

    EstablishContext(File);

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);

    while (!m_CurrentToken->IsEndOfParse())
    {
        ParseLine(&Parser::ParseDeclarationStatement);
        GetTokensForNextLine(false);
    }

    RecoverFromMissingConditionalEnds();
    RecoverFromMissingEnds(File);

    VSASSERT(
        m_Context == File,
        "ParseDecls: context must end up as source file.");

    SetFileBlockLocation(File, m_CurrentToken);

    *Result = File;
    *ConditionalCompilationConstants = m_ConditionalConstantsScope;

    VB_EXIT_NORETURN();

    m_IsDeclAndMethodBodiesParse = false;

    return hr;
}

/*****************************************************************************
*
* Function:
*     Parser::ParserProjectLevelCondCompDecls
*
* Purpose:
*     Parse declarations of project level conditional compilation symbols.
*
******************************************************************************/

HRESULT Parser::ParseProjectLevelCondCompDecls
(
    _In_ Scanner *InputStream,
    _In_ Symbols *SymbolTable,
    _Inout_ BCSYM_Hash *LookupTable,
    ErrorTable *Errors
)
{
    HRESULT hr = NOERROR;
    Token *Start;
    STRING *SymbolName;
    NorlsAllocator Storage(NORLSLOC);

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    m_CurrentToken = GetNextLine();
    Start = m_CurrentToken;

    bool ErrorInConstruct = false;

    while (m_CurrentToken->m_TokenType == tkID)
    {
        SymbolName = m_CurrentToken->m_Id.m_Spelling;

        GetNextToken();

        //
        // Parse the symbol's expression value
        //

        ParseTree::Expression *Expr;
        ConstantValue Value;

        if (m_CurrentToken->m_TokenType != tkEQ)
        {
            Value.Integral = true;
            Value.SetVType(t_bool);
        }
        else
        {
            VerifyTokenPrecedesExpression(tkEQ, ErrorInConstruct);

            Expr = ParseConditionalCompilationExpression(ErrorInConstruct);

            //
            // Evaluate the symbol's expression value
            //

            bool ExpressionIsBad;

            Value =
                Semantics::InterpretConstantExpression(
                    Expr,
                    NULL,
                    LookupTable,
                    NULL,
                    &Storage,
                    Errors,
                    true,
                    m_Compiler,
                    m_CompilerHost,
                    &ExpressionIsBad);
        }

        //
        // Allocate space for the symbol's expression
        //

        BCSYM_Expression *ExpressionSymbol = NULL;

        ExpressionSymbol = SymbolTable->GetFixedExpression(&Value);

        //
        // Allocate space for the symbol if it does not yet exist
        //

        BCSYM_NamedRoot *OldSymbol = NULL;
        BCSYM_CCConstant *NewSymbol = NULL;

        OldSymbol = LookupTable->SimpleBind(SymbolName);
        while (OldSymbol != NULL)
        {
            if (OldSymbol->GetKind() == SYM_CCConstant &&
                OldSymbol->GetName() == SymbolName)
            {
                break;
            }
            OldSymbol = OldSymbol->GetNextInHash();
        }

        if (OldSymbol)
        {
            NewSymbol = OldSymbol->PCCConstant();
        }
        else
        {
            NewSymbol = SymbolTable->AllocCCConstant();
        }

        Location ConstantLocation;

        ConstantLocation.m_lBegLine = Start->m_StartLine;
        ConstantLocation.m_lBegColumn = Start->m_StartColumn;
        ConstantLocation.m_lEndLine = m_CurrentToken->m_StartLine;
        ConstantLocation.m_lEndColumn = m_CurrentToken->m_StartColumn;

        SymbolTable->GetVariable(
            &ConstantLocation,
            SymbolName,
            SymbolName,
            DECLF_Const | DECLF_Value | DECLF_Public,
            VAR_Const,
            Value.TypeCode == t_ref ?
                m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType) :
                m_CompilerHost->GetFXSymbolProvider()->GetType(Value.TypeCode),
            ExpressionSymbol,
            NULL,
            NewSymbol);

        if (OldSymbol == NULL)
        {
            Symbols::AddSymbolToHash(LookupTable, NewSymbol, true, false, false);
        }

        // 

        //
        // Check if there is any other symbol to continue parsing
        //

        if (m_CurrentToken->m_TokenType == tkEOF)
        {
            break;
        }
        else if (m_CurrentToken->m_TokenType == tkEOL)
        {
            m_CurrentToken = GetNextLine();
            Start = m_CurrentToken;
        }
        else if (m_CurrentToken->m_TokenType == tkColon ||
                 m_CurrentToken->m_TokenType == tkComma)
        {
            Start = GetNextToken();
            while(m_CurrentToken->m_TokenType == tkColon ||
                  m_CurrentToken->m_TokenType == tkComma ||
                  m_CurrentToken->m_TokenType == tkEOL)
            {
                Start = GetNextToken();
            }

        }
        else if (m_CurrentToken == m_FirstTokenAfterNewline)
        {
            // This is the start of a new line.
        }
        else
        {
            ReportSyntaxError
                (
                ERRID_ExpectedEOS,
                m_CurrentToken,
                (m_CurrentToken->m_Next)? m_CurrentToken->m_Next : m_CurrentToken,
                ErrorInConstruct);
        }
    }

    if (m_CurrentToken->m_TokenType != tkEOF)
    {
        // This is the case when we have a colon at the end of our buffer
        // or when the first token of a line is not an identifier.
        //
        ReportSyntaxError
            (
            ERRID_ExpectedIdentifier,
            m_CurrentToken,
            (m_CurrentToken->m_Next)? m_CurrentToken->m_Next : m_CurrentToken,
            ErrorInConstruct);
    }

    return hr;
};

static ParseTree::BlockStatement *
Clone
(
    _In_ ParseTree::BlockStatement *Block,
    _Inout_ NorlsAllocator &TreeStorage
)
{
    switch (Block->Opcode)
    {
        case ParseTree::Statement::Enum:
            return new(TreeStorage) ParseTree::EnumTypeStatement(*Block->AsEnumType());

        case ParseTree::Statement::Structure:
        case ParseTree::Statement::Interface:
        case ParseTree::Statement::Class:
        case ParseTree::Statement::Module:
            return new(TreeStorage) ParseTree::TypeStatement(*Block->AsType());

        case ParseTree::Statement::Namespace:
            return new(TreeStorage) ParseTree::NamespaceStatement(*Block->AsNamespace());

        case ParseTree::Statement::File:
            return new(TreeStorage) ParseTree::FileBlockStatement(*Block->AsFileBlock());

        case ParseTree::Statement::Property:
            return new(TreeStorage) ParseTree::PropertyStatement(*Block->AsProperty());

        case ParseTree::Statement::BlockEventDeclaration:
            return new(TreeStorage) ParseTree::BlockEventDeclarationStatement(*Block->AsBlockEventDeclaration());

        case ParseTree::Statement::ProcedureBody:
        case ParseTree::Statement::PropertyGetBody:
        case ParseTree::Statement::PropertySetBody:
        case ParseTree::Statement::FunctionBody:
        case ParseTree::Statement::OperatorBody:
        case ParseTree::Statement::AddHandlerBody:
        case ParseTree::Statement::RemoveHandlerBody:
        case ParseTree::Statement::RaiseEventBody:
            return new(TreeStorage) ParseTree::MethodBodyStatement(*Block->AsMethodBody());

        case ParseTree::Statement::BlockIf:
        case ParseTree::Statement::LineIf:
            return new(TreeStorage) ParseTree::IfStatement(*Block->AsIf());

        case ParseTree::Statement::ElseIf:
            return new(TreeStorage) ParseTree::ElseIfStatement(*Block->AsElseIf());

        case ParseTree::Statement::Select:
            return new(TreeStorage) ParseTree::SelectStatement(*Block->AsSelect());

        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
        case ParseTree::Statement::DoWhileBottomTest:
        case ParseTree::Statement::DoUntilBottomTest:
        case ParseTree::Statement::DoForever:
        case ParseTree::Statement::While:
        case ParseTree::Statement::With:
        case ParseTree::Statement::SyncLock:
            return new(TreeStorage) ParseTree::ExpressionBlockStatement(*Block->AsExpressionBlock());

        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::LineElse:
            return new(TreeStorage) ParseTree::ElseStatement(*Block->AsElse());

        case ParseTree::Statement::Finally:
            return new(TreeStorage) ParseTree::FinallyStatement(*Block->AsFinally());

        case ParseTree::Statement::Try:
        case ParseTree::Statement::CaseElse:
            return new(TreeStorage) ParseTree::ExecutableBlockStatement(*Block->AsExecutableBlock());

        case ParseTree::Statement::LambdaBody:
            return new(TreeStorage) ParseTree::LambdaBodyStatement(*Block->AsLambdaBody());
        case ParseTree::Statement::Using:
            return new(TreeStorage) ParseTree::UsingStatement(*Block->AsUsing());

        case ParseTree::Statement::Case:
            return new(TreeStorage) ParseTree::CaseStatement(*Block->AsCase());

        case ParseTree::Statement::Catch:
            return new(TreeStorage) ParseTree::CatchStatement(*Block->AsCatch());

        case ParseTree::Statement::ForFromTo:
            return new(TreeStorage) ParseTree::ForFromToStatement(*Block->AsForFromTo());

        case ParseTree::Statement::ForEachIn:
            return new(TreeStorage) ParseTree::ForEachInStatement(*Block->AsForEachIn());

        case ParseTree::Statement::Region:
            return new(TreeStorage) ParseTree::RegionStatement(*Block->AsRegion());

        case ParseTree::Statement::CommentBlock:
            return new(TreeStorage) ParseTree::CommentBlockStatement(*Block->AsCommentBlock());

        default:
            VSFAIL("Surprising parse tree block opcode.");
            break;
    }

    return NULL;
}

/*********************************************************************
*
* Function:
*     Parser::ParseOneLine
*
* Purpose:
*     Builds the parse trees for a given line.
*
**********************************************************************/
void
Parser::ParseOneLine
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _In_ ParseTree::BlockStatement *Context,
    _Out_ ParseTree::StatementList **Result,
    _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
    _In_opt_ BCSYM_Container *pConditionalCompilationConstants
)
{
    StatementParsingMethod StatementParser;
    bool WithinMethod;

    switch (Context->Opcode)
    {
        case ParseTree::Statement::Module:
        case ParseTree::Statement::Class:
        case ParseTree::Statement::Structure:
        case ParseTree::Statement::Enum:
        case ParseTree::Statement::Interface:
        case ParseTree::Statement::Namespace:
        case ParseTree::Statement::File:
        case ParseTree::Statement::Property:
        case ParseTree::Statement::BlockEventDeclaration:

            StatementParser = &Parser::ParseDeclarationStatement;
            WithinMethod = false;
            break;

        default:

            StatementParser = &Parser::ParseStatementInMethodBody;
            WithinMethod = true;
            break;
    }

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Dev10 #789970 Set up proper conditional compilation scope
    SetupConditionalConstantsScopeForPartialParse ccs(this, pProjectLevelCondCompScope, pConditionalCompilationConstants); 

    // In order to prevent mutating the context, allocate a new context block.
    // This needs to be a full and accurate clone--various clients traverse
    // the cloned block.

    ParseTree::BlockStatement *ContextClone = Clone(Context, m_TreeStorage);
    ContextClone->Children = NULL;

    EstablishContext(ContextClone);

    m_LastStatementLinked = NULL;

    if (Errors == NULL)
    {
        DisableErrors();
    }

    m_IsLineParse = true;

    m_CurrentToken = NULL;
    GetTokensForNextLine(WithinMethod);

    ParseLine(StatementParser);

    *Result = ContextClone->Children;
}


/*********************************************************************
*
* Function:
*     Parser::ParseOnePropertyOrEventProcedureDefinition
*
* Purpose:
*     Builds the parse trees for one Get, Set, AddHandler, RemoveHandler, RaiseEvent.
*
**********************************************************************/
void
Parser::ParseOnePropertyOrEventProcedureDefinition
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _In_ ParseTree::BlockStatement *Context,
    _Out_ ParseTree::StatementList **Result,
    _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
    _In_opt_ BCSYM_Container *pConditionalCompilationConstants
)
{

    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Dev10 #789970 Set up proper conditional compilation scope
    SetupConditionalConstantsScopeForPartialParse ccs(this, pProjectLevelCondCompScope, pConditionalCompilationConstants); 

    // In order to prevent mutating the context, allocate a new context block.
    // This needs to be a full and accurate clone--various clients traverse
    // the cloned block.

    ParseTree::BlockStatement *ContextClone = Clone(Context, m_TreeStorage);
    ContextClone->Children = NULL;

    EstablishContext(ContextClone);

    m_LastStatementLinked = NULL;

    if (Errors == NULL)
    {
        DisableErrors();
    }

    m_IsLineParse = true;

    m_CurrentToken = NULL;
    GetTokensForNextLine(false /*WithinMethod*/);
    Token *Start = m_CurrentToken;
    bool ErrorInConstruct = false;
    CacheStmtLocation();
    ParsePropertyOrEventProcedureDefinition(Start, ErrorInConstruct);
    *Result = ErrorInConstruct ? NULL : ContextClone->Children;
}


/*********************************************************************
*
* Function:
*     Parser::ParseName
*
* Purpose: Will parse a dot qualified or unqualified name
*
*          Ex: class1.proc
*
*          This overloaded version is exposed to the outside world
*
**********************************************************************/
ParseTree::Name *
Parser::ParseName
(
    _Out_ bool &ParseError,
    _In_z_ const WCHAR *NameToParse,
    bool AllowGlobalNameSpace,
    bool AllowGenericArguments,
    bool *pHasTrailingTokens
)
{
    VSASSERT(
        NameToParse != NULL,
        "NameToParse can't be NULL.");

    // Note: this ErrorTable doesn't have a compiler project associated with it.
    // Its errors will not be reported and adding errors to it will not cause
    // cause a task-list update. This should be ok since that calling function
    // will report the error base on the ParseError out param.
    ErrorTable Errors(m_Compiler, NULL, NULL);

    bool ErrorInConstruct = false;
    Scanner NameTokens(m_pStringPool, NameToParse, wcslen(NameToParse), 0, 0, 0); // Microsoft: heads up - my names have no location so I send zeros.

    m_Errors = &Errors;
    m_InputStream = &NameTokens;

    m_Conditionals.Init(&m_TreeStorage);

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);
    ParseTree::Name *ParseTree = ParseName(false, ErrorInConstruct, AllowGlobalNameSpace, AllowGenericArguments);
    ParseError = Errors.HasErrors();

    if (pHasTrailingTokens)
    {
        *pHasTrailingTokens = !m_CurrentToken->IsEndOfLine();
    }

    return ParseTree;
}

ParseTree::Type *
Parser::ParseTypeName
(
    _Out_ bool &ParseError,
    _In_z_ const WCHAR *NameToParse
)
{
    VSASSERT(
        NameToParse != NULL,
        "NameToParse can't be NULL.");

    // Note: this ErrorTable doesn't have a compiler project associated with it.
    // Its errors will not be reported and adding errors to it will not cause
    // cause a task-list update. This should be ok since that calling function
    // will report the error base on the ParseError out param.
    ErrorTable Errors(m_Compiler, NULL, NULL);

    bool ErrorInConstruct = false;
    Scanner NameTokens(m_pStringPool, NameToParse, wcslen(NameToParse), 0, 0, 0); // Microsoft: heads up - my names have no location so I send zeros.

    InitParserFromScannerStream(&NameTokens, &Errors);

    m_Conditionals.Init(&m_TreeStorage);

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);
    ParseTree::Type *ParseTree = ParseTypeName(ErrorInConstruct);
    ParseError = Errors.HasErrors() ? true : false;

    return ParseTree;
}

ParseTree::Name *
Parser::ParseName
(
    _In_ Location *Loc,
    _In_count_(cchName) const WCHAR *Name,
    size_t cchName,
    _Out_opt_ bool *ParseError,
    bool AllowGlobalNameSpace,
    bool AllowGenericArguments
)
{
    ErrorTable Errors(m_Compiler, NULL, NULL);

    bool ErrorInConstruct = false;
    Scanner NameTokens(m_pStringPool, Name, cchName, 0, Loc->m_lBegLine, Loc->m_lBegColumn);

    InitParserFromScannerStream(&NameTokens, &Errors);

    m_Conditionals.Init(&m_TreeStorage);

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);
    ParseTree::Name *ParseTree = ParseName(false, ErrorInConstruct, AllowGlobalNameSpace, AllowGenericArguments);

    if (ParseError)
    {
        *ParseError = Errors.HasErrors();

        // If there are more tokens to consume, then the parser didn't parse all tokens, which means
        // that the text is not a good name. See VSWhidbey 188916.
        if (m_CurrentToken && !m_CurrentToken->IsEndOfLine())
        {
            *ParseError = true;
        }
    }

    return ParseTree;
}

ParseTree::Type *
Parser::ParseGeneralType
(
    _In_ Location *Loc,
    _In_count_(cchTypeText) const WCHAR *TypeText,
    size_t cchTypeText,
    _Out_opt_ bool *ParseError,
    bool AllowEmptyGenericArguments /* = false */
)
{
    ErrorTable Errors(m_Compiler, NULL, NULL);

    bool ErrorInConstruct = false;
    Scanner TypeTokens(m_pStringPool, TypeText, cchTypeText, 0, Loc->m_lBegLine, Loc->m_lBegColumn);

    InitParserFromScannerStream(&TypeTokens, &Errors);

    m_Conditionals.Init(&m_TreeStorage);

    m_CurrentToken = NULL;
    GetTokensForNextLine(false);
    ParseTree::Type *ParseTree = ParseGeneralType(ErrorInConstruct, AllowEmptyGenericArguments);

    if (ParseError)
    {
        *ParseError = Errors.HasErrors() || !m_CurrentToken->IsEndOfLine();
    }

    return ParseTree;
}

//
//============ Methods for parsing general syntactic constructs. =======
//

/*********************************************************************
*
* Function:
*     Parser::ParseDeclarationStatement
*
* Purpose:
*     Parse a declaration statement, at file, namespace, or type level.
*     Current token should be set to current stmt to be parsed.
*
**********************************************************************/
void
Parser::ParseDeclarationStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Statement *Stmt = NULL;

    Token *Start = m_CurrentToken;

    CacheStmtLocation();

    bool StatementRulesOutOption = true;
    bool StatementRulesOutImports = true;
    bool StatementRulesOutAttribute = true;

    // Before parsing any new declaration statement, decide whether or not we want
    // to terminate a currently existing comment block context.
    EndCommentBlockIfNeeded();

    if (!BeginsEmptyStatement(Start))
    {
        ParseTree::BlockStatement *Context = RealContext();
   
        if (Context->Opcode == ParseTree::Statement::Enum)
        {
            ParseEnumGroupStatement(ErrorInConstruct);
            m_ProhibitOptionStatement = true;
            m_ProhibitImportsStatement = true;
            m_ProhibitAttributeStatement = true;
            return;
        }
        else if (Context->Opcode == ParseTree::Statement::Interface)
        {
            ParseInterfaceGroupStatement(ErrorInConstruct);
            m_ProhibitOptionStatement = true;
            m_ProhibitImportsStatement = true;
            m_ProhibitAttributeStatement = true;
            return;
        }                
                
        else if (Context->Opcode == ParseTree::Statement::Property ||
                 Context->Opcode == ParseTree::Statement::BlockEventDeclaration)
        { 
            ParsePropertyOrEventGroupStatement(ErrorInConstruct);
            m_ProhibitOptionStatement = true;
            m_ProhibitImportsStatement = true;
            m_ProhibitAttributeStatement = true;
            return;
        }
    }

    switch (Start->m_TokenType)
    {
        case tkLT:
        {
            // If the attribute specifier includes "Module" or "Assembly",
            // it's a standalone attribute statement. Otherwise, it is
            // associated with a particular declaration.

            Token * pNext = Start->m_Next;

            if (pNext->IsContinuableEOL())
            {
                pNext = PeekNextLine(pNext);
            }

            if ((pNext->m_TokenType == tkID && IdentifierAsKeyword(pNext) == tkASSEMBLY) ||
                pNext->m_TokenType == tkMODULE)
            {
                // Attribute statements can appear only at file level before any
                // declarations or option statements.

                if (m_ProhibitAttributeStatement)
                {
                    ReportSyntaxError(
                        ERRID_AttributeStmtWrongOrder,
                        m_CurrentToken,
                        ErrorInConstruct);
                }

                Stmt = new(m_TreeStorage) ParseTree::AttributeStatement;
                Stmt->Opcode = ParseTree::Statement::Attribute;

                SetStmtBegLocation(Stmt, m_CurrentToken);

                Stmt->AsAttribute()->Attributes = ParseAttributeSpecifier(FileAttribute, ErrorInConstruct);

                SetStmtEndLocation(Stmt, m_CurrentToken);

                StatementRulesOutAttribute = false;

                break;
            }
            __fallthrough; // Fall through.
        }    

            
        case tkPRIVATE:
        case tkPROTECTED:
        case tkPUBLIC:
        case tkFRIEND:

        case tkMUSTINHERIT:
        case tkNOTOVERRIDABLE:
        case tkOVERRIDABLE:
        case tkMUSTOVERRIDE:
        case tkNOTINHERITABLE:

        case tkPARTIAL:

        case tkSTATIC:
        case tkSHARED:
        case tkSHADOWS:
        case tkWITHEVENTS:

        case tkOVERLOADS:
        case tkOVERRIDES:

        case tkCONST:
        case tkDIM:

        case tkREADONLY:
        case tkWRITEONLY:

        case tkWIDENING:
        case tkNARROWING:

        case tkDEFAULT:

        case tkASYNC:
        case tkITERATOR:

            Stmt = ParseSpecifierDeclaration(ErrorInConstruct);
            break;

        case tkENUM:
            Stmt = ParseEnumStatement(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkINHERITS:
        case tkIMPLEMENTS:
            Stmt = ParseInheritsImplementsStatement(ErrorInConstruct);
            break;

        case tkIMPORTS:

            if (m_ProhibitImportsStatement)
            {
                ReportSyntaxError(
                    ERRID_ImportsMustBeFirst,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            Stmt = ParseImportsStatement(ErrorInConstruct);

            StatementRulesOutImports = false;
            StatementRulesOutAttribute = false;

            break;

        case tkNAMESPACE:
        {
            ParseTree::BlockStatement *Context = RealContext();

            if (Context->Opcode != ParseTree::Statement::Namespace &&
                Context->Opcode != ParseTree::Statement::File)
            {
                ReportSyntaxError(
                    ERRID_NamespaceNotAtNamespace,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            Stmt = ParseNamespaceStatement(m_CurrentToken, ErrorInConstruct);
            break;
        }
        case tkMODULE:
        {
            ParseTree::BlockStatement *Context = RealContext();

            if (Context->Opcode != ParseTree::Statement::Namespace &&
                Context->Opcode != ParseTree::Statement::File)
            {
                ReportSyntaxError(
                    ERRID_ModuleNotAtNamespace,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            __fallthrough; // Fall through.
        }
        case tkCLASS:
        case tkSTRUCTURE:
        case tkINTERFACE:

            Stmt = ParseTypeStatement(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkDECLARE:
            Stmt = ParseProcDeclareStatement(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkCUSTOM:
            VSASSERT(m_CurrentToken->m_Next->m_TokenType == tkEVENT, "tkCUSTOM in wrong context!!!");
            __fallthrough; // Custom event. Fall through

        case tkEVENT:
            Stmt = ParseEventDefinition(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkDELEGATE:
            Stmt = ParseDelegateStatement(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkSUB:
        case tkFUNCTION:
        case tkOPERATOR:

            // These end the module level declarations and begin
            // the procedure definitions.

            ParseProcedureDefinition(NULL, NULL, m_CurrentToken, ErrorInConstruct);
            break;

        case tkPROPERTY:

            Stmt = ParsePropertyDefinition(NULL, 
                NULL, 
                m_CurrentToken, 
                ErrorInConstruct, 
                false);
            break;

        case tkREM:
        case tkXMLDocComment:

            // Comments in declaration context (non-method body) build comment blocks

            if (m_Context->Opcode != ParseTree::Statement::CommentBlock)
            {
                Stmt = CreateCommentBlockStatement(m_CurrentToken);
                LinkStatement(Stmt);
            }

            __fallthrough;

        case tkColon:
        case tkEOL:

            // Statement separator. If we see this, then that means
            // there is an empty statement. Create an empty statement.

            Stmt = new(m_TreeStorage) ParseTree::Statement;
            Stmt->Opcode = ParseTree::Statement::Empty;

            SetStmtBegLocation(Stmt, m_CurrentToken);
            SetStmtEndLocation(Stmt, m_CurrentToken->m_Next);

            StatementRulesOutAttribute = false;
            StatementRulesOutOption = false;
            StatementRulesOutImports = false;

            break;

        case tkID:

            // Enables better error for wrong uses of the "Custom" modifier
            if (TokenAsKeyword(m_CurrentToken) == tkCUSTOM)
            {
                Stmt = ParseSpecifierDeclaration(ErrorInConstruct);
            }
            else
            {
                Stmt =
                    ReportUnrecognizedStatementError(
                        IdentifierAsKeyword(m_CurrentToken) == tkTYPE ?
                            // "Type" is now "Structure"
                            ERRID_ObsoleteStructureNotType :
                            ERRID_ExpectedDeclaration,
                       ErrorInConstruct);
            }
            break;

        case tkEND:

            ParseGroupEndStatement(ErrorInConstruct);
            break;

        case tkOPTION:

            // Option statements can appear only at file level before any
            // declarations.

            if (m_ProhibitOptionStatement)
            {
                ReportSyntaxError(
                    ERRID_OptionStmtWrongOrder,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            Stmt = ParseOptionStatement(ErrorInConstruct);

            StatementRulesOutOption = false;
            StatementRulesOutImports = false;
            StatementRulesOutAttribute = false;

            break;

        case tkREDIM:
        case tkGOTO:
        case tkCASE:
        case tkSELECT:
        case tkIF:
        case tkELSE:
        case tkELSEIF:
        case tkDO:
        case tkLOOP:
        case tkWHILE:
        case tkFOR:
        case tkCONTINUE:
        case tkEXIT:
        case tkERASE:
        case tkMID:
        case tkTRY:
        case tkCATCH:
        case tkFINALLY:
        case tkTHROW:
        case tkSYNCLOCK:
        case tkUSING:
        case tkWITH:
        case tkON:
        case tkERROR:
        case tkSTOP:
        case tkRESUME:
        case tkRETURN:
            Stmt = ReportUnrecognizedStatementError(ERRID_ExecutableAsDeclaration, ErrorInConstruct);
            break;

        default:

            Stmt = ReportUnrecognizedStatementError(ERRID_Syntax, ErrorInConstruct);
            break;
    }

    if (Stmt)
    {
        LinkStatement(Stmt);

        if (Stmt->Opcode == ParseTree::Statement::Property &&
             Stmt->AsProperty()->Specifiers->HasSpecifier(ParseTree::Specifier::MustOverride) )
        {
            // An abstruct property statement is a block construct and
            // so has been established as the current context. It will
            // not be explicitly ended, so pop it.

            PopContext(true, m_CurrentToken);
        }
        else
        {
            if (m_Context->Opcode == ParseTree::Statement::CommentBlock)
            {
                VSASSERT(Stmt->Opcode == ParseTree::Statement::Empty, "Empty Statement expected!");

                // The Empty statements inside Comment Blocks inherit the "IsFirstOnLine" flag. This flag makes
                // sense only for the children of the CommentBlock, but not for the Block itself.
                Stmt->IsFirstOnLine = m_Context->IsFirstOnLine;
            }
        }
    }

    if (!m_ProhibitAttributeStatement)
    {
        if (StatementRulesOutOption)
        {
            m_ProhibitOptionStatement = true;
        }

        if (StatementRulesOutImports)
        {
            m_ProhibitImportsStatement = true;
        }

        if (StatementRulesOutAttribute)
        {
            m_ProhibitAttributeStatement = true;
        }

    }
}

// Test whether a given token is a relational operator.

bool
IsRelationalOperator
(
    _In_ Token *TokenToTest
)
{
    tokens TokenType = TokenToTest->m_TokenType;

    return TokenType >= tkLT && TokenType <= tkGE;
}

/*********************************************************************
*
* Function:
*     Parser::ParseStatementInMethodBody
*
* Purpose:
*     Parses a statement that can occur inside a method body.
*
**********************************************************************/
void
Parser::ParseStatementInMethodBody
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    VSASSERT(m_pFirstMultilineLambdaBody || m_LambdaBodyStatements.Count() == 0,"parsing stmt in method body. should have no leftover lambdabodies");

    CacheStmtLocation();
    
    if (m_SelectBeforeFirstCase)
    {
        switch (Start->m_TokenType)
        {
            case tkCASE:
            case tkEND:
            case tkREM:
            case tkXMLDocComment:
            case tkEOL:
            case tkColon:
                break;

            default:
                ReportSyntaxError(ERRID_ExpectedCase, Start, ErrorInConstruct);
        }
    }

    switch (Start->m_TokenType)
    {
        case tkGOTO:
        {
            ParseGotoStatement(ErrorInConstruct);
            break;
        }
        case tkCASE:
        {
            // 

            m_SelectBeforeFirstCase = false;

            // Close an existing CASE block.

            if (m_Context->Opcode == ParseTree::Statement::Case)
            {
                PopContext(true, Start);
            }
            else if (m_Context->Opcode == ParseTree::Statement::CaseElse)
            {
                ReportSyntaxError(ERRID_CaseAfterCaseElse, m_CurrentToken, ErrorInConstruct);
                PopContext(true, Start);
            }

            if (m_Context->Opcode == ParseTree::Statement::Select)
            {
                if (m_CurrentToken->m_Next->m_TokenType == tkELSE)
                {
                    GetNextToken(); // get off CASE
                    GetNextToken(); // get off ELSE

                    ParseTree::ExecutableBlockStatement *CaseElse = new(m_TreeStorage) ParseTree::ExecutableBlockStatement;
                    CaseElse->Opcode = ParseTree::Statement::CaseElse;

                    SetStmtBegLocation(CaseElse, Start);
                    SetStmtEndLocation(CaseElse, m_CurrentToken);

                    LinkStatement(CaseElse);
                    break;
                }

                ParseTree::CaseStatement *CaseStatement = new(m_TreeStorage) ParseTree::CaseStatement;
                CaseStatement->Opcode = ParseTree::Statement::Case;

                SetStmtBegLocation(CaseStatement, Start);

                // Target is where to link in the next case node.
                ParseTree::CaseList **Target = &CaseStatement->Cases;
                bool firstTimeThrough = true;
                
                do
                {
                    GetNextToken(); // get off CASE and onto the expression
                    EatNewLineIfNotFirst(firstTimeThrough);
                    Token *StartCase = m_CurrentToken; // dev10_500588 Snap the start of the expression token AFTER we've moved off the EOL (if one is present)
                    ParseTree::Case *Case = NULL;
                    Token *Punctuator = NULL;
                    ParseTree::PunctuatorLocation *PunctLoc = NULL;

                    if (StartCase->m_TokenType == tkIS || IsRelationalOperator(StartCase))
                    {
                        ParseTree::RelationalCase *Relational = new(m_TreeStorage) ParseTree::RelationalCase;
                        Relational->Opcode = ParseTree::Case::Relational;

                        if (StartCase->m_TokenType == tkIS)
                        {
                            SetPunctuator(&Relational->Is, StartCase, StartCase);
                            GetNextToken(); // Get off IS
                            EatNewLine(); // dev10_526560 Allow implicit newline after IS
                        }

                        if (IsRelationalOperator(m_CurrentToken))
                        {
                            Punctuator = m_CurrentToken;
                            Relational->RelationalOpcode = GetBinaryOperator(m_CurrentToken, NULL);
                            PunctLoc = &Relational->Operator;
                            GetNextToken(); // get off relational operator
                            EatNewLine(); // dev10_503248

                            ParseTree::Expression *CaseExpr = ParseExpression(ErrorInConstruct);

                            if (ErrorInConstruct)
                            {
                                ResyncAt(0);
                            }

                            Relational->Value = CaseExpr;
                        }
                        else
                        {
                            // Since we saw IS, create a relational case.
                            // This helps intellisense do a drop down of
                            // the operators that can follow "Is".

                            Relational->RelationalOpcode = ParseTree::Expression::SyntaxError;

                            ReportSyntaxError(ERRID_ExpectedRelational, m_CurrentToken, ErrorInConstruct);
                            ResyncAt(0);
                        }

                        Case = Relational;
                    }
                    else
                    {
                        ParseTree::Expression *Left = ParseExpression(ErrorInConstruct);
                        if (ErrorInConstruct)
                        {
                            ResyncAt(1, tkTO);
                        }

                        if (m_CurrentToken->m_TokenType == tkTO)
                        {
                            Punctuator = m_CurrentToken;
                            GetNextToken();
                            ParseTree::Expression *Right = ParseExpression(ErrorInConstruct);
                            if (ErrorInConstruct)
                            {
                                ResyncAt(0);
                            }

                            ParseTree::RangeCase *Range = new(m_TreeStorage) ParseTree::RangeCase;
                            Range->Opcode = ParseTree::Case::Range;
                            Range->Low = Left;
                            Range->High = Right;
                            PunctLoc = &Range->To;

                            Case = Range;
                        }
                        else
                        {
                            ParseTree::ValueCase *Value = new(m_TreeStorage) ParseTree::ValueCase;
                            Value->Opcode = ParseTree::Case::Value;
                            Value->Value = Left;

                            Case = Value;
                        }
                    }

                    SetLocationAndPunctuator(
                        &Case->TextSpan,
                        PunctLoc,
                        StartCase,
                        m_CurrentToken,
                        Punctuator);

                    CreateListElement(
                        Target,
                        Case,
                        StartCase,
                        m_CurrentToken,
                        m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

                } while (m_CurrentToken->m_TokenType == tkComma);

                FixupListEndLocations(CaseStatement->Cases);
                SetStmtEndLocation(CaseStatement, m_CurrentToken);
                LinkStatement(CaseStatement);
            }
            else
            {
                LinkStatement(
                    ReportUnrecognizedStatementError(
                        (m_CurrentToken->m_Next->m_TokenType == tkELSE) ?
                            ERRID_CaseElseNoSelect :
                            ERRID_CaseNoSelect,
                        ErrorInConstruct));
            }
            break;
        }

        case tkSELECT:
        {
            // 

            ParseTree::SelectStatement *Select = new(m_TreeStorage) ParseTree::SelectStatement;
            Select->Opcode = ParseTree::Statement::Select;
            SetStmtBegLocation(Select, Start);
            GetNextToken(); // get off SELECT

            // Allow the expected CASE token to be present or not.
            if (m_CurrentToken->m_TokenType == tkCASE)
            {
                SetPunctuator(&Select->Case, Start, m_CurrentToken);
                GetNextToken();
            }

            ParseTree::Expression *Expr = ParseExpression(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            Select->Operand = Expr;
            m_SelectBeforeFirstCase = true;
            SetStmtEndLocation(Select, m_CurrentToken);
            LinkStatement(Select);

            break;
        }

        case tkWITH:
        {
            ParseExpressionBlockStatement(ParseTree::Statement::With, ErrorInConstruct);
            break;
        }

        case tkWHILE:
        {
            ParseExpressionBlockStatement(ParseTree::Statement::While, ErrorInConstruct);
            break;
        }

        case tkUSING:
        {
            //ParseExpressionBlockStatement(ParseTree::Statement::Using, ErrorInConstruct);
            ParseTree::UsingStatement *Result = new(m_TreeStorage) ParseTree::UsingStatement;
            Result->Opcode = ParseTree::Statement::Using;
            Token *StmtStart = m_CurrentToken;
            SetStmtBegLocation(Result, m_CurrentToken);

            GetNextToken();
            if
            (
               this->m_CurrentToken->m_Next->m_TokenType == tkAS ||
               this->m_CurrentToken->m_Next->m_TokenType == tkEQ ||
               this->m_CurrentToken->m_Next->m_TokenType == tkNullable
            )
            {
                // 'using' control variables has no specifier. Make a temporary fake specifier to keep
                // ParseVarDeclStatement running. After the parse delete the specifier to avoid confusing
                // the pretty lister
                ParseTree::Specifier Specifier;
                Specifier.Opcode = ParseTree::Specifier::Dim;
                SetLocation(&Specifier.TextSpan, StmtStart, StmtStart);
                Specifier.TextSpan.SetLocationToHidden();

                ParseTree::SpecifierList ListElement;
                ListElement.Element = &Specifier;
                ListElement.Next = NULL;

                Result->ControlVariableDeclaration = ParseVarDeclStatement(NULL, &ListElement, StmtStart, ErrorInConstruct);
                Result->ControlVariableDeclaration->AsVariableDeclaration()->Specifiers = NULL;

                if (Result->ControlVariableDeclaration)
                {
                    // DevDiv 731208: The control variable declaration's span should not include the "using"
                    SetLocation(&Result->ControlVariableDeclaration->TextSpan, StmtStart->m_Next, m_CurrentToken);
                    UpdateStatementLinqFlags(Result->ControlVariableDeclaration);
                }
            }
            else
            {
                Result->ControlExpression = ParseExpression(ErrorInConstruct);
            }
            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            SetStmtEndLocation(Result, m_CurrentToken);
            LinkStatement(Result);
            break;
        }

        case tkSYNCLOCK:
        {
            ParseExpressionBlockStatement(ParseTree::Statement::SyncLock, ErrorInConstruct);
            break;
        }

        case tkTRY:
        {
            ParseTry();
            break;
        }

        case tkCATCH:
        {
            ParseCatch(ErrorInConstruct);
            break;
        }

        case tkFINALLY:
        {
            ParseFinally(ErrorInConstruct);
            break;
        }

        case tkIF:
        {
            // 

            bool fIsLineIf;
            ParseTree::ExpressionBlockStatement *If = ParseIfConstruct(NULL, &fIsLineIf, ErrorInConstruct);

            if (fIsLineIf)
            {
                GreedilyParseColonSeparatedStatements(ErrorInConstruct);

                // If the statements in the if clause did not leave the
                // context where it started, detect this and fix up the context.

                if (FindContainingContext(If))
                {
                    RecoverFromMissingEnds(If);

                    VSASSERT(m_Context == If, "Line if parsing must end in a matching context.");

                    Token *Next = m_CurrentToken;

                    PopContext(true, Next);

                    if (Next->m_TokenType == tkELSE)
                    {
                        // Create the new context.
                        ParseTree::ElseStatement *Else = new(m_TreeStorage) ParseTree::ElseStatement;
                        Else->Opcode = ParseTree::Statement::LineElse;
                        Else->ContainingIf = If->AsIf();
                        CacheStmtLocation();
                        SetStmtBegLocation(Else, Next);

                        LinkStatement(Else);

                        GetNextToken();
                        SetStmtEndLocation(Else, m_CurrentToken);

                        if (!IsValidStatementTerminator(m_CurrentToken))
                        {
                            ParseStatementInMethodBody(ErrorInConstruct);

                            while (m_CurrentToken->m_TokenType == tkColon)
                            {
                                GetNextToken();
                                ParseStatementInMethodBody(ErrorInConstruct);
                            }
                        }

                        if (FindContainingContext(Else))
                        {
                            RecoverFromMissingEnds(Else);

                            VSASSERT(m_Context == Else, "Line if parsing must end in a matching context.");

                            PopContext(true, m_CurrentToken);
                        }
                    }

                    // Update the statement location cache with what should
                    // be the token following the line if statement.  There is
                    // no actual 'END' statement, so use the location information
                    // of the// EOL/EOF/Colon/REM.
                    CacheStmtLocation();

                    ParseTree::Statement *EndIf = new(m_TreeStorage) ParseTree::EndBlockStatement;
                    EndIf->Opcode = ParseTree::Statement::EndIf;
                    SetStmtBegLocation(EndIf, m_CurrentToken);
                    SetStmtEndLocation(EndIf, m_CurrentToken->m_Next);
                    LinkStatement(EndIf);

                    // End the If block.
                    If->TerminatingConstruct = EndIf;
                }
            }

            break;
        }

        case tkELSE:
        {
            if (Start->m_Next->m_TokenType != tkIF)
            {
                // 

                ParseTree::Statement::Opcodes Context = m_Context->Opcode;

                if (Context == ParseTree::Statement::BlockIf ||
                    Context == ParseTree::Statement::ElseIf)
                {
                    ParseTree::BlockStatement *PreviousContext = m_Context;
                    PopContext(Context == ParseTree::Statement::ElseIf, Start);

                    // Create the new context.
                    ParseTree::ElseStatement *Else = new(m_TreeStorage) ParseTree::ElseStatement;
                    Else->Opcode = ParseTree::Statement::BlockElse;
                    Else->ContainingIf =
                        Context == ParseTree::Statement::BlockIf ?
                            PreviousContext->AsIf() :
                            PreviousContext->AsElseIf()->ContainingIf;
                    SetStmtBegLocation(Else, Start);

                    GetNextToken();
                    SetStmtEndLocation(Else, m_CurrentToken);

                    LinkStatement(Else);

                    if (!IsValidStatementTerminator(m_CurrentToken))
                    {
                        ParseStatementInMethodBody(ErrorInConstruct);
                    }
                }
                else if (Context == ParseTree::Statement::LineIf)
                {
                    // If the context is a line if, return without consuming
                    // or creating anything--the line if parse will handle
                    // the else.
                }
                else
                {
                    LinkStatement(ReportUnrecognizedStatementError(ERRID_ElseNoMatchingIf, ErrorInConstruct));
                }

                break;
            }

            else
            {
                // Treat Else If as ElseIf. (ParseIfConstruct will consume the leading Else.)
            }

            __fallthrough; // Fall through.
        }

        case tkELSEIF:
        {
            // 

            ParseTree::Statement::Opcodes Context = m_Context->Opcode;

            if (Context == ParseTree::Statement::BlockIf ||
                Context == ParseTree::Statement::ElseIf)
            {
                ParseTree::BlockStatement *PreviousContext = m_Context;
                PopContext(Context == ParseTree::Statement::ElseIf, Start);

                // Create the new context.
                bool fIsLineIf;
                ParseIfConstruct(
                    Context == ParseTree::Statement::BlockIf ?
                        PreviousContext->AsIf() :
                        PreviousContext->AsElseIf()->ContainingIf,
                    &fIsLineIf,
                    ErrorInConstruct);
            }
            else if (Context == ParseTree::Statement::LineIf)
            {
            }
            else
            {
                if (m_IsLineParse)
                {
                    // Parse the statement so that Intellisense will work,
                    // even though the Else If does not appear in the context of an If.

                    ReportSyntaxError(ERRID_ElseIfNoMatchingIf, m_CurrentToken, ErrorInConstruct);

                    bool fIsLineIf;
                    ParseIfConstruct(
                        NULL,
                        &fIsLineIf,
                        ErrorInConstruct);
                }
                else
                {
                    // Interpreting an Else If outside the context of an If gives semantic
                    // analysis headaches, so don't parse the statement during normal
                    // compilation.

                    LinkStatement(ReportUnrecognizedStatementError(ERRID_ElseIfNoMatchingIf, ErrorInConstruct));
                }
            }

            // End location will be set when the block ends.

            break;
        }

        case tkDO:
        {
            // 

            ParseTree::ExpressionBlockStatement *Do;
            if (m_CurrentToken->m_Next->m_TokenType == tkWHILE ||
                (m_CurrentToken->m_Next->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken->m_Next) == tkUNTIL))
            {
                Do = new(m_TreeStorage) ParseTree::TopTestDoStatement;
                SetPunctuator(&Do->AsTopTestDo()->WhileOrUntil, Start, m_CurrentToken->m_Next);
            }
            else
            {
                Do = new(m_TreeStorage) ParseTree::ExpressionBlockStatement;
            }

            SetStmtBegLocation(Do, Start);

            // Consume the Do.
            GetNextToken();

            bool IsWhile;

            ParseTree::Expression *Expr = ParseOptionalWhileUntilClause(&IsWhile, ErrorInConstruct);

            if (Expr)
            {
                Do->Operand = Expr;
                Do->Opcode = IsWhile ? ParseTree::Statement::DoWhileTopTest : ParseTree::Statement::DoUntilTopTest;
            }
            else
            {
                Do->Opcode = ParseTree::Statement::DoForever;
            }

            SetStmtEndLocation(Do, m_CurrentToken);

            LinkStatement(Do);

            break;
        }

        case tkLOOP:
        {
            // 

            GetNextToken();

            if (m_Context->Opcode == ParseTree::Statement::DoWhileTopTest ||
                m_Context->Opcode == ParseTree::Statement::DoUntilTopTest ||
                m_Context->Opcode == ParseTree::Statement::DoForever)
            {
                if (!IsValidStatementTerminator(m_CurrentToken) &&
                    m_Context->Opcode != ParseTree::Statement::DoForever)
                {
                    // Error: the loop has a condition in both header and trailer.
                    ReportSyntaxError(ERRID_LoopDoubleCondition, m_CurrentToken, ErrorInConstruct);
                    ResyncAt(0);
                }

                Token *WhileOrUntil = m_CurrentToken;
                bool IsWhile = false;

                ParseTree::Expression *Expr = ParseOptionalWhileUntilClause(&IsWhile, ErrorInConstruct);

                if (Expr)
                {
                    m_Context->AsExpressionBlock()->Operand = Expr;
                    m_Context->Opcode = IsWhile ? ParseTree::Statement::DoWhileBottomTest : ParseTree::Statement::DoUntilBottomTest;
                }

                // The statement group is ended, so take it off the context stack.
                ParseTree::Statement *Loop =
                    EndContext(
                        Start,
                        Expr ?
                            (IsWhile ?
                                ParseTree::Statement::EndLoopWhile :
                                ParseTree::Statement::EndLoopUntil) :
                            ParseTree::Statement::EndLoop,
                        Expr);

                if (Expr)
                {
                    SetPunctuator(&Loop->AsBottomTestLoop()->WhileOrUntil, Start, WhileOrUntil);
                }
            }
            else
            {
                RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndLoop, ErrorInConstruct);
            }

            break;
        }

        case tkFOR:
        {
            // 

            // Consume the FOR.
            GetNextToken();

            ParseTree::ExecutableBlockStatement *ForBlock;

            if (m_CurrentToken->m_TokenType == tkEACH)
            {
                ParseTree::ForEachInStatement *For = new(m_TreeStorage) ParseTree::ForEachInStatement;
                For->Opcode = ParseTree::Statement::ForEachIn;

                SetPunctuator(&For->Each, Start, m_CurrentToken);
                GetNextToken();

                ParseTree::Expression *Each = ParseForLoopControlVariable(For,
                                                                          Start,
                                                                          &For->ControlVariableDeclaration,
                                                                          ErrorInConstruct);

                ParseTree::Expression *In = NULL;

                if (ErrorInConstruct)
                {
                    ResyncAt(1, tkIN);
                }

                EatNewLineIfFollowedBy(tkIN);
                if (m_CurrentToken->m_TokenType == tkIN)
                {
                    SetPunctuator(&For->In, Start, m_CurrentToken);
                    GetNextToken();

                    EatNewLine();

                    In = ParseExpression(ErrorInConstruct);

                    if (ErrorInConstruct)
                    {
                        ResyncAt(0);
                    }
                }
                else
                {
                    In = ExpressionSyntaxError();
                    SetExpressionLocation(In, m_CurrentToken, m_CurrentToken, NULL);

                    ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);
                    ResyncAt(0);
                }

                For->ControlVariable = Each;
                For->Collection = In;
                ForBlock = For;
            }
            else
            {
                ParseTree::ForFromToStatement *For = new(m_TreeStorage) ParseTree::ForFromToStatement;
                For->Opcode = ParseTree::Statement::ForFromTo;

                ParseTree::Expression *ControlVariable = ParseForLoopControlVariable(For,
                                                                                     Start,
                                                                                     &For->ControlVariableDeclaration,
                                                                                     ErrorInConstruct);

                ParseTree::Expression *From = NULL;
                ParseTree::Expression *To = NULL;
                ParseTree::Expression *Step = NULL;

                if (ErrorInConstruct)
                {
                    ResyncAt(2, tkEQ, tkTO);
                }

                if (m_CurrentToken->m_TokenType == tkEQ)
                {
                    SetPunctuator(&For->Equals, Start, m_CurrentToken);
                    GetNextToken();
                    
                    EatNewLine(); // Dev10_545918 - Allow implicit line contiuation after '=' 

                    From = ParseExpression(ErrorInConstruct);

                    if (ErrorInConstruct)
                    {
                        ResyncAt(1, tkTO);
                    }
                }
                else
                {
                    From = ExpressionSyntaxError();
                    SetExpressionLocation(From, m_CurrentToken, m_CurrentToken, NULL);

                    ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);
                    ResyncAt(1, tkTO);
                }

                if (m_CurrentToken->m_TokenType == tkTO)
                {
                    SetPunctuator(&For->To, Start, m_CurrentToken);
                    GetNextToken();

                    To = ParseExpression(ErrorInConstruct);

                    if (ErrorInConstruct)
                    {
                        ResyncAt(1, tkSTEP);
                    }
                }
                else
                {
                    To = ExpressionSyntaxError();
                    SetExpressionLocation(To, m_CurrentToken, m_CurrentToken, NULL);

                    ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);
                    ResyncAt(1, tkSTEP);
                }

                if (m_CurrentToken->m_TokenType == tkSTEP)
                {
                    SetPunctuator(&For->Step, Start, m_CurrentToken);
                    GetNextToken();

                    Step = ParseExpression(ErrorInConstruct);

                    if (ErrorInConstruct)
                    {
                        ResyncAt(0);
                    }
                }

                For->ControlVariable = ControlVariable;
                For->InitialValue = From;
                For->FinalValue = To;
                For->IncrementValue = Step;

                ForBlock = For;
            }

            SetStmtBegLocation(ForBlock, Start);
            SetStmtEndLocation(ForBlock, m_CurrentToken);

            LinkStatement(ForBlock);

            break;
        }

        case tkNEXT:
        {
            // 

            Token *Following = m_CurrentToken->m_Next;
            ParseTree::ExpressionList *NextVariables = NULL;
            ParseTree::ExpressionList **NextTarget = &NextVariables;
            ParseTree::BlockStatement *LastContextEnded = NULL;
            ParseTree::BlockStatement *StartContext = m_Context;

            if (IsValidStatementTerminator(Following))
            {
                GetNextToken();

                if (m_Context->Opcode == ParseTree::Statement::ForFromTo ||
                    m_Context->Opcode == ParseTree::Statement::ForEachIn)
                {
                    LastContextEnded = m_Context;
                    PopContext(true, Start);
                }
                else
                {
                    RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndNext, ErrorInConstruct);
                }
            }
            else
            {
                bool first = true;
                
                do
                {
                    GetNextToken();
                    EatNewLineIfNotFirst(first);

                    if (m_Context->Opcode == ParseTree::Statement::ForFromTo ||
                        m_Context->Opcode == ParseTree::Statement::ForEachIn)
                    {
                        Token *VariableStart = m_CurrentToken;
                        ParseTree::Expression *Variable = ParseVariable(ErrorInConstruct);

                        if (ErrorInConstruct)
                        {
                            ResyncAt(0);
                        }

                        // Attach the parsed variable to the context tree.
                        m_Context->AsFor()->NextVariable = Variable;                       

                        CreateListElement(
                            NextTarget,
                            Variable,
                            VariableStart,
                            m_CurrentToken,
                            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

                        LastContextEnded = m_Context;
                        PopContext(true, Start);
                    }
                    else if (LastContextEnded)
                    {
                        ReportSyntaxError(
                            ERRID_ExtraNextVariable,
                            m_CurrentToken,
                            ErrorInConstruct);

                        ResyncAt(0);
                    }
                    else
                    {
                        RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndNext, ErrorInConstruct);
                        ResyncAt(0);
                    }
                } while (m_CurrentToken->m_TokenType == tkComma);
            }

            if (LastContextEnded)
            {
                FixupListEndLocations(NextVariables);

                ParseTree::EndNextStatement *EndNext = new(m_TreeStorage) ParseTree::EndNextStatement;
                EndNext->Opcode = ParseTree::Statement::EndNext;
                EndNext->Variables = NextVariables;

                SetStmtBegLocation(EndNext, Start);
                SetStmtEndLocation(EndNext, m_CurrentToken);

                LinkStatement(EndNext);

                unsigned short ResumeIndex = EndNext->ResumeIndex;

                for (ParseTree::BlockStatement *IntermediateContext = StartContext;
                     IntermediateContext != LastContextEnded;
                     IntermediateContext = IntermediateContext->GetParent())
                {
                    IntermediateContext->TerminatingConstruct = EndNext;
                    IntermediateContext->AsFor()->ResumeIndexForEndNext = ResumeIndex++;
                }

                LastContextEnded->TerminatingConstruct = EndNext;
                LastContextEnded->AsFor()->ResumeIndexForEndNext = ResumeIndex;
            }

            break;
        }

        case tkENDIF:
        {
            // EndIf is anachronistic

            ReportSyntaxErrorWithoutMarkingStatementBad(
                ERRID_ObsoleteEndIf,
                m_CurrentToken,
                ErrorInConstruct);

            GetNextToken();

            ParseTree::Statement::Opcodes Context = m_Context->Opcode;

            if (Context == ParseTree::Statement::BlockIf ||
                Context == ParseTree::Statement::ElseIf ||
                Context == ParseTree::Statement::BlockElse)
            {
                EndContext(Start, ParseTree::Statement::EndIf, NULL);
            }
            else
            {
                RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndIf, ErrorInConstruct);
            }

            break;
        }

        case tkEND:
        {
            // Dev10#708061
            // "End" is a keyword which takes an optional next argument. Things get confusing with "End Select"...
            // This might come from "Dim x = From i In Sub() End Select i". But Dev10 spec says "inside the body
            // of a single-line sub, we attempt to parse one statement greedily".
            //
            // * Therefore we treat this Select as part of an "End Select" construct (which will make the above statement
            //   an error), and not as part of the query (which would make the above statement work).
            //
            // The confusion never arose in Orcas. That's because the set of tokens which could come after an End in
            // a compound GroupEndStatement was disjoint from the set of tokens that could come after a statement.
            // Now in Dev10, in the case of a single-line sub, there's just one point of contention: "Select"
            // (A complete list of End constructs: End If, ExternalSource, Region, Namespace, Module, Enum, Structure, Interface,
            // Class, Sub, Operator, Enum, AddHandler, RemoveHandler, RaiseEvent, Property, Get, Set, With, SyncLock, Select,
            // Using, While, Try. I got this list from the "VBGrammar" tool in src\vb\language\tools\VBGrammar. Of thee,
            // Select is the only token that can follow an expression.)
            //
            // A beautiful bugfix would change the code to say: "First try to parse the following token as a compound
            // GroupEndStatement. If that fails, then try to parse it as a statement-following-thing (e.g. :, EOL, comment,
            // an "Else" in the context of a line-else, or a thing-that-follows-expression in the context of a single-line sub).
            // But since "Select" is the solitary point of contention, I'll go for a uglier smaller fix:
            if (CanFollowStatement(m_CurrentToken->m_Next) &&
                !(IsParsingSingleLineLambda() && m_CurrentToken->m_Next->m_TokenType == tkSELECT))
            {
                ParseTree::Statement *End = new(m_TreeStorage) ParseTree::Statement;
                End->Opcode = ParseTree::Statement::End;
                GetNextToken();
                SetStmtBegLocation(End, Start);
                SetStmtEndLocation(End, m_CurrentToken);

                LinkStatement(End);

                break;
            }

            ParseGroupEndStatement(ErrorInConstruct);

            break;
        }

        case tkWEND:
        {
            // While...Wend are anachronistic

            ReportSyntaxErrorWithoutMarkingStatementBad(
                ERRID_ObsoleteWhileWend,
                m_CurrentToken,
                ErrorInConstruct);

            GetNextToken();

            if (m_Context->Opcode == ParseTree::Statement::While)
            {
                EndContext(Start, ParseTree::Statement::EndWhile, NULL);
            }
            else
            {
                RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndWhile, ErrorInConstruct);
            }

            break;
        }

        case tkYIELD:
            AssertLanguageFeature(FEATUREID_Iterators, m_CurrentToken);
            __fallthrough; // fall through
        case tkRETURN:
        {

            ParseReturnOrYieldStatement(Start, ErrorInConstruct);

            break;
        }
        case tkSTOP:
        {
            ParseTree::Statement *Statement = new(m_TreeStorage) ParseTree::Statement;
            Statement->Opcode = ParseTree::Statement::Stop;

            GetNextToken();

            SetStmtBegLocation(Statement, Start);
            SetStmtEndLocation(Statement, m_CurrentToken);
            LinkStatement(Statement);

            break;
        }

        case tkCONTINUE:
        {
            ParseContinueStatement(Start, ErrorInConstruct);
            break;
        }

        case tkEXIT:
        {
            // 

            ParseTree::Statement::Opcodes ExitOpcode = ParseTree::Statement::SyntaxError;

            switch (GetNextToken()->m_TokenType)
            {
                case tkDO:
                    ExitOpcode = ParseTree::Statement::ExitDo;
                    break;
                case tkFOR:
                    ExitOpcode = ParseTree::Statement::ExitFor;
                    break;
                case tkWHILE:
                    ExitOpcode = ParseTree::Statement::ExitWhile;
                    break;
                case tkSELECT:
                    ExitOpcode = ParseTree::Statement::ExitSelect;
                    break;

                // The pretty lister is expected to turn Exit statements
                // that don't exit the correct kind of method into Exit
                // statements that do. That requires identifying this
                // condition during parsing and correcting the parse trees.

                case tkSUB:
                    if (m_ParsingMethodBody || m_pStatementLambdaBody)
                    {
                        if (m_ExpectedExitOpcode != ParseTree::Statement::ExitSub)
                        {
                            ReportSyntaxError(
                                ERRID_ExitSubOfFunc,
                                Start,
                                m_CurrentToken->m_Next,
                                m_ExpectedExitOpcode == ParseTree::Statement::SyntaxError,
                                ErrorInConstruct);
                        }
                        ExitOpcode = m_ExpectedExitOpcode;
                    }
                    else
                    {
                        ExitOpcode = ParseTree::Statement::ExitSub;
                    }
                    break;
                case tkFUNCTION:
                    if (m_ParsingMethodBody || m_pStatementLambdaBody)
                    {
                        if (m_ExpectedExitOpcode != ParseTree::Statement::ExitFunction)
                        {
                            ReportSyntaxError(
                                ERRID_ExitFuncOfSub,
                                Start,
                                m_CurrentToken->m_Next,
                                m_ExpectedExitOpcode == ParseTree::Statement::SyntaxError,
                                ErrorInConstruct);
                        }
                        ExitOpcode = m_ExpectedExitOpcode;
                    }
                    else
                    {
                        ExitOpcode = ParseTree::Statement::ExitFunction;
                    }
                    break;
                case tkPROPERTY:
                    if (m_ParsingMethodBody)
                    {
                        if (m_ExpectedExitOpcode != ParseTree::Statement::ExitProperty)
                        {
                            ReportSyntaxError(
                                ERRID_ExitPropNot,
                                Start,
                                m_CurrentToken->m_Next,
                                m_ExpectedExitOpcode == ParseTree::Statement::SyntaxError,
                                ErrorInConstruct);
                        }
                        ExitOpcode = m_ExpectedExitOpcode;
                    }
                    else
                    {
                        ExitOpcode = ParseTree::Statement::ExitProperty;
                    }
                    break;
                case tkTRY:
                    ExitOpcode = ParseTree::Statement::ExitTry;
                    break;

                case tkOPERATOR:
                case tkADDHANDLER:
                case tkREMOVEHANDLER:
                case tkRAISEEVENT:

                    if (m_ParsingMethodBody)
                    {
                        // "Exit Operator" is not a valid exit kind, but we keep track of when
                        // a user might be expected to use it to give a smarter error.
                        // "'Exit Operator' is not valid. Use 'Return' to exit an Operator."
                        // Or
                        // "'Exit AddHandler', 'Exit RemoveHandler' and 'Exit RaiseEvent' are not valid. Use 'Return' to exit from event members."

                        ReportSyntaxError(
                            m_CurrentToken->m_TokenType == tkOPERATOR ?
                                ERRID_ExitOperatorNotValid :
                                ERRID_ExitEventMemberNotInvalid,
                            Start,
                            m_CurrentToken->m_Next,
                            m_ExpectedExitOpcode == ParseTree::Statement::SyntaxError,
                            ErrorInConstruct);

                        ExitOpcode = m_ExpectedExitOpcode;
                        break;
                    }

                    __fallthrough; // fall through
                default:
                {
                    ExitOpcode =
                        IsValidStatementTerminator(m_CurrentToken) ?
                            ParseTree::Statement::ExitUnknown :
                            ParseTree::Statement::ExitInvalid;

                    ReportSyntaxError(ERRID_ExpectedExitKind, m_CurrentToken, ErrorInConstruct);
                    ResyncAt(0);
                }
            }

            if (ExitOpcode != ParseTree::Statement::ExitUnknown &&
                ExitOpcode != ParseTree::Statement::ExitInvalid)
            {
                GetNextToken();
            }

            ParseTree::Statement *Exit = new(m_TreeStorage) ParseTree::Statement;
            Exit->Opcode = ExitOpcode;

            SetStmtBegLocation(Exit, Start);
            SetStmtEndLocation(Exit, m_CurrentToken);

            LinkStatement(Exit);
            break;
        }

        case tkON:
        {
            ParseOnErrorStatement(ErrorInConstruct);
            break;
        }

        case tkRESUME:
        {
            ParseResumeStatement(ErrorInConstruct);
            break;
        }

        case tkCALL:
        {
            ParseCallStatement(ErrorInConstruct);
            break;
        }

        case tkRAISEEVENT:
        {
            ParseRaiseEventStatement(ErrorInConstruct);
            break;
        }

        case tkREDIM:
        {
            ParseRedimStatement(ErrorInConstruct);
            break;
        }

        case tkADDHANDLER:
        case tkREMOVEHANDLER:
        {
            ParseHandlerStatement(ErrorInConstruct);
            break;
        }

        case tkPRIVATE:
        case tkPROTECTED:
        case tkPUBLIC:
        case tkFRIEND:

        case tkNOTOVERRIDABLE:
        case tkOVERRIDABLE:
        case tkMUSTOVERRIDE:

        case tkSTATIC:
        case tkSHARED:
        case tkSHADOWS:
        case tkWITHEVENTS:

        case tkOVERLOADS:
        case tkOVERRIDES:

        case tkCONST:
        case tkDIM:

        case tkWIDENING:
        case tkNARROWING:

        case tkREADONLY:
        case tkWRITEONLY:
        case tkLT:
        {
            ParseTree::AttributeSpecifierList *Attributes = NULL;

            if (Start->m_TokenType == tkLT)
            {
                Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
            }

            ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInConstruct);

            // VS 314714
            // When we have specifiers or attributes preceding an invalid method declaration,
            // we want an error of ERRID_InvInsideEndsProc reported, so that this file is decompiled
            // to no state.  This will remove the task list error (no state) added in FindEndProc.
            // Without this fix, an invalid identifier error will be reported, and the file will
            // not decompile far enough.  The task list error will incorrectly hang around.
            //
            // Note: This bug addresses the requirement that this method (ParseStatementInMethodBody)
            //       should report ERRID_InvInsideEndsProc in exactly the same cases that FindEndProc
            //       does
            if (!(Specifiers->HasSpecifier(ParseTree::Specifier::Dim | ParseTree::Specifier::Const)) &&
                (m_CurrentToken->m_TokenType == tkSUB ||
                 m_CurrentToken->m_TokenType == tkFUNCTION ||
                 m_CurrentToken->m_TokenType == tkOPERATOR ||
                 m_CurrentToken->m_TokenType == tkPROPERTY ||
                 BeginsEvent(m_CurrentToken)))
            {
                 LinkStatement(ReportUnrecognizedStatementError(ERRID_InvInsideEndsProc, ErrorInConstruct));
            }
            else
            {
                ParseTree::Statement *Decl = ParseVarDeclStatement(Attributes, Specifiers, Start, ErrorInConstruct);

                if (Decl)
                    LinkStatement(Decl);
            }

            break;
        }

        case tkSET:
            ParseAssignmentStatement(ErrorInConstruct);
            break;
        case tkLET:
            ParseAssignmentStatement(ErrorInConstruct);
            break;

        case tkERROR:
        {
            GetNextToken();

            ParseTree::ExpressionStatement *Error = new(m_TreeStorage) ParseTree::ExpressionStatement;
            Error->Opcode = ParseTree::Statement::Error;

            Error->Operand = ParseExpression(ErrorInConstruct);

            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            SetStmtBegLocation(Error, Start);
            SetStmtEndLocation(Error, m_CurrentToken);

            LinkStatement(Error);

            break;
        }

        case tkTHROW:
        {
            GetNextToken();

            ParseTree::ExpressionStatement *Throw = new(m_TreeStorage) ParseTree::ExpressionStatement;
            Throw->Opcode = ParseTree::Statement::Throw;

            if (!IsValidStatementTerminator(m_CurrentToken))
            {
                Throw->Operand = ParseExpression(ErrorInConstruct);

                if (ErrorInConstruct)
                {
                    ResyncAt(0);
                }
            }

            SetStmtBegLocation(Throw, Start);
            SetStmtEndLocation(Throw, m_CurrentToken);

            LinkStatement(Throw);

            break;
        }

        case tkID:
        {
            // Check for a non-reserved keyword that can start
            // a special syntactic construct. Such identifiers are treated as keywords
            // unless the statement looks like an assignment statement.

            tokens IDToken = IdentifierAsKeyword(Start);

            if (IDToken == tkMID)
            {
                switch (Start->m_Next->m_TokenType)
                {
                    case tkEQ:
                    case tkConcatEQ:
                    case tkMultEQ:
                    case tkPlusEQ:
                    case tkMinusEQ:
                    case tkDivEQ:
                    case tkIDivEQ:
                    case tkPwrEQ:
                    case tkShiftLeftEQ:
                    case tkShiftRightEQ:
                        break;

                    default:
                        ParseMid(ErrorInConstruct);
                        return;
                }
            }

            __fallthrough; // Fall through.
        }
        case tkDot:
        case tkBang:
        case tkMYBASE:
        case tkMYCLASS:
        case tkME:
        case tkGLOBAL:

        case tkSHORT:
        case tkUSHORT:
        case tkINTEGER:
        case tkUINTEGER:
        case tkLONG:
        case tkULONG:
        case tkDECIMAL:
        case tkSINGLE:
        case tkDOUBLE:
        case tkSBYTE:
        case tkBYTE:
        case tkBOOLEAN:
        case tkCHAR:
        case tkDATE:
        case tkSTRING:
        case tkVARIANT:
        case tkOBJECT:

        case tkDIRECTCAST:
        case tkTRYCAST:
        case tkCTYPE:
        case tkCBOOL:
        case tkCDATE:
        case tkCDBL:
        case tkCSBYTE:
        case tkCBYTE:
        case tkCCHAR:
        case tkCSHORT:
        case tkCUSHORT:
        case tkCINT:
        case tkCUINT:
        case tkCLNG:
        case tkCULNG:
        case tkCSNG:
        case tkCSTR:
        case tkCDEC:
        case tkCOBJ:

        case tkGETTYPE:
        case tkGETXMLNAMESPACE:

        {
            ParseTree::Expression *Var = ParseTerm(ErrorInConstruct);

            if (ErrorInConstruct)
            {
                ResyncAt(1, tkEQ);
            }

            Token *Next = m_CurrentToken;

            // Could be a function call or it could be an assignment
            if (IsAssignmentOperator(Next))
            {
                ParseTree::AssignmentStatement *Assign = new(m_TreeStorage) ParseTree::AssignmentStatement;
                Assign->Opcode = GetAssignmentOperator(Next);

                Assign->Target = Var;

                SetStmtBegLocation(Assign, Start);

                // Consume the assignment operator
                SetPunctuator(&Assign->Operator, Start, m_CurrentToken);
                GetNextToken();

                EatNewLine();

                Assign->Source = ParseExpression(ErrorInConstruct);
                if (ErrorInConstruct)
                {
                    // Sync to avoid other errors
                    ResyncAt(0);
                }

                SetStmtEndLocation(Assign, m_CurrentToken);
                LinkStatement(Assign);
            }

            else
            {
                ParseTree::CallStatement *Call = CreateCallStatement(Start, Var);

                if (!CanEndExecutableStatement(m_CurrentToken) &&
                    Call->Arguments.Values == NULL &&
                    m_CurrentToken->m_TokenType != tkSyntaxError &&
                    // VS320205
                    Call->Target->Opcode !=  ParseTree::Expression::CastBoolean &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastCharacter &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastDate &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastDouble &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastSignedByte &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastByte &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastShort &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastUnsignedShort &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastInteger &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastUnsignedInteger &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastLong &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastUnsignedLong &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastDecimal &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastSingle &&
                    Call->Target->Opcode !=  ParseTree::Expression::CastString)
                {
                    // This is likely an old-style call. Diagnose the
                    // absence of parentheses and act as if they were present.

                    ReportSyntaxErrorWithoutMarkingStatementBad(
                        ERRID_ObsoleteArgumentsNeedParens,
                        m_CurrentToken,
                        ErrorInConstruct);

                    // A non-parenthesized argument list cannot contain
                    // a newline.

                    ParseArguments(&Call->Arguments.Values, ErrorInConstruct);

                    SetStmtEndLocation(Call, m_CurrentToken);
                }

                LinkStatement(Call);
            }

            break;
        }

        case tkEOL:
            break; // The scanner now (as of dev10 621395) produces EOL for all blank lines.

        case tkXMLDocComment:
            if (m_IsXMLDocEnabled)
            {
                // XMLDoc comments are not allowed inside method bodies, generate an error
                // and build a normal comment statement.
                ReportSyntaxErrorWithoutMarkingStatementBad(
                    WRNID_XMLDocInsideMethod,
                    m_CurrentToken,
                    ErrorInConstruct);
            }
            __fallthrough;

        case tkREM:
        case tkColon:
        {
            // Statement separator. If we see this, then that means
            // there is an empty statement. Create an empty statement.

            ParseTree::Statement *Empty = new(m_TreeStorage) ParseTree::Statement;
            Empty->Opcode = ParseTree::Statement::Empty;
            SetStmtBegLocation(Empty, Start);
            SetStmtEndLocation(Empty, Start->m_Next);

            LinkStatement(Empty);

            break;
        }

        case tkERASE:

            ParseErase(ErrorInConstruct);
            break;

        case tkAWAIT:

            ParseAwaitStatement(ErrorInConstruct);
            break;

        case tkINHERITS:
        case tkIMPLEMENTS:
        case tkOPTION:
        case tkDECLARE:
        case tkDELEGATE:

            LinkStatement(ReportUnrecognizedStatementError(ERRID_InvInsideProc, ErrorInConstruct));
            break;

        case tkGET:

            LinkStatement(ReportUnrecognizedStatementError(
                m_CompilerHost->IsStarliteHost() ?
                    ERRID_NoSupportGetStatement : ERRID_ObsoleteGetStatement ,
                ErrorInConstruct));
            break;

        case tkGOSUB:

            LinkStatement(ReportUnrecognizedStatementError(ERRID_ObsoleteGosub, ErrorInConstruct));
            break;

        case tkINTERFACE:
        case tkPROPERTY:
        case tkSUB:
        case tkFUNCTION:
        case tkOPERATOR:
        case tkCUSTOM:
            // Fall through
        case tkEVENT:
        case tkNAMESPACE:
        case tkCLASS:
        case tkSTRUCTURE:
        case tkENUM:
        case tkMODULE:

            LinkStatement(ReportUnrecognizedStatementError(ERRID_InvInsideEndsProc, ErrorInConstruct));
            break;

        default:

            if (IsParsingSingleLineLambda() && CanFollowStatement(Start))
            {
                // It's an error for a single-statement lambda to be empty, e.g. "Console.WriteLine(Sub())"
                // But we're not in the best position to report that error, because we don't know span locations &c.
                // So what we'll do is return an empty statement. Inside ParseStatementLambda it catches the case
                // where the first statement is empty and reports an error. It also catches the case where the
                // first statement is non-empty and is followed by a colon. Therefore, if we encounter this
                // branch we're in right now, then we'll definitely return to ParseStatementLambda / single-line,
                // and we'll definitely report a good and appropriate error. The error won't be lost!
                ParseTree::Statement *Empty = new(m_TreeStorage) ParseTree::Statement;
                Empty->Opcode = ParseTree::Statement::Empty;
                SetStmtBegLocation(Empty, m_CurrentToken);
                SetStmtEndLocation(Empty, m_CurrentToken->m_Next);
                LinkStatement(Empty);
            }
            else
            {
                LinkStatement(ReportUnrecognizedStatementError(ERRID_Syntax, ErrorInConstruct));
            }
            break;
    }
}

void 
Parser::ParseReturnOrYieldStatement
(
    Token *Start, 
    _Inout_ bool &ErrorInConstruct, 
    bool ImplicitReturn
)
{

    ParseTree::ExpressionStatement *ReturnOrYield = new(m_TreeStorage) ParseTree::ExpressionStatement;

    if (Start->m_TokenType == tkRETURN || ImplicitReturn)
    {
        ReturnOrYield->Opcode = ParseTree::Statement::Return;
    }
    else if (Start->m_TokenType == tkYIELD)
    {
        ReturnOrYield->Opcode = ParseTree::Statement::Yield;
    }
    else
    {
        VSFAIL("Supposed to be handling a return or a yield here!");
    }

    if (!ImplicitReturn)
    {
        GetNextToken();
    }    

#if DEBUG
    Token *StartToken = m_CurrentToken;
#endif

    // Dev10#694102 - Consider "Dim f = Sub() Return + 5". Which of the following should it mean?
    //   Dim f = (Sub() Return) + 5   ' use an overloaded addition operator
    //   Dim f = (Sub() Return (+5))  ' return the number +5
    // The spec says that we will greedily parse the body of a statement lambda.
    // And indeed doing so agrees with the user's intuition ("Return +5" should give an error
    // that subs cannot return values: it should not give an error that there is no overloaded
    // operator + between funcions and integers!)

    // We will try to parse the expression, but the final "true" argument means
    // "Bail if the first token isn't a valid way to start an expression; in this case just return NULL"
    ReturnOrYield->Operand = ParseExpression(PrecedenceNone, ErrorInConstruct, false, NULL, true); 
    
    // Note: Orcas behavior had been to bail immediately if IsValidStatementTerminator(m_CurrentToken).
    // Well, all such tokens are invalid as ways to start an expression, so the above call to ParseExpression
    // will bail correctly for them. I've put in this assert to show that it's safe to skip the check for IsValidStatementTerminator.
    VSASSERT(ReturnOrYield->Operand == NULL || !IsValidStatementTerminator(StartToken), "Unexpected: we should have bailed on the token after this return statement");

    if (ReturnOrYield->Operand == NULL) 
    {
        // if we bailed because the first token was not a way to start an expression, we might
        // be in a situation like "foo(Sub() Return, 15)" where next token was a valid thing
        // to come after this return statement, in which case we proceed without trying
        // to gobble up the return expression. Or we might be like "Return Select", where the next
        // token cannot possibly come after the statement, so we'll report on it now:
        if (!CanFollowStatement(m_CurrentToken) || ReturnOrYield->Opcode == ParseTree::Statement::Yield || ImplicitReturn)
        {
            // This time don't let it bail:
            ReturnOrYield->Operand = ParseExpression(PrecedenceNone, ErrorInConstruct, false, NULL, false);
        }
    }

    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }


    SetStmtBegLocation(ReturnOrYield, Start);
    SetStmtEndLocation(ReturnOrYield, m_CurrentToken);


    LinkStatement(ReturnOrYield);

    return;
}



// There is some tension between the line-oriented nature of the VB grammar, and the
// existence of multi-statement statement groups (e.g. if, enum, with, etc.).
//
// Getting the tokens for a line deals with labels and line numbers. The individual
// parsing routines ignore lines and deal with statements. The parse driver (ParseLine)
// below manages fetching new lines from the scanner as needed, via VerifyEndOfStatement.
//
// The parser maintains a syntactic context, in the form of a parse tree for the
// nearest-enclosing statement group construct. This allows the parser find the ends of
// statement groups, and to make informed guesses about recovering from mismatched or
// missing group end statements.

void 
Parser::ParseLine
(
    StatementParsingMethod StatementParser
)
{
    if (m_CurrentToken->m_TokenType == tkSharp )
    {
        ParseConditionalCompilationLine(false);
    }
    else
    {
        while (true)
        {
            bool ErrorInConstruct = false;
            CacheStmtLocation();
            
            VSASSERT(m_pFirstMultilineLambdaBody || m_LambdaBodyStatements.Count() == 0,"ParseLine. should have no leftover lambdabodies");

            (this->*StatementParser)(ErrorInConstruct);

            if (VerifyEndOfStatement(ErrorInConstruct))
            {
                break;
            }
        }
    }
}

//
//============ Methods for parsing declaration constructs ============
//

/*********************************************************************
*
* Function:
*     Parser::ParseSpecifierDeclaration
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseSpecifierDeclaration
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *StmtStart = m_CurrentToken;
    ParseTree::Statement *Result = NULL;

    ParseTree::AttributeSpecifierList *Attributes = NULL;
    if (m_CurrentToken->m_TokenType == tkLT)
    {
        Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
    }

    ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInConstruct);

    // Current token set to token after the last specifier
    switch (m_CurrentToken->m_TokenType)
    {
        case tkPROPERTY:
            Result = ParsePropertyDefinition(Attributes, 
                Specifiers, 
                StmtStart, 
                ErrorInConstruct, 
                false);
            break;
        case tkID:
            if (IdentifierAsKeyword(m_CurrentToken) == tkTYPE &&
                m_CurrentToken->m_Next->m_TokenType == tkID &&
                IsValidStatementTerminator(m_CurrentToken->m_Next->m_Next) &&
                Specifiers->HasSpecifier(ParseTree::Specifier::Accesses) &&
                !Specifiers->HasSpecifier(~ParseTree::Specifier::Accesses))
            {
                // Type is now Structure
                Result = ReportUnrecognizedStatementError(ERRID_ObsoleteStructureNotType, ErrorInConstruct);
                break;
            }

            // Dim or Const declaration.
            Result = ParseVarDeclStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkENUM:
            Result = ParseEnumStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkMODULE:
        {
            ParseTree::BlockStatement *Context = RealContext();

            if (Context->Opcode != ParseTree::Statement::Namespace &&
                Context->Opcode != ParseTree::Statement::File)
            {
                ReportSyntaxError(
                    ERRID_ModuleNotAtNamespace,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            __fallthrough; // Fall through.
        }
        case tkSTRUCTURE:
        case tkINTERFACE:
        case tkCLASS:

            Result = ParseTypeStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkDECLARE:
            Result = ParseProcDeclareStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkCUSTOM:
            VSASSERT(m_CurrentToken->m_Next->m_TokenType == tkEVENT, "tkCUSTOM unexpected in non-event context!!!");
            __fallthrough; // Custom event. Fall through

        case tkEVENT:
            Result = ParseEventDefinition(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkSUB:
        case tkFUNCTION:
        case tkOPERATOR:
            ParseProcedureDefinition(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        case tkDELEGATE:
            Result = ParseDelegateStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            break;

        default:
        {
            // Error recovery. Try to give a more descriptive error
            // depending on what we're currently at and possibly recover.
            //

            if (m_CurrentToken->m_TokenType == tkINHERITS   ||
                m_CurrentToken->m_TokenType == tkIMPLEMENTS ||
                m_CurrentToken->m_TokenType == tkIMPORTS)
            {
                ReportSyntaxError(
                    ERRID_SpecifiersInvalidOnInheritsImplOpt,
                    StmtStart,
                    m_CurrentToken,
                    ErrorInConstruct);

                CacheStmtLocation();

                Result =
                    m_CurrentToken->m_TokenType == tkIMPORTS ?
                        ParseImportsStatement(ErrorInConstruct) :
                        ParseInheritsImplementsStatement(ErrorInConstruct);
            }
            else if (m_CurrentToken->m_TokenType == tkNAMESPACE)
            {
                ReportSyntaxError(
                    ERRID_SpecifiersInvalidOnNamespace,
                    StmtStart,
                    m_CurrentToken,
                    ErrorInConstruct);

                CacheStmtLocation();

                Result = ParseNamespaceStatement(m_CurrentToken, ErrorInConstruct);
            }
            else if (m_CurrentToken->m_TokenType == tkOPTION)
            {
                ReportSyntaxError(
                    ERRID_SpecifiersInvalidOnInheritsImplOpt,
                    StmtStart,
                    m_CurrentToken,
                    ErrorInConstruct);

                CacheStmtLocation();
                Result = ParseOptionStatement(ErrorInConstruct);
            }
            else if (Specifiers->HasSpecifier(
                        ParseTree::Specifier::Dim |
                            ParseTree::Specifier::Const) ||
                    !Specifiers->HasSpecifier(
                        ParseTree::Specifier::Overloads |
                            ParseTree::Specifier::Overrides |
                            ParseTree::Specifier::NotOverridable |
                            ParseTree::Specifier::Overridable))
            {
                Result = ParseVarDeclStatement(Attributes, Specifiers, StmtStart, ErrorInConstruct);
            }
            else
            {
              /* Dev10_563428
                 Because the current token is one past the last specifier we saw,
                 back up one token so we are on the specifier.  Otherwise we end up setting
                 the location of the statement to whatever lies beyond the specifier, which is wrong.
                 We don't need to bookmark the current token location because ReportUnrecognizedStatementError()
                 is going to move the token position to the end of the statement, anyway, and we don't want to undo that. */
                m_CurrentToken = m_CurrentToken->m_Prev;
                Result = ReportUnrecognizedStatementError(ERRID_ExpectedDeclaration, ErrorInConstruct);
            }

            break;
        }
    }

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseEnumStatement
*
* Purpose:
*     Parses: Enum <ident>
*
**********************************************************************/
ParseTree::EnumTypeStatement *
Parser::ParseEnumStatement
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,
    Token *Start,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT( m_CurrentToken->m_TokenType == tkENUM, "ParseEnumStmt: current token should be at tkENUM.");

    ParseTree::EnumTypeStatement *EnumTree = new(m_TreeStorage) ParseTree::EnumTypeStatement;
    EnumTree->Opcode = ParseTree::Statement::Enum;
    EnumTree->Specifiers = Specifiers;
    EnumTree->Attributes = Attributes;

    SetPunctuator(&EnumTree->TypeKeyword, Start, m_CurrentToken);

    GetNextToken(); // Get off ENUM
    EnumTree->Name = ParseIdentifier(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(1, tkAS);
    }

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        SetPunctuator(&EnumTree->As, Start, m_CurrentToken);
        GetNextToken(); // get of AS

        EnumTree->UnderlyingRepresentation = ParseTypeName(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }
    }

    SetStmtBegLocation(EnumTree, Start);
    SetStmtEndLocation(EnumTree, m_CurrentToken);

    return EnumTree;
}

/*********************************************************************
*
* Function:
*     Parser::ParseEnumGroupStatement
*
* Purpose:
*     Parses an enumerator, or the end of the enum group.
*
**********************************************************************/
void
Parser::ParseEnumGroupStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(RealContext()->Opcode == ParseTree::Statement::Enum, "should be within an enum group.");

    if (m_CurrentToken->m_TokenType == tkEND)
    {
        ParseGroupEndStatement(ErrorInConstruct);
    }
    else
    {
        ParseTree::Statement *Enumerator = ParseEnumMember(ErrorInConstruct);
        if (Enumerator)
        {
            LinkStatement(Enumerator);
        }
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseEnumMember
*
* Purpose:
*     Parses an enum member definition.
*
*     Does NOT advance to next line so caller can recover from errors.
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseEnumMember
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::EnumeratorStatement *Enumerator = NULL;

    ParseTree::AttributeSpecifierList *Attributes = NULL;
    if (m_CurrentToken->m_TokenType == tkLT)
    {
        Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
    }

    if (m_InterpretationUnderIntelliSense && Attributes && 
        m_CurrentToken->m_TokenType != tkID)
    {
        // For IntelliSense to work for attributes in enums, an attribute statement
        // needs to be returned (rather than NULL), so that IntelliSense has something
        // to work with
        ParseTree::Statement *Stmt = new(m_TreeStorage) ParseTree::AttributeStatement;
        Stmt->Opcode = ParseTree::Statement::Attribute;

        SetStmtBegLocation(Stmt, Start);
        SetStmtEndLocation(Stmt, m_CurrentToken);
        
        Stmt->AsAttribute()->Attributes = Attributes;
        return Stmt;
    }

    if (m_CurrentToken->m_TokenType != tkID &&
        // If the current context is a Region, then the trick below won't work.
        m_Context->Opcode == ParseTree::Statement::Enum)
    {
        // Check to see if this construct is a valid module-level declaration.
        // If it is, end the current enum context and reparse the statement.
        // (This case is important for automatic end insertion.)

        if (BeginsDeclaration(m_CurrentToken))
        {
            ReportSyntaxError(ERRID_InvInsideEndsEnum, Start, m_CurrentToken->m_Next, ErrorInConstruct);
            PopContext(false, Start);

            // Popping the context has no effect if the context was the original one
            // provided for a line parse. In such cases, redoing the parse will
            // produce an infinite recursion.

            if (RealContext()->Opcode == ParseTree::Statement::Enum)
            {
                ResyncAt(0);
            }
            else
            {
                // Redo the parse as a regular module-level statement.
                ParseDeclarationStatement(ErrorInConstruct);
            }

            return NULL;
        }
    }

    ParseTree::IdentifierDescriptor Name = ParseIdentifier(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(1, tkEQ);
    }

    // See if there is an expression
    ParseTree::Expression *Expr = NULL;
    if (m_CurrentToken->m_TokenType == tkEQ)
    {
        Token *Equals = m_CurrentToken;
        GetNextToken(); // Get off '='

        Enumerator = new(m_TreeStorage) ParseTree::EnumeratorWithValueStatement;
        Enumerator->Opcode = ParseTree::Statement::EnumeratorWithValue;
        
        EatNewLine();
        
        Expr = ParseDeferredExpression(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            // Resync at EOS so we don't get any more errors.
            ResyncAt(0);
        }
        
        Enumerator->AsEnumeratorWithValue()->Value = Expr;

        SetLocationAndPunctuator(
            &Enumerator->TextSpan,
            &Enumerator->AsEnumeratorWithValue()->Equals,
            Start,
            m_CurrentToken,
            Equals);
    }
    else
    {
        Enumerator = new(m_TreeStorage) ParseTree::EnumeratorStatement;
        Enumerator->Opcode = ParseTree::Statement::Enumerator;

        SetStmtBegLocation(Enumerator, Start);
        SetStmtEndLocation(Enumerator, m_CurrentToken);
    }

    Enumerator->Name = Name;
    Enumerator->Attributes = Attributes;

    return Enumerator;
}


/*********************************************************************
*
* Function:
*     Parser::ParseTypeStatement
*
* Purpose:
*
**********************************************************************/
ParseTree::TypeStatement *
Parser::ParseTypeStatement
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,   // [in] specifiers on decl
    Token *Start,      // [in] token starting Enum statement
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Statement::Opcodes Opcode = ParseTree::Statement::SyntaxError;
    bool PossiblePartialType = false;

    switch (m_CurrentToken->m_TokenType)
    {
        case tkMODULE:
            Opcode = ParseTree::Statement::Module;
            break;
        case tkCLASS:
            Opcode = ParseTree::Statement::Class;
            PossiblePartialType = true;
            break;
        case tkSTRUCTURE:
            Opcode = ParseTree::Statement::Structure;
            PossiblePartialType = true;
            break;
        case tkINTERFACE:
            Opcode = ParseTree::Statement::Interface;
            break;
        default:
            VSFAIL("Declaration parser lost.");
    }

    Token *TypeKeyword = m_CurrentToken;
    GetNextToken();

    ParseTree::TypeStatement *Result = new(m_TreeStorage) ParseTree::TypeStatement;
    Result->Opcode = Opcode;
    
#if 0    
    EatNewLine(Result);
#endif

    ParseTree::IdentifierDescriptor Ident = ParseIdentifier(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkOF, tkLParen);
    }

    ParseTree::GenericParameterList *GenericParams = NULL;
    Token *Of = NULL;
    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    if (BeginsGeneric(m_CurrentToken))
    {
        // Modules cannot be generic
        //
        if (Opcode == ParseTree::Statement::Module)
        {
            ReportGenericParamsDisallowedError(
                ERRID_ModulesCannotBeGeneric,
                ErrorInConstruct);
        }
        else
        {
            GenericParams = ParseGenericParameters(Of, LeftParen, RightParen, ErrorInConstruct);
        }
    }

    Result->Name = Ident;
    Result->GenericParameters = GenericParams;
    Result->Attributes = Attributes;
    Result->Specifiers = Specifiers;

    SetPunctuator(&Result->TypeKeyword, Start, TypeKeyword);
    SetPunctuator(&Result->GenericOf, Start, Of);
    SetPunctuatorIfMatches(&Result->GenericLeftParen, Start, LeftParen, tkLParen);
    SetPunctuatorIfMatches(&Result->GenericRightParen, Start, RightParen, tkRParen);
    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

void
Parser::ReportGenericParamsDisallowedError
(
    ERRID errid,
    _Inout_ bool &ErrorInConstruct
)
{
    bool TempErrorInConstruct = false;
    Token *Of = NULL;
    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    bool PrevErrorReportingStatus = DisableErrors();
    ParseGenericParameters(Of, LeftParen, RightParen, TempErrorInConstruct);
    EnableErrors(PrevErrorReportingStatus);

    if (!RightParen ||
        RightParen->m_TokenType != tkRParen)
    {
        ResyncAt(0);
    }

    VSASSERT(LeftParen && LeftParen->m_TokenType == tkLParen &&
             Of && Of->m_TokenType == tkOF,
                "Generic params parsing lost!!!");

    Token *ErrorStart = LeftParen;
    Token *ErrorEnd =
        (RightParen && RightParen->m_TokenType == tkRParen) ?
            RightParen->m_Next :
            m_CurrentToken ?
                m_CurrentToken :
                LeftParen->m_Next;

    ReportSyntaxError(
        errid,
        ErrorStart,
        ErrorEnd,
        ErrorInConstruct);
}

void
Parser::ReportGenericArgumentsDisallowedError
(
    ERRID errid,
    _Inout_ bool &ErrorInConstruct
)
{
    bool TempErrorInConstruct = false;
    Token *Of = NULL;
    Token *LeftParen = NULL;
    Token *RightParen = NULL;
    bool AllowEmptyGenericArguments = true;
    bool AllowNonEmptyGenericArguments = true;

    bool PrevErrorReportingStatus = DisableErrors();

    ParseGenericArguments(
        Of,
        LeftParen,
        RightParen,
        AllowEmptyGenericArguments,
        AllowNonEmptyGenericArguments,
        TempErrorInConstruct);

    EnableErrors(PrevErrorReportingStatus);

    if (!RightParen ||
        RightParen->m_TokenType != tkRParen)
    {
        ResyncAt(0);
    }

    VSASSERT(LeftParen && LeftParen->m_TokenType == tkLParen,
                "Generic params parsing lost!!!");

    Token *ErrorStart = LeftParen;
    Token *ErrorEnd =
        (RightParen && RightParen->m_TokenType == tkRParen) ?
            RightParen->m_Next :
            m_CurrentToken ?
                m_CurrentToken :
                LeftParen->m_Next;

    ReportSyntaxError(
        errid,
        ErrorStart,
        ErrorEnd,
        ErrorInConstruct);
}

void
Parser::RejectGenericParametersForMemberDecl
(
    _In_ bool &ErrorInConstruct
)
{
    if (!BeginsGeneric(m_CurrentToken))
    {
        return;
    }

    bool TempErrorInConstruct = ErrorInConstruct;

    ReportGenericParamsDisallowedError(
        ERRID_GenericParamsOnInvalidMember,
        TempErrorInConstruct);
}

ParseTree::NamespaceStatement *
Parser::ParseNamespaceStatement
(
    _In_ Token *Start,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkNAMESPACE, "Namespace parsing lost.");

    GetNextToken(); // get off NAMESPACE token

    Token* startLoc = m_CurrentToken;

    ParseTree::NamespaceStatement *Result = new(m_TreeStorage) ParseTree::NamespaceStatement;
    Result->Opcode = ParseTree::Statement::Namespace;
    
    ParseTree::Name *Name = ParseName(
        false,                      // RequireQualification
        ErrorInConstruct,
        m_AllowGlobalNamespace,     // AllowGlobalNameSpace
        false,                      // AllowGenericArguments
        false,                      // DisallowGenericArgumentsOnLastQualifiedName
        false,                      // AllowEmptyGenericArguments
        NULL,                       // AllowedEmptyGenericArguments
        m_AllowGlobalNamespace      // AllowGlobalOnly
    );

    if (ParseTree::Name::GlobalNameSpace == Name->Opcode)
    {     
        AssertLanguageFeature(FEATUREID_GlobalNamespace, startLoc);
    }
    

    if (ErrorInConstruct)
    {
        // Resync at EOS so we don't get expecting EOS errors
        ResyncAt(0);
    }

    Result->Name = Name;

    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}


/*********************************************************************
*
* Function:
*     Parser::ParseInterfaceGroupStatement
*
* Purpose:
*     Parses an interface, or the end of the interface group.
*
**********************************************************************/
void
Parser::ParseInterfaceGroupStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(RealContext()->Opcode == ParseTree::Statement::Interface, "should be within an interface group.");

    if (m_CurrentToken->m_TokenType == tkEND)
    {
        ParseGroupEndStatement(ErrorInConstruct);
    }
    else
    {
        ParseTree::Statement *InterfaceMem = ParseInterfaceMember(ErrorInConstruct);

        if (InterfaceMem)
        {
            LinkStatement(InterfaceMem);

            if (InterfaceMem->Opcode == ParseTree::Statement::Property)
            {
                // A property is a block construct and so has been
                // established as the current context. It will not
                // be explicitly ended, so pop it.

                PopContext(true, m_CurrentToken);
            }
        }
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseInterfaceMember
*
* Purpose:
*     Parses an interface member definition.
*
*     Does NOT advance to next line so caller can recover from errors.
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseInterfaceMember
(
    _Inout_ bool &ErrorInConstruct
)
{
    // Parse the method definitions (without bodies) in the interface.

    ParseTree::Statement *Stmt = NULL;

    Token *Start = m_CurrentToken;

    ParseTree::AttributeSpecifierList *Attributes = NULL;
    Token *AttributesEnd = NULL;
    if (Start->m_TokenType == tkLT)
    {
        Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
        AttributesEnd = m_CurrentToken;
    }

    ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInConstruct);

    tokens ProcType = m_CurrentToken->m_TokenType;

    switch (ProcType)
    {
        case tkENUM:
            Stmt = ParseEnumStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkMODULE:
            ReportSyntaxError(
                ERRID_ModuleNotAtNamespace,
                m_CurrentToken,
                ErrorInConstruct);
            __fallthrough; // Fall through.
        case tkSTRUCTURE:
        case tkINTERFACE:
        case tkCLASS:
            Stmt = ParseTypeStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkDELEGATE:
            Stmt = ParseDelegateStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkCUSTOM:
            VSASSERT(m_CurrentToken->m_Next->m_TokenType == tkEVENT, "tkCUSTOM unexpected in non-event context!!!");
            __fallthrough; // Custom event. Fall through

        case tkEVENT:
            Stmt = ParseEventDefinition(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkSUB:
            Stmt = ParseSubDeclaration(Attributes, Specifiers, Start, false, ErrorInConstruct);
            break;

        case tkFUNCTION:
            Stmt = ParseFunctionDeclaration(Attributes, Specifiers, Start, false, ErrorInConstruct);
            break;

        case tkINHERITS:
            if (Specifiers != 0)
            {
                ReportSyntaxError(
                    ERRID_SpecifiersInvalidOnInheritsImplOpt,
                    Start,
                    m_CurrentToken,
                    ErrorInConstruct);

                CacheStmtLocation();
            }

            Stmt = ParseInheritsImplementsStatement(ErrorInConstruct);
            break;

        case tkPROPERTY:
            Stmt = ParsePropertyDefinition(Attributes, Specifiers, Start, ErrorInConstruct, true);
            break;

        case tkOPERATOR:
        default:

            if (m_InterpretationUnderIntelliSense && (Attributes || Specifiers))
            {
                // For IntelliSense to work for attributes and specifiers in interfaces, a statement
                // needs to be returned (rather than NULL), so that IntelliSense has something
                // to work with

                Stmt = new(m_TreeStorage) ParseTree::VariableDeclarationStatement;
                Stmt->Opcode = ParseTree::Statement::VariableDeclaration;

                SetStmtBegLocation(Stmt, Start);

                Stmt->AsVariableDeclaration()->Attributes = Attributes;
                Stmt->AsVariableDeclaration()->Specifiers = Specifiers;

                SetStmtEndLocation(Stmt, m_CurrentToken);

                return Stmt;
            }

            // Check to see if this construct is a valid module-level
            // declaration. If it is, end the current interface context
            // and reparse the statement.
            // (This case is important for automatic end insertion.)

            if (BeginsDeclaration(m_CurrentToken) &&
                // If the current context is a Region, then the trick below won't work.
                m_Context->Opcode == ParseTree::Statement::Interface)
            {

                ReportSyntaxError(ERRID_InvInsideEndsInterface, Start, m_CurrentToken->m_Next, ErrorInConstruct);
                PopContext(false, Start);

                // Popping the context has no effect if the context was the original one
                // provided for a line parse. In such cases, redoing the parse will
                // produce an infinite recursion.

                if (RealContext()->Opcode == ParseTree::Statement::Interface)
                {
                    ResyncAt(0);
                }
                else
                {
                    // Redo the parse as a regular module-level statement.
                    m_CurrentToken = Start;
                    ParseDeclarationStatement(ErrorInConstruct);
                }

                return NULL;
            }

            ParseTree::Statement *UnrecognizedStatement = ReportUnrecognizedStatementError(
                ( ProcType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkTYPE ) ?
                        // "Type" is now "Structure"
                        ERRID_ObsoleteStructureNotType :
                        ERRID_InterfaceMemberSyntax,
                ErrorInConstruct);

            AssertIfNull(UnrecognizedStatement);

            UnrecognizedStatement->TextSpan.m_lBegLine = Start->m_StartLine;
            UnrecognizedStatement->TextSpan.m_lBegColumn = TokenColumnBeg(Start);

            return UnrecognizedStatement;
    }

    SetStmtEndLocation(Stmt, m_CurrentToken);
    return Stmt;
}

void
Parser::ParsePropertyOrEventGroupStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(RealContext()->Opcode == ParseTree::Statement::Property ||
             RealContext()->Opcode == ParseTree::Statement::BlockEventDeclaration, "should be within a property or event group.");

    Token *Start = m_CurrentToken;

    //Check for an unexpected End statement
    if (Start->m_TokenType == tkEND)
    {
        ParseGroupEndStatement(ErrorInConstruct);
    }
    else
    {
        ParsePropertyOrEventProcedureDefinition(Start, ErrorInConstruct);
    }
}

/*********************************************************************
*
* Function:
*     Parser::CreateCommentBlockStatement
*
* Purpose:
*     Creates a new comment block, either an XMLDoc comment block or a
*     normal comment block.
*
**********************************************************************/
ParseTree::CommentBlockStatement *
Parser::CreateCommentBlockStatement
(
    _In_ Token *Start      // [in] token starting Enum statement
)
{
    ParseTree::CommentBlockStatement *Result = new(m_TreeStorage) ParseTree::CommentBlockStatement;
    Result->Opcode = ParseTree::Statement::CommentBlock;

    // Is this an XMLDoc comment block?
    Result->IsXMLDocComment = m_IsXMLDocEnabled && (Start->m_TokenType == tkXMLDocComment);

    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

// IsTwoLevelContext returns true if and only if the context is always nested
// within another specific context that must be popped to fully terminate this
// context.

static bool
IsTwoLevelContext
(
    ParseTree::Statement::Opcodes Context
)
{
    switch (Context)
    {
        case ParseTree::Statement::Case:
        case ParseTree::Statement::CaseElse:

            return true;
    }

    return false;
}

// Parse an End statement that ends a statement group.

void
Parser::ParseGroupEndStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::Statement::Opcodes EndOpcode = ParseEndClause();
    ParseTree::Statement::Opcodes Context = m_Context->Opcode;

    if (m_Context->Opcode != ParseTree::Statement::LineIf &&
        m_Context->Opcode != ParseTree::Statement::LineElse &&
        !IsParsingSingleLineLambda())
    {
        bool EndMatchesContext = false;

        switch (EndOpcode)
        {
            case ParseTree::Statement::EndIf:
                EndMatchesContext =
                    (Context == ParseTree::Statement::BlockIf ||
                     Context == ParseTree::Statement::ElseIf ||
                     Context == ParseTree::Statement::BlockElse);
                break;
            case ParseTree::Statement::EndWith:
                EndMatchesContext = Context == ParseTree::Statement::With;
                break;
            case ParseTree::Statement::EndEnum:
                EndMatchesContext = Context == ParseTree::Statement::Enum;
                break;
            case ParseTree::Statement::EndStructure:
                EndMatchesContext = Context == ParseTree::Statement::Structure;
                break;
            case ParseTree::Statement::EndInterface:
                EndMatchesContext = Context == ParseTree::Statement::Interface;
                break;
            case ParseTree::Statement::EndProperty:
                EndMatchesContext = Context == ParseTree::Statement::Property;
                break;
            case ParseTree::Statement::EndEvent:
                EndMatchesContext = Context == ParseTree::Statement::BlockEventDeclaration;
                break;
            case ParseTree::Statement::EndTry:
                EndMatchesContext =
                    (Context == ParseTree::Statement::Try ||
                     Context == ParseTree::Statement::Catch ||
                     Context == ParseTree::Statement::Finally);
                break;
            case ParseTree::Statement::EndClass:
                EndMatchesContext = Context == ParseTree::Statement::Class;
                break;
            case ParseTree::Statement::EndModule:
                EndMatchesContext = Context == ParseTree::Statement::Module;
                break;
            case ParseTree::Statement::EndNamespace:
                EndMatchesContext = Context == ParseTree::Statement::Namespace;
                break;
            case ParseTree::Statement::EndUsing:
                EndMatchesContext = Context == ParseTree::Statement::Using;
                break;
            case ParseTree::Statement::EndSyncLock:
                EndMatchesContext = Context == ParseTree::Statement::SyncLock;
                break;
            case ParseTree::Statement::EndWhile:
                EndMatchesContext = Context == ParseTree::Statement::While;
                break;
            case ParseTree::Statement::EndSelect:
                EndMatchesContext =
                    (Context == ParseTree::Statement::Select ||
                     Context == ParseTree::Statement::Case ||
                     Context == ParseTree::Statement::CaseElse);
                break;
                
            case ParseTree::Statement::EndSub:
            case ParseTree::Statement::EndFunction:
                VSASSERT(!IsParsingSingleLineLambda(), "unexpected: if we're parsing a single-line sub lambda, we shouldn't be asked to parse a group end statement");
                if  ((Context == ParseTree::Statement::LambdaBody))
                {
                    if ((m_Context->AsLambdaBody()->pOwningLambdaExpression->MethodFlags & DECLF_Function))
                    {   // we're in a Lambda Function context
                        if (EndOpcode == ParseTree::Statement::EndFunction) // and we got the wrong end construct
                        {
                            EndMatchesContext = true;   // tell the rest of this code that we have a match
                        }
                    }
                    else
                    {   //we're in a Lambda Sub context
                        if (EndOpcode == ParseTree::Statement::EndSub) // and we got the wrong end construct
                        {
                            EndMatchesContext = true;   // tell the rest of this code that we have a match
                        }
                    }
                }
                break;

            default:
                break;
            }

        if (EndMatchesContext)
        {
            // Some blocks always nest within other blocks, and the end construct
            // actually ends the enclosing block.

            if (IsTwoLevelContext(Context))
            {
                PopContext(true, Start);
            }

            EndContext(Start, EndOpcode, NULL);
            return;
        }

        // ParseOneLine creates end nodes for mismatched ends.

        if (m_IsLineParse)
        {
            m_CurrentStatementInError = true;
            CreateEndConstruct(Start, EndOpcode);
            return;
        }

        // ParseDeclsAndMethodBodies creates end proc nodes.

        if (m_IsDeclAndMethodBodiesParse)
        {
            if (EndOpcode == ParseTree::Statement::EndSub ||
                EndOpcode == ParseTree::Statement::EndFunction ||
                EndOpcode == ParseTree::Statement::EndOperator ||
                EndOpcode == ParseTree::Statement::EndGet ||
                EndOpcode == ParseTree::Statement::EndSet ||
                EndOpcode == ParseTree::Statement::EndAddHandler ||
                EndOpcode == ParseTree::Statement::EndRemoveHandler ||
                EndOpcode == ParseTree::Statement::EndRaiseEvent)
            {
                if (Context == ParseTree::Statement::ProcedureBody ||
                    Context == ParseTree::Statement::FunctionBody ||
                    Context == ParseTree::Statement::OperatorBody ||
                    Context == ParseTree::Statement::PropertyGetBody ||
                    Context == ParseTree::Statement::PropertySetBody ||
                    Context == ParseTree::Statement::AddHandlerBody ||
                    Context == ParseTree::Statement::RemoveHandlerBody ||
                    Context == ParseTree::Statement::RaiseEventBody)
                {
                    EndContext(Start, EndOpcode, NULL);
                    return;
                }
            }
        }
    }

    if (!m_IsLineParse)
    {
        RecoverFromMismatchedEnd(Start, EndOpcode, ErrorInConstruct);
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseEndClause
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement::Opcodes
Parser::ParseEndClause
(
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkEND,
        "must be at End token.");

    ParseTree::Statement::Opcodes EndOpcode = ParseTree::Statement::SyntaxError;

    // Check to see what kind of end construct it is
    switch (GetNextToken()->m_TokenType)
    {
        case tkIF:
            EndOpcode = ParseTree::Statement::EndIf;
            break;

        case tkUSING:
            EndOpcode = ParseTree::Statement::EndUsing;
            break;

        case tkWITH:
            EndOpcode = ParseTree::Statement::EndWith;
            break;

        case tkSTRUCTURE:
            EndOpcode = ParseTree::Statement::EndStructure;
            break;

        case tkENUM:
            EndOpcode = ParseTree::Statement::EndEnum;
            break;

        case tkINTERFACE:
            EndOpcode = ParseTree::Statement::EndInterface;
            break;

        case tkSUB:
            EndOpcode = ParseTree::Statement::EndSub;
            break;

        case tkFUNCTION:
            EndOpcode = ParseTree::Statement::EndFunction;
            break;

        case tkOPERATOR:
            EndOpcode = ParseTree::Statement::EndOperator;
            break;

        case tkSELECT:
            m_SelectBeforeFirstCase = false;
            EndOpcode = ParseTree::Statement::EndSelect;
            break;

        case tkTRY:
            EndOpcode = ParseTree::Statement::EndTry;
            break;

        case tkGET:
            EndOpcode = ParseTree::Statement::EndGet;
            break;

        case tkSET:
            EndOpcode = ParseTree::Statement::EndSet;
            break;

        case tkPROPERTY:
            EndOpcode = ParseTree::Statement::EndProperty;
            break;

        case tkADDHANDLER:
            EndOpcode = ParseTree::Statement::EndAddHandler;
            break;

        case tkREMOVEHANDLER:
            EndOpcode = ParseTree::Statement::EndRemoveHandler;
            break;

        case tkRAISEEVENT:
            EndOpcode = ParseTree::Statement::EndRaiseEvent;
            break;

        case tkEVENT:
            EndOpcode = ParseTree::Statement::EndEvent;
            break;

        case tkCLASS:
            EndOpcode = ParseTree::Statement::EndClass;
            break;

        case tkMODULE:
            EndOpcode = ParseTree::Statement::EndModule;
            break;

        case tkNAMESPACE:
            EndOpcode = ParseTree::Statement::EndNamespace;
            break;

        case tkSYNCLOCK:
            EndOpcode = ParseTree::Statement::EndSyncLock;
            break;

        case tkWHILE:
            EndOpcode = ParseTree::Statement::EndWhile;
            break;

        default:
            return
                IsValidStatementTerminator(m_CurrentToken) ?  
                    ParseTree::Statement::EndUnknown :
                    ParseTree::Statement::EndInvalid;
    }

    GetNextToken();
    return EndOpcode;
}

// Create a tree to represent the end of a statement group.

ParseTree::Statement *
Parser::CreateEndConstruct
(
    _In_ Token *StartOfEndConstruct,
    ParseTree::Statement::Opcodes EndOpcode,
    bool linkStatement
)
{
    VSASSERT(EndOpcode != ParseTree::Statement::EndCommentBlock, L"We should be trying to create an EndCommentBlock construct");

    ParseTree::Statement *Result;
    Token* Punctuator = NULL;

    switch (EndOpcode)
    {
        case ParseTree::Statement::EndNext:
            Result = new(m_TreeStorage) ParseTree::EndNextStatement;
            break;
        case ParseTree::Statement::EndRegion:
            // An end of conditional statement is produced by 3 tokens: 
            // tkSharp, tkEND, and keyword token (tkIf, tkRegion, etc).
            // EndBlockStatement keeps locations of two last tokens: 
            // tkEND (Statement->End) and the keyword token (Statement->Punctuator). 
            // SetPunctuator method calculates the location of the punctuator relative 
            // to the start token (the second argument). 
            // Since the span of statement node starts at the first token (tkSharp), 
            // we should calculate positions of both punctuators relative to tkSharp. 

            Result = new(m_TreeStorage) ParseTree::CCEndStatement;
            if (StartOfEndConstruct->m_Next != NULL)
            {
                // Set "End" keyword location - moved here to avoid cases when it is not initialized (Dev10 #713757)
                SetPunctuator( &(Result->AsCCEnd()->End), StartOfEndConstruct, StartOfEndConstruct->m_Next);

                // We need to skip 2 tokens: #, End to get 
                Punctuator = StartOfEndConstruct->m_Next->m_Next;
            }
            break;
        default:
            Result = new(m_TreeStorage) ParseTree::EndBlockStatement;
            Punctuator = StartOfEndConstruct->m_Next;
            break;
    }

    if (Punctuator != NULL)
    {
        SetPunctuator( &(Result->AsEndBlock()->Punctuator), StartOfEndConstruct, Punctuator);
    }

    Result->Opcode = EndOpcode;

    SetStmtBegLocation(Result, StartOfEndConstruct);
    SetStmtEndLocation(Result, m_CurrentToken);

    if (linkStatement)
    {
        LinkStatement(Result);
    }

    return Result;
}

ParseTree::Statement *
Parser::CreateEndConstructWithOperand
(
    _In_ Token *StartOfEndConstruct,
    ParseTree::Statement::Opcodes EndOpcode,
    _In_opt_ ParseTree::Expression *Operand
)
{
    ParseTree::ExpressionStatement *Result =
        (EndOpcode == ParseTree::Statement::EndLoopWhile ||
         EndOpcode == ParseTree::Statement::EndLoopUntil) ?
            new(m_TreeStorage) ParseTree::BottomTestLoopStatement :
            new(m_TreeStorage) ParseTree::ExpressionStatement;
    Result->Opcode = EndOpcode;

    SetStmtBegLocation(Result, StartOfEndConstruct);
    SetStmtEndLocation(Result, m_CurrentToken);

    Result->Operand = Operand;

    LinkStatement(Result);

    return Result;
}

static bool
IsSpecifier
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        case tkPUBLIC:
        case tkPRIVATE:
        case tkPROTECTED:
        case tkFRIEND:
        case tkSTATIC:
        case tkSHARED:
        case tkSHADOWS:
        case tkMUSTINHERIT:
        case tkOVERLOADS:
        case tkNOTINHERITABLE:
        case tkOVERRIDES:
        case tkPARTIAL:
        case tkNOTOVERRIDABLE:
        case tkOVERRIDABLE:
        case tkMUSTOVERRIDE:
        case tkREADONLY:
        case tkWRITEONLY:
        case tkDIM:
        case tkCONST:
        case tkDEFAULT:
        case tkWITHEVENTS:
        case tkWIDENING:
        case tkNARROWING:
        case tkCUSTOM:
        case tkASYNC:
        case tkITERATOR:
            return true;
    }
    
    return false;
}


static bool
CanStartSpecifierDeclaration
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        case tkPROPERTY:
        case tkID:
        case tkENUM:
        case tkMODULE:
        case tkSTRUCTURE:
        case tkINTERFACE:
        case tkCLASS:
        case tkDECLARE:
        case tkCUSTOM:
        case tkEVENT:
        case tkSUB:
        case tkFUNCTION:
        case tkOPERATOR:
        case tkDELEGATE:
        case tkASYNC:
        case tkITERATOR:
            return true;
    }

    return false;
    
}


/*********************************************************************
*
* Function:
*     Parser::ParseSpecifiers
*
* Purpose:
*     Parses the specifier list of a declaration. The current token
*     should be at the specifier. These specifiers can occur in
*     ANY order.
*
**********************************************************************/
ParseTree::SpecifierList *
Parser::ParseSpecifiers
(
    _Inout_ bool &ErrorInConstruct
)
{
    unsigned long Specifiers = 0;
    ParseTree::SpecifierList *Result = NULL;
    ParseTree::SpecifierList **Target = &Result;

    bool Done = false;

    // Checks for at most one specifier from each family
    while (!Done)
    {
        ParseTree::Specifier::Specifiers CurSpecifier = (ParseTree::Specifier::Specifiers)0;
        unsigned Errid = 0;

        switch (m_CurrentToken->m_TokenType)
        {
            // Access category

            case tkPUBLIC:
                CurSpecifier = ParseTree::Specifier::Public;
                goto lbl_Access;

            case tkPRIVATE:
                CurSpecifier = ParseTree::Specifier::Private;
                goto lbl_Access;

            case tkPROTECTED:
                CurSpecifier = ParseTree::Specifier::Protected;
                goto lbl_Access;

            case tkFRIEND:
                CurSpecifier = ParseTree::Specifier::Friend;
                goto lbl_Access;

lbl_Access:
                if (Specifiers & ParseTree::Specifier::Accesses)
                {
                    if (((Specifiers & ParseTree::Specifier::Accesses) | CurSpecifier) !=
                        (ParseTree::Specifier::Protected | ParseTree::Specifier::Friend))
                    {
                        Errid = ERRID_DuplicateAccessCategoryUsed;
                    }
                }
                break;

            // Storage category

            case tkSTATIC:
                CurSpecifier = ParseTree::Specifier::Static;
                break;

            case tkSHARED:
                CurSpecifier = ParseTree::Specifier::Shared;
                break;

            case tkSHADOWS:
                CurSpecifier = ParseTree::Specifier::Shadows;
                break;

            // Inheritance category

            case tkMUSTINHERIT:
                CurSpecifier = ParseTree::Specifier::MustInherit;
                break;

            case tkOVERLOADS:
                CurSpecifier = ParseTree::Specifier::Overloads;
                break;

            case tkNOTINHERITABLE:
                CurSpecifier = ParseTree::Specifier::NotInheritable;
                break;

            case tkOVERRIDES:
                CurSpecifier = ParseTree::Specifier::Overrides;
                break;

            // Partial types category

            case tkPARTIAL:
                CurSpecifier = ParseTree::Specifier::Partial;
                break;

            // Modifier category

            case tkNOTOVERRIDABLE:
                CurSpecifier = ParseTree::Specifier::NotOverridable;
                goto lbl_Modifier;

            case tkOVERRIDABLE:
                CurSpecifier = ParseTree::Specifier::Overridable;
                goto lbl_Modifier;

            case tkMUSTOVERRIDE:
                CurSpecifier = ParseTree::Specifier::MustOverride;

lbl_Modifier:
                if (Specifiers & ParseTree::Specifier::OverrideModifiers)
                {
                    Errid = ERRID_DuplicateModifierCategoryUsed;
                }
                break;

            // Writeability category

            case tkREADONLY:
                CurSpecifier = ParseTree::Specifier::ReadOnly;
                goto lbl_Writeability;

            case tkWRITEONLY:
                CurSpecifier = ParseTree::Specifier::WriteOnly;

lbl_Writeability:
                if (Specifiers & ParseTree::Specifier::WriteabilityModifiers)
                {
                    Errid = ERRID_DuplicateWriteabilityCategoryUsed;
                }
                break;

            case tkDIM:
                CurSpecifier = ParseTree::Specifier::Dim;
                goto lbl_Data;

            case tkCONST:
                CurSpecifier = ParseTree::Specifier::Const;

lbl_Data:
                // Combining "Dim" and "Const" is detected by declaration semantics.
#if 0
                if (Specifiers & ParseTree::Specifier::DataModifiers)
                {
                    Errid = ERRID_DuplicateDataCategoryUsed;
                }
#endif
                break;

            case tkDEFAULT:
                CurSpecifier = ParseTree::Specifier::Default;
                break;

            case tkWITHEVENTS:
                CurSpecifier = ParseTree::Specifier::WithEvents;
                break;

            // Conversion category

            case tkWIDENING:
                CurSpecifier = ParseTree::Specifier::Widening;
                goto lbl_Conversion;

            case tkNARROWING:
                CurSpecifier = ParseTree::Specifier::Narrowing;

lbl_Conversion:
                if (Specifiers & ParseTree::Specifier::ConversionModifiers)
                {
                    Errid = ERRID_DuplicateConversionCategoryUsed;
                }
                break;

            case tkASYNC:
            {
                AssertLanguageFeature(FEATUREID_AsyncExpressions, m_CurrentToken);
                CurSpecifier = ParseTree::Specifier::Async;
                break;
            }    

            case tkITERATOR:
            {
                AssertLanguageFeature(FEATUREID_Iterators, m_CurrentToken);               
                CurSpecifier = ParseTree::Specifier::Iterator;
                break;
            }
            case tkID:

                // This enables better error reporting for invalid uses of
                // Custom, Async, and Iterator as specifiers.
                //
                // Note that while we always treat Async and Iterator as
                // specifiers, we only treat Custom as a specifier in this
                // particular error case. This is because the use of Custom is
                // very narrow (it can only appear on event declarations)
                // whereas Async can appear on functions, subs, and lambdas, and
                // Iterator can appear on functions, property accessors, and
                // lambdas.
                //
                // Also, we need need to make sure that the usage of Custom,
                // Async, and Iterator as variable names, etc., continues to
                // work correctly.
                // 
                // See Bug VSWhidbey 379914 for the original issue with CUSTOM.
                //
                if (TokenAsKeyword(m_CurrentToken) == tkCUSTOM)
                {
                    Token *  pNext = m_CurrentToken->m_Next;

                    if
                    (    
                        IsSpecifier(pNext) ||
                        CanStartSpecifierDeclaration(pNext)
                    )
                    {
                        CurSpecifier = ParseTree::Specifier::Custom;
                        Errid = ERRID_InvalidUseOfCustomModifier;
                        break;
                    }
                }    

                __fallthrough; // Fall through

            default:
                Done = true;
        }

        if (!Done)
        {
            if (Specifiers & CurSpecifier)
            {
                Errid = ERRID_DuplicateSpecifier;
            }

            if (Errid)
            {
                // Mark the current token with the error and ignore.
                ReportSyntaxError(
                    Errid,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            Specifiers |= CurSpecifier;

            ParseTree::Specifier *Specifier = new(m_TreeStorage) ParseTree::Specifier;
            Specifier->Opcode = CurSpecifier;
            SetLocation(&Specifier->TextSpan, m_CurrentToken, m_CurrentToken->m_Next);

            ParseTree::SpecifierList *ListElement = new(m_TreeStorage) ParseTree::SpecifierList;
            ListElement->Element = Specifier;
            SetLocation(&ListElement->TextSpan, m_CurrentToken, m_CurrentToken->m_Next);

            *Target = ListElement;
            Target = &ListElement->Next;

            GetNextToken();
#if 0            
            EatNewLine(ListElement);
#endif
        }
    }

    FixupListEndLocations(Result);
    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseForLoopControlVariable
*
* Purpose:
*     Parses: <Expression> | <ident>[ArrayList] As <type>
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseForLoopControlVariable
(
    ParseTree::BlockStatement *ForBlock,
    _In_ Token *ForStart,
    _Out_ ParseTree::VariableDeclarationStatement **Decl,
    _Inout_ bool &ErrorInConstruct
)
{
    switch (m_CurrentToken->m_TokenType)
    {
        case tkID:
            switch (m_CurrentToken->m_Next->m_TokenType)
            {
                case tkNullable:
                case tkAS:
                    return ParseForLoopVariableDeclaration(ForBlock, ForStart, Decl, ErrorInConstruct);
                case tkLParen:
                    Token *LookAhead;
                    LookAhead = PeekAheadForToken(m_CurrentToken, 3, tkAS, tkIN, tkEQ);
                    if (LookAhead && (LookAhead->m_TokenType == tkAS) && (LookAhead->m_Prev->m_TokenType == tkRParen))
                    {
                        return ParseForLoopVariableDeclaration(ForBlock, ForStart, Decl, ErrorInConstruct);
                    }

                // Fall through to Non-Declaration, i.e. ParseVariable below
            }
            __fallthrough;

        default:
            return ParseVariable(ErrorInConstruct);
    }
}


/*********************************************************************
*
* Function:
*     Parser::ParseForLoopVariableDeclaration
*
* Purpose:
*     Parses: <ident>[ArrayList] As <type>
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseForLoopVariableDeclaration
(
    ParseTree::BlockStatement *ForBlock,
    _In_ Token *ForStart,
    _Out_ ParseTree::VariableDeclarationStatement **Decl,
    _Inout_ bool &ErrorInConstruct
)
{
    // Parse the control variable declaration
    Token *DeclaratorStart = m_CurrentToken;
    ParseTree::Declarator *Declarator = new(m_TreeStorage) ParseTree::Declarator;

    ParseDeclarator(true, Declarator, ErrorInConstruct);
    SetLocation(&Declarator->TextSpan, DeclaratorStart, m_CurrentToken);

    if (ErrorInConstruct)
    {
        // If we see As before a In or Each, then assume that
        // we are still on the Control Variable Declaration.
        // Otherwise, don't resync and allow the caller to
        // decide how to recover.
        if (PeekAheadFor(3, tkAS, tkIN, tkEQ) == tkAS)
        {
            ResyncAt(1, tkAS);
        }
    }

    Token *As = NULL;
    ParseTree::Type *Type = NULL;

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;

        // Parse the type
        GetNextToken();
        Type = ParseGeneralType(ErrorInConstruct);
    }   // Else if "As" is not present, the error falls out as a "Syntax error" IN the caller

    ParseTree::DeclaratorList *Declarators = NULL;
    ParseTree::DeclaratorList **DeclaratorTarget = &Declarators;

    CreateListElement(
                DeclaratorTarget,
                Declarator,
                DeclaratorStart,
                m_CurrentToken,
                NULL);

    ParseTree::VariableDeclarationList *Declarations = NULL;
    ParseTree::VariableDeclarationList **DeclarationTarget = &Declarations;
    ParseTree::VariableDeclaration *Declaration;

    Declaration = new(m_TreeStorage) ParseTree::VariableDeclaration;
    Declaration->Opcode = ParseTree::VariableDeclaration::NoInitializer;
    Declaration->HasSyntaxError = false;
    Declaration->Variables = Declarators;
    Declaration->Type = Type;

    SetLocationAndPunctuator(
            &Declaration->TextSpan,
            &Declaration->As,
            DeclaratorStart,
            m_CurrentToken,
            As);

    CreateListElement(
            DeclarationTarget,
            Declaration,
            DeclaratorStart,
            m_CurrentToken,
            NULL);

    ParseTree::VariableDeclarationStatement *Result = new(m_TreeStorage) ParseTree::VariableDeclarationStatement;
    Result->Opcode = ParseTree::Statement::VariableDeclaration;
    Result->SetParent(ForBlock);

    Result->Attributes = NULL;
    Result->Declarations = Declarations;

    // Dev10 699841 - Set the location of the Variable decl, e.g. given For x as integer in ..., set the location
    // for 'x as integer' to be just the span where 'x as integer' was found.
    SetLocation(&Result->TextSpan, DeclaratorStart, m_CurrentToken); 

    // Create a Named Expression from the variable declared in the For Statement
    // as the Control Variable Expression for the loop

    ParseTree::NameExpression *ReturnExpr = new(m_TreeStorage) ParseTree::NameExpression;
    ReturnExpr->Name = CreateId(DeclaratorStart);
    SetExpressionLocation(ReturnExpr, DeclaratorStart, DeclaratorStart, NULL);

    if (ErrorInConstruct)
    {
        // Mark the expression bad if the declaration is wrong in order to avoid more errors
        // on the same statement in semantics

        ReturnExpr->Opcode = ParseTree::Expression::SyntaxError;
    }
    else
    {
        ReturnExpr->Opcode = ParseTree::Expression::Name;
    }

    *Decl = Result;
    return ReturnExpr;
}

ParseTree::Statement *
Parser::ParseContinueStatement
(
    _In_ Token *StmtStart,    // [in] Token starting the statement
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(StmtStart && StmtStart->m_TokenType == tkCONTINUE,
                   "Invalid for non-continue contexts.");

    ParseTree::Statement::Opcodes ContinueOpcode = ParseTree::Statement::SyntaxError;
    bool DoGetNextToken = true;

    switch(GetNextToken()->m_TokenType)
    {
        case tkDO:
            ContinueOpcode = ParseTree::Statement::ContinueDo;
            break;
        case tkFOR:
            ContinueOpcode = ParseTree::Statement::ContinueFor;
            break;
        case tkWHILE:
            ContinueOpcode = ParseTree::Statement::ContinueWhile;
            break;

        default:

        {
            ContinueOpcode =
                IsValidStatementTerminator(m_CurrentToken)?
                    ParseTree::Statement::ContinueUnknown:
                    ParseTree::Statement::ContinueInvalid;

#if IDE
            // The pretty lister is expected to turn Continue statements
            // that don't specify a Do, While or For to have the correct
            // form. That requires identifying this condition during
            // parsing and correcting the parse trees.

            if (ContinueOpcode == ParseTree::Statement::ContinueUnknown &&
                m_ParsingMethodBody)
            {
                for(ParseTree::BlockStatement *EnclosingBlock = m_Context;
                    EnclosingBlock;
                    EnclosingBlock = EnclosingBlock->GetParent())
                {
                    switch (EnclosingBlock->Opcode)
                    {
                        case ParseTree::Statement::While:
                            ContinueOpcode = ParseTree::Statement::ContinueWhile;
                            break;
                        case ParseTree::Statement::ForFromTo:
                        case ParseTree::Statement::ForEachIn:
                            ContinueOpcode = ParseTree::Statement::ContinueFor;
                            break;
                        case ParseTree::Statement::DoWhileTopTest:
                        case ParseTree::Statement::DoWhileBottomTest:
                        case ParseTree::Statement::DoUntilTopTest:
                        case ParseTree::Statement::DoUntilBottomTest:
                        case ParseTree::Statement::DoForever:
                            ContinueOpcode = ParseTree::Statement::ContinueDo;
                            break;
                    }

                    if (ContinueOpcode != ParseTree::Statement::ContinueUnknown)
                    {
                        break;
                    }
                }
            }
            else
            {
                ContinueOpcode = ParseTree::Statement::ContinueInvalid;
            }

            if (ContinueOpcode != ParseTree::Statement::ContinueInvalid &&
                ContinueOpcode != ParseTree::Statement::ContinueUnknown)
            {
                ReportSyntaxErrorWithoutMarkingStatementBad(
                    ERRID_ExpectedContinueKind,
                    m_CurrentToken,
                    ErrorInConstruct);
            }
            else
#endif
            {
                ReportSyntaxError(
                    ERRID_ExpectedContinueKind,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            DoGetNextToken = false;
            ResyncAt(0);
        }
    }

    if (DoGetNextToken)
    {
        GetNextToken();
    }

    ParseTree::Statement *Continue = new(m_TreeStorage) ParseTree::Statement;
    Continue->Opcode = ContinueOpcode;

    SetStmtBegLocation(Continue, StmtStart);
    SetStmtEndLocation(Continue, m_CurrentToken);
    LinkStatement(Continue);

    return Continue;
}

/*********************************************************************
*
* Function:
*     Parser::ParseVarDeclStatement
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseVarDeclStatement
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,     // [in] specifiers on declaration
    _In_ Token *StmtStart,    // [in] Token starting the statement
    _Inout_ bool &ErrorInConstruct
)
{
    // There must be at least one specifier.
    if (Specifiers == NULL)
    {
        ReportSyntaxError(
            Attributes ?
                ERRID_StandaloneAttribute :
                ERRID_ExpectedSpecifier,
            m_CurrentToken,
            ErrorInConstruct);
    }
    else if (Specifiers->HasSpecifier(ParseTree::Specifier::Static) && 
             m_pStatementLambdaBody &&
             (!m_Errors ||
                !m_Errors->HasThisErrorWithLocation(ERRID_StaticInLambda, Specifiers->Element->TextSpan)))
    {
        ReportSyntaxError( ERRID_StaticInLambda, &Specifiers->Element->TextSpan, ErrorInConstruct);
    }

    bool CurrentStatementInError = false;

    // Parse the declarations.
    ParseTree::VariableDeclarationList *Declarations = NULL;
    Declarations = ParseVariableDeclaration(Declarations, CurrentStatementInError, ErrorInConstruct);

    m_CurrentStatementInError = CurrentStatementInError;

    FixupListEndLocations(Declarations);

    ParseTree::VariableDeclarationStatement *Result = new(m_TreeStorage) ParseTree::VariableDeclarationStatement;
    Result->Opcode = ParseTree::Statement::VariableDeclaration;

    Result->Specifiers = Specifiers;
    Result->Attributes = Attributes;
    Result->Declarations = Declarations;

    SetStmtBegLocation(Result, StmtStart);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

ParseTree::VariableDeclarationList *
Parser::ParseVariableDeclaration
(
    _Inout_ ParseTree::VariableDeclarationList *Declarations,
    _Out_ bool &CurrentStatementInError,
    _Inout_ bool &ErrorInConstruct
)
{
    
    ParseTree::VariableDeclarationList **DeclarationTarget = &Declarations;

    // If there is a pre-existing list of declarations, let's find the end of the list.
    if (Declarations)
    {
        while ( (*DeclarationTarget) != NULL)
        {
            DeclarationTarget = &(Declarations->Next); 
        }
    }
    
    CurrentStatementInError = m_CurrentStatementInError;
    
    do
    {
        Token *DeclarationStart = m_CurrentToken;
        ParseTree::DeclaratorList *Declarators = NULL;
        ParseTree::DeclaratorList **DeclaratorTarget = &Declarators;

        m_CurrentStatementInError = false;

        // Parse the declarators.

        do
        {
            Token *DeclaratorStart = m_CurrentToken;

            ParseTree::Declarator *Declarator = new(m_TreeStorage) ParseTree::Declarator;

            ParseDeclarator(true, Declarator, ErrorInConstruct);

            if (ErrorInConstruct)
            {
                // Resync so we don't get more errors later.
                ResyncAt(5, tkAS, tkComma, tkNEW, tkEQ, tkREM);
            }

            SetLocation(&Declarator->TextSpan, DeclaratorStart, m_CurrentToken);            
            
            CreateListElement(
                DeclaratorTarget,
                Declarator,
                DeclaratorStart,
                m_CurrentToken,
                m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

            // If this is a declaration of a local, increment the locals count
            // of the containing block (skipping over Region blocks, which don't
            // establish scopes).

            ParseTree::BlockStatement *Context = m_Context;

            while (Context->Opcode == ParseTree::Statement::Region)
            {
                Context = Context->GetParent();
            }

            if (Context->IsExecutableBlock())   //  && !m_IsLineParse   this used to say !m_IsLineParse, but with stmt lambdas, we need locals count: Devdiv 491251
            {
                Context->AsExecutableBlock()->LocalsCount++;
            }

        } while (VariableDeclaratorsContinue());

        FixupListEndLocations(Declarators);

        // Parse the type clause.

        Token *As = NULL;
        Token *New = NULL;
        ParseTree::ParenthesizedArgumentList NewArguments;
        long DeferredInitializationTextStart = -1;
        ParseTree::Type *Type = NULL;
        Token *From = NULL;
        ParseTree::BracedInitializerList * CollectionInitializer = NULL;

        if (m_CurrentToken->m_TokenType == tkAS)
        {
            As = m_CurrentToken;
            GetNextToken();

            // At this point, we've seen the As so we're expecting a type.

            if (m_CurrentToken->m_TokenType == tkNEW)
            {
                New = m_CurrentToken;
                GetNextToken();

                Token *TypeStart = m_CurrentToken;
                Type = ParseTypeName(ErrorInConstruct);

                if (m_CurrentToken->m_TokenType == tkLParen)
                {
                    // New <Type> ( <Arguments> )

                    // The argument list must be preserved as text
                    // between declaration semantics and method body semantics.
                    DeferredInitializationTextStart = m_CurrentToken->m_StartCharacterPosition;

                    NewArguments = ParseParenthesizedArguments(ErrorInConstruct);
                }

                if (TokenAsKeyword(m_CurrentToken) == tkFROM)
                {
                    AssertLanguageFeature(FEATUREID_CollectionInitializers, m_CurrentToken);
                    if (DeferredInitializationTextStart  < 0)
                    {
                        DeferredInitializationTextStart  = m_CurrentToken->m_StartCharacterPosition;
                    }

                    From = m_CurrentToken;
                    GetNextToken();
                    EatNewLine(); // Dev10_497006 - allow implicit line continuation after 'FROM'
                    
                    CollectionInitializer = 
                        ParseInitializerList
                        (
                            ErrorInConstruct, 
                            true,  //allow expressions
                            false //don't allow assignments.
                        );
                }
            }
            else
            {
                Type = ParseGeneralType(ErrorInConstruct);

                if (ErrorInConstruct)
                {
                    ResyncAt(2, tkComma, tkEQ);
                }
            }
        }

        // Parse the initializer.

        Token *Equals = NULL;
        ParseTree::Initializer *InitialValue = NULL;

        if (m_CurrentToken->m_TokenType == tkEQ && New == NULL)
        {
            Equals = m_CurrentToken;
            GetNextToken(); // get off the '='
            EatNewLine(); // allow '=' to be followed by EOL  Dev10 410824

            InitialValue = ParseDeferredInitializer(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(1, tkComma);
            }
        }

        ParseTree::VariableDeclaration *Declaration;

        if (New)
        {
            Declaration = new(m_TreeStorage) ParseTree::NewVariableDeclaration;
            Declaration->Opcode = ParseTree::VariableDeclaration::WithNew;

            Declaration->AsNew()->Arguments = NewArguments;
            SetPunctuator(&Declaration->AsNew()->New, DeclarationStart, New);

            if (m_CurrentToken->m_TokenType == tkWITH)
            {
                if (From)
                {
                    ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, m_CurrentToken, ErrorInConstruct);
                }
                else
                {
                    if (-1 == DeferredInitializationTextStart)
                    {
                        DeferredInitializationTextStart = m_CurrentToken->m_StartCharacterPosition;
                    }

                    Declaration->AsNew()->ObjectInitializer = ParseObjectInitializerList(ErrorInConstruct);

                    if (TokenAsKeyword(m_CurrentToken) == tkFROM)
                    {
                        ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, m_CurrentToken, ErrorInConstruct);
                    }
                }
            }

            if (!ErrorInConstruct && DeferredInitializationTextStart > -1)
            {
                long DeferredInitializationTextEnd = m_CurrentToken->m_StartCharacterPosition;
                
                void *DeferredBytes =
                    new(m_TreeStorage)
                    char[sizeof(ParseTree::DeferredText) + ((DeferredInitializationTextEnd - DeferredInitializationTextStart) * sizeof(WCHAR))];
                ParseTree::DeferredText *DeferredInitializerText = new(DeferredBytes) ParseTree::DeferredText;

                DeferredInitializerText->TextLengthInCharacters = DeferredInitializationTextEnd - DeferredInitializationTextStart;
                m_InputStream->CopyString(DeferredInitializationTextStart, DeferredInitializationTextEnd, DeferredInitializerText->Text);

                Declaration->AsNew()->DeferredInitializerText = DeferredInitializerText;
            }

            if (From)
            {
                Declaration->AsNew()->CollectionInitializer = CollectionInitializer;
                SetPunctuator(&Declaration->AsNew()->From, DeclarationStart, From);
            }
        }
        else if (InitialValue)
        {
            Declaration = new(m_TreeStorage) ParseTree::InitializerVariableDeclaration;

            Declaration->Opcode = ParseTree::VariableDeclaration::WithInitializer;
            Declaration->AsInitializer()->InitialValue = InitialValue;
            SetPunctuator(&Declaration->AsInitializer()->Equals, DeclarationStart, Equals);
        }
        else
        {
            Declaration = new(m_TreeStorage) ParseTree::VariableDeclaration;
            Declaration->Opcode = ParseTree::VariableDeclaration::NoInitializer;
        }

        Declaration->Variables = Declarators;
        Declaration->Type = Type;
        Declaration->HasSyntaxError = m_CurrentStatementInError;
        CurrentStatementInError |= m_CurrentStatementInError;

        SetLocationAndPunctuator(
            &Declaration->TextSpan,
            &Declaration->As,
            DeclarationStart,
            m_CurrentToken,
            As);

        CreateListElement(
            DeclarationTarget,
            Declaration,
            DeclarationStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (VariableDeclarationsContinue());

    return Declarations;

}

ParseTree::ObjectInitializerList *
Parser::ParseObjectInitializerList
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkWITH, "ParseObjectInitializerList() parsing lost");

    // Handle the "With" clause in the following syntax:
    //  Dim x as new Customer With {.Id = 1, .Name = "A"}

    ParseTree::ObjectInitializerList *InitializerList = new(m_TreeStorage) ParseTree::ObjectInitializerList;

    // Set the location for the "With"
    SetLocation(&InitializerList->With, m_CurrentToken, m_CurrentToken);
    GetNextToken(); // Get off WITH
    EatNewLine(); // Dev10 622723 allow implicit line continuation after WITH

    // Parse the initializer list after the "With" keyword
    InitializerList->BracedInitializerList =
        ParseInitializerList(
            ErrorInConstruct,
            false,  // disallow expression initializers
            true,   // allow assignment initializers
            false,  // Not an anonymous type initializer
            true);  // require at least one initializer in the list

    return InitializerList;
}

bool
Parser::VariableDeclaratorsContinue
(
)
{
    Token *Next = m_CurrentToken;

    if (Next->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return true;
    }

    return false;
}

bool
Parser::VariableDeclarationsContinue
(
)
{
    Token *Next = m_CurrentToken;

    if (Next->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return true;
    }

    return false;
}

/*********************************************************************
*
* Function:
*
*     Parser::ParseInitializerList
*
* Purpose:
*     Parses a list of various kinds of initializers: <BracedInitializerList>
*         <BracedInitializerList> -> "{" {<InitializerList>} "}"
*         <InitializerList> -> Initializer {, Initializer}
*
*
**********************************************************************/
ParseTree::BracedInitializerList *
Parser::ParseInitializerList
(
    _Inout_ bool &ErrorInConstruct,
    bool AllowExpressionInitializers,
    bool AllowAssignmentInitializers,
    bool AnonymousTypeInitializer,
    bool RequireAtleastOneInitializer
)
{
    Token *Start = m_CurrentToken;

    ParseTree::BracedInitializerList *Result = new(m_TreeStorage) ParseTree::BracedInitializerList;

    if (false == VerifyExpectedToken(tkLBrace, ErrorInConstruct, true, Result))
    {
        return NULL;
    }

    EatNewLineIfFollowedBy(tkRBrace, Result);

    if (RequireAtleastOneInitializer &&
        (tkRBrace == m_CurrentToken->m_TokenType ||
            tkEOL == m_CurrentToken->m_TokenType ||
            tkColon == m_CurrentToken->m_TokenType))
    {
        ReportSyntaxError(ERRID_InitializerExpected, m_CurrentToken, ErrorInConstruct);
    }
    else if (tkRBrace != m_CurrentToken->m_TokenType)
    {
        ParseTree::InitializerList **ListTarget = &Result->InitialValues;
        ParseTree::Expression * initExpr = NULL;

        do
        {
            bool FieldIsKey = false;
            Token *InitializerStart = m_CurrentToken;
            Token *KeyToken = InitializerStart;

            // The token is a key identifier all the time in this position.

            if ( AnonymousTypeInitializer &&
                InitializerStart->m_TokenType == tkID &&
                IdentifierAsKeyword( InitializerStart ) == tkKey )
            {
                InitializerStart = GetNextToken();
#if 0                
                //ILC: undone
                //       If we uncomment this we need to figure out what tree node to pass into
                //       EatNewLine.
                EatNewLine();
#endif
                FieldIsKey = true;
            }

            ParseTree::Initializer *Initializer =
                ParseInitializer(
                    ErrorInConstruct,
                    false,
                    AllowExpressionInitializers,
                    AllowAssignmentInitializers);


            // ParseInitializer always sets FieldIsKey to true.

            Initializer->FieldIsKey = false;

            if ( FieldIsKey )
            {
                Initializer->FieldIsKey = FieldIsKey;
                SetPunctuator(&Initializer->Key, KeyToken, KeyToken);
            }

            if (Initializer &&
                AnonymousTypeInitializer &&
                Initializer->Opcode == ParseTree::Initializer::Expression &&
                (initExpr = (Initializer->AsExpression()->Value)) &&
                initExpr->Opcode != ParseTree::Expression::SyntaxError)
            {
                ParseTree::IdentifierDescriptor *PropertyName = NULL;
                bool IsNameBangQualified = false;
                bool IsXMLNameRejectedAsBadVBIdentifier = false;
                ParseTree::XmlNameExpression *XMLNameInferredFrom = NULL;

                PropertyName =
                    Semantics::ExtractAnonTypeMemberName(
                        initExpr,
                        IsNameBangQualified,
                        IsXMLNameRejectedAsBadVBIdentifier,
                        XMLNameInferredFrom);

                if (!(PropertyName && !PropertyName->IsBad && PropertyName->Name) )
                {
                    switch(initExpr->Opcode)
                    {
                        case ParseTree::Expression::IntegralLiteral:                                // IntegralLiteralExpression
                        case ParseTree::Expression::CharacterLiteral:                               // CharacterLiteralExpression
                        case ParseTree::Expression::BooleanLiteral:                                 // BooleanLiteralExpression
                        case ParseTree::Expression::DecimalLiteral:                                 // DecimalLiteralExpression
                        case ParseTree::Expression::FloatingLiteral:                               // FloatingLiteralExpression
                        case ParseTree::Expression::DateLiteral:                                    // DateLiteralExpression
                        case ParseTree::Expression::StringLiteral:                                  // StringLiteralExpression
                        case ParseTree::Expression::Nothing:                                        // Expression
                            ReportSyntaxError(ERRID_AnonymousTypeExpectedIdentifier, InitializerStart, ErrorInConstruct);
                            break;

                        case ParseTree::Expression::Equal:
                            // (Bug #27657 - DevDiv Bugs)
                            if(initExpr->AsBinary()->Left &&
                                initExpr->AsBinary()->Left->Opcode == ParseTree::Expression::Name &&
                                initExpr->AsBinary()->Left->AsName()->Name.Name)
                            {
                                ReportSyntaxError(ERRID_AnonymousTypeNameWithoutPeriod, &initExpr->AsBinary()->Left->TextSpan, ErrorInConstruct);
                                break;
                            }

                            __fallthrough; // fall through
                        default:
                            ResyncAt(2, tkComma, tkRBrace);

                            if (IsXMLNameRejectedAsBadVBIdentifier && XMLNameInferredFrom)
                            {
                                ReportSyntaxError(ERRID_AnonTypeFieldXMLNameInference, &XMLNameInferredFrom->TextSpan, ErrorInConstruct);
                            }
                            else
                            {
                                ReportSyntaxError(ERRID_AnonymousTypeFieldNameInference, InitializerStart, m_CurrentToken, ErrorInConstruct);
                            }
                            break;
                    }
                }
            }

            if (ErrorInConstruct)
            {
                ResyncAt(2, tkComma, tkRBrace);
            }
            
            CreateListElement(
                ListTarget,
                Initializer,
                InitializerStart,
                m_CurrentToken,
                m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        } while (InitializersContinue());

        FixupListEndLocations(Result->InitialValues);
    }

    EatNewLineIfFollowedBy(tkRBrace, Result);
    Result->RBracePresent = VerifyExpectedToken(tkRBrace, ErrorInConstruct);

    SetLocation(&Result->TextSpan, Start, m_CurrentToken);

    return Result;
}

// Test to see if an initializer list continues, and consume a separating comma if present.

bool
Parser::InitializersContinue
(
)
{
    Token *Next = m_CurrentToken;
    EatNewLineIfFollowedBy(tkRBrace);

    if (Next->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return true;
    }

    else if (Next->m_TokenType == tkRBrace || MustEndStatement(Next))
    {
        return false;
    }

    else
    {
        // There is a syntax error of some kind.
        //
        // 
    }

    return false;
}

// Parse a list of comma-separated keyword Initializers.
ParseTree::AssignmentInitializer *
Parser::ParseAssignmentInitializer
(
    _Inout_ bool &ErrorInConstruct,
    bool MakeExpressionsDeferred
)
{
    Token *InitializerStart = m_CurrentToken;

    ParseTree::AssignmentInitializer *Result = new(m_TreeStorage) ParseTree::AssignmentInitializer;
    Result->Opcode = ParseTree::Initializer::Assignment;
    Result->FieldIsKey = true;

    Token *Dot = NULL;
    Token *Equals = NULL;

    // Parse form: '.'<IdentiferOrKeyword> '=' <Expression>

    if (InitializerStart->m_TokenType == tkDot)
    {
        Dot = InitializerStart;
        GetNextToken();

        Result->Name = ParseIdentifierAllowingKeyword(ErrorInConstruct);

        if (tkNullable == m_CurrentToken->m_TokenType)
        {
            GetNextToken();

            Location textSpan;
            SetLocation(&textSpan,InitializerStart, m_CurrentToken);

            ReportSyntaxError
            (
                ERRID_NullableTypeInferenceNotSupported,
                &textSpan,
                ErrorInConstruct
            );
        }


        if (m_CurrentToken->m_TokenType == tkEQ)
        {
            Equals = m_CurrentToken;
            GetNextToken();
            EatNewLine();
        }
        else
        {
            ReportSyntaxError(
                ERRID_ExpectedAssignmentOperatorInInit, m_CurrentToken, ErrorInConstruct);

            // Name is bad because only a simple name is allowed. But this is arguable.
            // This is required for semantics to avoid giving more confusing errors to the user in this context.
            Result->Name.IsBad = true;
        }
    }
    else
    {
        // Assume that the "'.'<IdentiferOrKeyword> '='" was left out.

        ReportSyntaxError(ERRID_ExpectedQualifiedNameInInit, m_CurrentToken, ErrorInConstruct);
        Result->Name.IsBad = true;
    }

    Result->Initializer =
        ParseInitializer(
            ErrorInConstruct,
            MakeExpressionsDeferred,
            true,   // allow expression initializer
            false);  // disallow assignment initializer


    SetLocation(
        &Result->TextSpan,
        InitializerStart,
        m_CurrentToken);

    SetPunctuator(
        &Result->Dot,
        InitializerStart,
        Dot);

    SetPunctuator(
        &Result->Equals,
        InitializerStart,
        Equals);

    return Result;
}

/*********************************************************************
*
* Function:
*
*     Parser::ParseInitializer
*
* Purpose:
*     Parses a variable initializer:
*         <"{" [ InitializerList ] "}"> |
*         <expr>
*
*         <InitializerList> -> Initializer {, Initializer}
*
*     The initializer list is added for array initialization
*
*     Current token should be at the beginning of the initializer
*
**********************************************************************/
ParseTree::Initializer *
Parser::ParseInitializer
(
    _Inout_ bool &ErrorInConstruct,
    bool MakeExpressionsDeferred,
    bool AllowExpressionInitializer,
    bool AllowAssignmentInitializer
)
{
    AssertIfFalse(AllowAssignmentInitializer || AllowExpressionInitializer);

    if (AllowAssignmentInitializer &&
        (m_CurrentToken->m_TokenType == tkDot || // Named initializer of form "."<Identifier>"="
         false == AllowExpressionInitializer))
    {
        return ParseAssignmentInitializer(ErrorInConstruct, MakeExpressionsDeferred);
    }

    if (AllowExpressionInitializer)
    {
        ParseTree::ExpressionInitializer *Result = new(m_TreeStorage) ParseTree::ExpressionInitializer;
        Result->Opcode = ParseTree::Initializer::Expression;
        Result->Value =
            MakeExpressionsDeferred ?
                ParseDeferredExpression(ErrorInConstruct) :
                ParseExpression(ErrorInConstruct);
        Result->FieldIsKey = true;

        return Result;
    }

    ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);

    ParseTree::ExpressionInitializer *Result = new(m_TreeStorage) ParseTree::ExpressionInitializer;
    Result->Opcode = ParseTree::Initializer::Expression;
    Result->Value = ExpressionSyntaxError();
    Result->FieldIsKey = true;
    return Result;
}

void
Parser::ParseGenericArguments
(
    Token *Start,           // [in] the start token of the statement or expression containing the generic arguments
    ParseTree::GenericArguments &Arguments,
    _Inout_ bool &AllowEmptyGenericArguments,
    _Inout_ bool &AllowNonEmptyGenericArguments,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Of = NULL;
    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    Arguments.Arguments =
        ParseGenericArguments(
            Of,
            LeftParen,
            RightParen,
            AllowEmptyGenericArguments,
            AllowNonEmptyGenericArguments,
            ErrorInConstruct);

    Arguments.Opcode =
        AllowEmptyGenericArguments ?
            ParseTree::GenericArguments::WithoutTypes :
            ParseTree::GenericArguments::WithTypes;

    SetPunctuator(&Arguments.Of, Start, Of);
    SetPunctuator(&Arguments.LeftParen, Start, LeftParen);
    SetPunctuator(&Arguments.RightParen, Start, RightParen);
}

ParseTree::TypeList *
Parser::ParseGenericArguments
(
    _Out_ Token *&Of,
    _Out_ Token *&LeftParen,
    _Out_ Token *&RightParen,
    _Inout_ bool &AllowEmptyGenericArguments,
    _Inout_ bool &AllowNonEmptyGenericArguments,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(AllowEmptyGenericArguments || AllowNonEmptyGenericArguments,
                "Cannot disallow both empty and non-empty generic arguments!!!");

    Of = NULL;
    LeftParen = NULL;
    RightParen = NULL;

    VSASSERT(m_CurrentToken->m_TokenType == tkLParen, "Generic arguments parsing lost!!!");

    LeftParen = m_CurrentToken;
    GetNextToken(); // get off '('
    EatNewLine(); // '(' allows implicit line continuation

    if (m_CurrentToken->m_TokenType == tkOF)
    {
        Of = m_CurrentToken;
        GetNextToken();
    }
    else
    {
        ReportSyntaxErrorWithoutMarkingStatementBad(
            ERRID_OfExpected,
            m_CurrentToken,
            ErrorInConstruct);
    }
    
    ParseTree::TypeList *Arguments = NULL;
    ParseTree::TypeList **Target = &Arguments;
    ParseTree::TypeList **LastTarget = NULL;
    ParseTree::Type *Type = NULL;
    bool first = true;

    do
    {
        Type = NULL;
        
        EatNewLineIfNotFirst(first);
        EatNewLineIfFollowedBy(tkRParen);
     
        Token *StartName = m_CurrentToken;

        // Either all generic arguments should be unspecified or all need to be specified.

        if (m_CurrentToken->m_TokenType == tkComma || m_CurrentToken->m_TokenType == tkRParen)
        {
            if (AllowEmptyGenericArguments)
            {
                // If a non-empty type argument is already specified, then need to always look for
                // non-empty type arguments, else we can allow empty type arguments.

                AllowNonEmptyGenericArguments = false;
            }
            else
            {
                Type = ParseGeneralType(ErrorInConstruct);
            }
        }
        else
        {
            // If an empty type argument is already specified, then need to always look for
            // empty type arguments and reject non-empty type arguments, else we can allow
            // non-empty type arguments.

            if (AllowNonEmptyGenericArguments)
            {
                Type = ParseGeneralType(ErrorInConstruct);
                AllowEmptyGenericArguments = false;
            }
            else
            {
                ReportSyntaxError(
                    ERRID_TypeParamMissingCommaOrRParen,
                    m_CurrentToken,
                    ErrorInConstruct);
            }
        }

        VSASSERT(AllowEmptyGenericArguments || AllowNonEmptyGenericArguments,
                    "Cannot disallow both empty and non-empty generic arguments!!!");

        if (AllowEmptyGenericArguments)
        {
            // There is no argument. What is the location of something that doesn't exist?
            // Treating the non-existing argument as starting at the current token
            // causes problems, because in that case the last element of the argument list 
            // will have a location outside the list.

            StartName = m_CurrentToken->m_Prev;
            // Dev10_535407 - If implicit line continuation happened after the 'OF', then we need to back up to the OF token since
            // that was the intent of the line above before implicit line continuation came along.
            if ( StartName->m_TokenType == tkEOL )
            {
                StartName = StartName->m_Prev;
            }
        }

        if (ErrorInConstruct)
        {
            ResyncAt(2, tkRParen, tkComma);
        }
        
        LastTarget = Target;

        CreateListElement(
            Target,
            Type,
            StartName,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        if (m_CurrentToken->m_TokenType != tkComma)
        {
            break;
        }

        GetNextToken();
    } while (true);


    FixupListEndLocations(Arguments);

    if (LeftParen)
    {
        EatNewLineIfFollowedBy(tkRParen, *LastTarget);
        if (m_CurrentToken->m_TokenType == tkRParen)
        {
            RightParen = m_CurrentToken;
        }
        else if (!m_InterpretationUnderIntelliSense)
        {
            // Bug VSWhidbey 157413
            // Propagate badness in the type argument list to the last type argument.
            // This is needed to avoid spurious binding errors in the other parts of the
            // compiler during name binding. It is better to propagate this badness than
            // trying to resync at the right paren in order to avoid spurious errors that
            // could possibly result due to a bad resync in extremely bad parse scenarios.
            //
            if (Type)
            {
                Type->Opcode = ParseTree::Type::SyntaxError;
            }
        }

        VerifyExpectedToken(tkRParen, ErrorInConstruct);
    }

    return Arguments;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDeclarator
*
* Purpose:
*     Parses: Identifier[ArrayList]
*     in a variable declaration or a type field declaration.
*
*     Current token should be at beginning of expected declarator.
*
*     The result will have been created by the caller.
*
**********************************************************************/
void
Parser::ParseDeclarator
(
    bool AllowExplicitArraySizes,
    _Out_ ParseTree::Declarator *Result,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *IdentifierStart = m_CurrentToken;

    // Often, programmers put extra decl specifiers where they are
    // not required. Eg:
    //    Dim x as Integer, Dim y as Long
    // We want to check for this and give a more informative error.

    bool Errors = DisableErrors();
    bool ErrorInAttempt = false;
    bool save_m_CurrentStatementInError = m_CurrentStatementInError;
    Result->Name = ParseIdentifier(ErrorInAttempt, true /*allowNullable*/);
    EnableErrors(Errors);

    //We don't want to look for specifiers if the errorneous declarator starts on a new line.
    //This is because we want to recover the error on the previous line and treat the line with the
    //specifier as a new statement
    if (ErrorInAttempt)
    {
        // Reset the current token and try to see if specifiers were
        // used.

        m_CurrentToken = IdentifierStart;
        Errors = DisableErrors();
        ErrorInAttempt = false;

        ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInAttempt);

        EnableErrors(Errors);

        if (Specifiers)
        {
            if (IdentifierStart->m_Prev && IdentifierStart->m_Prev->IsEndOfLine())
            {
                // Dev10 #635488 reset to EOL and parse Identifier from there to get proper errors reported
                m_CurrentToken = IdentifierStart->m_Prev;
                m_CurrentStatementInError = save_m_CurrentStatementInError; 

                Result->Name = ParseIdentifier(ErrorInConstruct);

                m_CurrentToken = IdentifierStart;
                return;
            }

            ReportSyntaxError(ERRID_ExtraSpecifiers, IdentifierStart, m_CurrentToken, ErrorInConstruct);
        }

        // Try to parse a declarator again. We don't mark the
        // declarator with an error even though there really was an error.
        // If we do get back a valid declarator, we have a well-formed tree.
        // We've corrected the error. Otherwise, the second parse is necessary in order
        // to produce a diagnostic.

        // 




        //m_CurrentToken = IdentifierStart;
        Result->Name = ParseIdentifier(ErrorInConstruct);
    }

    // Check for an array declarator.

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        Result->ArrayInfo = ParseArrayDeclarator(NULL, NULL, AllowExplicitArraySizes, false, ErrorInConstruct);
    }
}

bool
Parser::IsIntrinsicType
(_In_
    Token *Token
)
{
    switch (Token->m_TokenType)
    {
        case tkSHORT:
        case tkUSHORT:
        case tkINTEGER:
        case tkUINTEGER:
        case tkLONG:
        case tkULONG:
        case tkDECIMAL:
        case tkSINGLE:
        case tkDOUBLE:
        case tkSBYTE:
        case tkBYTE:
        case tkBOOLEAN:
        case tkCHAR:
        case tkDATE:
        case tkSTRING:
        case tkVARIANT:
        case tkOBJECT:
            return true;
    }

    return false;
}

bool
Parser::CanTokenStartTypeName
(_In_
    Token *Token
)
{
    if (IsIntrinsicType(Token))
    {
        return true;
    }

    switch (Token->m_TokenType)
    {
        case tkGLOBAL:
        case tkID:
            return true;
    }

    return false;
}


class MissingTypeArgumentVisitor : public AutoParseTreeVisitor
{
private:
    Parser * m_pParser;
    ErrorTable * m_pErrors;
    bool & m_ErrorInConstruct;

public:
    
    MissingTypeArgumentVisitor
    (
        Parser * pParser,
        ErrorTable * pErrors,
        bool &ErrorInConstruct
    ) :
    m_ErrorInConstruct(ErrorInConstruct)
    {
        m_pParser = pParser;
        m_pErrors = pErrors;
    }


    virtual void VisitSimpleWithArgumentsName(ParseTree::SimpleWithArgumentsName * pName)
    {
        if (pName)
        {
            CheckGenericArguments(pName->TextSpan, pName->Arguments);
        }
    }
    
    virtual void VisitQualifiedWithArgumentsName(ParseTree::QualifiedWithArgumentsName * pName)
    {
        if (pName)
        {
            CheckGenericArguments(pName->TextSpan, pName->Arguments);
        }
    }

private:
    
    void CheckGenericArguments(const Location & TextSpan, ParseTree::GenericArguments & pGenericArguments)
    {
        if (pGenericArguments.Opcode == ParseTree::GenericArguments::WithoutTypes)
        {
            // If we don't have the list, something is really wrong and we should have reported an error by now.
            if (pGenericArguments.Arguments) 
            {
                if (pGenericArguments.Arguments->Element == NULL)
                {
                    // we need to report an error now
                    Location errorLocation= {0,0,0,0}; 
                
                    // It looks like we don't need to worry about reporting duplicate errors because
                    // when Parser::ParseGenericArguments reports this error, it doesn't use
                    // ParseTree::GenericArguments::WithoutTypes opcode and it adds 
                    // ParseTree::Type::SyntaxError node as an element into the Arguments list.
                    
                    if (pGenericArguments.Arguments->Next == NULL)
                    {
                        // Only one generic type argument
                        // if we have location for the right paren, use it as the error location
                        if (pGenericArguments.RightParen.Column!=0 || pGenericArguments.RightParen.Line!=0)
                        {
                            errorLocation.SetLocation(TextSpan.m_lBegLine, &pGenericArguments.RightParen, 0);
                        }
                        else
                        {
                            // report the error at the end of the list
                            errorLocation = pGenericArguments.Arguments->TextSpan;
                            errorLocation.SetStart(errorLocation.m_lEndLine, errorLocation.m_lEndColumn);
                        }
                    }
                    else
                    {
                        // there is more than one argument, report the error at the first comma
                        errorLocation.SetLocation(pGenericArguments.Arguments->TextSpan.m_lBegLine, 
                                            &pGenericArguments.Arguments->Punctuator, 0);

                    }

                    bool ErrorInConstruct = false; //force the error to be reported
                    unsigned errorCount = m_pErrors ? m_pErrors->GetErrorCount() : 0;

                    m_pParser->ReportSyntaxError(ERRID_UnrecognizedType, &errorLocation, ErrorInConstruct);

                    m_ErrorInConstruct = (m_ErrorInConstruct || ErrorInConstruct);

                    // Need to delete ERRID_TypeParamMissingCommaOrRParen error for other items in the list
                    if (pGenericArguments.Arguments->Next != NULL && m_pErrors && 
                        errorCount < m_pErrors->GetErrorCount())
                    {
                        m_pErrors->DeleteSpecificError(ERRID_TypeParamMissingCommaOrRParen, &pGenericArguments.Arguments->Next->TextSpan);

                        if (m_pErrors->GetErrorCount() == 0)
                        {
                            AssertIfFalse(m_pErrors->GetErrorCount());
                            ErrorInConstruct = false; //force the error to be reported
                            errorLocation = TextSpan;
                            m_pParser->ReportSyntaxError(ERRID_InternalCompilerError, &errorLocation, ErrorInConstruct);
                        }
                    }
                }
            }
        }
    }
    
};

/*********************************************************************
*
* Function:
*     Parser::ParseTypeName
*
* Purpose:
*     Parses a Type name.
**********************************************************************/
ParseTree::Type *
Parser::ParseTypeName
(
    _Inout_ bool &ErrorInConstruct,
    bool AllowEmptyGenericArguments,
    _Out_opt_ bool *AllowedEmptyGenericArguments
)
{
    ParseTree::Type::Opcodes Opcode;

    Token *Start = m_CurrentToken;
    ParseTree::Type *Type;

    switch (Start->m_TokenType)
    {
        case tkSHORT:
            Opcode = ParseTree::Type::Short;
            break;
        case tkUSHORT:
            Opcode = ParseTree::Type::UnsignedShort;
            break;
        case tkINTEGER:
            Opcode = ParseTree::Type::Integer;
            break;
        case tkUINTEGER:
            Opcode = ParseTree::Type::UnsignedInteger;
            break;
        case tkLONG:
            Opcode = ParseTree::Type::Long;
            break;
        case tkULONG:
            Opcode = ParseTree::Type::UnsignedLong;
            break;
        case tkDECIMAL:
            Opcode = ParseTree::Type::Decimal;
            break;
        case tkSINGLE:
            Opcode = ParseTree::Type::Single;
            break;
        case tkDOUBLE:
            Opcode = ParseTree::Type::Double;
            break;
        case tkSBYTE:
            Opcode = ParseTree::Type::SignedByte;
            break;
        case tkBYTE:
            Opcode = ParseTree::Type::Byte;
            break;
        case tkBOOLEAN:
            Opcode = ParseTree::Type::Boolean;
            break;
        case tkCHAR:
            Opcode = ParseTree::Type::Char;
            break;
        case tkDATE:
            Opcode = ParseTree::Type::Date;
            break;
        case tkSTRING:
            Opcode = ParseTree::Type::String;
            break;
        case tkVARIANT:
            {
                // Variant is now object

                ReportSyntaxErrorWithoutMarkingStatementBad(
                    ERRID_ObsoleteObjectNotVariant,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            __fallthrough;
        case tkOBJECT:
            Opcode = ParseTree::Type::Object;
            break;

        case tkGLOBAL:
        case tkID:
        {
            ParseTree::Name *TypeName =
                ParseName(
                    false,
                    ErrorInConstruct,
                    true,   // AllowGlobalNameSpace
                    true,   // Alow generic arguments
                    false,  // Don't disallow generic arguments on last qulaified name
                    AllowEmptyGenericArguments,
                    AllowedEmptyGenericArguments);

            ParseTree::NamedType *NamedType = new(m_TreeStorage) ParseTree::NamedType;
            NamedType->Opcode = ParseTree::Type::Named;
            NamedType->TypeName = TypeName;
            SetLocation(&NamedType->TextSpan, Start, m_CurrentToken);

            VSASSERT(CanTokenStartTypeName(Start), "Inconsistency in type parsing routines!!!");
            Type = NamedType;
            goto checkNullable;

        }

        default:
        {
            RESID errorID = 0;
            if (Start->m_TokenType == tkNEW && Start->m_Next->m_TokenType == tkID)
            {
                errorID = ERRID_InvalidNewInType;
            }
            else if (Start->m_Prev && Start->m_Prev->m_TokenType == tkNEW && Start->m_TokenType == tkLBrace)
            {
                errorID = ERRID_UnrecognizedTypeOrWith;
            }
            else if (Start->IsKeyword())
            {
                errorID = ERRID_UnrecognizedTypeKeyword;
            }
            else
            {
                errorID = ERRID_UnrecognizedType;
            }


            ReportSyntaxError(errorID, m_CurrentToken, ErrorInConstruct);

            ParseTree::Type *ErrorType = new(m_TreeStorage) ParseTree::Type;
            ErrorType->Opcode = ParseTree::Type::SyntaxError;
            SetLocation(&ErrorType->TextSpan, Start, m_CurrentToken);

            VSASSERT(!CanTokenStartTypeName(Start), "Inconsistency in type parsing routines!!!");

            return ErrorType;
        }
    }

    VSASSERT(CanTokenStartTypeName(Start), "Inconsistency in type parsing routines!!!");

    GetNextToken();

    Type = new(m_TreeStorage) ParseTree::Type;
    Type->Opcode = Opcode;
    SetLocation(&Type->TextSpan, Start, m_CurrentToken);

checkNullable:
    if (tkNullable == m_CurrentToken->m_TokenType)
    {
        if (m_EvaluatingConditionCompilationExpression)
        {
            ReportSyntaxError(
                ERRID_BadNullTypeInCCExpression,
                m_CurrentToken,
                ErrorInConstruct);

            ParseTree::Type *ErrorType = new(m_TreeStorage) ParseTree::Type;
            ErrorType->Opcode = ParseTree::Type::SyntaxError;
            SetLocation(&ErrorType->TextSpan, Start, m_CurrentToken);

            return ErrorType;
        }

        MissingTypeArgumentVisitor missingTypeArgumentErrorVisitor(this, this->m_Errors, ErrorInConstruct);

        // Dev10 #512958
        missingTypeArgumentErrorVisitor.VisitType(Type);

        ParseTree::NullableType *NullableType = new(m_TreeStorage) ParseTree::NullableType;
        NullableType->Opcode = ParseTree::Type::Nullable;
        NullableType->ElementType = Type;

        Token *pQuestionMark = m_CurrentToken;
        GetNextToken();

        SetLocationAndPunctuator(
            &NullableType->TextSpan,
            &NullableType->QuestionMark,
            Start,
            m_CurrentToken,
            pQuestionMark);

        Type = NullableType;
    }

    return Type;
}

// Parse a simple type followed by an optional array list.

ParseTree::Type *
Parser::ParseGeneralType
(
    _Inout_ bool &ErrorInConstruct,
    bool AllowEmptyGenericArguments
)
{
    Token *Start = m_CurrentToken;

    if (m_EvaluatingConditionCompilationExpression && !IsIntrinsicType(Start))
    {
        ReportSyntaxError(
            ERRID_BadTypeInCCExpression,
            Start,
            ErrorInConstruct);

        ParseTree::Type *ErrorType = new(m_TreeStorage) ParseTree::Type;
        ErrorType->Opcode = ParseTree::Type::SyntaxError;
        SetLocation(&ErrorType->TextSpan, Start, Start);

        return ErrorType;
    }

    bool AllowedEmptyGenericArguments = false;

    ParseTree::Type *Result =
        ParseTypeName(ErrorInConstruct, AllowEmptyGenericArguments, &AllowedEmptyGenericArguments);

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        if (AllowedEmptyGenericArguments)
        {
            ReportSyntaxError(
                ERRID_ArrayOfRawGenericInvalid,
                m_CurrentToken,
                ErrorInConstruct);

            // Need to eat up the array syntax to avoid spuriously parsing
            // the array syntax "(10)" as default property syntax for
            // constructs such a GetType(A(Of )()) and GetType(A(Of )()()()).
            // Even resyncing to tkRParen will not help in the array of array
            // cases. So instead using ParseArrayDeclarator to help skip
            // all of the array syntax.
            //
            ParseArrayDeclarator(Start, Result, false, false, ErrorInConstruct);
        }
        else
        {
            Result = ParseArrayDeclarator(Start, Result, false, false, ErrorInConstruct);
        }
    }

    return Result;
}

ParseTree::ArrayType *
Parser::ParseArrayDeclarator
(
    _In_opt_ Token *StartType,
    _In_opt_ ParseTree::Type *ElementType,
    bool AllowExplicitSizes,
    bool InnerArrayType,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkLParen,
        "should be a (.");

    if (StartType == NULL)
    {
        StartType = m_CurrentToken;
    }
    ParseTree::ArrayType *ArrayType = NULL;
    Token *LeftParen = m_CurrentToken;

    GetNextToken();
    EatNewLine();

    unsigned Rank = 0;

    EatNewLineIfFollowedBy(tkRParen);
    
    if (m_CurrentToken->m_TokenType == tkRParen || m_CurrentToken->m_TokenType == tkComma)
    {
        ArrayType = new(m_TreeStorage) ParseTree::ArrayType;
        ArrayType->Opcode = ParseTree::Type::ArrayWithoutSizes;

        Rank++;

        while (m_CurrentToken->m_TokenType == tkComma)
        {
            Rank++;
            GetNextToken();
            EatNewLine();
        }
    }

    else
    {
        if (!AllowExplicitSizes)
        {
            ReportSyntaxError(
                InnerArrayType ? ERRID_NoConstituentArraySizes : ERRID_NoExplicitArraySizes,
                m_CurrentToken,
                ErrorInConstruct);
        }

        ArrayType = new(m_TreeStorage) ParseTree::ArrayWithSizesType;
        ArrayType->Opcode = ParseTree::Type::ArrayWithSizes;

        ParseTree::ArrayDimList **ListTarget = &ArrayType->AsArrayWithSizes()->Dims;

        do
        {
            Rank++;
            Token *DimStart = m_CurrentToken;
            ParseTree::ArrayDim *Dim = new(m_TreeStorage) ParseTree::ArrayDim;

            Dim->upperBound = ParseDeferredExpression(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(3, tkComma, tkRParen, tkAS);
            }
            if (!ErrorInConstruct && m_CurrentToken->m_TokenType == tkTO)
            {
                Dim->lowerBound = Dim->upperBound;
                SetPunctuator(&Dim->To, DimStart, m_CurrentToken);

                if(Dim->lowerBound ->AsDeferred()->Value->Opcode != ParseTree::Expression::IntegralLiteral ||
                    Dim->lowerBound ->AsDeferred()->Value->AsIntegralLiteral()->Value !=0)
                {
                    ReportSyntaxError(
                        ERRID_OnlyNullLowerBound,
                        &Dim->lowerBound->TextSpan,
                        ErrorInConstruct);
                }
                GetNextToken(); // consume tkTO
#if 0
                //ILC: undone
                //     if we enable this code we need to figure out what tree node to pass in
                //     to EatNewLine below.
                EatNewLine();
#endif
                Dim->upperBound = ParseDeferredExpression(ErrorInConstruct);
            }
            SetLocation(&Dim->TextSpan, DimStart, m_CurrentToken);
            if (ErrorInConstruct)
            {
                ResyncAt(3, tkComma, tkRParen, tkAS);
            }

            CreateListElement(
                ListTarget,
                Dim,
                DimStart,
                m_CurrentToken,
                m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        } while (ArrayBoundsContinue(ErrorInConstruct));

        FixupListEndLocations(ArrayType->AsArrayWithSizes()->Dims);
    }

    EatNewLineIfFollowedBy(tkRParen, ArrayType);

    VerifyExpectedToken(tkRParen, ErrorInConstruct);

    ArrayType->Rank = Rank;
    Token *EndType = m_CurrentToken;

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        ArrayType->ElementType =
            ParseArrayDeclarator(
                m_CurrentToken,
                ElementType,
                false,
                true,
                ErrorInConstruct);

        if (!ErrorInConstruct)
        {
            EndType = m_CurrentToken;
        }
    }
    else
    {
        ArrayType->ElementType = ElementType;
    }

    SetLocationAndPunctuator(
        &ArrayType->TextSpan,
        &ArrayType->LeftParen,
        StartType,
        EndType,
        LeftParen);

    return ArrayType;
}

// Test to see if a list of array bounds continues, and consume a separating comma if present.

bool
Parser::ArrayBoundsContinue
(
    _Inout_ bool &ErrorInConstruct
)
{
    
    Token *Next = m_CurrentToken;
    EatNewLineIfFollowedBy(tkRParen);

    if (Next->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return true;
    }

    else if (Next->m_TokenType == tkRParen || MustEndStatement(Next))
    {
        return false;
    }

    else
    {
        // There is a syntax error of some kind.
        //
        // 

        // At very least, if we see a "To", give an error on the old syntax
        if (Next->m_TokenType == tkTO)
        {
            ReportSyntaxError(
                ERRID_ObsoleteArrayBounds,
                m_CurrentToken,
                ErrorInConstruct);
        }
    }

    return false;
}

/*********************************************************************
*
* Function:
*     Parser::ParseProcedureDefinition
*
* Purpose:
*
**********************************************************************/
void
Parser::ParseProcedureDefinition
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,
    _In_ Token *StmtStart,
    _Inout_ bool &ErrorInConstruct
)
{
    // Parse a method definition in the module.

    if (!m_FirstStatementOnLine)
    {
        ReportSyntaxErrorWithoutMarkingStatementBad(ERRID_MethodMustBeFirstStatementOnLine, m_CurrentToken, ErrorInConstruct);
    }

    ParseTree::MethodDefinitionStatement *Procedure = NULL;
    tokens ProcType;

    ProcType = m_CurrentToken->m_TokenType;
    switch (ProcType)
    {
        case tkSUB:
            Procedure = ParseSubDeclaration(Attributes, Specifiers, StmtStart, false, ErrorInConstruct)->AsMethodDefinition();
            break;

        case tkFUNCTION:
            Procedure = ParseFunctionDeclaration(Attributes, Specifiers, StmtStart, false, ErrorInConstruct)->AsMethodDefinition();
            break;

        case tkOPERATOR:
            Procedure = ParseOperatorDeclaration(Attributes, Specifiers, StmtStart, false, ErrorInConstruct)->AsMethodDefinition();
            break;

        default:
            VSFAIL("Expected method definition.");
    }

    LinkStatement(Procedure);

    if (Specifiers->HasSpecifier(ParseTree::Specifier::MustOverride) &&
        (ProcType == tkSUB ||
         ProcType == tkFUNCTION))
    {
        // Mustoverride method does not need an end construct.
        Procedure->HasProperTermination = true;
    }
    else
    {
        // Deal with the body of the method.

        if (m_IsDeclAndMethodBodiesParse)
        {
            m_ParsingMethodBody = true;

            ParseTree::MethodBodyStatement *MethodBody = new(m_TreeStorage) ParseTree::MethodBodyStatement;
            switch (ProcType)
            {
                case tkSUB:
                    MethodBody->Opcode = ParseTree::Statement::ProcedureBody;
                    break;
                case tkFUNCTION:
                    MethodBody->Opcode = ParseTree::Statement::FunctionBody;
                    break;
                case tkOPERATOR:
                    MethodBody->Opcode = ParseTree::Statement::OperatorBody;
                    break;
                default:
                    VSFAIL("Surprising method definition.");
                    break;
            }
            SetStmtBegLocation(MethodBody, m_CurrentToken);

            MethodBody->SetParent(m_Context);
            EstablishContext(MethodBody);

            // Parse the body
            while (!m_CurrentToken->IsEndOfParse())
            {
                ParseLine(&Parser::ParseStatementInMethodBody);

                // Get the next line
                GetTokensForNextLine(true);
            }

            m_ParsingMethodBody = false;

            SetStmtEndLocation(MethodBody, m_CurrentToken);

            Procedure->EntireMethodLocation.m_lEndLine = m_CurrentToken->m_StartLine;
            Procedure->EntireMethodLocation.m_oEnd = m_CurrentToken->m_StartCharacterPosition;

            Procedure->Body = MethodBody;
        }
        else
        {
            bool ProcEndsNormally = FindEndProc(ProcType, StmtStart, Procedure, ErrorInConstruct);

            // If the body did not end normally, the token stream is presently pointed at the
            // start of a statement presumed to start a new module-level declaration.
            if (!ProcEndsNormally)
            {
                ParseDeclarationStatement(ErrorInConstruct);
            }
        }
    }
}


/*********************************************************************
*
* Function:
*     Parser::ParsePropertyProcedureDefinition
*
* Purpose:
*
**********************************************************************/
void
Parser::ParsePropertyOrEventProcedureDefinition
(
    _In_ Token *StmtStart,
    _Inout_ bool &ErrorInConstruct
)
{
    // Parse a property or event method definition in the module.

    bool IsPropertyContext = (RealContext()->Opcode == ParseTree::Statement::Property);

    VSASSERT(RealContext()->Opcode == ParseTree::Statement::Property ||
             RealContext()->Opcode == ParseTree::Statement::BlockEventDeclaration, "Property or event context expected!!!");

    ParseTree::AttributeSpecifierList *Attributes = NULL;
    if (m_CurrentToken->m_TokenType == tkLT)
    {
        Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
    }

    Token *PossibleStartOfSpecifiers = m_CurrentToken;

    // property or accessors can have more restrictive access flags.
    //
    ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInConstruct);

    if ((IsPropertyContext &&
            m_CurrentToken->m_TokenType != tkGET &&
            m_CurrentToken->m_TokenType != tkSET) ||
        (!IsPropertyContext &&
            m_CurrentToken->m_TokenType != tkADDHANDLER &&
            m_CurrentToken->m_TokenType != tkREMOVEHANDLER &&
            m_CurrentToken->m_TokenType != tkRAISEEVENT))
    {
        bool CreatedStatement = false;

        if (m_InterpretationUnderIntelliSense && (Attributes || Specifiers))
        {
            // For IntelliSense to work for attributes and access specifiers in Get/Set, an valid statement
            // needs to be returned (rather than NULL), so that IntelliSense has something
            // to work with.

            ParseTree::Statement *Stmt = new(m_TreeStorage) ParseTree::VariableDeclarationStatement;
            Stmt->Opcode = ParseTree::Statement::VariableDeclaration;

            SetStmtBegLocation(Stmt, StmtStart);

            Stmt->AsVariableDeclaration()->Attributes = Attributes;
            Stmt->AsVariableDeclaration()->Specifiers = Specifiers;

            SetStmtEndLocation(Stmt, m_CurrentToken);

            LinkStatement(Stmt);

            CreatedStatement = true;
        }

        if (BeginsDeclaration(StmtStart))
        {
            // The current statement is really a module level
            // declaration. Treat the property as concluded.
            //
            // ISSUE: This may behave strangely if the current context
            // is a #Region nested within the property.
            ReportSyntaxError(
                IsPropertyContext ?
                    ERRID_InvInsideEndsProperty :
                    ERRID_InvInsideEndsEvent,
                StmtStart,
                m_CurrentToken,
                ErrorInConstruct);

            PopContext(false, StmtStart);

            // Popping the context has no effect if the context was the original one
            // provided for a line parse. In such cases, redoing the parse will
            // produce an infinite recursion.

            if (RealContext()->Opcode ==
                    (IsPropertyContext ? ParseTree::Statement::Property : ParseTree::Statement::BlockEventDeclaration))
            {
                ResyncAt(0);
            }
            else
            {
                // Redo the parse as a regular module-level statement.
                m_CurrentToken = StmtStart;
                ParseDeclarationStatement(ErrorInConstruct);
            }
        }
        else if (CreatedStatement)
        {
            ReportSyntaxError(
                IsPropertyContext ?
                    ERRID_PropertyMemberSyntax :
                    ERRID_EventMemberSyntax,
                StmtStart,
                m_CurrentToken,
                ErrorInConstruct);
        }
        else
        {
            LinkStatement(
                ReportUnrecognizedStatementError(
                    IsPropertyContext ?
                        ERRID_PropertyMemberSyntax :
                        ERRID_EventMemberSyntax,
                    ErrorInConstruct));
        }

        return;
    }

    // Specifiers only allowed for property accessors, not for event
    // methods.
    if (!IsPropertyContext && Specifiers)
    {
        // Specifiers are not valid on 'AddHandler', 'RemoveHandler' and 'RaiseEvent' methods.

        ReportSyntaxErrorWithoutMarkingStatementBad(
            ERRID_SpecifiersInvOnEventMethod,
            PossibleStartOfSpecifiers,
            m_CurrentToken,
            ErrorInConstruct);
    }

    if (!m_FirstStatementOnLine)
    {
        ReportSyntaxErrorWithoutMarkingStatementBad(ERRID_MethodMustBeFirstStatementOnLine, m_CurrentToken, ErrorInConstruct);
    }

    ParseTree::MethodDefinitionStatement *Procedure = NULL;
    tokens ProcType;

    ProcType = m_CurrentToken->m_TokenType;

    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    ParseTree::ParameterList *Params = NULL;

    Procedure = new(m_TreeStorage) ParseTree::MethodDefinitionStatement;
    Procedure->Attributes = Attributes;
    Procedure->Specifiers = Specifiers;

    if (IsPropertyContext)
    {
        Procedure->Opcode = ProcType == tkGET ? ParseTree::Statement::PropertyGet : ParseTree::Statement::PropertySet;
    }
    else
    {
        Procedure->Opcode =
            ProcType == tkADDHANDLER ?
                ParseTree::Statement::AddHandlerDeclaration :
                ProcType == tkREMOVEHANDLER ?
                    ParseTree::Statement::RemoveHandlerDeclaration :
                    ParseTree::Statement::RaiseEventDeclaration;
    }

    SetPunctuator(&Procedure->MethodKind, StmtStart, m_CurrentToken);
    GetNextToken();

    if(ProcType != tkGET)
    {
#if 0        
        //In the case of non-get accessors, we want to eat a new line after the accessor name
        //if it is followed by parens, which means that it may have arguments.
        EatNewLineIfFollowedBy(tkLParen);
#endif
    }

    RejectGenericParametersForMemberDecl(ErrorInConstruct);

    if (ProcType != tkGET &&
        m_CurrentToken->m_TokenType == tkLParen)
    {
        Procedure->Parameters = ParseParameters(ErrorInConstruct, LeftParen, RightParen, Procedure);

        SetPunctuator(&Procedure->LeftParen, StmtStart, LeftParen);
        SetPunctuator(&Procedure->RightParen, StmtStart, RightParen);
    }

    SetStmtBegLocation(Procedure, StmtStart);
    SetStmtEndLocation(Procedure, m_CurrentToken);

    if (!IsValidStatementTerminator(m_CurrentToken))
    {
        Token *ErrStart = m_CurrentToken;
        ResyncAt(0);
        ReportSyntaxError(
            ERRID_Syntax,
            ErrStart,
            m_CurrentToken,
            ErrorInConstruct);
    }

    LinkStatement(Procedure);

    // Deal with the body of the method.
    if (m_IsDeclAndMethodBodiesParse)
    {
        m_ParsingMethodBody = true;

        ParseTree::MethodBodyStatement *MethodBody = new(m_TreeStorage) ParseTree::MethodBodyStatement;
        switch (ProcType)
        {
            case tkGET:
                MethodBody->Opcode = ParseTree::Statement::PropertyGetBody;
                m_ExpectedExitOpcode = ParseTree::Statement::ExitProperty;
                break;
            case tkSET:
                MethodBody->Opcode = ParseTree::Statement::PropertySetBody;
                m_ExpectedExitOpcode = ParseTree::Statement::ExitProperty;
                break;
            case tkADDHANDLER:
                MethodBody->Opcode = ParseTree::Statement::AddHandlerBody;
                m_ExpectedExitOpcode = ParseTree::Statement::SyntaxError;
                break;
            case tkREMOVEHANDLER:
                MethodBody->Opcode = ParseTree::Statement::RemoveHandlerBody;
                m_ExpectedExitOpcode = ParseTree::Statement::SyntaxError;
                break;
            case tkRAISEEVENT:
                MethodBody->Opcode = ParseTree::Statement::RaiseEventBody;
                m_ExpectedExitOpcode = ParseTree::Statement::SyntaxError;
                break;
            default:
                VSFAIL("Surprising method definition.");
                break;
        }
        SetStmtBegLocation(MethodBody, m_CurrentToken);

        MethodBody->SetParent(m_Context);
        EstablishContext(MethodBody);

        // Parse the body
        while (!m_CurrentToken->IsEndOfParse())
        {
            ParseLine(&Parser::ParseStatementInMethodBody);

            // Get the next line
            GetTokensForNextLine(true);
        }

        m_ParsingMethodBody = false;

        SetStmtEndLocation(MethodBody, m_CurrentToken);

        Procedure->EntireMethodLocation.m_lEndLine = m_CurrentToken->m_StartLine;
        Procedure->EntireMethodLocation.m_oEnd = m_CurrentToken->m_StartCharacterPosition;

        Procedure->Body = MethodBody;
    }
    else
    {
        bool ProcEndsNormally = FindEndProc(ProcType, StmtStart, Procedure, ErrorInConstruct);

        // If the body did not end normally, the token stream is presently pointed at the
        // start of a statement presumed to start a new module-level declaration.
        if (!ProcEndsNormally)
        {
            ParseDeclarationStatement(ErrorInConstruct);
        }
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseName
*
* Purpose: Will parse a dot qualified or unqualified name
*
*          Ex: class1.proc
*
**********************************************************************/
ParseTree::Name *
Parser::ParseName
(
    bool RequireQualification,
    _Inout_ bool &ErrorInConstruct,
    bool AllowGlobalNameSpace,
    bool AllowGenericArguments,
    bool DisallowGenericArgumentsOnLastQualifiedName,
    bool AllowEmptyGenericArguments,
    _Out_opt_ bool *AllowedEmptyGenericArguments,
    bool AllowGlobalOnly
)
{
    VSASSERT(AllowGenericArguments || !AllowEmptyGenericArguments, "Inconsistentcy in generic arguments parsing requirements!!!");
    VSASSERT(!RequireQualification || !AllowGlobalOnly, "Inconsistency in qualification and global only requireements!!!");
    VSASSERT(AllowGlobalNameSpace || !AllowGlobalOnly, "Inconsistency in global namespace handling!!!");

    ParseTree::Name *Result = NULL;
    Token *Start = m_CurrentToken;
    Token *Dot = NULL;
    bool AllowNonEmptyGenericArguments = true;
    bool KeepParsing = true;

    if (Start->m_TokenType == tkGLOBAL)
    {
        if (!AllowGlobalNameSpace)
        {
            // Report the error and turn into a bad simple name in order to let compilation continue.
            ReportSyntaxError(ERRID_NoGlobalExpectedIdentifier, m_CurrentToken, ErrorInConstruct);
            Result = new(m_TreeStorage) ParseTree::SimpleName;
            Result->AsSimple()->ID = CreateId(m_CurrentToken);
            Result->AsSimple()->ID.IsBad = true;
        }
        else
        {
            Result = new(m_TreeStorage) ParseTree::Name;
            Result->Opcode = ParseTree::Name::GlobalNameSpace;
        }

        SetLocation(&Result->TextSpan, Start, m_CurrentToken);
        GetNextToken();

        if (m_CurrentToken->m_TokenType == tkDot)
        {
            Dot = m_CurrentToken;
            GetNextToken();
            EatNewLine();
        }
        else if (AllowGlobalOnly) 
        {
            KeepParsing = false;
        }
        else
        {
            ReportSyntaxError(ERRID_ExpectedDot, m_CurrentToken, ErrorInConstruct);
        }
    }

    while (KeepParsing)
    {
        // Allow keyword as Identifer only after qualifier, but not as the leading name
        //
        ParseTree::IdentifierDescriptor Name =
            Result ?
                ParseIdentifierAllowingKeyword(ErrorInConstruct) :
                ParseIdentifier(ErrorInConstruct);

        bool GenericArgumentsExist = false;

        if (AllowGenericArguments)
        {
            // Test for a generic type name.

            if (BeginsGenericWithPossiblyMissingOf(m_CurrentToken))
            {
                VSASSERT(m_CurrentToken->m_TokenType == tkLParen, "Generic parameter parsing lost!!!");

                if (!m_InterpretationUnderIntelliSense &&
                    Result &&
                    DisallowGenericArgumentsOnLastQualifiedName &&
                    AreGenericsArgumentsOnLastName(m_CurrentToken))
                {
                    // Note that we don't want to do this when parsing for intellisense
                    // because in that case we are dealing with partial names that are
                    // still being typed and the interface name also can at some point
                    // be the last qualified name and we do not want to rule out intellisense
                    // for such scenarios.
                    //
                    ReportGenericArgumentsDisallowedError(
                        ERRID_TypeArgsUnexpected,
                        ErrorInConstruct);
                }
                else
                {
                    GenericArgumentsExist = true;
                }
            }
        }

        if (Result)
        {
            ParseTree::QualifiedName *Qualified;

            if (GenericArgumentsExist)
            {
                Qualified = new(m_TreeStorage) ParseTree::QualifiedWithArgumentsName;
                Qualified->Opcode = ParseTree::Name::QualifiedWithArguments;

                ParseGenericArguments(
                    Start,
                    Qualified->AsQualifiedWithArguments()->Arguments,
                    AllowEmptyGenericArguments,
                    AllowNonEmptyGenericArguments,
                    ErrorInConstruct);
            }
            else
            {
                Qualified = new(m_TreeStorage) ParseTree::QualifiedName;
                Qualified->Opcode = ParseTree::Name::Qualified;
            }

            Qualified->Base = Result;
            Qualified->Qualifier = Name;
            SetPunctuator(&Qualified->Dot, Start, Dot);

            Result = Qualified;
        }
        else
        {
            ParseTree::SimpleName *Simple;

            if (GenericArgumentsExist)
            {
                Simple = new(m_TreeStorage) ParseTree::SimpleWithArgumentsName;
                Simple->Opcode = ParseTree::Name::SimpleWithArguments;

                ParseGenericArguments(
                    Start,
                    Simple->AsSimpleWithArguments()->Arguments,
                    AllowEmptyGenericArguments,
                    AllowNonEmptyGenericArguments,
                    ErrorInConstruct);
            }
            else
            {
                Simple = new(m_TreeStorage) ParseTree::SimpleName;
                Simple->Opcode = ParseTree::Name::Simple;
            }

            Simple->ID = Name;
            Result = Simple;
        }

        SetLocation(&Result->TextSpan, Start, m_CurrentToken);

        if (m_CurrentToken->m_TokenType == tkDot)
        {
            Dot = m_CurrentToken;

            GetNextToken();
            EatNewLine();
            continue;
        }
        else
        {
            break;
        }
    }

    if (RequireQualification && Dot == NULL)
    {
        ReportSyntaxError(ERRID_ExpectedDot, m_CurrentToken, ErrorInConstruct);
    }

    VSASSERT(!AllowGenericArguments || AllowEmptyGenericArguments || AllowNonEmptyGenericArguments,
                    "Generic argument parsing inconsistency!!!");

    if (AllowedEmptyGenericArguments)
    {
        *AllowedEmptyGenericArguments = (AllowNonEmptyGenericArguments == false);
    }

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::AreGenericsArgumentsOnLastName
*
* Purpose:
*     Determines whether Start beings the type arguments for either
*     a simple name of the last name in a qualified name.
*
*     The state of the parse must be restored to the state before function
*     entry.
*
**********************************************************************/
bool
Parser::AreGenericsArgumentsOnLastName
(
    _In_ Token *Start            // [in] start token of generic arguments
)
{
    VSASSERT(BeginsGeneric(Start) || Start->m_TokenType == tkLParen,
                "start of generic arguments expected !!!");

    Scanner::LineMarker LineMark;
    bool Error;
    bool Abandoned;
    bool Tracking;
    Token *CurPrev;

    // we are trying to determine if the generic arguments specified are
    // on the last name of a possible qualified name. There ie useful to
    // know for member level "implements" clauses because type arguments
    // are disallowed on the method name (i.e. the last name).
    //
    // Trying parsing the generic arguments and then check if a tkDOT is
    // present. If tkDOT is present, then this is not the last name else
    // this is the last name.
    //
    // Restore the parse state to the state it was at function entry.

    Abandoned = DisableAbandonLines();
    LineMark = m_InputStream->MarkLine();
    Error = DisableErrors();
    Tracking = EnableErrorTracking();
    CurPrev = m_CurrentToken;
    Token * pLastToken = m_InputStream->GetLastToken();

#if DEBUG
    long CurrentStatementStartLine   = m_CurrentStatementStartLine;
    long CurrentStatementStartColumn = m_CurrentStatementStartColumn;
#endif

    m_CurrentToken = Start;

    CacheStmtLocation();

    bool ErrorInConstruct = false;

    Token *Of = NULL;
    Token *LeftParen = NULL;
    Token *RightParen = NULL;
    bool AllowEmptyGenericArguments = true;
    bool AllowNonEmptyGenericArguments = true;

    ParseGenericArguments(
        Of,
        LeftParen,
        RightParen,
        AllowEmptyGenericArguments,
        AllowNonEmptyGenericArguments,
        ErrorInConstruct);

    bool GenericsArgumentsOnLastName = (m_CurrentToken->m_TokenType != tkDot);

    // Reset to original state.
    m_InputStream->ResetToLine(LineMark);
    EnableErrors(Error);
    DisableErrorTracking(Tracking);
    EnableAbandonLines(Abandoned);
    m_CurrentToken = CurPrev;

    if (pLastToken->IsEndOfLine() && pLastToken->m_EOL.m_NextLineAlreadyScanned && pLastToken->m_Next)
    {
        //The position of the scanner, upon entry to the function, is at the point in the stream
        //just after what produced pLastToken. If we end up linking a token in after
        //pLastToken, then we would have consumed text past the line mark we saved when
        //we entered the function. If we do that, then we will potentally have EOL tokens with their
        //"next line already scanned" flag set to true. Because we restored the stream to before that point, we need to 
        //abandon any tokens that were created.
        m_InputStream->AbandonTokens(pLastToken->m_Next);

        pLastToken->m_EOL.m_NextLineAlreadyScanned = false;
    }

#if DEBUG
    m_CurrentStatementStartLine = CurrentStatementStartLine;
    m_CurrentStatementStartColumn = CurrentStatementStartColumn;
#endif

    return GenericsArgumentsOnLastName;
}


/*********************************************************************
*
* Function:
*     Parser::ParseImplementsList
*
**********************************************************************/
ParseTree::NameList *
Parser::ParseImplementsList
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::NameList *ImplementsList = NULL;
    ParseTree::NameList **ListTarget = &ImplementsList;

    VSASSERT(
        m_CurrentToken->m_TokenType == tkIMPLEMENTS,
        "Implements list parsing lost.");

    
    bool first = true;
    do
    {
        GetNextToken();
        EatNewLineIfNotFirst(first);


        Token *NameStart = m_CurrentToken;

        ParseTree::Name *Term =
            ParseName(
                true,
                ErrorInConstruct,
                true,   // AllowGlobalNameSpace
                true,   // Allow generic arguments
                true);  // Disallow generic arguments on last qualified name i.e. on the method name

        if (ErrorInConstruct)
        {
            ResyncAt(1, tkComma);
        }
       
        CreateListElement(
            ListTarget,
            Term,
            NameStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    FixupListEndLocations(ImplementsList);

    return ImplementsList;
}

ParseTree:: NameList *
Parser::ParseHandlesList
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::NameList *HandlesList = NULL;
    ParseTree::NameList **ListTarget = &HandlesList;

    VSASSERT( m_CurrentToken->m_TokenType == tkHANDLES, "Handles list parsing lost.");

    do
    {
        Token *PotentialComma = m_CurrentToken;
        GetNextToken(); // get off the handles / comma token
        if ( PotentialComma->m_TokenType == tkComma )
        {
            EatNewLine(); // allow new line after the comma Dev10#430142
        }

        Token *NameStart = m_CurrentToken;
        ParseTree::Name *Term = NULL;

        if (m_CurrentToken->m_TokenType == tkMYBASE ||
            m_CurrentToken->m_TokenType == tkMYCLASS ||
            m_CurrentToken->m_TokenType == tkME)
        {
            ParseTree::Name *Base = new(m_TreeStorage) ParseTree::SimpleName;
            Base->Opcode = ParseTree::Name::Simple;

            Base->AsSimple()->ID = ParseIdentifierAllowingKeyword(ErrorInConstruct);

            SetLocation(&Base->TextSpan, NameStart, m_CurrentToken);

            Token *Dot = m_CurrentToken;
            VerifyExpectedToken(tkDot, ErrorInConstruct);

            // VSW#153956
            // If there is no dot token, the term is the simple name
            if (Dot->m_TokenType == tkDot)
            {
                EatNewLine(); // allow implicit line continuation after '.' in handles list - dev10_503311

                ParseTree::QualifiedName *Qualified = new(m_TreeStorage) ParseTree::QualifiedName;
                Qualified->Opcode = ParseTree::Name::Qualified;

                Qualified->Base = Base;
                Qualified->Qualifier = ParseIdentifierAllowingKeyword(ErrorInConstruct);

                SetLocationAndPunctuator(
                    &Qualified->TextSpan,
                    &Qualified->Dot,
                    NameStart,
                    m_CurrentToken,
                    Dot);

                Term = Qualified;
            }
            else
            {
                Term = Base;
            }
        }
        else
        {
            if (m_CurrentToken->m_TokenType == tkGLOBAL)
            {
                // A handles name can't start with Global, it is local.
                // Produce the error, ignore the token and let the name parse for sync.
                ReportSyntaxError(ERRID_NoGlobalInHandles, m_CurrentToken, ErrorInConstruct);
                Term = new(m_TreeStorage) ParseTree::SimpleName;
                Term->AsSimple()->ID = CreateId(m_CurrentToken);
                Term->AsSimple()->ID.IsBad = true;
                ErrorInConstruct = true;
            }
            else
            {
                Term = ParseName(true, ErrorInConstruct, true /* AllowGlobalNameSpace */, true);
            }

        }

        if (ErrorInConstruct)
        {
            ResyncAt(1, tkComma);
        }

        CreateListElement(
            ListTarget,
            Term,
            NameStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    FixupListEndLocations(HandlesList);

    return HandlesList;
}

ParseTree::GenericParameterList *
Parser::ParseGenericParameters
(
    _Out_ Token *&Of,
    _Out_ Token *&LeftParen,
    _Out_ Token *&RightParen,
    _Inout_ bool &ErrorInConstruct
)
{
    LeftParen = NULL;
    
    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        LeftParen = m_CurrentToken;
        GetNextToken();
        EatNewLine();
    }

    VSASSERT(m_CurrentToken->m_TokenType == tkOF, "Generic parameter parsing lost.");

    Of = m_CurrentToken;

    ParseTree::GenericParameterList *GenericParameters = NULL;
    ParseTree::GenericParameterList **Target = &GenericParameters;
    ParseTree::GenericParameterList ** LastTarget = NULL;
    Token *As = NULL;

    bool first = true;
    do
    {
        GetNextToken(); // Consume Comma
        // the first time around the token we consume is OF rather than ',' Thus the sort of wierd EatNewLineIfNotFirst()
        EatNewLineIfNotFirst(first); 
        Token *ParameterStart = m_CurrentToken;

        // (Of In T) or (Of Out T) or just (Of T). If the current token is "Out" or "In"
        // then we have to consume it and get the next token...
        ParseTree::GenericParameter::Variance_Kind Variance;
        Token *VarianceLocation = NULL;
        switch (TokenAsKeyword(m_CurrentToken))
        {
            case tkOUT:
                Variance = ParseTree::GenericParameter::Variance_Out;
                VarianceLocation = m_CurrentToken;
                AssertLanguageFeature(FEATUREID_CoContraVariance, VarianceLocation);
                GetNextToken();
                EatNewLineIfFollowedBy(tkRParen, false /* not a query op */); // dev10_503122 Allow EOL before ')'
                break;
            case tkIN:
                Variance = ParseTree::GenericParameter::Variance_In;
                VarianceLocation = m_CurrentToken;
                AssertLanguageFeature(FEATUREID_CoContraVariance, VarianceLocation);
                GetNextToken();
                break;
            default:
                Variance = ParseTree::GenericParameter::Variance_None;
                break;
        }

        ParseTree::IdentifierDescriptor Name;

        // ... unless the next token is ) or , or As -- which indicate that the "Out" we just consumed
        // should have been taken as the identifier instead.
        if (Variance==ParseTree::GenericParameter::Variance_Out &&
            (m_CurrentToken->m_TokenType==tkRParen || m_CurrentToken->m_TokenType==tkComma || m_CurrentToken->m_TokenType==tkAS))
        {
            Variance = ParseTree::GenericParameter::Variance_None;
            m_CurrentToken = VarianceLocation;
            VarianceLocation = NULL;
        }

        Name = ParseIdentifier(ErrorInConstruct);
        
        As = NULL;
        Token *LeftBrace = NULL;
        Token *RightBrace = NULL;

        ParseTree::ConstraintList *Constraints = NULL;

        if (m_CurrentToken->m_TokenType == tkAS)
        {
            As = m_CurrentToken;

            ParseTree::ConstraintList **ConstraintTarget = &Constraints;
            ParseTree::ConstraintList **LastConstraintTarget;
            GetNextToken();
#if 0           
            EatNewLine();
#endif

            if (m_CurrentToken->m_TokenType == tkLBrace)
            {
                LeftBrace = m_CurrentToken;
                GetNextToken();
                EatNewLine();
            }

            do
            {
                Token *ConstraintStart = m_CurrentToken;
                ParseTree::Constraint *Constraint = NULL;

                if (ConstraintStart->m_TokenType == tkNEW)
                {
                    // New constraint

                    Constraint = new(m_TreeStorage) ParseTree::Constraint;
                    Constraint->Opcode = ParseTree::Constraint::New;
                    GetNextToken();
                }
                else if (ConstraintStart->m_TokenType == tkCLASS)
                {
                    // Class constraint

                    Constraint = new(m_TreeStorage) ParseTree::Constraint;
                    Constraint->Opcode = ParseTree::Constraint::Class;
                    GetNextToken();
                }
                else if (ConstraintStart->m_TokenType == tkSTRUCTURE)
                {
                    // Struct constraint

                    Constraint = new(m_TreeStorage) ParseTree::Constraint;
                    Constraint->Opcode = ParseTree::Constraint::Struct;
                    GetNextToken();
                }
                else
                {
                    if (!CanTokenStartTypeName(ConstraintStart))
                    {
                        ReportSyntaxError(
                            ERRID_BadConstraintSyntax,
                            ConstraintStart,
                            ErrorInConstruct);

                        // Continue parsing as a type constraint
                    }

                    // Type constraint

                    ParseTree::TypeConstraint *TypeConstraint = new(m_TreeStorage) ParseTree::TypeConstraint;

                    TypeConstraint->Opcode = ParseTree::Constraint::Type;
                    TypeConstraint->Type = ParseGeneralType(ErrorInConstruct);

                    Constraint = TypeConstraint;
                }

                SetLocation(&Constraint->TextSpan, ConstraintStart, m_CurrentToken);

                if (ErrorInConstruct)
                {
                    ResyncAt(3, tkComma, tkRBrace, tkRParen);
                }

                LastConstraintTarget = ConstraintTarget;
                
                CreateListElement(
                    ConstraintTarget,
                    Constraint,
                    ConstraintStart,
                    m_CurrentToken,
                    (LeftBrace && m_CurrentToken->m_TokenType == tkComma) ?
                        m_CurrentToken :
                        NULL);

                // continue parsing for multiple constraints only if {} are specified.
                //
                if (!LeftBrace || m_CurrentToken->m_TokenType != tkComma)
                {
                    break;
                }

                GetNextToken();
                EatNewLine();
            } while (true);

            FixupListEndLocations(Constraints);

            if (LeftBrace)
            {
                EatNewLineIfFollowedBy(tkRBrace, *LastConstraintTarget);
                if (m_CurrentToken->m_TokenType == tkRBrace)
                {
                    RightBrace  = m_CurrentToken;
                }

                VerifyExpectedToken
                (
                    tkRBrace, 
                    ErrorInConstruct
                );
            }
        }

        ParseTree::GenericParameter *Parameter = new(m_TreeStorage) ParseTree::GenericParameter;
        Parameter->Name = Name;
        Parameter->Constraints = Constraints;
        Parameter->Variance = Variance;
        SetLocationAndPunctuator(
            &Parameter->TextSpan,
            &Parameter->As,
            ParameterStart,
            m_CurrentToken,
            As);

        SetPunctuator(&Parameter->VarianceLocation, ParameterStart, VarianceLocation);
        SetPunctuator(&Parameter->LeftBrace, ParameterStart, LeftBrace);
        SetPunctuator(&Parameter->RightBrace, ParameterStart, RightBrace);

        LastTarget = Target;
        
        CreateListElement(
            Target,
            Parameter,
            ParameterStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    FixupListEndLocations(GenericParameters);

    if (LeftParen)
    {
        EatNewLineIfFollowedBy(tkRParen, *LastTarget);
        RightParen = m_CurrentToken;

        if (m_CurrentToken->m_TokenType == tkRParen)
        {
            GetNextToken();
        }
        else
        {
            ReportSyntaxError(
                As == NULL ?
                    ERRID_TypeParamMissingAsCommaOrRParen :
                    ERRID_TypeParamMissingCommaOrRParen,
                m_CurrentToken,
                ErrorInConstruct);
        }
    }

    return GenericParameters;
}

/*********************************************************************
*
* Function:
*     Parser::ParseSubDeclaration
*
* Purpose:
*
**********************************************************************/
ParseTree::MethodDeclarationStatement *
Parser::ParseSubDeclaration
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,     // [in] specifiers on definition
    _In_ Token *Start,        // [in] token starting definition
    bool IsDelegate,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Sub = m_CurrentToken;

    VSASSERT(Sub->m_TokenType == tkSUB, "must be at a Sub.");

    GetNextToken();
#if 0    
    EatNewLine();
#endif

    ParseTree::IdentifierDescriptor Ident;
    ParseTree::Statement::Opcodes Opcode;

    // tkNEW is allowed as a Sub name but no other keywords.
    if (m_CurrentToken->m_TokenType == tkNEW)
    {
        Ident = ParseIdentifierAllowingKeyword(ErrorInConstruct);
        Opcode = ParseTree::Statement::ConstructorDeclaration;

        // Not allowed Async constructors
        ParseTree::SpecifierList *asyncSpecifier = Specifiers->HasSpecifier(ParseTree::Specifier::Async);
        if (asyncSpecifier != NULL)
        {
            ReportSyntaxError(ERRID_ConstructorAsync, &asyncSpecifier->TextSpan, ErrorInConstruct);
        }
    }
    else
    {
        Ident = ParseIdentifier(ErrorInConstruct);
        Opcode = ParseTree::Statement::ProcedureDeclaration;
    }

    if (ErrorInConstruct)
    {
        ResyncAt(2, tkLParen, tkOF);
    }

    ParseTree::GenericParameterList *GenericParams = NULL;
    Token *Of = NULL;
    Token *GenericLeftParen = NULL;
    Token *GenericRightParen = NULL;

    // Dev10_504604 we are parsing a method declaration and will need to let the scanner know that we
    // are so the scanner can correctly identify attributes vs. xml while scanning the declaration.
    VSASSERT( m_ForceMethodDeclLineState == false, "m_ForceMethodDeclLineState not designed for nested calls.  Use a counter instead of a boolean if you need nesting");
    BackupValue<bool> backupForceMethodDeclLineState(&m_ForceMethodDeclLineState); // will restore value of m_ForceMethodDeclLineState when we return
    m_ForceMethodDeclLineState = true; 
    if (BeginsGeneric(m_CurrentToken))
    {
        if (Opcode == ParseTree::Statement::ConstructorDeclaration)
        {
            // We want to do this error checking here during parsing and not in
            // declared (which would have been more ideal) because for the invalid
            // case, when this error occurs, we don't want any parse errors for
            // parameters to show up.

            // We want other errors such as those on regular parameters reported too,
            // so don't mark ErrorInConstruct, but instead use a temp.
            //
            RejectGenericParametersForMemberDecl(ErrorInConstruct);
        }
        else
        {
            GenericParams = ParseGenericParameters(Of, GenericLeftParen, GenericRightParen, ErrorInConstruct);
        }
    }

    ParseTree::ParameterList *Params = NULL;

    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        Params = ParseParameters(ErrorInConstruct, LeftParen, RightParen);
    }

    // See if we have the HANDLES or the IMPLEMENTS clause on this procedure.
    ParseTree::NameList *Handles = NULL;
    ParseTree::NameList *Implements = NULL;
    Token *HandlesOrImplements = NULL;

    if (m_CurrentToken->m_TokenType == tkHANDLES)
    {
        HandlesOrImplements = m_CurrentToken;
        Handles = ParseHandlesList(ErrorInConstruct);
    }
    else if (m_CurrentToken->m_TokenType == tkIMPLEMENTS)
    {
        HandlesOrImplements = m_CurrentToken;
        Implements = ParseImplementsList(ErrorInConstruct);
    }

    // We should be at the end of the statement.
    if (!IsValidStatementTerminator(m_CurrentToken))
    {
        Token *ErrStart = m_CurrentToken;

        ResyncAt(0);

        ReportSyntaxError(
            ERRID_ExpectedEOS,
            ErrStart,
            m_CurrentToken,
            ErrorInConstruct);
    }

    return 
        CreateMethodDeclaration
        (
            Opcode, 
            Attributes, 
            Ident, 
            GenericParams, 
            Params, 
            NULL, 
            NULL, 
            Specifiers, 
            Implements, 
            Handles, 
            Start, 
            m_CurrentToken, 
            Sub, 
            LeftParen, 
            RightParen, 
            NULL, 
            HandlesOrImplements, 
            Of, 
            GenericLeftParen, 
            GenericRightParen, 
            IsDelegate
        );
}

/*********************************************************************
*
* Function:
*     Parser::ParseFunctionDeclaration
*
* Purpose:
*     Parses a Function definition.
*
**********************************************************************/
ParseTree::MethodDeclarationStatement *
Parser::ParseFunctionDeclaration
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,     // [in] specifiers on definition
    _In_ Token *Start,        // [in] token starting definition
    bool IsDelegate,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Function = m_CurrentToken;

    VSASSERT(Function->m_TokenType == tkFUNCTION, "Function parsing lost.");

    // Dev10_504604 we are parsing a method declaration and will need to let the scanner know that we
    // are so the scanner can correctly identify attributes vs. xml while scanning the declaration.
    VSASSERT( m_ForceMethodDeclLineState == false, "m_ForceMethodDeclLineState not designed for nested calls.  Use a counter instead of a boolean if you need nesting");
    BackupValue<bool> backupForceMethodDeclLineState(&m_ForceMethodDeclLineState); // will restore value of m_ForceMethodDeclLineState when we return
    m_ForceMethodDeclLineState = true;

    GetNextToken();

    ParseTree::IdentifierDescriptor Ident;

    if (m_CurrentToken->m_TokenType == tkNEW)
    {
        // "New" gets special attention because attempting to declare a constructor
        // as a function is, we expect, a common error.

        ReportSyntaxError(ERRID_ConstructorFunction, m_CurrentToken, ErrorInConstruct);

        Ident = ParseIdentifierAllowingKeyword(ErrorInConstruct);
    }
    else
    {
        Ident = ParseIdentifier(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(2, tkLParen, tkAS);
        }
    }

    ParseTree::GenericParameterList *GenericParams = NULL;
    Token *Of = NULL;
    Token *GenericLeftParen = NULL;
    Token *GenericRightParen = NULL;
    if (BeginsGeneric(m_CurrentToken))
    { 
        GenericParams = ParseGenericParameters(Of, GenericLeftParen, GenericRightParen, ErrorInConstruct);
    }

    ParseTree::ParameterList *Params = NULL;

    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        Params = ParseParameters(ErrorInConstruct, LeftParen, RightParen);
    }

    ParseTree::Type *ReturnType = NULL;
    ParseTree::AttributeSpecifierList *ReturnTypeAttributes = NULL;

    Token *As = NULL;

    // Check the return type.
    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;
        GetNextToken();
#if 0    
        EatNewLine();
#endif

        if (m_CurrentToken->m_TokenType == tkLT)
        {
            ReturnTypeAttributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
        }

        ReturnType = ParseGeneralType(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }
    }

    // See if we have the HANDLES or the IMPLEMENTS clause on this procedure.
    ParseTree::NameList *Handles = NULL;
    ParseTree::NameList *Implements = NULL;
    Token *HandlesOrImplements = NULL;
    if (m_CurrentToken->m_TokenType == tkHANDLES)
    {
        HandlesOrImplements = m_CurrentToken;
        Handles = ParseHandlesList(ErrorInConstruct);
    }
    else if (m_CurrentToken->m_TokenType == tkIMPLEMENTS)
    {
        HandlesOrImplements = m_CurrentToken;
        Implements = ParseImplementsList(ErrorInConstruct);
    }

    return
        CreateMethodDeclaration(
            ParseTree::Statement::FunctionDeclaration,
            Attributes,
            Ident,
            GenericParams,
            Params,
            ReturnType,
            ReturnTypeAttributes,
            Specifiers,
            Implements,
            Handles,
            Start,
            m_CurrentToken,
            Function,
            LeftParen,
            RightParen,
            As,
            HandlesOrImplements,
            Of,
            GenericLeftParen,
            GenericRightParen,
            IsDelegate);
}

bool
IsOverloadableOperatorToken
(
    tokens T
)
{
    switch (T)
    {
        case tkCTYPE:
        case tkISTRUE:
        case tkISFALSE:
        case tkNOT:
        case tkPlus:
        case tkMinus:
        case tkMult:
        case tkDiv:
        case tkPwr:
        case tkIDiv:
        case tkConcat:
        case tkShiftLeft:
        case tkShiftRight:
        case tkMOD:
        case tkOR:
        case tkXOR:
        case tkAND:
        case tkLIKE:
        case tkEQ:
        case tkNE:
        case tkLT:
        case tkLE:
        case tkGE:
        case tkGT:
            return true;
        default:
            return false;
    }
}

bool
IsOperatorToken
(
    tokens T
)
{
    switch (T)
    {
        case tkAND:
        case tkANDALSO:
        case tkCBOOL:
        case tkCBYTE:
        case tkCCHAR:
        case tkCDATE:
        case tkCDEC:
        case tkCDBL:
        case tkCINT:
        case tkCLNG:
        case tkCOBJ:
        case tkCSBYTE:
        case tkCSHORT:
        case tkCSNG:
        case tkCSTR:
        case tkCTYPE:
        case tkCUINT:
        case tkCULNG:
        case tkCUSHORT:
        case tkDIRECTCAST:
        case tkGETTYPE:
        case tkIS:
        case tkISFALSE:
        case tkISNOT:
        case tkISTRUE:
        case tkLIKE:
        case tkMOD:
        case tkNEW:
        case tkNOT:
        case tkOR:
        case tkORELSE:
        case tkTRYCAST:
        case tkTYPEOF:
        case tkXOR:
        case tkPlus:
        case tkMinus:
        case tkMult:
        case tkDiv:
        case tkPwr:
        case tkIDiv:
        case tkConcat:
        case tkShiftLeft:
        case tkShiftRight:
        case tkEQ:
        case tkNE:
        case tkLT:
        case tkLE:
        case tkGE:
        case tkGT:
        case tkBang:
        case tkDot:
        case tkConcatEQ:
        case tkMultEQ:
        case tkPlusEQ:
        case tkMinusEQ:
        case tkDivEQ:
        case tkIDivEQ:
        case tkPwrEQ:
        case tkShiftLeftEQ:
        case tkShiftRightEQ:
            return true;
        default:
            return false;
    }
}


/*********************************************************************
*
* Function:
*     Parser::ParseOperatorDeclaration
*
* Purpose:
*     Parses an Operator definition.
*
**********************************************************************/
ParseTree::MethodDeclarationStatement *
Parser::ParseOperatorDeclaration
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,     // [in] specifiers on definition
    _In_ Token *Start,        // [in] token starting definition
    bool IsDelegate,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *OperatorToken = m_CurrentToken;
    VSASSERT(OperatorToken->m_TokenType == tkOPERATOR, "Operator parsing lost.");

    // Dev10_504604 we are parsing a method declaration and will need to let the scanner know that we
    // are so the scanner can correctly identify attributes vs. xml while scanning the declaration.
    VSASSERT( m_ForceMethodDeclLineState == false, "m_ForceMethodDeclLineState not designed for nested calls.  Use a counter instead of a boolean if you need nesting");
    BackupValue<bool> backupForceMethodDeclLineState(&m_ForceMethodDeclLineState); // will restore value of m_ForceMethodDeclLineState when we return
    m_ForceMethodDeclLineState = true;

    GetNextToken();

    ParseTree::Specifier *HangingSpecifier = NULL;

#if IDE
    // Under the IDE, we accept the Widening or Narrowing specifier coming after the Operator keyword.
    //
    // Example:  Public Shared Operator Widening CType( ...
    //
    // This is still a syntax error, but we won't mark the statement bad so that the pretty lister
    // can move the specifier to before the Operator keyword.
    // The OperatorDefinition parse tree keeps track of this "hanging" specifier.

    if (m_CurrentToken->m_TokenType == tkWIDENING || m_CurrentToken->m_TokenType == tkNARROWING)
    {
        ParseTree::Specifier::Specifiers CurSpecifier =
            m_CurrentToken->m_TokenType == tkWIDENING ?
                ParseTree::Specifier::Widening :
                ParseTree::Specifier::Narrowing;

        HangingSpecifier = new(m_TreeStorage) ParseTree::Specifier;
        HangingSpecifier->Opcode = CurSpecifier;
        SetLocation(&HangingSpecifier->TextSpan, m_CurrentToken, m_CurrentToken->m_Next);

        ReportSyntaxErrorWithoutMarkingStatementBad(
            ERRID_UnknownOperator,
            m_CurrentToken,
            ErrorInConstruct);

        GetNextToken();
    }
#endif

    ParseTree::IdentifierDescriptor Ident = {0};
    tokens OperatorTokenType = tkNone;
    tokens CurrentTokenType = TokenAsKeyword(m_CurrentToken);

    if (IsOverloadableOperatorToken(CurrentTokenType))
    {
        OperatorTokenType = CurrentTokenType;

        Ident.Name = m_pStringPool->TokenToString(OperatorTokenType);
        Ident.TypeCharacter = chType_NONE;
        Ident.IsBracketed = false;
        Ident.IsNullable = false;
        Ident.IsBad = false;

        SetLocation(&Ident.TextSpan, m_CurrentToken, m_CurrentToken);
        GetNextToken();
    }
    else
    {
        if (IsOperatorToken(CurrentTokenType))
        {
            ReportSyntaxError(ERRID_OperatorNotOverloadable, m_CurrentToken, ErrorInConstruct);
        }
        else
        {
            ReportSyntaxError(ERRID_UnknownOperator, m_CurrentToken, ErrorInConstruct);
        }

        Ident.IsBad = true;
        SetLocation(&Ident.TextSpan, m_CurrentToken, m_CurrentToken);
        ResyncAt(1, tkLParen);
    }

    ParseTree::ParameterList *Params = NULL;

    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    if (m_CurrentToken->m_TokenType != tkLParen)
    {
        HandleUnexpectedToken(tkLParen, ErrorInConstruct);
        ResyncAt(2, tkLParen, tkAS);
    }

    RejectGenericParametersForMemberDecl(ErrorInConstruct);

    
    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        Params = ParseParameters(ErrorInConstruct, LeftParen, RightParen);
    }

    ParseTree::Type *ReturnType = NULL;
    ParseTree::AttributeSpecifierList *ReturnTypeAttributes = NULL;

    Token *As = NULL;

    // Check the return type.
    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;
        GetNextToken();
#if 0        
        EatNewLine();
#endif

        if (m_CurrentToken->m_TokenType == tkLT)
        {
            ReturnTypeAttributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
        }

        ReturnType = ParseGeneralType(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }
    }

    // HANDLES and IMPLEMENTS clauses are not allowed on Operator declarations.
    if (m_CurrentToken->m_TokenType == tkHANDLES)
    {
        ReportSyntaxError(ERRID_InvalidHandles, m_CurrentToken, ErrorInConstruct);
    }
    else if (m_CurrentToken->m_TokenType == tkIMPLEMENTS)
    {
        ReportSyntaxError(ERRID_InvalidImplements, m_CurrentToken, ErrorInConstruct);
    }

    ParseTree::GenericParameterList *GenericParams = NULL;
    Token *Of = NULL;
    Token *GenericLeftParen = NULL;
    Token *GenericRightParen = NULL;

    ParseTree::MethodDeclarationStatement *Procedure =
        CreateMethodDeclaration(
            ParseTree::Statement::OperatorDeclaration,
            Attributes,
            Ident,
            GenericParams,
            Params,
            ReturnType,
            ReturnTypeAttributes,
            Specifiers,
            NULL,
            NULL,
            Start,
            m_CurrentToken,
            OperatorToken,
            LeftParen,
            RightParen,
            As,
            NULL,
            Of,
            GenericLeftParen,
            GenericRightParen,
            IsDelegate);

    Procedure->AsOperatorDefinition()->OperatorTokenType = OperatorTokenType;
    Procedure->AsOperatorDefinition()->HangingSpecifier = HangingSpecifier;

    return Procedure;
}

/*****************************************************************************************
;ParsePropertyDefinition

Parses a property definition.  This will deal with both regular properties and 
auto-properties.  There are interesting challenges here to be aware of.  The biggest
problem is that the syntax for auto-properties requires potentially massive lookahead
to figure out if the property is auto or regular.  There are some clues up front as to
whether you are looking at a regular property (they have readonly/writeonly specifiers, 
for instance) but often you have to go find the get/set/end property to know.  That requires
look ahead parsing that has side effects as you can encounter #if, 'comments, and <attributes>
along the way.  Parsing those things throws statements onto the context block but since
we don't know at the time if we have an auto or regular property, the context isn't set up
yet.  So we have to do some evil stuff and move the statements to the property context when
we finally create one if it turns out that we are looking at a regular property instead of
an auto property.

I've tried to keep lookahead to a minimum as implicit line continuation is another thorn here.
We really need to use the parser to look ahead because it understand line continuation in all
the many places we may encounter it before getting to the get/set/end property statements.
Parameters can have implicit line continuation as can the property type, and the property
initializer, etc.  So we parse as far as we can before doing speculative parsing.  But doing
so requires that we haul along enough information that we discover along the way so that when
we finally do know what kind of property tree to build, we can build it.
******************************************************************************************/
ParseTree::PropertyStatement * // the property tree for the auto/regular property we are on now
Parser::ParsePropertyDefinition
(
    ParseTree::AttributeSpecifierList *pAttributes, // [in] attributes that preceded the property definition
    ParseTree::SpecifierList *pSpecifiers, // [in] specifiers on the property definition
    _In_ Token *pStart, // [in] token starting definition (should be tkPROPERTY)
    _Inout_ bool &errorInConstruct, // [out] whether we encounter errors trying to parse the property
    bool propertyDefinedInInterface // [in] whether the property is defined within the context of an interface 
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkPROPERTY, "Property definition parsing lost.");

    /* The token ring gets reused whenever the parser calls abandonLines, so tokens have a short shelf life 
       But we need to hang on to tokens across lines as we parse the property to set location for various
       punctuators, etc.  So we need to lock the token ring so our tokens don't go away until we are done */
    BackupValue<bool> backupKeepLines(&m_KeepLines); // will restore value of m_KeepLines when we return
    m_KeepLines = true; 

    Token *pPropertyPunctuatorEnd = m_CurrentToken; 
    GetNextToken(); // get off PROPERTY
    
    // ====== Check for the obsolete style (Property Get, Property Set, Property Let)- not allowed any longer.
    if (m_CurrentToken->m_TokenType == tkGET ||
        m_CurrentToken->m_TokenType == tkSET ||
        m_CurrentToken->m_TokenType == tkLET)
    {
        ReportSyntaxError(ERRID_ObsoletePropertyGetLetSet, m_CurrentToken, errorInConstruct);
        GetNextToken();
    }

    // ===== Parse the property name

    Token *pIdentifierStart = m_CurrentToken;
    ParseTree::IdentifierDescriptor propertyName = ParseIdentifier(errorInConstruct);
    Token *pIdentifierEnd = m_CurrentToken;

    RejectGenericParametersForMemberDecl(errorInConstruct);

    // ===== Parse the Property parameters, e.g. Property bob(x as integer, y as integer)

    Token *pLeftParen = NULL; // Track where this is so we can set the punctuators when we build the tree
    Token *pRightParen = NULL; // Track where this is so we can set the punctuators when we build the tree
    ParseTree::ParameterList *pPropertyParameters = NULL;

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        pPropertyParameters = ParseParameters(errorInConstruct, pLeftParen, pRightParen);
    }

    // If we ---- up on the parameters try to resume on the AS, =, or Implements
    if (errorInConstruct)
    {
        ResyncAt(3, tkAS, tkIMPLEMENTS, tkEQ);
    }

    // ===== Parse the property's type (e.g. Property Foo(params) AS type )

    // A bunch of stuff we'll need in order to build the property tree once we know what kind we have

    Token * pNewKeyword = NULL, * pFromKeyword = NULL, * pAsKeyword = NULL, * pWithKeyword = NULL,
          * pImplementsKeyword = NULL; // Track whether we saw NEW, FROM, AS, WITH, IMPLEMENTS
    long deferredInitializationTextStart = -1; // location of the text we need to copy for the deferred initializer (-1 if none)
    ParseTree::ParenthesizedArgumentList AsTypeConstructorArguments; // if we have As New Type(args) this captures the ctor args
    ParseTree::BracedInitializerList * pCollectionInitializer = NULL; // if we have a collection initializer {1,2,3} this captures it
    ParseTree::ObjectInitializerList * pObjectInitializerList = NULL; // if we have an object initializer (with {.foo=1}) this captures it
    ParseTree::ParenthesizedArgumentList PropertyTypeConstructorArguments; // The property type may have ctor args, e.g. Property No as Respect("Zilch") <-- ctor arg
    ParseTree::NameList * pImplementsList = NULL; // in case the property implements an interface
    ParseTree::AttributeSpecifierList *pPropertyTypeAttributes = NULL; // The type of the property may have attributes, e.g. Property foo as <obsolete> bar
    ParseTree::Type *pPropertyType = NULL; // given Property Foo() as Bar, this is Bar

    // ===== Parse AS [NEW] <attributes> TYPE[(ctor args)] [ObjectCreationExpressionInitializer]

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        ParsePropertyType( errorInConstruct,
            &pAsKeyword, &pNewKeyword, &pFromKeyword, &pWithKeyword,
            &pPropertyTypeAttributes, &pPropertyType,
            deferredInitializationTextStart, 
            PropertyTypeConstructorArguments,
            &pCollectionInitializer,
            &pObjectInitializerList);        
    }

    // ==== Parse '=' <expression>  e.g. Property Foo() as Integer = 42 or Property Foo() = 42, etc.

    Token *pEquals = NULL; // if we see an '=' we track it here
    ParseTree::Initializer *pInitialValue = NULL;

    if (m_CurrentToken->m_TokenType == tkEQ && pNewKeyword == NULL )
    {
        pEquals = m_CurrentToken;
        GetNextToken(); // get off '='

        EatNewLine();  
        pInitialValue = ParseDeferredInitializer(errorInConstruct);

        if (errorInConstruct)
        {
            ResyncAt(1, tkIMPLEMENTS);
        }
    }

    // Parse the IMPLEMENTS statement if any.  Note that the Implements statement
    // must be on the same line as the Property definition statement.  In cases of
    // implicit line continuation, it must be on the same logical line as the Property
    // definition, e.g. following the initializer or the property type

    if (m_CurrentToken->m_TokenType == tkIMPLEMENTS)
    {
        pImplementsKeyword = m_CurrentToken;
        pImplementsList = ParseImplementsList(errorInConstruct);
    }

    // ===== Make an initial ---- at determining if this might be an auto or expanded property
    // We can determine which it is in some cases by looking at the specifiers and/or 
    // whether it is defined in an interface. At this point there isn't enough information to
    // know whether it is an autoproperty or not just based on the presence of parameters.
    bool mustBeAnExpandedProperty = propertyDefinedInInterface ||
            ( pSpecifiers && 
                (pSpecifiers->HasSpecifier(ParseTree::Specifier::MustOverride) ||
                 pSpecifiers->HasSpecifier(ParseTree::Specifier::ReadOnly) ||
                 pSpecifiers->HasSpecifier(ParseTree::Specifier::WriteOnly) ||
                 pSpecifiers->HasSpecifier(ParseTree::Specifier::Default)
                ) 
            );

    if ( !mustBeAnExpandedProperty ) 
    {
        /* If we are on ':' then we don't have to do lookahead parsing because we now can make a decision based on the parameters:
           - If the property has parameters, then it must be an expanded property because auto-properties can't have params.
           - If there aren't parameters, then it must be an autoproperty since we are on ':', e.g. this is an auto property:
            
            Property foo() as foo(of bar) : msgbox("AutoProperty dude") <-- the ':' on the same logical line as the definition means
            we are looking at an autoproperty because it doesn't take params */
        if ( m_CurrentToken->m_TokenType == tkColon )
        {
            mustBeAnExpandedProperty = pPropertyParameters != NULL; // if it has parameters we consider it an expanded property 
        }
        else 
        {
            mustBeAnExpandedProperty = DoLookAheadToSeeIfPropertyIsExpanded();
        }
    } // if ( !mustBeAnExpandedProperty ) 

    // Build the tree for the property and do some simple semantics like making sure a regular property doesn't have an initializer, etc.
    return BuildPropertyTree(
        mustBeAnExpandedProperty,
        pStart,
        pIdentifierStart, pIdentifierEnd, pLeftParen, pRightParen,
        pEquals, pAsKeyword, pNewKeyword, pWithKeyword, pFromKeyword,
        pImplementsKeyword,
        pPropertyPunctuatorEnd,
        deferredInitializationTextStart,
        pAttributes, pSpecifiers, propertyName, pPropertyParameters, 
        pPropertyType, pPropertyTypeAttributes, PropertyTypeConstructorArguments, 
        pInitialValue, pCollectionInitializer, pObjectInitializerList,
        pImplementsList,
        errorInConstruct
    );
}

/*****************************************************************************************
;DoLookAheadToSeeIfPropertyIsExpanded

Spins up another parser to do the lookahead between the property definition, e.g.
Property foo() ...
and the get/set/end property. 

At this point we have made it through the Property definition, including initializer and implements list.
     
But we haven't been able to tell from the clues so far whether we have an auto-property or not.
So we need to do some speculative parsing to see whether we have an auto or regular property.
Before we see the GET/SET/End Property there may be #if, 'Comments, and Attributes that precede the get/set
We need to work our way through those to determine if we are an auto-property or not.  Sadly, doing look-ahead
parsing will modify our parser state (the current context could morph, statements will get added to the current context 
which isn't the right one yet because we haven't established the Property declaration as the context because we don't 
know which kind of property declaration to build yet, errors can be logged which we don't want because we are just doing
speculative parsing, etc.  We tried backing up parser state and doing it here but that approach never worked out.
So instead I'll crank up a seperate Parser to do the lookahead parsing--which we can party on however we want without 
messing up our current parser state 
******************************************************************************************/
bool // true: Property is expanded / false: property is an auto-property
Parser::DoLookAheadToSeeIfPropertyIsExpanded()
{
    NorlsAllocator AllocatorForLookAhead(NORLSLOC); // put state here while we do lookahead - we will throw it away when this function returns.
    // Note: this ErrorTable doesn't have a compiler project associated with it.
    // Its errors will not be reported and adding errors to it will not cause
    // a task-list update. We are doing look-ahead parsing so we don't want the errors, anyway.
    ErrorTable ThrowAwayErrors(m_Compiler, NULL, NULL);
    Parser LookAheadParser( 
        &AllocatorForLookAhead,
        m_Compiler,
        m_CompilerHost,
        m_IsXMLDocEnabled,
        m_CompilingLanguageVersion,
        m_InterpretationUnderDebugger,
        m_InterpretationUnderIntelliSense,
        m_InitializingFields );

    // This scanner hack is necessary because if we start the scanner out on an EOL, the scanner will ignore it when asked
    // for the next line.  Which will honk up the entry to DoesPropertyHaveExpandedPropertyMembers() which jumps to the end of the line
    // and we will end up skipping a line.  So we back it up a token if we are on EOL.  I know, I shed tears too.
    Token *pStartScanningFromHere = m_CurrentToken->m_TokenType != tkEOL ? m_CurrentToken : m_CurrentToken->m_Prev;

    const WCHAR *wszEndOfStream = m_InputStream->GetInputStreamEnd(); // We don't know the length of the stream so we'll grab it from the current scanner
    const WCHAR *wszWhereWeAreNow = m_InputStream->TokenToStreamPosition(pStartScanningFromHere); // We need to pick up in the temp scanner
    // where we are right now.  Build a scanner that we can use to scan forward with so that won't mess up the state of our current scanner 
    // since we want to pick up parsing from where we are now, not from wherever look-ahead parsing may take us.
    Scanner tokenStream(m_Compiler->GetStringPool(), 
                      wszWhereWeAreNow, 
                      wszEndOfStream, // we don't know the length and the buffer isn't zero terminated so we'll use the existing known end.
                      // Provide initial line information to our temporary scanner so it looks like we are picking up where we
                      // are leaving off to do speculative parsing.  We need to do this for #if conditional statements, e.g.:
                      // given #if FOO we use the line number of the #if statement to see whether the FOO variable was defined
                      // before or after the #if.  So we need to provide the correct line number to the scanner, given where we
                      // are in the current text, so that it can provide correct line number information when semantics tries to
                      // decide whether FOO is in scope or not (it must come before the #if)
                      // The other line info doesn't matter as we are just speculatively parsing.
                      0, pStartScanningFromHere->GetLocation().m_lEndLine, 0 );

    // Take a snapshot of the current Conditional Compilation state so we can pass it to the look-ahead parser in the event it encounters conditionals.
    Symbols *pSymbolAllocatorForConditionals = new( AllocatorForLookAhead )Symbols( m_Compiler, &AllocatorForLookAhead, NULL /* no line marker table */ );
    // We create a scratch conditionals container because we don't want to hork up the current conditionals container with #const symbols we
    // encounter while doing look-ahead parsing.
    BCSYM_CCContainer *pConditionals = pSymbolAllocatorForConditionals->AllocCCContainer(false /*no location*/, 
        m_ConditionalConstantsScope ? m_ConditionalConstantsScope->GetCompilerFile() : NULL );
    pSymbolAllocatorForConditionals->GetConditionalCompilationScope(pConditionals); // This creates a hash table of default size

    if ( m_ConditionalConstantsScope ) // we don't always have a scope.  Depends on who is calling the parser
    {
        // link the scratch conditional constant scope to the current one so we can see #Const symbols that have been defined 
        // so far, and also the project conditional constants.
        Symbols::AddSymbolToHash( m_ConditionalConstantsScope->GetHash(), pConditionals, true /* set the parent*/,
            false /* container isn't a module */,false /* container isn't a namespace */);
    }

    /* We need to pass the current state of the conditional stack because we may have a preceding #if and we'll have to deal with the #else, e.g.
       #if true
       Property AutoProp() as integer = 3
       #else   <-- we need the current context so we know what to do when doing lookahead for the autoprop above
       Property AutoProp() as integer = 4
       #endif  */
    ConditionalCompilationStack ConditionalsStack; 
    ConditionalsStack.Init( &AllocatorForLookAhead );
    m_Conditionals.ShallowCopyInto( &ConditionalsStack );

    bool errorInConstruct = false;
    bool IsExpandedProperty =  LookAheadParser.DoesPropertyHaveExpandedPropertyMembers(
        tokenStream, // where to start doing lookahead parsing from
        ThrowAwayErrors, // we don't care if there are errors so put them in this wastebasket 
        pConditionals, // we need to carry over our #Const definitions we've encountered so far in case there are #if statements in the property definition
        pSymbolAllocatorForConditionals, // if we encounter #Const doing look-ahead we need to use the same allocator as was used to produce symbols in m_ConditionalConstantsScope
        ConditionalsStack,
        errorInConstruct );

    if ( m_ConditionalConstantsScope )
    {
        BCSYM_NamedRoot *pRemoved = Symbols::RemoveSymbolFromHash( m_ConditionalConstantsScope->GetHash(), pConditionals);
        VSASSERT( pRemoved, "Why didn't we remove the hash?" );
    }

    return IsExpandedProperty;
}

/*****************************************************************************************
;ParsePropertyType

Given Property foo() AS TYPE, this parses the TYPE, including any initializer
that is present for the type.  
******************************************************************************************/
void Parser::ParsePropertyType(
    _Inout_ bool &errorInConstruct, // [out] whether we encounter an error parsing the type and initializer
    _Inout_ Token ** ppAsKeyword, // [out] the token for the AS keyword for setting punctuator info later.  NULL if not encountered.
    _Inout_ Token ** ppNewKeyword, // [out] the token for the NEW keyword for setting punctuator info later.  NULL if not encountered.
    _Inout_ Token ** ppFromKeyword, // [out] the token for the FROM keyword for setting punctuator info later.  NULL if not encountered.
    _Inout_ Token ** ppWithKeyword, // [out] the token for the WITH keyword for setting punctuator info later.  NULL if not encountered.
    _Inout_ ParseTree::AttributeSpecifierList **ppPropertyTypeAttributes, // [out] attributes discovered on the type.  NULL if not encountered.
    _Inout_ ParseTree::Type **ppPropertyType, // [out] the type of the property, e.g. Property foo as bar NULL if not encountered.
    _Inout_ long &deferredInitializationTextStart, // [out] the line offset of the initialization expression.  -1 if not encountered.
    _Inout_ ParseTree::ParenthesizedArgumentList &PropertyTypeConstructorArguments, // [out] If the property type has a constructor, these are the args
    _Inout_ ParseTree::BracedInitializerList ** ppCollectionInitializer,// [out] holds the type initialization list, if we encounter one, e.g. {1,2,3}
    _Inout_ ParseTree::ObjectInitializerList ** ppObjectInitializerList // [out] holds the object initializer list, if we encounter one, e.g. With {.a = 1}
)
{
    VSASSERT( m_CurrentToken->m_TokenType == tkAS, "ParsePropertyType off on the wrong foot" );
    ThrowIfFalse( ppAsKeyword && ppNewKeyword && ppFromKeyword && ppWithKeyword &&
                  ppPropertyTypeAttributes && ppPropertyType && ppCollectionInitializer &&
                  ppObjectInitializerList); // fail fast if our caller didn't give us these 

    deferredInitializationTextStart = -1; // in case we don't see this.
    *ppAsKeyword = m_CurrentToken;
    GetNextToken(); // Get off AS

    // AS NEW
    if (m_CurrentToken->m_TokenType == tkNEW )
    {
        *ppNewKeyword = m_CurrentToken;
        GetNextToken(); // get off NEW

        // Are there attributes before the type of the property?
        if (m_CurrentToken->m_TokenType == tkLT)
        {
            *ppPropertyTypeAttributes = ParseAttributeSpecifier( NotFileAttribute, errorInConstruct );
        }

        // Parse the Property type
        *ppPropertyType = ParseTypeName( errorInConstruct );

        // Does the type have contructor arguments, e.g. AS NEW Type(ctor args)?  
        if (m_CurrentToken->m_TokenType == tkLParen)
        {
            // The arg list must be preserved as text between declaration semantics and method body semantics.
            deferredInitializationTextStart = m_CurrentToken->m_StartCharacterPosition;
            PropertyTypeConstructorArguments = ParseParenthesizedArguments(errorInConstruct);
        }

        EatNewLineIfFollowedBy(tkFROM); // Dev10_509577

        // Is the AS NEW TYPE[(ctor args)] followed by a FROM initializer?
        if ( TokenAsKeyword(m_CurrentToken) == tkFROM)
        {
            *ppFromKeyword = m_CurrentToken;
            GetNextToken(); // Get off FROM
            AssertLanguageFeature(FEATUREID_CollectionInitializers, *ppFromKeyword);
            
            EatNewLine(); // allow implicit line continuation after FROM (dev10_508839)

            if (deferredInitializationTextStart < 0)
            {
                deferredInitializationTextStart = (*ppFromKeyword)->m_StartCharacterPosition;
            }
            
            *ppCollectionInitializer = ParseInitializerList( errorInConstruct, 
                                                            true,  // enable expression initializers
                                                            false ); // disable statements.
        }
        
        // WITH initializer?  e.g. type AS NEW TYPE[(ctor args)] WITH 
        if ( m_CurrentToken->m_TokenType == tkWITH )
        {
            *ppWithKeyword = m_CurrentToken;
            if (*ppFromKeyword)
            {
                ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, errorInConstruct);
            }
            else
            {
                if (-1 == deferredInitializationTextStart)
                {
                    deferredInitializationTextStart = m_CurrentToken->m_StartCharacterPosition;
                }

                *ppObjectInitializerList = ParseObjectInitializerList(errorInConstruct);

                if (TokenAsKeyword(m_CurrentToken) == tkFROM)
                {
                    ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, errorInConstruct);
                }
            }
        }
    }
    else // AS [attributes] TYPE (rather than As NEW [initializer], which is handled above)
    {
        // Attributes in front of the Property type?
        if (m_CurrentToken->m_TokenType == tkLT)
        {
            *ppPropertyTypeAttributes = ParseAttributeSpecifier(NotFileAttribute, errorInConstruct);
        }

        *ppPropertyType = ParseGeneralType(errorInConstruct);
    }

    if (errorInConstruct)
    {
        ResyncAt(2, tkIMPLEMENTS, tkEQ);
    }
}

/*****************************************************************************************
;BuildPropertyTree

Builds the appropriate parse tree for either a regular property or auto-property.
No parsing happens here - we just manufacture the correct tree and set the location info
related punctuators (like New, As, From, etc. etc.)

<Rationalization>
It seems like I'm trying to set a world record for number of parameters to a function.
The deal here is that this is what it takes to build the tree, and this is really a
factorization of some code that I don't think will become a general utility function.
</Rationalization>
******************************************************************************************/
ParseTree::PropertyStatement * // returns the built property tree
Parser::BuildPropertyTree(
    bool mustBeAnExpandedProperty, // tells us whether we are making an expanded property or an autoproperty
    _In_ Token *pStart, // Provides location info for the PROPERTY keyword used to define this property, e.g. ->property foo
    _In_ Token *pIdentifierStart, // location info for the start of the property name, e.g. property ->foo
    _In_ Token *pIdentifierEnd, // location info for the end of the property name, e.g. property foo<-
    _In_ Token *pLeftParen, // location info for the left paren if one exists, e.g. property foo(<-
    _In_ Token *pRightParen, // location info for the right paren if one exists, e.g. property foo()<-
    _In_ Token *pEquals, // location info for the equals if one exists, e.g. property foo() =<- 
    _In_ Token *pAsKeyword, // location info for the AS keyword if one exists, e.g. property foo() as<-
    _In_ Token *pNewKeyword, // location info for the NEW keyword if one exists, e.g. property foo() as new<-
    _In_ Token *pWithKeyword, // location info for the WITH keyword if one exists, e.g. property NewProperty3 As New Foo With<-
    _In_ Token *pFromKeyword, // location info for the FROM keyword if one exists, e.g. property x As List(Of Car) = From<- 
    _In_ Token *pImplementsKeyword, // location info for the IMPLEMENTS keyword if one exists, e.g. Properyt foo() implements<- 
    _In_ Token *pPropertyPunctuatorEnd, // location of the end of the PROPERTY keyword, e.g. property<- 
    long deferredInitializationTextStart, // location of the text we need to copy for the intializer, e.g. AS NEW Type(ctor args) this points at the lparen
    _In_ ParseTree::AttributeSpecifierList * pAttributes, // attribute for the property, e.g. <obsolete> property foo
    _In_ ParseTree::SpecifierList *pSpecifiers, // specifiers for the property, e.g. Private property foo
    _In_ ParseTree::IdentifierDescriptor &propertyName, // name of the property, e.g. property foo<--
    _In_ ParseTree::ParameterList *pPropertyParameters, // parameters to the property, e.g. property(x as integer)
    _In_ ParseTree::Type *pPropertyType, // type of the property, e.g. property foo() as integer<-
    _In_ ParseTree::AttributeSpecifierList *pPropertyTypeAttributes, // attribute on the property type, e.g. property foo() as <obsolete>Win31
    _In_ ParseTree::ParenthesizedArgumentList &PropertyTypeConstructorArguments, // ctor arguments on the property type, e.g. property foo() as Type("We types are tired of being stereotyped"<-
    _In_ ParseTree::Initializer *pInitialValue, // the initial value of the property if present, e.g. property MagicNumber = 42<-
    _In_ ParseTree::BracedInitializerList * pCollectionInitializer, // collection initializer for the type if present, e.g. property foo as type = {"Dont","forget","to","use","SAL"}
    _In_ ParseTree::ObjectInitializerList * pObjectInitializerList, // object initializer for the type if present, e.g. Public Property NewProperty3 As New Foo With {.a = 1}<-
    _In_ ParseTree::NameList * pImplementsList, // what this property implements, e.g. property Miss----() Implements World.Peace
    _Inout_ bool &errorInConstruct // [out] whether we have problems with this tree, e.g we have an autoproperty that has params, for instance.
)
{
    ThrowIfNull( pStart && pIdentifierStart && pIdentifierEnd && pPropertyPunctuatorEnd ); // fail fast if the mandatory values are not provided

    // Now we know what kind of property we have, build up the correct parse trees
    ParseTree::PropertyStatement *pPropertyStatement;
    if ( !mustBeAnExpandedProperty ) 
    {
        AssertLanguageFeature(FEATUREID_AutoProperties, pIdentifierStart);

        pPropertyStatement = new(m_TreeStorage) ParseTree::AutoPropertyStatement;
        pPropertyStatement->Opcode = ParseTree::Statement::AutoProperty;
        pPropertyStatement->AsAutoProperty()->pAutoPropertyDeclaration = NULL;

        ParseTree::AutoPropertyInitialization *pInitialization = NULL;
        if ( pNewKeyword )
        {
            pInitialization = new(m_TreeStorage) ParseTree::NewAutoPropertyDeclaration;
            pInitialization->Opcode = ParseTree::AutoPropertyInitialization::WithNew;
            SetLocation(&pInitialization->TextSpan, pIdentifierStart, m_CurrentToken);
            pInitialization->AsNew()->Arguments = PropertyTypeConstructorArguments;
            SetPunctuator(&pInitialization->AsNew()->New, pStart, pNewKeyword);

            if ( pWithKeyword )
            {
                if ( pFromKeyword )
                {
                    ReportSyntaxError(ERRID_CantCombineInitializers, pNewKeyword, pNewKeyword, errorInConstruct);
                }
                else
                {
                    if (-1 == deferredInitializationTextStart)
                    {
                        deferredInitializationTextStart = m_CurrentToken->m_StartCharacterPosition;
                    }

                    pInitialization->AsNew()->ObjectInitializer = pObjectInitializerList;

                    if (TokenAsKeyword(m_CurrentToken) == tkFROM)
                    {
                        ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, m_CurrentToken, errorInConstruct);
                    }
                }
            } // if ( pWithKeyword )

            if (!errorInConstruct && deferredInitializationTextStart > -1)
            {
                long DeferredInitializationTextEnd = m_CurrentToken->m_StartCharacterPosition;

                void *DeferredBytes = new(m_TreeStorage)
                    char[sizeof(ParseTree::DeferredText) + ((DeferredInitializationTextEnd - deferredInitializationTextStart) * sizeof(WCHAR))];

                ParseTree::DeferredText *DeferredInitializerText = new(DeferredBytes) ParseTree::DeferredText;
                DeferredInitializerText->TextLengthInCharacters = DeferredInitializationTextEnd - deferredInitializationTextStart;
                m_InputStream->CopyString(deferredInitializationTextStart, DeferredInitializationTextEnd, DeferredInitializerText->Text);

                pInitialization->AsNew()->DeferredInitializerText = DeferredInitializerText;
            }

            if (pFromKeyword)
            {
               SetPunctuator(&pInitialization->AsNew()->From, pStart, pFromKeyword);
               pInitialization->AsNew()->CollectionInitializer = pCollectionInitializer;
            }

            pPropertyStatement->AsAutoProperty()->pAutoPropertyDeclaration = pInitialization;
        } // if NEW keyword was present
        else if ( pInitialValue ) // initialized with '='?
        {
            pInitialization = new(m_TreeStorage) ParseTree::InitializerAutoPropertyDeclaration;
            pInitialization->Opcode = ParseTree::AutoPropertyInitialization::WithInitializer;
            pInitialization->AsInitializer()->InitialValue = pInitialValue;
            SetPunctuator(&pInitialization->AsInitializer()->Equals, pStart, pEquals);
            SetLocation(&pInitialization->TextSpan, pIdentifierStart, m_CurrentToken);

            pPropertyStatement->AsAutoProperty()->pAutoPropertyDeclaration = pInitialization;
        }

        // Handle error cases for autoproperties
        if ( pPropertyParameters )
        {
            ReportSyntaxError( ERRID_AutoPropertyCantHaveParams, &pPropertyParameters->TextSpan, errorInConstruct);    
        }
    }
    else // Build a regular property
    {
        pPropertyStatement = new(m_TreeStorage) ParseTree::PropertyStatement;
        pPropertyStatement->Opcode = ParseTree::Statement::Property;

        // Handle error cases for regular properties
        if ( pInitialValue )
        {
            ReportSyntaxError( ERRID_InitializedExpandedProperty, pEquals, errorInConstruct);
        }
        if ( pNewKeyword )
        {
            ReportSyntaxError( ERRID_InitializedExpandedProperty, pNewKeyword, errorInConstruct);
        }
    }

    // == Build those things that are common to both auto/regular property trees
    SetStmtBegLocation(pPropertyStatement, pStart);
    SetStmtEndLocation(pPropertyStatement, m_CurrentToken);

    pPropertyStatement->Attributes = pAttributes; // Attributes that preceded the Property Statement itself
    pPropertyStatement->Specifiers = pSpecifiers; // specifiers for the property itself (as opposed to the get/set)
    pPropertyStatement->PropertyType = pPropertyType; // property foo as bar - this is bar
    pPropertyStatement->Implements = pImplementsList; // Property foo() implements iFOO - this is the list of names we implement
    pPropertyStatement->Name = propertyName;
    pPropertyStatement->Parameters = pPropertyParameters;
    pPropertyStatement->PropertyTypeAttributes = pPropertyTypeAttributes;

    SetPunctuator(&pPropertyStatement->Property, pStart, pPropertyPunctuatorEnd);
    if ( pLeftParen )
    {
        SetPunctuator(&pPropertyStatement->LeftParen, pStart, pLeftParen);
    }
    if ( pRightParen )
    {
        SetPunctuator(&pPropertyStatement->RightParen, pStart, pRightParen);
    }
    if ( pAsKeyword )
    {
        SetPunctuator(&pPropertyStatement->As, pStart, pAsKeyword);
    }
    if ( pImplementsKeyword )
    {
        SetPunctuator(&pPropertyStatement->Impl, pStart, pImplementsKeyword);
    }

    return pPropertyStatement;
}


//////////////////////////////////////////////////////////////
//
// ;DoesPropertyHaveExpandedPropertyMembers
//
// ==============================================================================
//   !!!!!!!!!!!!!!      !!!!!!!!!!!!!!      !!!!!!!!!!!!!!      !!!!!!!!!!!!!!      
//   !!! DANGER !!!      !!! DANGER !!!      !!! DANGER !!!      !!! DANGER !!!      
//   !!!!!!!!!!!!!!      !!!!!!!!!!!!!!      !!!!!!!!!!!!!!      !!!!!!!!!!!!!!    
// ==============================================================================
// THIS FUNCTION CHANGES GLOBAL PARSER STATE!
// To call this function spin up a new parser first and call this function using
// that parser.
// You only want to call this function after spinning up a new parser instance 
// first so that it doesn't clobber the state of the parser you are currently
// using.  The look-ahead parsing this function does will add statements to the
// current context, will modify global state, will change where the scanner thinks
// we are, will change what we think the last linked statement was, etc.  So you
// definetly want to spin up a new, temporary parser, and call this function on it.
// ==============================================================================
// We perform infinite lookahead to discover whether the property is an expanded property or an auto property.
// Well, infinite in the turing machine sense of the word, anyway.  
//
// Determines this by looking for a GET, SET, or END PROPERTY token on the next logical line of code.
// The presence of any of these tokens means the property is an expanded property.
//
// This method handles these scenarios.
//
// 1. The simplest scenario is when the next line tokenized by the scanner is a line of code which does not 
//     have any attributes on it. We then look at the first token on this line to decide if the property is an auto
//     property.
//
// 2. The next line may begin with attributes. If this is the case, we must parse through all the attributes to discover
//     the first token on the line which is not describing an attribute. 
//
// 3. The next line may be a conditional compilation statement (i.e. #If). If this is the case, we must evaluate
//     the conditional compilation statement to discover the next logical line of code.
//
// 4. The next line may be a comment. If this is the case we parse the comment and look for the next logical line of code.
//
bool // TRUE - this must be an expanded property because we found get/set/params/end property : FALSE - this appears to be an autoproperty
Parser::DoesPropertyHaveExpandedPropertyMembers
(
    _In_    Scanner & InputStream, // points at the property text, immediately following the property definition
    _In_    ErrorTable & Errors, // we need a blank error table for logging errors
    _In_    BCSYM_CCContainer *pConditionalConstants, // the conditional compilation constants so we can handle #if statements
    _In_    Symbols *pSymbolAllocatorForConditionals, // if we encounter #Const during lookahead, use this allocator to build them
    _In_    ConditionalCompilationStack &ConditionalsStack, // we use this to track #if depth
    _Inout_ bool &errorInConstruct // [out] True if we hit parse errors trying to find the get/set/end property
)
{
    m_ConditionalConstantsScope = pConditionalConstants; 
    m_ConditionalCompilationSymbolsSource = pSymbolAllocatorForConditionals;
    m_Conditionals = ConditionalsStack;
    // we need a context to hang the things we parse while doing lookahead parsing onto.
    ParseTree::PropertyStatement *Result = new(m_TreeStorage) ParseTree::PropertyStatement;
    Result->Opcode = ParseTree::Statement::Property;
    EstablishContext( Result );

    // Do not report errors while doing lookahead parsing - we'll do that when we reparse the property block for real
    m_ErrorReportingDisabled = true;
    
    InitParserFromScannerStream(&InputStream, &Errors);
    m_CurrentToken = NULL;
    GetTokensForNextLine(false /* not within a method */);

    /*  On entry to this function we expect to be on the token immedately after the Property definition, including intializer, if any.
        e.g. given Property someprop3 As New List(
                                                    Of String
                                                    ) From {
                                                        "One",
                                                        "two",
                                                        } <-- WE EXPECT TO BE HERE, IMMEDIATELY FOLLOWING the '}' */
    Token *FirstTokenOnLine = NULL;
    do
    {
        // Get past broken property declarations (e.g. Property test) As Integer) as well as the End if in #End If, etc.
        GoToEndOfLine(&m_CurrentToken);

        // Eat through blank lines since it's legal to have them between the property signature and the get/set
        if ( m_CurrentToken->m_TokenType == tkEOL )
        {
            if (!m_CurrentToken->m_EOL.m_NextLineAlreadyScanned)
            {
                m_FirstTokenAfterNewline = NULL;
                m_FirstStatementOnLine = true;
                m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = true;
                
                m_CurrentToken = m_InputStream->GetNextLine();
            }
            else
            {
                m_CurrentToken = m_CurrentToken->m_Next; // We already scanned the next line, which is now attached to the end of the token ring
            }
        }
        FirstTokenOnLine = m_CurrentToken;

        // We want to parse through all the attributes that may be on the next line.
        if (m_CurrentToken->m_TokenType == tkLT)
        {     
            // Since we are not storing the attributes we parse here, errorInConstruct should not be mutated
            // due to an error in the attributes. Thus we create errorInAttributes.
            bool temp = false;
            ParseTree::AttributeSpecifierList *Attributes = ParseAttributeSpecifier(NotFileAttribute, temp);
        }      
        // Skip through conditional compilation lines, comments, and XML docs.
        else if ((m_CurrentToken->m_TokenType == tkSharp) ||
                 (m_CurrentToken->m_TokenType == tkREM) ||
                 (m_CurrentToken->m_TokenType == tkXMLDocComment))
        {
            if (m_CurrentToken->m_TokenType == tkSharp)
            {
                // Want to keep lines to ensure tokens aren't lost because of
                // ParsingConditionalCompilationLine().
                BackupValue<bool> backupKeepLines(&m_KeepLines);
                m_KeepLines = true;

                EndCommentBlockIfNeeded();
                
                // Process the conditional compilation line.
                // This adds the conditional compilation statement into the Statement List.
                ParseConditionalCompilationLine(false);
                // We will leave off on the End If (the # has already been eaten).
                // We need to get to the end of the #End If before continuing parsing.
                // Handled by the GoToEndOfLine() at the top of this DO loop.
            }
            else if ( m_CurrentToken->m_TokenType == tkREM || m_CurrentToken->m_TokenType == tkXMLDocComment )
            {
                // We do not want to mutate members that describe the current state of the parser.
                BackupValue<bool>backupErrorReportingDisabled(&m_ErrorReportingDisabled);

                // Do not report errors while parsing attributes
                m_ErrorReportingDisabled = true;

                // Parse Comment.
                ParseDeclarationStatement(errorInConstruct);

                bool errorInComment = false;
                VerifyEndOfStatement(errorInComment);
            }
            
        } // process conditional compilation
        
    // Keep going as long as we continue to encounter comments, XML docs, or conditional compilation
    } while ( m_CurrentToken->m_TokenType != tkEOF &&
              ( FirstTokenOnLine->m_TokenType == tkREM ||
                FirstTokenOnLine->m_TokenType == tkXMLDocComment ||
                FirstTokenOnLine->m_TokenType == tkSharp ||
                FirstTokenOnLine->m_TokenType == tkEOL ));

    EndCommentBlockIfNeeded();

    // Since we are not storing the specifiers we parse here, errorInConstruct should not be mutated
    // due to an error in the specifiers. Thus we create errorInSpecifiers.
    bool errorInSpecifiers = false;
    ParseSpecifiers(errorInSpecifiers);

    // If the property has a Get/Set or End Property, then it isn't an auto property
    if (m_CurrentToken->m_TokenType == tkGET ||
        m_CurrentToken->m_TokenType == tkSET || 
        (m_CurrentToken->m_TokenType == tkEND && m_CurrentToken->m_Next->m_TokenType == tkPROPERTY))
    {
        return true;         
    }

    // We are looking at an auto property
    return false;
}


// Parse a declaration of a delegate.

ParseTree::MethodDeclarationStatement *
Parser::ParseDelegateStatement
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,    // [in] proecudure specifiers
    _In_ Token *Start,            // [in] token starting statement
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkDELEGATE,
        "must be at Delegate.");

    Token *Delegate = m_CurrentToken;
    ParseTree::MethodDeclarationStatement *Result = NULL;

    GetNextToken();
#if 0    
    EatNewLine();
#endif
    
    switch (m_CurrentToken->m_TokenType)
    {
        case tkSUB:
            Result = ParseSubDeclaration(Attributes, Specifiers, Start, true, ErrorInConstruct);
            Result->Opcode = ParseTree::Statement::DelegateProcedureDeclaration;
            break;

        case tkFUNCTION:
            Result = ParseFunctionDeclaration(Attributes, Specifiers, Start, true, ErrorInConstruct);
            Result->Opcode = ParseTree::Statement::DelegateFunctionDeclaration;
            break;

        default:
        {
            // Syntax error. Issue the diagnostic, then produce a skeletal
            // delegate declaration tree.

            ReportSyntaxError(ERRID_ExpectedSubOrFunction, m_CurrentToken, ErrorInConstruct);

            ParseTree::IdentifierDescriptor Ident = ParseIdentifier(ErrorInConstruct);

            ResyncAt(1, tkLParen);

            Token *LeftParen = NULL;
            Token *RightParen = NULL;
            ParseTree::ParameterList *Params = NULL;

            if (m_CurrentToken->m_TokenType == tkLParen)
            {
                Params = ParseParameters(ErrorInConstruct, LeftParen, RightParen);
            }

            Result = 
                CreateMethodDeclaration
                (
                    ParseTree::Statement::DelegateProcedureDeclaration, 
                    NULL, 
                    Ident, 
                    NULL, 
                    Params, 
                    NULL, 
                    NULL, 
                    Specifiers, 
                    NULL, 
                    NULL, 
                    Start, 
                    m_CurrentToken, 
                    NULL, 
                    LeftParen, 
                    RightParen, 
                    NULL, 
                    NULL, 
                    NULL, 
                    NULL, 
                    NULL, 
                    true
                );

            // Force the type char to always be none in this error case because by default we
            // are setting the delegate to be a Procedure and not a Function below. Type chars
            // are not allowed on Procedures because they do not have a return type.
            // This prevents spurious errors later on in declared. See Bug VSWhidbey 183964.
            //
            Result->Name.TypeCharacter = chType_NONE;

            break;
        }
    }

    SetPunctuator(&Result->AsDelegateDeclaration()->Delegate, Start, Delegate);
    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::GetMissingEndProcError
*
* Purpose:
*
**********************************************************************/
unsigned
Parser::GetMissingEndProcError
(
    tokens ProcKind
)
{
    unsigned Errid;

    switch (ProcKind)
    {
        case tkSUB:
            Errid = ERRID_EndSubExpected;
            break;
        case tkFUNCTION:
            Errid = ERRID_EndFunctionExpected;
            break;
        case tkOPERATOR:
            Errid = ERRID_EndOperatorExpected;
            break;
        case tkGET:
            Errid = ERRID_MissingEndGet;
            break;
        case tkSET:
            Errid = ERRID_MissingEndSet;
            break;
        case tkADDHANDLER:
            Errid = ERRID_MissingEndAddHandler;
            break;
        case tkREMOVEHANDLER:
            Errid = ERRID_MissingEndRemoveHandler;
            break;
        case tkRAISEEVENT:
            Errid = ERRID_MissingEndRaiseEvent;
            break;
        default:
            VSFAIL("Surprising procedure kind.");
            Errid = ERRID_Syntax;
            break;
    }

    return Errid;
}

/*****************************************************************************************
 ;FindEndProc

 This function is used during declaration parsing to skip past the bodies of method definitions.

 Microsoft:Consider
 This function is a travesty and needs to be refactored.  In the smallest font I can read, the print-out 
 of this function spans half my office.  

 The other problem is that it is not amenable to implicit line continuation.  To really make this function
 work for implicit line continuation, we'd have to duplicate the parser logic around implicit line continuation.
 So someday this function should be gutted and replaced with a parser based evaluation of the method 
 body contents so we can correctly find the end of the method.

Return: 
    True - The end of the procedure ended with End * or EOF
    False - The end of the procedure ended with something else so we guessed where 
        it should have ended.
*******************************************************************************************/
bool Parser::FindEndProc
(
    tokens ExpectedProcedureKind, // token ending the proc (Sub, Function, Get, Set)
    _In_ Token* StartToken,      // token statement starts at (for location information)
    _In_ ParseTree::MethodDefinitionStatement *Procedure,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        Procedure->Opcode != ParseTree::Statement::DelegateProcedureDeclaration && Procedure->Opcode != ParseTree::Statement::DelegateFunctionDeclaration,
        "FindEndProc() should not be called for Delegates.");

    VSASSERT(
        !Procedure->Specifiers->HasSpecifier(ParseTree::Specifier::MustOverride) ||
        (Procedure->Opcode == ParseTree::Statement::OperatorDeclaration ||
            Procedure->Opcode == ParseTree::Statement::AddHandlerDeclaration ||
            Procedure->Opcode == ParseTree::Statement::RemoveHandlerDeclaration ||
            Procedure->Opcode == ParseTree::Statement::RaiseEventDeclaration ||
            Procedure->Opcode == ParseTree::Statement::PropertyGet ||
            Procedure->Opcode == ParseTree::Statement::PropertySet),
        "FindEndProc() should not be called for abstract methods.");

    // If there is more than one statement on the line, it is necessary to
    // eat the statement separator before collecting location info for
    // the method body: otherwise, the function body appears to begin
    // with an empty statement. However, Intellisense apparently requires
    // that the function body start on the same line as the function
    // definition. (This is why the current line is not abandoned until
    // after collecting the location info for the start of the body.)

    bool AtEndOfLine = VerifyEndOfStatement(ErrorInConstruct);

    //  Save the token location information we're interested in because
    //  our tokens will be reused and the location information lost.

    long ProcStartCharacterOffset = StartToken->m_StartCharacterPosition;
    long ProcStartLineNumber = StartToken->m_StartLine;
    long ProcStartColumnNumber = StartToken->m_StartColumn;

    long BodyStartCharacterOffset = m_CurrentToken->m_StartCharacterPosition;
    long BodyStartLineNumber = m_CurrentToken->m_StartLine;
    long BodyStartColumnNumber = m_CurrentToken->m_StartColumn;
    long BodyEndCharacterOffset;
    long BodyEndLineNumber;
    long BodyEndColumn;

    // HashOutlinableContent means that the procedure has an expression which is outlinable.
    // To optimize outlining performance we check in FindEndProc for special expressions.
    // Right now only Xml literals are outlinable but other expressions and constructs may be outlinable
    // in the future.

    Procedure->HasOutlinableContent = false;

    if (AtEndOfLine)
    {
        m_FirstStatementOnLine = true;

        m_CurrentToken =
            (m_CurrentToken->m_TokenType == tkEOL &&
             m_CurrentToken->m_EOL.m_NextLineAlreadyScanned) ?
                m_CurrentToken->m_Next :
                GetNextLine();
    }
    else
    {
        // At least for the time being, don't allow the first statement of the
        // method body to be on the same line as the declaration of the method.
        // (Allowing it upsets pretty listing.)
        ReportSyntaxError(
            ERRID_MethodBodyNotAtLineStart,
            m_CurrentToken,
            ErrorInConstruct);
        Procedure->HasSyntaxError = true;
    }

    bool Done = false;
    bool EndsWithEnd = false;
    bool EndsWithEOF = false;
    bool AtFirstStatementOnLine = true;

    ParseTree::StatementList **StaticLocalsTarget = &Procedure->StaticLocalDeclarations;

    Stack<tokens> endProcStack;
    endProcStack.Push(ExpectedProcedureKind);

    // We just want to look for End proc and skip the function body.
    while (!Done)
    {
        Token *Start = m_CurrentToken;

        BodyEndCharacterOffset = Start->m_StartCharacterPosition;
        BodyEndLineNumber = Start->m_StartLine;
        BodyEndColumn = Start->m_StartColumn;

        switch (Start->m_TokenType)
        {
            case tkEND:
            {
                CacheStmtLocation();
                Token *Next = m_CurrentToken->m_Next;
                if (Next->m_TokenType == endProcStack.Top())
                {
                    if (endProcStack.Count() == 1) 
                    {
                        Procedure->HasProperTermination = true;
                        Done = true;
                    }
                    else
                    {
                        endProcStack.Pop();
                    }
                }
                else if (((Next->m_TokenType == tkSUB ||
                           Next->m_TokenType == tkFUNCTION ||
                           Next->m_TokenType == tkOPERATOR) &&
                          (endProcStack.Top() == tkSUB ||
                           endProcStack.Top() == tkFUNCTION ||
                           endProcStack.Top() == tkOPERATOR)) ||
                         ((Next->m_TokenType == tkGET ||
                           Next->m_TokenType == tkSET) &&
                          (endProcStack.Top() == tkGET ||
                           endProcStack.Top() == tkSET)) ||
                         ((Next->m_TokenType == tkADDHANDLER ||
                           Next->m_TokenType == tkREMOVEHANDLER ||
                           Next->m_TokenType == tkRAISEEVENT) &&
                          (endProcStack.Top() == tkADDHANDLER ||
                           endProcStack.Top() == tkREMOVEHANDLER ||
                           endProcStack.Top() == tkRAISEEVENT)))
                {
                    // We will have 1 element on the endProcStack unless we encountered a multiline lambda.
                    // We should not generate errors here if the end construct should belong
                    // to a lambda because we will do that elsewhere.
                    if (endProcStack.Count() == 1)
                    {
                        // Assume end of Proc - just wrong End. Pick the correct
                        // error. Tag the End statement with the error.

                        ReportSyntaxErrorWithoutMarkingStatementBad(
                            GetMissingEndProcError(endProcStack.Top()),
                            m_CurrentToken,
                            Next->m_Next,
                            ErrorInConstruct);
                    
                        // Treat this as proper termination because the trees will
                        // be repaired to make it proper.
                        Procedure->HasProperTermination = true;
                        Done = true;
                    }
                    else
                    {
                        endProcStack.Pop();
                    }
                }
                else if (Next->m_TokenType == tkCLASS ||
                         Next->m_TokenType == tkSTRUCTURE ||
                         Next->m_TokenType == tkMODULE ||
                         Next->m_TokenType == tkNAMESPACE)
                {
                    // This is an end construct for a construct that
                    // cannot occur within a procedure. End the procedure
                    // body and reset the current token so that that
                    // end construct gets reparsed.

                    ReportSyntaxError(ERRID_InvInsideEndsProc, Start, Next->m_Next, ErrorInConstruct);
                    Done = true;
                    m_CurrentToken = Start;
                    break;
                }
                else if (Next->m_TokenType == tkEVENT &&
                         (endProcStack.Top()== tkADDHANDLER ||
                          endProcStack.Top() == tkREMOVEHANDLER ||
                          endProcStack.Top() == tkRAISEEVENT))
                {
                    ReportSyntaxError(GetMissingEndProcError(endProcStack.Top()), Start, Next->m_Next, ErrorInConstruct);
                    Done = true;
                    m_CurrentToken = Start;
                    break;
                }

                if (Done)
                {
                    if (!AtFirstStatementOnLine )
                    {
                        unsigned ErrorId;

                        switch (Next->m_TokenType)
                        {
                            case tkSUB:
                                ErrorId = ERRID_EndSubNotAtLineStart;
                                break;
                            case tkFUNCTION:
                                ErrorId = ERRID_EndFunctionNotAtLineStart;
                                break;
                            case tkOPERATOR:
                                ErrorId = ERRID_EndOperatorNotAtLineStart;
                                break;
                            case tkGET:
                                ErrorId = ERRID_EndGetNotAtLineStart;
                                break;
                            case tkSET:
                                ErrorId = ERRID_EndSetNotAtLineStart;
                                break;
                            case tkADDHANDLER:
                                ErrorId = ERRID_EndAddHandlerNotAtLineStart;
                                break;
                            case tkREMOVEHANDLER:
                                ErrorId = ERRID_EndRemoveHandlerNotAtLineStart;
                                break;
                            case tkRAISEEVENT:
                                ErrorId = ERRID_EndRaiseEventNotAtLineStart;
                                break;
                            default:
                                ErrorId = 0;
                                break;
                        }

                        if (ErrorId != 0)
                        {
                            ReportSyntaxError(
                                ErrorId,
                                m_CurrentToken,
                                ErrorInConstruct);

                            Procedure->HasSyntaxError = true;
                        }
                    }

                    // Consume the End.
                    GetNextToken();

                    // Consume the Sub/Function/Operator/Get/Set token
                    GetNextToken();

                    // Create the end construct. Use the expected procedure kind
                    // so the pretty lister will fix it up.

                    switch (endProcStack.Top())
                    {
                        case tkSUB:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndSub);
                            break;
                        }

                        case tkFUNCTION:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndFunction);
                            break;
                        }

                        case tkOPERATOR:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndOperator);
                            break;
                        }

                        case tkGET:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndGet);
                            break;
                        }

                        case tkSET:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndSet);
                            break;
                        }

                        case tkADDHANDLER:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndAddHandler);
                            break;
                        }

                        case tkREMOVEHANDLER:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndRemoveHandler);
                            break;
                        }

                        case tkRAISEEVENT:
                        {
                            Procedure->TerminatingConstruct =
                                CreateEndConstruct(Start, ParseTree::Statement::EndRaiseEvent);
                            break;
                        }

                        default:
                        {
                            VSFAIL("Unsupported end construct type.");
                            break;
                        }
                    }

                    //  Indicate alleged success...
                    EndsWithEnd = true;
                }

                break;
            }

            case tkEOF:

                if (!m_IsLineParse)
                {
                    // Encountering EOF means that there is no end to the method.
                    // Tag the stmt definition with the error.

                    ReportSyntaxError(
                        GetMissingEndProcError(endProcStack.Top()),
                        &Procedure->TextSpan,
                        ErrorInConstruct);
                }

                Done = true;

                // Although the function body does not, strictly speaking, end with an
                // end, pretend that it does so that the module declaration parser won't
                // try to parse a declaration beginning with EOF.

                EndsWithEOF = true;

                break;

            case tkGET:
            case tkSET:

                // These are problematic in that they can begin (obsolete) executable
                // statements, but also begin definitions of property methods. Recognizing
                // these as declaring property methods is necessary for end
                // construct insertion.

                if (!(endProcStack.Top() == tkGET || endProcStack.Top() == tkSET) ||
                    !(IsValidStatementTerminator(Start->m_Next) || Start->m_Next->m_TokenType == tkLParen))
                {
                    break;
                }

                __fallthrough; // Fall through.

            case tkPROPERTY:
            case tkOPERATOR:
            case tkCUSTOM:
                // Fall through
            case tkEVENT:
            case tkNAMESPACE:
            case tkCLASS:
            case tkSTRUCTURE:
            case tkENUM:
            case tkMODULE:
            case tkINTERFACE:

                ReportSyntaxError(ERRID_InvInsideEndsProc, Start, m_CurrentToken->m_Next, ErrorInConstruct);
                Done = true;
                break;

            case tkSUB:
            case tkFUNCTION:
            case tkASYNC:
            case tkITERATOR:
            {
                Token *tkToCheck = Start;
                if (Start->m_TokenType == tkASYNC || Start->m_TokenType == tkITERATOR)
                {
                    tkToCheck = (Start->m_Next != NULL) ? Start->m_Next : Start;
                }

                // If this is not true, we are assuming the function or sub represents a multiline lambda.
                bool RegularFunction = tkToCheck->m_Next && 
                    ( tkToCheck->m_Next->m_TokenType == tkID || tkToCheck->m_Next->m_TokenType == tkNEW ); // dev10_514957 - ctors are methods too ;-)
                
                if ( RegularFunction )
                {
                    ReportSyntaxError(ERRID_InvInsideEndsProc, Start, m_CurrentToken->m_Next, ErrorInConstruct);
                    Done = true;
                }
                break;
            }
            
            case tkPRIVATE:
            case tkPROTECTED:
            case tkPUBLIC:
            case tkFRIEND:
            case tkNOTOVERRIDABLE:
            case tkOVERRIDABLE:
            case tkMUSTOVERRIDE:
            case tkSTATIC:
            case tkSHARED:
            case tkSHADOWS:
            case tkWITHEVENTS:
            case tkOVERLOADS:
            case tkOVERRIDES:
            case tkREADONLY:
            case tkWRITEONLY:
            case tkWIDENING:
            case tkNARROWING:
            case tkDEFAULT:
            case tkLT:
            case tkPARTIAL:
            case tkDIM: // Dev10 #779745 Make sure locals are looked at, in case there is also a STATIC specifier.
            {
                /* May or may not end the procedure block depending on what follows.

                   Microsoft: Consider tkLT is interesting because it may be the harbinger
                   for an attribute or it may preface an XmlNCName.
                   This code really expects it to be an attribute because it tries to consume
                   it below.  But trapping for the XmlNCName and doing a break; so that we
                   can spin through the XML tokens below, results in more errors than just
                   allowing the ParseAttributeSpecifier() code to have a whack at it.
                   The problem is that once you open up xml, e.g. <xml> the scanner will just
                   keep eating tokens until it finds the concluding XML token so it will consume
                   the end sub, the end module, etc. resulting in interesting errors.  A case
                   for this kind of wierdness would be:
                   
                    dim x = 1 +
                       <obsolete>foo

                   So you can't have an attribute here anyway, but XML could be legal here.  And XML
                   is what the scanner will return, so we will encounter tkLT tkXmlNCName here
                   followed by heavens knows what depending on what follows.  But trying to trap
                   for that case here just generates more errors then not so I won't. */

                CacheStmtLocation();

                ParseTree::AttributeSpecifierList *Attributes = NULL;
                if (m_CurrentToken->m_TokenType == tkLT)
                {
                    // Look for all possible tokens that can start xml markup.  If markup is found then
                    // break and skip over the xml.
                    if (m_CurrentToken->m_Next && 
                            (m_CurrentToken->m_Next->m_TokenType == tkXmlNCName ||
                            m_CurrentToken->m_Next->m_TokenType == tkXmlBeginComment ||
                            m_CurrentToken->m_Next->m_TokenType == tkXmlBeginPI ||
                            m_CurrentToken->m_Next->m_TokenType == tkXmlBeginCData ||
                            m_CurrentToken->m_Next->m_TokenType == tkXmlBeginEmbedded || 
                                (m_CurrentToken->m_Next->m_TokenType == tkLT && 
                                m_CurrentToken->m_Next->m_Next &&
                                m_CurrentToken->m_Next->m_Next->m_TokenType == tkXmlBeginEmbedded))

                        )
                    {
                        break; // Dev10_502369 We need to spin through XML markup in the if (!Done) block below
                    }

                    bool Errors = DisableErrors();                 
                    Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
                    EnableErrors(Errors);                    
                }

                bool Errors = DisableErrors(); 
                bool ErrorInAttempt = false;             
                ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInAttempt);
                EnableErrors(Errors);

                if (!(Specifiers->HasSpecifier(
                        ParseTree::Specifier::Dim | ParseTree::Specifier::Const)) &&
                    (m_CurrentToken->m_TokenType == tkSUB ||
                     m_CurrentToken->m_TokenType == tkFUNCTION ||
                     m_CurrentToken->m_TokenType == tkOPERATOR ||
                     m_CurrentToken->m_TokenType == tkPROPERTY ||
                     BeginsEvent(m_CurrentToken)))
                {
                    ReportSyntaxErrorWithoutMarkingStatementBad(
                        ERRID_InvInsideEndsProc,
                        Start,
                        m_CurrentToken->m_Next,
                        ErrorInConstruct);

                    // Reset the error status and reparse from the start of
                    // of current line as part of the next sub or function

                    ErrorInConstruct = false;
                    m_CurrentToken = Start;

                    Done = true;
                    break;
                }

                if (Specifiers->HasSpecifier(ParseTree::Specifier::Static) && endProcStack.Count() >= 1)
                {
                    // Declarations of static locals are attached to the tree for the
                    // method definition. Bleah.
                    //
                    // Disable error reporting for these declarations because
                    // the errors will be detected in parsing the method
                    // bodies.
                    //
                    // Dev10#671584: why do we check that endProcStack.Count() >= 1?
                    // The thing is that static variables are not allowed inside lambdas (i.e. when endProcStack.Count>1)
                    //     Sub Main()
                    //        Dim x = Sub()
                    //                      Static a As Integer = 23  ' error BC36672: statics are not allowed in lambdas
                    //                End Sub
                    //     End Sub
                    // All statics have to be fields inside the containing Class. It's the job of FindEndProc to do this.
                    // Incidentally, the error BC36672 itself is reported elsewhere: FindEndProc isn't responsible for
                    // reporting the error, but only for gathering up the statics into the class.
                    //
                    // FindEndProc used to try to be smart about this, by only adding statics into the class when endProcStack.Count
                    // was EXACTLY equal to 1, i.e. we weren't inside a lambda. Trouble is that FindEndProc isn't smart enough:
                    // it's just a scanner-based thing, and can't possible recognize that
                    //     Dim x = Sub() Function()
                    //     Static a As Integer = 23
                    // is an erroneous single-line sub followed by a valid static variable declaration. It ends up believing that
                    // it has an endProcStack.Count==3 !
                    //
                    // The answer is that FindEndProc should not try to be smart. It should just register all statics
                    // into the class. If some of them were erroneous according to other places in the parser, well, that's
                    // fine.
                    // It does result in an unfortunate error-correction suggestion, however:
                    // if "static a" is declared inside a lambda, and again after the lambda, then the error correction
                    // suggests to delete the second declaration when it should really have been the first that was deleted.
                    // We could fix it by collecting statics in an entirely separate pass on the real parse-tree, after FindEndProc.
                    // However that might have perf implications.

                    bool Errors2 = DisableErrors();
                    bool ErrorInDeclaration = false;

                    ParseTree::Statement *Decl = ParseVarDeclStatement(Attributes, Specifiers, Start, ErrorInDeclaration);

                    if (Decl)
                    {
                        if (m_CurrentStatementInError)
                        {
                            m_CurrentStatementInError = false;
                            Decl->HasSyntaxError = true;
                            
                            // If we encountered an error, we need to back up a token to ensure that we don't 
                            // miss another static local declaration. Dev11 239526
                            //
                            // Static local1=
                            // Static local2
                            m_CurrentToken = BackupIfImpliedNewline(m_CurrentToken);
                        }

                        ParseTree::StatementList *ListElement = new(m_TreeStorage) ParseTree::StatementList;
                        *StaticLocalsTarget = ListElement;
                        StaticLocalsTarget = &ListElement->NextInBlock;

                        ListElement->Element = Decl;
                    }

                    EnableErrors(Errors2);
                }

                break;
            }
            case tkColon:
                if (m_CurrentToken->m_State.m_LexicalState == VB)
                    AtFirstStatementOnLine = false;
                m_CurrentToken = m_CurrentToken->m_Next;
                continue;

            case tkREM:
            case tkXMLDocComment:

                m_CurrentToken = m_CurrentToken->m_Next;
                break;

            case tkEOL:

                m_CurrentToken = 
                    Start->m_EOL.m_NextLineAlreadyScanned ?
                    Start->m_Next :
                    GetNextLine();
                AtFirstStatementOnLine = true; // Microsoft - 
                continue;

            case tkSharp:

                ParseConditionalCompilationLine(true);
                // If this skipped a section of code, the current token is NULL.
                if (m_CurrentToken == NULL)
                {
                    m_CurrentToken = GetNextLine();
                }
                continue;
        } // Switch (Start->m_TokenType )
        
        // If we didn't handle one of the special constructs above, spin through the tokens on the line
        // until we hit the end of the statement.  Take note of multiline lambdas so we can keep track of the
        // proper end construct to look for.
        if (!Done)
        {
            // Search for multiline lambda declarations; otherwise, skip to end of statement.
            while (m_CurrentToken->m_TokenType != tkEOL && 
                   m_CurrentToken->m_TokenType != tkColon &&
                  m_CurrentToken->m_TokenType != tkEOF)
            {
                Token *tkMethodKind = m_CurrentToken;
                if (tkMethodKind->m_TokenType == tkASYNC || tkMethodKind->m_TokenType == tkITERATOR)
                {
                    tkMethodKind = tkMethodKind->m_Next;
                }

                if ((tkMethodKind->m_TokenType == tkSUB || tkMethodKind->m_TokenType == tkFUNCTION) &&
                    !(tkMethodKind->m_Next && tkMethodKind->m_Next->m_TokenType == tkID) &&// if this is true, we have a method not a lambda
                    !(tkMethodKind->m_Prev->m_TokenType == tkEND ||
                        tkMethodKind->m_Prev->m_TokenType == tkDot ||
                        tkMethodKind->m_Prev->m_TokenType == tkBang))
                {
                    // resets the token stream to where we were before calling it. Microsoft: consider - take a flag to govern resetting
                    // the token stream so we don't have to re-spin through the tokens again when we don't need to.  Plus, that would take 
                    // care of the problem with line-continuation in the parameter list
                    if (IsMultiLineLambda()) 
                    {
                        // If this lambda is a multiline lambda, we want to push the Sub or Function token onto the stack so 
                        // we know what type of end construct to look for.
                        endProcStack.Push(tkMethodKind->m_TokenType);
                    }  
                }

                // If the state is anything other than VB then there is an xml literal in the procedure.
                Procedure->HasOutlinableContent |= tkMethodKind->m_State.IsXmlState();

#if IDE
                m_SeenXmlLiterals |= m_CurrentToken->m_State.IsXmlState();
#endif

                m_CurrentToken = tkMethodKind->m_Next;
            }

            if (m_CurrentToken->m_State.m_LexicalState == VB)
                AtFirstStatementOnLine = m_CurrentToken->m_TokenType != tkColon; // Microsoft - 
        }
    } // while (!Done)

    Procedure->EntireMethodLocation.m_lBegLine = ProcStartLineNumber;
    Procedure->EntireMethodLocation.m_lBegColumn = ProcStartColumnNumber;
    Procedure->EntireMethodLocation.m_oBegin = ProcStartCharacterOffset;

    Procedure->BodyLocation.m_lBegLine = BodyStartLineNumber;
    Procedure->BodyLocation.m_lBegColumn = BodyStartColumnNumber;
    Procedure->BodyLocation.m_oBegin = BodyStartCharacterOffset;

    //  If we saw an end construct then the CodeBlock ends at the token before the end construct, and the
    //  ProcBlock ends after the end construct.

    Procedure->BodyLocation.m_lEndLine = BodyEndLineNumber;
    Procedure->BodyLocation.m_lEndColumn = BodyEndColumn;
    Procedure->BodyLocation.m_oEnd = BodyEndCharacterOffset;

    if (EndsWithEnd || EndsWithEOF)
    {
        Token *TokenFollowingEnd =
            EndsWithEOF ?
                m_CurrentToken :
                BackupIfImpliedNewline(m_CurrentToken);

        Procedure->EntireMethodLocation.m_lEndLine = TokenFollowingEnd->m_StartLine;
        Procedure->EntireMethodLocation.m_lEndColumn = TokenFollowingEnd->m_StartColumn;
        Procedure->EntireMethodLocation.m_oEnd = TokenFollowingEnd->m_StartCharacterPosition;
    }
    else
    {
        Procedure->EntireMethodLocation.m_lEndLine = BodyEndLineNumber;
        Procedure->EntireMethodLocation.m_lEndColumn = BodyEndColumn;
        Procedure->EntireMethodLocation.m_oEnd = BodyEndCharacterOffset;
    }

    VSASSERT(
        (Procedure->BodyLocation.m_oBegin >  Procedure->EntireMethodLocation.m_oBegin) &&
        (Procedure->BodyLocation.m_oEnd <= Procedure->EntireMethodLocation.m_oEnd) &&
        (Procedure->BodyLocation.m_lBegLine >= Procedure->EntireMethodLocation.m_lBegLine) &&
        ((Procedure->BodyLocation.m_lBegColumn > Procedure->EntireMethodLocation.m_lBegColumn) ||
         (Procedure->BodyLocation.m_lBegLine > Procedure->EntireMethodLocation.m_lBegLine)) &&
        (Procedure->BodyLocation.m_lEndLine <= Procedure->EntireMethodLocation.m_lEndLine),
        "Expecting to find the method body inside the entire procedure");

    return EndsWithEnd || EndsWithEOF;
}

ParseTree::ParameterList *
Parser::ParseParameters
(
    _Inout_ bool &ErrorInConstruct,
    _Out_ Token *&LeftParen,
    _Out_ Token *&RightParen,
    _Inout_ ParseTree::ParseTreeNode * pParent
)
{
    ThrowIfNull(pParent);

    ParseTree::ParameterList * pRet = ParseParameters(ErrorInConstruct, LeftParen, RightParen);

    return pRet;
}


////////////////////////////////////////////////////
//
// Parser::IsMultilineLambda
//
// This method must be passed a tkSub or tkFunction token, it then peeks ahead
// through the line and matches left parens to right parens. When it finds
// the right paren that matches to the first left paren, it checks to see if the
// next token in the line is a tkEOL or a tkREM. If they are, then the lambda
// on this line is a multiline lambda, and it returns true.

// If there are no parens on the line, a mismatching number of left to right parens
// or if there are other tokens after the last right paren besides tkEOL or tkREM,
// then the lambda is assumed to be a single line lambda and this method
// returns false.
//
bool 
Parser::IsMultiLineLambda()
{
    bool FoundMultiLineLambda = false;
    Token * pStart = m_CurrentToken;
    bool restoreErrors = DisableErrors();

    if (m_CurrentToken->m_TokenType == tkASYNC || m_CurrentToken->m_TokenType == tkITERATOR)
    {
        GetNextToken();
    }

    if (m_CurrentToken->m_TokenType == tkSUB || m_CurrentToken->m_TokenType == tkFUNCTION)
    {        
        GetNextToken(); // consume the SUB token

        if (m_CurrentToken->m_TokenType == tkLParen)
        {
            bool errorInConstruct = false;
            Token * leftParen = NULL;
            Token * rightParen = NULL;

            ParseTree::ParameterList * parameters = ParseParameters(errorInConstruct, leftParen, rightParen);

            if (rightParen && rightParen->m_Next && 
                (rightParen->m_Next->IsEndOfLine() || (rightParen->m_Next->m_TokenType == tkAS)))
            {
                FoundMultiLineLambda = true;
            }
        }
        EnableErrors(restoreErrors);
    }

    // we've parsed enough to know whether we have a multi-line lambda or not, reset to the start of the lambda now to resume parsing.
    m_CurrentToken = pStart; 

    return FoundMultiLineLambda;
}

/*********************************************************************
*
* Function:
*     Parser::ParseParameters
*
* Purpose:
*     Parses a parenthesized parameter list of non-optional followed by
*     optional parameters (if any).
*
**********************************************************************/
ParseTree::ParameterList *
Parser::ParseParameters
(
    _Inout_ bool &ErrorInConstruct,
    _Out_ Token *&LeftParen,
    _Out_ Token *&RightParen
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkLParen, "Parameter list parsing confused.");

    LeftParen = m_CurrentToken;
    GetNextToken();
    EatNewLine();

    ParseTree::ParameterList *Result = NULL;
    ParseTree::ParameterList **ParamListTarget = &Result;
    ParseTree::ParameterList **LastParamListTarget = NULL;

    EatNewLineIfFollowedBy(tkRParen);

    if (m_CurrentToken->m_TokenType != tkRParen)
    {
        // Loop through the list of parameters.
        while (true)
        {
            Token *ParamStart = m_CurrentToken;

            ParseTree::AttributeSpecifierList *Attributes = NULL;
            
            if (ParamStart->m_TokenType == tkLT)
            {
                Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
            }

            ParseTree::ParameterSpecifierList *Specifiers = ParseParameterSpecifiers(ErrorInConstruct);
            if (Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))
            {
                ParseOptionalParameters(
                    Attributes,
                    Specifiers,
                    ParamStart,
                    ParamListTarget,
                    ErrorInConstruct);

                // Done with the param list. List is already connected together.
                break;
            }

            ParseTree::Parameter *Param = ParseParameter(Attributes, Specifiers, ParamStart, ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(2, tkComma, tkRParen);
            }

            LastParamListTarget = ParamListTarget;
            
            CreateListElement(
                ParamListTarget,
                Param,
                ParamStart,
                m_CurrentToken,
                m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

            if (DoneParamList(ErrorInConstruct))
            {
                break;
            }

            // If we just parsed a ParamArray parameter, we're done with the
            // parameter list.

            if (Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::ParamArray))
            {
                break;
            }
        }

        // Current token is left at either tkRParen, EOS, or the
        // beginning of the parameter after an OptionalParameters list.

        FixupListEndLocations(Result);
    }

    
    EatNewLineIfFollowedBy(tkRParen);
    
    if (m_CurrentToken->m_TokenType == tkRParen)
    {
        RightParen = m_CurrentToken;
        GetNextToken();
        
        return Result;
    }

    // Check for ParamArray as the last argument.
    ParseTree::Parameter *LastParam = GetLastListItem(Result);

    if (LastParam && LastParam->Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::ParamArray))
    {
        Token *ErrStart = m_CurrentToken;
        ResyncAt(1, tkRParen);
        ReportSyntaxError(ERRID_ParamArrayMustBeLast, ErrStart, m_CurrentToken, ErrorInConstruct);
    }
    else
    {
        // We should be at tkRParen.
        // If there are still parameters, this means that these
        // parameters should be optional. Give this error instead
        // of missing Rparen.

        while (!MustEndStatement(m_CurrentToken) &&
               m_CurrentToken->m_TokenType != tkRParen)
        {
            Token *ParamStart = m_CurrentToken;

            ParseTree::AttributeSpecifierList *Attributes = NULL;
            
            if (ParamStart->m_TokenType == tkLT)
            {
                Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
            }

            ParseTree::ParameterSpecifierList *Specifiers = ParseParameterSpecifiers(ErrorInConstruct);

            if (Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::ParamArray))
            {
                // By highlighting all the specifiers, it is guaranteed that
                // "ParamArray" will appear in the highlighted region.
                ReportSyntaxError(ERRID_ParamArrayWithOptArgs, ParamStart, m_CurrentToken, ErrorInConstruct);
            }
            else
            {
                ReportSyntaxError(ERRID_ExpectedOptional, ParamStart, ErrorInConstruct);
            }

            // If the parameter isn't marked as optional, don't call ParseOptionalParameter,
            // because we'll get an error because it doesn't have an =
            ParseTree::Parameter *Param = ParseParameter(Attributes, Specifiers, ParamStart, ErrorInConstruct);
            ResyncAt(2, tkComma, tkRParen);

            if (m_CurrentToken->m_TokenType == tkComma)
            {
                GetNextToken();
                EatNewLine();
            }
            EatNewLineIfFollowedBy(tkRParen);
        }
    }

    if (m_CurrentToken->m_TokenType == tkRParen)
    {
        RightParen = m_CurrentToken;
    }
    VerifyExpectedToken(tkRParen, ErrorInConstruct);

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseParameterSpecifiers
*
* Purpose:
*
**********************************************************************/
ParseTree::ParameterSpecifierList *
Parser::ParseParameterSpecifiers
(
    _Inout_ bool &ErrorInConstruct
)
{
    unsigned Specifiers = 0;
    bool Done = false;

    ParseTree::ParameterSpecifierList *Result = NULL;
    ParseTree::ParameterSpecifierList **ListTarget = &Result;

    while (true)
    {
        ParseTree::ParameterSpecifier::Specifiers Opcode;

        switch (m_CurrentToken->m_TokenType)
        {
            case tkBYVAL:
                if (Specifiers & ParseTree::ParameterSpecifier::ByRef)
                {
                    ReportSyntaxError(
                        ERRID_MultipleParameterSpecifiers,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                Opcode = ParseTree::ParameterSpecifier::ByVal;
                break;

            case tkBYREF:
                if (Specifiers & ParseTree::ParameterSpecifier::ByVal)
                {
                    ReportSyntaxError(
                        ERRID_MultipleParameterSpecifiers,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                else if (Specifiers & ParseTree::ParameterSpecifier::ParamArray)
                {
                    ReportSyntaxError(
                        ERRID_ParamArrayMustBeByVal,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                Opcode = ParseTree::ParameterSpecifier::ByRef;
                break;

            case tkOPTIONAL:
                if (Specifiers & ParseTree::ParameterSpecifier::ParamArray)
                {
                    ReportSyntaxError(
                        ERRID_MultipleOptionalParameterSpecifiers,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                Opcode = ParseTree::ParameterSpecifier::Optional;
                break;

            case tkPARAMARRAY:
                if (Specifiers & ParseTree::ParameterSpecifier::Optional)
                {
                    ReportSyntaxError(
                        ERRID_MultipleOptionalParameterSpecifiers,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                else if (Specifiers & ParseTree::ParameterSpecifier::ByRef)
                {
                    ReportSyntaxError(
                        ERRID_ParamArrayMustBeByVal,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
                Opcode = ParseTree::ParameterSpecifier::ParamArray;
                break;

            default:
                FixupListEndLocations(Result);
                return Result;
        }

        if (Specifiers & Opcode)
        {
            ReportSyntaxError(
                ERRID_DuplicateParameterSpecifier,
                m_CurrentToken,
                ErrorInConstruct);
        }
        else
        {
            Specifiers |= Opcode;
        }

        ParseTree::ParameterSpecifier *Specifier = new(m_TreeStorage) ParseTree::ParameterSpecifier;
        Specifier->Opcode = Opcode;

        SetLocation(&Specifier->TextSpan, m_CurrentToken, m_CurrentToken->m_Next);

        CreateListElement(
            ListTarget,
            Specifier,
            m_CurrentToken,
            m_CurrentToken->m_Next,
            NULL);

        GetNextToken();
#if 0        
        EatNewLine();
#endif
    }

    VSFAIL("Parsing parameter specifiers lost.");
    FixupListEndLocations(Result);
    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseParameter
*
* Purpose:
*     Parses: <ident> [()] [As <type>]
*
**********************************************************************/
ParseTree::Parameter *
Parser::ParseParameter
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::ParameterSpecifierList *Specifiers,
    _In_ Token *Start,           // [in] token starting parameter
    _Inout_ bool &ErrorInConstruct
)
{
    bool CreateAsOptional = (Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional) != NULL);
    Token *DeclaratorStart = m_CurrentToken;
    ParseTree::Declarator *Declarator = new(m_TreeStorage) ParseTree::Declarator;

    ParseDeclarator(false, Declarator, ErrorInConstruct);
    SetLocation(&Declarator->TextSpan, DeclaratorStart, m_CurrentToken);
    if (ErrorInConstruct)
    {
        // If we see As before a comma or RParen, then assume that
        // we are still on the same parameter. Otherwise, don't resync
        // and allow the caller to decide how to recover.

        if (PeekAheadFor(3, tkAS, tkComma, tkRParen) == tkAS)
        {
            ResyncAt(1, tkAS);
        }
    }

    Token *As = NULL;
    ParseTree::Type *Type = NULL;

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;
        GetNextToken();
#if 0        
        EatNewLine();
#endif

        Type = ParseGeneralType(ErrorInConstruct);
    }

    ParseTree::Parameter *Param =
        CreateAsOptional ?
            new(m_TreeStorage) ParseTree::OptionalParameter :
            new(m_TreeStorage) ParseTree::Parameter;
    Param->Name = Declarator;
    Param->Type = Type;
    Param->Specifiers = Specifiers;
    Param->Attributes = Attributes;

    SetLocationAndPunctuator(
        &Param->TextSpan,
        &Param->As,
        Start,
        m_CurrentToken,
        As);

    return Param;
}

/*********************************************************************
*
* Function:
*     Parser::ParseOptionalParameter
*
* Purpose:
*     Parses an optional parameter. Returns NULL if the parameter
*     is not optional.
*
**********************************************************************/
ParseTree::Parameter *
Parser::ParseOptionalParameter
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::ParameterSpecifierList *Specifiers,     // [in] parameter specifiers
    _In_ Token *ParamStart,       // [in] token starting parameter
    _Inout_ bool &ErrorInConstruct
)
{
    AssertIfFalse(Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))

    ParseTree::OptionalParameter *Param = ParseParameter(Attributes, Specifiers, ParamStart, ErrorInConstruct)->AsOptional();
    if (ErrorInConstruct)
    {
        ResyncAt(3, tkEQ, tkComma, tkRParen);
    }

    Token *Equals = m_CurrentToken;

    if (m_CurrentToken->m_TokenType == tkEQ)
    {
        GetNextToken(); // get off the '='
        EatNewLine(); // allow implicit EOL after '='
    }
    else
    {
        ReportSyntaxError(
            ERRID_ObsoleteOptionalWithoutValue,
            m_CurrentToken,
            ErrorInConstruct);
    }

    Param->DefaultValue = ParseDeferredExpression(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkComma, tkRParen);
    }

    SetLocationAndPunctuator(
        &Param->TextSpan,
        &Param->Equals,
        ParamStart,
        m_CurrentToken,
        Equals);

    return Param;
}

/*********************************************************************
*
* Function:
*     Parser::ParseOptionalParameters
*
* Purpose:
*
**********************************************************************/
void
Parser::ParseOptionalParameters
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::ParameterSpecifierList *Specifiers,      // [in] parameter specifiers
    _In_ Token *ParamStart,         // [in] token starting parameter
    ParseTree::ParameterList **&ParamListTarget,
    _Inout_ bool &ErrorInConstruct
)
{
    // Loop through list of optional parameters until we're
    // at the end of list or we hit a parameter that's not optional.

    ParseTree::ParameterList ** LastTarget;
    
    while (true)
    {
        ParseTree::Parameter *Param = ParseOptionalParameter(Attributes, Specifiers, ParamStart, ErrorInConstruct);

        LastTarget = ParamListTarget;
        
        CreateListElement(
            ParamListTarget,
            Param,
            ParamStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        EatNewLineIfFollowedBy(tkRParen, *LastTarget);

        if (m_CurrentToken->m_TokenType != tkComma)
        {
            if (m_CurrentToken->m_TokenType == tkRParen ||
                MustEndStatement(m_CurrentToken))
            {
                return;
            }
            else
            {
                Token *ErrStart = m_CurrentToken;

                ResyncAt(2, tkRParen, tkComma);
                ReportSyntaxError(
                    ERRID_InvalidParameterSyntax,
                    ErrStart,
                    m_CurrentToken,
                    ErrorInConstruct);

                if (m_CurrentToken->m_TokenType != tkComma)
                {
                    return;
                }

                // Sitting at a comma at this point implies a belief in synchronization.
                ErrorInConstruct = false;
            }
        }

        VSASSERT(
            m_CurrentToken->m_TokenType == tkComma,
            "must be at a comma.");

        // Set up for next parameter.
        GetNextToken();
        EatNewLine();

        ParamStart = m_CurrentToken;

        Attributes = NULL;
        if (ParamStart->m_TokenType == tkLT)
        {
            Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
        }

        Specifiers = ParseParameterSpecifiers(ErrorInConstruct);
        if (!Specifiers->HasSpecifier(ParseTree::ParameterSpecifier::Optional))
        {
            // Set the current token back to the specifiers so they
            // can be reparsed by the caller.

            m_CurrentToken = ParamStart;
            return;
        }
    }

    // Current token at either Rparen, EOS, or the start of
    // a non-optional parameter.
}

/*********************************************************************
*
* Function:
*     Parser::DoneParamList
*
* Purpose:
*
*     Test to see if the parsing of a paramter list is finished,
*     and, if not,  advance over a separating token.
*
**********************************************************************/
bool
Parser::DoneParamList
(
    _Inout_ bool &ErrorInConstruct
)
{
    EatNewLineIfFollowedBy(tkRParen);
    if (m_CurrentToken->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return false;
    }
    else if (m_CurrentToken->m_TokenType != tkRParen &&
             !MustEndStatement(m_CurrentToken))
    {
        if (m_CurrentToken->IsContinuableEOL())
        {
            if (PeekNextLine()->m_TokenType == tkRParen)
            {
                return true;
            }
        }
        // Error.
        Token *ErrStart = m_CurrentToken;

        ResyncAt(2, tkComma, tkRParen);
        ReportSyntaxError(
            ErrStart->m_TokenType == tkEQ ?
                ERRID_DefaultValueForNonOptionalParam :
                ERRID_InvalidParameterSyntax,
            ErrStart,
            m_CurrentToken,
            ErrorInConstruct);

        if (m_CurrentToken->m_TokenType == tkComma)
        {
            GetNextToken();
            EatNewLine();
            ErrorInConstruct = false;
            return false;
        }
    }

    return true;
}

ParseTree::ImportsStatement *
Parser::ParseImportsStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ImportsStatement *Result = new(m_TreeStorage) ParseTree::ImportsStatement;
    Result->Opcode = ParseTree::Statement::Imports;

    SetStmtBegLocation(Result, m_CurrentToken);

    // Target is where to link in the next list node.
    ParseTree::ImportDirectiveList **Target = &Result->Imports;

    bool first = true;

    do
    {
        Token *StartName = m_CurrentToken;
        GetNextToken();
        //Eat a new line following "," but not IMPORTS
        EatNewLineIfNotFirst(first);

        ParseTree::ImportDirective *Import = ParseOneImportsDirective(ErrorInConstruct);

        CreateListElement(
            Target,
            Import,
            StartName,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    FixupListEndLocations(Result->Imports);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

ParseTree::ImportDirective*
Parser::ParseOneImportsDirective
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::ImportDirective *Import = NULL;

    // If the imports directive begins with '<', then it is an Xml imports directive
    if (m_CurrentToken->m_TokenType == tkLT)
    {
        GetNextToken();

#if IDE
        m_SeenXmlLiterals = true;
#endif

        Import = new(m_TreeStorage) ParseTree::XmlNamespaceImportDirective;
        Import->Mode = ParseTree::ImportDirective::XmlNamespace;

        if (m_CurrentToken->m_TokenType == tkXmlNCName && StringPool::IsEqualCaseSensitive(m_CurrentToken->m_Id.m_Spelling, STRING_CONST2(m_pStringPool, XmlNs)))
        {
            // Parse namespace declaration as a regular attribute
            Import->AsXmlNamespace()->NamespaceDeclaration = ParseXmlAttribute(false, ErrorInConstruct);
            VerifyExpectedToken(tkGT, ErrorInConstruct);
        }
        else
        {
            ReportSyntaxError(ERRID_ExpectedXmlns, m_CurrentToken, ErrorInConstruct);
        }

        if (!Import->AsXmlNamespace()->NamespaceDeclaration)
        {
            Import->AsXmlNamespace()->NamespaceDeclaration = ExpressionSyntaxError();
        }
    }
    else
    {
        // Handle Clr namespace imports if we have currently have tokens for ID =
        if (m_CurrentToken->m_TokenType == tkID && 
            m_CurrentToken->m_Next->m_TokenType == tkEQ)
        {
            Import = new(m_TreeStorage) ParseTree::AliasImportDirective;
            Import->Mode = ParseTree::ImportDirective::Alias;

            if (m_CurrentToken->m_Id.m_TypeCharacter != chType_NONE)
            {
                ReportSyntaxError(ERRID_NoTypecharInAlias, m_CurrentToken, ErrorInConstruct);
            }

            Import->AsAlias()->Alias = ParseIdentifier(ErrorInConstruct);
            SetPunctuator(&Import->AsAlias()->Equals, Start, m_CurrentToken);

            GetNextToken(); // Get off the '='
            EatNewLine(); // Dev10_496850 Allow implicit line continuation after the '=', e.g. Imports a=
        }
        else
        {
            Import = new(m_TreeStorage) ParseTree::NamespaceImportDirective;
            Import->Mode = ParseTree::ImportDirective::Namespace;
        }

        Import->AsNamespace()->ImportedName = ParseName(false, ErrorInConstruct, false /* AllowGlobalNameSpace */, true /* Allow generics */);
    }

    if (ErrorInConstruct)
    {
        // Just resync at the end so we don't get any expecting EOS errors
        ResyncAt(0);
    }

    SetLocation(&Import->TextSpan, Start, m_CurrentToken);

    return Import;
}

ParseTree::ImportDirective*
Parser::ParseOneImportsDirective
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _Out_ bool &ErrorInConstruct
)
{
    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Get the first (and only) line.
    m_CurrentToken = GetNextLine(tkIMPORTS);
    ErrorInConstruct = false;

    ParseTree::ImportDirective *Result =
        ParseOneImportsDirective(ErrorInConstruct);

    if (!ErrorInConstruct &&
        m_CurrentToken &&
        !m_CurrentToken->IsEndOfLine())
    {
        Token *Start = m_CurrentToken;
        GetNextToken();

        ReportSyntaxError(
            ERRID_ExpectedEOS,
            Start,
            m_CurrentToken,
            ErrorInConstruct);
    }

    return Result;
}


// parser is passed a new stream to parse. Initlize the parser
void 
Parser::InitParserFromScannerStream
(
    _In_ Scanner *InputStream,
    _In_ ErrorTable *Errors
)
{
    m_InputStream = InputStream;
    m_Errors = Errors;
    
    m_LambdaBodyStatements.Reset();  // reset any leftover lambda bodies that may have been parsed

//      m_Conditionals.Init(&m_TreeStorage);    // we should do this too

}



/*********************************************************************
*
* Function:
*     Parser::ParseInheritsImplementsStatement
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseInheritsImplementsStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Statement::Opcodes Opcode = ParseTree::Statement::SyntaxError;

    VSASSERT(
        m_CurrentToken->m_TokenType == tkINHERITS ||
        m_CurrentToken->m_TokenType == tkIMPLEMENTS,
        "must be at Inherits or Implements.");

    switch(m_CurrentToken->m_TokenType)
    {
        case tkINHERITS:
            Opcode = ParseTree::Statement::Inherits;
            break;

        case tkIMPLEMENTS:
            Opcode = ParseTree::Statement::Implements;
            break;

        default:
            VSFAIL("Can't get here.");
            VbThrow(HrMake(ERRID_InternalCompilerError));
            break;
    }

    ParseTree::TypeListStatement *Result = new(m_TreeStorage) ParseTree::TypeListStatement;
    Result->Opcode = Opcode;
    SetStmtBegLocation(Result, m_CurrentToken);

    // Target is where to link in the next list node.
    ParseTree::TypeList **Target = &Result->Types;

    bool first = true;

    do
    {
        GetNextToken();
        //Eat a new line after "," but not "INHERITS" or "IMPLEMENTS"
        EatNewLineIfNotFirst(first);
        Token *StartName = m_CurrentToken;

        ParseTree::Type *Type = ParseTypeName(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(1, tkComma);
        }

        CreateListElement(
            Target,
            Type,
            StartName,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    FixupListEndLocations(Result->Types);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseOptionStatement
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseOptionStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    unsigned Errid = ERRID_None;
    Token *ErrStart = NULL;
    ParseTree::Statement::Opcodes Opcode = ParseTree::Statement::SyntaxError;

    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkOPTION, "must be at Option.");

    ParseTree::OptionStatement *Result = new(m_TreeStorage) ParseTree::OptionStatement;
    SetStmtBegLocation(Result, Start);

    GetNextToken();
#if 0    
    EatNewLine();
#endif
    SetPunctuator(&Result->FirstPunctuator, Start, m_CurrentToken);

    switch (TokenAsKeyword(m_CurrentToken))
    {
        case tkCOMPARE:

            GetNextToken();
#if 0
            EatNewLineIfFollowedBy(tkTEXT);
            EatNewLineIfFollowedBy(tkBINARY);
#endif            

            switch (TokenAsKeyword(m_CurrentToken))
            {
                case tkTEXT:
                    Opcode = ParseTree::Statement::OptionCompareText;
                    GetNextToken();
                    break;

                case tkBINARY:
                    Opcode = ParseTree::Statement::OptionCompareBinary;
                    GetNextToken();
                    break;

                default:
                    if (IsValidStatementTerminator(m_CurrentToken))
                    {
                        // For IntelliSense to trigger off dropping down options for Compare.
                        // Don't want to advance current token.

                        Opcode = ParseTree::Statement::OptionCompareNone;
                        Errid = ERRID_InvalidOptionCompare;
                        ErrStart = m_CurrentToken;
                    }
                    else
                    {
                        // Skip over the invalid token.
                        Errid = ERRID_InvalidOptionCompare;
                        Opcode = ParseTree::Statement::OptionInvalid;
                        ErrStart = m_CurrentToken;

                        GetNextToken();
                    }
                    break;
            }
            break;

        case tkEXPLICIT:
        case tkSTRICT:
        case tkINFER:
        {
            bool IsStrict = TokenAsKeyword(m_CurrentToken) == tkSTRICT;
            bool IsExplicit = TokenAsKeyword(m_CurrentToken) == tkEXPLICIT;
            GetNextToken();

#if 0            
            EatNewLineIfFollowedBy(tkOFF);
            EatNewLineIfFollowedBy(tkON);
            EatNewLineIfFollowedBy(tkCUSTOM);
#endif            

            if (TokenAsKeyword(m_CurrentToken) == tkOFF || m_CurrentToken->m_TokenType == tkON)
            {
                switch (TokenAsKeyword(m_CurrentToken))
                {
                    case tkON:
                        if (IsStrict)
                        {
                            Opcode = ParseTree::Statement::OptionStrictOn;
                        }
                        else if (IsExplicit)
                        {
                            Opcode = ParseTree::Statement::OptionExplicitOn;
                        }
                        else
                        {
                            Opcode = ParseTree::Statement::OptionInferOn;
                        }

                        break;

                    case tkOFF:
                        if (IsStrict)
                        {
                            Opcode = ParseTree::Statement::OptionStrictOff;
                        }
                        else if (IsExplicit)
                        {
                            Opcode = ParseTree::Statement::OptionExplicitOff;
                        }
                        else
                        {
                            Opcode = ParseTree::Statement::OptionInferOff;
                        }

                        break;

                    default:
                        VSFAIL("Option parsing is lost.");
                        if (IsStrict)
                        {
                            Errid = ERRID_InvalidOptionStrict;
                        }
                        else if (IsExplicit)
                        {
                            Errid = ERRID_InvalidOptionExplicit;
                        }
                        else
                        {
                            Errid = ERRID_InvalidOptionInfer;
                        }

                        ErrStart = m_CurrentToken;
                        Opcode = ParseTree::Statement::OptionInvalid;
                        break;
                }

                // Skip over the identifier.
                GetNextToken();
            }
            else if (TokenAsKeyword(m_CurrentToken) == tkCUSTOM)
            {
                if (IsStrict)
                {
                    Errid = ERRID_InvalidOptionStrictCustom;
                }
                else if (IsExplicit)
                {
                    Errid = ERRID_InvalidOptionExplicit;
                }
                else
                {
                    Errid = ERRID_InvalidOptionInfer;
                }

                Opcode = ParseTree::Statement::OptionInvalid;
                ErrStart = m_CurrentToken;

                GetNextToken();
            }
            else if (IsValidStatementTerminator(m_CurrentToken))
            {
                if (IsStrict)
                {
                    Opcode = ParseTree::Statement::OptionStrictOn;
                }
                else if (IsExplicit)
                {
                    Opcode = ParseTree::Statement::OptionExplicitOn;
                }
                else
                {
                    Opcode = ParseTree::Statement::OptionInferOn;
                }
            }
            else
            {
                // Skip over the invalid token.
                if (IsStrict)
                {
                    Errid = ERRID_InvalidOptionStrict;
                }
                else if (IsExplicit)
                {
                    Errid = ERRID_InvalidOptionExplicit;
                }
                else
                {
                    Errid = ERRID_InvalidOptionInfer;
                }

                Opcode = ParseTree::Statement::OptionInvalid;
                ErrStart = m_CurrentToken;

                GetNextToken();
            }

            break;
        }

        default:
            ErrStart = m_CurrentToken;

            // Error: want to distinguish between "Option" and "Option <invalid>"
            // for IntelliSense.

            Opcode =
                IsValidStatementTerminator(m_CurrentToken) ?
                    ParseTree::Statement::OptionUnknown :
                    ParseTree::Statement::OptionInvalid;

            Errid = ERRID_ExpectedForOptionStmt;

            // Error recovery.
            switch (TokenAsKeyword(m_CurrentToken))
            {
                // The following are errors but we can probably guess what was intended
                case tkTEXT:
                case tkBINARY:
                    Errid = ERRID_ExpectedOptionCompare;
                    GetNextToken();
                    break;
            }
            break;
    }

    if (Errid != ERRID_None)
    {
        VSASSERT(ErrStart, "start error token must be set.");

        ReportSyntaxError(Errid, ErrStart, m_CurrentToken, ErrorInConstruct);

        // Resync at EOS so we don't get anymore errors
        ResyncAt(0);
    }

    Result->Opcode = Opcode;
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDeclareStatement
*
* Purpose:
*
**********************************************************************/
ParseTree::ForeignMethodDeclarationStatement *
Parser::ParseProcDeclareStatement
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,
    _In_ Token *Start,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkDECLARE, "must be at a Declare.");

    // Dev10_667800 we are parsing a method declaration and will need to let the scanner know that we
    // are so the scanner can correctly identify attributes vs. xml while scanning the declaration.
    VSASSERT( m_ForceMethodDeclLineState == false, "m_ForceMethodDeclLineState not designed for nested calls.  Use a counter instead of a boolean if you need nesting");
    BackupValue<bool> backupForceMethodDeclLineState(&m_ForceMethodDeclLineState); // will restore value of m_ForceMethodDeclLineState when we return
    m_ForceMethodDeclLineState = true; 

    ParseTree::ForeignMethodDeclarationStatement *Result = new(m_TreeStorage) ParseTree::ForeignMethodDeclarationStatement;
    SetStmtBegLocation(Result, Start);

    Result->Specifiers = Specifiers;

    // Skip DECLARE.
    SetPunctuator(&Result->Declare, Start, m_CurrentToken);
    GetNextToken();

    SetPunctuator(&Result->Mode, Start, m_CurrentToken);
    if (m_CurrentToken->m_TokenType == tkID)
    {
        switch (IdentifierAsKeyword(m_CurrentToken))
        {
            case tkUNICODE:
                Result->StringMode = ParseTree::ForeignMethodDeclarationStatement::Unicode;
                GetNextToken();
                break;

            case tkANSI:
                Result->StringMode = ParseTree::ForeignMethodDeclarationStatement::ANSI;
                GetNextToken();
                break;

            case tkAUTO:
                Result->StringMode = ParseTree::ForeignMethodDeclarationStatement::Auto;
                GetNextToken();
                break;
        }
    }

    SetPunctuator(&Result->MethodKind, Start, m_CurrentToken);

    if (m_CurrentToken->m_TokenType == tkSUB)
    {
        Result->Opcode = ParseTree::Statement::ForeignProcedureDeclaration;
        GetNextToken();
    }
    else if (m_CurrentToken->m_TokenType == tkFUNCTION)
    {
        Result->Opcode = ParseTree::Statement::ForeignFunctionDeclaration;
        GetNextToken();
    } 
    else
    {
        ReportSyntaxError(ERRID_ExpectedSubFunction, m_CurrentToken, ErrorInConstruct);
        Result->Opcode = ParseTree::Statement::ForeignFunctionNone;
    }

    Result->Attributes = Attributes;

    // Parse the function name.
    Result->Name = ParseIdentifier(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkLIB, tkLParen);
    }

    Token *Lib = NULL;
    Token *Alias = NULL;

    ParseDeclareLibClause(&Result->Library, &Result->Alias, Lib, Alias, ErrorInConstruct);

    SetPunctuator(&Result->Lib, Start, Lib);
    SetPunctuator(&Result->Al, Start, Alias);

    Token *LeftParen = NULL;
    Token *RightParen = NULL;

    RejectGenericParametersForMemberDecl(ErrorInConstruct);

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        Result->Parameters = ParseParameters(ErrorInConstruct, LeftParen, RightParen, Result);
    }

    SetPunctuator(&Result->LeftParen, Start, LeftParen);
    SetPunctuator(&Result->RightParen, Start, RightParen);

    if (Result->Opcode == ParseTree::Statement::ForeignFunctionDeclaration &&
        m_CurrentToken->m_TokenType == tkAS)
    {
        SetPunctuator(&Result->As, Start, m_CurrentToken);
        GetNextToken();

        if (m_CurrentToken->m_TokenType == tkLT)
        {
            Result->ReturnTypeAttributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
        }

        Result->ReturnType = ParseGeneralType(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            // Sync at EOS to avoid any more errors.
            ResyncAt(0);
        }
    }

    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDeclareLibClause
*
* Purpose:
*
**********************************************************************/
void
Parser::ParseDeclareLibClause
(
    _Deref_out_ ParseTree::Expression **LibResult,         // [out] string literal representing Lib clause
    _Deref_out_ ParseTree::Expression **AliasResult,       // [out] string literal representing Alias clause
    _Deref_out_ Token *&Lib,
    _Deref_out_ Token *&Alias,
    _Inout_ bool &ErrorInConstruct
)
{
    Lib = NULL;
    Alias = NULL;

    if (m_CurrentToken->m_TokenType == tkLIB)
    {
        Lib = m_CurrentToken;

        // Syntax: LIB StringLiteral [ALIAS StringLiteral]
        GetNextToken();
#if 0        
        EatNewLine();
#endif

        *LibResult = ParseStringLiteral(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            ResyncAt(2, t----AS, tkLParen);
        }
    }
    else
    {
        // See if there was a Lib component somewhere and the user
        // just put it in the wrong place.

        Token *ErrStart = m_CurrentToken;
        tokens Peek = PeekAheadFor(1, tkLIB);

        if (Peek == tkLIB)
        {
            ResyncAt(1, tkLIB);

            // Create error and parse Lib clause again.
            ReportSyntaxError(ERRID_MissingLibInDeclare, ErrStart, m_CurrentToken, ErrorInConstruct);
            ParseDeclareLibClause(LibResult, AliasResult, Lib, Alias, ErrorInConstruct);
            return;
        }
        else
        {
            ResyncAt(2, t----AS, tkLParen);
            ReportSyntaxError(ERRID_MissingLibInDeclare, ErrStart, m_CurrentToken, ErrorInConstruct);

            *LibResult = ExpressionSyntaxError();
            SetExpressionLocation(*LibResult, ErrStart, m_CurrentToken, NULL);
        }
    }

    if (m_CurrentToken->m_TokenType == t----AS)
    {
        Alias = m_CurrentToken;
        GetNextToken();
#if 0        
        EatNewLine();
#endif
        *AliasResult = ParseStringLiteral(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            ResyncAt(1, tkLParen);
        }
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseEventDefinition
*
* Purpose:
*
**********************************************************************/
ParseTree::Statement *
Parser::ParseEventDefinition
(
    ParseTree::AttributeSpecifierList *Attributes,
    ParseTree::SpecifierList *Specifiers,
    _In_ Token *StatementStart,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Event = NULL;
    Token *BlockEventModifier = NULL;

    bool InInterfaceContext =
        RealContext() && RealContext()->Opcode == ParseTree::Statement::Interface;

    VSASSERT(BeginsEvent(m_CurrentToken), "must be at Event or Custom Event.");

    if (m_CurrentToken->m_TokenType == tkCUSTOM)
    {
        BlockEventModifier = m_CurrentToken;
        GetNextToken();
        Event = m_CurrentToken;

        if (InInterfaceContext)
        {
            // 'Custom' modifier invalid on event declared in an interface.

            ReportSyntaxErrorWithoutMarkingStatementBad(
                ERRID_CustomEventInvInInterface,
                BlockEventModifier,
                ErrorInConstruct);
        }
    }
    else
    {
        Event = m_CurrentToken;
    }

    VSASSERT(Event->m_TokenType == tkEVENT, "must be at Event!!!");

    GetNextToken();
#if 0    
    EatNewLine();
#endif

    ParseTree::IdentifierDescriptor Name = ParseIdentifier(ErrorInConstruct);

    ParseTree::ParameterList *Parameters = NULL;

    Token *LeftParen = NULL;
    Token *RightParen = NULL;
    ParseTree::Type *ReturnType = NULL;
    Token *As = NULL;

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;
        GetNextToken();
#if 0        
        EatNewLine();
#endif
        ReturnType = ParseGeneralType(ErrorInConstruct);
        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }
    }
    else
    {
        if (BlockEventModifier && !InInterfaceContext)
        {
            // 'Custom' modifier invalid on event declared without an explicit delegate type.

            ReportSyntaxErrorWithoutMarkingStatementBad(
                ERRID_CustomEventRequiresAs,
                BlockEventModifier,
                ErrorInConstruct);
        }

        RejectGenericParametersForMemberDecl(ErrorInConstruct);

        if (m_CurrentToken->m_TokenType == tkLParen)
        {
            Parameters = ParseParameters(ErrorInConstruct, LeftParen, RightParen);
        }

        // Give a good error if they attempt to do a return type
        if (m_CurrentToken->m_TokenType == tkAS)
        {
            Token *ErrStart = m_CurrentToken;

            ResyncAt(1, tkIMPLEMENTS);
            ReportSyntaxError(ERRID_EventsCantBeFunctions, ErrStart, m_CurrentToken, ErrorInConstruct);
        }
    }

    ParseTree::NameList *Implements = NULL;
    Token *HandlesOrImplements = NULL;
    if (m_CurrentToken->m_TokenType == tkIMPLEMENTS)
    {
        HandlesOrImplements = m_CurrentToken;
        Implements = ParseImplementsList(ErrorInConstruct);
    }

    ParseTree::EventDeclarationStatement *EventDeclaration =
        CreateMethodDeclaration(
            ParseTree::Statement::EventDeclaration,
            Attributes,
            Name,
            NULL,
            Parameters,
            ReturnType,
            NULL,
            Specifiers,
            Implements,
            NULL,
            StatementStart,
            m_CurrentToken,
            Event,
            LeftParen,
            RightParen,
            As,
            HandlesOrImplements,
            NULL,
            NULL,
            NULL,
            false)->AsEventDeclaration();

    if (BlockEventModifier)
    {
        SetLocation(&EventDeclaration->BlockEventModifier, BlockEventModifier, BlockEventModifier);
    }

    ParseTree::Statement *Result = NULL;

    // Build a block event if all the requirements for one are met
    //
    if (BlockEventModifier && As && !InInterfaceContext)
    {
        ParseTree::BlockEventDeclarationStatement *BlockEvent = new(m_TreeStorage) ParseTree::BlockEventDeclarationStatement;
        BlockEvent->Opcode = ParseTree::Statement::BlockEventDeclaration;
        BlockEvent->EventSignature = EventDeclaration;

        SetStmtBegLocation(BlockEvent, StatementStart);
        SetStmtEndLocation(BlockEvent, m_CurrentToken);

        Result = BlockEvent;
    }
    else
    {
        Result = EventDeclaration;
    }

    return Result;
}

ParseTree::AttributeSpecifierList *
Parser::ParseAttributeSpecifier
(
    ExpectedAttributeKind Expected,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkLT, "Attribute parsing lost.");

    ParseTree::AttributeSpecifierList *ResultAttributeList = new(m_TreeStorage) ParseTree::AttributeSpecifierList;
    ParseTree::AttributeSpecifierList **AttributeList = &ResultAttributeList;

    Token *AttributeListStart = m_CurrentToken;
    bool AnotherAttributeBlockExists;

    Token *pTokenToUseForEndLocation = NULL;
    
    do
    {
        Token *LessThan = m_CurrentToken;
        ParseTree::AttributeSpecifier *Result = new(m_TreeStorage) ParseTree::AttributeSpecifier;
        ParseTree::AttributeList **ListTarget = &Result->Values;

        do
        {
            GetNextToken();
            // Eat a new line following "<", or ","
            EatNewLine();

            Token *AttributeStart = m_CurrentToken;

            ParseTree::Attribute *Attribute = new(m_TreeStorage) ParseTree::Attribute;

            if (Expected == FileAttribute)
            {
                if ((m_CurrentToken->m_TokenType == tkID) && (IdentifierAsKeyword(m_CurrentToken) == tkASSEMBLY))
                {
                    Attribute->IsAssembly = true;
                    GetNextToken();
                    SetPunctuatorIfMatches(&Attribute->Colon, AttributeStart, m_CurrentToken, tkColon);
                    VerifyExpectedToken(tkColon, ErrorInConstruct);
                }
                else if (m_CurrentToken->m_TokenType == tkMODULE)
                {
                    Attribute->IsModule = true;
                    GetNextToken();
                    SetPunctuatorIfMatches(&Attribute->Colon, AttributeStart, m_CurrentToken, tkColon);
                    VerifyExpectedToken(tkColon, ErrorInConstruct);
                }
                else
                {
                    ReportSyntaxError(
                        ERRID_FileAttributeNotAssemblyOrModule,
                        m_CurrentToken,
                        ErrorInConstruct);
                }
            }

            Token *AttributeExpressionStart = m_CurrentToken;

            bool ErrorInThisAttribute = false;
            unsigned PreviousErrorCount = m_Errors ? m_Errors->GetErrorCount() : 0;

            Attribute->Name = ParseName(false, ErrorInThisAttribute, true /* AllowGlobalNameSpace*/, false);

            Token *StartOfGenericArgs = BeginsGeneric(m_CurrentToken);
            if (StartOfGenericArgs)
            {
                // Don't want to mark the construct after the attribute bad, so pass in
                // temporary instead of ErrorInThisAttribute

                bool ErrorInGenericArgs = ErrorInThisAttribute;

                ReportSyntaxError(
                    ERRID_GenericArgsOnAttributeSpecifier,
                    m_CurrentToken,
                    StartOfGenericArgs->m_Next,
                    ErrorInGenericArgs);

                // Resyncing to something more meaningful is hard, so just resync to ">"
                ResyncAt(1, tkGT);
            }
            else if (m_CurrentToken->m_TokenType == tkLParen)
            {
                Attribute->Arguments = ParseParenthesizedArguments(ErrorInThisAttribute);
            }

            SetLocation(&Attribute->TextSpan, AttributeStart, m_CurrentToken);

            // Error recovery can have set ErrorInThisAttribute false
            // in the presence of syntax errors, so also check the error count.

            if (ErrorInThisAttribute ||
                (m_Errors && m_Errors->GetErrorCount() > PreviousErrorCount))
            {
                if (ErrorInThisAttribute)
                {
                    ErrorInConstruct = true;
                }
                Attribute->DeferredRepresentationAsCall = NULL;
            }
            else
            {
                // Create a deferred expression for what looks like a call to the attribute.

                long DeferredStartPosition = AttributeExpressionStart->m_StartCharacterPosition;
                long DeferredEndPosition = m_CurrentToken->m_StartCharacterPosition;

                void *ExprBytes =
                    new(m_TreeStorage)
                    char[sizeof(ParseTree::DeferredExpression) + ((DeferredEndPosition - DeferredStartPosition) * sizeof(WCHAR))];

                ParseTree::DeferredExpression *Deferred = new(ExprBytes) ParseTree::DeferredExpression;
                Deferred->Opcode = ParseTree::Expression::Deferred;

                Deferred->TextLengthInCharacters = DeferredEndPosition - DeferredStartPosition;
                m_InputStream->CopyString(DeferredStartPosition, DeferredEndPosition, Deferred->Text);

                SetExpressionLocation(Deferred, AttributeExpressionStart, m_CurrentToken, NULL);

                Attribute->DeferredRepresentationAsCall = Deferred;

#if IDE
                FixAttributeArgumentValueOffsets(Attribute->Arguments.Values, AttributeExpressionStart->m_StartCharacterPosition);
#endif
            }

            CreateListElement(
                ListTarget,
                Attribute,
                AttributeStart,
                m_CurrentToken,
                m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        } while (m_CurrentToken->m_TokenType == tkComma);

        FixupListEndLocations(Result->Values);

        EatNewLineIfFollowedBy(tkGT);

        /* The pTokenToUseForEndLocation thing needs some explanation.  Dev10 # 585194
           When we get called by Intellisense we will only be given a portion of the text.  In that case
           the attribute will end with '>' EOL EOF  The EOL is manufactured by the scanner because the 
           scanner assures that all lines end in a carriage return.  Then we have the EOF because we were 
           only given a portion of the text.
           Implicit line continuation causes us, in this situation, to walk past the EOL and gives the
           SetLocation() function the EOF as the token to use for the ending location.  That causes us
           to be off by 1 in terms of the column information and intellisense breaks because it is looking
           at the location of the '>' token to decide if a given attribute fits in the span it is looking at.
           We go past that location and fail to understand that the attribute really does belong on the line
           being checked.

           If that wasn't fun enough, SetLocation() takes the token you hand it and backs up one token to
           determine the real end of the statement.  So we hand it one past the tkGT so it will back up to the tkGT. */
        pTokenToUseForEndLocation = m_CurrentToken->m_Next; // m_CurrentToken should be on the tkGT right now.
        VerifyExpectedToken(tkGT, ErrorInConstruct);

        if ( !ErrorInConstruct && m_CurrentToken->IsContinuableEOL())
        {
            Token * pNextToken = PeekNextLine();
            //We want to introduce an implicit line continuation after the ending ">" in an attribute declaration when:
            //       1. We are parsing file level attributes, and the next line starts with:
            //                a) <Assembly:
            //                b) <Module:
            //       2. We are parsing non file level attributes.
            if (Expected == NotFileAttribute)
            {
                EatNewLine();
            }
#if 0
            // Microsoft: Why was this code introduced? It breaks us big time when the /langVersion switch is specified (#557228-you can't
            // process any code that has two assembly attributes in a row anymore, which was legal in vs 2008) and it is unnecessary in v10.
            // I think he was anticipating a normal attribute immediately preceding an assembly attribute.  Which is an error but the way
            // the code worked before we catch it anyway.  Getting rid of this gives us exactly the same errors we had before without
            // horking /langVersion.
            else if ( pNextToken->m_TokenType == tkLT && Expected == FileAttribute )    
            {
                tokens attributeNameTokenType  = TokenAsKeyword(pNextToken->m_Next);
                if (attributeNameTokenType == tkASSEMBLY || attributeNameTokenType == tkMODULE)
                {
                    if (pNextToken->m_Next->m_Next->m_TokenType == tkColon)
                    {
                        EatNewLine();
                    }
                }
            }
#endif
        }

        if (m_CurrentToken->m_TokenType == tkLT)
        {
            AnotherAttributeBlockExists = true;
        }
        else
        {
            AnotherAttributeBlockExists = false;
        }

        SetLocation(&Result->TextSpan, LessThan, pTokenToUseForEndLocation);

        CreateListElement(
            AttributeList,
            Result,
            LessThan,
            pTokenToUseForEndLocation,
            NULL);

    } while (AnotherAttributeBlockExists);

    FixupListEndLocations(ResultAttributeList);
    SetLocation(&ResultAttributeList->TextSpan, AttributeListStart, pTokenToUseForEndLocation);

    return ResultAttributeList;
}

/*********************************************************************
*
* Function:
*     Parser::BeginsDeclaration
*
* Purpose:
*     Determines whether Start begins a module level declaration
*     or not.
*
*     The state of the parse must be restored to before function
*     entry.
*
**********************************************************************/
bool
Parser::BeginsDeclaration
(
    _In_ Token *Start            // [in] start token of potential declaration
)
{
    Scanner::LineMarker LineMark;
    bool Error;
    bool Abandoned;
    bool Tracking;
    Token *CurrentTokenBackup;
    bool CurrentTokenEolNextLineAlreadyScanned;

    // We could be in a situation where we are beginning another
    // declaration and there is a missing End.
    // Look at the first token to see if it begins a declaration.
    // If so, try parsing it as that declaration. If succeeded, then
    // keep current token at what it was at function entry and
    // return true to indicate to caller to reparse line as declaration.
    //

    Abandoned = DisableAbandonLines();
    LineMark = m_InputStream->MarkLine();
    Error = DisableErrors();
    Tracking = EnableErrorTracking();
    CurrentTokenBackup = m_CurrentToken;
    GoToEndOfLine(&CurrentTokenBackup);
    CurrentTokenEolNextLineAlreadyScanned = CurrentTokenBackup->m_EOL.m_NextLineAlreadyScanned;
    CurrentTokenBackup = m_CurrentToken;

#if DEBUG
    long CurrentStatementStartLine   = m_CurrentStatementStartLine;
    long CurrentStatementStartColumn = m_CurrentStatementStartColumn;
#endif

    m_CurrentToken = Start;

    CacheStmtLocation();

    bool ErrorInConstruct = false;

    ParseTree::AttributeSpecifierList *Attributes = NULL;
    if (m_CurrentToken->m_TokenType == tkLT)
    {
        Attributes = ParseAttributeSpecifier(NotFileAttribute, ErrorInConstruct);
    }

    // Eat up any specifiers.
    ParseTree::SpecifierList *Specifiers = ParseSpecifiers(ErrorInConstruct);

    switch (m_CurrentToken->m_TokenType)
    {
        case tkPROPERTY:
            ParsePropertyDefinition(
                Attributes, 
                Specifiers, 
                Start, 
                ErrorInConstruct, 
                false);

            break;

        case tkID:
            ParseVarDeclStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkSUB:
            ParseSubDeclaration(Attributes, Specifiers, Start, false, ErrorInConstruct);
            break;

        case tkFUNCTION:
            ParseFunctionDeclaration(Attributes, Specifiers, Start, false, ErrorInConstruct);
            break;

        case tkOPERATOR:
            ParseOperatorDeclaration(Attributes, Specifiers, Start, false, ErrorInConstruct);
            break;

        case tkINHERITS:
        case tkIMPLEMENTS:
            CacheStmtLocation();
            ParseInheritsImplementsStatement(ErrorInConstruct);
            break;

        case tkIMPORTS:
            CacheStmtLocation();
            ParseImportsStatement(ErrorInConstruct);
            break;

        case tkOPTION:
            CacheStmtLocation();
            ParseOptionStatement(ErrorInConstruct);
            break;

        case tkENUM:
            ParseEnumStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkSTRUCTURE:
            ParseTypeStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkDELEGATE:
            ParseDelegateStatement(Attributes, Specifiers, Start, ErrorInConstruct);
            break;

        case tkCUSTOM:
            // Fall through
        case tkEVENT:
        case tkDECLARE:
            break;
    }

    bool ModuleDecl = (CountOfTrackedErrors() == 0);

    // Reset to original state.
    m_InputStream->ResetToLine(LineMark);
    EnableErrors(Error);
    DisableErrorTracking(Tracking);
    EnableAbandonLines(Abandoned);
    m_CurrentToken = CurrentTokenBackup;
    GoToEndOfLine(&CurrentTokenBackup);
    CurrentTokenBackup->m_EOL.m_NextLineAlreadyScanned = CurrentTokenEolNextLineAlreadyScanned;

#if DEBUG
    m_CurrentStatementStartLine = CurrentStatementStartLine;
    m_CurrentStatementStartColumn = CurrentStatementStartColumn;
#endif

    return ModuleDecl;
}

//
//============ Methods for parsing specific executable statements
//

// Given a line number, generate a String to serve as a label identifier.

static STRING *
CreateIdentifierFromLineNumber
(
    Compiler *TheCompiler,
    __int64 LineNumber
)
{
    WCHAR LineNumberSpelling[MaxStringLengthForIntToStringConversion] = {0};

    /* Convert the line number to ASCII. */
    _ui64tow_s(LineNumber, LineNumberSpelling, _countof(LineNumberSpelling), 10);

    STRING *String;
    String = TheCompiler->AddString(LineNumberSpelling);

    return String;
}

// Parse the expression (and following text) in an If or ElseIf statement.

ParseTree::ExpressionBlockStatement *
Parser::ParseIfConstruct
(
    ParseTree::IfStatement *IfContainingElseIf,
    _Out_ bool *IsLineIf,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::IfStatement *Result;

    if (Start->m_TokenType == tkIF)
    {
        Result = new(m_TreeStorage) ParseTree::IfStatement;
        Result->Opcode = ParseTree::Statement::BlockIf;
    }
    else if (Start->m_TokenType == tkELSE)
    {
        VSASSERT(Start->m_Next->m_TokenType == tkIF, "Parser was sure it would see an Else If.");
        GetNextToken();
        ParseTree::ElseIfStatement *ElseIf = new(m_TreeStorage) ParseTree::ElseIfStatement;
        ElseIf->Opcode = ParseTree::Statement::ElseIf;
        ElseIf->ContainingIf = IfContainingElseIf;

        Result = ElseIf;
    }
    else
    {
        VSASSERT(Start->m_TokenType == tkELSEIF, "Parser was sure it would see an ElseIf.");
        ParseTree::ElseIfStatement *ElseIf = new(m_TreeStorage) ParseTree::ElseIfStatement;
        ElseIf->Opcode = ParseTree::Statement::ElseIf;
        ElseIf->ContainingIf = IfContainingElseIf;

        Result = ElseIf;
    }

    SetStmtBegLocation(Result, Start);

    GetNextToken();

    Result->Operand = ParseExpression(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(1, tkTHEN);
    }

    LinkStatement(Result);

    // If the statement turns out to be a line If, mark the If block as being line-based,
    // to avoid confusion in the case where an explicit (and extraneous) End If
    // statement (or an Else) appears on the line. Prematurely ending the if block
    // would be disastrous.

    Token *Next = m_CurrentToken;

    if (Next->m_TokenType == tkTHEN)
    {
        SetPunctuator(&Result->Then, Start, Next);

        // Fetch from the Next field instead of just getting the next token
        // in order to detect line if constructs.
        Next = m_CurrentToken->m_Next;
        GetNextToken();

        if (!m_IsLineIf && IsValidStatementTerminator(Next))
        {
            // It's a block if.
            *IsLineIf = false;
            SetStmtEndLocation(Result, m_CurrentToken);
            return Result;
        }

        if (Next->m_TokenType == tkELSE)
        {
            // It's a line if with an empty then clause.
            *IsLineIf = true;
            Result->Opcode = ParseTree::Statement::LineIf;
            SetStmtEndLocation(Result, m_CurrentToken);
            return Result;
        }

        // It's a line if.


        if (Result->Opcode == ParseTree::Statement::BlockIf)
        {
            *IsLineIf = true;
            Result->Opcode = ParseTree::Statement::LineIf;
            SetStmtEndLocation(Result, m_CurrentToken);

            bool contextIsLineIf = m_IsLineIf;
            m_IsLineIf = true;
            ParseStatementInMethodBody(ErrorInConstruct);
            m_IsLineIf = contextIsLineIf;
        }
       // else
       // {
       //     // 'line' ElseIf not legal. Exit and let 'EndOfLine expected' be reported
       // }

    }
    else
    {
        // It's a block if with a missing Then, or malformed.
        *IsLineIf = false;
        SetStmtEndLocation(Result, m_CurrentToken);
    }

    return Result;
}

// Parse a reference to a label, which can be an identifier or a line number.

void
Parser::ParseLabelReference
(
    _Out_ ParseTree::LabelReferenceStatement *LabelReference,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Next = m_CurrentToken;

    if (Next->m_TokenType == tkID)
    {
        if (Next->m_Id.m_TypeCharacter != chType_NONE)
        {
            ReportSyntaxError(ERRID_NoTypecharInLabel, Next, ErrorInConstruct);
        }
        LabelReference->Label = ParseIdentifier(ErrorInConstruct);
    }

    else if (Next->m_TokenType == tkIntCon)
    {
        if (Next->m_IntLiteral.m_TypeCharacter != chType_NONE)
        {
            ReportSyntaxError(ERRID_Syntax, Next, ErrorInConstruct);
        }

        LabelReference->Label.Name =
            CreateIdentifierFromLineNumber(
                m_Compiler,
                Next->m_IntLiteral.m_Value);
        LabelReference->Label.TypeCharacter = chType_NONE;
        LabelReference->Label.IsBad = false;
        LabelReference->Label.IsBracketed = false;
        LabelReference->Label.IsNullable = false;
        SetLocation(&LabelReference->Label.TextSpan, Next, Next->m_Next);

        LabelReference->LabelIsLineNumber = true;

        GetNextToken();
    }
    else
    {
        ReportSyntaxError(ERRID_ExpectedIdentifier, Next, ErrorInConstruct);

        LabelReference->Label.IsBad = true;
        SetLocation(&LabelReference->Label.TextSpan, Next, Next->m_Next);
    }
}

void
Parser::ParseGotoStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkGOTO,
        "Alleged GOTO isn't.");

    GetNextToken();

    ParseTree::LabelReferenceStatement *Goto = new(m_TreeStorage) ParseTree::LabelReferenceStatement;
    Goto->Opcode = ParseTree::Statement::Goto;

    ParseLabelReference(Goto, ErrorInConstruct);
    if (ErrorInConstruct)
    {
        // The label reference parse failed and produced an error.
        ResyncAt(0);
    }

    SetStmtBegLocation(Goto, Start);
    SetStmtEndLocation(Goto, m_CurrentToken);

    LinkStatement(Goto);
}

void
Parser::ParseOnErrorStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkON, "ON statement must start with ON.");

    Token *Next = GetNextToken();

    if (m_CurrentToken->m_TokenType != tkERROR)
    {
        ParseTree::Statement *UnrecognizedStatement = ReportUnrecognizedStatementError(ERRID_ObsoleteOnGotoGosub, ErrorInConstruct);
        AssertIfNull(UnrecognizedStatement);

        UnrecognizedStatement->TextSpan.m_lBegLine = Start->m_StartLine;
        UnrecognizedStatement->TextSpan.m_lBegColumn = TokenColumnBeg(Start);

        LinkStatement(UnrecognizedStatement);
        return;
    }

    ParseTree::OnErrorStatement *OnError = new(m_TreeStorage) ParseTree::OnErrorStatement;
    OnError->Opcode = ParseTree::Statement::OnError;

    if (m_Procedure && !m_pStatementLambdaBody)
    {
        // If there is a LambdaBody on the MultilineLambdaBodyStack then this
        // OnError statement is inside a lambda and should not be counted as
        // an OnError in the procedure.
        m_Procedure->ProcedureContainsOnError = true;
    }

    SetPunctuator(&OnError->Error, Start, m_CurrentToken);
    Next = GetNextToken();
    SetPunctuator(&OnError->GotoOrResume, Start, Next);

    if (Next->m_TokenType == tkRESUME)
    {
        if (GetNextToken()->m_TokenType == tkNEXT)
        {
            GetNextToken();
        }
        else
        {
            ReportSyntaxErrorWithoutMarkingStatementBad(
                ERRID_MissingNext,
                m_CurrentToken,
                ErrorInConstruct);
        }

        OnError->GotoType = ParseTree::OnErrorStatement::Next;

        if (m_Procedure)
        {
            // On Error Resume Next handlers start at index -2 in the On Error table.
            OnError->OnErrorIndex = -2 - m_Procedure->OnErrorResumeCount ;
            m_Procedure->OnErrorResumeCount++;

            m_Procedure->ProcedureContainsResume = true;
        }
    }
    else if (Next->m_TokenType == tkGOTO &&
             Next->m_Next->m_TokenType == tkIntCon &&
             Next->m_Next->m_IntLiteral.m_Value == 0)
    {
        GetNextToken();
        GetNextToken();

        OnError->GotoType = ParseTree::OnErrorStatement::Zero;
    }
    else if (Next->m_TokenType == tkGOTO &&
             Next->m_Next->m_TokenType == tkMinus &&
             Next->m_Next->m_Next->m_TokenType == tkIntCon &&
             Next->m_Next->m_Next->m_IntLiteral.m_Value == 1)
    {
        GetNextToken();
        GetNextToken();
        GetNextToken();

        OnError->GotoType = ParseTree::OnErrorStatement::MinusOne;
    }
    else if (Next->m_TokenType == tkGOTO)
    {
        GetNextToken();

        OnError->GotoType = ParseTree::OnErrorStatement::GotoLabel;
        ParseLabelReference(OnError, ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }

        if (m_Procedure)
        {
            // On Error Goto <label> handlers start at index 2 in the On Error table.
            OnError->OnErrorIndex = m_Procedure->OnErrorHandlerCount + 2;
            m_Procedure->OnErrorHandlerCount++;
        }
    }
    else
    {
        ReportSyntaxError(ERRID_ExpectedResumeOrGoto, Next, ErrorInConstruct);
        OnError->Label.IsBad = true;
    }

    SetStmtBegLocation(OnError, Start);
    SetStmtEndLocation(OnError, m_CurrentToken);
    LinkStatement(OnError);
}

void
Parser::ParseResumeStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkRESUME, "Resume statement must start with Resume.");

    ParseTree::ResumeStatement *Resume = new(m_TreeStorage) ParseTree::ResumeStatement;
    Resume->Opcode = ParseTree::Statement::Resume;

    if (m_Procedure)
    {
        m_Procedure->ProcedureContainsResume = true;
    }

    Token *Next = GetNextToken();

    if ( !IsValidStatementTerminator(Next) )

    {
        if (Next->m_TokenType == tkNEXT)
        {
            GetNextToken();
            Resume->ResumeType = ParseTree::ResumeStatement::Next;
        }
        else
        {
            ParseLabelReference(Resume, ErrorInConstruct);

            if (ErrorInConstruct)
            {
                Resume->ResumeType = ParseTree::ResumeStatement::None;
                ResyncAt(0);
            }
            else
            {
                Resume->ResumeType = ParseTree::ResumeStatement::ResumeLabel;
            }
        }
    }

    SetStmtBegLocation(Resume, Start);
    SetStmtEndLocation(Resume, m_CurrentToken);
    LinkStatement(Resume);
}

void
Parser::ParseCallStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkCALL, "Call statement must start with Call.");

    GetNextToken();

    ParseTree::Expression *Variable = ParseVariable(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    ParseTree::CallStatement *Result = CreateCallStatement(Start, Variable);

    Result->CallIsExplicit = true;

    LinkStatement(Result);
}

ParseTree::CallStatement *
Parser::CreateCallStatement
(
    _In_ Token *Start,
    _In_ ParseTree::Expression *Variable
)
{
    ParseTree::CallStatement *Result = new(m_TreeStorage) ParseTree::CallStatement;
    Result->Opcode = ParseTree::Statement::Call;

    if (Variable->Opcode == ParseTree::Expression::CallOrIndex)
    {
        // Extract the operands of the call/index expression and make
        // them operands of the call statement.

        Result->Arguments = Variable->AsCallOrIndex()->Arguments;

        Result->LeftParenthesis = Variable->FirstPunctuator;

        Variable = Variable->AsCallOrIndex()->Target;
    }

    Result->Target = Variable;

    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

void
Parser::ParseRaiseEventStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkRAISEEVENT, "RaiseEvent statement must start with RaiseEvent.");

    GetNextToken();

    ParseTree::RaiseEventStatement *Result = new(m_TreeStorage) ParseTree::RaiseEventStatement;
    Result->Opcode = ParseTree::Statement::RaiseEvent;

    Result->Event = ParseIdentifierAllowingKeyword(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        SetPunctuator(&Result->LeftParenthesis, Start, m_CurrentToken);
        Result->Arguments = ParseParenthesizedArguments(ErrorInConstruct);
    }

    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    LinkStatement(Result);
}

void
Parser::ParseRedimStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkREDIM, "Redim statement must start with Redim.");

    ParseTree::RedimStatement *Result = new(m_TreeStorage) ParseTree::RedimStatement;
    Result->Opcode = ParseTree::Statement::Redim;

    if (m_CurrentToken->m_Next->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken->m_Next) == tkPRESERVE)
    {
        Result->HasPreserve = true;
        SetPunctuator(&Result->Preserve, Start, m_CurrentToken->m_Next);
        GetNextToken();
    }

    ParseTree::ExpressionList **ListTarget = &Result->Redims;

    bool ParentRedimOrNew = m_RedimOrNew; // save old flag to preserve recursivity
    m_RedimOrNew = true;

    bool first = true;
    
    do
    {
        Token *StartDeclarator = GetNextToken();
        EatNewLineIfNotFirst(first);
        ParseTree::Expression *Var = ParseVariable(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }

        CreateListElement(
            ListTarget,
            Var,
            StartDeclarator,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (m_CurrentToken->m_TokenType == tkComma);

    m_RedimOrNew = ParentRedimOrNew;

    FixupListEndLocations(Result->Redims);

    if (m_CurrentToken->m_TokenType == tkAS)
    {
        ReportSyntaxError(
            ERRID_ObsoleteRedimAs,
            m_CurrentToken,
            ErrorInConstruct);

        ResyncAt(0);
    }

    SetStmtBegLocation(Result, Start);
    SetStmtEndLocation(Result, m_CurrentToken);

    LinkStatement(Result);
}

void
Parser::ParseHandlerStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    LinkStatement(
        ParseHandlerWithoutLinkingStatement(ErrorInConstruct));
}

ParseTree::HandlerStatement*
Parser::ParseHandlerWithoutLinkingStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkADDHANDLER || Start->m_TokenType == tkREMOVEHANDLER,
        "Handler statement parsing confused.");

    ParseTree::HandlerStatement *Result = new(m_TreeStorage) ParseTree::HandlerStatement;
    Result->Opcode =
        Start->m_TokenType == tkADDHANDLER ?
            ParseTree::Statement::AddHandler :
            ParseTree::Statement::RemoveHandler;
    SetStmtBegLocation(Result, Start);

    GetNextToken();

    Result->Event = ParseExpression(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(1, tkComma);
    }

    SetPunctuatorIfMatches(&Result->AsHandler()->Comma, Start, m_CurrentToken, tkComma);
    VerifyTokenPrecedesExpression(tkComma, ErrorInConstruct,true, Result);

    Result->Delegate = ParseExpression(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    SetStmtEndLocation(Result, m_CurrentToken);

    return Result;
}

ParseTree::HandlerStatement*
Parser::ParseHandlerStatement
(
    _In_ Scanner *InputStream,
    ErrorTable *Errors,
    _Out_ bool &ErrorInConstruct
)
{
    InitParserFromScannerStream(InputStream, Errors);

    m_Conditionals.Init(&m_TreeStorage);

    // Get the first (and only) line.
    m_CurrentToken = GetNextLine();

    ErrorInConstruct = false;

    return ParseHandlerWithoutLinkingStatement(ErrorInConstruct);
}

void
Parser::ParseExpressionBlockStatement
(
    ParseTree::Statement::Opcodes Opcode,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionBlockStatement *Result = new(m_TreeStorage) ParseTree::ExpressionBlockStatement;
    Result->Opcode = Opcode;

    SetStmtBegLocation(Result, m_CurrentToken);

    GetNextToken();

    Result->Operand = ParseExpression(ErrorInConstruct);
    
    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    SetStmtEndLocation(Result, m_CurrentToken);

    LinkStatement(Result);
}

void
Parser::ParseAssignmentStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkLET ||
            Start->m_TokenType == tkSET,
        "Assignment statement parsing is lost.");

    // Let and set are now illegal

    ReportSyntaxErrorWithoutMarkingStatementBad(
        ERRID_ObsoleteLetSetNotNeeded,
        m_CurrentToken,
        ErrorInConstruct);

    GetNextToken();

    ParseTree::AssignmentStatement *Result = new(m_TreeStorage) ParseTree::AssignmentStatement;
    Result->Opcode = ParseTree::Statement::Assign;
    SetStmtBegLocation(Result, Start);

    Result->Target = ParseVariable(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(1, tkEQ);
    }

    Token *Next = m_CurrentToken;

    if (IsAssignmentOperator(Next))
    {
        SetPunctuator(&Result->Operator, Start, Next);
        Result->Opcode = GetAssignmentOperator(Next);

        GetNextToken();
    }
    else
    {
        ReportSyntaxError(ERRID_ExpectedAssignmentOperator, m_CurrentToken, ErrorInConstruct);
    }

    Result->Source = ParseExpression(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    SetStmtEndLocation(Result, m_CurrentToken);
    LinkStatement(Result);
}

void
Parser::ParseTry
(
)
{
    Token *Start = m_CurrentToken;
    VSASSERT(Start->m_TokenType == tkTRY, "Try statement parsing lost.");

    ParseTree::ExecutableBlockStatement *Try = new(m_TreeStorage) ParseTree::ExecutableBlockStatement;
    Try->Opcode = ParseTree::Statement::Try;

    if (m_Procedure)
    {
        m_Procedure->ProcedureContainsTry = true;
    }

    SetStmtBegLocation(Try, Start);
    GetNextToken();
    SetStmtEndLocation(Try, m_CurrentToken);

    LinkStatement(Try);
}

void
Parser::ParseCatch
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    VSASSERT(Start->m_TokenType == tkCATCH, "Catch statement parsing lost.");

    // Find the nearest containing Try, Catch, or Finally, and make that the
    // current context (if it isn't already).
    ParseTree::BlockStatement *ContainingHandlerContext = FindContainingHandlerContext();
    if (ContainingHandlerContext)
    {
        RecoverFromMissingEnds(ContainingHandlerContext);
    }

    ParseTree::ExecutableBlockStatement *ContainingTry = NULL;
    ParseTree::Statement::Opcodes Context = m_Context->Opcode;

    if (Context == ParseTree::Statement::Try ||
        Context == ParseTree::Statement::Catch)
    {
        ContainingTry =
            Context == ParseTree::Statement::Try ?
                m_Context->AsExecutableBlock() :
                m_Context->AsCatch()->ContainingTry;
        PopContext(Context == ParseTree::Statement::Catch, Start);
    }
    else if (Context == ParseTree::Statement::Finally)
    {
        ReportSyntaxError(ERRID_CatchAfterFinally, m_CurrentToken, ErrorInConstruct);
        ContainingTry = m_Context->AsFinally()->ContainingTry;
        PopContext(true, Start);
    }
    else
    {
        ReportSyntaxError(ERRID_CatchNoMatchingTry, m_CurrentToken, ErrorInConstruct);
    }

    // Create the new context.

    ParseTree::CatchStatement *Catch = new(m_TreeStorage) ParseTree::CatchStatement;
    Catch->Opcode = ParseTree::Statement::Catch;
    Catch->ContainingTry = ContainingTry;
    SetStmtBegLocation(Catch, Start);

    GetNextToken();

    if (m_CurrentToken->m_TokenType == tkID)
    {
        Token *StartDecl = m_CurrentToken;

        Catch->Name = ParseIdentifier(ErrorInConstruct);

        if (m_CurrentToken->m_TokenType == tkAS)
        {
            SetPunctuator(&Catch->As, Start, m_CurrentToken);

            GetNextToken();

            Catch->Type = ParseTypeName(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(2, tkWHEN, tkREM);
            }

            Catch->LocalsCount++;
        }
    }

    if (m_CurrentToken->m_TokenType == tkWHEN)
    {
        SetPunctuator(&Catch->When, Start, m_CurrentToken);
        GetNextToken();

        Catch->WhenClause = ParseExpression(ErrorInConstruct);
    }

    SetStmtEndLocation(Catch, m_CurrentToken);

    LinkStatement(Catch);
}

void
Parser::ParseFinally
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    VSASSERT(Start->m_TokenType == tkFINALLY, "Finally statement parsing lost.");

    // Find the nearest containing Try, Catch, or Finally, and make that the
    // current context (if it isn't already).
    ParseTree::BlockStatement *ContainingHandlerContext = FindContainingHandlerContext();
    if (ContainingHandlerContext)
    {
        RecoverFromMissingEnds(ContainingHandlerContext);
    }

    ParseTree::ExecutableBlockStatement *ContainingTry = NULL;
    ParseTree::Statement::Opcodes Context = m_Context->Opcode;

    if (Context == ParseTree::Statement::Try ||
        Context == ParseTree::Statement::Catch)
    {
        ContainingTry =
            Context == ParseTree::Statement::Try ?
                m_Context->AsExecutableBlock() :
                m_Context->AsCatch()->ContainingTry;
        PopContext(Context == ParseTree::Statement::Catch, Start);
    }
    else if (Context == ParseTree::Statement::Finally)
    {
        ReportSyntaxError(ERRID_FinallyAfterFinally, m_CurrentToken, ErrorInConstruct);
        ContainingTry = m_Context->AsFinally()->ContainingTry;
        PopContext(true, Start);
    }
    else
    {
        ReportSyntaxError(ERRID_FinallyNoMatchingTry, m_CurrentToken, ErrorInConstruct);
    }

    // Create the new context.

    ParseTree::FinallyStatement *Finally = new(m_TreeStorage) ParseTree::FinallyStatement;
    Finally->Opcode = ParseTree::Statement::Finally;
    Finally->ContainingTry = ContainingTry;
    SetStmtBegLocation(Finally, Start);

    GetNextToken();
    SetStmtEndLocation(Finally, m_CurrentToken);

    LinkStatement(Finally);
}

// Parse an Erase statement.

void
Parser::ParseErase
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkERASE,
        "Erase statement parsing lost.");

    ParseTree::EraseStatement *Erase = new(m_TreeStorage) ParseTree::EraseStatement;
    Erase->Opcode = ParseTree::Statement::Erase;

    SetStmtBegLocation(Erase, Start);

    GetNextToken();

    Erase->Arrays = ParseVariableList(ErrorInConstruct);

    SetStmtEndLocation(Erase, m_CurrentToken);

    LinkStatement(Erase);
}

// Parse a Mid statement.

void
Parser::ParseMid
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkID &&
        IdentifierAsKeyword(Start) == tkMID,
        "Mid statement parsing lost.");

    GetNextToken();

    ParseTree::AssignMidStatement *AssignMid = new(m_TreeStorage) ParseTree::AssignMidStatement;
    AssignMid->Opcode = ParseTree::Statement::AssignMid;

    SetPunctuatorIfMatches(&AssignMid->LeftParen, Start, m_CurrentToken, tkLParen);
    VerifyTokenPrecedesVariable(tkLParen, ErrorInConstruct, true, AssignMid);

    AssignMid->Target = ParseVariable(ErrorInConstruct);
    AssignMid->Length = NULL;
    AssignMid->TypeCharacter = Start->m_Id.m_TypeCharacter;

    if (ErrorInConstruct)
    {
        AssignMid->Start = ExpressionSyntaxError();
        SetExpressionLocation(AssignMid->Start, m_CurrentToken, m_CurrentToken, NULL);

        ResyncAt(0);
    }

    else
    {
        SetPunctuatorIfMatches(&AssignMid->FirstComma, Start, m_CurrentToken, tkComma);
        VerifyTokenPrecedesExpression(tkComma, ErrorInConstruct, true, AssignMid);

        AssignMid->Start = ParseExpression(ErrorInConstruct);
        EatNewLineIfFollowedBy(tkRParen, AssignMid);

        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }

        else
        {
            if (m_CurrentToken->m_TokenType == tkComma)
            {
                SetPunctuator(&AssignMid->SecondComma, Start, m_CurrentToken);
                GetNextToken();
                EatNewLine();
                AssignMid->Length = ParseExpression(ErrorInConstruct);
                EatNewLineIfFollowedBy(tkRParen, AssignMid);

                if (ErrorInConstruct)
                {
                    ResyncAt(0);
                }
            }

            SetPunctuatorIfMatches(&AssignMid->RightParen, Start, m_CurrentToken, tkRParen);
            VerifyExpectedToken(tkRParen, ErrorInConstruct);
        }
    }

    SetPunctuatorIfMatches(&AssignMid->Equals, Start, m_CurrentToken, tkEQ);
    VerifyTokenPrecedesExpression(tkEQ, ErrorInConstruct);

    if (ErrorInConstruct)
    {
        AssignMid->Source = ExpressionSyntaxError();
        SetExpressionLocation(AssignMid->Source, m_CurrentToken, m_CurrentToken, NULL);

        ResyncAt(0);
    }
    else
    {
        AssignMid->Source = ParseExpression(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(0);
        }
    }

    SetStmtBegLocation(AssignMid, Start);
    SetStmtEndLocation(AssignMid, m_CurrentToken);

    LinkStatement(AssignMid);
}

void
Parser::ParseAwaitStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionStatement *Await = new(m_TreeStorage) ParseTree::ExpressionStatement;
    Await->Opcode = ParseTree::Statement::Await;

    Token* Start = m_CurrentToken;

    Await->Operand = ParseExpression(PrecedenceAwait, ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(0);
    }

    SetStmtBegLocation(Await, Start);
    SetStmtEndLocation(Await, m_CurrentToken);

    LinkStatement(Await);
};

//
//============ Methods for parsing portions of executable statements ==
//

/*********************************************************************
*
* Function:
*     Parser::ParseDeferredExpression
*
* Purpose:
*
*     Parses a deferred expression.  This also saves the text
*     of the expression in addition to the trees.
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseDeferredExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    long StartPosition = Start->m_StartCharacterPosition;

    ParseTree::Expression *Expr = ParseExpression(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        return Expr;
    }

    // Remember where the expression ended.
    long EndPosition = m_CurrentToken->m_StartCharacterPosition;

    void *ExprBytes = new(m_TreeStorage)
        char[sizeof(ParseTree::DeferredExpression) + ((EndPosition - StartPosition) * sizeof(WCHAR))];

    ParseTree::DeferredExpression *Deferred = new(ExprBytes) ParseTree::DeferredExpression;
    Deferred->Opcode = ParseTree::Expression::Deferred;
    Deferred->Value = Expr;
    Deferred->TextLengthInCharacters = EndPosition - StartPosition;
    m_InputStream->CopyString(StartPosition, EndPosition, Deferred->Text);

    SetExpressionLocation(Deferred, Start, m_CurrentToken, NULL);

    return Deferred;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDeferredInitializer
*
* Purpose:
*
*     Parses a deferred initializer.  This also saves the text
*     of the expression in addition to the trees.
*
**********************************************************************/
ParseTree::Initializer *
Parser::ParseDeferredInitializer
(
    _Inout_ bool &ErrorInConstruct
)
{
    long StartPosition = m_CurrentToken->m_StartCharacterPosition;
    long EndPosition;
    Token *Start = m_CurrentToken;

    ParseTree::Initializer *Initializer =
        ParseInitializer(
            ErrorInConstruct,
            true,   // Make the initializer expression a deferred expression
            true,   // Allow expression initializer
            false  // Disallow assignment initializer
        );

    if (ErrorInConstruct)
    {
        return Initializer;
    }

    // Remember where the expression ended.
    EndPosition = m_CurrentToken->m_StartCharacterPosition;

    void *InitializerBytes =
        new(m_TreeStorage)
        char[sizeof(ParseTree::DeferredInitializer) + ((EndPosition - StartPosition) * sizeof(WCHAR))];

    ParseTree::DeferredInitializer *Deferred = new(InitializerBytes) ParseTree::DeferredInitializer;
    Deferred->Opcode = ParseTree::Initializer::Deferred;
    Deferred->Value = Initializer;
    Deferred->TextLengthInCharacters = EndPosition - StartPosition;
    m_InputStream->CopyString(StartPosition, EndPosition, Deferred->Text);

    return Deferred;
}

/*****************************************************************************************
 ;EatNewLine

 If the current token is an EOL, we move to the next token (i.e. the next line)
 
 Note: Doesn't do anything if we are evaluating a conditional compilation expression
If the next token is marked as a syntax error, we stay put.
******************************************************************************************/
void Parser::EatNewLine()
{
    // Don't eat EOL's while evaluating conditional complication constants. 
    if (m_CurrentToken->IsEndOfLine() && ! m_EvaluatingConditionCompilationExpression)
    {
        if ( !NextLineStartsWith( tkEOL, false ))
        {
            AssertLanguageFeature(FEATUREID_LineContinuation, m_CurrentToken);
            GetNextToken();
        }
    }
}

/*****************************************************************************************
 ;EatNewLineIfFollowedBy

 If the current token is an EOL token, eat the EOL on the condition that the next line
 starts with the specified token
******************************************************************************************/
void Parser::EatNewLineIfFollowedBy
(
    tokens tokenType, // the token we want to check for on start of the next line
    bool isQueryOp // [in] whether the tokenType represents a query operator token
)
{    
    if ( NextLineStartsWith( tokenType, isQueryOp ))
    {
        EatNewLine();
    }
}

/*****************************************************************************************
 ;NextLineStartsWith

 Determine if the next line starts with the specified token.
******************************************************************************************/
bool
Parser::NextLineStartsWith
(
    tokens tokenType, // the token we want to check for on start of the next line
    bool isQueryOp // [in] whether the tokenType represents a query operator token
)
{
    bool NextLineDoesStartWithToken = false;

    if (m_CurrentToken->IsEndOfLine())
    {
        Token * pLookAhead = PeekNextLine();
        
        // If the start of the next line starts with the specified token, then eat the current EOL
        if (pLookAhead->m_TokenType == tokenType || 
            ( pLookAhead->m_TokenType == tkID && 
              IdentifierAsKeyword(pLookAhead) == tokenType))
        {
            NextLineDoesStartWithToken = true;
        }
    }
    return NextLineDoesStartWithToken;
}

/*****************************************************************************************
 ;EatNewLineIfNotFollowedBy

 If the current token is an EOL token, eat the EOL on the condition that the next line
 does not start with the specified token.
******************************************************************************************/
void Parser::EatNewLineIfNotFollowedBy
(
    tokens tokenType, // the token we want to check for on start of the next line
    bool isQueryOp // [in] whether the tokenType represents a query operator token
)
{    
    if ( !NextLineStartsWith( tokenType, isQueryOp ))
    {
        EatNewLine();
    }
}

void Parser::EatNewLineIfNotFirst
(
    bool & first
)
{
    if (!first)
    {
       EatNewLine();
    }
    else
    {
        first = false;
    }
}


/*********************************************************************
*
* Function:
*     Parser::ParseExpression
*
* Purpose:
*
*     Parses an expression.
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseExpression
(
    OperatorPrecedence PendingPrecedence,         // [in] precedence of previous oper
    _Inout_ bool &ErrorInConstruct,
    bool EatLeadingNewLine,                       // Note: this flag is incompatible with BailIfFirstTokenRejected
    ParseTree::Expression * pContainingExpression,
    bool BailIfFirstTokenRejected                 // bail (return NULL) if the first token isn't a valid expression-starter, rather than reporting an error or setting ErrorInConstruct
)
{
    // Note: this function will only ever return NULL if the flag "BailIfFirstTokenIsRejected" is set,
    // and if the first token isn't a valid way to start an expression. In all other error scenarios
    // it returns a "bad expression".
    ThrowIfFalse(!EatLeadingNewLine || pContainingExpression);
    ThrowIfFalse(!pContainingExpression || EatLeadingNewLine);
    ThrowIfFalse(!EatLeadingNewLine || !BailIfFirstTokenRejected); // because the current implementation doesn't support undoing the eating if it turned out we had to bail
        
    ParseTree::Expression *Expr = NULL;
    
    if (EatLeadingNewLine)
    {
        EatNewLine();
    }

    Token *Start = m_CurrentToken;    

    if (m_EvaluatingConditionCompilationExpression &&
        !StartsValidConditionalCompilationExpr(Start))
    {
        if (BailIfFirstTokenRejected)
        {
            return NULL;
        }

        ReportSyntaxError(
            ERRID_BadCCExpression,
            Start,
            ErrorInConstruct);

        Expr = ExpressionSyntaxError();
        SetExpressionLocation(Expr, Start, Start, NULL);
        return Expr;
    }

    // Check for leading unary operators
    switch (Start->m_TokenType)
    {
        case tkMinus:           // "-" unary minus
            GetNextToken();

            Expr = new(m_TreeStorage) ParseTree::UnaryExpression;
            Expr->Opcode = ParseTree::Expression::Negate;
            Expr->AsUnary()->Operand = ParseExpression(PrecedenceNegate, ErrorInConstruct);
            SetExpressionLocation(Expr, Start, m_CurrentToken, Start);
            break;

        case tkNOT:             // NOT expr
            GetNextToken();
            Expr = new(m_TreeStorage) ParseTree::UnaryExpression;
            Expr->Opcode = ParseTree::Expression::Not;
            Expr->AsUnary()->Operand = ParseExpression(PrecedenceNot, ErrorInConstruct);
            SetExpressionLocation(Expr, Start, m_CurrentToken, Start);
            break;

        case tkPlus:            // "+" unary plus
            GetNextToken();

            // unary "+" has the same precedence as unary "-"

            Expr = new(m_TreeStorage) ParseTree::UnaryExpression;
            Expr->Opcode = ParseTree::Expression::UnaryPlus;
            Expr->AsUnary()->Operand = ParseExpression(PrecedenceNegate, ErrorInConstruct);
            SetExpressionLocation(Expr, Start, m_CurrentToken, Start);
            break;

        case tkADDRESSOF:
            GetNextToken();

            Expr = new(m_TreeStorage) ParseTree::AddressOfExpression;
            Expr->Opcode = ParseTree::Expression::AddressOf;
            Expr->AsUnary()->Operand = ParseExpression(PrecedenceNegate, ErrorInConstruct);
            Expr->AsAddressOf()->UseLocationOfTargetMethodForStrict = false;
            SetExpressionLocation(Expr, Start, m_CurrentToken, Start);
            break;

        case tkAWAIT:
            AssertLanguageFeature(FEATUREID_AsyncExpressions, m_CurrentToken);
            GetNextToken();

            Expr = new(m_TreeStorage) ParseTree::UnaryExpression;
            Expr->Opcode = ParseTree::Expression::Await;
            Expr->AsUnary()->Operand = ParseExpression(PrecedenceAwait, ErrorInConstruct);
            SetExpressionLocation(Expr, Start, m_CurrentToken, Start);
            break;

        default:
            Expr = ParseTerm(ErrorInConstruct, BailIfFirstTokenRejected);

            if (Expr == NULL)
            {
                VSASSERT(BailIfFirstTokenRejected, "Unexpected: the only way for ParseTerm to return NULL is if we set the flag BailIfFirstTokenRejected");
                return NULL;
            }

            break;
    }

    if (ParseTree::Expression::ArrayInitializer != Expr->Opcode &&
        !( (Expr->Opcode == ParseTree::Expression::Lambda) && 
           Expr->AsLambda()->IsStatementLambda &&
         !Expr->AsLambda()->GetStatementLambdaBody()->HasProperTermination))    // Array initializer expressions NYI and we do not want to enter here if the expression is a multiline lambda without an end construct.
    {
        // Parse operators that follow the term according to precedence.

        while (true)
        {
            OperatorPrecedence Precedence;
            ParseTree::Expression::Opcodes Opcode;

            if (!IsBinaryOperator(m_CurrentToken))
            {
                break;
            }

            if (m_EvaluatingConditionCompilationExpression &&
                !IsValidOperatorForConditionalCompilationExpr(m_CurrentToken))
            {
                ReportSyntaxError(
                    ERRID_BadCCExpression,
                    m_CurrentToken,
                    ErrorInConstruct);

                break;
            }

            Opcode = GetBinaryOperator(m_CurrentToken, &Precedence);

            VSASSERT(Precedence != PrecedenceNone, "should have a non-zero precedence for operators.");

            // Only continue parsing if precedence is high enough
            if (Precedence <= PendingPrecedence)
            {
                break;
            }

            Token *Operator = m_CurrentToken;

            // Parse the next operand, make node, and continue looking.
            GetNextToken();

            ParseTree::BinaryExpression *Binary = new(m_TreeStorage) ParseTree::BinaryExpression;
            Binary->Opcode = Opcode;

            Binary->Left = Expr;
            Binary->Right = 
                ParseExpression
                (
                    Precedence, 
                    ErrorInConstruct, 
                    true, // eat leading EOL tokens because we allow implicit line continuations after binary operators
                    Binary
                );

            Expr = Binary;

            SetExpressionLocation(Expr, Start, m_CurrentToken, Operator);
        }
    }

    return Expr;
}

ParseTree::LambdaExpression*
Parser::ParseStatementLambda
( 
    _In_ ParseTree::ParameterList *pParams, 
    Token *pStart,
    bool functionLambda, 
    bool isSingleLine,
    bool parseBody,  // normally true. Set this to false to prevent it trying to parse the body. That's only allowed when isSingleLine.
    bool &errorInConstruct
)
{   
    // This function parses a statement lambda and returns a ParseTree::LambdaExpression with
    // text-spans all set up. Here's how the text-spans work for statement lambdas:
    //
    // <1 <2 <3 Sub() 3>                           <1..1> LambdaExpression.TextSpan
    //              Console.WriteLine("hello")     <2..2> LambdaExpression.GetStatementLambdaBody.BodyTextSpan
    //       2> End Sub 1>                         <3..3> LambdaExpression.GetStatementLambdaBody.TextSpan
    //
    // <1 <2 <3 Sub() 3> Console.WriteLine("hello") 2> 1>
    // 
    // Note that in the multiline case, 2> was the first character of EndSub (i.e. the token following the statement).
    // This is needed so that the debugger can highlight the "End Sub" line.
    // But in the single-line case, 2> is like 1> the last character of the statement.

    VSASSERT( isSingleLine ||
              m_CurrentToken->m_TokenType == tkEOL || 
              m_CurrentToken->m_TokenType == tkREM ||
              m_CurrentToken->m_TokenType == tkAS, 
              "for a multiline lambda, won't the next token always be an EOL, REM, or AS token?");

    ThrowIfFalse(parseBody || isSingleLine); // If we're asked not to parse the body, this only makes
    // sense if it's a single-line lambda. (why? because a multiline lambda only makes sense if we can
    // parse the body up to the final "End Sub/Function").
    
    // Backup m_LastStatementLinked because we will parse the statements within the multiline lambda and this
    // will cause m_LastStatementLinked to be modified; however, when we are finished parsing the lambda, we will
    // want to link the statement that contains the lambda expression into the statement list as a peer to the current
    // m_LastStatementLinked. Thus, we backup the pointer so it is not lost.
    BackupValue<ParseTree::StatementList*> backupLastStatementLinked(&m_LastStatementLinked);
    // Backup m_KeepLines because we are modifying this bool while we parse the statements within the multiline lambda
    // expression. We need to keep the current lines because we need the location information of many of the current
    // tokens when we return from ParseStatementLambda and continue parsing the statement that contains the lambda.
    BackupValue<bool> backupKeepLines(&m_KeepLines);
    // Backup m_SeenLambdaExpression because this gets reset in nested lambdas and we do not want to loose the
    // value of this bool for the statement that contains the lambda expression.
    BackupValue<bool> backupSeenLambda(&m_SeenLambdaExpression);
    // Backup m_FirstStatementOnLine because this value will get reset while we parse the statements within the 
    // multiline lambda.
    BackupValue<bool> backupFirstOnLine(&m_FirstStatementOnLine);
    // Backup the context and context index because we may lose contexts in error scenarios.
    // e.g. We may pop contexts because we call RecoverFromMissingEnds. This is ok if we
    // only pop as many contexts as we add, however, we may pop MORE contexts than we add if
    // RecoverFromMissingEnds does not find the context it is looking for.  We do not want to lose
    // any of the contexts that we had when we entered this method.
    BackupValue<ParseTree::BlockStatement*> backupContext(&m_Context);
    BackupValue<int> backupContextIndex(&m_ContextIndex);
    BackupValue<ParseTree::LambdaBodyStatement*> backupLambdaBody(&m_pStatementLambdaBody);
    BackupValue<ParseTree::LambdaBodyStatement*>backupFirstLambdaBody(&m_pFirstMultilineLambdaBody); // Microsoft: this is subtle - it is what ultimately sets m_pFirstMultilineLambdaBody == NULL on return from outermost lambda

    BackupValue<bool> backupCurrentError(&m_CurrentStatementInError);

    // Backup the last label pointer, this points to the last label that has been added to the current context.
    // We want to set this to null before we parse the body of the statement label, but we want to restore the current
    // value after we are done parsing the lambda and return to the parent context.
    BackupValue<ParseTree::StatementList*> backupLastLabel(&m_LastLabel);

    BackupValue<ParseTree::Statement::Opcodes> backupExitOpcode(&m_ExpectedExitOpcode);

    // Microsoft: 501503
    // We have to back up m_IsLineIf and set it to false when we parse the multiline lambda body. This is because
    // the lambda may be in a line if, and this will cause the flag to be true, which causes parsing subsequence
    // ifs to be totally horked.
    BackupValue<bool> backupLineIf(&m_IsLineIf);
    m_IsLineIf = false;

    // Dev10#680412
    // The flag "m_IsLineParse" indicates that we're parsing a line without any context. The effect is that
    // the parser does better error recovery for mismatched End nodes &c. to give a better parse-tree for
    // intellisense. But within a lambda, we do have full context, so this isn't needed.
    BackupValue<bool> backupIsLineParse(&m_IsLineParse);
    m_IsLineParse = false;

    


#if DEBUG
    // These longs are only used when DEBUG is true, they are used to verify location information.
    // We need to backup these values because they can be modified while parsing the statements
    // within the multiline lambda expression.
    BackupValue<long> backupStartLine(&m_CurrentStatementStartLine);
    m_CurrentStatementStartLine = m_CurrentToken->m_StartLine;
    BackupValue<long> backupStartColumn(&m_CurrentStatementStartColumn);
    m_CurrentStatementStartColumn = m_CurrentToken->m_StartColumn;
#endif
    // Set the last statement linked to NULL because we do not want to link the multiline lambda
    // statements as peers to this statement.
    m_LastStatementLinked = NULL;
    // Need to keep the lines in the parser.
    m_KeepLines = true;

    // We parse field level multiline lambdas twice. The first pass is during regular parsing
    // and the second pass is during InitializeFields(). If the lambda contains errors that mark
    // a given parse tree bad, then its owning field will not run through InitializeFields. However,
    // there are some errors (e.g. missing the end construct of a block) that do not mark a tree bad.
    // We do NOT want to report these errors twice, so we disable error reporting during this pass.
    m_ErrorReportingDisabled = m_InitializingFields ? true : m_ErrorReportingDisabled;

    // Set the last label to null.
    m_LastLabel = NULL;

    ParseTree::Type *pReturnType = NULL;
    ParseTree::AttributeSpecifierList *pReturnTypeAttributes = NULL;

    Token *As = NULL;

    // Check the return type.
    if (!isSingleLine && m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;

        if (functionLambda)
        {
            GetNextToken();

            if (m_CurrentToken->m_TokenType == tkLT)
            {
                Token *pAttrStart = m_CurrentToken;
                pReturnTypeAttributes = ParseAttributeSpecifier(NotFileAttribute, errorInConstruct);
                ReportSyntaxError( ERRID_AttributeOnLambdaReturnType, 
                                   pAttrStart,
                                   m_CurrentToken,
                                   errorInConstruct);
            }

            pReturnType = ParseGeneralType(errorInConstruct);

            if (errorInConstruct)
            {
                ResyncAt(0);
            }
        }
        else
        {
            GoToEndOfLine(&m_CurrentToken);
            
            ReportSyntaxError
                (
                ERRID_ExpectedEOS,
                As,
                m_CurrentToken,
                errorInConstruct);
        }

    }

    ParseTree::LambdaExpression* pLambdaExpr = 
        new (m_TreeStorage) ParseTree::LambdaExpression;
    pLambdaExpr->Opcode = ParseTree::Expression::Lambda;
    pLambdaExpr->Parameters = pParams;
    pLambdaExpr->MethodFlags |= functionLambda ? DECLF_Function : 0;
    pLambdaExpr->MethodFlags |= (pStart->m_TokenType == tkASYNC) ? DECLF_Async : 0;
    pLambdaExpr->MethodFlags |= (pStart->m_TokenType == tkITERATOR) ? DECLF_Iterator : 0;
    pLambdaExpr->AllowRelaxationSemantics = true;



    // Set the expected exit opcode in case we have Exit Sub or Exit Function statements within the lambda.
    m_ExpectedExitOpcode = functionLambda ? ParseTree::Statement::ExitFunction : ParseTree::Statement::ExitSub;
    
    ParseTree::HiddenBlockStatement *pHiddenBlock = new (m_TreeStorage) ParseTree::HiddenBlockStatement;
    pHiddenBlock->Opcode = ParseTree::Statement::HiddenBlock;
    pHiddenBlock->SetParent(m_Context);
    pLambdaExpr->SetStatementLambdaHiddenBlock(pHiddenBlock, functionLambda, isSingleLine);
    EstablishContext(pHiddenBlock);

    ParseTree::LambdaBodyStatement *pLambdaBody = new (m_TreeStorage) ParseTree::LambdaBodyStatement;
    pLambdaBody->Opcode = ParseTree::Statement::LambdaBody;
    SetPunctuator(&pLambdaBody->As, pStart, As);
    pLambdaBody->pReturnType= pReturnType;
    pLambdaBody->pReturnTypeAttributes = pReturnTypeAttributes;
    pLambdaBody->pOwningLambdaExpression = pLambdaExpr;
    pLambdaBody->linkedIntoStatementList = false;
    
    LinkStatement(pLambdaBody);                 // must occur before m_pStatementLambdaBody changed
    m_pStatementLambdaBody = pLambdaBody;   // this must occur after LinkStatement so owner hookup works.

    // If m_pFirstMultilineLabdaBody is NULL, then this is the first, outermost lambda, we need to keep track of this.
    if (m_pFirstMultilineLambdaBody == NULL)
    {
        m_pFirstMultilineLambdaBody = pLambdaBody;
    }

    m_LambdaBodyStatements.Add()= pLambdaBody;   //let's save this: we set the owner in LinkStatement when we finish parsing the owner

    pLambdaBody->pOuterMostLambdaBody = m_pFirstMultilineLambdaBody;

    // Must reset m_CurrentStatementStartColumn and m_CurrentStatementStartLine because this may be a
    // nested multiline lambda expression. If that is the case, m_CurrentStatementStartColumn and 
    // m_CurrentStatementStartLine refer to the column and line that the first multiline lambda was created on.
    //
    #if DEBUG
    m_CurrentStatementStartLine = pStart->m_StartLine;
    m_CurrentStatementStartColumn = pStart->m_StartColumn;
    #endif
    
    SetStmtBegLocation(pLambdaExpr->GetStatementLambdaBody(), pStart);
    SetStmtEndLocation(pLambdaExpr->GetStatementLambdaBody(), m_CurrentToken);

    // For multi-line, collect comments trailing the first line of the lambda if there are any.
    if (!isSingleLine)
    {
        VerifyEndOfStatement(errorInConstruct, false /* ignoreColon = false - we need to deal with the ':' if it is there */);
    }
    
    // Sometimes we re-parse the tokens of the multiline lambda, for example, when we are parsing a new expression.
    // Thus, we need to keep track of the EOL tokens and mark them as already having their next lines scanned so that the
    // scanner and parser do not become out of sync.
    if (!isSingleLine)
    {
        if ( m_CurrentToken->m_TokenType == tkEOL )
        {
            Token *pEOLToken = m_CurrentToken;
            GetTokensForNextLine(true);
            pEOLToken->m_EOL.m_NextLineAlreadyScanned = true;
        }
        else 
        {
            // dev10_487997 if we aren't on EOL then there are other statements on the same line as the method def.
            // Which is illegal, e.g. function () : msgbox("illegal")
            // FindEndProc() deals with this for normal methods.  We need to catch the situation for multi-line lambdas
            // here, as we are parsing them.
            ReportSyntaxError( ERRID_MethodBodyNotAtLineStart, m_CurrentToken, errorInConstruct);
            pLambdaBody->HasSyntaxError = true;    
        }
    }

    BackupValue<ParseTree::BlockStatement*> lambdaContext(&m_Context);
    BackupValue<int> lambdaContextIndex(&m_ContextIndex);
    int initialContextIndex = m_ContextIndex;

    // Parse the body of the lambda.
    while (true)
    {
        // The logic for breaking out of parsing a multiline lambda is subtle...
        if (!isSingleLine && EndOfMultilineLambda(m_CurrentToken, pLambdaExpr->GetStatementLambdaBody()))
        {
            break;
        }
        // But the logic for breaking out of parsing a single-line lambda is straightforward:
        // parse one statement, and then break out (at the end of this while loop)

        if ((m_CurrentToken->m_TokenType == tkCATCH) || (m_CurrentToken->m_TokenType == tkFINALLY))
        {
            // If the token is a catch or finally, we need to find out if the catch or finally belongs to the lambda and break if it does not.
            // Otherwise, the lambda context will be popped off the context stack and the catch context will be added.
            // This is a side effect of how we parse catch or finally statements.

            // Unfortunately, this won't catch a handful of cases where the Catch or Finally token is not at the beginning of the line.
            // We'll have slightly less helpful error reporting in those scenarios.
            // e.g.  dim x = 4 : Catch ex
            //       if true then Catch ex
            bool catchOrFinallyBelongsToLambda = CatchOrFinallyBelongsToLambda(pLambdaBody);

            if (!catchOrFinallyBelongsToLambda)
            {
                m_CurrentToken = m_CurrentToken->m_Prev;
                VSASSERT (m_CurrentToken->m_TokenType == tkEOL, "Expected the previous token to be an EOL token.");
                m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = true;
                break;
            }
        }
        
#if DEBUG
        // We assert later that m_Context is still within this lambda context except in the
        // case where there is an existing Try block containing this lambda declaration.
        bool hasContainingHandlerContext = FindContainingHandlerContext();
#endif

        if (isSingleLine)
        {
            // SINGLESUB: This is the key bit of code that parses a single-line sub.

            Token *statementStartToken = m_CurrentToken;
            bool IsAsyncFunction = functionLambda &&  HasFlag(pLambdaExpr->MethodFlags, DECLF_Async);


            if (!parseBody)
            {
                pLambdaBody->HasSyntaxError = true;
            }
            else
            {
                if (IsAsyncFunction)
                {
#if DEBUG
                    m_CurrentStatementStartLine = m_CurrentToken->m_StartLine;
                    m_CurrentStatementStartColumn = m_CurrentToken->m_StartColumn;
#endif

                    ParseReturnOrYieldStatement(m_CurrentToken, errorInConstruct, true);

                }
                else
                {
                    ParseStatementInMethodBody(errorInConstruct);

                    // If it was something like "Sub() 'comment" or "Sub() :" then ParseStatementInMethodBody will have synthesized
                    // an empty statement. It's an error.
                    // Dev10#670455: We'll squiggle the colon if there is one, or the Sub() if there isn't.
                    if (pLambdaBody->Children != NULL && pLambdaBody->Children->Element != NULL &&
                        pLambdaBody->Children->Element->Opcode == ParseTree::Statement::Empty)
                    {
                        ReportSyntaxError(ERRID_SubRequiresSingleStatement,
                                        m_CurrentToken->m_TokenType==tkColon ? m_CurrentToken : pStart,
                                        m_CurrentToken,
                                        errorInConstruct);
                    }

                    // We disallow things like "Sub() Dim x=e1 , y=e2" in case the comma came from "Dim f1 = Sub() ..."
                    // In fact, we disallow variable declarations entirely.
                    if (pLambdaBody->Children != NULL && pLambdaBody->Children->Element != NULL &&
                        pLambdaBody->Children->Element->Opcode == ParseTree::Statement::VariableDeclaration)
                    {
                        ReportSyntaxError(ERRID_SubDisallowsStatement, &pLambdaBody->Children->Element->TextSpan, errorInConstruct);
                    }
                    

                    // We disallow things like "Dim x = Sub() stmt1 : stmt2" because it'd be unclear whether stmt2 followed
                    // stmt1 or if it followed the assignment to x.
                    if (m_CurrentToken->m_TokenType == tkColon)
                    {
                        // Note: normally in SetErrorLocation, that errors reported past the end of a statement
                        // are actually squigglied on the last token of the statement instead (i.e. before the colon). But we want to
                        // report on the colon itself. So we added some ugly special-case code in SetErrorLocation
                        // which says that for this particular error, when it's reported on a colon, we won't do that.
                        ReportSyntaxError(ERRID_SubRequiresSingleStatement, m_CurrentToken, errorInConstruct);
                    }
        
                    // Even though it's an error to have more statements lexically after the colon, we still parse them greedily
                    // to put them inside the lambda. Note that the IDE's prettylister will print all lexically-following statements
                    // inside the body of the lambda. It has to in the case of LineIf. So if ever we decide that
                    // "Dim x = Sub() stmt1 : stmt2" means "(Dim x = Sub() stmt1) : stmt2" without error, then we'll have to
                    // remove the following line, and we'll have to figure out how to change the prettylister.

                    GreedilyParseColonSeparatedStatements(errorInConstruct);
                }

            }

            // And gobble up any trailing comments. They will be associated with the statement in the body of the single-line lambda.
            // (or to the LambdaBodyStatement itself if there were no statement in the body)
            if ( (m_CurrentToken->m_TokenType == tkREM || m_CurrentToken->m_TokenType == tkXMLDocComment) &&
                 !IsAsyncFunction)
            {
                ParseTrailingComments();
            }

            // For a single-line lambda, we can break now: no need to do logic of fetching the next line.
            // We'll pop the context. In a multiline lambda, this popping is done on encountering
            // an "End Sub" or "End Function". But in a single-line, the popping is implicit.
            // (the context was established earlier in this function by EstablishContext)
            if (m_Context == pLambdaBody)
            {
                // The following "EndContext" call has some asserts that the EndSub statement it generates
                // is in the correct location. Those asserts are generally useful (which is why we don't disable
                // them), but they're not useful here, which is why we work around them...
                #if DEBUG
                BackupValue<long> backupStartLine(&m_CurrentStatementStartLine);
                BackupValue<long> backupStartColumn(&m_CurrentStatementStartColumn);
                m_CurrentStatementStartLine = m_CurrentToken->m_Prev->m_StartLine;
                m_CurrentStatementStartColumn = m_CurrentToken->m_Prev->m_StartColumn;
                #endif

                EndContext(m_CurrentToken->m_Prev, ParseTree::Statement::EndSub, NULL);
                // This creates and links an "End Sub" statement whose span goes from 1st arg (m_Prev) to m_CurrentToken (global variable)
                // and it pops the context
            }
            else
            {
                // If something else has closed our context (e.g. "Sub() Select", since
                // the select statement pops context), then we'll end up here.
                ReportSyntaxError(ERRID_SubRequiresSingleStatement, statementStartToken, errorInConstruct);
                pLambdaBody->HasSyntaxError = true;    // Dev10 678271.  If the context is unexpected then the lambda is in error.  Mark it 
                // as bad so the prettylister doesn't try to complete incomplete blocks (like throwing in end if), etc.
            }
            break;
        }
        else
        {
            ParseLine(&Parser::ParseStatementInMethodBody);
        }

        if(m_Context == pLambdaBody->GetParent() || m_Context == pLambdaBody->GetRawParent())
        {
            // We lost the lambda as a context.
            // Break because we are no longer parsing the body of the multiline lambda.
            break;
        }
        else if (initialContextIndex > m_ContextIndex)
        {
            // This can happen when we hit a Catch/Finally statement that was not the first token of a line.
            // This only occurs when there was a containing Try context.
            VSASSERT(hasContainingHandlerContext, "How did we drop so many contexts?");
            break;
        }

        // In conditional compilation scenarios, m_CurrentToken may not be an EOL token.
        VSASSERT( (m_CurrentToken->m_TokenType == tkEOL) || 
                  (m_CurrentToken->m_TokenType == tkEOF), 
                   "Why did we not finish with all the tokens before getting a new line?");
        
        // Sometimes we re-parse the tokens of the multiline lambda, for example, when we are parsing a new expression.
        // Thus, we need to keep track of the EOL tokens and mark them as already having their next lines scanned so that the
        // scanner and parser do not become out of sync.
        Token *pEOLToken = m_CurrentToken;

        // Get the next line
        GetTokensForNextLine(true);

        if (pEOLToken->m_TokenType == tkEOL)
        {
            pEOLToken->m_EOL.m_NextLineAlreadyScanned = true;
        }
        
        pEOLToken = NULL;
    }

    if (!isSingleLine && (m_CurrentToken->m_TokenType == tkSUB || m_CurrentToken->m_TokenType == tkFUNCTION))
    {
        // In a scenario like:
        //
        // Class c1
        //    Dim x = Sub()  (1)
        //                   (2)
        //    Sub Main()     (3)
        //    End Sub
        //
        // We exit the above while loop when m_CurrentToken points to the
        // tkSUB token from line (3). We want to reset m_CurrentToken to the
        // tkEOL token on line (2) because the multiline lambda should not consume
        // line (3).
        m_CurrentToken = m_CurrentToken->m_Prev;

        if (m_CurrentToken->m_TokenType == tkEOL)
        {
            m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = true;
        }
    }

    VSASSERT(pLambdaExpr->GetStatementLambdaBody() == pLambdaBody, "unexpected: I thought that pLambdaBody was always the same as GetStatementLambdaBody");
    // if the above assert is true, then it's inconsistent that the following code uses both,
    // and they should be unified.

    if (pLambdaExpr->GetStatementLambdaBody()->Children)
    {
        pLambdaExpr->GetStatementLambdaBody()->BodyTextSpan.m_lBegColumn = pStart->m_StartColumn;
        pLambdaExpr->GetStatementLambdaBody()->BodyTextSpan.m_lBegLine = pStart->m_StartLine;
    }

    if (m_CurrentToken->m_TokenType == tkEOF)
    {
        VSASSERT(isSingleLine || (pLambdaBody->BodyTextSpan.m_lEndLine==0 && pLambdaBody->BodyTextSpan.m_lEndColumn==0), "unexpected: if we got to a tkEOF, then how did BodyTextSpan.End get set?");

        // If we reached an EOF, there is an error in the file and we assume the lambda
        // body consumes the entire file.
        pLambdaBody->BodyTextSpan.m_lEndLine = m_CurrentToken->m_StartLine;
        pLambdaBody->BodyTextSpan.m_lEndColumn = m_CurrentToken->m_StartColumn; // dev10_647539  Don't leave the BodyTExtSpan.m_lEndColumn hanging at -1
    }

    // Dev10#505591: normally the body's end-position will have been set when it encountered
    // the appropriate End statement through EndMatchesContext elsewhere. But in syntax-error cases we
    // might have aborted lambda processing without having encountered an End. We'll catch all
    // remaining cases here. One such case is Try : Dim x = Sub() : Catch. Another case is EOF, dealt with above.
    // And another case is a single-line sub lambda, which of course won't have an End Sub
    if (isSingleLine)
    {
        // We don't set the end of BodyTextSpan here. That's because it's already calculated after
        // we return by our caller, in the form of LambdaExpression.TextSpan, which is a complicated
        // calculation involving line continuation end state.
    }
    else if (pLambdaExpr->GetStatementLambdaBody()->BodyTextSpan.m_lEndLine==0)
    {
        pLambdaExpr->GetStatementLambdaBody()->BodyTextSpan.m_lEndLine = m_CurrentToken->m_StartLine;
        pLambdaExpr->GetStatementLambdaBody()->BodyTextSpan.m_lEndColumn = m_CurrentToken->m_StartColumn;
    }

    // Dev10 #792852 If we are still in context of the Lamda's body, the Lambda and maybe some blocks are not properly terminated.
    if (!isSingleLine && initialContextIndex <= m_ContextIndex) 
    {
        AssertIfFalse(initialContextIndex != m_ContextIndex || m_Context == pLambdaBody);

        // Handle unterminated blocks within the Lambda.
        RecoverFromMissingEnds(pLambdaBody); 

        AssertIfFalse(m_Context == pLambdaBody);
    }

    if (m_Context == pLambdaBody)
    {
        bool ErrorInBlockEnd = false;

        // Note: this code will never be entered for a single-line sub. That's because single-line subs
        // have already dealt with their expected case "m_Context==pLambdaBody" by popping context.
        // However, if for some reason they do end up here, well, let's recover as best we can.
        VSASSERT(!isSingleLine, "how odd... single-line statements should never result in m_Context==pLambdaBody");
       
        ReportSyntaxError(
            (m_pStatementLambdaBody->pOwningLambdaExpression->MethodFlags & DECLF_Function) ?
                ERRID_MultilineLambdaMissingFunction : 
                ERRID_MultilineLambdaMissingSub, 
            &m_Context->TextSpan, 
            ErrorInBlockEnd);
           
        // Pop the LambdaBody context.
        PopContext(pLambdaExpr->GetStatementLambdaBody()->TerminatingConstruct, m_CurrentToken);    
        // Pop the hidden block.
        PopContext(false, m_CurrentToken);
    }

    // We don't set m_pStatementLambdaBody = NULL because it is going to be restored to whatever it was on entry to this function 
    // by the backupLambdaBody variable above.

    return pLambdaExpr;
}

/*****************************************************************************************
;EndOfMultilineLambda

Determines whether we are looking at text that indicates we are done processing the
body of a multi-line lambda (the End Sub may be missing and we may now be parsing into another
method or who knows what)
It is somewhat subtle trying to recognize whether the text we are hitting represents something
that clearly falls outside of the lambda body. See the comments in the function for details.
******************************************************************************************/
bool // true - we are parsing outside of the lambda body, false - we think we are still inside a multi-line lambda body
Parser::EndOfMultilineLambda( _In_ Token *pCurrentLineToken, _In_ ParseTree::LambdaBodyStatement *pLambdaBody )
{
    // We do not want to return true if the context is a lambda body but NOT the lambda body of the current call.
    //
    // For example:
    //
    // foo( sub()                     '1
    //           dim x = Sub()        '2
    //                       End Sub  '3
    //        End Sub)                '4
    //
    // We do not want to return true while we are parsing the lambda on line 1 when we reach the End Sub for the lambda on line 3.
    // This scenario is a problem because we parse multiline lambdas in variable declaration statements differently than multiline lambdas
    // in call statements. 

    /* Dev10 # 799342 We can detect that we are done processing the lambda body if we encounter things like:
           private sub Kermit()
           overloads function Grover()
           readonly property Bob()
     since hitting something like that clearly indicates that a new method is being defined and our current lambda is missing an end sub/function.
     The test below isn't exhaustive (it doesn't check for leading attributes, multiple specifiers, etc) but it is sufficient to provide
     a good intellisense experience.
    
     

*/
     
    tokens PotentialEndOfLambdaToken = pCurrentLineToken->m_TokenType;

    bool stillEncounteringSpecifiers = true;
    while ( stillEncounteringSpecifiers ) // handle multiple specifiers
    {
        switch ( PotentialEndOfLambdaToken )
        {
            case tkPRIVATE:
            case tkPROTECTED:
            case tkPUBLIC:
            case tkFRIEND:

            case tkNOTOVERRIDABLE:
            case tkOVERRIDABLE:
            case tkMUSTOVERRIDE:

            case tkSTATIC:
            case tkSHARED:
            case tkSHADOWS:
            case tkWITHEVENTS:

            case tkOVERLOADS:
            case tkOVERRIDES:

            case tkWIDENING:
            case tkNARROWING:

            case tkREADONLY:
            case tkWRITEONLY:
            {
                // grab the token after the private/public, etc. to see if it is a token that means we should
                // stop processing the lambda body.  For instance, even though we ran into Private so far, we
                // should only stop if we have Private Sub|Function|Operator|Property.  If we hit Private x as integer,
                // for instance, we wouldn't want to conclude that we were done processing the lambda body.  Yes, it
                // is true that you can't have private x as integer in a lambda body, but we get better errors for the
                // user if we keep going in that instance as if we are still in the lambda body.
                pCurrentLineToken = pCurrentLineToken->m_Next;
                VSASSERT( pCurrentLineToken, "There is always a next token since the previous token wasn't tkNone or tkEOF" );
                PotentialEndOfLambdaToken = pCurrentLineToken->m_TokenType;
                break;
            }
            default: // done looking at specifiers
                stillEncounteringSpecifiers = false;
                break;
        }
    }

    bool TokenIndicatesEndOfMultiLineLambda = false;
    if ( PotentialEndOfLambdaToken == tkSUB ||
         PotentialEndOfLambdaToken == tkFUNCTION ||
         PotentialEndOfLambdaToken == tkOPERATOR ||
         PotentialEndOfLambdaToken == tkPROPERTY ||
         ( !pCurrentLineToken->IsEndOfParse() && BeginsEvent( pCurrentLineToken )))
    {
        TokenIndicatesEndOfMultiLineLambda = true;
    }

    if ( (m_Context->Opcode != ParseTree::Statement::LambdaBody || m_Context == pLambdaBody) &&
         (pCurrentLineToken->IsEndOfParse() || 
          TokenIndicatesEndOfMultiLineLambda
         )
       )
    {
        return true;
    }
    return false;
}

bool
Parser::CatchOrFinallyBelongsToLambda( _In_ ParseTree::LambdaBodyStatement *pLambdaBody)
{
    ParseTree::BlockStatement *pHandlerContext = FindContainingHandlerContext();

    // If there is no pHandlerContext this is a Catch floating around without a Try. Thus, it DOES belong to the lambda,
    // but it will be reported as an error.
    return pHandlerContext ? ConfirmParentChildRelationship(pLambdaBody, pHandlerContext) : true;
}

/*********************************************************************
*
* Function:
*     Parser::ParseTerm
*
* Purpose:
*     Parse a term of a generic expression, for example: constants, variables,
*     'Me', built-in and user-defined function invocations, and so on.
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseTerm
(
    _Inout_ bool &ErrorInConstruct,
    bool BailIfFirstTokenRejected       // bail (return NULL) if the first token isn't a valid expression-starter, rather than reporting an error or setting ErrorInConstruct
)
{
    // Note: this function will only ever return NULL if the flag "BailIfFirstTokenIsRejected" is set,
    // and if the first token isn't a valid way to start an expression. In all other error scenarios
    // it returns a "bad expression".
    VSASSERT(!m_EvaluatingConditionCompilationExpression ||
             StartsValidConditionalCompilationExpr(m_CurrentToken), "Conditional compilation expression parsing confused!!!");

    ParseTree::Expression *Term = NULL;
    Token *Start = m_CurrentToken;

    // Resync points are delimiters.

    switch (Start->m_TokenType)
    {
        case tkID:
        {
            tokens keyWord = IdentifierAsKeyword(Start);

            // See if this is a beginning of a query
            if (keyWord == tkAGGREGATE ||keyWord == tkFROM)
            {
                bool ParsedQuery = false;
                Term = ParsePotentialQuery( ParsedQuery, ErrorInConstruct );
                if ( ParsedQuery )
                {
                    break;
                }
            }

            Term = ParseIdentifierExpressionAllowingKeyword(ErrorInConstruct);
            
            if (!m_EvaluatingConditionCompilationExpression &&
                BeginsGenericWithPossiblyMissingOf(m_CurrentToken))
            {
                Term = ParseGenericQualifier(Start, Term, ErrorInConstruct);
            }
            break;
        }

        case tkBang:
        {
            Term = ParseQualifiedExpr(Start, NULL, ErrorInConstruct);
            break;
        }

        case tkDot:
        {
            Term = ParseQualifiedExpr(Start, NULL, ErrorInConstruct);
            if (BeginsGenericWithPossiblyMissingOf(m_CurrentToken))
            {
                Term = ParseGenericQualifier(Start, Term, ErrorInConstruct);
            }
            break;
        }

        case tkGLOBAL:
        case tkMYBASE:
        case tkMYCLASS:
        {
            // NB. GetNextToken has the side-effect of advancing m_CurrentToken.
            if (GetNextToken()->m_TokenType == tkDot)
            {
                Term = new(m_TreeStorage) ParseTree::Expression;
                Term->Opcode =
                    Start->m_TokenType == tkMYBASE ?
                    ParseTree::Expression::MyBase :
                        ( Start->m_TokenType == tkMYCLASS ?
                            ParseTree::Expression::MyClass :
                            ParseTree::Expression::GlobalNameSpace
                        );
            }
            else
            {
                // Dev10#519742: MyClass/MyBase/Global on their own are bad parse-tree nodes.
                // If we don't mark them bad now, then InterpretExpression will try to bind them later on
                // (which would be incorrect).
                Term = ExpressionSyntaxError();
                ReportSyntaxError(
                    Start->m_TokenType == tkMYBASE ?
                        ERRID_ExpectedDotAfterMyBase :
                        ( Start->m_TokenType == tkMYCLASS ?
                            ERRID_ExpectedDotAfterMyClass :
                            ERRID_ExpectedDotAfterGlobalNameSpace
                        ),
                    Start,
                    ErrorInConstruct);
            }

            SetExpressionLocation(Term, Start, Start->m_Next, NULL);
            break;
        }
        case tkME:
        {
            Term = new(m_TreeStorage) ParseTree::Expression;
            Term->Opcode = ParseTree::Expression::Me;
            GetNextToken();
            SetExpressionLocation(Term, Start, m_CurrentToken, NULL);

            break;
        }

        case tkLParen:
            Term = ParseParenthesizedExpression(ErrorInConstruct);
            break;

        case tkLT:
            // Xml Literals
            // 1. single element "<" element ">"
            // 2. xml document  "<?xml ...?>
            // 3. xml pi
            // 4. xml cdata <![CDATA[
            // 5. xml comment <!--

            if (m_CurrentToken->m_State.IsXmlState() ||
                /* Dev10_427764 : Allow an implicit line continuation for XML after '(', e.g. foo(
                                                                                                  <xml></xml>)  */
                ( m_CurrentToken->m_State.IsVBImplicitLineContinuationState() && 
                  m_CurrentToken->m_State.TokenTypeBeforeLineContinuation() == tkLParen ))
            {
                Term = ParseXmlExpression(ErrorInConstruct);
            }
            else
            {
                ReportSyntaxError(ERRID_XmlRequiresParens, m_CurrentToken, ErrorInConstruct);

                Term = ExpressionSyntaxError();
                SetExpressionLocation(Term, m_CurrentToken, m_CurrentToken, NULL);
            }

            break;

        case tkIntCon:
            Term = ParseIntLiteral();
            break;

        case tkCharCon:
            Term = ParseCharLiteral();
            break;

        case tkDecCon:
            Term = ParseDecLiteral();
            break;

        case tkFltCon:
            Term = ParseFltLiteral();
            break;

        case tkDateCon:
            Term = ParseDateLiteral();
            break;

        case tkStrCon:
            Term = ParseStringLiteral(ErrorInConstruct);
            break;

        case tkTRUE:
            Term = CreateBoolConst(true);
            GetNextToken();
            SetExpressionLocation(Term, Start, m_CurrentToken, NULL);
            break;

        case tkFALSE:
            Term = CreateBoolConst(false);
            GetNextToken();
            SetExpressionLocation(Term, Start, m_CurrentToken, NULL);
            break;

        case tkNOTHING:
            Term = new(m_TreeStorage) ParseTree::Expression;
            Term->Opcode = ParseTree::Expression::Nothing;
            GetNextToken();
            SetExpressionLocation(Term, Start, m_CurrentToken, NULL);
            break;

        case tkTYPEOF:

            Term = ParseTypeOf(ErrorInConstruct);
            break;

        case tkGETTYPE:

            Term = ParseGetType(ErrorInConstruct);
            break;

        case tkGETXMLNAMESPACE:

            Term = ParseGetXmlNamespace(ErrorInConstruct);
            break;

        case tkNEW:

            Term = ParseNewExpression(ErrorInConstruct);
            break;

        case tkCBOOL:
        case tkCDATE:
        case tkCDBL:
        case tkCSBYTE:
        case tkCBYTE:
        case tkCCHAR:
        case tkCSHORT:
        case tkCUSHORT:
        case tkCINT:
        case tkCUINT:
        case tkCLNG:
        case tkCULNG:
        case tkCSNG:
        case tkCSTR:
        case tkCDEC:
        case tkCOBJ:

            Term = ParseCastExpression(ErrorInConstruct);
            break;

        case tkCTYPE:
        case tkDIRECTCAST:
        case tkTRYCAST:

            Term = ParseCType(ErrorInConstruct);
            break;


        case tkIF:
        {
            Token *IfStart = m_CurrentToken;

            Term = new(m_TreeStorage) ParseTree::IIfExpression();
            Term->Opcode = ParseTree::Expression::IIf;

            GetNextToken();

            if (tkLParen == m_CurrentToken->m_TokenType)
            {
                SetPunctuator(&Term->AsIIf()->FirstPunctuator, IfStart, m_CurrentToken);
                Term->AsIIf()->Arguments = ParseParenthesizedArguments(ErrorInConstruct);
            }
            else
            {
                HandleUnexpectedToken(tkLParen, ErrorInConstruct);
            }

            SetLocation(&Term->TextSpan, IfStart, m_CurrentToken);

            break;
        }

        case tkSHORT:
        case tkUSHORT:
        case tkINTEGER:
        case tkUINTEGER:
        case tkLONG:
        case tkULONG:
        case tkDECIMAL:
        case tkSINGLE:
        case tkDOUBLE:
        case tkSBYTE:
        case tkBYTE:
        case tkBOOLEAN:
        case tkCHAR:
        case tkDATE:
        case tkSTRING:
        case tkVARIANT:
        case tkOBJECT:
        {
            ParseTree::TypeReferenceExpression *TypeReference = new(m_TreeStorage) ParseTree::TypeReferenceExpression;
            TypeReference->Opcode = ParseTree::Expression::TypeReference;
            TypeReference->ReferencedType = ParseTypeName(ErrorInConstruct);
            SetExpressionLocation(TypeReference, Start, m_CurrentToken, NULL);

            Term = TypeReference;

            if (m_CurrentToken->m_TokenType == tkDot)
            {
                Term = ParseQualifiedExpr(Start, Term, ErrorInConstruct);
            }
            else
            {
                HandleUnexpectedToken(tkDot, ErrorInConstruct);
            }

            break;
        }

        case tkLBrace:
        {
            ParseTree::ArrayInitializerExpression *ArrayInit = new(m_TreeStorage) ParseTree::ArrayInitializerExpression;
            ArrayInit->Opcode = ParseTree::Expression::ArrayInitializer;
            ArrayInit->Elements =
                ParseInitializerList(
                    ErrorInConstruct,
                    true,   // allow expressions
                    false); // disallow assignments

            SetExpressionLocation(ArrayInit, Start, m_CurrentToken, NULL);
            Term = ArrayInit;
            break;
        }

        case tkITERATOR:
        case tkASYNC:
            __fallthrough;

        case tkSUB:
        case tkFUNCTION:
        {
            BackupValue<MethodDeclKind> backupMethodDeclKind(m_InputStream->GetMethodDeclKind());
            bool isAsync = false;
            
            if (!m_SeenLambdaExpression)    // if it's the first one we've seen (easier to set bpt)
            {
                m_SeenLambdaExpression = true;
            }

            if (m_CurrentToken->m_TokenType == tkASYNC)
            {
                m_InputStream->SetMethodDeclKind(AsyncKind);
                isAsync = true;
            }
            else if (m_CurrentToken->m_TokenType == tkITERATOR)
            {
                m_InputStream->SetMethodDeclKind(IteratorKind);
            }
            else
            {
                m_InputStream->SetMethodDeclKind(NormalKind);
            }

        
            ParseTree::SpecifierList *Specifiers = NULL;
            if (m_CurrentToken->m_TokenType == tkASYNC || m_CurrentToken->m_TokenType == tkITERATOR)
            {


                Specifiers = ParseSpecifiers(ErrorInConstruct);

                if (m_CurrentToken == NULL || (m_CurrentToken->m_TokenType != tkSUB && m_CurrentToken->m_TokenType != tkFUNCTION))
                {
                    HandleUnexpectedToken(tkFUNCTION, ErrorInConstruct);
                    Term = ExpressionSyntaxError();
                    break;
                }
            }

            bool functionLambda = (m_CurrentToken->m_TokenType == tkFUNCTION);
            bool isMultiLineLambda = IsMultiLineLambda();
            Token *pMethodKindToken = m_CurrentToken;

            // SINGLESUB: we used to AssertLanguageFeature here on "Sub() ..." (whether it was parseable or not).
            // But now parseable "Sub() ..." falls into the ParseStatementLambda branch below, where it always
            // AssertsLanguageFeature, so we can omit it from here. One difference is that we no longer AssertLanguageFeature
            // on unparseable "Sub() ...". But that's no real loss.

            GetNextToken();

            Token *LeftParen = NULL;
            Token *RightParen = NULL;

            ParseTree::ParameterList * Parameters = NULL;

            bool ErrorEncounteredParsingParams = false;
            if (m_CurrentToken->m_TokenType == tkLParen)
            {
                RejectGenericParametersForMemberDecl(ErrorEncounteredParsingParams);

                if (m_CurrentToken->m_TokenType == tkLParen)
                {
                    Parameters = ParseParameters(ErrorEncounteredParsingParams, LeftParen, RightParen);
                }
            }
            else
            {
                HandleUnexpectedToken(tkLParen, ErrorEncounteredParsingParams);
            }
            ErrorInConstruct |= ErrorEncounteredParsingParams;

            if (isMultiLineLambda || !functionLambda || isAsync)
            {
                // The routine "ParseStatementLambda" is used to parse four things:
                // "Sub() : ... : End Sub"
                // "Function() : ... : End Function"
                // "Sub() stmt" (single-line lambda)
                // "Async Function() expr"
                // also "Sub( |" i.e. an incomplete Sub. Dev10#687565: in this case, don't parse the body
                bool parseBody = !ErrorEncounteredParsingParams || isMultiLineLambda;
                Term = ParseStatementLambda(Parameters, Start, functionLambda, !isMultiLineLambda, parseBody, ErrorInConstruct);
                AssertLanguageFeature(FEATUREID_StatementLambdas, pMethodKindToken);
                VSASSERT(Term->AsLambda()->IsStatementLambda || Term->AsLambda()->IsAsync(), "Expected to have gotten a statement lambda");
            }
            else
            {
                VSASSERT( functionLambda, "Expecting a function lambda in this block");
                Term = new (m_TreeStorage) ParseTree::LambdaExpression;
                Term->AsLambda()->Opcode = ParseTree::Expression::Lambda;
                Term->AsLambda()->SetSingleLineLambdaExpression(ErrorEncounteredParsingParams ? ExpressionSyntaxError() : ParseExpression(ErrorInConstruct));
                Term->AsLambda()->Parameters = Parameters;
                Term->AsLambda()->AllowRelaxationSemantics = true;
                Term->AsLambda()->MethodFlags |= DECLF_Function;
                VSASSERT(!Term->AsLambda()->IsStatementLambda, "Expected to have gotten an expression lambda");
            }
            // Note: "Sub() <unparseable>" actually gets stored as an expression lambda because otherwise
            // it'd be too much hassle creating an empty block in the ParseStatementLambda function.
            // The intellisense keys off this fact to know to pop up intellisense for statement completion
            // when you've only just typed "Sub() |"

            SetExpressionLocation(Term, Start, m_CurrentToken, Start);

            // Process specifiers and report errors for invalid specifiers
            Term->AsLambda()->Specifiers = Specifiers;
            if (Specifiers->HasSpecifier(ParseTree::Specifier::Async))
            {
                Term->AsLambda()->MethodFlags |= DECLF_Async;
            }
            if (Specifiers->HasSpecifier(ParseTree::Specifier::Iterator))
            {
                Term->AsLambda()->MethodFlags |= DECLF_Iterator;
            }
            ParseTree::SpecifierList *invalidSpecifiers = Specifiers->HasSpecifier(~(ParseTree::Specifier::Async | ParseTree::Specifier::Iterator));
            if (invalidSpecifiers)
            {
                ReportSyntaxError(ERRID_InvalidLambdaModifier, &invalidSpecifiers->TextSpan, ErrorInConstruct);
            }
            
            if (!isMultiLineLambda && !functionLambda && Term->AsLambda()->IsStatementLambda)
            {
                // This is where we calculate the end of the BodyTextSpan of a single-line sub lambda.
                // The BodyTextSpan of multi-line lambdas could be calculated earlier, in ParseStatementLambda,
                // because they are the span "<% Sub() : ... : %> End Sub", i.e. they end immediately before the
                // token following the last statement. That's so the debugger can know where the "End Sub" starts.
                // But in a single-line sub they are instead the span "<% Sub() stmt %>", i.e. end immediately after
                // the last token of the last statement. That's calculated in the SetExpressionLocation() call immediately
                // above. Hence we why have to do this calculation for single-line subs here and now, and not inside
                // ParseStatementLambda.
                Term->AsLambda()->GetStatementLambdaBody()->BodyTextSpan.m_lEndLine = Term->TextSpan.m_lEndLine;
                Term->AsLambda()->GetStatementLambdaBody()->BodyTextSpan.m_lEndColumn = Term->TextSpan.m_lEndColumn;
            }

            AssertIfFalse(!Term->AsLambda()->IsStatementLambda || Term->AsLambda()->GetStatementLambdaBody()->BodyTextSpan.m_lEndColumn >= 0);
            AssertIfFalse(Term->AsLambda()->IsStatementLambda || !Term->AsLambda()->GetSingleLineLambdaExpression() || Term->AsLambda()->GetSingleLineLambdaExpression()->TextSpan.m_lEndColumn >= 0);
            AssertIfFalse(Term->TextSpan.m_lEndColumn >= 0);

            if (LeftParen)
            {
                SetPunctuator(&(Term->AsLambda()->LeftParen), Start, LeftParen);
            }

            if (RightParen)
            {
                SetPunctuator(&(Term->AsLambda()->RightParen), Start, RightParen);
            }

            if (pMethodKindToken)
            {
                SetPunctuator(&(Term->AsLambda()->MethodKind), Start, pMethodKindToken);
            }


            break;
        }

        default:
            if (BailIfFirstTokenRejected)
            {
                return NULL;
            }

            ReportSyntaxError(ERRID_ExpectedExpression, m_CurrentToken, ErrorInConstruct);

            Term = ExpressionSyntaxError();
            SetExpressionLocation(Term, m_CurrentToken, m_CurrentToken, NULL);
            break;
    }


    // Complex expressions such as "." or "!" qualified, etc are not allowed cond comp expressions.
    //
    if (!m_EvaluatingConditionCompilationExpression)
    {
        // Valid suffixes are ".", "!", and "(". Everything else is considered
        // to end the term.

        while (true)
        {
            Token *Next = m_CurrentToken;

            // Dev10#670442 - you can't apply invocation parentheses directly to a "----" single-line sub lambda,
            // nor DotQualified/BangDictionaryLookup
            bool isAfterSingleLineSub = (Term->Opcode == ParseTree::Expression::Lambda && Term->AsLambda()->IsSingleLine &&
                                      Term->AsLambda()->IsStatementLambda && !Term->AsLambda()->IsFunction() );

            if (Next->m_TokenType == tkDot)
            {
                if (isAfterSingleLineSub)
                {
                    ReportSyntaxError(ERRID_SubRequiresParenthesesDot, &Term->TextSpan, ErrorInConstruct);
                }
                Term = ParseQualifiedExpr(Start, Term, ErrorInConstruct);
                if (BeginsGenericWithPossiblyMissingOf(m_CurrentToken))
                {
                    Term = ParseGenericQualifier(Start, Term, ErrorInConstruct);
                }
            }
            else if (Next->m_TokenType == tkBang)
            {
                if (isAfterSingleLineSub)
                {
                    ReportSyntaxError(ERRID_SubRequiresParenthesesBang, &Term->TextSpan, ErrorInConstruct);
                }
                Term = ParseQualifiedExpr(Start, Term, ErrorInConstruct);
            }
            else if (Next->m_TokenType == tkLParen)
            {
                if (isAfterSingleLineSub)
                {
                    ReportSyntaxError(ERRID_SubRequiresParenthesesLParen, &Term->TextSpan, ErrorInConstruct);
                }
                Term = ParseParenthesizedQualifier(Start, Term, ErrorInConstruct);
            }
            else
            {
                // We're done with the term.
                break;
            }
        }
    }

    if (m_CurrentToken && m_CurrentToken->m_TokenType == tkNullable)
    {
        ReportSyntaxError(ERRID_NullableCharNotSupported, m_CurrentToken, ErrorInConstruct);
        GetNextToken();
    }
    return Term;
}

ParseTree::Expression *
Parser::ParseXmlExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *StartToken = m_CurrentToken;
    ParseTree::Expression *Result;

    GetNextXmlToken();
    if (m_CurrentToken->m_TokenType == tkXmlBeginPI &&
        m_CurrentToken->m_Next &&
        m_CurrentToken->m_Next->m_TokenType == tkXmlNCName &&
        m_CurrentToken->m_Next->m_State.m_IsDocument)
    {
        // Parse Xml Document
        Result = ParseXmlDocument(ErrorInConstruct);
    }
    else
    {
        // Parse all other Xml
        Result = ParseXmlMarkup(NULL, false, ErrorInConstruct);
    }

    if (m_CurrentToken->m_TokenType == tkXmlAbort)
    {
        m_InputStream->AbandonTokens(m_CurrentToken->m_Next);
        m_CurrentToken->m_TokenType = tkEOL;
        m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = false;
    }

    ParseTree::ExpressionList *XmlRoots = new (m_TreeStorage) ParseTree::ExpressionList;
    XmlRoots->Element = Result;
    XmlRoots->Next = NULL;
    *m_XmlRootsEnd = XmlRoots;
    m_XmlRootsEnd = &XmlRoots->Next;

    return Result;
}

bool CheckXmlWhiteSpace(ParseTree::ExpressionList *Content)
 {
     ParseTree::ExpressionList *WhiteSpace = NULL;
     ParseTree::XmlCharDataExpression *CharData;
     bool IsSignificant = false;
     bool HasSignificantContent = false;

     for (;Content; Content = Content->Next)
     {
         ParseTree::Expression *Expr = Content->Element;

         switch (Expr->Opcode)
         {
         case ParseTree::Expression::XmlCharData:
         case ParseTree::Expression::XmlReference:
             CharData = Expr->AsXmlCharData();
             if (WhiteSpace == NULL)
             {
                 if (CharData->IsWhitespace)
                 {
                    WhiteSpace = Content;
                 }
             }

             if (!CharData->IsWhitespace)
             {
                 IsSignificant = true;
                 HasSignificantContent  = true;
             }
             break;

         case ParseTree::Expression::XmlElement:
         case ParseTree::Expression::XmlEmbedded:
         case ParseTree::Expression::XmlPI:
         case ParseTree::Expression::XmlComment:
         case ParseTree::Expression::XmlCData:
             if (!IsSignificant)
             {
                 for(;WhiteSpace && WhiteSpace != Content; WhiteSpace = WhiteSpace->Next)
                 {
                     CharData = WhiteSpace->Element->AsXmlCharData();
                     CharData->IsSignificant = false;
                     CharData->IsMovable =true;
                 }
             }
             WhiteSpace = NULL;
             IsSignificant = false;
             HasSignificantContent = true;
             break;

         default:
             IsSignificant = true;
             HasSignificantContent = true;
             break;
         }
     }
     if (!IsSignificant)
     {
         for(;WhiteSpace && WhiteSpace != Content; WhiteSpace = WhiteSpace->Next)
         {
             CharData = WhiteSpace->Element->AsXmlCharData();
             CharData->IsSignificant = false;
             CharData->IsMovable =true;
         }
     }

     return HasSignificantContent;
 }


ParseTree::Expression *
Parser::ParseXmlDocument
(
     _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken->m_Prev; // The current token is '?xml' so back up to the '<' to get the full span.

    GetNextXmlToken(); // Current token is on ? so get the next token.
    GetNextXmlToken(); // Skip xml token

    // Read version and encoding
    ParseTree::XmlDocumentExpression *Result = ParseXmlDecl(ErrorInConstruct);

    // Read PI's and comments

    ParseTree::ExpressionList **Prev = ParseXmlMisc(&Result->Content, true, ErrorInConstruct);

    Token *ContentStart = m_CurrentToken;

    if (VerifyExpectedXmlToken(tkLT, ErrorInConstruct))
    {
        // Get root element
        // This is either a single xml expression hole or an xml element
        ParseTree::ExpressionList *Current = new(m_TreeStorage) ParseTree::ExpressionList;

        if (m_CurrentToken->m_TokenType == tkXmlBeginEmbedded)
            Current->Element = ParseXmlEmbedded(true, ErrorInConstruct);
        else
            Current->Element = ParseXmlElement(NULL, ErrorInConstruct);

        SetLocation(
           &Current->TextSpan,
           ContentStart,
           m_CurrentToken);
        *Prev = Current;
        Prev = &Current->Next;

        // More PI's and comments
        ParseXmlMisc(Prev, false, ErrorInConstruct);
    }

    if (Result->Content)
        FixupListEndLocations(Result->Content);

    SetLocation(
       &Result->TextSpan,
       Start,
       m_CurrentToken);

    CheckXmlWhiteSpace(Result->AsXml()->Content);

    return Result;
}

ParseTree::XmlDocumentExpression *
Parser::ParseXmlDecl
(
     _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::XmlDocumentExpression *Result = new(m_TreeStorage) ParseTree::XmlDocumentExpression;
    Result->Opcode = ParseTree::Expression::XmlDocument;

    Result->Attributes = ParseXmlAttributes(true, ErrorInConstruct);

    VerifyExpectedXmlToken(tkXmlEndPI, ErrorInConstruct);

    // Span is only to the end of the Xml declaration
    SetLocation(
       &Result->DeclTextSpan,
       Start,
       m_CurrentToken);

    return Result;
}


ParseTree::ExpressionList **
Parser::ParseXmlMisc
(
    _Inout_ ParseTree::ExpressionList **Prev,
    bool IsProlog,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionList **Content = Prev;
    ParseTree::Expression *Result;

    Token *Start = m_CurrentToken;

    while (true)
    {
        switch (m_CurrentToken->m_TokenType)
        {
        case tkLT:
            if (!m_CurrentToken->m_Next)
            {
                goto FixupContentList;
            }

            switch (m_CurrentToken->m_Next->m_TokenType)
            {
                case tkBang:
                    GetNextXmlToken();
                    if (m_CurrentToken->m_Next &&
                        m_CurrentToken->m_Next->m_TokenType == tkXmlNCName &&
                        m_CurrentToken->m_Next->m_Id.m_Spelling == STRING_CONST2(m_pStringPool, DOCTYPE))
                    {
                        GetNextXmlToken();
                        ReportSyntaxError(ERRID_DTDNotSupported, m_CurrentToken, ErrorInConstruct);
                    }
                    else
                    {
                        HandleUnexpectedToken(tkXmlBeginComment, ErrorInConstruct);
                    }
                    Result = ExpressionSyntaxError();
                    ResyncAt(1, tkGT);
                    GetNextXmlToken();
                    break;

                case tkXmlBeginComment:
                    GetNextXmlToken();
                    Result = ParseXmlComment(ErrorInConstruct);
                    break;

                case tkXmlBeginPI:
                    GetNextXmlToken();
                    Result = ParseXmlPI(ErrorInConstruct);
                    break;

                default:
                   goto FixupContentList;
            }

            break;

        case tkXmlWhiteSpace:
            if (IsProlog ||
                (m_CurrentToken->m_Next &&
                (m_CurrentToken->m_Next->m_TokenType == tkLT ||
                    m_CurrentToken->m_Next->m_TokenType == tkXmlWhiteSpace)))
            {
                Result = MakeXmlStringLiteral();
            }
            else
            {
                goto FixupContentList;
            }
            break;

        default:
            goto FixupContentList;
        }

        ParseTree::ExpressionList *Current = new(m_TreeStorage) ParseTree::ExpressionList;
        SetLocation(
           &Current->TextSpan,
           Start,
           m_CurrentToken);
        Current->Element = Result;
        *Prev = Current;
        Prev = &Current->Next;
    }

FixupContentList:
    if (*Content)
    {
        FixupListEndLocations(*Content);
    }

    return Prev;
}


ParseTree::Expression *
Parser::ParseXmlMarkup
(
     _In_opt_ ParseTree::XmlElementExpression *Parent,
     bool AllowEmbeddedExpression,
     _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Result;

    switch (m_CurrentToken->m_TokenType)
    {
    case tkXmlBeginCData:
        Result = ParseXmlCData(ErrorInConstruct);
        break;

    case tkXmlBeginComment:
        Result = ParseXmlComment(ErrorInConstruct);
        break;

    case tkXmlBeginPI:
        Result = ParseXmlPI(ErrorInConstruct);
        break;

    case tkXmlBeginEmbedded:
        Result = ParseXmlEmbedded(AllowEmbeddedExpression, ErrorInConstruct);
        break;

    case tkXmlAbort: // Dev10 695100 - tkXmlAbort is a signal from the scanner that it thinks we have exited the XML block. When that is the case,
                     // fall into ParseXMLElement which will create an XML node with the syntax error element built in and we can present a uniform
                     // set of XML elements to our callers, in much the same way that we provide uniform parse-trees that contain syntax error nodes
                     // as appropriate.
    default:
        Result = ParseXmlElement(Parent, ErrorInConstruct);
        break;
    }

    return Result;
}

ParseTree::XmlElementExpression *
MatchXmlEndElement(_In_opt_ ParseTree::XmlElementExpression *Element, ParseTree::XmlNameExpression *EndName)
{
    if (!Element)
        return NULL;

    if (Element->Name && Element->Name->Opcode == ParseTree::Expression::XmlName)
    {
        // This element has a fixed name
        ParseTree::XmlNameExpression *Name = Element->Name->AsXmlName();
        if (EndName)
        {
            if (Name->Prefix.Name == EndName->Prefix.Name &&
                Name->LocalName.Name == EndName->LocalName.Name)
            {
                return Element;
            }
        }
        else
        {
            // Empty EndName matches everything
            return Element;
        }
    }
    else
    {
        // This element has an expression name
        if (!EndName)
            return Element;
    }

    // Try matching the EndName with an ancestor
    return MatchXmlEndElement(Element->Parent, EndName);
}

ParseTree::XmlElementExpression *
Parser::ParseXmlElement
(
    _In_opt_ ParseTree::XmlElementExpression *Parent,
    _Inout_ bool &ErrorInConstruct
)
{
    Token * StartToken = m_CurrentToken->m_Prev;
    ParseTree::XmlElementExpression *Result = new(m_TreeStorage) ParseTree::XmlElementExpression;
    Result->Opcode = ParseTree::Expression::XmlElement;
    Result->Parent = Parent;
    Result->HasWSBeforeName = false;
    Result->HasWSBeforeEndName = false;
    Result->IsWellFormed = false;
    Result->NamesMatch = false;
    Result->HasEndName = false;
    Result->IsEmpty = false;
    Result->HasBeginEnd = false;
    Result->HasSignificantContent = false;

    if (m_CurrentToken->m_TokenType == tkXmlWhiteSpace)
    {
        Result->HasWSBeforeName = true;
        SkipXmlWhiteSpace();
    }

    Result->Name = ParseXmlQualifiedName(true /* AllowExpr */, false /* IsBracketed */, true /* IsElement */, ErrorInConstruct);

    Result->Attributes = ParseXmlAttributes(true, ErrorInConstruct);

    switch (m_CurrentToken->m_TokenType)
    {
    case tkXmlAbort:
    case tkEOF:
    case tkEOL:
        VerifyExpectedXmlToken(tkGT, ErrorInConstruct);
        break;

    case tkDiv:
        Result->IsEmpty = true;
        SetPunctuator(&Result->FirstPunctuator, StartToken, m_CurrentToken);
        GetNextXmlToken();
        if (!VerifyExpectedXmlToken(tkGT, ErrorInConstruct))
        {
            ErrorInConstruct = false;
            ReportSyntaxError(ERRID_MissingXmlEndTag, &Result->Name->TextSpan, ErrorInConstruct);
        }
        else
        {
            Result->IsWellFormed = true;
        }
        break;

    case tkGT:
        SetPunctuator(&Result->FirstPunctuator, StartToken, m_CurrentToken);
        GetNextXmlToken();
        // Set initial value for BeginEndTag Punctuator Location in case it's not found later
        SetPunctuator(&Result->BeginEndElementLoc, StartToken, m_CurrentToken);

        Result->Content = ParseXmlContent(Result, ErrorInConstruct);
        ParseXmlEndElement(Result, StartToken, ErrorInConstruct);
        break;

    default:
        ReportSyntaxError(ERRID_ExpectedGreater, m_CurrentToken, ErrorInConstruct);
        if (m_CurrentToken->m_TokenType != tkEOL && m_CurrentToken->m_TokenType != tkEOF)
            GetNextXmlToken();
    }

    SetExpressionLocation(Result, StartToken, m_CurrentToken, NULL);

    return Result;
}

ParseTree::XmlElementExpression *
Parser::ParseXmlEndElement(_Inout_ ParseTree::XmlElementExpression *Result, Token *StartToken, _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::XmlElementExpression *BestMatchElement;
    ParseTree::XmlNameExpression *EndName = NULL;
    Token *LT_Token;

    SetPunctuator(&Result->BeginEndElementLoc, StartToken, m_CurrentToken);

    LT_Token = m_CurrentToken;

    if (!VerifyExpectedXmlToken(tkLT, ErrorInConstruct))
    {
        ReportSyntaxError(ERRID_MissingXmlEndTag, &Result->Name->TextSpan, ErrorInConstruct);
        return Result;
    }

    if (!VerifyExpectedXmlToken(tkDiv, ErrorInConstruct))
        return Result;

    Result->HasBeginEnd = true;

    if (m_CurrentToken->m_TokenType == tkXmlWhiteSpace)
    {
        Result->HasWSBeforeEndName = true;
        SkipXmlWhiteSpace();
    }

    if (m_CurrentToken->m_TokenType == tkXmlNCName)
    {
        ParseTree::Expression *Expr = ParseXmlQualifiedName(false /* AllowExpr */, false /* IsBracketed */, true /* IsElement */, ErrorInConstruct);
        if (Expr && Expr->Opcode == ParseTree::Expression::XmlName)
        {
            EndName = Expr->AsXmlName();
            Result->EndNameTextSpan = EndName->TextSpan;
            Result->HasEndName = true;
        }
    }

    SkipXmlWhiteSpace();

    VerifyExpectedXmlToken(tkGT, ErrorInConstruct);

    // Check end names match

    BestMatchElement = MatchXmlEndElement(Result, EndName);

    // Don't consume the end element unless the names match. If they don't match then
    // this element is not well formed.

    if (!BestMatchElement)
    {
        Result->IsWellFormed = true;  // Found a closing tag but the names don't match
    }
    else if (BestMatchElement == Result) // Found a closing tag and the names match
    {
        Result->IsWellFormed = true;
        Result->NamesMatch = true; // EndName || Result->Name->Opcode != ParseTree::Expression::XmlName; to disallow short end tags
    }
    else // EndName matches an element higher up in the document
    {
        // back up so that this end element can be consumed higher up in the tree
        m_CurrentToken = LT_Token;
        Result->HasEndName = false;
        Result->HasWSBeforeEndName = false;
    }

    return Result;
}


ParseTree::ExpressionList *
Parser::ParseXmlAttributes
(
    bool AllowNameAsExpression,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionList *Attributes = NULL;
    ParseTree::ExpressionList **Prev = &Attributes;

    SkipXmlWhiteSpace();

    if (m_CurrentToken->m_TokenType == tkDiv)
        return NULL;

    while (m_CurrentToken->m_TokenType != tkEOL && m_CurrentToken->m_TokenType != tkEOF && m_CurrentToken->m_TokenType != tkXmlAbort)
    {
        Token *Start = m_CurrentToken;

        ParseTree::Expression *Result = ParseXmlAttribute(AllowNameAsExpression, ErrorInConstruct);

        if (!Result)
            break;

        ParseTree::ExpressionList *Current = new(m_TreeStorage) ParseTree::ExpressionList;
        SetLocation(
           &Current->TextSpan,
           Start,
           m_CurrentToken);
        Current->Element = Result;
        *Prev = Current;
        Prev = &Current->Next;
    }

    SkipXmlWhiteSpace();

    if (Attributes)
        FixupListEndLocations(Attributes);

    return Attributes;
}


ParseTree::Expression *
Parser::ParseXmlAttribute
(
    bool AllowNameAsExpression,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Result = NULL;

    Token *Start = m_CurrentToken;

    if (m_CurrentToken->m_TokenType == tkXmlNCName ||
        (AllowNameAsExpression && m_CurrentToken->m_TokenType == tkLT) ||
        m_CurrentToken->m_TokenType == tkQuote ||
        m_CurrentToken->m_TokenType == tkSQuote)
    {
        bool PrecededByWhiteSpace = m_CurrentToken->m_Prev->m_TokenType == tkXmlWhiteSpace;
        ParseTree::Expression *Name = ParseXmlQualifiedName(true /* AllowExpr */, false /* IsBracketed */, false /* IsElement */, ErrorInConstruct);
        ParseTree::XmlAttributeExpression *Attribute = new(m_TreeStorage) ParseTree::XmlAttributeExpression;
        Attribute->Opcode = ParseTree::Expression::XmlAttribute;
        SetExpressionLocation(Attribute, Start, m_CurrentToken, NULL);
        Attribute->Delimiter = tkNone;
        Attribute->Name = Name;
        Attribute->PrecededByWhiteSpace = PrecededByWhiteSpace;
        Attribute->IsImportedNamespace = false;

        SkipXmlWhiteSpace();

        if (m_CurrentToken->m_TokenType == tkEQ)
        {
            SetPunctuator(&Attribute->FirstPunctuator, Start, m_CurrentToken);

            GetNextXmlToken();

            SkipXmlWhiteSpace();

            Attribute->Value = ParseXmlAttributeValue(Attribute, ErrorInConstruct);

            SetExpressionLocation(Attribute, Start, m_CurrentToken, NULL);

            Result = Attribute;
        }
        else if (Name->Opcode == ParseTree::Expression::XmlName)
        {
            // Names must be followed by an "="
            Result = Attribute;
            Attribute->Value = ExpressionSyntaxError();
            HandleUnexpectedToken(tkEQ, ErrorInConstruct);
        }
        else if (Name->Opcode == ParseTree::Expression::XmlEmbedded)
        {
            // In this case, the Name is some expression which may evaluate to an attribute
            Result = Name;
        }
        else
        {
            // Case of quoted string without an attribute name.

            // todo Microsoft - Move error to Semantics
            ReportSyntaxError(ERRID_ExpectedXmlName, m_CurrentToken, m_CurrentToken, ErrorInConstruct);

            Attribute->Value = ParseXmlAttributeValue(Attribute, ErrorInConstruct);

            SetExpressionLocation(Attribute, Start, m_CurrentToken, NULL);

            Result = Attribute;
        }

        SkipXmlWhiteSpace();
    }

    return Result;
}

ParseTree::Expression *
Parser::ParseXmlAttributeValue
(
    _Out_ ParseTree::XmlAttributeExpression *Attribute,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Result;

    switch (m_CurrentToken->m_TokenType)
    {
    case tkSQuote:
    case tkQuote:
        Result = ParseXmlString(Attribute, ErrorInConstruct);
        break;

    case tkXmlAttributeData:
        // String value will be missing quote delimiter.  PrettyLister can add quotes in IDE.  Semantics can error if
        // quotes are missing.
        Attribute->Delimiter = tkXmlWhiteSpace;
        Result = MakeXmlStringLiteral();
        break;

    case tkLT:
        // <%= expr %>
        GetNextXmlToken();
        Result = ParseXmlEmbedded(true, ErrorInConstruct);
        break;

    default:
        ReportSyntaxError(ERRID_StartAttributeValue, m_CurrentToken, ErrorInConstruct);
        Result = ExpressionSyntaxError();
        SetExpressionLocation(Result, m_CurrentToken, m_CurrentToken, NULL);
        if (m_CurrentToken->m_TokenType != tkEOL && m_CurrentToken->m_TokenType != tkEOF && m_CurrentToken->m_TokenType != tkXmlAbort)
            GetNextXmlToken();
        break;
    }

    return Result;
}


ParseTree::Expression *
Parser::ParseXmlQualifiedName
(
    bool AllowExpr,
    bool IsBracketed,
    bool IsElementName,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Result = NULL;

    Token *Start = m_CurrentToken;

    switch (m_CurrentToken->m_TokenType)
    {
    case tkXmlNCName:
        {
            ParseTree::XmlNameExpression *NameExpr = new(m_TreeStorage) ParseTree::XmlNameExpression;
            NameExpr->Opcode = ParseTree::Expression::XmlName;
            NameExpr->IsBracketed = IsBracketed;
            NameExpr->IsElementName = IsElementName;

            ParseTree::IdentifierDescriptor PrefixOrLocalName = CreateXmlNameIdentifier(m_CurrentToken, ErrorInConstruct);

            GetNextXmlToken();

            if (m_CurrentToken->m_TokenType == tkXmlColon)
            {
                GetNextXmlToken();

                if (m_CurrentToken->m_TokenType != tkXmlNCName)
                {
                    goto SyntaxError;
                }

                NameExpr->Prefix = PrefixOrLocalName;
                NameExpr->LocalName = CreateXmlNameIdentifier(m_CurrentToken, ErrorInConstruct);

                GetNextXmlToken();
            }
            else
            {
                NameExpr->Prefix.Name = STRING_CONST2(m_pStringPool, EmptyString);
                NameExpr->LocalName = PrefixOrLocalName;
            }

            Result = NameExpr;
        }
        break;

    case tkLT:
        if (!AllowExpr)
            goto SyntaxError;

        // <%= expr %>
        GetNextXmlToken();
        Result = ParseXmlEmbedded(true, ErrorInConstruct);
        break;

    default:
SyntaxError:
        ReportSyntaxError(ERRID_ExpectedXmlName, m_CurrentToken, ErrorInConstruct);
        Result = ExpressionSyntaxError();
        SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

        if (m_CurrentToken->m_TokenType == tkSyntaxError)
            GetNextXmlToken();

        return Result;
    }

    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);
    return Result;
}

ParseTree::ExpressionList *
Parser::ParseXmlContent
(
    _Inout_ ParseTree::XmlElementExpression *Parent,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionList *Content = NULL;
    ParseTree::ExpressionList **Next = &Content;
    ParseTree::ExpressionList **Prev = &Content;
    ParseTree::Expression *Result;
    ParseTree::ExpressionList *Current = NULL;

    Token *Start = m_CurrentToken;

    while (m_CurrentToken->m_TokenType != tkEOL && m_CurrentToken->m_TokenType != tkEOF)
    {
        switch (m_CurrentToken->m_TokenType)
        {
        case tkLT:
            if (m_CurrentToken->m_Next && m_CurrentToken->m_Next->m_TokenType == tkDiv)
            {
                Parent->HasBeginEnd = true;
                goto FixupContentList;
            }

            GetNextXmlToken();
            Result = ParseXmlMarkup(Parent, true, ErrorInConstruct);
            break;

        case tkXmlAbort:
        case tkXmlEndEmbedded:
            // If an expression hole contains an unterminated xml element, it is possible to get a tkEndVB token in content.
            // Let this token function as </> and terminate the xml literal.
            goto FixupContentList;


        case tkXmlWhiteSpace:
        case tkXmlCharData:
            Result = MakeXmlStringLiteral();
            break;

        case tkConcat:
            Result =  ParseXmlReference(ErrorInConstruct);
            break;

        default:
            // Todo: Microsoft - Provide list of expected tokens instead of one.
            HandleUnexpectedToken(tkLT, ErrorInConstruct);
            Result = ExpressionSyntaxError();
            SetExpressionLocation(Result, m_CurrentToken, m_CurrentToken, NULL);
            GetNextXmlToken();
            break;

        }

        Current = new(m_TreeStorage) ParseTree::ExpressionList;
        SetLocation(
               &Current->TextSpan,
               Start,
               m_CurrentToken);
        Current->Element = Result;
        Prev = Next;
        *Next = Current;
        Next = &Current->Next;
    }

FixupContentList:
    if (Content)
    {
        FixupListEndLocations(Content);
        Parent->HasSignificantContent = CheckXmlWhiteSpace(Content);
    }

    return Content;
}


ParseTree::Expression *
Parser::ParseXmlPI(_Inout_ bool &ErrorInConstruct)
{
    Token *Start = m_CurrentToken->m_Prev;

    ParseTree::XmlPIExpression *PI = new(m_TreeStorage) ParseTree::XmlPIExpression;
    PI->Opcode = ParseTree::Expression::XmlPI;

    GetNextXmlToken();
    PI->Name = ParseXmlQualifiedName(false /* AllowExpr */, false /* IsBracketed */, false /*IsElement */, ErrorInConstruct);

    if (m_CurrentToken->m_TokenType == tkXmlWhiteSpace)
    {
        PI->MissingWhiteSpace = false;
        SkipXmlWhiteSpace();
    }
    else
    {
        PI->MissingWhiteSpace = true;
    }

    PI->Content = ParseXmlOtherData(tkXmlEndPI, ErrorInConstruct);
    if (PI->Content)
    {
        ParseTree::Expression *Expr = PI->Content->Element;
        if (Expr->Opcode == ParseTree::Expression::XmlCharData)
        {
            Expr->AsXmlCharData()->IsMovable = true;
        }
    }
    else
    {
        // whitespace is required only when there is content
        PI->MissingWhiteSpace = false;
    }

    SetExpressionLocation(PI, Start, m_CurrentToken, NULL);
    return PI;
}


ParseTree::Expression *
Parser::ParseXmlCData(_Inout_ bool &ErrorInConstruct)
{
    Token *Start = m_CurrentToken->m_Prev;
    ParseTree::XmlExpression *Result = new(m_TreeStorage) ParseTree::XmlExpression;
    Result->Opcode = ParseTree::Expression::XmlCData;

    GetNextXmlToken();
    Result->Content = ParseXmlOtherData(tkXmlEndCData, ErrorInConstruct);
    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

    return Result;
}


ParseTree::Expression *
Parser::ParseXmlComment(_Inout_ bool &ErrorInConstruct)
{
    Token *Start = m_CurrentToken->m_Prev;
    ParseTree::XmlExpression *Result = new(m_TreeStorage) ParseTree::XmlExpression;
    Result->Opcode = ParseTree::Expression::XmlComment;

    GetNextXmlToken();
    Result->Content = ParseXmlOtherData(tkXmlEndComment, ErrorInConstruct);
    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

    return Result;
}

ParseTree::ExpressionList *
Parser::ParseXmlOtherData(tokens EndToken, _Inout_ bool &ErrorInConstruct)
{
    ParseTree::ExpressionList *Content = NULL;
    ParseTree::Expression *SyntaxError;
    ParseTree::ExpressionList **Prev = &Content;
    bool Done = false;

    while (!Done)
    {
        ParseTree::ExpressionList *Current = new(m_TreeStorage) ParseTree::ExpressionList;

        SetLocation(
           &Current->TextSpan,
           m_CurrentToken,
           m_CurrentToken);

        switch (m_CurrentToken->m_TokenType)
        {
        case tkXmlWhiteSpace:
        case tkXmlCommentData:
        case tkXmlCData:
        case tkXmlPIData:
            Current->Element = MakeXmlStringLiteral();
            *Prev = Current;
            Prev = &Current->Next;
            break;

        case tkXmlEndComment:
        case tkXmlEndPI:
        case tkXmlEndCData:
            Done = true;
            break;

        default:
            HandleUnexpectedToken(EndToken, ErrorInConstruct);
            SyntaxError = ExpressionSyntaxError();
            SetExpressionLocation(SyntaxError, m_CurrentToken, m_CurrentToken, NULL);
            if (m_CurrentToken->m_TokenType == tkSyntaxError)
                GetNextXmlToken();
            Current->Element = SyntaxError;
            *Prev = Current;
            Done = true;
            break;
        }
    }

    if (Content)
    {
       FixupListEndLocations(Content);
    }

    GetNextXmlToken();

    return Content;
}

ParseTree::Expression *
Parser::ParseXmlString
(
    _Out_ ParseTree::XmlAttributeExpression *Attribute,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Result = NULL;
    ParseTree::ExpressionList *ResultList = NULL;
    ParseTree::ExpressionList **Prev = &ResultList;
    bool Done = false;
    tokens StringDelimiter = m_CurrentToken->m_TokenType;

    Attribute->Delimiter = StringDelimiter;
    SetPunctuator(&Attribute->StartDelimiterLoc, Attribute->TextSpan, m_CurrentToken);

    Token *Start = m_CurrentToken;

    GetNextXmlToken();

    Token *t;

    while (!Done)
    {
        ParseTree::ExpressionList *Current = NULL;
        ParseTree::Expression *Literal = NULL;

        switch (m_CurrentToken->m_TokenType)
        {
        case tkXmlWhiteSpace:
        case tkXmlAttributeData:
            Literal = MakeXmlStringLiteral();
            break;

        case tkConcat:
            Literal = ParseXmlReference(ErrorInConstruct);
            break;

        case tkLT:
            t = m_CurrentToken;
            // Look ahead for an embedded expression
            GetNextXmlToken();
            if (m_CurrentToken->m_TokenType == tkXmlBeginEmbedded)
            {
                ReportSyntaxError(ERRID_QuotedEmbeddedExpression, m_CurrentToken, m_CurrentToken, ErrorInConstruct);
            }
            m_CurrentToken = t;

            __fallthrough; // fall through

        default:
            if (!Result)
            {
                ParseTree::XmlCharDataExpression *EmptyLiteral = new(m_TreeStorage) ParseTree::XmlCharDataExpression;
                EmptyLiteral->Opcode = ParseTree::Expression::XmlCharData;
                EmptyLiteral->LengthInCharacters = 0;
                EmptyLiteral->Value = L"";
                EmptyLiteral->IsWhitespace = true;
                EmptyLiteral->IsSignificant = true;
                EmptyLiteral->IsMovable = false;
                SetExpressionLocation(EmptyLiteral, m_CurrentToken, m_CurrentToken, NULL);
                Literal = EmptyLiteral;
            }

            Done = true;
            break;
        }

        if (Literal)
        {
            if (!Result)
            {
               Result = Literal;
            }
            else
            {
                Current = new(m_TreeStorage) ParseTree::ExpressionList;
                Current->Element = Literal;
                *Prev = Current;
                Prev = &Current->Next;

                SetLocation(
                   &Current->TextSpan,
                   Start,
                   m_CurrentToken);
            }
        }
    }

    SetPunctuator(&Attribute->EndDelimiterLoc, Attribute->TextSpan, m_CurrentToken);

    if (ResultList && Result)
    {
        // Add first result to head of the list

        ParseTree::ExpressionList *Head = new(m_TreeStorage) ParseTree::ExpressionList;
        Head->Element = Result;
        Head->Next = ResultList;
        ResultList = Head;

        SetLocation(
           &Head->TextSpan,
           Start,
           m_CurrentToken);

        FixupListEndLocations(ResultList);

        ParseTree::XmlExpression *ListValue = new(m_TreeStorage) ParseTree::XmlExpression;
        ListValue->Opcode = ParseTree::Expression::XmlAttributeValueList;

        ListValue->Content = ResultList;
        Result = ListValue;
    }

    // Don't get next token until after FixupListEndLocations.  Otherwise, Fixup Asserts.

    if (m_CurrentToken->m_TokenType == StringDelimiter)
    {
        GetNextXmlToken();
    }
    else
    {
        ErrorInConstruct = false;
        HandleUnexpectedToken(StringDelimiter, ErrorInConstruct);
    }

    SetLocation(
        &Result->TextSpan,
        Start,
        m_CurrentToken);

    return Result;
}

ParseTree::Expression *
Parser::ParseXmlReference(_Inout_ bool &ErrorInConstruct)
{
    Token *StartReference = m_CurrentToken;

    GetNextXmlToken();

    ParseTree::XmlReferenceExpression *Reference;

    if (m_CurrentToken->m_TokenType == tkXmlReference)
    {
        Reference = MakeXmlCharReference();
    }
    else if (m_CurrentToken->m_TokenType == tkXmlNCName)
    {
        Reference = MakeXmlEntityReference();
    }
    else
    {
        HandleUnexpectedToken(tkXmlReference, ErrorInConstruct);
        if (m_CurrentToken->m_TokenType == tkSyntaxError)
            GetNextXmlToken();
        return  ExpressionSyntaxError();
    }

    if (m_CurrentToken->m_TokenType == tkSColon)
    {
        GetNextXmlToken();
        SetExpressionLocation(Reference, StartReference, m_CurrentToken, NULL);
        Reference->HasSemiColon = true;
    }
    else
    {
        Reference->HasSemiColon = false;
    }

    return Reference;
}


ParseTree::Expression *
Parser::ParseXmlEmbedded(bool AllowEmbedded, _Inout_ bool &ErrorInConstruct)
{
    Token *Start = m_CurrentToken->m_Prev; // The start of the expression hole is the <.  The current token is %=.
    ParseTree::ExpressionList *OldRoots = m_XmlRoots;
    ParseTree::ExpressionList **OldRootsEnd = m_XmlRootsEnd;
    m_XmlRoots = NULL;
    m_XmlRootsEnd = &m_XmlRoots;

    if (m_CurrentToken->m_TokenType != tkXmlBeginEmbedded)
    {
        HandleUnexpectedToken(tkXmlBeginEmbedded, ErrorInConstruct);
        return ExpressionSyntaxError();
    }

    ParseTree::XmlEmbeddedExpression *Result = new(m_TreeStorage) ParseTree::XmlEmbeddedExpression;
    Result->Opcode = ParseTree::Expression::XmlEmbedded;
    Result->AllowEmdedded = AllowEmbedded;
    Result->FirstPunctuator.Line = Result->FirstPunctuator.Column = 0;

    GetNextXmlToken();
    EatNewLine();
    
    Result->Operand = ParseExpression(ErrorInConstruct);

    Result->XmlRoots = m_XmlRoots;

    EatNewLineIfFollowedBy(tkXmlEndEmbedded, Result);

    if (m_CurrentToken->m_TokenType != tkXmlEndEmbedded)
    {
        HandleUnexpectedToken(tkXmlEndEmbedded, ErrorInConstruct);

        while (true)
        {
            ResyncAt(2, tkXmlEndEmbedded, tkLT);

            if (m_CurrentToken->m_TokenType == tkColon)
                GetNextXmlToken();
            else
                break;
        }
    }

    if (m_CurrentToken->m_TokenType == tkXmlEndEmbedded)
    {
        SetPunctuator(&Result->FirstPunctuator, Start, m_CurrentToken);
        GetNextXmlToken();
    }

    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

    m_XmlRoots = OldRoots;
    m_XmlRootsEnd = OldRootsEnd;

    return Result;
}

ParseTree::XmlReferenceExpression *
Parser::MakeXmlCharReference()
{
    VSASSERT(m_CurrentToken->m_TokenType == tkXmlReference, "Wrong token type");

    Token::XmlCharTypes CharType = static_cast<Token::XmlCharTypes>(m_CurrentToken->m_XmlCharData.m_Type);

    ParseTree::XmlReferenceExpression *Reference = MakeXmlCharReference(m_CurrentToken->m_XmlCharData.m_Value,
        m_CurrentToken->m_XmlCharData.m_Length,
        true);

    Reference->IsHexCharRef = CharType == Token::XmlHexCharRef;

    return Reference;
}


ParseTree::XmlReferenceExpression *
Parser::MakeXmlCharReference(const WCHAR *Value, size_t Length, bool CopyData)
{
    Token *Start = m_CurrentToken->m_Prev;
    ParseTree::XmlReferenceExpression *Reference = new(m_TreeStorage) ParseTree::XmlReferenceExpression;
    Reference->Opcode = ParseTree::Expression::XmlReference;
    Reference->IsSignificant = true;
    Reference->IsMovable = false;
    Reference->IsHexCharRef = false;
    Reference->IsWhitespace = false;
    Reference->IsDefined = true;
    Reference->Name = NULL;

    Reference->LengthInCharacters = Length;

    // Copy string.

    if (CopyData)
    {
        SafeInt<size_t> SafeLength(Length);
        SafeLength += 1;
        Reference->Value = new(m_TreeStorage) WCHAR[SafeLength.Value()];
        memcpy(
            Reference->Value,
            Value,
            Length * sizeof(WCHAR));
        Reference->Value[Length] = L'\0';
    }
    else
    {
        Reference->Value = const_cast<WCHAR*>(Value);
    }

    // Skip past the semi colon
    GetNextXmlToken();

    SetExpressionLocation(Reference, Start, m_CurrentToken, NULL);

    return Reference;
}

ParseTree::XmlReferenceExpression *
Parser::MakeXmlEntityReference()
{
    VSASSERT(m_CurrentToken->m_TokenType == tkXmlNCName, "Wrong token type");

    STRING *Id = m_CurrentToken->m_Id.m_Spelling;
    bool IsDefined = true;
    ParseTree::XmlReferenceExpression *Reference;


    if (Id == STRING_CONST2(m_pStringPool, amp))
    {
        Reference =  MakeXmlCharReference(L"&", 1, false);
    }
    else if (Id == STRING_CONST2(m_pStringPool, lt))
    {
        Reference =  MakeXmlCharReference(L"<", 1, false);
    }
    else if (Id == STRING_CONST2(m_pStringPool, gt))
    {
        Reference =  MakeXmlCharReference(L">", 1, false);
    }
    else if (Id == STRING_CONST2(m_pStringPool, apos))
    {
        Reference =  MakeXmlCharReference(L"'", 1, false);
    }
    else if (Id == STRING_CONST2(m_pStringPool, quot))
    {
        Reference =  MakeXmlCharReference(L"\"", 1, false);
    }
    else
    {
        Reference =  MakeXmlCharReference(L"", 0, false);
        IsDefined = false;
    }

    Reference->IsDefined = IsDefined;
    Reference->Name = Id;

    return Reference;
}


ParseTree::XmlCharDataExpression *
Parser::MakeXmlStringLiteral()
{
    VSASSERT(m_CurrentToken->m_XmlCharData.m_Type == Token::XmlCharData, "Wrong token type");

    ParseTree::XmlCharDataExpression *CharData = new(m_TreeStorage) ParseTree::XmlCharDataExpression;
    CharData->Opcode = ParseTree::Expression::XmlCharData;
    CharData->Value = NULL;
    CharData->IsSignificant = true;
    CharData->IsMovable = false;
    Token *Start = m_CurrentToken;

    CharData->IsWhitespace = m_CurrentToken->m_TokenType == tkXmlWhiteSpace;
    if (m_CurrentToken->m_TokenType == tkXmlWhiteSpace &&  m_CurrentToken->m_XmlCharData.m_IsNewLine)
    {
        CharData->Value = L"\n";
        CharData->LengthInCharacters = 1;
    }

    if (CharData->Value == NULL)
    {
        size_t Length = m_CurrentToken->m_XmlCharData.m_Length;
        const WCHAR *Value = m_CurrentToken->m_XmlCharData.m_Value;

        CharData->LengthInCharacters = Length;

        // Copy string.

        CharData->Value = new(m_TreeStorage) WCHAR[Length + 1];
        memcpy(
            CharData->Value,
            Value,
            Length * sizeof(WCHAR));
        CharData->Value[Length] = L'\0';
    }

    GetNextXmlToken();

    SetExpressionLocation(CharData, Start, m_CurrentToken, NULL);

    return CharData;
}

bool
Parser::SkipXmlWhiteSpace()
{
    bool FoundWhiteSpace = false;
    while (m_CurrentToken->m_TokenType == tkXmlWhiteSpace)
    {
        FoundWhiteSpace = true;
        GetNextXmlToken();
    }

    return FoundWhiteSpace;
}

ParseTree::Initializer *
Parser::ParseSelectListInitializer
(
    _Inout_ bool &ErrorInConstruct
)
{
    if
    (
        (
            m_CurrentToken->m_TokenType == tkID ||
            m_CurrentToken->IsKeyword()
        ) &&
       m_CurrentToken->m_Next &&
       (
            m_CurrentToken->m_Next->m_TokenType == tkEQ ||
            (
                m_CurrentToken->m_Next->m_TokenType == tkNullable &&
                m_CurrentToken->m_Next->m_Next &&
                m_CurrentToken->m_Next->m_Next->m_TokenType == tkEQ
            )
        )
    )
    {

        ParseTree::AssignmentInitializer *Result = new(m_TreeStorage) ParseTree::AssignmentInitializer;
        Result->Opcode = ParseTree::Initializer::Assignment;
        Result->FieldIsKey = true;

        //
        Token *Dot = m_CurrentToken;
        Token * Equals = m_CurrentToken->m_TokenType == tkEQ ? m_CurrentToken : m_CurrentToken->m_Next;

        // Parse form: <IdentiferOrKeyword> '=' <Expression>
        Result->Name = ParseIdentifier(ErrorInConstruct, true);

        if (Result->Name.IsNullable)
        {
            ReportSyntaxError
            (
                ERRID_NullableTypeInferenceNotSupported,
                Dot,
                m_CurrentToken,
                ErrorInConstruct
            );
        }

        if (ErrorInConstruct)
        {
            ResyncAt(14, tkEQ, tkComma, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN,tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
        }

        if (m_CurrentToken->m_TokenType == tkEQ)
        {
            Equals = m_CurrentToken;
            GetNextToken();
            EatNewLine(); // Dev10_472105: Allow line continuation after =, e.g. Select foo = <eol>

            ParseTree::ExpressionInitializer *expr = new(m_TreeStorage) ParseTree::ExpressionInitializer;
            expr->Opcode = ParseTree::Initializer::Expression;
            expr->Value = ParseExpression(ErrorInConstruct);
            expr->FieldIsKey = true;

            Result->Initializer = expr;
        }

        SetLocation(
            &Result->TextSpan,
            Dot,
            m_CurrentToken);

        SetPunctuator(
            &Result->Dot,
            Dot,
            Dot);

        SetPunctuator(
            &Result->Equals,
            Dot,
            Equals);

        return Result;
    }

    ParseTree::ExpressionInitializer *Result = new(m_TreeStorage) ParseTree::ExpressionInitializer;
    Result->Opcode = ParseTree::Initializer::Expression;
    Result->Value = ParseExpression(ErrorInConstruct);
    Result->FieldIsKey = true;

    return Result;
}


ParseTree::InitializerList *
Parser::ParseSelectList
(
    _Inout_ bool &ErrorInConstruct
)
{

    ParseTree::InitializerList *Result = NULL;
    ParseTree::InitializerList **ListTarget = &Result;

    for(;;)
    {
        Token *InitializerStart = m_CurrentToken;

        ParseTree::Initializer *Initializer = ParseSelectListInitializer(ErrorInConstruct);

        if (ErrorInConstruct)
        {
            ResyncAt(13, tkComma, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkFROM, tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
        }

        CreateListElement(
            ListTarget,
            Initializer,
            InitializerStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        if (m_CurrentToken->m_TokenType == tkComma)
        {
            GetNextToken();
            EatNewLine();
            continue;
        }

        break;
    }

    FixupListEndLocations(Result);

    return Result;
}


ParseTree::Expression *
Parser::ParseAggregationExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::AggregationExpression *Result = new(m_TreeStorage) ParseTree::AggregationExpression;
    Result->Opcode = ParseTree::Expression::Aggregation;

    Token * Start = m_CurrentToken;
    bool TempErrorInConstruct = ErrorInConstruct;

    if (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkGROUP)
    {
        if (m_CurrentToken->m_Next && m_CurrentToken->m_Next->m_TokenType == tkLParen)
        {
            ReportSyntaxError(
                ERRID_InvalidUseOfKeyword,
                m_CurrentToken,
                ErrorInConstruct);
        }
        else
        {
            ReportSyntaxError(
                ERRID_UnexpectedGroup,
                m_CurrentToken,
                ErrorInConstruct);
        }

        GetNextToken();
        return ExpressionSyntaxError();
    }

    Result->AggFunc = ParseIdentifier(ErrorInConstruct);

    if(Result->AggFunc.IsBad)
    {
        return ExpressionSyntaxError();
    }
    else
    {
        if(m_CurrentToken->m_TokenType == tkLParen)
        {
            SetPunctuator(&Result->FirstPunctuator, Start, m_CurrentToken);
            Result->HaveLParen = true;
            GetNextToken();
            EatNewLine();
            EatNewLineIfFollowedBy(tkRParen, Result);

            if(m_CurrentToken->m_TokenType != tkRParen)
            {
                Result->Argument = ParseExpression(ErrorInConstruct);
            }

            EatNewLineIfFollowedBy(tkRParen, Result);

            if(m_CurrentToken->m_TokenType == tkRParen)
            {
                SetPunctuator(&Result->RParenLocation, Start, m_CurrentToken);
                Result->HaveRParen = true;
                GetNextToken();

                // check that expression doesn't continue
                if(!CheckForEndOfExpression(Start, ErrorInConstruct))
                {
                    return ExpressionSyntaxError();
                }
            }
            else
            {
                ReportSyntaxError(ERRID_ExpectedRparen, m_CurrentToken, ErrorInConstruct);
            }
        }
        else
        {
            // check that expression doesn't continue
            if(!CheckForEndOfExpression(Start, ErrorInConstruct))
            {
                return ExpressionSyntaxError();
            }
        }
    }

    SetLocation(&Result->TextSpan, Start, m_CurrentToken);

    return Result;
}


// check that expression doesn't continue
bool
Parser::CheckForEndOfExpression
(
    Token * Start,
    bool &ErrorInConstruct 
)
{
    Token * End = m_CurrentToken;
    bool TempErrorInConstruct = ErrorInConstruct;

    // check that expression doesn't continue
    m_CurrentToken = Start;
    bool ErrorsAlreadyDisabled = DisableErrors();
    ParseExpression(TempErrorInConstruct);


    EnableErrors(ErrorsAlreadyDisabled);

    if(End != m_CurrentToken)
    {
        m_CurrentToken = End;
        ReportSyntaxError(ERRID_ExpectedEndOfExpression, m_CurrentToken, ErrorInConstruct);
        return false;
    }

    return true;
}


ParseTree::Initializer *
Parser::ParseAggregateListInitializer
(
    bool AllowGroupName,
    _Inout_ bool &ErrorInConstruct
)
{
    if
    (
        (
            m_CurrentToken->m_TokenType == tkID ||
            m_CurrentToken->IsKeyword()
        ) &&
        m_CurrentToken->m_Next &&
        (
            m_CurrentToken->m_Next->m_TokenType == tkEQ ||
            (
                m_CurrentToken->m_Next->m_TokenType == tkNullable &&
                m_CurrentToken->m_Next->m_Next &&
                m_CurrentToken->m_Next->m_Next->m_TokenType == tkEQ
            )
        )
    )
    {
        ParseTree::AssignmentInitializer *Result = new(m_TreeStorage) ParseTree::AssignmentInitializer;
        Result->Opcode = ParseTree::Initializer::Assignment;
        Result->FieldIsKey = true;

        Token *Dot = m_CurrentToken;
        Token *Equals = m_CurrentToken->m_Next;

        // Parse form: <IdentiferOrKeyword> '=' <Expression>
        Result->Name = ParseIdentifier(ErrorInConstruct, true);

        if (Result->Name.IsNullable)
        {
            ReportSyntaxError
            (
                ERRID_NullableTypeInferenceNotSupported,
                Dot,
                m_CurrentToken,
                ErrorInConstruct
            );
        }

        if (ErrorInConstruct)
        {
            ResyncAt(14, tkEQ, tkComma, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN,tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
        }

        AssertIfFalse(m_CurrentToken->m_TokenType == tkEQ);

        if (m_CurrentToken->m_TokenType == tkEQ)
        {
            Equals = m_CurrentToken;
            GetNextToken(); // move off the '='

            ParseTree::ExpressionInitializer *expr = new(m_TreeStorage) ParseTree::ExpressionInitializer;
            expr->Opcode = ParseTree::Initializer::Expression;
            EatNewLine(); // line continuation allowed after  '=' 

            if (m_CurrentToken->m_TokenType == tkID || m_CurrentToken->IsKeyword())
            {
                if (AllowGroupName &&
                    m_CurrentToken->m_TokenType == tkID &&
                    IdentifierAsKeyword(m_CurrentToken) == tkGROUP &&
                    !(m_CurrentToken->m_Next && m_CurrentToken->m_Next->m_TokenType == tkLParen))
                {
                    Token * Start = m_CurrentToken;

                    expr->Value = new(m_TreeStorage) ParseTree::Expression;
                    expr->Value->Opcode = ParseTree::Expression::GroupRef;
                    
                    GetNextToken(); // Move off 'Group'

                    SetExpressionLocation(expr->Value, Start, m_CurrentToken, NULL);

                    // check that expression doesn't continue
                    if (!CheckForEndOfExpression(Start, ErrorInConstruct))
                    {
                        expr->Value = ExpressionSyntaxError();
                    }
                }
                else
                {
                    expr->Value = ParseAggregationExpression(ErrorInConstruct); // this must be an Aggregation
                    AssertIfFalse(expr->Value->Opcode == ParseTree::Expression::Aggregation ||
                        expr->Value->Opcode == ParseTree::Expression::SyntaxError);
                }
            }
            else if(AllowGroupName)
            {
                ReportSyntaxError(ERRID_ExpectedIdentifierOrGroup, m_CurrentToken, ErrorInConstruct);
                expr->Value = ExpressionSyntaxError();
            }
            else
            {
                ReportSyntaxError(ERRID_ExpectedIdentifier, m_CurrentToken, ErrorInConstruct);
                expr->Value = ExpressionSyntaxError();
            }

            expr->FieldIsKey = true;

            Result->Initializer = expr;
        }

        SetLocation(
            &Result->TextSpan,
            Dot,
            m_CurrentToken);

        SetPunctuator(
            &Result->Dot,
            Dot,
            Dot);

        SetPunctuator(
            &Result->Equals,
            Dot,
            Equals);

        return Result;
    }

    ParseTree::ExpressionInitializer *Result = new(m_TreeStorage) ParseTree::ExpressionInitializer;
    Result->Opcode = ParseTree::Initializer::Expression;
    Result->FieldIsKey = true;

    if (m_CurrentToken->m_TokenType == tkID || m_CurrentToken->IsKeyword())
    {
        if(AllowGroupName && IdentifierAsKeyword(m_CurrentToken) == tkGROUP &&
            !(m_CurrentToken->m_Next && m_CurrentToken->m_Next->m_TokenType == tkLParen))
        {
            Token * Start = m_CurrentToken;

            Result->Value = new(m_TreeStorage) ParseTree::Expression;
            Result->Value->Opcode = ParseTree::Expression::GroupRef;
            GetNextToken();
            SetExpressionLocation(Result->Value, Start, m_CurrentToken, NULL);

            // check that expression doesn't continue
            if(!CheckForEndOfExpression(Start, ErrorInConstruct))
            {
                Result->Value = ExpressionSyntaxError();
            }
        }
        else
        {
            // must be an Aggregation
            Result->Value = ParseAggregationExpression(ErrorInConstruct);
            AssertIfFalse(Result->Value->Opcode == ParseTree::Expression::Aggregation ||
                Result->Value->Opcode == ParseTree::Expression::SyntaxError);
        }
    }
    else if(AllowGroupName)
    {
        ReportSyntaxError(ERRID_ExpectedIdentifierOrGroup, m_CurrentToken, ErrorInConstruct);
        Result->Value = ExpressionSyntaxError();
    }
    else
    {
        ReportSyntaxError(ERRID_ExpectedIdentifier, m_CurrentToken, ErrorInConstruct);
        Result->Value = ExpressionSyntaxError();
    }

    return Result;
}


ParseTree::InitializerList *
Parser::ParseAggregateList
(
    bool AllowGroupName,
    bool IsGroupJoinProjection,
    _Inout_ bool &ErrorInConstruct
)
{

    ParseTree::InitializerList *Result = NULL;
    ParseTree::InitializerList **ListTarget = &Result;

    for(;;)
    {
        Token *InitializerStart = m_CurrentToken;

        ParseTree::Initializer *Initializer = ParseAggregateListInitializer(AllowGroupName, ErrorInConstruct);

        if (ErrorInConstruct)
        {
            if(IsGroupJoinProjection)
            {
                ResyncAt(14, tkComma, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkFROM, tkDISTINCT,tkAGGREGATE,tkINTO,tkON,tkSKIP,tkTAKE,tkLET);
            }
            else
            {
                ResyncAt(13, tkComma, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkFROM, tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
            }
        }

        CreateListElement(
            ListTarget,
            Initializer,
            InitializerStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

        if (m_CurrentToken->m_TokenType == tkComma)
        {
            GetNextToken();
            EatNewLine();
            continue;
        }

        break;
    }

    FixupListEndLocations(Result);

    return Result;
}


ParseTree::FromList *
Parser::ParseFromList
(
    bool AssignmentList,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::FromList *ResultFromList = NULL;
    ParseTree::FromList **FromList = &ResultFromList;
    tokens peek;

    Token *FromListStart = m_CurrentToken;
    bool AnotherFromExists;

    for(;;)
    {
        ParseTree::FromItem *Result = new(m_TreeStorage) ParseTree::FromItem;
        Token *FromItemStart = m_CurrentToken;

        // Parse the control variable declaration. We always parse it as a declaration because
        // it doesn't bind to locals.
        Token *DeclaratorStart = m_CurrentToken;
        ParseTree::Declarator *Declarator = new(m_TreeStorage) ParseTree::Declarator;
        Declarator->Name = ParseIdentifier(ErrorInConstruct, true);
        SetLocation(&Declarator->TextSpan, DeclaratorStart, m_CurrentToken);

        if (ErrorInConstruct)
        {
            // If we see As or In before other query operators, then assume that
            // we are still on the Control Variable Declaration.
            // Otherwise, don't resync and allow the caller to
            // decide how to recover.

            peek = PeekAheadFor(15, tkAS, tkIN, tkComma, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN,tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);

            switch(peek)
            {
                case tkAS:
                case tkIN:
                case tkComma:

                    ResyncAt(1, peek);
                    break;
            }
        }

        Token *As = NULL;
        ParseTree::Type *Type = NULL;

        if ( Declarator->Name.IsNullable && 
            ( m_CurrentToken->m_TokenType == tkIN ||
              m_CurrentToken->m_TokenType == tkEQ ))
        {
            ReportSyntaxError(ERRID_NullableTypeInferenceNotSupported, FromItemStart, m_CurrentToken, ErrorInConstruct);
        }

        bool TokenFollowingAsWasIn = false;

        // Parse the type if specified
        if (m_CurrentToken->m_TokenType == tkAS)
        {
            As = m_CurrentToken;
            GetNextToken(); // get off AS

            if ( m_CurrentToken->m_TokenType == tkIN )
            {
                TokenFollowingAsWasIn = true;
            }
#if 0            
            EatNewLine();
#endif
            Type = ParseGeneralType(ErrorInConstruct);

            // try to recover
            if (ErrorInConstruct)
            {
                peek = PeekAheadFor(15, tkIN, tkComma, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,tkEQ,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);

                switch(peek)
                {
                    case tkEQ:
                    case tkIN:
                    case tkComma:

                        ResyncAt(1, peek);
                        break;
                }
            }
        }

        ParseTree::DeclaratorList *Declarators = NULL;
        ParseTree::DeclaratorList **DeclaratorTarget = &Declarators;

        CreateListElement(
                    DeclaratorTarget,
                    Declarator,
                    DeclaratorStart,
                    m_CurrentToken,
                    NULL);

        ParseTree::VariableDeclaration *Declaration = new(m_TreeStorage) ParseTree::VariableDeclaration;
        Declaration->Opcode = ParseTree::VariableDeclaration::NoInitializer;
        Declaration->HasSyntaxError = false;
        Declaration->Variables = Declarators;
        Declaration->Type = Type;

        SetLocationAndPunctuator
        (
                &Declaration->TextSpan,
                &Declaration->As,
                FromItemStart,
                m_CurrentToken,
                As
        );

        Result->ControlVariableDeclaration = Declaration;
        bool parseExpression = true;

        EatNewLineIfFollowedBy( tkIN ); // implicit line continuation before 'IN'

        if (AssignmentList)
        {
            Result->IsLetControlVar = true;

            if (m_CurrentToken->m_TokenType != tkEQ)
            {
                ReportSyntaxError(ERRID_ExpectedAssignmentOperator, m_CurrentToken, ErrorInConstruct);
                parseExpression = false;
            }
        }
        else
        {
            if (m_CurrentToken->m_TokenType != tkIN)
            {
                ReportSyntaxError(ERRID_ExpectedIn, m_CurrentToken, ErrorInConstruct);
                parseExpression = false;
            }
        }

        // parse expression
        if (parseExpression)
        {
            SetPunctuator(&Result->InOrEq, FromItemStart, m_CurrentToken);
            GetNextToken();

            if ( TokenFollowingAsWasIn == false || m_CurrentToken->m_TokenType == tkEQ )
            {
                EatNewLine(); // enable implicit LC after 'In' or '=' But not if somebody did from x as IN 
            }
            Result->Source = ParseExpression(ErrorInConstruct);
        }
        else
        {
            // error have been reported already
            Result->Source = ExpressionSyntaxError();
            SetExpressionLocation(Result->Source, m_CurrentToken, m_CurrentToken, NULL);
        }

        // try to recover
        if (ErrorInConstruct)
        {
            peek = PeekAheadFor(13, tkComma, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
            if ( peek == tkComma )
            {
                ResyncAt(1, peek);
            }
        }

        // check for list continuation
        AnotherFromExists = (m_CurrentToken->m_TokenType == tkComma);

        SetLocation(&Result->TextSpan, FromItemStart, m_CurrentToken);

        CreateListElement(
            FromList,
            Result,
            FromItemStart,
            m_CurrentToken,
            AnotherFromExists ? m_CurrentToken : NULL);

        if (AnotherFromExists)
        {
            GetNextToken();
            EatNewLine();
            continue;
        }

        break;
    }

    FixupListEndLocations(ResultFromList);
    SetLocation(&ResultFromList->TextSpan, FromListStart, m_CurrentToken);

    return ResultFromList;
}

/*********************************************************************
;ParsePotentialQuery

The parser determined that we might be on a query expression because we were on 'From' or
'Aggregate'  We now see if we actually are on a query expression and parse it if we are.
**********************************************************************/
ParseTree::Expression * // the query expression if this is in fact a query we are on
Parser::ParsePotentialQuery(
    _Out_ bool &ParsedQuery, // [out] true = that we were on a query and that we handled it / false = not a query
    _Out_ bool &ErrorInConstruct // [out] whether we encountered an error processing the query statement (assuming this actually is a query statement)
)
{
    // Look ahead and see if it looks like a query, i.e.
    // {AGGREGATE | FROM } <id>[?] {In | As | = }
    ParsedQuery = false; // assume that it isn't a query
    Token *Start = m_CurrentToken;
    tokens keyword = IdentifierAsKeyword( Start );
    Token *current = Start->m_Next;
    bool newLineAfterFrom = false;
    ParseTree::Expression *Term = NULL;

    if (current && current->IsEndOfLine())
    {
        if ( !NextLineStartsWith( tkEOL, false )) // we don't allow two EOLs in a row here
        {
            PeekNextLine();
            current = current->m_Next;
            newLineAfterFrom = true;
        }
    }

    if (current && (tkID == current->m_TokenType || current->IsKeyword())) 
    {
        current = current->m_Next;

        // Look ahead for 'IN' as it can start it's own line
        if ( current && current->m_TokenType == tkEOL )
        {
            PeekNextLine(current); 
            Token *FollowsEOL = current->m_Next;
            if ( FollowsEOL && FollowsEOL->m_TokenType == tkIN )
            {
                current  = FollowsEOL; // skip past the EOL to the tkIN
            }
        }

        // Skip '?'
        if (current && tkNullable == current->m_TokenType)
        {
            current = current->m_Next;
        }

        if ( current &&
            ( tkIN == current->m_TokenType || tkAS == current->m_TokenType || 
                ( !newLineAfterFrom && tkEQ == current->m_TokenType )))
        {
            if (keyword == tkAGGREGATE) 
            {
                Term = ParseAggregateQueryExpression(ErrorInConstruct);
            }
            else
            {
                AssertIfFalse(keyword == tkFROM);
                Term = ParseFromQueryExpression(false, ErrorInConstruct); 
            }

            ParsedQuery = true; // this was a query and we parsed it
        }
    }
    return Term;
}

ParseTree::LinqExpression *
Parser::ParseFromExpression
(
    bool StartingQuery,
    bool ImplicitFrom,
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(ImplicitFrom ||
        m_CurrentToken->m_TokenType == tkLET ||
        m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkFROM,
        "should be at From or Let.");

    Token *FromOrAggregateToken;
    if (ImplicitFrom)
    {
        // if it's Implicit, then it's an Aggregate: we need to set textspan and FirstPunctuator correctly
        if (m_CurrentToken->m_Prev->m_TokenType == tkEOL)    // EatNewLine could have skipped the tkEOL DevDiv 475078
        {
            FromOrAggregateToken = m_CurrentToken->m_Prev->m_Prev;
        }
        else
        {
            FromOrAggregateToken = m_CurrentToken->m_Prev;
        }
        VSASSERT(FromOrAggregateToken->m_TokenType == tkID && IdentifierAsKeyword(FromOrAggregateToken) == tkAGGREGATE,"expecting Aggregate token");
    }
    else
    {
        FromOrAggregateToken = m_CurrentToken;
    }

    ParseTree::FromExpression * From = new(m_TreeStorage) ParseTree::FromExpression;
    From->Opcode = (!ImplicitFrom && m_CurrentToken->m_TokenType == tkLET) ? ParseTree::Expression::Let : ParseTree::Expression::From;
    From->StartingQuery = StartingQuery;    // record whether this From starts the query

    SetPunctuator(&From->FirstPunctuator, FromOrAggregateToken, FromOrAggregateToken);   // set location so common code CIntelliSense::IsLinqKeywordHelper can handle all query clauses

    if (!ImplicitFrom)
    {
        GetNextToken(); //skip the tkFrom
        EatNewLine();
    }

    From->ImplicitFrom = ImplicitFrom;
    From->FromItems = ParseFromList(From->Opcode == ParseTree::Expression::Let, ErrorInConstruct);

    SetLocation(&From->TextSpan, FromOrAggregateToken, m_CurrentToken);

    // It's ok to call EatNewLineIfFollowedBy() multiple times.  If the first time it ate a new line, then the current token
    // will not be tkEOL (the scanner strips empty lines) and so the second call will not do anything. Secondly if the 
    // first one did not do anything and the current token was EOL then it will remain EOL.
    EatNewLineIfFollowedBy(tkFROM, true);
    EatNewLineIfFollowedBy(tkLET, true);

    if ((m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkFROM) ||
        m_CurrentToken->m_TokenType == tkLET)
    {
        return ParseCrossJoinExpression(From, ErrorInConstruct);
    }

    AssertIfNull(From);
    return From;
}

ParseTree::CrossJoinExpression *
Parser::ParseCrossJoinExpression
(
    ParseTree::LinqExpression * Source,
    _Inout_ bool &ErrorInConstruct
)
{
    AssertIfNull(Source);
    AssertIfFalse((m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkFROM) || m_CurrentToken->m_TokenType == tkLET);

    ParseTree::CrossJoinExpression *  crossJoin =  new(m_TreeStorage) ParseTree::CrossJoinExpression;
    crossJoin->Opcode = ParseTree::Expression::CrossJoin;
    crossJoin->Source = Source;
    crossJoin->JoinTo = ParseFromExpression(false, false, ErrorInConstruct);

    crossJoin->TextSpan.SetLocation(Source->TextSpan.m_lBegLine, Source->TextSpan.m_lBegColumn,
                                        crossJoin->JoinTo->TextSpan.m_lEndLine, crossJoin->JoinTo->TextSpan.m_lEndColumn);

    return crossJoin;
}

static void MakeSureLeftControlVarIsNamed(_In_opt_ ParseTree::LinqExpression * Source)
{
    while(Source)
    {
        switch(Source->Opcode)
        {
        case ParseTree::Expression::InnerJoin:
        case ParseTree::Expression::GroupJoin:
        case ParseTree::Expression::CrossJoin:
        case ParseTree::Expression::From:
        case ParseTree::Expression::Let:
        case ParseTree::Expression::GroupBy:
        case ParseTree::Expression::Aggregate:
        case ParseTree::Expression::LinqSource:
            return; // these operators do not produce unnamed vars

        case ParseTree::Expression::Where:
        case ParseTree::Expression::SkipWhile:
        case ParseTree::Expression::TakeWhile:
        case ParseTree::Expression::Take:
        case ParseTree::Expression::Skip:
        case ParseTree::Expression::Distinct:
        case ParseTree::Expression::OrderBy:
            Source = Source->AsLinqOperator()->Source; // these operators do not declare control vars
            break;

        case ParseTree::Expression::Select:
            AssertIfTrue(Source->AsSelect()->ForceNameInferenceForSingleElement);
            Source->AsSelect()->ForceNameInferenceForSingleElement = true;
            return; // done

         default:
                AssertIfFalse(false); // unknown Opcode
                return;
          }
    }
}

ParseTree::InnerJoinExpression *
Parser::ParseInnerJoinExpression
(
    _In_ Token * beginSourceToken,
    _In_opt_ ParseTree::LinqExpression * Source,
    _Inout_ bool &ErrorInConstruct
)
{
    AssertIfNull(Source);
    AssertIfFalse(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkJOIN);

    // Make sure control var on the left is always named
    MakeSureLeftControlVarIsNamed(Source);

    Token *JoinToken = m_CurrentToken;
    GetNextToken(); // get off JOIN

    ParseTree::InnerJoinExpression * innerJoin = new(m_TreeStorage) ParseTree::InnerJoinExpression;
    innerJoin->Opcode = ParseTree::Expression::InnerJoin;

    EatNewLine();
    
    innerJoin->Source = Source;
    innerJoin->JoinTo = ParseJoinSourceExpression(ErrorInConstruct);

    EatNewLineIfFollowedBy(tkON, true);
    Token *onToken = m_CurrentToken;

    if (m_CurrentToken->m_TokenType != tkON)
    {
        ReportSyntaxError(ERRID_ExpectedOn, m_CurrentToken, ErrorInConstruct);
    }
    else
    {
        SetPunctuator(&innerJoin->On, beginSourceToken, onToken);
        
        GetNextToken();
        EatNewLine();
        
        innerJoin->Predicate = ParseJoinPredicateExpression(ErrorInConstruct);
    }

    SetExpressionLocation(innerJoin, beginSourceToken, m_CurrentToken, JoinToken);

    if (innerJoin->Predicate)
    {
        innerJoin->TextSpan.SetEnd(&innerJoin->Predicate->TextSpan);// in case no items, like "Join" followed by spaces...
    }
    else
    {
        if (innerJoin->JoinTo && innerJoin->TextSpan.EndsBefore(&innerJoin->JoinTo->TextSpan))  // in case of expr ending in "On "
        {
            innerJoin->TextSpan.SetEnd(&innerJoin->JoinTo->TextSpan);// in case no items, like "Join" followed by spaces...
        }
    }

    return innerJoin;
}


ParseTree::GroupJoinExpression *
Parser::ParseGroupJoinExpression
(
    _In_ Token * beginSourceToken,
    _In_opt_ ParseTree::LinqExpression * Source,
    _Inout_ bool &ErrorInConstruct
)
{
    AssertIfNull(Source);
    AssertIfFalse(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkGROUP);

    // Make sure control var on the left is always named
    MakeSureLeftControlVarIsNamed(Source);

    Token *GroupToken = m_CurrentToken;

    GetNextToken(); // get off 'Group'

    Token *JoinToken = m_CurrentToken;
    bool HaveJoin = false;

    if (!(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkJOIN))
    {
        ReportSyntaxError(ERRID_ExpectedJoin, m_CurrentToken, ErrorInConstruct);
    }
    else
    {
        HaveJoin = true;
        GetNextToken();
        EatNewLine();
    }

    ParseTree::GroupJoinExpression *  groupJoin =  new(m_TreeStorage) ParseTree::GroupJoinExpression;
    groupJoin->Opcode = ParseTree::Expression::GroupJoin;
    groupJoin->Source = Source;
    groupJoin->HaveJoin = HaveJoin;
    groupJoin->JoinTo = ParseJoinSourceExpression(ErrorInConstruct);

    if (HaveJoin)
    {
        SetPunctuator(&groupJoin->Join, beginSourceToken, JoinToken);
    }

    EatNewLineIfFollowedBy(tkON, true);

    Token *onToken = m_CurrentToken;

    if (m_CurrentToken->m_TokenType != tkON)
    {
        ReportSyntaxError(ERRID_ExpectedOn, m_CurrentToken, ErrorInConstruct);
    }
    else
    {
        SetPunctuator(&groupJoin->On, beginSourceToken, onToken);
        GetNextToken();
        EatNewLine();
        
        groupJoin->Predicate = ParseJoinPredicateExpression(ErrorInConstruct);
    }

    EatNewLineIfFollowedBy(tkINTO, true);
    Token *intoToken = m_CurrentToken;

    if (!(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkINTO))
    {
        ReportSyntaxError(ERRID_ExpectedInto, m_CurrentToken, ErrorInConstruct);
    }
    else
    {
        SetPunctuator(&groupJoin->Into, beginSourceToken, intoToken);
        GetNextToken();

        EatNewLine();
        
        groupJoin->Projection = ParseAggregateList(true, true, ErrorInConstruct);
    }

    SetExpressionLocation(groupJoin, beginSourceToken, m_CurrentToken, GroupToken);

    if (groupJoin->Projection)
    {
        groupJoin->TextSpan.SetEnd(&groupJoin->Projection->TextSpan);// in case no items, like "Join" followed by spaces...
    }
    else
    {
        if (groupJoin->Predicate && groupJoin->TextSpan.EndsBefore(&groupJoin->Predicate->TextSpan))
        {
            groupJoin->TextSpan.SetEnd(&groupJoin->Predicate->TextSpan);// in case no items, like "Join" followed by spaces...
        }
        else
        {
            if (groupJoin->JoinTo && groupJoin->TextSpan.EndsBefore(&groupJoin->JoinTo->TextSpan))  // in case of expr ending in "On "
            {
                groupJoin->TextSpan.SetEnd(&groupJoin->JoinTo->TextSpan);// in case no items, like "Join" followed by spaces...
            }
        }
    }

    return groupJoin;
}

ParseTree::LinqSourceExpression *
Parser::ParseJoinControlVarExpression
(
    _Inout_ bool &ErrorInConstruct
)
{

    ParseTree::LinqSourceExpression * Result = new(m_TreeStorage) ParseTree::LinqSourceExpression;
    Result->Opcode = ParseTree::Expression::LinqSource;

    // Parse the control variable declaration. We always parse it as a declaration because
    // it doesn't bind to locals.
    Token *DeclaratorStart = m_CurrentToken;
    ParseTree::Declarator *Declarator = new(m_TreeStorage) ParseTree::Declarator;
    Declarator->Name = ParseIdentifier(ErrorInConstruct, true);
    SetLocation(&Declarator->TextSpan, DeclaratorStart, m_CurrentToken);

    EatNewLineIfFollowedBy( tkIN, Result ); // allow implicit new line before IN - Dev10_500708

    tokens peek;

    if (ErrorInConstruct)
    {
        // If we see As or In before other query operators, then assume that
        // we are still on the Control Variable Declaration.
        // Otherwise, don't resync and allow the caller to
        // decide how to recover.

        peek = PeekAheadFor(15, tkAS, tkIN, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN,
                            tkDISTINCT,tkAGGREGATE,tkINTO, tkON,tkSKIP,tkTAKE,tkLET);

        switch(peek)
        {
            case tkAS:
            case tkIN:
            case tkGROUP:
            case tkJOIN:
            case tkON:

                ResyncAt(1, peek);
                break;
        }
    }

    Token *As = NULL;
    ParseTree::Type *Type = NULL;

     if ( Declarator->Name.IsNullable && (
            m_CurrentToken->m_TokenType == tkIN ||
            m_CurrentToken->m_TokenType == tkEQ )
        )
    {
        ReportSyntaxError(ERRID_NullableTypeInferenceNotSupported, DeclaratorStart, m_CurrentToken, ErrorInConstruct);
    }

    // Parse the type if specified
    if (m_CurrentToken->m_TokenType == tkAS)
    {
        As = m_CurrentToken;

        // Parse the type
        GetNextToken(); // get off AS
        Type = ParseGeneralType(ErrorInConstruct);

        // try to recover
        if(ErrorInConstruct)
        {
            peek = PeekAheadFor(15, tkIN, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,
                        tkEQ,tkAGGREGATE,tkINTO, tkON,tkSKIP,tkTAKE,tkLET);

            switch(peek)
            {
                case tkEQ:
                case tkIN:
                case tkGROUP:
                case tkJOIN:
                case tkON:

                    ResyncAt(1, peek);
                    break;

            }
        }
    }

    ParseTree::DeclaratorList *Declarators = NULL;
    ParseTree::DeclaratorList **DeclaratorTarget = &Declarators;

    CreateListElement(
                DeclaratorTarget,
                Declarator,
                DeclaratorStart,
                m_CurrentToken,
                NULL);

    ParseTree::VariableDeclaration *Declaration = new(m_TreeStorage) ParseTree::VariableDeclaration;
    Declaration->Opcode = ParseTree::VariableDeclaration::NoInitializer;
    Declaration->HasSyntaxError = false;
    Declaration->Variables = Declarators;
    Declaration->Type = Type;

    SetLocationAndPunctuator(
            &Declaration->TextSpan,
            &Declaration->As,
            DeclaratorStart,
            m_CurrentToken,
            As);

    Result->ControlVariableDeclaration = Declaration;

    Token * inToken = m_CurrentToken;

    // parse collection
    if (m_CurrentToken->m_TokenType == tkIN)
    {
        GetNextToken(); // get off IN
        EatNewLine(); // dev10_500708 allow line continuation after 'IN'
        Result->Source = ParseExpression(ErrorInConstruct);
    }
    else
    {
        Result->Source = ExpressionSyntaxError();
        SetExpressionLocation(Result->Source, m_CurrentToken, m_CurrentToken, NULL);

        ReportSyntaxError(ERRID_ExpectedIn, m_CurrentToken, ErrorInConstruct);
    }

    // try to recover
    if (ErrorInConstruct)
    {
        peek = PeekAheadFor(13, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,
                            tkAGGREGATE,tkINTO, tkON,tkSKIP,tkTAKE,tkLET);

        switch(peek)
        {
            case tkGROUP:
            case tkJOIN:
            case tkON:
                ResyncAt(1, peek);
                break;

        }
    }

    SetExpressionLocation(Result, DeclaratorStart, m_CurrentToken, inToken);

    return Result;
}


ParseTree::LinqExpression *
Parser::ParseJoinSourceExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token * Start = m_CurrentToken;

    ParseTree::LinqExpression * Result = ParseJoinControlVarExpression(ErrorInConstruct);

    for(;;)
    {
        EatNewLineIfFollowedBy(tkJOIN, true);
        EatNewLineIfFollowedBy(tkGROUP, true);
    
        if(m_CurrentToken->m_TokenType == tkID)
        {
            switch(IdentifierAsKeyword(m_CurrentToken))
            {
            case tkJOIN:
                Result =  ParseInnerJoinExpression(Start,  Result, ErrorInConstruct);
                continue;

            case tkGROUP: // must be a 'Group Join'
                Result =  ParseGroupJoinExpression(Start,  Result, ErrorInConstruct);
                continue;
            }
        }

        break;
    }

    return Result;
}


ParseTree::Expression *
Parser::ParseJoinPredicateExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::BinaryExpression *AndExpr = NULL;
    Token * AndStart = m_CurrentToken;
    Token * AndOperator = NULL;

    do
    {
        Token * Start = m_CurrentToken;
        ParseTree::BinaryExpression *Binary = NULL;
        if (Start->m_TokenType != tkEOL)
        {

            Binary = new(m_TreeStorage) ParseTree::BinaryExpression;
            Binary->Opcode = ParseTree::Expression::Equals;

            Binary->Left = ParseExpression(PrecedenceRelational, ErrorInConstruct);

            // try to recover
            if (ErrorInConstruct)
            {
                ResyncAt(18, tkEQUALS, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,tkAGGREGATE,tkINTO, tkON, tkAND, tkANDALSO, tkOR, tkORELSE,tkSKIP,tkTAKE,tkLET);
            }

            Token *Operator = m_CurrentToken;

            if(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken)==tkEQUALS)
            {
                GetNextToken();
                EatNewLine();
                Binary->Right = ParseExpression(PrecedenceRelational, ErrorInConstruct);
            }
            else
            {
                ReportSyntaxError(ERRID_ExpectedEquals, m_CurrentToken, ErrorInConstruct);
                Binary->Right = ExpressionSyntaxError();
            }

            SetExpressionLocation(Binary, Start, m_CurrentToken, Operator);

        }else
        {
            ReportSyntaxError(ERRID_ExpectedExpression, m_CurrentToken, ErrorInConstruct);
        }
        // try to recover
        if (ErrorInConstruct)
        {
            ResyncAt(17, tkAND, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,tkAGGREGATE,tkINTO, tkON, tkANDALSO, tkOR, tkORELSE,tkSKIP,tkTAKE,tkLET);
        }

        if(AndExpr)
        {
            ParseTree::BinaryExpression *newAnd = new(m_TreeStorage) ParseTree::BinaryExpression;
            newAnd->Opcode = ParseTree::Expression::And;
            newAnd->Left = AndExpr;
            newAnd->Right = Binary ? Binary : ExpressionSyntaxError();
            SetExpressionLocation(newAnd, AndStart, m_CurrentToken, AndOperator);

            AndExpr = newAnd;
        }
        else
        {
            AndExpr = Binary;
        }

        AndOperator = NULL;

        switch(m_CurrentToken->m_TokenType)
        {
            case  tkAND:
                AndOperator = m_CurrentToken;
                GetNextToken();
                EatNewLine();
                break;
            case  tkANDALSO:
            case  tkOR:
            case  tkORELSE:
                ReportSyntaxError(ERRID_ExpectedAnd, m_CurrentToken, ErrorInConstruct);
                    break;
        }
    }
    while(AndOperator);
    
    // try to recover
    if (ErrorInConstruct)
    {
        ResyncAt(13, tkFROM, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkDISTINCT,tkAGGREGATE,tkINTO, tkON,tkSKIP,tkTAKE,tkLET);
    }

    return AndExpr;
}


ParseTree::OrderByList *
Parser::ParseOrderByList
(
    _Inout_ bool &ErrorInConstruct // [out] whether we ran into an error processing the OrderBy list.
)
{
    ParseTree::OrderByList *ResultOrderByList = new(m_TreeStorage) ParseTree::OrderByList;
    ParseTree::OrderByList **OrderByList = &ResultOrderByList;

    Token *OrderByListStart = m_CurrentToken;

    for(;;)
    {
        ParseTree::OrderByItem *Result = new(m_TreeStorage) ParseTree::OrderByItem;
        Token *OrderByStart = m_CurrentToken;

        Result->OrderExpression = ParseExpression(ErrorInConstruct);

        // try to recover
        if (ErrorInConstruct)
        {
            ResyncAt(15, tkComma, tkASCENDING, tkDESCENDING, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkFROM, tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
        }

        EatNewLineIfFollowedBy(tkASCENDING, true); 
        EatNewLineIfFollowedBy(tkDESCENDING, true);

        if (m_CurrentToken->m_TokenType == tkID)
        {
            if (IdentifierAsKeyword(m_CurrentToken) == tkASCENDING)
            {
                SetPunctuator(&Result->AscendingDescending, OrderByStart, m_CurrentToken);
                GetNextToken();
#if 0
                EatNewLine();
#endif
                Result->IsAscending = true;
            }
            else if (IdentifierAsKeyword(m_CurrentToken) == tkDESCENDING)
            {
                SetPunctuator(&Result->AscendingDescending, OrderByStart, m_CurrentToken);
                GetNextToken();
#if 0
                EatNewLine();
#endif
                Result->IsDescending = true;
            }
        }

        SetLocation(&Result->TextSpan, OrderByStart, m_CurrentToken);

        EatNewLineIfFollowedBy(tkComma, true); 
        if (m_CurrentToken->m_TokenType == tkComma)
        {
            // another item exists
            Token *comma =  m_CurrentToken;
            GetNextToken();

            CreateListElement(
                OrderByList,
                Result,
                OrderByStart,
                m_CurrentToken,
                comma);

            EatNewLine();

            continue;
        }
        else
        {
            CreateListElement(
                OrderByList,
                Result,
                OrderByStart,
                m_CurrentToken,
                NULL);
        }
        break;
    }

    FixupListEndLocations(ResultOrderByList);
    SetLocation(&ResultOrderByList->TextSpan, OrderByListStart, m_CurrentToken);

    return ResultOrderByList;
}

ParseTree::AggregateExpression *
Parser::ParseAggregateQueryExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    AssertIfFalse(m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkAGGREGATE);

    ParseTree::AggregateExpression * Aggregate = new(m_TreeStorage) ParseTree::AggregateExpression;
    Aggregate->Opcode = ParseTree::Expression::Aggregate;

    Token *Start = m_CurrentToken;
    GetNextToken();  // Skip 'Aggregate'
    EatNewLine();

    Aggregate->AggSource = ParseFromQueryExpression(true, ErrorInConstruct);

    EatNewLineIfFollowedBy(tkINTO, true);

    if (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkINTO)
    {
        SetPunctuator(&Aggregate->Into, Start, m_CurrentToken);
        Aggregate->HaveInto = true;
        GetNextToken();

        //ILC:  undone:
        //        I took the liberty of adding implicit line continuations after query keywords in addition to before them...
        EatNewLine();

        // parse result selector
        Aggregate->Projection = ParseAggregateList(false, false, ErrorInConstruct);
    }
    else
    {
        ReportSyntaxError(ERRID_ExpectedInto, m_CurrentToken, ErrorInConstruct);
    }

    SetExpressionLocation(Aggregate, Start, m_CurrentToken,Start);
    if (Aggregate->Projection)
    {
        Aggregate->TextSpan.SetEnd(&Aggregate->Projection->TextSpan);   // in case no items, like "Aggregate x in "" Into" followed by spaces....
    }

    return Aggregate;
}

bool
Parser::IsContinuableQueryOperator
(
    Token * pToken
)
{
    if (pToken)
    {
        tokens tokenType  = pToken->m_TokenType;

        if (tokenType == tkID)
        {
            tokenType = IdentifierAsKeyword(pToken);
        }

        bool isQueryKwd = g_tkKwdDescs[tokenType].kdIsQueryClause;

        if (isQueryKwd && tokenType == tkSELECT)
        {
            //We do not want to allow an implicit line continuation before a "select" keyword if it is immedietly
            //followed by the "case" keyword. This allows code like the following to parse correctly:
            //    dim a = from x in xs
            //    select case b
            //    end select
            if (pToken->m_Next && pToken->m_Next->m_TokenType == tkCASE)
            {
                isQueryKwd = false;
            }
        }

        return isQueryKwd;
    }
    else
    {
        return false;
    }
}


ParseTree::LinqExpression *
Parser::ParseFromQueryExpression
(
    bool ImplicitFrom,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

#if LINQ_INTELLISENSE
    m_IsSelectExpression = true;
#endif LINQ_INTELLISENSE

    ParseTree::LinqExpression * From = ParseFromExpression(true, ImplicitFrom, ErrorInConstruct);
    ParseTree::LinqExpression * Query = From;

    AssertIfNull(Query)
    m_SeenQueryExpression = true;
    bool fContinue=true;

    do
    {
        // try to recover
        if (ErrorInConstruct)
        {
            ResyncAt(12, tkWHERE, tkGROUP, tkSELECT, tkORDER, tkJOIN, tkFROM, tkDISTINCT,tkAGGREGATE,tkINTO,tkSKIP,tkTAKE,tkLET);
        }

        switch (m_CurrentToken->m_TokenType)
        {                
            case tkID:
                // handle contectual keywords here
                switch (IdentifierAsKeyword(m_CurrentToken))
                {
                    case tkWHERE:
                    {
                        Token * whereToken = m_CurrentToken;
                        GetNextToken();

                        ParseTree::WhereExpression *Where = new(m_TreeStorage) ParseTree::WhereExpression;
                        Where->Opcode = ParseTree::Expression::Where;

                        EatNewLine();                               

                        Where->Source = Query;
                        Where->Predicate = ParseExpression(ErrorInConstruct);
                        SetExpressionLocation(Where, Start, m_CurrentToken, whereToken);
                        Where->TextSpan.SetEnd(&Where->Predicate->TextSpan);    // in case no items found, like "Where" followed by just spaces
                        Query = Where;
                    }
                    break;

                    case tkSKIP:
                    {
                        Token * skipToken = m_CurrentToken;
                        GetNextToken();

                        if (m_CurrentToken->m_TokenType == tkWHILE)
                        {
                            // Note: no line continuation if Skip is followed by While, i.e. Skip While is treated as a unit
                            ParseTree::WhileExpression *skipWhile = new(m_TreeStorage) ParseTree::WhileExpression;
                            skipWhile->Opcode = ParseTree::Expression::SkipWhile;
                            skipWhile->Source = Query;

                            SetPunctuator(&skipWhile->While, Start, m_CurrentToken);
                            GetNextToken();

                            EatNewLine();

                            skipWhile->Predicate = ParseExpression(ErrorInConstruct);
                            SetExpressionLocation(skipWhile, Start, m_CurrentToken, skipToken);
                            skipWhile->TextSpan.SetEnd(&skipWhile->Predicate->TextSpan);    // in case no items found, like "Skip" followed by just spaces

                            Query = skipWhile;
                        }
                        else
                        {
                            ParseTree::SkipTakeExpression *skip = new(m_TreeStorage) ParseTree::SkipTakeExpression;
                            skip->Opcode = ParseTree::Expression::Skip;
                            skip->Source = Query;
                            
                            EatNewLineIfNotFollowedBy(tkWHILE, true); // when Skip ends the line, allow a implicit line continuation

                            skip->Count = ParseExpression(ErrorInConstruct);
                            SetExpressionLocation(skip, Start, m_CurrentToken, skipToken);
                            skip->TextSpan.SetEnd(&skip->Count->TextSpan);  // in case no items found, like "skip" followed by just spaces

                            Query = skip;
                        }
                    }
                    break;

                    case tkTAKE:
                    {
                        Token * takeToken = m_CurrentToken;
                        GetNextToken();

                        if (m_CurrentToken->m_TokenType == tkWHILE)
                        {
                            // Note: no line continuation if TAKE is followed by WHILE, i.e. Take While is treated as a unit
                            ParseTree::WhileExpression *takeWhile = new(m_TreeStorage) ParseTree::WhileExpression;
                            takeWhile->Opcode = ParseTree::Expression::TakeWhile;
                            takeWhile->Source = Query;

                            SetPunctuator(&takeWhile->While, Start, m_CurrentToken);
                            GetNextToken();

                            EatNewLine();                                   

                            takeWhile->Predicate = ParseExpression(ErrorInConstruct);
                            SetExpressionLocation(takeWhile, Start, m_CurrentToken, takeToken);
                            takeWhile->TextSpan.SetEnd(&takeWhile->Predicate->TextSpan);    // in case no items found, like "Take" followed by just spaces

                            Query = takeWhile;
                        }
                        else
                        {                                     
                            ParseTree::SkipTakeExpression *take = new(m_TreeStorage) ParseTree::SkipTakeExpression;
                            take->Opcode = ParseTree::Expression::Take;
                            take->Source = Query;

                            EatNewLineIfNotFollowedBy(tkWHILE, true); // when TAKE ends the line, allow a implicit line continuation

                            take->Count = ParseExpression(ErrorInConstruct);
                            SetExpressionLocation(take, Start, m_CurrentToken, takeToken);
                            take->TextSpan.SetEnd(&take->Count->TextSpan);  //// in case no items found, like "take" followed by just spaces

                            Query = take;
                        }
                    }
                    break;

                    case tkGROUP:
                    {
                        Token * pLookAhead = NULL;

                        // See if this is a 'Group Join'
                        if ( m_CurrentToken->m_Next &&
                            (
                                (
                                    m_CurrentToken->m_Next->m_TokenType == tkID && 
                                    IdentifierAsKeyword(m_CurrentToken->m_Next) == tkJOIN
                                ) ||
                                (
                                    // Did they put join on the next line? Check so we can get to the parsing 
                                    // logic that can give us a good error at least.
                                    m_CurrentToken->m_Next->IsContinuableEOL() &&
                                    (pLookAhead = PeekNextLine()) &&
                                    pLookAhead->m_TokenType == tkID &&
                                    IdentifierAsKeyword(pLookAhead) == tkJOIN
                                )
                            )
                        )
                        {
                            EatNewLine();
                            Query = ParseGroupJoinExpression(Start, Query, ErrorInConstruct);
                        }
                        else
                        {
                            Token * groupToken = m_CurrentToken;
                            GetNextToken();

                            ParseTree::GroupByExpression *Group = new(m_TreeStorage) ParseTree::GroupByExpression;
                            Group->Opcode = ParseTree::Expression::GroupBy;
                            Group->Source = Query;

                            EatNewLine(); 

                            if (! (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkBY))
                            {
                                // parse element selector
                                Group->Element = ParseSelectList(ErrorInConstruct);

                                EatNewLineIfFollowedBy(tkBY, true);
                            }

                            if (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkBY)
                            {
                                SetPunctuator(&Group->By, Start, m_CurrentToken);
                                Group->HaveBy = true;
                                GetNextToken();

                                EatNewLine();

                                // parse key selector
                                Group->Key = ParseSelectList(ErrorInConstruct);
                            }
                            else
                            {
                                ReportSyntaxError(ERRID_ExpectedBy, m_CurrentToken, ErrorInConstruct);
                            }

                            EatNewLineIfFollowedBy(tkINTO, true);
                            
                            if (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkINTO)
                            {
                                SetPunctuator(&Group->Into, Start, m_CurrentToken);
                                Group->HaveInto = true;
                                GetNextToken(); // Move off tkInto

                                EatNewLine(); // allow implicit line continuation after Into                                   

                                // parse result selector
                                Group->Projection = ParseAggregateList(true, false, ErrorInConstruct);
                            }
                            else
                            {
                                ReportSyntaxError(ERRID_ExpectedInto, m_CurrentToken, ErrorInConstruct);
                            }

                            SetExpressionLocation(Group, Start, m_CurrentToken, groupToken);
                            if (Group->Projection)  // in case no items found, like "Group" followed by just spaces
                            {
                                Group->TextSpan.SetEnd(&Group->Projection->TextSpan);
                            }
                            else
                            {
                                if (Group->Key && Group->TextSpan.EndsBefore(&Group->Key->TextSpan))
                                {
                                    Group->TextSpan.SetEnd(&Group->Key->TextSpan);
                                }
                                else
                                {
                                    if (Group->Element && Group->Element->TextSpan.EndsBefore(&Group->Element->TextSpan))
                                    {
                                        Group->TextSpan.SetEnd(&Group->Element->TextSpan);
                                    }
                                }
                            }
                            Query = Group;
                        }
                        break;
                    }    

                    case tkAGGREGATE:
                    {
                        Token * aggToken = m_CurrentToken;
                        ParseTree::AggregateExpression * Aggregate = ParseAggregateQueryExpression(ErrorInConstruct);

                        Aggregate->Source = Query;

                        // Fixup location
                        AssertIfFalse(Aggregate->FirstPunctuator.Line==0);
                        SetExpressionLocation(Aggregate, Start, m_CurrentToken, aggToken);
                        Aggregate->Into.Line += Aggregate->FirstPunctuator.Line;

                        if (Aggregate->Projection)
                        {
                            Aggregate->TextSpan.SetEnd(&Aggregate->Projection->TextSpan);   // in case no items, like "Aggregate x in "" Into" followed by spaces....
                        }

                        Query = Aggregate;
                    }
                    break;

                    case tkORDER:
                    {
                        Token * orderToken = m_CurrentToken;
                        GetNextToken();

                        ParseTree::OrderByExpression *Order = new(m_TreeStorage) ParseTree::OrderByExpression;
                        Order->Opcode = ParseTree::Expression::OrderBy;
                        Order->Source = Query;

                        if (m_CurrentToken->m_TokenType == tkID && IdentifierAsKeyword(m_CurrentToken) == tkBY)
                        {
                            Order->WithBy = true;

                            SetPunctuator(&Order->By, Start, m_CurrentToken);
                            GetNextToken();

                            EatNewLine();
                        }
                        else
                        {
                            ReportSyntaxErrorWithoutMarkingStatementBad(ERRID_ExpectedBy, m_CurrentToken, ErrorInConstruct);
                        }

                        Order->OrderByItems = ParseOrderByList(ErrorInConstruct);
                        SetExpressionLocation(Order, Start, m_CurrentToken, orderToken);
                        Order->TextSpan.SetEnd(&Order->OrderByItems->TextSpan); // in case no items, like "Order by" followed by spaces...
                        Query = Order;
                    }
                    break;

                    case tkDISTINCT:
                    {
                        Token * distinctToken = m_CurrentToken;
                        GetNextToken();

                        ParseTree::DistinctExpression *Distinct = new(m_TreeStorage) ParseTree::DistinctExpression;
                        Distinct->Opcode = ParseTree::Expression::Distinct;
                        Distinct->Source = Query;
                        SetExpressionLocation(Distinct, Start, m_CurrentToken, distinctToken);

                        EatNewLine();   

                        Query = Distinct;
                    }
                    break;

                    case tkJOIN:
                        Query = ParseInnerJoinExpression(Start, Query, ErrorInConstruct);
                        break;

                    case tkFROM:
                        Query = ParseCrossJoinExpression(Query, ErrorInConstruct);
                        break;

                    default:
                        fContinue = false;
                        break;
                }

                break;

            case tkSELECT:
            {
                Token *SelectToken = m_CurrentToken;
                GetNextToken();                       

                ParseTree::SelectExpression *Select = new(m_TreeStorage) ParseTree::SelectExpression;
                Select->Opcode = ParseTree::Expression::Select;
                Select->Source = Query;
                Query = Select;

                EatNewLine();

                Select->Projection = ParseSelectList(ErrorInConstruct);

                SetExpressionLocation(Select, Start, m_CurrentToken, SelectToken);
                Select->TextSpan.SetEnd(&Select->Projection->TextSpan); // in case no items found, like "Select" followed by just spaces
                VSASSERT(Select->TextSpan.ContainsInclusive(&Select->Projection->TextSpan),"Inner textspan not included by outer");
            }
            break;

            case tkLET:
                Query = ParseCrossJoinExpression(Query, ErrorInConstruct);
                break;
            case tkEOL:
            {
                if ( !NextLineStartsWith( tkEOL, false ))
                {
                    Token * pNextLine = PeekNextLine();
                    if (IsContinuableQueryOperator(pNextLine))
                    {
                        //We allow implicit line continuations before query keywords when in a query context.
                        AssertLanguageFeature(FEATUREID_LineContinuation, m_CurrentToken);
                        GetNextToken(); // get off the EOL
                        break;
                    }
                }
                __fallthrough; // Fall through to default:
            }
            default:
                fContinue = false;
                break;
        }
    } while(fContinue);

#if LINQ_INTELLISENSE
    m_IsSelectExpression = false;
#endif LINQ_INTELLISENSE

    return Query;
}

ParseTree::Expression *
Parser::ParseGetType
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkGETTYPE,
        "should be at GetType.");

    Token *Start = m_CurrentToken;
    GetNextToken();

    Token *LeftParen = m_CurrentToken;
    ParseTree::GetTypeExpression *Result = new(m_TreeStorage) ParseTree::GetTypeExpression;    
    Result->Opcode = ParseTree::Expression::GetType;
    
    VerifyTokenPrecedesExpression(tkLParen, ErrorInConstruct, true, Result);

    // Dev10 #526220 Note, we are passing 'true' here to allow empty type arguments, 
    // this may be incorrect for a Nullable type when '?' is used at the end of the type name. 
    // In this case, we may generate some spurious errors and fail to report an expected error. 
    // In order to deal with this issue, Parser::ParseTypeName() will walk the type tree and 
    // report expected errors, removing false errors at the same time.
    Result->TypeToGet = ParseGeneralType(ErrorInConstruct, true /* Allow type arguments to be empty i.e. allow C1(Of , ,)*/ );
    EatNewLineIfFollowedBy(tkRParen, Result);

    VerifyExpectedToken(tkRParen, ErrorInConstruct);

    SetExpressionLocation(Result, Start, m_CurrentToken, LeftParen);

    return Result;
}

ParseTree::Expression *
Parser::ParseGetXmlNamespace
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkGETXMLNAMESPACE,
        "should be at GetXmlNamespace.");

    Token *Start = m_CurrentToken;
    GetNextToken();

    Token *LeftParen = m_CurrentToken;
    VerifyExpectedToken(tkLParen, ErrorInConstruct);

    ParseTree::GetXmlNamespaceExpression *Result = new(m_TreeStorage) ParseTree::GetXmlNamespaceExpression;
    Result->Opcode = ParseTree::Expression::GetXmlNamespace;

    if (m_CurrentToken->m_TokenType == tkXmlNCName)
    {
        Result->Prefix = CreateXmlNameIdentifier(m_CurrentToken, ErrorInConstruct);
        GetNextToken();

        VerifyExpectedToken(tkRParen, ErrorInConstruct);
    }
    else if (m_CurrentToken->m_TokenType == tkRParen)
    {
        Result->Prefix = ParserHelper::CreateIdentifierDescriptor(STRING_CONST2(m_pStringPool, EmptyString));
        GetNextToken();
    }
    else
    {
        Result->Prefix.IsBad = true;
        HandleUnexpectedToken(tkXmlNCName, ErrorInConstruct);
    }

    SetExpressionLocation(Result, Start, m_CurrentToken, LeftParen);

    return Result;
}

ParseTree::Expression *
Parser::ParseCastExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::UnaryExpression *Cast = new(m_TreeStorage) ParseTree::UnaryExpression;

    switch (Start->m_TokenType)
    {
        case tkCBOOL:
            Cast->Opcode = ParseTree::Expression::CastBoolean;
            break;
        case tkCCHAR:
            Cast->Opcode = ParseTree::Expression::CastCharacter;
            break;
        case tkCDATE:
            Cast->Opcode = ParseTree::Expression::CastDate;
            break;
        case tkCDBL:
            Cast->Opcode = ParseTree::Expression::CastDouble;
            break;
        case tkCSBYTE:
            Cast->Opcode = ParseTree::Expression::CastSignedByte;
            break;
        case tkCBYTE:
            Cast->Opcode = ParseTree::Expression::CastByte;
            break;
        case tkCSHORT:
            Cast->Opcode = ParseTree::Expression::CastShort;
            break;
        case tkCUSHORT:
            Cast->Opcode = ParseTree::Expression::CastUnsignedShort;
            break;
        case tkCINT:
            Cast->Opcode = ParseTree::Expression::CastInteger;
            break;
        case tkCUINT:
            Cast->Opcode = ParseTree::Expression::CastUnsignedInteger;
            break;
        case tkCLNG:
            Cast->Opcode = ParseTree::Expression::CastLong;
            break;
        case tkCULNG:
            Cast->Opcode = ParseTree::Expression::CastUnsignedLong;
            break;
        case tkCSNG:
            Cast->Opcode = ParseTree::Expression::CastSingle;
            break;
        case tkCSTR:
            Cast->Opcode = ParseTree::Expression::CastString;
            break;
        case tkCDEC:
            Cast->Opcode = ParseTree::Expression::CastDecimal;
            break;
        case tkCOBJ:
            Cast->Opcode = ParseTree::Expression::CastObject;
            break;
        default:
            VSASSERT(false, "Cast expressions must start with a cast token.");
            break;
    }

    GetNextToken();

    Token *LeftParen = m_CurrentToken;
    VerifyExpectedToken(tkLParen, ErrorInConstruct, true, Cast);

    Cast->Operand = ParseExpression(ErrorInConstruct);

    EatNewLineIfFollowedBy(tkRParen, Cast);
    VerifyExpectedToken(tkRParen, ErrorInConstruct);

    SetExpressionLocation(Cast, Start, m_CurrentToken, LeftParen);

    return Cast;
}

ParseTree::Expression *
Parser::ParseNewExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkNEW,
        "must be at a New expression.");

    Token *Start = m_CurrentToken;
    GetNextToken(); // get off 'new'

    if (tkWITH == m_CurrentToken->m_TokenType)
    {
        // Anonymous type initializer
        // Parsing from "With" in "New With {Initializer list}"

        ParseTree::NewObjectInitializerExpression *NewObjectInit =
            new(m_TreeStorage) ParseTree::NewObjectInitializerExpression;

        NewObjectInit->Opcode = ParseTree::Expression::NewObjectInitializer;
        NewObjectInit->NewExpression = NULL;
        SetPunctuator(&NewObjectInit->With, Start, m_CurrentToken);

        GetNextToken(); // Get off WITH
        EatNewLine(); // Dev10 #766620 allow implicit line continuation after WITH

        NewObjectInit->InitialValues =
            ParseInitializerList(
                ErrorInConstruct,
                true,   // allow expressions
                true,   // allow assignments
                true); // AnonymousTypeInitializer

        SetExpressionLocation(NewObjectInit, Start, m_CurrentToken, NULL);
        m_SeenAnonymousTypeInitialization = true;

        return NewObjectInit;
    }

    ParseTree::Type *Type = NULL;
    ParseTree::ParenthesizedArgumentList Arguments;
    Token *TypeStart = m_CurrentToken;
    Type = ParseTypeName(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(1, tkLParen);
    }

    if (m_CurrentToken->m_TokenType == tkLParen)
    {
        // This is an ambiguity in the grammar between
        //
        // New <Type> ( <Arguments> )
        // New <Type> <ArrayDeclaratorList> <AggregateInitializer>
        //
        // Try it as the first form, and if this fails, try the second.
        // (All valid instances of the second form have a beginning that is a valid
        // instance of the first form, so no spurious errors should result.)

        bool parentRedimOrNew = m_RedimOrNew; // save old flag to preserve recursivity
        m_RedimOrNew = true;

        Token *ArgumentsStart = m_CurrentToken;
        bool ErrorsAlreadyDisabled = DisableErrors();

        // Try parsing the arguments to determine whether they are constructor
        // arguments or array declarators.
        {
            bool TempErrorInConstruct = ErrorInConstruct;

            Arguments = ParseParenthesizedArguments(TempErrorInConstruct);

            EnableErrors(ErrorsAlreadyDisabled);
        }

        if (m_CurrentToken->m_TokenType == tkLBrace ||
            m_CurrentToken->m_TokenType == tkLParen)
        {
            // Treat this as the form of New expression that allocates an array.

            m_CurrentToken = ArgumentsStart;

            Type =
                ParseArrayDeclarator(
                    TypeStart,
                    Type,
                    true,
                    false,
                    ErrorInConstruct);

            if (m_CurrentToken->m_TokenType != tkLBrace)
            {
                HandleUnexpectedToken(tkLBrace, ErrorInConstruct);

                ParseTree::Expression *BadExpr = ExpressionSyntaxError();
                SetExpressionLocation(BadExpr, Start, m_CurrentToken, NULL);
                return BadExpr;
            }
            else
            {
                ParseTree::NewArrayInitializerExpression *NewArray =
                    new(m_TreeStorage) ParseTree::NewArrayInitializerExpression;

                NewArray->Opcode = ParseTree::Expression::NewArrayInitializer;
                NewArray->ArrayType = Type->AsArray();
                NewArray->Elements =
                    ParseInitializerList(ErrorInConstruct, true /* allow expressions */, false /* disallow assignments */);

                SetExpressionLocation(NewArray, Start, m_CurrentToken, NULL);
                return NewArray;
            }
        }
        else
        {
            // #28507 - DevDiv Bugs: do not reparse arguments if we don't care about errors
            if(!ErrorsAlreadyDisabled)
            {
                m_CurrentToken = ArgumentsStart;
                Arguments = ParseParenthesizedArguments(ErrorInConstruct);
            }
        }
    }

    ParseTree::NewExpression *New = new(m_TreeStorage) ParseTree::NewExpression;
    New->Opcode = ParseTree::Expression::New;

    New->InstanceType = Type;
    New->Arguments = Arguments;

    SetExpressionLocation(New, Start, m_CurrentToken, NULL);
    
    if (TokenAsKeyword(m_CurrentToken) == tkFROM)
    {
        Token *FromToken = m_CurrentToken;
        AssertLanguageFeature(FEATUREID_CollectionInitializers, FromToken);
        GetNextToken(); // consume the FROM
        EatNewLineIfFollowedBy(tkLBrace, New);

        if (m_CurrentToken->m_TokenType == tkLBrace)
        {
            ParseTree::CollectionInitializerExpression * ColInit =
                new (m_TreeStorage) ParseTree::CollectionInitializerExpression;
            ColInit->Opcode = ParseTree::Expression::CollectionInitializer;
            ColInit->NewExpression = New;
            SetPunctuator(&ColInit->From, Start, FromToken);

            ColInit->Initializer = 
                ParseInitializerList
                (
                    ErrorInConstruct, 
                    true, //allow expressions, 
                    false //do not allow assignments
                );

            SetExpressionLocation(ColInit, Start, m_CurrentToken, NULL);
            if (m_CurrentToken->m_TokenType == tkWITH)
            {
                ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, m_CurrentToken, ErrorInConstruct);
            }
            return ColInit;
        }
        else
        {
            // recant parsing ahead on the From to remain compatible with the state this func left things in
            // before I refactored it for implicit line continuation
            m_CurrentToken = FromToken; 
        }   
    }
    
    if (m_CurrentToken->m_TokenType == tkWITH)
    {
        // Parsing from "With" in "New Type(ParameterList) with {Initializer list}"

        ParseTree::NewObjectInitializerExpression *NewObjectInit =
            new(m_TreeStorage) ParseTree::NewObjectInitializerExpression;

        NewObjectInit->Opcode = ParseTree::Expression::NewObjectInitializer;
        NewObjectInit->NewExpression = New;
        SetPunctuator(&NewObjectInit->With, Start, m_CurrentToken);

        GetNextToken(); // Get off WITH
        EatNewLine(); // Dev10 #766620 allow implicit line continuation after WITH


        NewObjectInit->InitialValues =
            ParseInitializerList(
                ErrorInConstruct,
                false,  // disallow expressions
                true,   // allow assignments
                false,  // Not an anonymous type initializer
                true);  // require at least one initializer in the list

        SetExpressionLocation(NewObjectInit, Start, m_CurrentToken, NULL);

        if (TokenAsKeyword(m_CurrentToken)== tkFROM && m_CurrentToken->m_Next->m_TokenType == tkLBrace)
        {
            ReportSyntaxError(ERRID_CantCombineInitializers, m_CurrentToken, m_CurrentToken, ErrorInConstruct);
        }

        return NewObjectInit;
    }

    return New;
}

/*********************************************************************
*
* Function:
*     Parser::ParseTypeOf
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseTypeOf
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::TypeValueExpression *Result = new(m_TreeStorage) ParseTree::TypeValueExpression;
    Result->Opcode = ParseTree::Expression::IsType;

    Token *Start = m_CurrentToken;

    VSASSERT(m_CurrentToken->m_TokenType == tkTYPEOF, "must be at TypeOf.");

    // Consume 'TypeOf'.
    GetNextToken();

    Result->Value = ParseVariable(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(1, tkIS);
    }

    Token *Is = m_CurrentToken;
    if (VerifyExpectedToken(tkIS, ErrorInConstruct))
    {
        Result->HasIs = true;
        EatNewLine(); // allow implicit line continuation after 'IS' - dev10_504582
    }

    Result->TargetType = ParseGeneralType(ErrorInConstruct);

    SetExpressionLocation(Result, Start, m_CurrentToken, Is);
    return Result;
}

/*********************************************************************
*
* Function:
*     Parser::ParseVariable
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseVariable
(
    _Inout_ bool &ErrorInConstruct
)
{
    return ParseExpression(PrecedenceRelational, ErrorInConstruct);
}

/*********************************************************************
*
* Function:
*     Parser::ParseQualifiedExpr
*
* Purpose:
*     Parses a dot or bang reference, starting at the dot or bang.
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseQualifiedExpr
(
    _In_ Token *Start,                   // [in] token starting term
    _In_opt_ ParseTree::Expression *Term,    // [in] stuff before "." or "!"
    _Inout_ bool &ErrorInConstruct
)
{
    Token *DotOrBangToken = m_CurrentToken;
    VSASSERT( DotOrBangToken->m_TokenType == tkDot ||
              DotOrBangToken->m_TokenType == tkBang,
              "Must be on either a '.' or '!' when entering parseQualifiedExpr()");
    GetNextToken();

    ParseTree::QualifiedExpression *Qualified = new(m_TreeStorage) ParseTree::QualifiedExpression;
    Qualified->Base = Term;

    if (DotOrBangToken->m_TokenType == tkBang)
    {
        Qualified->Opcode = ParseTree::Expression::BangQualified;
        Qualified->Name = ParseIdentifierExpressionAllowingKeyword(ErrorInConstruct);
    }
    else
    {
        if (m_CurrentToken->IsEndOfLine() && !m_CurrentToken->IsEndOfParse())
        {
            VSASSERT( m_CurrentToken->m_TokenType == tkEOL && 
                      m_CurrentToken->m_Prev->m_TokenType == tkDot, 
                      "We shouldn't get here without .<eol> tokens");

            /* We know we are sitting on an EOL preceded by a tkDot.  What we need to catch is the
               case where a tkDot following an EOL isn't preceded by a valid token.  Bug Dev10_429652  For example:
               with i <eol>
                 .  <-- this is bad.  This . follows an EOL and isn't preceded by a tkID.  Can't have it hanging out like this
                 field = 42
            */
            if ( m_CurrentToken->m_Prev->m_Prev == NULL || // make sure we can look back far enough.  We know we can look back once, but twice we need to test
                 m_CurrentToken->m_Prev->m_Prev->m_TokenType == tkEOL ) // Make sure there is something besides air before the '.' DEV10_486908
            {
                ReportSyntaxError(ERRID_ExpectedIdentifier, m_CurrentToken->m_Prev, ErrorInConstruct);
                Qualified->Opcode = ParseTree::Expression::DotQualified;
                SetExpressionLocation(Qualified, Start, m_CurrentToken, DotOrBangToken);

                ParseTree::NameExpression *pName = new (m_TreeStorage) ParseTree::NameExpression;
                pName->Name.IsBad = true;
                pName->Opcode = ParseTree::Expression::SyntaxError;
                SetExpressionLocation(pName, Start, m_CurrentToken, NULL);
                Qualified->Name = pName;
                
                return Qualified; // We are sitting on the tkEOL so let's just return this and keep parsing.  No ReSync() needed here, in other words.
            }
            else if ( !NextLineStartsWith( tkEOL, false ))
            {
                Token * pNextToken = PeekNextLine();
                if 
                (
                    pNextToken->m_TokenType != tkDot
                )
                {
                    //ILC: undone
                    //       Right now we don't continue after a "." when the following tokens indicate XML member access
                    //       We should probably enable this.
                    EatNewLine();
                }
            }    
        }

        // Decide whether we're parsing:
        // 1. Element axis i.e. ".<ident>"
        // 2. Attribute axis i.e. ".@ident" or ".@<ident>
        // 3. Descendant axis i.e. "...<ident>"
        // 4. Regular CLR member axis i.e. ".ident"
        switch (m_CurrentToken->m_TokenType)
        {
            case tkAt:
                // Consume the @ & remember that this is attribute axis
                GetNextToken();
                Qualified->Opcode = ParseTree::Expression::XmlAttributeQualified;

                // Parse the Xml attribute name (allow name with and without angel brackets)
                if (m_CurrentToken->m_TokenType == tkLT)
                    Qualified->Name = ParseBracketedXmlQualifiedName(false, ErrorInConstruct);
                else
                    Qualified->Name = ParseXmlQualifiedName(false /* AllowExpr */, false /* IsBracketed */, false /* IsElement */, ErrorInConstruct);
                break;

            case tkLT:
                // Remember that this is element axis
                Qualified->Opcode = ParseTree::Expression::XmlElementsQualified;

                // Parse the Xml element name
                Qualified->Name = ParseBracketedXmlQualifiedName(true, ErrorInConstruct);
                break;

            case tkDot:
                if (m_CurrentToken->m_Next->m_TokenType == tkDot)
                {
                    // Consume the 2nd and 3rd dots and remember that this is descendant axis
                    GetNextToken();
                    GetNextToken();

                    /* Dev10_407378 Allow implicit line continuation after ...
                       We can't call EatNewLine(Qualified) here because we have a special case that we can't
                       inject into the EatNewLine() code without breaking scenarios.  This case is a problem:
                            e... _
                            <foo>

                       This wreaks havoc because the scanner eats the newline after '_' and if there is another blank line
                       below the current one, we eat that EOL instead, which then messes up the location span
                       for this statement.  CalculateIndent() then breaks so indentation is off.
                       Trying to fix this in EatNewLine() breaks a number of other scenarios because EatNewLine() is
                       in a path called by the IDE.  Consider intellisense when we have: <attribute> _ public 
                       and you backspace up against the '_'  The reason having this logic in EatNewLine() breaks that scenario
                       is because we always add a tkEOL after the last line that hits the EOF.  And that tkEOL will be
                       marked as explicit line continuation.  So if the logic to skip advancing on tkEOL if it is marked
                       as explicit line continuation is in EatNewLine(), we will fail in the intellisense case.
                       Trying to avoid building the tkEOL in the first place if we are at tkEOF is no good either as that breaks 
                       upstream code that assumes it can always find the end of the line by looking for EOL.
                       And on and on it goes so this logic must be broken out and not be made part of EatNewLine() */

                    if ( m_CurrentToken->IsEndOfLine() && !m_EvaluatingConditionCompilationExpression &&
                         !m_CurrentToken->m_State.m_ExplicitLineContinuation )
                    {
                        if ( !NextLineStartsWith( tkEOL, false ))
                        {
                            if ( PeekNextLine()->m_TokenType != tkSyntaxError )
                            {
                                GetNextToken(); // move off the EOL
                            }
                        }
                    }

                    Qualified->Opcode = ParseTree::Expression::XmlDescendantsQualified;

                    // Parse the Xml element name
                    Qualified->Name = ParseBracketedXmlQualifiedName(true, ErrorInConstruct);
                    break;
                }
                __fallthrough; // Fall through

            default:
                Qualified->Opcode = ParseTree::Expression::DotQualified;
                Qualified->Name = ParseIdentifierExpressionAllowingKeyword(ErrorInConstruct);
                break;
        }
    }

    SetExpressionLocation(Qualified, Start, m_CurrentToken, DotOrBangToken);

    return Qualified;
}

ParseTree::Expression *
Parser::ParseBracketedXmlQualifiedName
(
    bool IsElement,
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Name;

    // Parse '<', Xml QName, '>'
    VerifyExpectedToken(tkLT, ErrorInConstruct);
    Name = ParseXmlQualifiedName(false /* AllowExpr */, true /* IsBracketed */, IsElement, ErrorInConstruct);
    VerifyExpectedToken(tkGT, ErrorInConstruct);

    return Name;
}

// Parse an argument list enclosed in parentheses.

ParseTree::ParenthesizedArgumentList
Parser::ParseParenthesizedArguments
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkLParen, "should be at tkLParen.");
    GetNextToken();
    
    ParseTree::ParenthesizedArgumentList *Result = new(m_TreeStorage) ParseTree::ParenthesizedArgumentList;

    EatNewLine();
    EatNewLineIfFollowedBy(tkRParen, Result);

    if (m_CurrentToken->m_TokenType != tkRParen)
    {
        ParseArguments(&Result->Values, ErrorInConstruct);
    }

    ParseTree::ParseTreeNode * pLastNode = Result;
    
    if (Result->Values)
    {
        pLastNode = GetLastListNode(Result->Values);
    }
        
    EatNewLineIfFollowedBy(tkRParen, Result);

    if (m_CurrentToken->m_TokenType == tkRParen)
    {
        Result->ClosingParenthesisPresent = true;
        GetNextToken();
    }
    else
    {
        // On error, peek for ")" with "(". If ")" seen before
        // "(", then sync on that. Otherwise, assume missing ")"
        // and let caller decide.

        tokens Clue = PeekAheadFor(2, tkLParen, tkRParen);

        if (Clue == tkRParen)
        {
            ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);

            ResyncAt(1, tkRParen);
            GetNextToken();
        }
        else
        {
            ReportSyntaxError(ERRID_ExpectedRparen, m_CurrentToken, ErrorInConstruct);
        }
    }

    SetLocation(&Result->TextSpan, Start, m_CurrentToken);

    return *Result;
}


/*********************************************************************
*
* Function:
*     Parser::ParseParenthesizedQualifier
*
* Purpose:
*     Parses a parenthesized qualifier:
*         <qualifier>(...)
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseParenthesizedQualifier
(
    _In_ Token *Start,      // [in] token starting term
    _In_ ParseTree::Expression *Term,       // [in] preceding term
    _Inout_ bool &ErrorInConstruct
)
{
    // Because parentheses are used for array indexing, parameter passing, and array
    // declaring (via the Redim statement), there is some ambiguity about how to handle
    // a parenthesized list that begins with an expression. The most general case is to
    // parse it as an argument list, with special treatment if a TO occurs in a Redim
    // context.

    // Term must be SX_NAME.

    ParseTree::CallOrIndexExpression *Result = new(m_TreeStorage) ParseTree::CallOrIndexExpression;
    Result->Opcode = ParseTree::Expression::CallOrIndex;
    Result->AlreadyResolvedTarget = false;

    Token *LeftParen = m_CurrentToken;

    Result->Target = Term;
    Result->Arguments = ParseParenthesizedArguments(ErrorInConstruct);

    SetExpressionLocation(Result, Start, m_CurrentToken, LeftParen);

    return Result;
}

// Parse generic arguments appended to an expression.

ParseTree::Expression *
Parser::ParseGenericQualifier
(
    Token *Start,      // [in] token starting term
    _In_opt_ ParseTree::Expression *Term,       // [in] preceding term
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::GenericQualifiedExpression *GenericExpr = new(m_TreeStorage) ParseTree::GenericQualifiedExpression;
    GenericExpr->Opcode = ParseTree::Expression::GenericQualified;

    bool AllowEmptyGenericArguments = false;
    bool AllowNonEmptyGenericArguments = true;

    ParseGenericArguments(
        Start,
        GenericExpr->Arguments,
        AllowEmptyGenericArguments,
        AllowNonEmptyGenericArguments,
        ErrorInConstruct);

    SetLocation(&GenericExpr->TextSpan, Start, m_CurrentToken);
    GenericExpr->Base = Term;

    return GenericExpr;
}

// Parse a list of comma-separated arguments.

void
Parser::ParseArguments
(
    _In_ ParseTree::ArgumentList **Target,             // Where to insert the next list element
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ArgumentList *&ArgumentList = *Target;

    do
    {
        Token *ArgumentStart = m_CurrentToken;

        // Check for the first keyword argument.

        if ((ArgumentStart->m_TokenType == tkID || ArgumentStart->IsKeyword()) &&
            Following(m_CurrentToken)->m_TokenType == tkColEq)
        {
            ParseNamedArguments(Target, ErrorInConstruct);
            break;
        }

        ParseTree::Argument *Argument = NULL;
        EatNewLineIfFollowedBy(tkRParen);

        if (ArgumentStart->m_TokenType == tkComma ||
            ArgumentStart->m_TokenType == tkRParen)
        {
            Argument = new(m_TreeStorage) ParseTree::Argument;
            // There is no argument. What is the location of something that doesn't exist?
            // Treating the non-existing argument as starting at the current token
            // causes problems, because the last element of the argument list has a
            // location outside the list.
            ArgumentStart = ArgumentStart->m_Prev;

            SetLocation(&Argument->TextSpan, ArgumentStart, m_CurrentToken);
        }
        else
        {
            Argument = ParseArgument(ErrorInConstruct);
        }

        CreateListElement(
            Target,
            Argument,
            ArgumentStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (ArgumentsContinue(ErrorInConstruct));

    FixupListEndLocations(ArgumentList);
}

// Test to see if an argument list continues, and consume a separating comma if present.

bool
Parser::ArgumentsContinue
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Next = m_CurrentToken;

    EatNewLineIfFollowedBy(tkRParen);
    
    if (Next->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
        return true;
    }
    else if (Next->m_TokenType == tkRParen || MustEndStatement(Next))
    {
        return false;
    }

    // There is a syntax error of some kind.

    ReportSyntaxError(ERRID_ArgumentSyntax, m_CurrentToken, ErrorInConstruct);
    ResyncAt(2, tkComma, tkRParen);

    if (m_CurrentToken->m_TokenType == tkComma)
    {
        ErrorInConstruct = false;
        return true;
    }

    return false;
}

// Parse a list of comma-separated keyword arguments.

void
Parser::ParseNamedArguments
(
    ParseTree::ArgumentList **Target,              // Where to insert the next list element
    _Inout_ bool &ErrorInConstruct
)
{
    do
    {
        Token *ArgumentStart = m_CurrentToken;

        ParseTree::IdentifierDescriptor ArgumentName = {0};
        Token *ColonEq = NULL;

        if ((ArgumentStart->m_TokenType == tkID || ArgumentStart->IsKeyword()) &&
            Following(ArgumentStart)->m_TokenType == tkColEq)
        {
            ArgumentName = ParseIdentifierAllowingKeyword(ErrorInConstruct);
            ColonEq = m_CurrentToken;
            GetNextToken();
            EatNewLine();
        }
        else
        {
            // Assume that the ID := was left out.

            ReportSyntaxError(ERRID_ExpectedNamedArgument, m_CurrentToken, ErrorInConstruct);

            ArgumentName.IsBad = true;
        }

        ParseTree::Argument *Argument = ParseArgument(ErrorInConstruct);

        Argument->Name = ArgumentName;

        SetLocationAndPunctuator(
            &Argument->TextSpan,
            &Argument->ColonEquals,
            ArgumentStart,
            m_CurrentToken,
            ColonEq);

        CreateListElement(
            Target,
            Argument,
            ArgumentStart,
            m_CurrentToken,
            m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);

    } while (ArgumentsContinue(ErrorInConstruct));
}

ParseTree::Argument *
Parser::ParseArgument
(
    _Inout_ bool &ErrorInConstruct
)
{
    bool OrigErrorInConstruct = ErrorInConstruct;
    bool RedimOrNewParent = m_RedimOrNew;
    m_RedimOrNew = false;

    Token *ArgumentStart = m_CurrentToken;

    ParseTree::Argument *Argument = new(m_TreeStorage) ParseTree::Argument;

    Argument->Value = ParseExpression(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkComma, tkRParen);

        if (m_CurrentToken->m_TokenType == tkComma &&
            !OrigErrorInConstruct)          // Avoids reporting of duplicate errors for errors after the
                                            // first argument error, when reparsing argument lists multiple
                                            // times as in the "New" expression scenario.
        {
            ErrorInConstruct = false;
        }
    }

    if (RedimOrNewParent && m_CurrentToken->m_TokenType == tkTO)
    {
        Argument->lowerBound = Argument->Value;
        SetPunctuator(&Argument->To, ArgumentStart, m_CurrentToken);

        if(Argument->lowerBound->Opcode != ParseTree::Expression::IntegralLiteral ||
            Argument->lowerBound->AsIntegralLiteral()->Value !=0)
        {
            ReportSyntaxError(
                ERRID_OnlyNullLowerBound,
                &Argument->lowerBound->TextSpan,
                ErrorInConstruct);
        }
        GetNextToken(); // consume tkTO
        Argument->Value = ParseExpression(ErrorInConstruct);
    }

#if IDE 
    if (Argument->Value)
    {
        Argument->ValueStartPosition = ArgumentStart->m_StartCharacterPosition;
        Argument->ValueWidth =
            m_CurrentToken->m_Prev->m_StartCharacterPosition +
            m_CurrentToken->m_Prev->m_Width -
            ArgumentStart->m_StartCharacterPosition;
    }
    else
    {
        Argument->ValueStartPosition = 0;
        Argument->ValueWidth = 0;
    }
#endif

    SetLocation(&Argument->TextSpan, ArgumentStart, m_CurrentToken);
    m_RedimOrNew = RedimOrNewParent;

    return Argument;
}

// Parse a CType expression.

ParseTree::Expression *
Parser::ParseCType
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    VSASSERT(Start->m_TokenType == tkCTYPE || Start->m_TokenType == tkDIRECTCAST || Start->m_TokenType == tkTRYCAST,
             "Expected CTYPE or DIRECTCAST or TRYCAST token.");

    GetNextToken();

    Token *LeftParen = m_CurrentToken;
    ParseTree::ConversionExpression *Result = new(m_TreeStorage) ParseTree::ConversionExpression;    
    Result->Opcode =
        Start->m_TokenType == tkCTYPE ?
            ParseTree::Expression::Conversion :
            Start->m_TokenType == tkDIRECTCAST ?
                ParseTree::Expression::DirectCast :
                ParseTree::Expression::TryCast;
    
    VerifyTokenPrecedesExpression(tkLParen, ErrorInConstruct, true, Result);
    
    Result->Value = ParseExpression(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkComma, tkRParen);
    }

    Token *Comma = m_CurrentToken;

    if (m_CurrentToken->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();
    }
    else
    {
        ReportSyntaxError(ERRID_SyntaxInCastOp, m_CurrentToken, ErrorInConstruct);
    }

    Result->TargetType = ParseGeneralType(ErrorInConstruct);

    EatNewLineIfFollowedBy(tkRParen, Result);
    VerifyExpectedToken(tkRParen, ErrorInConstruct);

    SetExpressionLocation(Result, Start, m_CurrentToken, LeftParen);
    SetPunctuatorIfMatches(&Result->Comma, Start, Comma, tkComma);

    return Result;
}

// Parse an expression and add it to an argument list.

void
Parser::ParseSimpleExpressionArgument
(
    ParseTree::ArgumentList **&Target,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *ArgumentStart = m_CurrentToken;

    ParseTree::Argument *Argument = new(m_TreeStorage) ParseTree::Argument;
    Argument->Value = ParseExpression(ErrorInConstruct);
    if (ErrorInConstruct)
    {
        ResyncAt(2, tkComma, tkRParen);
    }
    SetLocation(&Argument->TextSpan, ArgumentStart, m_CurrentToken);

    CreateListElement(
        Target,
        Argument,
        ArgumentStart,
        m_CurrentToken,
        m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);
}

// Parse a Variable and insert the resulting tree in a List tree.

void
Parser::ParseVariableInList
(
    ParseTree::ExpressionList **&Target,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;
    ParseTree::Expression *Variable = ParseVariable(ErrorInConstruct);

    if (ErrorInConstruct)
    {
        ResyncAt(1, tkComma);
    }

    CreateListElement(
        Target,
        Variable,
        Start,
        m_CurrentToken,
        m_CurrentToken->m_TokenType == tkComma ? m_CurrentToken : NULL);
}

// Parse a list of comma-separated Variables.

ParseTree::ExpressionList *
Parser::ParseVariableList
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::ExpressionList *List = NULL;
    ParseTree::ExpressionList **Target = &List;
    ParseTree::ExpressionList **LastTarget = NULL;
        
    ParseVariableInList(Target, ErrorInConstruct);

    while (m_CurrentToken->m_TokenType == tkComma)
    {
        GetNextToken();
        EatNewLine();

        ParseVariableInList(Target, ErrorInConstruct);
    }

    FixupListEndLocations(List);
    return List;
}

// Deal with the case where a token is not what is expected.
// Produces an error unless the construct is already in error.
// Always returns false.

bool
Parser::HandleUnexpectedToken
(
    tokens TokenType,
    _Inout_ bool &ErrorInConstruct
)
{
    unsigned ErrorId;

    switch (TokenType)
    {
        case tkComma:
            ErrorId = ERRID_ExpectedComma;
            break;
        case tkLParen:
            ErrorId = ERRID_ExpectedLparen;
            break;
        case tkRParen:
            ErrorId = ERRID_ExpectedRparen;
            break;
        case tkEQ:
            ErrorId = ERRID_ExpectedEQ;
            break;
        case tkAS:
            ErrorId = ERRID_ExpectedAs;
            break;
        case tkLBrace:
            ErrorId = ERRID_ExpectedLbrace;
            break;
        case tkRBrace:
            ErrorId = ERRID_ExpectedRbrace;
            break;
        case tkDot:
            ErrorId = ERRID_ExpectedDot;
            break;
        case tkMinus:
            ErrorId = ERRID_ExpectedMinus;
            break;
        case tkIS:
            ErrorId = ERRID_MissingIsInTypeOf;
            break;
        case tkGT:
            ErrorId = ERRID_ExpectedGreater;
            break;
        case tkLT:
            ErrorId = ERRID_ExpectedLT;
            break;
        case tkDiv:
            ErrorId = ERRID_ExpectedDiv;
            break;
        case tkXmlEndComment:
            ErrorId = ERRID_ExpectedXmlEndComment;
            break;
        case tkXmlEndCData:
            ErrorId = ERRID_ExpectedXmlEndCData;
            break;
        case tkXmlEndPI:
            ErrorId = ERRID_ExpectedXmlEndPI;
            break;
        case tkQuote:
            ErrorId = ERRID_ExpectedQuote;
            break;
        case tkSQuote:
            ErrorId = ERRID_ExpectedSQuote;
            break;
        case tkXmlNCName:
            ErrorId = ERRID_ExpectedXmlName;
            break;
        case tkSColon:
            ErrorId = ERRID_ExpectedSColon;
            break;
        case tkXmlBeginEmbedded:
            ErrorId = ERRID_ExpectedXmlBeginEmbedded;
            break;
        case tkXmlEndEmbedded:
            ErrorId = ERRID_ExpectedXmlEndEmbedded;
            break;
        default:
            ErrorId = ERRID_Syntax;
            break;
    }

    ReportSyntaxError(ErrorId, m_CurrentToken, ErrorInConstruct);

    return false;
}

// Produce an error message if the current token is not the expected TokenType.
// If these is an error, attempt to synchronize at the start of an Expression.

void
Parser::VerifyTokenPrecedesExpression
(
    tokens TokenType,
    _Inout_ bool &ErrorInConstruct,
    bool EatNewLineAfterToken,
    ParseTree::ParseTreeNode * pNode
)
{
    if (!VerifyExpectedToken(TokenType, ErrorInConstruct, EatNewLineAfterToken, pNode))
    {
        // 

    }
}

// Produce an error message if the current token is not the expected TokenType.
// If these is an error, attempt to synchronize at the start of a Variable.

void
Parser::VerifyTokenPrecedesVariable
(
    tokens TokenType,
    _Inout_ bool &ErrorInConstruct,
    bool EatNewLineAfterToken,
    ParseTree::ParseTreeNode * pNode    
)
{
    if (!VerifyExpectedToken(TokenType, ErrorInConstruct, EatNewLineAfterToken, pNode))
    {
        // 

    }
}

// If present, parse a While or Until clause.

ParseTree::Expression *
Parser::ParseOptionalWhileUntilClause
(
    _Out_ bool *IsWhile, // Out
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::Expression *Expr = NULL;

    if (!IsValidStatementTerminator(m_CurrentToken))
    {
        tokens WhileOrUntil = m_CurrentToken->m_TokenType;

        if (WhileOrUntil == tkWHILE ||
            (WhileOrUntil == tkID && IdentifierAsKeyword(m_CurrentToken) == tkUNTIL))
        {
            GetNextToken();

            Expr = ParseExpression(ErrorInConstruct);

            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            *IsWhile = WhileOrUntil == tkWHILE;
        }

        else
        {
            ReportSyntaxError(ERRID_Syntax, m_CurrentToken, ErrorInConstruct);
            ResyncAt(0);
        }
    }

    return Expr;
}

//
//============ Methods for parsing syntactic terminals ===============
//

/*********************************************************************
*
* Function:
*     Parser::ParseIdentifier
*
* Purpose:
*     Parse an identifier. Current token must be at the expected
*     identifer. Keywords are NOT allowed as identifiers.
*
**********************************************************************/
ParseTree::IdentifierDescriptor
Parser::ParseIdentifier
(
    _Inout_ bool &ErrorInConstruct,
    bool allowNullable
)
{
    ParseTree::IdentifierDescriptor Ident = {0};
    Token *Start = m_CurrentToken;
    if (m_CurrentToken->m_TokenType == tkID)
    {
        Ident = CreateId(m_CurrentToken);
        GetNextToken();
    }
    else
    {
        // If the token is a keyword, assume that the user meant it to
        // be an identifer and consume it. Otherwise, leave current token
        // as is and let caller decide what to do.

        if (m_CurrentToken->IsKeyword())
        {
            Ident = CreateId(m_CurrentToken);


            ReportSyntaxError(
                ERRID_InvalidUseOfKeyword,
                m_CurrentToken,
                ErrorInConstruct);

            GetNextToken();
        }
        else
        {
            ReportSyntaxError(ERRID_ExpectedIdentifier, m_CurrentToken, ErrorInConstruct);
        }

        Ident.IsBad = true;
    }
    Ident.IsNullable = false;
    if (allowNullable && tkNullable == m_CurrentToken->m_TokenType && !Ident.IsBad )
    {
        Ident.IsNullable = true;
        SetLocation(&Ident.TextSpan, Start, m_CurrentToken);
        GetNextToken();
    }

    return Ident;
}

/*********************************************************************
*
* Function:
*     Parser::ParseIdentifierAllowingKeyword
*
* Purpose:
*     Parse an identifier. Current token must be at the expected
*     identifer. Keywords are allowed as identifiers.
*
**********************************************************************/
ParseTree::IdentifierDescriptor
Parser::ParseIdentifierAllowingKeyword
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTree::IdentifierDescriptor Ident = {0};

    if (m_CurrentToken->m_TokenType == tkID || m_CurrentToken->IsKeyword())
    {
        Ident = CreateId(m_CurrentToken);
        GetNextToken();
    }
    else
    {
        // Current token is not advanced. Let caller decide what to do.
        ReportSyntaxError(ERRID_ExpectedIdentifier, m_CurrentToken, ErrorInConstruct);
        Ident.IsBad = true;
    }

    return Ident;
}

ParseTree::Expression *
Parser::ParseIdentifierExpressionAllowingKeyword
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::NameExpression *Result= new(m_TreeStorage) ParseTree::NameExpression;
    Result->Name = ParseIdentifierAllowingKeyword(ErrorInConstruct);
    Result->Opcode =
        Result->Name.IsBad ?
            ParseTree::Expression::SyntaxError :
            ParseTree::Expression::Name;
    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

    return Result;
}

ParseTree::Expression *
Parser::ParseIdentifierExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    Token *Start = m_CurrentToken;

    ParseTree::NameExpression *Result= new(m_TreeStorage) ParseTree::NameExpression;
    Result->Name = ParseIdentifier(ErrorInConstruct);
    Result->Opcode =
        Result->Name.IsBad ?
            ParseTree::Expression::SyntaxError :
            ParseTree::Expression::Name;
    SetExpressionLocation(Result, Start, m_CurrentToken, NULL);

    return Result;
}


/*********************************************************************
*
* Function:
*     Parser::ParseIntLiteral
*
* Purpose:
*     Parses an integral literal.
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseIntLiteral
(
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkIntCon, "Expected Integer literal.");

    ParseTree::IntegralLiteralExpression *Literal = new(m_TreeStorage) ParseTree::IntegralLiteralExpression;
    Literal->Opcode = ParseTree::Expression::IntegralLiteral;
    Literal->Value = m_CurrentToken->m_IntLiteral.m_Value;

    switch (m_CurrentToken->m_IntLiteral.m_Base)
    {
        case Token::Decimal:
            Literal->Base = ParseTree::IntegralLiteralExpression::Decimal;
            break;

        case Token::Hexadecimal:
            Literal->Base = ParseTree::IntegralLiteralExpression::Hexadecimal;
            break;

        case Token::Octal:
            Literal->Base = ParseTree::IntegralLiteralExpression::Octal;
            break;

        default:
            VSFAIL("Surprising base for integral Literal.");
    }

    Literal->TypeCharacter = m_CurrentToken->m_IntLiteral.m_TypeCharacter;

    SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
    GetNextToken();

    return Literal;
}

ParseTree::Expression *
Parser::ParseCharLiteral
(
)
{
    VSASSERT(m_CurrentToken->m_TokenType == tkCharCon, "Expected Char literal.");

    ParseTree::CharacterLiteralExpression *Literal = new(m_TreeStorage) ParseTree::CharacterLiteralExpression;
    Literal->Opcode = ParseTree::Expression::CharacterLiteral;
    Literal->Value = m_CurrentToken->m_CharLiteral.m_Value;

    SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
    GetNextToken();

    return Literal;
}

/*********************************************************************
*
* Function:
*     Parser::ParseStringLiteral
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseStringLiteral
(
    _Inout_ bool &ErrorInConstruct
)
{
    if (m_CurrentToken->m_TokenType == tkStrCon)
    {
        // Tokenizer gives us internal pointers into its own buffers. Copy
        // the string into our own memory, which will go away when the trees
        // go away. Length of string in Token does not include the opening
        // and closing quotes.

        ParseTree::StringLiteralExpression *Literal = new(m_TreeStorage) ParseTree::StringLiteralExpression;
        Literal->Opcode = ParseTree::Expression::StringLiteral;

        size_t CharacterLength = m_CurrentToken->m_StringLiteral.m_Length;
        Literal->LengthInCharacters = CharacterLength;

        // Copy string.

        Literal->Value = new(m_TreeStorage) WCHAR[CharacterLength + 1];
        memcpy(
            Literal->Value,
            m_CurrentToken->m_StringLiteral.m_Value,
            CharacterLength * sizeof(WCHAR));
        Literal->Value[CharacterLength] = L'\0';

        SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
        GetNextToken();

        return Literal;
    }
    else
    {
        ParseTree::Expression *Error = ExpressionSyntaxError();
        SetExpressionLocation(Error, m_CurrentToken, m_CurrentToken, NULL);

        ReportSyntaxError(ERRID_ExpectedStringLiteral, m_CurrentToken, ErrorInConstruct);

        return Error;
    }
}

/*********************************************************************
*
* Function:
*     Parser::ParseDecLiteral
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseDecLiteral()
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkDecCon,
        "must be at a decimal literal.");

    ParseTree::DecimalLiteralExpression *Literal = new(m_TreeStorage) ParseTree::DecimalLiteralExpression;
    Literal->Opcode = ParseTree::Expression::DecimalLiteral;

    memcpy(
        &(Literal->Value),
        &(m_CurrentToken->m_DecimalLiteral.m_Value),
        sizeof(Literal->Value));
    Literal->TypeCharacter = m_CurrentToken->m_DecimalLiteral.m_TypeCharacter;

    SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
    GetNextToken();

    return Literal;
}

/*********************************************************************
*
* Function:
*     Parser::ParseFltLiteral
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseFltLiteral()
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkFltCon,
        "must be at a float literal.");

    ParseTree::FloatingLiteralExpression *Literal = new(m_TreeStorage) ParseTree::FloatingLiteralExpression;
    Literal->Opcode = ParseTree::Expression::FloatingLiteral;

    Literal->Value = m_CurrentToken->m_FloatLiteral.m_Value;
    Literal->TypeCharacter = m_CurrentToken->m_FloatLiteral.m_TypeCharacter;

    SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
    GetNextToken();

    return Literal;
}

/*********************************************************************
*
* Function:
*     Parser::ParseDateLiteral
*
* Purpose:
*
**********************************************************************/
ParseTree::Expression *
Parser::ParseDateLiteral()
{
    VSASSERT(
        m_CurrentToken->m_TokenType == tkDateCon,
        "must be at a date literal.");

    ParseTree::DateLiteralExpression *Literal = new(m_TreeStorage) ParseTree::DateLiteralExpression;
    Literal->Opcode = ParseTree::Expression::DateLiteral;

    Literal->Value = m_CurrentToken->m_DateLiteral.m_Value;

    SetExpressionLocation(Literal, m_CurrentToken, m_CurrentToken->m_Next, NULL);
    GetNextToken();

    return Literal;
}

// Parses comments attached to the end of a logical line.
void
Parser::ParseTrailingComments
(
)
{
    m_CurrentToken = BackupIfImpliedNewline(m_CurrentToken);
    m_FirstUndisposedComment = NULL;

    if (m_CurrentToken->m_TokenType == tkREM || m_CurrentToken->m_TokenType == tkXMLDocComment)
    {
        do
        {
            CreateComment(m_CurrentToken);

            // Consume the tkREM or tkXMLDocComment, without advancing past an EOL.
            m_CurrentToken = m_CurrentToken->m_Next;

        } while (m_CurrentToken->m_TokenType == tkREM || m_CurrentToken->m_TokenType == tkXMLDocComment);
    }

    // Attach the trailing comments, along with any parsed mid-statement,
    // to the last statement, which will already have been linked.
    // Unless lookahead parsing has occurred. Then we want to attach the trailing
    // comments to the CURRENT statement, and not the last statement.
    //
    // Property foo() As Integer 'C1
    // 'C2
    //    Get
    // ...
    //
    // In the above scenario, lookahead parsing has caused us to parse and link
    // C2; thus, it is the m_LastStatementLinked. However, when we return to
    // the Property declaration line to continue parsing the property. When we reach
    // the trailing comment C1, we must attach this comment to the PropertyStatement
    // not m_LastStatementLinked.

    ParseTree::StatementList *pStmtToApplyComments = m_pCurrentStatement ;

  
    if (pStmtToApplyComments)
    {
        if (!pStmtToApplyComments->Element->Comments)
        {
            // Add comments to last statement linked.
            pStmtToApplyComments->Element->Comments = m_CommentsForCurrentStatement;
        }
#if DEBUG
        // For statement lambdas there are cases where a comment trailing the terminating End (Sub/Function) has already been
        // added to the End (Sub/Function) statement. If this is the case, there shouldn't be another comment to add, thus, 
        // m_CommentsForCurrentStatement should be null.
        else
        {
            VSASSERT(!m_CommentsForCurrentStatement, "There should not be any comment to add.");
        }
#endif
        m_CommentsForCurrentStatement = NULL;
        m_CommentListTarget = &m_CommentsForCurrentStatement;
    }
}

void
Parser::CreateComment
(
    _In_ Token *CommentToken
)
{
    VSASSERT(
        CommentToken->m_TokenType == tkREM || CommentToken->m_TokenType == tkXMLDocComment,
        "Expected to find a Comment token.");

    ParseTree::Comment *Comment = new(m_TreeStorage) ParseTree::Comment;
    Comment->IsGeneratedFromXMLDocToken = (CommentToken->m_TokenType == tkXMLDocComment);

    // Tokenizer gives us internal pointers. Copy into our own
    // allocator.

    size_t CharacterLength = CommentToken->m_Comment.m_Length;
    Comment->LengthInCharacters = CharacterLength;

    // Copy string.

    Comment->Spelling = new(m_TreeStorage) WCHAR[CharacterLength + 1];
    memcpy(
        Comment->Spelling,
        CommentToken->m_Comment.m_Spelling,
        CharacterLength * sizeof(WCHAR));
    Comment->Spelling[CharacterLength] = L'\0';
    Comment->IsRem = CommentToken->m_Comment.m_IsRem;

    SetLocation(&Comment->TextSpan, CommentToken, CommentToken->m_Next);

    CreateListElement(
        m_CommentListTarget,
        Comment,
        CommentToken,
        CommentToken,
        NULL);

    if (m_IsXMLDocEnabled &&
        CommentToken->m_TokenType == tkXMLDocComment &&
        m_LastStatementLinked &&
        m_LastStatementLinked->Element->Opcode != ParseTree::Statement::Empty)
    {
        // XMLDoc comments can only be attached to empty statements, otherwise they are not
        // first on line and an error should be reported.
        bool ErrorInConstruct = false;
        ReportSyntaxErrorWithoutMarkingStatementBad(WRNID_XMLDocNotFirstOnLine, m_CurrentToken,
                                                    m_CurrentToken->m_Next, ErrorInConstruct);
    }

#if IDE
    if (m_Errors)
    {
        VSTASKPRIORITY Priority;

        if (GetCompilerSharedState()->IsTaskCommentString(Comment->Spelling,
                                                      &Priority))
        {
            m_Errors->CreateComment(&Comment->TextSpan,
                                    Comment->Spelling,
                                    Priority);
        }
    }
#endif
}

//
//============ Methods related to managing text locations. =================
//

/*********************************************************************
*
* Function:
*     Parser::TokenColumnEnd
*
* Purpose:
*     Columns are 0 based. End column is the column of the last
*     character of the token, not the one after.
*
**********************************************************************/
long
Parser::TokenColumnEnd
(
    _In_ Token *End        // token to calculate column end
)
{
    long ColumnEnd;

    // Some tokens (tkEOF, tkEOL) do not have a width.
    if (End->m_Width)
    {
        ColumnEnd = TokenColumnBeg(End) + End->m_Width - 1;
    }
    else
    {
        ColumnEnd = TokenColumnBeg(End);
    }

    return ColumnEnd;
}

// ISSUE Microsoft -- these method templates are now defined inline in Parser.h.
// They are not necessarily appropriate as inline methods, but VC does not allow
// non-inline definitions of template member functions.

#if 0

template <class ElementType, class ListType> void
Parser::FixupListEndLocations
(
    ParseTree::List<ElementType, ListType> *List
)
{
    Token *LastTokenOfList = m_CurrentToken->m_Prev;

    for (ParseTree::List<ElementType, ListType> *ListElement = List; ListElement; ListElement = ListElement->Next)
    {
        // The last element of the list should already have correct end location info.
        VSASSERT(
            ListElement->Next ||
            m_CurrentStatementInError ||
            (m_ErrorReportingDisabled && m_ErrorCount > 0) ||
            // The comma case arises in parsing a parameter list where an optional parameter
            // is followed by a non-optional parameter--the current token in this case is
            // the first token of the non-optional parameter. This case either ends up as
            // an error or, for a property assignment, the list gets fixed up again later.
            LastTokenOfList->m_TokenType == tkComma ||
            (ListElement->TextSpan.m_lEndLine == LastTokenOfList->m_StartLine &&
             ListElement->TextSpan.m_lEndColumn == LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1),
            "List ends at a surprising point.");

        ListElement->TextSpan.m_lEndLine = LastTokenOfList->m_StartLine;
        ListElement->TextSpan.m_lEndColumn = LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1;
    }
}

template <class ElementType, class ListType> void
Parser::CreateListElement
(
    ListType **&Target,
    ElementType *Element,
    Token *Start,
    Token *End,
    Token *ListPunctuator
)
{
    ListType *ListElement = new(m_TreeStorage) ListType;

    ListElement->Element = Element;

    SetLocationAndPunctuator(
        &ListElement->TextSpan,
        &ListElement->Punctuator,
        Start,
        End,
        ListPunctuator);

    *Target = ListElement;
    Target = &ListElement->Next;
}

#endif

//
//============ Methods to manage internal state. ================================
//


//
//============ Methods to test properties of tokens. ====================
//

//
//============ Methods related to creating error messages. ================
//

// Add an error to the error table.

void
Parser::AddError
(
    ParseError *Error,
    _In_opt_z_ const WCHAR *Substitution1 /*=NULL*/
)
{
    if (m_Errors)
    {
        if(Substitution1)
        {
            m_Errors->CreateError(Error->Errid, &(Error->TextSpan), Substitution1);
        }
        else
        {
            m_Errors->CreateError(Error->Errid, &(Error->TextSpan));
        }
    }
}

void
Parser::AddError
(
    ParseError *Error,
    _In_z_ const WCHAR *Substitution1,
    _In_z_ const WCHAR *Substitution2
)
{
    AssertIfNull(Substitution1);
    AssertIfNull(Substitution2);

    if (m_Errors)
    {
        m_Errors->CreateError(Error->Errid, &(Error->TextSpan), Substitution1, Substitution2);
    }
}

/*********************************************************************
*
* Function:
*     Parser::SetErrorLocation
*
* Purpose:
*     Sets the error location in ParseError.
*
**********************************************************************/
void
Parser::SetErrorLocation
(
    _Inout_ ParseError *Error,       // [in] error
    _In_ Token *Beg,    // [in] beg token of error (inclusive)
    _In_ Token *End     // [in] end token of error (non-inclusive)
)
{
    bool ErrorAtEndOfStatement = false;

    if (Beg->m_TokenType != tkXMLDocComment && !(Error->Errid == ERRID_SubRequiresSingleStatement && Beg->m_TokenType == tkColon))
    {
        // A syntax error that occurs at the end of a statement is reported
        // (at the end of) the preceding token.
        // Except for "SubRequiresSingleStatement", where it reports on the colon that separates two statements.

        ErrorAtEndOfStatement = IsValidStatementTerminator(Beg);

        if (ErrorAtEndOfStatement)
        {
            Beg = Beg->m_Prev;
        }
    }

    SetLocation(&Error->TextSpan, Beg, End);

    if (ErrorAtEndOfStatement && (Beg->m_Width > 0))
    {
        Error->TextSpan.m_lBegColumn = Error->TextSpan.m_lBegColumn + Beg->m_Width - 1;
    }
}

/*********************************************************************
*
* Function:
*     Parser::ReportSyntaxError
*
* Purpose:
*     Creates and adds a general parse error to the error table, getting
*     the error location info from a span of Tokens.
*
**********************************************************************/
void
Parser::ReportSyntaxError
(
    unsigned Errid,       // [in] error id
    _In_ Token *Beg,      // [in] token beginning error (inclusive)
    _In_ Token *End,      // [in] token ending error (non-inclusive)
    bool MarkErrorStatementBad, // [in] indicates whether the statement containing the error should be marked bad or not
    _Inout_ bool &ErrorInConstruct
)
{
    m_ErrorCount += 1;

    if (MarkErrorStatementBad)
    {
        m_CurrentStatementInError = true;
    }

    if (ErrorInConstruct)
    {
        return;
    }

    if (MarkErrorStatementBad)
    {
        ErrorInConstruct = true;
    }

    if (!IsErrorDisabled())
    {
        // A lexical error takes precedence over whatever the
        // parser has detected.

        if (Beg->m_TokenType == tkSyntaxError)
        {
            ParseError Error;

            // Bug Devdiv 7000.
            // When line continuation char is added without a
            // preceding backspace or with comments following it,
            // then a very unhelpful error indicating
            // "Identifier Expected" is given. This fix is to give
            // a better error in that scenario and also let pretty
            // lister fix up the code.
            //
            if (Beg->m_Error.m_errid == ERRID_ExpectedIdentifier &&
                Beg->m_Error.m_IsTokenForInvalidLineContinuationChar &&
                Beg->m_Next &&
                (Beg->m_Next->m_TokenType == tkEOL ||
                    Beg->m_Next->m_TokenType == tkEOF ||
                    Beg->m_Next->m_TokenType == tkREM))
            {
                // "Line continuation character '_' should be precededed
                // by at least one white space and cannot be followed by
                // any other characters.
                Error.Errid = ERRID_LineContWithCommentOrNoPrecSpace;
            }
            else
            {
                Error.Errid = Beg->m_Error.m_errid;
            }

            SetErrorLocation(&Error, Beg, Beg);
            AddError(&Error);
        }
#if IDE 
        // Report any unreported unterminated string errors only if
        // the current statement is being marked bad, because
        // this case the pretty lister will not list back the missing
        // ending quote.
        else if (MarkErrorStatementBad &&
                 Beg->m_Prev &&
                 Beg->m_Prev->m_TokenType == tkStrCon &&
                 Beg->m_Prev->m_StringLiteral.m_HasUnReportedUnterminatedStringError)
        {
            ParseError Error;

            Error.Errid = ERRID_UnterminatedStringLiteral;
            SetErrorLocation(&Error, Beg->m_Prev, Beg->m_Prev);

            AddError(&Error);
        }

        // A lexical error takes precedence over whatever the
        // parser has detected.
#endif
        else
        {
            ParseError Error;
            Error.Errid = Errid;
            SetErrorLocation(&Error, Beg, End);

            AddError(&Error);
        }
    }
}

void
Parser::ReportSyntaxError
(
    unsigned Errid,       // [in] error id
    _Inout_ Token *Beg,      // [in] token beginning error (inclusive)
    _Inout_ Token *End,      // [in] token ending error (non-inclusive)
    _Inout_ bool &ErrorInConstruct
)
{
    ReportSyntaxError(
        Errid,
        Beg,
        End,
        true,       // Mark current statement bad
        ErrorInConstruct);
}

void
Parser::ReportSyntaxError
(
    unsigned ErrorId,
    _In_ Token *StartAndEnd,
    _Inout_ bool &ErrorInConstruct
)
{
    // If the token is an EOL, then it's not permitted to look beyond it,
    // so pass along the start token twice (rather than passing the following
    // token as the end token).

    ReportSyntaxError(ErrorId, StartAndEnd, StartAndEnd, ErrorInConstruct);
}

void
Parser::ReportSyntaxErrorWithoutMarkingStatementBad
(
    unsigned ErrorId,
    _In_ Token *Start,
    _In_ Token *End,
    bool ErrorInConstruct
)
{
    // This error does not create a need for error recovery,
    // and does not require disabling pretty listing.

    // Under the debugger, treat this as no error at all.
    // This allows constructs that pretty listing would fix up to
    // be evaluated under the debugger.

    if (!m_InterpretationUnderDebugger)
    {
        ReportSyntaxError(
            ErrorId,
            Start,
            End,
            false,       // Don't mark current statement bad
            ErrorInConstruct);
    }
}

void
Parser::ReportSyntaxErrorWithoutMarkingStatementBad
(
    unsigned ErrorId,
    _In_ Token *StartAndEnd,
    bool ErrorInConstruct
)
{
    ReportSyntaxErrorWithoutMarkingStatementBad(ErrorId, StartAndEnd, StartAndEnd, ErrorInConstruct);
}

void
Parser::ReportSyntaxErrorWithoutMarkingStatementBad
(
    unsigned Errid,       // [in] error id
    _In_ Location *TextSpan,           // [in] location of error
    _Inout_ bool &ErrorInConstruct,
    _In_opt_z_ const WCHAR *Substitution1 /*=NULL*/
)
{
    ReportSyntaxError(
        Errid,
        TextSpan,
        ErrorInConstruct,
        Substitution1,
        false);
}

/*********************************************************************
*
* Function:
*     Parser::ReportSyntaxError
*
* Purpose:
*
*     Creates and adds a general parse error to the error table, getting
*     the error location info from a Location.
*
**********************************************************************/
void
Parser::ReportSyntaxError
(
    unsigned Errid,       // [in] error id
    _In_ Location *TextSpan,           // [in] location of error
    _Inout_ bool &ErrorInConstruct,
    _In_opt_z_ const WCHAR *Substitution1, /*=NULL*/
    _In_opt_ bool MarkStatementBad // true
)
{
    m_ErrorCount += 1;

    if (MarkStatementBad)
    {
        m_CurrentStatementInError = true;
    }

    if (ErrorInConstruct)
    {
        return;
    }

    ErrorInConstruct = true;

    if (!IsErrorDisabled())
    {
        ParseError Error;

        Error.Errid = Errid;
        Error.TextSpan = *TextSpan;

        AddError(&Error, Substitution1);
    }
}

// Create an error for a statement not recognized as anything meaningful in the
// current context. Return a statement for the erroneous text.
ParseTree::Statement *
Parser::ReportUnrecognizedStatementError
(
    unsigned ErrorId,
    _Inout_ bool &ErrorInConstruct
)
{
    // Create a statement with no operands. It will end up with its error flag set.

    CacheStmtLocation();

    ParseTree::Statement *Statement = new(m_TreeStorage) ParseTree::Statement;
    Statement->Opcode = ParseTree::Statement::SyntaxError;

    SetStmtBegLocation(Statement, m_CurrentToken);

    ReportSyntaxError(ErrorId, m_CurrentToken, ErrorInConstruct);
    ResyncAt(0);

    SetStmtEndLocation(Statement, m_CurrentToken);
    return Statement;
}

void
Parser::ReportSyntaxErrorForPreviousStatement
(
    unsigned ErrorId,
    _In_ Token *Start,
    _In_ Token *End,
    _Inout_ bool &ErrorInConstruct
)
{
    ReportSyntaxError(ErrorId, Start, End, ErrorInConstruct);

    // Render the current statement, which will already have been linked, malformed.

    if (m_CurrentStatementInError)
    {
        m_CurrentStatementInError = false;
        if (m_pCurrentStatement)
        {
            SetStmtEndLocation(m_pCurrentStatement->Element,  End);
            m_pCurrentStatement->Element->HasSyntaxError = true;

            // If this statement terminates a multiline lambda or is within a multiline lambda, we want to mark the
            // multiline lambda as having a syntax error also.
            if (m_LastStatementLinked &&
                 (((m_LastStatementLinked->Element->Opcode == ParseTree::Statement::EndSub) || 
                        (m_LastStatementLinked->Element->Opcode == ParseTree::Statement::EndFunction)) &&
                m_LastStatementLinked->PreviousInBlock &&
                m_LastStatementLinked->PreviousInBlock->Element->Opcode == ParseTree::Statement::LambdaBody))
            {
                // Statement terminates a lambda.
                m_LastStatementLinked->PreviousInBlock->Element->HasSyntaxError = true;
            }
            else if (m_pStatementLambdaBody)
            {
                m_pStatementLambdaBody->HasSyntaxError = true;
            }
        }
    }
}

/*****************************************************************************************
;ReportSyntaxErrorForLanguageFeature

Reports errors for cases where a feature was introduced after the version specified by
the /LangVersion switch.  

The reason that this function is seperate from the normal error reporting functions is that
we don't want the ErrorInConstruct flag set to true.  When ErrorInConstruct is true the
parser tries to resynchronize, and we don't want that to happen because we are happily
parsing the text and wish to continue doing so unimpeded--we have just noted that the
version of the compiler being targeted doesn't support the language feature, is all.
******************************************************************************************/
void Parser::ReportSyntaxErrorForLanguageFeature
(
    unsigned Errid, // the error to log.  
    _In_ Token *Start, // We log the error at the location contained in this token 
    unsigned Feature, // A FEATUREID_* constant defined in errors.inc
    _In_opt_z_ const WCHAR *wszVersion // the string for the version that /LangVersion is targeting
)
{
    m_ErrorCount += 1;

    // We want the statement to be marked as having a syntax error so that decompilation will
    // work correctly when the issue is fixed (dev10 #647657)
    m_CurrentStatementInError = true;

    if (!IsErrorDisabled())
    {
        ParseError Error;

        Error.Errid = Errid;
        SetErrorLocation(&Error, Start, Start);

        WCHAR wszLoad[CCH_MAX_LOADSTRING];
        IfFailThrow( ResLoadString( Feature, wszLoad, DIM( wszLoad ) ) );

        AddError(&Error, wszLoad, wszVersion);
    }
}

//
//============ Methods to manage syntactic contexts ======================
//

// Assigning Resume Indices
//
// When a procedure utilizes On Error and Resume, each statement needs to be
// labeled with a number.  This number is used at runtime to track on which statement
// an exception occured so that a Resume (or Resume Next) can jump back to the
// correct location.  The Parser assigns these numbers using the sequential order
// of Statement nodes as they get linked into the method's parse trees.  Semantics
// propagates these "resume indices" into the bound trees where Codegen then uses
// them to build the appropriate IL for On Error and Resume.
//
// Roughly, the Parser sequentially counts and labels each Statement in a procedure starting from 1.
// However, some nodes are special and require two, three or more resume indices per Statement.
//
// The Parser's work for assigning resume indicies is greatly simplified for two reasons:
//
//    1.  Codegen handles "gaps" in the indices between two statements.  For example, two
//        sequential statements may have indices N and M, respectively, where M > N + 1.  The gaps
//        between N and M will be filled in by Codegen to make Resume and Resume Next behave
//        correctly.  As long as the resume indicies assigned by the Parser are always increasing,
//        Codegen can handle it correctly.
//
//    2.  For language constructs which require multiple indicies, the assignment of indices
//        can be formed such that indices are grouped into sequential blocks.  For example,
//        a Case block at index I has no "End Case" construct that the Parser can use to assign
//        the terminating resume index J (where J - I is the number of statements in the Case).
//        Rather than perform the assignment of J when the Case context gets popped, we instead
//        assign two indices, J and J+1, to the next Case block.  The next Case uses J to terminate
//        the previous Case block and J+1 to allow Resume on itself.  This sequential grouping
//        of indicies grants the Parser much simplicity, allowing the it to perform assignments
//        given only the Statement opcode.
//
//    3.  Unfortunately the logic inside AssignResumeIndices, doesn't work with only one pass,
//         when considering constructs such as the following:
//
//             For I = 1 to N
//                 For J = 1 to N
//             Next J, I
//
//             Resume Label
//
//         Extra information needs to be stored in the parse tree and processed in semantics. We need
//         to effectively assign a separate resume index to each for loop terminating construct, but in this
//         case they share the same parse tree element.

void
Parser::AssignResumeIndices
(
    _Inout_ ParseTree::Statement *Statement
)
{
    // First determine if we don't need to assign resume indices to this statement.

    // Resume indicies don't matter when the procedure contains a Try
    // because Try and On Error don't mix.
    if (m_Procedure->ProcedureContainsTry)
    {
        return;
    }

    // No need to assign indices to statements which have no executable form.
    switch (Statement->Opcode)
    {
        case ParseTree::Statement::Empty:
        case ParseTree::Statement::CommentBlock:
        case ParseTree::Statement::CCConst:
        case ParseTree::Statement::CCElse:
        case ParseTree::Statement::CCElseIf:
        case ParseTree::Statement::CCEndIf:
        case ParseTree::Statement::CCIf:
            return;

        case ParseTree::Statement::Label:
            // Labels that are line numbers turn into executable code
            // when On Error is present.
            if (!Statement->AsLabelReference()->LabelIsLineNumber)
            {
                return;
            }
            break;
    }

    // Resume indicies don't matter when the statement is contained in
    // an exception context like SyncLock or Using.  The entire block
    // acts as one big statement in the eyes of On Error.

    // If the current statement is the current context, use the parent.
    // Otherwise, a block will act as if it contains itself.
    ParseTree::BlockStatement *Context =
        (Statement == m_Context) ? m_Context->GetParent() : m_Context;

    if (Context && Context->IsOrContainedByExceptionContext)
    {
        return;
    }

    // Now do the actual assignment of indices.

    switch (Statement->Opcode)
    {
        case ParseTree::Statement::Select:
            // Select gets two sequential resume indices, one for the
            // line and one for protecting against fall-through into the
            // first Case.

        case ParseTree::Statement::ElseIf:
        case ParseTree::Statement::BlockElse:
        case ParseTree::Statement::LineElse:
        case ParseTree::Statement::Case:
        case ParseTree::Statement::CaseElse:
            // Else, ElseIf, Case, and Case Else get two sequential resume
            // indices, one for protecting fall-through from above, then next
            // for resume on the line itself.

        case ParseTree::Statement::While:
        case ParseTree::Statement::DoWhileTopTest:
        case ParseTree::Statement::DoUntilTopTest:
            // While, Do While and Do Until get two sequential resume indices, one for
            // the line itself, one for ensuring fall-through into the loop.

        case ParseTree::Statement::Resume:
            // Resume gets two sequential resume indices, one for the line itself,
            // one for ensuring the branch to label is taken upon a Resume Next.

            Statement->ResumeIndex = ++m_Procedure->ResumeTargetCount;
            ++m_Procedure->ResumeTargetCount;
            break;

        case ParseTree::Statement::Erase:
            // Erase gets as many sequential resume indices as there are arrays
            // in the Erase statement.

            Statement->ResumeIndex = ++m_Procedure->ResumeTargetCount;

            if (Statement->AsErase()->Arrays)
            {
                for (ParseTree::ExpressionList *Arrays = Statement->AsErase()->Arrays->Next;
                     Arrays;
                     Arrays = Arrays->Next)
                {
                    ++m_Procedure->ResumeTargetCount;
                }
            }
            break;

        case ParseTree::Statement::Redim:
            // Redim gets three resume indices for each expression in the Redim statement,
            // one for the item itself and two for guarding the assignments to and from a temporary.

            Statement->ResumeIndex = ++m_Procedure->ResumeTargetCount;

            // Add two for the expression which we've already counted.
            m_Procedure->ResumeTargetCount += 2;

            if (Statement->AsRedim()->Redims)
            {
                for (ParseTree::ExpressionList *Redims = Statement->AsRedim()->Redims->Next;
                    Redims;
                    Redims = Redims->Next)
                {
                    m_Procedure->ResumeTargetCount += 3;
                }
            }
            break;

        case ParseTree::Statement::VariableDeclaration:
            // A Variable declaration statement gets as many resume indices as it has initialized variables.

            if (!Statement->AsVariableDeclaration()->Specifiers->HasSpecifier(ParseTree::Specifier::Const))
            {
                for (ParseTree::VariableDeclarationList *Declarations = Statement->AsVariableDeclaration()->Declarations;
                     Declarations;
                     Declarations = Declarations->Next)
                {
                    for (ParseTree::DeclaratorList *Variables = Declarations->Element->Variables;
                         Variables;
                         Variables = Variables->Next)
                    {
                        if (Declarations->Element->Opcode != ParseTree::VariableDeclaration::NoInitializer ||
                            Variables->Element->ArrayInfo && Variables->Element->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes)
                        {
                            Variables->Element->ResumeIndex = ++m_Procedure->ResumeTargetCount;
                        }
                    }
                }
            }
            break;

        case ParseTree::Statement::EndNext:
            // For Next Statements get as many resume indices as it has variables.

            Statement->ResumeIndex = ++m_Procedure->ResumeTargetCount;

            if (Statement->AsEndNext()->Variables)
            {
                for (ParseTree::ExpressionList *Variables = Statement->AsEndNext()->Variables->Next;
                     Variables;
                     Variables = Variables->Next)
                {
                    ++m_Procedure->ResumeTargetCount;
                }
            }
            break;

        default:
            Statement->ResumeIndex = ++m_Procedure->ResumeTargetCount;
            break;
    }
}

// Attach a statement to the current context.

void
Parser::LinkStatement
(
    _Out_ ParseTree::Statement *Statement
)
{
    // Set this statement as the owner of any multiline lambda blocks that may be inside it. This must be done here because the Statement nodes are not usually
    // built when we are parsing the lambda block.

    if (m_SeenLambdaExpression && 
        !m_pStatementLambdaBody &&  // a multiline lambda or anything inside is never the owner
        Statement->Opcode != ParseTree::Statement::LambdaBody)    // we don't want to use self
    {
        for (ULONG i = 0 ; i < m_LambdaBodyStatements.Count() ; i++)
        {
            ParseTree::LambdaBodyStatement *pLambdaBodyStatement = m_LambdaBodyStatements.Element(i);

            pLambdaBodyStatement->pOwningStatement = Statement;

            if (pLambdaBodyStatement->HasSyntaxError)
            {
                // If the lambda body has a syntax error in it, then the owning statement should
                // be marked as having a syntax error also, as well as any lambda that may own this lambda.
                Statement->HasSyntaxError = true;
            }
            
        }
        m_LambdaBodyStatements.Reset();  //we've set the owners: we don't need them around anymore

    }

    if (m_FirstStatementOnLine)
    {
        m_FirstStatementOnLine = false;
        Statement->IsFirstOnLine = true;
    }
    
    if (m_CurrentStatementInError)
    {
        m_CurrentStatementInError = false;
        Statement->HasSyntaxError = true;
    }

    if (m_pStatementLambdaBody &&
        (m_CurrentStatementInError || Statement->HasSyntaxError))
    {
        m_pStatementLambdaBody->HasSyntaxError = true;
    }
    
    UpdateStatementLinqFlags(Statement);

    if (m_Procedure)
    {
        m_Procedure->ContainsAnonymousTypeInitialization |= m_SeenAnonymousTypeInitialization;
        m_Procedure->ContainsQueryExpression |= m_SeenQueryExpression;
        m_Procedure->ContainsLambdaExpression |= m_SeenLambdaExpression;
    }

    m_SeenAnonymousTypeInitialization = false;
    m_SeenQueryExpression = false;
    m_SeenLambdaExpression = false;

    ParseTree::StatementList *ListElement = new(m_TreeStorage) ParseTree::StatementList;

    ParseTree::StatementList *Previous = NULL;

    Previous = m_LastInContexts[m_ContextIndex];
    
    ListElement->PreviousInBlock = Previous;
    if (Previous)
    {
        ListElement->NextInBlock = Previous->NextInBlock;
        Previous->NextInBlock = ListElement;
    }
    else
    {
        ListElement->NextInBlock = m_Context->Children;
        m_Context->Children = ListElement;
    }
    if (ListElement->NextInBlock)
    {
        ListElement->NextInBlock->PreviousInBlock = ListElement;
    }

    VSASSERT(m_LastStatementLinked == NULL || m_LastStatementLinked->NextLexical == NULL, "We are about to corrupt our lexical statement list!");

    ListElement->PreviousLexical = m_LastStatementLinked;
    if (m_LastStatementLinked)
    {
        m_LastStatementLinked->NextLexical = ListElement;
    }
    ListElement->NextLexical = NULL;
    m_LastInContexts[m_ContextIndex] = ListElement;
    m_LastStatementLinked = ListElement;

    ListElement->Element = Statement;
    Statement->ContainingList = ListElement;
    Statement->SetParent(m_Context);
    // Save the last linked statement because it may be changed if this statment contains a multi-line lambda
    // by FindFirstExpressionWithArgument below.  Comments are always attached to m_pCurrentStatement.
    m_pCurrentStatement = m_LastStatementLinked;

    // Multiline lambda statements are added as NextLexicals after the statement
    // that owns them.
    //
    // i.e.
    // Dim x = Sub()
    //             Dim a = 1
    //         End Sub 
    //
    // Dim y = 1
    //
    // Would have a lexical StatementList that looks like:
    // x->Lambda->a->End Sub->y
    // 
    // Thus, we must add the lambda's statements to the lexical list.
    //

    VSASSERT( Statement == ListElement->Element, 
              "The statement list we pass as an argument to the parsetree visitor must contain the statement we have just linked to the statement list.");
    VSASSERT( ListElement == m_LastStatementLinked, 
              "The statement list we pass into the parsetree visitor should be the list of the last statement linked into the statement list.");

    if (Statement->ContainsLambdaExpression)
    {
        ParseTreeSearcher::FindFirstExpressionWithArgument( Statement, 
                                                        &m_LastStatementLinked, 
                                                        Parser::TryAddMultilineLambdaStatementsToLexicalStatementList);
    }

    if (Statement->IsBlock())
    {
        EstablishContext(Statement->AsBlock());
    }
    else if (Statement->Opcode == ParseTree::Statement::Label)
    {
        if (m_Procedure || m_pStatementLambdaBody)
        {
            // If m_pStatementLambdaBody is NULL, then we are not within a lambda body and we should add
            // this label to the procedure.
            // However, if we are in a statement lambda body, we want to add the label to the statement lambda and not the
            // procedure that may contain it.
            //
            if (m_pStatementLambdaBody)
            {
                m_pStatementLambdaBody->definedLabelCount++;
            }
            else
            {
                m_Procedure->DefinedLabelCount++;
            }

            ParseTree::StatementList *LabelListElement = new(m_TreeStorage) ParseTree::StatementList;
            ParseTree::StatementList *PreviousLabel = m_LastLabel;

            LabelListElement->PreviousInBlock = PreviousLabel;
            if (PreviousLabel)
            {
                LabelListElement->NextInBlock = PreviousLabel->NextInBlock;
                PreviousLabel->NextInBlock = LabelListElement;
            }
            else
            {
                LabelListElement->NextInBlock = NULL;

                if (m_pStatementLambdaBody)
                {
                    m_pStatementLambdaBody->pDefinedLabels = LabelListElement;
                }
                else
                {
                    m_Procedure->DefinedLabels = LabelListElement;
                }
            }
            if (LabelListElement->NextInBlock)
            {
                LabelListElement->NextInBlock->PreviousInBlock = ListElement;
            }

            LabelListElement->Element = Statement;
            m_LastLabel = LabelListElement;
        }
    }

    if (m_Procedure)
    {
        AssignResumeIndices(Statement);
    }

    Statement->XmlRoots = m_XmlRoots;
    m_XmlRoots = NULL;
    m_XmlRootsEnd = &m_XmlRoots;
}

// Establish a block statement as the current context.

void
Parser::EstablishContext
(
    _Inout_ ParseTree::BlockStatement *Block
)
{
    if (m_Context && m_Context->IsOrContainedByExceptionContext)
    {
        Block->IsOrContainedByExceptionContext = true;
    }
    else
    {
        switch (Block->Opcode)
        {
            case ParseTree::Statement::Try:
            case ParseTree::Statement::Catch:
            case ParseTree::Statement::Finally:
            case ParseTree::Statement::Using:
            case ParseTree::Statement::SyncLock:
                Block->IsOrContainedByExceptionContext = true;
                break;
        }
    }

    m_Context = Block;
    m_ContextIndex++;

    if (m_ContextIndex == m_MaxContextDepth)
    {
        // The maximum depth of the context stack has been reached. Increase
        // the size of the context stack.

        m_MaxContextDepth += 50;
        ParseTree::StatementList **OldContexts = m_LastInContexts;
        m_LastInContexts = new(m_TreeStorage) ParseTree::StatementList*[m_MaxContextDepth];

        for (int Index = 0; Index < m_ContextIndex; Index++)
        {
            m_LastInContexts[Index] = OldContexts[Index];
        }
    }

    m_LastInContexts[m_ContextIndex] = NULL;
}

/////////////////////////////////////////////////////////////////////
//
// TryAddMultilineLambdaStatementsToLexicalStatementList
//
// Multiline lambda statements are added as NextLexicals after the statement
// that owns them.
//
// i.e.
// Dim x = Sub()
//             Dim a = 1
//         End Sub 
//
// Dim y = 1
//
// Would have a lexical StatementList that looks like:
// x->Lambda->a->End Sub->y
// 
// Remember that the lambda's statements cannot be accessed through the NextInBlock pointer
// of the StatementList that contains the statement which owns the lambda (dim x in the above
// example). This is because the Statement lambda is in a different block (a HiddenBlockStatement).
//
bool Parser::TryAddMultilineLambdaStatementsToLexicalStatementList
(
    _In_ ParseTree::Expression *pExpr,
    _In_ ParseTree::StatementList **ppListElement
)
{
    ThrowIfNull(pExpr);
    ThrowIfNull(ppListElement);
    ThrowIfNull((*ppListElement));
    
    if (ParseTree::LambdaExpression::GetIsStatementLambda(pExpr, NULL) && 
        !pExpr->AsLambda()->GetStatementLambdaBody()->linkedIntoStatementList)
    {
        ParseTree::StatementList *pList = pExpr->AsLambda()->GetStatementLambdaBody()->ContainingList;

        (*ppListElement)->NextLexical = pList;
        pList->PreviousLexical = *ppListElement;

#if DEBUG
        // Check to ensure the assumption that the statement that is next in block to the lambda body
        // is the end construct for the lambda body AND that the end construct for the lambda body is
        // the last lexical statement in the statement list that contains the lambda body.
        if (pList->NextInBlock)
        {
            // Microsoft: 486319
            // It turns out that while the below assumptions hold for compiler parsing, it does not
            // hold when the IDE calls through here.
            // IE, the IDE will call this method even if there are errors, while in the compiler, we will
            // not call this method. So Remove this.

            //ThrowIfFalse((pList->NextInBlock->Element->Opcode == ParseTree::Statement::EndSub) ||
            //             (pList->NextInBlock->Element->Opcode == ParseTree::Statement::EndFunction));
            //ThrowIfFalse(pList->NextInBlock == Parser::GetLastLexicalStatement(pList));
        }
#endif 

        // The last statement linked (assumed to be pListElement) should be the last lexical statement in the statement list that contains the lambda body.
        *ppListElement = pList->NextInBlock ? pList->NextInBlock : Parser::GetLastLexicalStatement(pList);

        pExpr->AsLambda()->GetStatementLambdaBody()->linkedIntoStatementList = true;
        
    }

    // Always return false because we want the parsetree visitor to keep digging into the Statement node's parse tree.
    return false; 
}

void
Parser::IterateAndAddMultilineLambdaToLexicalStatementList( _In_ ParseTree::LambdaBodyIter *pLambdaBodyIter)
{
    ParseTree::LambdaBodyStatement *pLambdaBody = NULL;
    
    while (pLambdaBody = pLambdaBodyIter->GetNext())
    {
        VSASSERT( pLambdaBody->GetRawParent() && pLambdaBody->GetRawParent()->Opcode == ParseTree::Statement::HiddenBlock,
                  "Expect the lambda body's parent to be a HiddenBlock.");

        VSASSERT( pLambdaBody->GetRawParent()->Children &&
                  (pLambdaBody->GetRawParent()->Children->Element == pLambdaBody),
                  "Expect the hidden block's first child to be the lambda body.");
        
        ParseTree::StatementList *pList = pLambdaBody->ContainingList;

        m_LastStatementLinked->NextLexical = pList;
        pList->PreviousLexical = m_LastStatementLinked;

#if DEBUG
        // Check to ensure the assumption that the statement that is next in block to the lambda body
        // is the end construct for the lambda body AND that the end construct for the lambda body is
        // the last lexical statement in the statement list that contains the lambda body.
        if (pList->NextInBlock)
        {
            ThrowIfFalse((pList->NextInBlock->Element->Opcode == ParseTree::Statement::EndSub) ||
                         (pList->NextInBlock->Element->Opcode == ParseTree::Statement::EndFunction));
            
            ThrowIfFalse(pList->NextInBlock == Parser::GetLastLexicalStatement(pList));
        }
#endif 

        // The last statement linked should be the last lexical statement in the statement list that contains the lambda body.
        m_LastStatementLinked = pList->NextInBlock ? pList->NextInBlock : Parser::GetLastLexicalStatement(pList);
    }
    
}

ParseTree::StatementList*
Parser::GetLastLexicalStatement(_In_ ParseTree::StatementList *pList)
{
    ThrowIfNull(pList);

    while (pList->NextLexical)
    {
        pList = pList->NextLexical;
    }

    return pList;
}

// End the current context and restore its parent as the current context.

void
Parser::PopContext
(
    bool ContextHasProperTermination,
    _In_ Token *StartOfEndConstruct
)
{
    // For IntelliSense (or any other context that parses one line and expects
    // to get a list of statements back), popping the statement target back
    // to a construct enclosing the original context causes subsequently
    // parsed statements to be lost. Therefore, in such cases leave the
    // context index (which controls the statement insertion target) alone.
    //
    // In fact, the requirement is even stronger. LinkStatement puts the
    // first statement parsed in the current context's statement list. If the
    // context is popped prior to linking in a statement, the first statement
    // linked would become a child of the parent of the original context
    // if setting the context to the parent of the original context were
    // permitted.
    //
    // 




    if (m_ContextIndex > 0)
    {
        SetBlockEndLocationInfo(StartOfEndConstruct);

        m_ContextIndex--;

        if (ContextHasProperTermination)
        {
            m_Context->HasProperTermination = true;
        }

        m_Context = m_Context->GetRawParent();
    }
}

// End the current context of a comment block and restore its parent as the current context.
void
Parser::PopBlockCommentContext
(
    Token *StartOfEndConstruct
)
{
    if (m_ContextIndex > 0)
    {
        m_Context->BodyTextSpan.m_lEndLine = m_LastStatementLinked->Element->TextSpan.m_lEndLine;
        m_Context->BodyTextSpan.m_lEndColumn = m_LastStatementLinked->Element->TextSpan.m_lEndColumn;

        // The text span for the comment block statement is the same as the span for the first comment
        VSASSERT(m_Context->Children && m_Context->Children->Element, "Comment block must have at least one child comment");
        m_Context->TextSpan = m_Context->Children->Element->TextSpan;

        m_ContextIndex--;

        // Comment blocks alway have a proper termination.
        m_Context->HasProperTermination = true;
        m_Context = m_Context->GetParent();
    }
}

// Decide whether or not we want to terminate a currently existing comment block context,
// and terminate the comment block context if we need to.
void
Parser::EndCommentBlockIfNeeded
(
)
{
    // First, we have to be inside a comment block
    if (m_Context && m_Context->Opcode == ParseTree::Statement::CommentBlock)
    {
        if (m_IsXMLDocEnabled)
        {
            // If XMLDoc comments are enabled, then terminate a comment block if:
            // 1) We are inside an XMLDoc comment block and we see an non XMLDocComment token, or
            // 2) We are in a normal comment block and we see a non Rem token, or
            // 3) There is a blank line inside an XMLDoc comment block.
            if ((m_Context->AsCommentBlock()->IsXMLDocComment && m_CurrentToken->m_TokenType != tkXMLDocComment) ||
                (!m_Context->AsCommentBlock()->IsXMLDocComment && m_CurrentToken->m_TokenType != tkREM) ||
                (m_Context->AsCommentBlock()->IsXMLDocComment && (m_CurrentToken->m_StartLine - m_LastStatementLinked->Element->TextSpan.m_lEndLine > 1)))
            {
                EndContext(m_CurrentToken, ParseTree::Statement::EndCommentBlock, NULL);
            }
        }
        else
        {
            // If XMLDoc comments are disabled, then terminate the current comment block if we see anything
            // other than an XMLDocComment token or a Rem token.
            if ((m_CurrentToken->m_TokenType != tkREM) && (m_CurrentToken->m_TokenType != tkXMLDocComment))
            {
                EndContext(m_CurrentToken, ParseTree::Statement::EndCommentBlock, NULL);
            }
        }
    }
}

ParseTree::Statement *
Parser::EndContext
(
    _In_ Token *StartOfEndConstruct,
    ParseTree::Statement::Opcodes EndOpcode,
    _In_opt_ ParseTree::Expression *Operand
)
{
    ParseTree::BlockStatement *TerminatedContext = NULL;

    // The current context can't be ended unless it was created during this parsing
    // session.

    if (m_ContextIndex > 0)
    {
        // Determine the start of the context that is ended. If and Try statements can
        // have siblings between themselves and their end constructs. Walk backwards
        // through these.

        TerminatedContext = m_Context;

        if (EndOpcode == ParseTree::Statement::EndCommentBlock)
        {
            PopBlockCommentContext(StartOfEndConstruct);
        }
        else
        {
            PopContext(true, StartOfEndConstruct);
        }

        switch (TerminatedContext->Opcode)
        {
            case ParseTree::Statement::BlockElse:
            case ParseTree::Statement::ElseIf:
            case ParseTree::Statement::Finally:
            case ParseTree::Statement::Catch:
            {
                ParseTree::StatementList *TerminatedStatement = m_LastInContexts[m_ContextIndex];

                if (TerminatedStatement)
                {
                    VSASSERT(TerminatedStatement->Element == TerminatedContext, "Context matching in parsing is lost.");

                    do
                    {
                        TerminatedStatement = TerminatedStatement->PreviousInBlock;
                    }
                    while (TerminatedStatement &&
                           (TerminatedStatement->Element->Opcode == ParseTree::Statement::BlockElse ||
                            TerminatedStatement->Element->Opcode == ParseTree::Statement::ElseIf ||
                            TerminatedStatement->Element->Opcode == ParseTree::Statement::Finally ||
                            TerminatedStatement->Element->Opcode == ParseTree::Statement::Catch));

                    if (TerminatedStatement &&
                        (TerminatedStatement->Element->Opcode == ParseTree::Statement::BlockIf ||
                         TerminatedStatement->Element->Opcode == ParseTree::Statement::Try))
                    {
                        TerminatedContext = TerminatedStatement->Element->AsBlock();
                        TerminatedContext->HasProperTermination = true;
                    }
                }
            }
        }
    }

    ParseTree::Statement *Result = NULL;

    switch (EndOpcode)
    {
        case ParseTree::Statement::EndLoopWhile:
        case ParseTree::Statement::EndLoopUntil:
    
            Result =
                CreateEndConstructWithOperand(StartOfEndConstruct, EndOpcode, Operand);
            break;
    
            // Comment blocks have implicit end constructs.
        case ParseTree::Statement::EndCommentBlock:
            break;
    
        default:
    
            VSASSERT(Operand == NULL, "End construct that can't have an operand has one.");
            Result =
                CreateEndConstruct(StartOfEndConstruct, EndOpcode);
            break;
    }

    if (TerminatedContext)
    {
        TerminatedContext->TerminatingConstruct = Result;

        if( TerminatedContext->Opcode == ParseTree::Statement::LambdaBody)
        {

            VSASSERT( m_Context->Opcode == ParseTree::Statement::HiddenBlock, "There should have been a hidden block context");

            // Pop the hidden block context.
            PopContext(true, m_CurrentToken);        
        }
    }
    
    return Result;
}

ParseTree::BlockStatement *
Parser::RealContext
(
)
{

    ParseTree::BlockStatement *Context = m_Context;

    while (Context->Opcode == ParseTree::Statement::Region)
    {
         Context = Context->GetParent();
    }

    return Context;
}

ParseTree::BlockStatement *
Parser::FindContainingHandlerContext
(
)
{
    for (ParseTree::BlockStatement *Context = m_Context;
         Context;
         Context = Context->GetParent())
    {
        if (Context->Opcode == ParseTree::Statement::Try ||
            Context->Opcode == ParseTree::Statement::Catch ||
            Context->Opcode == ParseTree::Statement::Finally)
        {
            return Context;
        }
    }

    return NULL;
}

bool
Parser::FindContainingContext
(
    ParseTree::BlockStatement *TargetContext
)
{
    for (ParseTree::BlockStatement *Context = m_Context;
         Context;
         Context = Context->GetParent())
    {
        if (Context == TargetContext)
        {
            return true;
        }
    }

    return false;
}

bool
Parser::ConfirmParentChildRelationship
(
    _In_ ParseTree::BlockStatement *pExpectedParent,
    _In_ ParseTree::Statement *pChild
)
{
    ThrowIfNull(pExpectedParent);
    ThrowIfNull(pChild);
    
    ParseTree::BlockStatement * pActualParent = pChild->GetParent();

    while( pActualParent && pActualParent != pExpectedParent)
    {
        pActualParent = pActualParent->GetParent();
    }

    return (pActualParent == pExpectedParent);
}

//
//============ Methods to encapsulate scanning ========================
//

// Parser::IsValidStatementTerminator: Is this token a valid "StatementTerminator"?
// NOTE: a compound statement e.g. "If b then : S1 : end if" has StatementTerminators inside it
// This function is used e.g. in parsing "If b Then <token>" to determine whether it's a line-if or a block-if.
// The statement terminators are newline, colon, comment. (Also due to a quirk in the way things flow together,
// "else" is considered a statement terminator if we're in a line-if.)
bool
Parser::IsValidStatementTerminator
(
    _In_ Token *T
)
{
    return
        T->m_TokenType == tkColon ||
        T->m_TokenType == tkEOL ||
        // The parser usually skips EOL tokens (if implicit line continuation is
        // enabled), and it isn't possible to look back past the first token of a
        // line, so test if this is the first token of the last read line.
        T == m_FirstTokenAfterNewline ||
        T->m_TokenType == tkEOF ||
        T->m_TokenType == tkREM ||
        T->m_TokenType == tkXMLDocComment ||
        // (bug 32704) "else" is a special case in the construct "if foo then resume else stmt"
        (m_IsLineIf && T->m_TokenType == tkELSE);
}


// Parser::CanFollowStatement -- Can this token follow a complete statement?
// NOTE: e.g. in "Dim x = Sub() S, y=3", if we're within a statement lambda, then the token tkCOMMA
// can follow a complete statement
// This function is used e.g. in parsing "End <token>" or "Call f() <token>" to determine whether
// the token is part of the construct, or may be part of the following construct, or is just an error.
bool
Parser::CanFollowStatement
(
    _In_ Token *T
)
{
     // Dev10#670492: in a single-line sub, e.g. "Dim x = (Sub() Stmt)", things like RParen are happy to end the statement
     return IsParsingSingleLineLambda() ? CanFollowExpression(T) : IsValidStatementTerminator(T);
}


// Parser::CanEndExecutableStatement -- Can this token follow a complete "executable" statement?
// WARNING: This function is *only* used when parsing a Call statement, to judge whether the tokens that follow
// it should produce an "obsolete: arguments must be enclosed in parentheses" message. It shouldn't be
// used by anything else.
// It's only difference from "CanFollowStatement" is that it returns true when given tkELSE: for all other
// inputs it's identical.
bool
Parser::CanEndExecutableStatement
(
    _In_ Token *T
)
{
    return CanFollowStatement(T) || T->m_TokenType == tkELSE;
}


// Parser::CanFollowExpression -- Can this token follow a complete expression??
// NOTE: a statement can end with an expression. Therefore, the set denoted by CanFollowExpression
// is not smaller than that denoted by CanFollowStatement. (actually, the two sets are equal
// if we happen to be parsing the statement body of a single-line sub lambda).
bool
Parser::CanFollowExpression
(
    _In_ Token *T
)
{
   return
       (T->m_TokenType == tkID && g_tkKwdDescs[TokenAsKeyword(T)].kdCanFollowExpr) || // e.g. "Aggregate" can end an expression
       g_tkKwdDescs[T->m_TokenType].kdCanFollowExpr ||
       IsValidStatementTerminator(T);
}

bool
Parser::BeginsEmptyStatement
(
    _In_ Token *T
)
{
    return
        T->m_TokenType == tkColon ||
        T->m_TokenType == tkEOF ||
        T->m_TokenType == tkREM ||
        T->m_TokenType == tkXMLDocComment ||
        T->m_TokenType == tkEOL;
}

bool
Parser::BeginsStatement
(
    _In_ Token *T
)
{
    if (!IsValidStatementTerminator(T))
    {
        return false;
    }

    switch (T->m_TokenType)
    {
        case tkADDHANDLER:
        case tkCALL:
        case tkCASE:
        case tkCATCH:
        case tkCLASS:
        case tkCONST:
        case tkCUSTOM:
            // Fall through
        case tkDECLARE:
        case tkDELEGATE:
         case tkDIM:
        case tkDO:
        case tkELSE:
        case tkELSEIF:
        case tkEND:
        case tkENDIF:
        case tkENUM:
        case tkERASE:
        case tkERROR:
        case tkCONTINUE:
        case tkEVENT:
        case tkEXIT:
        case tkPARTIAL:
        case tkFINALLY:
        case tkFOR:
        case tkFRIEND:
        case tkFUNCTION:
        case tkOPERATOR:
        case tkGET:
        case tkGOTO:
        case tkGOSUB:
        case tkIF:
        case tkIMPLEMENTS:
        case tkIMPORTS:
        case tkINHERITS:
        case tkINTERFACE:
        case tkLOOP:
        case tkMODULE:
        case tkMUSTINHERIT:
        case tkMUSTOVERRIDE:
        case tkNAMESPACE:
        case tkNARROWING:
        case tkNEXT:
        case tkNOTINHERITABLE:
        case tkNOTOVERRIDABLE:
        case tkOPTION:
        case tkOVERLOADS:
        case tkOVERRIDABLE:
        case tkOVERRIDES:
        case tkPRIVATE:
        case tkPROPERTY:
        case tkPROTECTED:
        case tkPUBLIC:
        case tkRAISEEVENT:
        case tkREADONLY:
        case tkREDIM:
        case tkREMOVEHANDLER:
        case tkRESUME:
        case tkRETURN:
        case tkSELECT:
        case tkSHADOWS:
        case tkSHARED:
        case tkSTATIC:
        case tkSTOP:
        case tkSTRUCTURE:
        case tkSUB:
        case tkSYNCLOCK:
        case tkTHROW:
        case tkTRY:
        case tkUSING:
        case tkWEND:
        case tkWHILE:
        case tkWIDENING:
        case tkWITH:
        case tkWITHEVENTS:
        case tkWRITEONLY:
        case tkSharp:
        case tkAWAIT:
        case tkYIELD:
            return true;
    }

    return false;
}

Token* // Returns the token following tkOF, or NULL if we aren't looking at generic type syntax 
Parser::BeginsGeneric // A generic is signified by '(' [tkEOL] tkOF
(
    _In_ Token *T 
)
{
    // Returning a token instead of a bool is useful to retain the optional
    // parantheses semantics in case the decision to make them mandatory is
    // ever changed.

    if (T->m_TokenType == tkLParen)
    {
        Token * pNext = T->m_Next;

        if (pNext->IsContinuableEOL()) // See if we need to go to the next line to find tkOF since '(' allows for implicit line continuation
        {
            Token * pNextLine = PeekNextLine(pNext);
            if (pNextLine->m_StartLine == pNext->m_StartLine + 1) // Microsoft: this seems like a wierd check.  If the EOL is continuable this condition is always true so why check?
            {
                pNext = pNextLine;
            }
        }

        if (pNext->m_TokenType == tkOF)
        {
            return T->m_Next;
        }
    }

    return NULL;
}

bool
Parser::BeginsGenericWithPossiblyMissingOf
(
    _In_ Token *T
)
{
    if (BeginsGeneric(T))
    {
        return true;
    }
    else if (T->m_TokenType == tkLParen)
    {

        // To enable a better user experience in some common generics'
        // error scenarios, we special case foo(Integer) and
        // foo(Integer, Junk).
        //
        // "(Integer" indicates possibly type parameters with missing "of",
        // but not "(Integer." and "Integer!" because they could possibly
        // imply qualified names or expressions. Also note that "Integer :="
        // could imply named arguments. Here "Integer" is just an example,
        // it could be any intrinsic type.
        //

        Token * pNext = T->m_Next;

        if (pNext->IsContinuableEOL())
        {
            pNext = PeekNextLine(pNext);
        }
        
        return
            IsIntrinsicType(pNext) &&
            (
                pNext->m_Next->m_TokenType == tkRParen ||
                pNext->m_Next->m_TokenType == tkComma
            );
    }
    else
    {
        return false;
    }
}

inline bool
Parser::BeginsEvent
(
    _In_ Token *T
)
{
    VSASSERT(T->m_TokenType != tkCUSTOM ||
             (T->m_TokenType == tkCUSTOM && T->m_Next->m_TokenType == tkEVENT),
                "Malformed custom event token stream!!!");

    return
        T->m_TokenType == tkEVENT || T->m_TokenType == tkCUSTOM;
}

bool
Parser::MustEndStatement
(
    _In_ Token *T
)
{
    return
        T->m_TokenType == tkColon ||
        T->m_TokenType == tkEOF ||
        T->m_TokenType == tkEOL ||
        T->m_TokenType == tkREM ||
        T->m_TokenType == tkXMLDocComment  ||
        (T == m_FirstTokenAfterNewline && BeginsStatement(T));
}

bool
Parser::IsEndOfLine
(
    _In_ Token *T
)
{
    return
        T->m_TokenType == tkEOL ||
        // The parser usually skips EOL tokens, and it isn't possible to look
        // back past the first token of a line, so test if this is the first
        // token of the last read line.
        T == m_FirstTokenAfterNewline ||
        T->m_TokenType == tkEOF;
}

Token *
Parser::Following
(
    _In_ Token *T
)
{
    Token *Next = T->m_Next;
    
    return Next;
}


Token *
Parser::GetNextLine
(
    tokens TokenType
)
{
    m_FirstTokenAfterNewline = NULL;
    m_FirstStatementOnLine = true;
    m_CurrentToken = NULL;
    AbandonAllLines();
    return m_InputStream->GetNextLine(TokenType);
}

Token * Parser::PeekNextLine(Token * pAfter)
{
    Token * pToken = pAfter ? pAfter : m_CurrentToken; // Start on the current token, or the one the caller specifies
    // Find the EOL
    while (!pToken->IsEndOfLine() && pToken->m_Next)
    {
        pToken = pToken->m_Next;
    }

    if (pToken && pToken->IsEndOfLine())
    {
        // Don't get the next line if a previous PeekNextLine() has already pulled it in.  
        // We don't want to ---- beyond where the stream point currently is in the text
        if (!pToken->m_EOL.m_NextLineAlreadyScanned) 
        {
            pToken->m_EOL.m_NextLineAlreadyScanned = true;
            if ( m_ForceMethodDeclLineState )
            {
                // Scanner doesn't have enough context to know we are doing parameters.
                // It needs to know so it can tell that XML is not allowed but should rather
                // scan '<' as an attribute.
                // Tokens are added to the end of the current token ring
                m_InputStream->GetNextLine( FunctionDecl );
            }
            else
            {
                m_InputStream->GetNextLine(); // adds tokens to the end of the current token ring
            }
        }
        
        if (pToken == m_CurrentToken && pToken->m_Next->m_TokenType != tkEOF)
        {
            //The idea here is that if we use this function to lookahead more than one token, then we don't
            //want to set m_FirstTokenAfterNewLine because we may end up digging through multiple new lines,
            //and then determine that we don't want to take a particular path in the parser. If we did that and we set
            //m_FirstTokenAfterNewLine we could end up setting it to a token that is not on the line immedietly after 
            //the current line, which could mess up the parser's error recovery. To avoid that we only set the
            //m_FirstTokenAfterNewLine if we are looking ahead exactly one token.
            m_FirstTokenAfterNewline = pToken->m_Next;
        }            

        return pToken->m_Next;
    }
    else
    {
        return NULL;
    }
}

/*****************************************************************************************
;GoToEndOfLine

Find the tkEOL at the end of the line.  This doesn't affect the current parsing position,
i.e. m_CurrentToken is not affected.
******************************************************************************************/
void Parser::GoToEndOfLine
(
    _Inout_ Token **ppInputToken // [in] where you want to start looking from [out] set to the EOL at the end of the line
)
{
    VSASSERT( ppInputToken && *ppInputToken, "Expected a current token");

    Token *pInputToken = *ppInputToken;
    
    while (pInputToken->m_TokenType != tkEOF && pInputToken->m_TokenType != tkEOL)
    {
        pInputToken = pInputToken->m_Next;
        ThrowIfNull(pInputToken); // We should always have a EOL token at the end of a line.
    }

    *ppInputToken = pInputToken;
}

// Get the tokens for the next line, screening out labels and line numbers.

void Parser::GetTokensForNextLine
(
    bool WithinMethod // Are we looking for tokens within a method body?
)
{
    bool ErrorInConstruct = false;

    Token *Next = NULL;

    if (m_CurrentToken)
    {
        if (m_CurrentToken->m_TokenType == tkEOL)
        {
            if (m_CurrentToken->m_EOL.m_NextLineAlreadyScanned)
            {
                VSASSERT(!m_CurrentToken->m_Next ||
                    m_CurrentToken->m_Next->m_TokenType == tkEOF ||
                    m_CurrentToken->m_Next->m_TokenType == tkEOL ||
                    (m_CurrentToken->m_Next->m_StartLine > m_CurrentToken->m_StartLine), "not advancing to a new line");
                Next = m_CurrentToken->m_Next;
            }
        }
        else if (m_CurrentToken == m_FirstTokenAfterNewline)
        {
            Next = m_CurrentToken;
        }
    }

    m_CurrentToken = Next ? Next : GetNextLine();

    AbandonPreviousLines();
    m_FirstTokenAfterNewline = NULL;
    m_FirstStatementOnLine = true;

    Token *First = m_CurrentToken;

    // Check for a line number.

    if (First->m_TokenType == tkIntCon)
    {
        if (WithinMethod)
        {
            if (m_SelectBeforeFirstCase)
            {
                ReportSyntaxError(ERRID_ExpectedCase, First, First->m_Next->m_Next, ErrorInConstruct);
            }

            ParseTree::LabelReferenceStatement *Label = new(m_TreeStorage) ParseTree::LabelReferenceStatement;
            Label->Opcode = ParseTree::Statement::Label;

            CacheStmtLocation();
            SetStmtBegLocation(Label, First);
            ParseLabelReference(Label, ErrorInConstruct);
            SetStmtEndLocation(Label, m_CurrentToken);

            if (m_CurrentToken->m_TokenType == tkColon)
            {
                GetNextToken();
            }
            else
            {
                // Line numbers have to be labels now (i.e. have a trailing :)

                ReportSyntaxErrorWithoutMarkingStatementBad(
                    ERRID_ObsoleteLineNumbersAreLabels,
                    First,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            LinkStatement(Label);
        }
        else
        {
            ReportSyntaxError(ERRID_InvOutsideProc, First, ErrorInConstruct);
        }
        First = m_CurrentToken;
    }
    // Check for a named label.
    else if (First->m_TokenType == tkID &&
        m_CurrentToken->m_Next->m_TokenType == tkColon)
    {

        if (WithinMethod)
        {
            if (m_SelectBeforeFirstCase)
            {
                ReportSyntaxError(ERRID_ExpectedCase, First, First->m_Next->m_Next, ErrorInConstruct);
            }

            ParseTree::LabelReferenceStatement *Label = new(m_TreeStorage) ParseTree::LabelReferenceStatement;
            Label->Opcode = ParseTree::Statement::Label;

            CacheStmtLocation();
            SetStmtBegLocation(Label, First);
            ParseLabelReference(Label, ErrorInConstruct);
            SetStmtEndLocation(Label, m_CurrentToken);

            LinkStatement(Label);
        }
        else
        {
            ReportSyntaxError(ERRID_InvOutsideProc, First, First->m_Next->m_Next, ErrorInConstruct);
        }

        // Advance past the colon.

        GetNextToken();
    }
}

/*********************************************************************
*
* Function:
*     Parser::PeekAheadFor
*
* Purpose:
*     Peek ahead the statement for the requested tokens. Return the
*     token that was found. If the requested tokens are not encountered,
*     return tkNone.
*
*     This routine does not consume the tokens. The current token is not
*     advanced. To consume tokens, use ResyncAt().
*
**********************************************************************/
tokens __cdecl
Parser::PeekAheadFor
(
    unsigned Count,        // [in] count of tokens to look for
    ...                     // [in] var_list of tokens (tk[]) to look for
)
{
    va_list Tokens;

    Token *Next = m_CurrentToken;
    while (!IsValidStatementTerminator(Next))
    {
        va_start(Tokens, Count);

        for (unsigned i = 0; i < Count; i++)
        {
            tokens TokenType = va_arg(Tokens, tokens);

            if (TokenType == Next->m_TokenType ||
                EqNonReservedToken(Next, TokenType))
            {
                va_end(Tokens);
                return TokenType;
            }
        }

        Next = Next->m_Next;
        va_end(Tokens);
    }

    return tkNone;
}

// Needed a seperate method to peek for comments because
// PeekAheadFor ends before finding comment tokens.
tokens __cdecl
Parser::PeekAheadForComment
(
)
{
    Token *Next = m_CurrentToken;
    while (!IsValidStatementTerminator(Next))
    {
        Next = Next->m_Next;
    }

    if (Next->m_TokenType == tkREM)
    {
        return tkREM;
    }
    
    return tkNone;
}

/*********************************************************************
*
* Function:
*     Parser::PeekAheadForToken
*
* Purpose:
*     Peek ahead the statement for the requested tokens. Return the
*     token Object that was found. If the requested tokens are not encountered,
*     return NULL.
*
*     This routine does not consume the tokens. The input token is not
*     advanced. To consume tokens, use ResyncAt().
*
**********************************************************************/
Token* __cdecl
Parser::PeekAheadForToken
(
    _In_ Token *pInputToken, // Initial token
    unsigned Count,        // [in] count of tokens to look for
    ...                     // [in] var_list of tokens (tk[]) to look for
)
{
    va_list Tokens;

    VSASSERT( pInputToken != NULL, "Invalid input");
    Token *Next = pInputToken;
    
    while (!IsValidStatementTerminator(Next))
    {
        va_start(Tokens, Count);

        for (unsigned i = 0; i < Count; i++)
        {
            tokens TokenType = va_arg(Tokens, tokens);

            if (TokenType == Next->m_TokenType ||
                EqNonReservedToken(Next, TokenType))
            {
                va_end(Tokens);
                return Next;
            }
        }

        Next = Next->m_Next;
        va_end(Tokens);
    }

    return NULL;
}


/*********************************************************************
*
* Function:
*     Parser::ResyncAt
*
* Purpose:
*     Consume tokens on the line until we encounter a token in the
*     given list or EOS.
*
**********************************************************************/
tokens 
Parser::ResyncAtInternal
(
    bool TreatColonAsStatementTerminator,
    unsigned Count,        // [in] count of tokens to look for
    va_list Tokens         // [in] var_list of tokens to look for
)
{
    
    while ((!TreatColonAsStatementTerminator|| m_CurrentToken->m_TokenType != tkColon) &&
           m_CurrentToken->m_TokenType != tkEOF &&
           m_CurrentToken->m_TokenType != tkEOL &&
           !BeginsStatement(m_CurrentToken))
    {
        va_list TokenCopy = Tokens;
        for (unsigned i = 0; i < Count; i++)
        {
            tokens TokenType = va_arg(TokenCopy, tokens);
            if (TokenType == m_CurrentToken->m_TokenType ||
                EqNonReservedToken(m_CurrentToken, TokenType))
            {
                return TokenType;
            }
        }

        GetNextToken();

    }
    return tkNone;
}

tokens __cdecl
Parser::ResyncAt
(
    unsigned Count,        // [in] count of tokens to look for
    ...                     // [in] var_list of tokens to look for
)
{
    va_list Tokens;
    va_start(Tokens, Count);
    return ResyncAtInternal(true, Count, Tokens);
}

tokens __cdecl
Parser::CCResyncAt
(
    unsigned Count,        // [in] count of tokens to look for
    ...                     // [in] var_list of tokens to look for
)
{
    va_list Tokens;
    va_start(Tokens, Count);
    return ResyncAtInternal(false, Count, Tokens);
}



//
//============ Methods related to conditional compilation. ==============
//

// Parse a line containing a conditional compilation directive.

void
Parser::ParseConditionalCompilationStatement
(
    bool SkippingMethodBody
)
{
#pragma prefast(push)
#pragma prefast(disable:25011, "This method needs to be carefully examined.  Too many missing __fallthrough's")
    CacheStmtLocation();

    Token *Start = m_CurrentToken;

    VSASSERT(
        Start->m_TokenType == tkSharp,
        "Conditional compilation lines start with \'#\'.");

    bool ErrorInConstruct = false;

    // First parse the conditional line, then interpret it.

    ParseTree::Statement *Statement = NULL;
    SaveErrorStateGuard errorStateGuard(this);

    if (m_ParsingMethodBody || 
        m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
    {
        // The errors will have been reported during the declaration parse,
        // and don't need to be repeated.
        //
        // Both syntactic (via DisableErrors) and semantic (via setting
        // the error table to NULL) errors must be disabled.

        DisableErrors();
        m_Errors = NULL;
    }

    // Every path that exits this method must restore error reporting.
    // The try ... finally construct accomplishes this.

    switch (GetNextToken()->m_TokenType)
    {
        case tkELSE:
        {
            if (m_CurrentToken->m_Next->m_TokenType != tkIF)
            {
                GetNextToken();

                Statement = new(m_TreeStorage) ParseTree::CCElseStatement;
                Statement->Opcode = ParseTree::Statement::CCElse;
                break;
            }
            else
            {
                // Accept Else If as a synonym for ElseIf.

                GetNextToken();
            }

            __fallthrough; // Fall through.
        }

        case tkIF:
        case tkELSEIF:
        {
            Statement = new(m_TreeStorage) ParseTree::CCIfStatement;
            Statement->Opcode =
                Start->m_Next->m_TokenType == tkIF ?
                    ParseTree::Statement::CCIf :
                    ParseTree::Statement::CCElseIf;

            SetPunctuator(&Statement->AsCCIf()->IfOrElseIf, Start, m_CurrentToken);
            GetNextToken();

            Statement->AsExpression()->Operand = ParseConditionalCompilationExpression(ErrorInConstruct);

            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            if (m_CurrentToken->m_TokenType == tkTHEN)
            {
                SetPunctuator(&Statement->AsCCIf()->Then, Start, m_CurrentToken);
                GetNextToken();
            }

            break;
        }

        case tkEND:
        {
            if (m_CurrentToken->m_Next->m_TokenType == tkIF)
            {
                Statement = new(m_TreeStorage) ParseTree::CCEndStatement;
                Statement->Opcode = ParseTree::Statement::CCEndIf;

                SetPunctuator(&Statement->AsCCEnd()->End, Start, m_CurrentToken);
                SetPunctuator(&Statement->AsCCEnd()->Punctuator, Start, m_CurrentToken->m_Next);
                GetNextToken();
                GetNextToken();

                break;
            }

            else if (EqNonReservedToken(m_CurrentToken->m_Next, tkREGION))
            {
                if (SkippingMethodBody)
                {
                    return;
                }

                if (m_ParsingMethodBody || m_pStatementLambdaBody)
                {
                    // Force reporting of this error during method body parsing.
                    errorStateGuard.RunAction();
                    LinkStatement(
                        ReportUnrecognizedStatementError(
                            ERRID_RegionWithinMethod,
                            ErrorInConstruct));
                    return;
                }

                Token *Region = m_CurrentToken;
                GetNextToken();
                GetNextToken();

                if (m_Context->Opcode == ParseTree::Statement::Region)
                {
                    ParseTree::BlockStatement *EndedRegion = m_Context;
                    ParseTree::StatementList *LastRegionBodyLink = m_LastInContexts[m_ContextIndex];

                    ParseTree::Statement *EndRegion =
                        EndContext(Start, ParseTree::Statement::EndRegion, NULL);

                    // Move the statements within the Region block to be siblings
                    // of the Region block. This makes semantic processing of the
                    // statements within the Region simpler.

                    MoveRegionChildrenToRegionParent(EndedRegion, EndRegion, LastRegionBodyLink);

                    VerifyEndOfConditionalCompilationStatement(ErrorInConstruct);
                    break;
                }

                // ParseOneLine creates end nodes for mismatched ends.

                if (m_IsLineParse)
                {
                    m_CurrentStatementInError = true;
                    CreateEndConstruct(Start, ParseTree::Statement::EndRegion);
                }
                else
                {
                    RecoverFromMismatchedEnd(Start, ParseTree::Statement::EndRegion, ErrorInConstruct);
                }

                break;
            }

            else if (EqNonReservedToken(m_CurrentToken->m_Next, tkEXTERNALSOURCE))
            {
                if (m_ParsingMethodBody ||
                    m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                {
                    // External source directives are fully processed during declaration
                    // parsing, and so are ignored during method body parsing.

                    ResyncAt(0);
                }
                else
                {
                    if (m_CurrentExternalSourceDirective)
                    {
                        m_CurrentExternalSourceDirective->LastLine = (unsigned)m_CurrentToken->m_StartLine - 1;
                        m_CurrentExternalSourceDirective = NULL;
                    }

                    else if (m_ExternalSourceDirectiveTarget)
                    {
                        ReportSyntaxError(ERRID_EndExternalSource, m_CurrentToken, ErrorInConstruct);
                    }

                    GetNextToken();
                    GetNextToken();
#if IDE
                    //VSWhidbey#36069,don't remove comments
                    //eat everything after #end externalsource
                    while (!IsEndOfLine(m_CurrentToken))
                    {
                        GetNextToken();
                    }
#else
                    //vbc.exe here
                    VerifyEndOfConditionalCompilationStatement(ErrorInConstruct);
#endif
                }

                // There are no trees to process for these.
                return;
            }

            else
            {
                if (SkippingMethodBody)
                {
                    return;
                }

                // Force reporting of this error during method body parsing.
                errorStateGuard.RunAction();

                // Make the error appear for the token following the End.
                GetNextToken();

                LinkStatement(
                    ReportUnrecognizedStatementError(
                        ERRID_Syntax,
                        ErrorInConstruct));
                return;
            }

            break;
        }

        case tkENDIF:
        {
            if (!SkippingMethodBody)
            {
                // EndIf is anachronistic

                ReportSyntaxErrorWithoutMarkingStatementBad(
                    ERRID_ObsoleteEndIf,
                    m_CurrentToken,
                    ErrorInConstruct);
            }

            Statement = new(m_TreeStorage) ParseTree::CCEndStatement;
            Statement->Opcode = ParseTree::Statement::CCEndIf;

            SetPunctuator(&Statement->AsCCEnd()->End, Start, m_CurrentToken);
            GetNextToken();
            break;
        }

        case tkCONST:
        {
            Statement = new(m_TreeStorage) ParseTree::CCConstStatement;
            Statement->Opcode = ParseTree::Statement::CCConst;

            SetPunctuator(&Statement->AsCCConst()->Const, Start, m_CurrentToken);
            GetNextToken();

            Statement->AsNamedValue()->Name = ParseIdentifier(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(1, tkEQ);
            }

            SetPunctuatorIfMatches(&Statement->AsNamedValue()->Equals, Start, m_CurrentToken, tkEQ);
            VerifyTokenPrecedesExpression(tkEQ, ErrorInConstruct);

            Statement->AsNamedValue()->Value = ParseConditionalCompilationExpression(ErrorInConstruct);
            if (ErrorInConstruct)
            {
                ResyncAt(0);
            }

            break;
        }

        case tkID:
        {
            switch (IdentifierAsKeyword(m_CurrentToken))
            {
                case tkEXTERNALSOURCE:
                {
                    bool hasErrors = false;
                    if (m_ParsingMethodBody ||
                        m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                    {
                        // External source directives are fully processed during declaration
                        // parsing, and so are ignored during method body parsing.

                        ResyncAt(0);
                    }
                    else
                    {
                        GetNextToken();

                        const WCHAR *ExternalSourceFileName = L"";
                        unsigned ExternalSourceFileStartLine = 0;

                        VerifyExpectedToken(tkLParen, ErrorInConstruct);

                        ParseTree::Expression *StringLiteral = ParseStringLiteral(ErrorInConstruct);
                        if (StringLiteral->Opcode == ParseTree::Expression::StringLiteral)
                        {
                            ExternalSourceFileName = StringLiteral->AsStringLiteral()->Value;
                        }

                        VerifyExpectedToken(tkComma, ErrorInConstruct);

                        if (m_CurrentToken->m_TokenType == tkIntCon)
                        {
                            ExternalSourceFileStartLine = (unsigned)m_CurrentToken->m_IntLiteral.m_Value;
                            GetNextToken();
                        }
                        else
                        {
                            ReportSyntaxError(ERRID_ExpectedIntLiteral, m_CurrentToken, ErrorInConstruct);
                            hasErrors = true;
                        }

                        VerifyExpectedToken(tkRParen, ErrorInConstruct);

                        if (ErrorInConstruct)
                        {
                            ResyncAt(0);
                        }

                        if (m_CurrentExternalSourceDirective)
                        {
                            ReportSyntaxError(ERRID_NestedExternalSource, Start, m_CurrentToken, ErrorInConstruct);
                            hasErrors = true;

                        }
                        else if (m_ExternalSourceDirectiveTarget)
                        {
                            ParseTree::ExternalSourceDirective *Directive = new(m_TreeStorage) ParseTree::ExternalSourceDirective;

                            Directive->FirstLine = (unsigned)m_CurrentToken->m_StartLine + 1;
                            // The last line will be determined when the end of the directive occurs.
                            Directive->LastLine = 0;
                            Directive->ExternalSourceFileName = ExternalSourceFileName;
                            Directive->ExternalSourceFileStartLine = ExternalSourceFileStartLine;
                            Directive->Next = NULL;
                            Directive->hasErrors = ErrorInConstruct || hasErrors;

                            *m_ExternalSourceDirectiveTarget = Directive;
                            m_ExternalSourceDirectiveTarget = &Directive->Next;

                            m_CurrentExternalSourceDirective = Directive;
                        }

                        // VSW#43381
                        // External source directives have no parse tree statements, which means
                        // that m_LastStatementLinked is the statement BEFORE the external source
                        // directive.  VerifyEndOfConditionalCompilationStatement changes the comment
                        // list of m_LastStatementLinked, which is wrong in this case.  However, we
                        // still want VerifyEndOfConditionalCompilationStatement to report errors if garbage
                        // text follows the external source directive, so m_LastStatementLinked is temporarily
                        // NULL-ed out during the call
                        ParseTree::StatementList *CachedStatement = m_LastStatementLinked;
                        m_LastStatementLinked = NULL;

                        VerifyEndOfConditionalCompilationStatement(ErrorInConstruct);

                        m_LastStatementLinked = CachedStatement;
                    }

                    // There are no trees to process for these.
                    return;
                }

                case tkEXTERNALCHECKSUM:
                {
                    if (m_ParsingMethodBody ||
                        m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                    {
                        // External checksum directives are fully processed during declaration
                        // parsing, and so are ignored during method body parsing.

                        ResyncAt(0);
                    }
                    else
                    {
                        GetNextToken();

                        const WCHAR *ExternalSourceFileName = L"";
                        const WCHAR *ExternalGuid = L"";
                        const WCHAR *ExternalChecksumVal = L"";

                        VerifyExpectedToken(tkLParen, ErrorInConstruct);

                        ParseTree::Expression *StringLiteralFileName = ParseStringLiteral(ErrorInConstruct);
                        if (StringLiteralFileName->Opcode == ParseTree::Expression::StringLiteral)
                        {
                            ExternalSourceFileName = StringLiteralFileName->AsStringLiteral()->Value;
                        }
                        VerifyExpectedToken(tkComma, ErrorInConstruct);

                        ParseTree::Expression *StringLiteralGuid = ParseStringLiteral(ErrorInConstruct);
                        if (StringLiteralGuid->Opcode == ParseTree::Expression::StringLiteral)
                        {
                            ExternalGuid = StringLiteralGuid->AsStringLiteral()->Value;
                        }
                        VerifyExpectedToken(tkComma, ErrorInConstruct);

                        ParseTree::Expression *StringLiteralChecksumVal = ParseStringLiteral(ErrorInConstruct);
                        if (StringLiteralChecksumVal->Opcode == ParseTree::Expression::StringLiteral)
                        {
                            ExternalChecksumVal = StringLiteralChecksumVal->AsStringLiteral()->Value;
                        }
                        VerifyExpectedToken(tkRParen, ErrorInConstruct);

                        if (ErrorInConstruct)
                        {
                            ResyncAt(0);
                        }
#if !IDE
                        else
                        {
                            ERRID error;
                            bool result =
                                m_Compiler->GetProjectBeingCompiled()->AddExternalChecksum
                                (
                                    ExternalSourceFileName,
                                    ExternalGuid,
                                    ExternalChecksumVal,
                                    error);
                            if (!result)
                            {
                                Location *wrnSpan;
                                switch (error)
                                {
                                case WRNID_BadChecksumValExtChecksum:
                                    wrnSpan = &StringLiteralChecksumVal->TextSpan;
                                    break;
                                case WRNID_MultipleDeclFileExtChecksum:
                                    wrnSpan = &StringLiteralFileName->TextSpan;
                                    break;
                                case WRNID_BadGUIDFormatExtChecksum:
                                    wrnSpan = &StringLiteralGuid->TextSpan;
                                    break;
                                default:
                                    VSFAIL("unexpected error in external checksum");
                                    wrnSpan = &StringLiteralFileName->TextSpan;
                                }

                                ReportSyntaxError(error, wrnSpan, ErrorInConstruct);
                            }
                        }
#endif

                        // Avoid repreat VSW#43381(see above tkExternalChecksum
                        ParseTree::StatementList *CachedStatement = m_LastStatementLinked;
                        m_LastStatementLinked = NULL;

                        VerifyEndOfConditionalCompilationStatement(ErrorInConstruct);

                        m_LastStatementLinked = CachedStatement;
                    }

                    // There are no trees to process for these.
                    return;
                }

                case tkREGION:
                {
                    if (SkippingMethodBody)
                    {
                        return;
                    }

                    if (m_ParsingMethodBody || m_pStatementLambdaBody)
                    {
                        // Force reporting of this error during method body parsing.
                        errorStateGuard.RunAction();

                        LinkStatement(
                            ReportUnrecognizedStatementError(
                                ERRID_RegionWithinMethod,
                                ErrorInConstruct));
                        return;
                    }

                    Statement = new(m_TreeStorage) ParseTree::RegionStatement;
                    Statement->Opcode = ParseTree::Statement::Region;

                    SetPunctuator(&Statement->AsRegion()->Region, Start, m_CurrentToken);
                    GetNextToken();

                    Statement->AsRegion()->Title = ParseStringLiteral(ErrorInConstruct);
                    break;
                }

                default:

                    LinkStatement(
                        ReportUnrecognizedStatementError(
                            ERRID_ExpectedConditionalDirective,
                            ErrorInConstruct));
                    return;
            }

            break;
        }

        default:

            ParseTree::Statement *TempStatement = ReportUnrecognizedStatementError(
                ERRID_ExpectedConditionalDirective,
                ErrorInConstruct);

            // Forced location adjustment: make all # line squigle (bug #158394)
            TempStatement->TextSpan.m_lBegLine = Start->m_StartLine;
            TempStatement->TextSpan.m_lBegColumn = TokenColumnBeg(Start);
            LinkStatement(TempStatement);
            return;
    }

    if (Statement)
    {
        SetStmtBegLocation(Statement, Start);
        SetStmtEndLocation(Statement, m_CurrentToken);
        LinkStatement(Statement);

        VerifyEndOfConditionalCompilationStatement(ErrorInConstruct);
    }

    if (m_IsLineParse || Statement == NULL)
    {
        return;
    }

    switch (Statement->Opcode)
    {
        case ParseTree::Statement::CCIf:
        {
            ConditionalCompilationDescriptor NewConditional;
            NewConditional.IntroducingStatement = Statement->AsCCBranch();
            m_Conditionals.Push(NewConditional);

            InterpretConditionalCompilationConditional(Statement->AsExpression());

            break;
        }

        case ParseTree::Statement::CCElseIf:
        {
            // If there has been no preceding #If, give an error and pretend that there
            // had been one.

            if (m_Conditionals.IsEmpty())
            {
                ConditionalCompilationDescriptor NewConditional;
                NewConditional.IntroducingStatement = Statement->AsCCBranch();

                if (m_ParsingMethodBody ||
                    m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                {
                    // The #If might have preceded the start of the method body, so this
                    // isn't an error.

                    // Inhibit compiling any of the code in the #else if section. This
                    // minimizes the number of spurious error messages.
                    NewConditional.IntroducingStatement->BranchTaken = true;
#if IDE
                    NewConditional.AnyBranchTaken = true;
#endif
                }
                else
                {
                    ReportSyntaxErrorForPreviousStatement(ERRID_LbBadElseif, Start, m_CurrentToken, ErrorInConstruct);
                }

                m_Conditionals.Push(NewConditional);
            }

            // An #ElseIf can't follow an #Else.

            if (m_Conditionals.Top()->ElseSeen)
            {
                ReportSyntaxErrorForPreviousStatement(ERRID_LbElseifAfterElse, Start, m_CurrentToken, ErrorInConstruct);
            }

            InterpretConditionalCompilationConditional(Statement->AsExpression());

            break;
        }

        case ParseTree::Statement::CCElse:
        {
            // If there has been no preceding #If, give an error and pretend that there
            // had been one.

            if (m_Conditionals.IsEmpty())
            {
                ConditionalCompilationDescriptor NewConditional;
                NewConditional.IntroducingStatement = Statement->AsCCBranch();

                if (m_ParsingMethodBody ||
                    m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                {
                    // The #If might have preceded the start of the method body, so this
                    // isn't an error.

                    // Inhibit compiling any of the code in the #else section. This
                    // minimizes the number of spurious error messages.
                    NewConditional.IntroducingStatement->BranchTaken = true;
#if IDE
                    NewConditional.AnyBranchTaken = true;
#endif
                }
                else
                {
                    ReportSyntaxErrorForPreviousStatement(ERRID_LbElseNoMatchingIf, Start, m_CurrentToken, ErrorInConstruct);
                }

                m_Conditionals.Push(NewConditional);
            }

            ConditionalCompilationDescriptor *ConditionalState = m_Conditionals.Top();

            if (ConditionalState->ElseSeen &&
                // The error will be reported during a declaration parse, so there's no
                // need to report it for a method body parse.
                !m_ParsingMethodBody &&
                !m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
            {
                ReportSyntaxErrorForPreviousStatement(ERRID_LbElseNoMatchingIf, Start, m_CurrentToken, ErrorInConstruct);
            }

            ConditionalState->ElseSeen = true;

#if IDE
            if (m_Conditionals.Top()->AnyBranchTaken)
#else
            if (m_Conditionals.Top()->IntroducingStatement->BranchTaken)
#endif
            {
                SkipConditionalCompilationSection( Statement->AsCCBranch()->SkippedLocation );
            }

            break;
        }

        case ParseTree::Statement::CCEndIf:
        {
            if (m_Conditionals.IsEmpty())
            {
                if (m_ParsingMethodBody ||
                    m_DoLiteParsingOfConditionalCompilation) // Dev10 #789970
                {
                    // The #If might have preceded the start of the method body, so this
                    // isn't an error.
                }
                else
                {
                    ReportSyntaxErrorForPreviousStatement(ERRID_LbNoMatchingIf, Start, m_CurrentToken, ErrorInConstruct);
                }
            }
            else
            {
                ParseTree::CCBranchStatement *IntroducingStatement =
                    m_Conditionals.Pop()->IntroducingStatement;

                if (IntroducingStatement->Opcode == ParseTree::Statement::CCIf)
                {
                    IntroducingStatement->AsCCIf()->TerminatingConstruct = Statement;
                }
            }

            break;
        }

        case ParseTree::Statement::CCConst:
        {
            if (m_ParsingMethodBody ||
                m_DoLiteParsingOfConditionalCompilation || // Dev10 #789970
                !m_ConditionalConstantsScope) // Dev10 #789970
            {
                // Conditional compilation constant declarations are fully processed
                // during the parsing of declarations, and so are ignored during method
                // body parsing.

                break;
            }

            if (Statement->AsNamedValue()->Name.IsBad)
            {
                break;
            }

            STRING *ConstantName = Statement->AsNamedValue()->Name.Name;

            if (ConstantName)
            {
                NorlsAllocator Storage(NORLSLOC);

                bool ExpressionIsBad;

                BCSYM_Hash *LookupTable = NULL;
                LookupTable = m_ConditionalConstantsScope->GetHash();

                ConstantValue Value =
                    Semantics::InterpretConstantExpression(
                        Statement->AsNamedValue()->Value,
                        NULL,
                        LookupTable,
                        NULL,
                        &Storage,
                        m_Errors,
                        true,
                        m_Compiler,
                        m_CompilerHost,
                        &ExpressionIsBad);

                BCSYM_Expression *ExpressionSymbol = NULL;
                ExpressionSymbol = m_ConditionalCompilationSymbolsSource->GetFixedExpression(&Value);

                BCSYM_CCConstant *NewSymbol =
                    m_ConditionalCompilationSymbolsSource->AllocCCConstant();

                Location ConstantLocation;

                ConstantLocation.m_lBegLine = Start->m_StartLine;
                ConstantLocation.m_lBegColumn = Start->m_StartColumn;
                ConstantLocation.m_lEndLine = m_CurrentToken->m_Prev->m_StartLine;
                ConstantLocation.m_lEndColumn = m_CurrentToken->m_Prev->m_StartColumn;

                m_ConditionalCompilationSymbolsSource->GetVariable(
                    &ConstantLocation,
                    ConstantName,
                    ConstantName,
                    DECLF_Const | DECLF_Value | DECLF_Public,
                    VAR_Const,
                    Value.TypeCode == t_ref ?
                        m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType) :
                        m_CompilerHost->GetFXSymbolProvider()->GetType(Value.TypeCode),
                    ExpressionSymbol,
                    NULL,
                    NewSymbol);

                // 

                // Detect duplicate #const declarations.
                BCSYM_CCConstant *ExistingConstant;

                if (ExistingConstant = LookupTable->SimpleBind(ConstantName)->PCCConstant())
                {
                    while (ExistingConstant->GetNextCCConstant())
                    {
                        ExistingConstant = ExistingConstant->GetNextCCConstant();
                    }

                    ExistingConstant->SetNextCCConstant(NewSymbol);
                }
                else
                {
                    Symbols::AddSymbolToHash(LookupTable, NewSymbol, true, false, false);
                }
            }

            break;
            }
        }
#pragma prefast(pop)
}

ParseTree::Expression*
Parser::ParseConditionalCompilationExpression
(
    _Inout_ bool &ErrorInConstruct
)
{
    bool PrevEvaluatingConditionCompilationExpressions = m_EvaluatingConditionCompilationExpression;
    m_EvaluatingConditionCompilationExpression = true;

    ParseTree::Expression *Expr = ParseExpression(ErrorInConstruct);

    m_EvaluatingConditionCompilationExpression = PrevEvaluatingConditionCompilationExpressions;

    return Expr;
}

bool
Parser::StartsValidConditionalCompilationExpr
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        // Identifiers - note that only simple indentifiers are allowed.
        // This check is done in ParseTerm.
        case tkID:

        // Paranthesized expressions
        case tkLParen:

        // Literals
        case tkIntCon:
        case tkCharCon:
        case tkDecCon:
        case tkFltCon:
        case tkDateCon:
        case tkStrCon:
        case tkTRUE:
        case tkFALSE:
        case tkNOTHING:

        // Conversion operators
        case tkCBOOL:
        case tkCDATE:
        case tkCDBL:
        case tkCSBYTE:
        case tkCBYTE:
        case tkCCHAR:
        case tkCSHORT:
        case tkCUSHORT:
        case tkCINT:
        case tkCUINT:
        case tkCLNG:
        case tkCULNG:
        case tkCSNG:
        case tkCSTR:
        case tkCDEC:
        case tkCOBJ:
        case tkCTYPE:
        case tkIF:
        case tkDIRECTCAST:
        case tkTRYCAST:

        // Unary operators
        case tkNOT:
        case tkPlus:
        case tkMinus:

        // Allow "EOL" to start CC expressions to enable better error reporting.
        case tkEOL:
            return true;
    }

    return false;
}

bool
Parser::IsValidOperatorForConditionalCompilationExpr
(
    _In_ Token *T
)
{
    switch (T->m_TokenType)
    {
        case tkNOT:
        case tkAND:
        case tkANDALSO:
        case tkOR:
        case tkORELSE:
        case tkXOR:
        case tkMult:
        case tkPlus:
        case tkMinus:
        case tkDiv:
        case tkIDiv:
        case tkMOD:
        case tkPwr:
        case tkLT:
        case tkLE:
        case tkNE:
        case tkEQ:
        case tkGT:
        case tkGE:
        case tkShiftLeft:
        case tkShiftRight:
        case tkConcat:
            return true;
    }

    return false;
}

void
Parser::MoveRegionChildrenToRegionParent
(
    _Inout_ ParseTree::BlockStatement *EndedRegion,
    _In_opt_ ParseTree::Statement *EndRegionStatement,
    _Out_ ParseTree::StatementList *LastRegionBodyLink
)
{
    // Move the statements within the Region block to be siblings
    // of the Region block. This makes semantic processing of the
    // statements within the Region simpler.

    if (EndedRegion->Opcode != ParseTree::Statement::Region)
    {
        VSFAIL("Unexpected non-region context!!!");
        return;
    }

    if (EndedRegion->Children)
    {
        ParseTree::StatementList *EndRegionLink;
        ParseTree::StatementList *RegionLink;

        if (EndRegionStatement)
        {
            EndRegionLink = EndRegionStatement->ContainingList;
            VSASSERT(
                EndRegionLink && EndRegionLink->Element->Opcode == ParseTree::Statement::EndRegion,
                "Region body absorption is lost.");

            RegionLink = EndRegionLink->PreviousInBlock;

            // If the #End Region was parsed as part of a line parse,
            // then the #Region block is possibly unavailable.

            VSASSERT(
                RegionLink || m_IsLineParse,
                "Region body absorption is lost.");
        }
        else
        {
            RegionLink = EndedRegion->ContainingList;
            EndRegionLink = NULL;
        }

        if (RegionLink)
        {
            RegionLink->NextInBlock = EndedRegion->Children;
            EndedRegion->Children->PreviousInBlock = RegionLink;

            if (EndRegionLink)
            {
                VSASSERT(LastRegionBodyLink, "Last region body statement lost!!!");

                LastRegionBodyLink->NextInBlock = EndRegionLink;
                EndRegionLink->PreviousInBlock = LastRegionBodyLink;
            }

            EndedRegion->Children = NULL;
        }
    }
}

void
Parser::ParseConditionalCompilationLine
(
    bool SkippingMethodBody
)
{

    // Before parsing any new CC statement, decide whether or not we want
    // to terminate a currently existing comment block context.
    EndCommentBlockIfNeeded();

    ParseConditionalCompilationStatement(SkippingMethodBody);
}

// Interpret a conditional compilation #if or #elseif.

void
Parser::InterpretConditionalCompilationConditional
(
    _In_ ParseTree::ExpressionStatement *Directive
)
{
    VSASSERT(
        !m_Conditionals.IsEmpty(),
        "Conditional compilation is lost.");

    ConditionalCompilationDescriptor *ConditionalState = m_Conditionals.Top();

    bool CompileThisSection =
        // Evaluate the expression to detect errors, whether or not
        // its result is needed.
        EvaluateConditionalCompilationExpression(
            Directive->Operand,
            ConditionalState) &&
#if IDE
        !ConditionalState->AnyBranchTaken;
#else
        !ConditionalState->IntroducingStatement->BranchTaken;
#endif

#if IDE
    if (CompileThisSection)
    {
        Directive->AsCCBranch()->BranchTaken = true;
        ConditionalState->AnyBranchTaken = true;
    }
    else
    {
        SkipConditionalCompilationSection(Directive->AsCCBranch()->SkippedLocation);
    }
#else
    if (CompileThisSection)
    {
        ConditionalState->IntroducingStatement->BranchTaken = true;
    }
    else
    {
        SkipConditionalCompilationSection( ConditionalState->IntroducingStatement->SkippedLocation );
    }
#endif
}

// Produce a diagnostic if the parse has not reached the end of a conditional
// compilation statement.

void
Parser::VerifyEndOfConditionalCompilationStatement
(
    _Inout_ bool &ErrorInConstruct
)
{
    ParseTrailingComments();

    if (IsEndOfLine(m_CurrentToken))
    {
        // Guarantee that the current token is EOL or EOF--finding
        // the ends of procedures depends on this.
        if (m_CurrentToken == m_FirstTokenAfterNewline)
        {
            VSASSERT(m_CurrentToken->m_Prev->m_TokenType == tkEOL, "Elusive end-of-line token.");
            m_CurrentToken = m_CurrentToken->m_Prev;
        }
        VSASSERT(
            m_CurrentToken->m_TokenType == tkEOL || m_CurrentToken->m_TokenType == tkEOF,
            "Line ends in an inscrutable fashion.");
    }
    else
    {
        Token *ErrorStart = m_CurrentToken;

        // Syntax error -- a conditional compilation statement is followed by
        // something other than a comment.

        CCResyncAt(0);

        ReportSyntaxErrorForPreviousStatement(ERRID_ExpectedEOS, ErrorStart, m_CurrentToken, ErrorInConstruct);
    }
}


void
Parser::GreedilyParseColonSeparatedStatements
(
    _Inout_ bool &ErrorInConstruct
)
{
    while (m_CurrentToken->m_TokenType == tkColon)
    {
        if (m_LastStatementLinked)
        {
            SetPunctuator(
                &m_LastStatementLinked->Colon,
                m_LastStatementLinked->Element->TextSpan,
                m_CurrentToken);
        }

        GetNextToken();

        ParseStatementInMethodBody(ErrorInConstruct);
    }
}


//-------------------------------------------------------------------------------------------------
//
// Skip all text until the end of the current conditional section.  This will also return the
// span of text that was skipped
// 
//-------------------------------------------------------------------------------------------------
void
Parser::SkipConditionalCompilationSection
(
    _Out_ Location& skippedLocation
)
{
    VSASSERT(
        !m_Conditionals.IsEmpty(),
        "Conditional compilation is lost.");

    ConditionalCompilationDescriptor *ConditionalState = m_Conditionals.Top();
    Token startToken = *m_CurrentToken;

    // If skipping encounters a nested #if, it is necessary to skip all of it through its
    // #end. NestedConditionalsToSkip keeps track of how many nested #if constructs
    // need skipping.
    unsigned NestedConditionalsToSkip = 0;
    
    VSASSERT(m_CurrentToken->m_TokenType == tkEOL, "Should be at the EOL when starting SkipConditionalCompilationSection");

    if (m_CurrentToken->m_TokenType == tkEOL && m_CurrentToken->m_EOL.m_NextLineAlreadyScanned)
    {
        // If the scanner has looked ahead then these tokens must be abandonned because they are in the false region.
        m_InputStream->AbandonTokens(m_CurrentToken->m_Next);
    }
    
    while (true)
    {
        m_InputStream->SkipToNextConditionalLine();

        Token *Next = GetNextLine();
        m_CurrentToken = Next;

        if (m_CurrentToken->m_TokenType == tkSharp)
        {
            Token *SharpToken = m_CurrentToken;
            Token *Next = GetNextToken();

            tokens NextTokenType = Next->m_TokenType;

            if (((NextTokenType == tkEND &&
                  !EqNonReservedToken(Next->m_Next, tkEXTERNALSOURCE) &&
                  !EqNonReservedToken(Next->m_Next, tkREGION)) ||
                 NextTokenType == tkENDIF ||
                 NextTokenType == tkELSEIF ||
                 NextTokenType == tkELSE) &&
                NestedConditionalsToSkip == 0)
            {
                // Finding one of these is sufficient to stop skipping. It is then necessary
                // to process the line as a conditional compilation line. The normal
                // parsing logic will do this.

                m_CurrentToken = SharpToken->m_Prev;

                // Mark the current EOL with next line already scanned to true. Clients will see tkEOL, when they ask for the next token they
                // will also see that they have already been scanned.  The ParseStatementLambda code relies on this behavior.
         
                m_CurrentToken->m_TokenType = tkEOL;
                m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = true;

                // VB IDE uses the span to decide whether to gray the text in a False block,
                // the span IDE expect should include "#End If"
                while( SharpToken->m_TokenType != tkEOL && SharpToken->m_TokenType != tkNone)
                {
                    SharpToken = SharpToken->m_Next;
                    ThrowIfNull(SharpToken);
                }

                // Record the span of text that we skipped
                SetLocation( &skippedLocation, &startToken, SharpToken);

                break;
            }
            else if (NextTokenType == tkENDIF ||
                     (NextTokenType == tkEND &&
                      !EqNonReservedToken(Next->m_Next, tkEXTERNALSOURCE) &&
                      !EqNonReservedToken(Next->m_Next, tkREGION)))
            {
                NestedConditionalsToSkip--;
            }
            else if (NextTokenType == tkIF)
            {
                NestedConditionalsToSkip++;
            }
        }
        else if (m_CurrentToken->m_TokenType != tkDateCon) // Dev10 #777522 Do not confuse Date literal with an end of conditional block.
        {
            VSASSERT(
                m_CurrentToken->m_TokenType == tkEOF,
                "Conditional compilation skips to a conditional line or EOF.");

            // Record the span of text that we skipped
            SetLocation( &skippedLocation, &startToken, m_CurrentToken );

            break;
        }
    }

}


// Evaluate an expression that appears in a conditional compilation directive.

bool
Parser::EvaluateConditionalCompilationExpression
(
    ParseTree::Expression *Expr,
    _Inout_ ConditionalCompilationDescriptor *ConditionalState
)
{
    bool Result = false;

    if (ConditionalState->IsBad || m_ConditionalConstantsScope == NULL || m_ConditionalConstantsScope->GetCompilerFile() == NULL)
    {
        Result = false;
    }
    else
    {
        NorlsAllocator Storage(NORLSLOC);

        bool ExpressionIsBad;

        BCSYM_Hash *LookupTable = m_ConditionalConstantsScope->GetHash();

        ConstantValue Value =
            Semantics::InterpretConstantExpression(
                Expr,
                NULL,
                LookupTable,
                m_CompilerHost->GetFXSymbolProvider()->GetType(t_bool),
                &Storage,
                m_Errors,
                true,
                m_Compiler,
                m_CompilerHost,
                &ExpressionIsBad);

        if (ExpressionIsBad)
        {
            ConditionalState->IsBad = true;
            Result = false;
        }
        else
        {
            switch (Value.TypeCode)
            {
                case t_bool:
                case t_i1:
                case t_ui1:
                case t_i2:
                case t_ui2:
                case t_i4:
                case t_ui4:
                case t_i8:
                case t_ui8:

                    Result = Value.Integral != 0;
                    break;
            }
        }
    }

    return Result;
}

// If compilation ends in the middle of a non-skipped conditional section,
// produce appropriate diagnostics.

void
Parser::RecoverFromMissingConditionalEnds
(
)
{
    while (!m_Conditionals.IsEmpty())
    {
        ConditionalCompilationDescriptor *ConditionalState = m_Conditionals.Pop();

        // There's no need to report an error if the sequence is missing the
        // beginning #if--the error will appear extraneous.

        if (ConditionalState->IntroducingStatement->Opcode == ParseTree::Statement::CCIf)
        {
            bool ErrorInConstruct = false;

            ReportSyntaxError(
                ERRID_LbExpectedEndIf,
                &(ConditionalState->IntroducingStatement->TextSpan),
                ErrorInConstruct);
        }
    }
}

Parser::ConditionalCompilationDescriptor::ConditionalCompilationDescriptor
(
) :
#if IDE
    AnyBranchTaken(false),
#endif
    ElseSeen(false),
    IsBad(false),
    IntroducingStatement(NULL)
{
}

void
Parser::ConditionalCompilationStack::Init
(
    NorlsAllocator *ElementStorage
)
{
    Elements = InitialElements;
    MaxElements = sizeof(InitialElements) / sizeof(ConditionalCompilationDescriptor); 
    Depth = 0;
    ElementOverflowStorage = ElementStorage;
}

Parser::ConditionalCompilationDescriptor *
Parser::ConditionalCompilationStack::Top
(
)
{
    VSASSERT(Depth > 0, "An empty stack has no top.");

    return &Elements[Depth - 1];
}

Parser::ConditionalCompilationDescriptor *
Parser::ConditionalCompilationStack::Pop
(
)
{
    VSASSERT(Depth > 0, "Can't pop an empty stack.");

    return &Elements[--Depth];
}

void
Parser::ConditionalCompilationStack::Push
(
    ConditionalCompilationDescriptor NewTop
)
{
    if (Depth == MaxElements)
    {
        const unsigned IncrementSize = 10;

        ConditionalCompilationDescriptor *NewElements =
            new(*ElementOverflowStorage) ConditionalCompilationDescriptor[MaxElements + IncrementSize];

        for (unsigned i = 0; i < Depth; i++)
        {
            NewElements[i] = Elements[i];
        }

        Elements = NewElements;
        MaxElements += IncrementSize;
    }

    Elements[Depth++] = NewTop;
}

bool
Parser::ConditionalCompilationStack::IsEmpty
(
)
{
    return Depth == 0;
}

/***************************************************************************************** 
;ShallowCopyInto

Make a SHALLOW copy of the current ConditionalCompilationStack into
a ConditionalCompilationStack that you provide.

Note that you should provide an empty newCloneStack or your end
result won't look like this ConditionalCompilationStack
******************************************************************************************/
void
Parser::ConditionalCompilationStack::ShallowCopyInto
(
    _In_ ConditionalCompilationStack *newCloneStack  // becomes a clone of 'this' ConditionalCompilationStack
)
{
    VSASSERT( newCloneStack != NULL, "newCloneStack can't be NULL" );
    VSASSERT( newCloneStack->IsEmpty(), "newCloneStack must be empty" );
    
    for ( unsigned i=0; i < Depth; i++ )
    {
        // These are structs so a byval copy is happening, except for the StatementField,
        // which is being copied shallow
        newCloneStack->Push( Elements[ i ] ); 
    }   
}



//
//============ Miscellaneous methods. ===================================
//

// Produce a diagnostic if the parse has not reached the end of a statement.
// If this is the last statement on the line, the return value is true; otherwise,
// the return value is false and the input stream is left positioned at the start
// of the next statement.

bool
Parser::VerifyEndOfStatement
(
    _Inout_ bool &ErrorInConstruct,
    bool ignoreColon
)
{
    ParseTrailingComments();
    
    if (m_LastStatementLinked)
    {
        // If we are on End Sub/Function, bail if we have finished off the Lambda we are currently working on
        if ( ((m_LastStatementLinked->Element->Opcode == ParseTree::Statement::EndSub) ||
                (m_LastStatementLinked->Element->Opcode == ParseTree::Statement::EndFunction)) &&
              m_LastStatementLinked->PreviousInBlock &&
              (m_LastStatementLinked->PreviousInBlock->Element->Opcode == ParseTree::Statement::LambdaBody) &&
              (m_LastStatementLinked->PreviousInBlock->Element == m_pStatementLambdaBody))
        {
            // We are at the end construct of a lambda body, this is the end of its statement context. We want to return to an expression
            // context so we state that this is the end of the statement.
            return true;
        }
    }
             
    Token *Next = m_CurrentToken;

    if (IsEndOfLine(Next))
    {
        VSASSERT(
            m_CurrentToken->m_TokenType == tkEOL || m_CurrentToken->m_TokenType == tkEOF,
            "Line ends in an inscrutable fashion.");
        return true;
    }
    else if (Next->m_TokenType == tkColon && !ignoreColon)
    {
        // Set the punctuator location for the colon in the statement list
        // element last created, unless it is an End Sub for a multiline lambda, then
        // set the punctuator location in the statement list of the element that OWNS
        // the multiline lambda.

        ParseTree::StatementList *pStatementList = NULL;

        // If the last statement linked was an END SUB and it belongs to a hidden block, then
        // we are processing the End Sub of a multi-line lambda.  The statement that owns
        // the lambda is in m_LastInContexts[m_ContextIndex] 
        if (m_LastStatementLinked &&
            m_LastStatementLinked->Element &&
            (m_LastStatementLinked->Element->Opcode == ParseTree::Statement::EndSub) &&
            m_LastStatementLinked->Element->GetRawParent() &&
            (m_LastStatementLinked->Element->GetRawParent()->Opcode == ParseTree::Statement::HiddenBlock)) // assumes only multi-line lambdas have hidden blocks
        {
            VSASSERT( m_LastStatementLinked->PreviousInBlock &&
                      m_LastStatementLinked->PreviousInBlock->Element &&
                      (m_LastStatementLinked->PreviousInBlock->Element->Opcode == ParseTree::Statement::LambdaBody),
                      "Expect the PreviousInBlock statement in the statement list that contains the End Sub of a multiline lambda to be the multiline lambda itself.");

            /* dev10_648726 - The statement that owns the lambda is the last context active as the Lambda was created
               We can't rely on m_LastStatementLinked->PreviousInBlock->Element->AsLambdaBody()->pOwningStatement being set (the previous code relied on this)
               as it won't be set yet for expressions like:
                   Dim r = Sub(rr)
                      Dim s = Sub(ss)
                      End Sub:dim x = 1 <-- when we hit the ':' in this case we haven't processed the m_LambdaBodyStatements array in LinkStatement() yet
                      to set the owner for Sub(ss)
                   End Sub */

            pStatementList = m_LastInContexts[m_ContextIndex]; 
        }
        else
        {
            pStatementList = m_LastStatementLinked;
        }

        if (pStatementList)
        {
            SetPunctuator(
                &pStatementList->Colon,
                pStatementList->Element->TextSpan,
                Next);
        }

        // Eat the colon and go on to the next statement.
        GetNextToken();

        return false;
    }        
    else
    {
        // Syntax error -- a valid statement is followed by something other than
        // a colon, end-of-line, or a comment.

        Token *Start = m_CurrentToken;
        ResyncAt(0);

        ReportSyntaxErrorForPreviousStatement(ERRID_ExpectedEOS, Start, m_CurrentToken, ErrorInConstruct);

        return VerifyEndOfStatement(ErrorInConstruct);
    }
}

// Intellisense requires a Loop statement with an expression, even if it is
// outside a proper context.

void
Parser::ParseMismatchedLoopClause
(
    _In_ Token *Start,
    _Inout_ bool &ErrorInConstruct
)
{
    Token *WhileOrUntil = m_CurrentToken;
    bool IsWhile = false;

    ParseTree::Expression *Expr = ParseOptionalWhileUntilClause(&IsWhile, ErrorInConstruct);

    ParseTree::Statement *Loop =
        EndContext(
            Start,
            Expr ?
                (IsWhile ?
                    ParseTree::Statement::EndLoopWhile :
                    ParseTree::Statement::EndLoopUntil) :
                ParseTree::Statement::EndLoop,
            Expr);

    if (Expr)
    {
        SetPunctuator(&Loop->AsBottomTestLoop()->WhileOrUntil, Start, WhileOrUntil);
    }
}

// For an end construct that does match the current context, produce a diagnostic
// and recover.

void
Parser::RecoverFromMismatchedEnd
(
    _In_ Token *Start,
    ParseTree::Statement::Opcodes EndOpcode,
    _Inout_ bool &ErrorInConstruct
)
{
    // Check to see if this end ends an enclosing context. If it does, assume that the
    // ends for intervening contexts are missing.

    
    bool lambdaEndBlockMissing = false;
    for (ParseTree::BlockStatement *Context = m_Context; Context; Context = Context->GetParent())
    {
        bool EndsEnclosing = false;
        switch (EndOpcode)
        {
            case ParseTree::Statement::EndIf:
                EndsEnclosing =
                    Context->Opcode == ParseTree::Statement::BlockIf ||
                    Context->Opcode == ParseTree::Statement::ElseIf ||
                    Context->Opcode == ParseTree::Statement::BlockElse;
                break;
            case ParseTree::Statement::EndWith:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::With;
                break;
            case ParseTree::Statement::EndSelect:
                EndsEnclosing =
                    Context->Opcode == ParseTree::Statement::Case ||
                    Context->Opcode == ParseTree::Statement::CaseElse ||
                    Context->Opcode == ParseTree::Statement::Select;
                break;
            case ParseTree::Statement::EndWhile:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::While;
                break;
            case ParseTree::Statement::EndLoop:
                EndsEnclosing =
                    Context->Opcode >= ParseTree::Statement::FirstDoLoop &&
                    Context->Opcode <= ParseTree::Statement::LastDoLoop;
                break;
            case ParseTree::Statement::EndNext:
                EndsEnclosing =
                    Context->Opcode == ParseTree::Statement::ForFromTo ||
                    Context->Opcode == ParseTree::Statement::ForEachIn;
                break;
            case ParseTree::Statement::EndStructure:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Structure;
                break;
            case ParseTree::Statement::EndEnum:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Enum;
                break;
            case ParseTree::Statement::EndProperty:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Property;
                break;
            case ParseTree::Statement::EndEvent:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::BlockEventDeclaration;
                break;
            case ParseTree::Statement::EndInterface:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Interface;
                break;
            case ParseTree::Statement::EndTry:
                EndsEnclosing =
                    Context->Opcode == ParseTree::Statement::Try ||
                    Context->Opcode == ParseTree::Statement::Catch ||
                    Context->Opcode == ParseTree::Statement::Finally;
                break;
            case ParseTree::Statement::EndClass:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Class;
                break;
            case ParseTree::Statement::EndModule:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Module;
                break;
            case ParseTree::Statement::EndNamespace:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Namespace;
                break;
            case ParseTree::Statement::EndUsing:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Using;
                break;
            case ParseTree::Statement::EndSyncLock:
                EndsEnclosing = Context->Opcode == ParseTree::Statement::SyncLock;
                break;
            case ParseTree::Statement::EndRegion :
                EndsEnclosing = Context->Opcode == ParseTree::Statement::Region;
                break;
            case ParseTree::Statement::EndSub:
            case ParseTree::Statement::EndFunction:
                EndsEnclosing = (Context->Opcode == ParseTree::Statement::LambdaBody && !IsParsingSingleLineLambda());
                break;
            case ParseTree::Statement::EndOperator:
            case ParseTree::Statement::EndGet:
            case ParseTree::Statement::EndSet:
            case ParseTree::Statement::EndAddHandler:
            case ParseTree::Statement::EndRemoveHandler:
            case ParseTree::Statement::EndRaiseEvent:
            case ParseTree::Statement::SyntaxError:
            case ParseTree::Statement::EndInvalid:
            case ParseTree::Statement::EndUnknown:
                EndsEnclosing = false;
                break;
            default:
                VSFAIL("Unexpected syntactic context.");
        }

        if (m_pStatementLambdaBody && 
            m_pStatementLambdaBody == Context)
        {
            /* EndsEnclosing may be set not because we found the matching End Sub/Function for the lambda, but because
               we climbed the contexts and found a match for a different enclosing block, e.g. 
                    if true then
                         foo( Sub()
                            using obj as            // Dev11 131021
                         )
                    End If  <-- this is what set EndsEnclosing = true because it matched an IF higher up.
                It doesn't have anything to do with a match for the lambda. */
            lambdaEndBlockMissing = true;
        }

        if (EndsEnclosing)
        {
            if (lambdaEndBlockMissing)
            {
                RecoverFromMissingEnds(m_pStatementLambdaBody);

                // Tag the beginning construct with the error about its lack of termination.
                bool ErrorInBlockEnd = false;
                ReportSyntaxError(
                    (m_pStatementLambdaBody->pOwningLambdaExpression->MethodFlags & DECLF_Function) ?
                        ERRID_MultilineLambdaMissingFunction : 
                        ERRID_MultilineLambdaMissingSub, 
                    &m_pStatementLambdaBody->TextSpan, ErrorInBlockEnd);

                // Dev10-486789 - Lambdas don't anticipate ':' Which is an error, but we still need to anticipate that it could be here instead of EOL
                VSASSERT( Start->m_Prev->m_TokenType == tkEOL || Start->m_Prev->m_TokenType == tkColon, "Expected this to be the first token on the line.");
                if ( Start->m_Prev->m_TokenType == tkEOL )
                {
                    m_CurrentToken = Start->m_Prev; // make the current token the EOL
                    m_CurrentToken->m_EOL.m_NextLineAlreadyScanned = true; // and mark it so we know that the next line has been scanned so we don't get out of sync with the scanner
                }

                VSASSERT (m_pStatementLambdaBody->GetRawParent()->Opcode == ParseTree::Statement::HiddenBlock,
                          "Expected this context's parent to be the hidden block for the lambda.");

                PopContext(false, m_CurrentToken);
  
                return;
            }
             
            // The end construct does end an enclosing context. Report errors for the
            // (unterminated) intervening contexts, then patch up the properly ended
            // context so that it appears normally terminated.

            RecoverFromMissingEnds(Context);

            if (EndOpcode == ParseTree::Statement::EndLoop)
            {
                ParseMismatchedLoopClause(Start, ErrorInConstruct);
                return;
            }

            if (m_ContextIndex == 0)
            {
                // There was an attempt to pop past the context
                // supplied to a line parse. Punt.

                return;
            }

            VSASSERT(Context == m_Context, "End recovery lost.");

            if (IsTwoLevelContext(m_Context->Opcode))
            {
                PopContext(true, Start);
            }

            EndContext(Start, EndOpcode, NULL);
            return;
        }
    }


    // The end construct is extraneous. Report an error and leave
    // the current context alone.

    unsigned ErrorId = ERRID_Syntax;

    if (m_Context->Opcode == ParseTree::Statement::Interface &&
        (EndOpcode == ParseTree::Statement::EndSub ||
         EndOpcode == ParseTree::Statement::EndFunction ||
         EndOpcode == ParseTree::Statement::EndOperator ||
         EndOpcode == ParseTree::Statement::EndProperty ||
         EndOpcode == ParseTree::Statement::EndGet ||
         EndOpcode == ParseTree::Statement::EndSet ||
         EndOpcode == ParseTree::Statement::EndEvent ||
         EndOpcode == ParseTree::Statement::EndAddHandler ||
         EndOpcode == ParseTree::Statement::EndRemoveHandler ||
         EndOpcode == ParseTree::Statement::EndRaiseEvent))
    {
        ErrorId = ERRID_InvInsideInterface;
    }
    else
    {
        switch (EndOpcode)
        {
            case ParseTree::Statement::EndIf:
                ErrorId = ERRID_EndIfNoMatchingIf;
                break;
            case ParseTree::Statement::EndWith:
                ErrorId = ERRID_EndWithWithoutWith;
                break;
            case ParseTree::Statement::EndSelect:
                ErrorId = ERRID_EndSelectNoSelect;
                break;
            case ParseTree::Statement::EndWhile:
                ErrorId = ERRID_EndWhileNoWhile;
                break;
            case ParseTree::Statement::EndLoop:
                ErrorId = ERRID_LoopNoMatchingDo;
                break;
            case ParseTree::Statement::EndNext:
                ErrorId = ERRID_NextNoMatchingFor;
                break;
            case ParseTree::Statement::EndSub:
                ErrorId = ERRID_InvalidEndSub;
                break;
            case ParseTree::Statement::EndFunction:
                ErrorId = ERRID_InvalidEndFunction;
                break;
            case ParseTree::Statement::EndOperator:
                ErrorId = ERRID_InvalidEndOperator;
                break;
            case ParseTree::Statement::EndProperty:
                ErrorId = ERRID_InvalidEndProperty;
                break;
            case ParseTree::Statement::EndGet:
                ErrorId = ERRID_InvalidEndGet;
                break;
            case ParseTree::Statement::EndSet:
                ErrorId = ERRID_InvalidEndSet;
                break;
            case ParseTree::Statement::EndEvent:
                ErrorId = ERRID_InvalidEndEvent;
                break;
            case ParseTree::Statement::EndAddHandler:
                ErrorId = ERRID_InvalidEndAddHandler;
                break;
            case ParseTree::Statement::EndRemoveHandler:
                ErrorId = ERRID_InvalidEndRemoveHandler;
                break;
            case ParseTree::Statement::EndRaiseEvent:
                ErrorId = ERRID_InvalidEndRaiseEvent;
                break;
            case ParseTree::Statement::EndStructure:
                ErrorId = ERRID_EndStructureNoStructure;
                break;
            case ParseTree::Statement::EndEnum:
                ErrorId = ERRID_InvalidEndEnum;
                break;
            case ParseTree::Statement::EndInterface:
                ErrorId = ERRID_InvalidEndInterface;
                break;
            case ParseTree::Statement::EndTry:
                ErrorId = ERRID_EndTryNoTry;
                break;
            case ParseTree::Statement::EndClass:
                ErrorId = ERRID_EndClassNoClass;
                break;
            case ParseTree::Statement::EndModule:
                ErrorId = ERRID_EndModuleNoModule;
                break;
            case ParseTree::Statement::EndNamespace:
                ErrorId = ERRID_EndNamespaceNoNamespace;
                break;
             case ParseTree::Statement::EndUsing:
                ErrorId = ERRID_EndUsingWithoutUsing;
                break;
           case ParseTree::Statement::EndSyncLock:
                ErrorId = ERRID_EndSyncLockNoSyncLock;
                break;
            case ParseTree::Statement::EndRegion:
                ErrorId = ERRID_EndRegionNoRegion;
                break;
            case ParseTree::Statement::SyntaxError:
            case ParseTree::Statement::EndUnknown:
            case ParseTree::Statement::EndInvalid:
                ErrorId = ERRID_UnrecognizedEnd;
                break;
            default:
                VSFAIL("Unexpected");
        }
    }

    ReportSyntaxError(ErrorId, Start, m_CurrentToken, ErrorInConstruct);

    // It is necessary to create the end construct, even though it does not
    // end anything. A statement is necessary to represent the input text, and as
    // a place to hang comments.
    //
    // If the statement contains an expression, Intellisense needs access to it.

    if (EndOpcode == ParseTree::Statement::EndLoop)
    {
        ParseMismatchedLoopClause(Start, ErrorInConstruct);
    }
    else
    {
        CreateEndConstruct(Start, EndOpcode);
    }
}

// Verify that compilation has not left contexts unterminated. Produce diagnostics
// and recover as appropriate. Pop contexts until ContextToMatch is current.

void
Parser::RecoverFromMissingEnds
(
    ParseTree::BlockStatement *ContextToMatch
)
{
    bool showHiddenBlockErrors = true;
    while (m_Context != ContextToMatch && m_ContextIndex > 0)
    {
        // A context does not have a matching end.

        unsigned ErrorId;

        switch(m_Context->Opcode)
        {
            case ParseTree::Statement::Structure:
                ErrorId = ERRID_ExpectedEndStructure;
                break;
            case ParseTree::Statement::Enum:
                ErrorId = ERRID_MissingEndEnum;
                break;
            case ParseTree::Statement::Property:
                ErrorId = ERRID_EndProp;
                break;
            case ParseTree::Statement::BlockEventDeclaration:
                ErrorId = ERRID_MissingEndEvent;
                break;
            case ParseTree::Statement::Interface:
                ErrorId = ERRID_MissingEndInterface;
                break;
            case ParseTree::Statement::ProcedureBody:
            case ParseTree::Statement::LambdaBody:
                ErrorId = ERRID_EndSubExpected;
                break;
            case ParseTree::Statement::PropertyGetBody:
                ErrorId = ERRID_MissingEndGet;
                break;
            case ParseTree::Statement::PropertySetBody:
                ErrorId = ERRID_MissingEndSet;
                break;
            case ParseTree::Statement::FunctionBody:
                ErrorId = ERRID_EndFunctionExpected;
                break;
            case ParseTree::Statement::OperatorBody:
                ErrorId = ERRID_EndOperatorExpected;
                break;
            case ParseTree::Statement::AddHandlerBody:
                ErrorId = ERRID_MissingEndAddHandler;
                break;
            case ParseTree::Statement::RemoveHandlerBody:
                ErrorId = ERRID_MissingEndRemoveHandler;
                break;
            case ParseTree::Statement::RaiseEventBody:
                ErrorId = ERRID_MissingEndRaiseEvent;
                break;
            case ParseTree::Statement::BlockIf:
            case ParseTree::Statement::ElseIf:
            case ParseTree::Statement::BlockElse:
                ErrorId = ERRID_ExpectedEndIf;
                break;
            case ParseTree::Statement::ForFromTo:
            case ParseTree::Statement::ForEachIn:
                ErrorId = ERRID_ExpectedNext;
                break;
            case ParseTree::Statement::With:
                ErrorId = ERRID_ExpectedEndWith;
                break;
            case ParseTree::Statement::Select:
            case ParseTree::Statement::Case:
            case ParseTree::Statement::CaseElse:
                ErrorId = ERRID_ExpectedEndSelect;
                break;
            case ParseTree::Statement::Try:
                ErrorId = ERRID_ExpectedEndTry;
                break;
            case ParseTree::Statement::Catch:
                ErrorId = ERRID_ExpectedEndTryCatch;
                break;
            case ParseTree::Statement::Finally:
                ErrorId = ERRID_ExpectedEndTryFinally;
                break;
            case ParseTree::Statement::While:
                ErrorId = ERRID_ExpectedEndWhile;
                break;
            case ParseTree::Statement::DoWhileTopTest:
            case ParseTree::Statement::DoUntilTopTest:
            case ParseTree::Statement::DoWhileBottomTest:
            case ParseTree::Statement::DoUntilBottomTest:
            case ParseTree::Statement::DoForever:
                ErrorId = ERRID_ExpectedLoop;
                break;
            case ParseTree::Statement::Class:
                ErrorId = ERRID_ExpectedEndClass;
                break;
            case ParseTree::Statement::Module:
                ErrorId = ERRID_ExpectedEndModule;
                break;
            case ParseTree::Statement::Namespace:
                ErrorId = ERRID_ExpectedEndNamespace;
                break;
            case ParseTree::Statement::Using:
                ErrorId = ERRID_ExpectedEndUsing;
                break;
            case ParseTree::Statement::SyncLock:
                ErrorId = ERRID_ExpectedEndSyncLock;
                break;
            case ParseTree::Statement::Region:
                MoveRegionChildrenToRegionParent(m_Context, NULL, NULL);

                ErrorId = ERRID_ExpectedEndRegion;
                break;

            default:
                ErrorId = ERRID_Syntax;
                break;
        }

        if (m_Context->Opcode == ParseTree::Statement::CommentBlock)
        {
            PopBlockCommentContext(m_CurrentToken);
        }
        else
        {
            // Force the generation of the error by using a localized
            // ErrorInConstruct flag.
            bool ErrorInBlockEnd = false;

            if (m_Context->Opcode == ParseTree::Statement::LineIf ||
                m_Context->Opcode == ParseTree::Statement::LineElse)
            {
                // Tag the current construct with its inappropriateness
                // within a line if.
                ReportSyntaxError(ERRID_BogusWithinLineIf, m_CurrentToken, ErrorInBlockEnd);
            }
            else if (m_Context->Opcode != ParseTree::Statement::HiddenBlock || showHiddenBlockErrors)
            {
                // Tag the beginning construct with the error about its
                // lack of termination.
                ReportSyntaxError(ErrorId, &m_Context->TextSpan, ErrorInBlockEnd);
            }

            // Now that we've reported an error, let's not report errors for hidden blocks.
            // INVARIANT: By the time we get here, at least one error is guaranteed to have
            // been reported, and thus suppressing further errors will here never cause us to
            // drop to 0 reported errors.
            showHiddenBlockErrors = false;

            if (IsTwoLevelContext(m_Context->Opcode))
            {
                PopContext(false, m_CurrentToken);
            }

            PopContext(false, m_CurrentToken);
        }
    }
}


// Gives us access to the /langVersion switch constants defined in langversion.cpp
extern struct LanguageFeatureMap g_LanguageFeatureMap[]; // The list of features and version they were introduced for use with /langVersion 
extern int g_CountOfPreviousLanguageVersions; // the number of elements in g_LanguageFeatureMap
extern WCHAR *g_LanguageVersionStrings[]; // strings for previous versions of vb; used in error reporting to indicate when a feature was introduced 


/*****************************************************************************************
;AssertLanguageFeature

Given a FEATUREID_*, determines if that feature was introduced after the version that is
currently being targeted via the /LangVersion switch.  If it was introduced after the 
version being targeted, an error is raised to that effect.
******************************************************************************************/
void Parser::AssertLanguageFeature(
    unsigned feature, // the feature we are testing for, e.g. FEATUREID_StatementLambdas
    _Inout_ Token* ErrorLocationToken // if we end up logging an error, this token will be used for the error location
)
{
    // If we are targeting the latest version of the compiler, all features are fair game
    if ( m_CompilingLanguageVersion == LANGUAGE_CURRENT )
    {
        return;
    }

    // If a feature was introduced after the version of the compiler we are targeting
    if ( m_CompilingLanguageVersion < g_LanguageFeatureMap[ FEATUREID_TO_INDEX(feature) ].m_Introduced )
    {
        const WCHAR* wszVersion = NULL;
        if ( m_CompilingLanguageVersion < g_CountOfPreviousLanguageVersions )
        {
            wszVersion = g_LanguageVersionStrings[ m_CompilingLanguageVersion ];
        }
        else
        {
            wszVersion = L"???";
        }

        AssertIfNull( wszVersion );
        ReportSyntaxErrorForLanguageFeature(ERRID_LanguageVersion, ErrorLocationToken, feature, wszVersion );
    }
}


#if IDE 
void Parser::FixAttributeArgumentValueOffsets(
    ParseTree::ArgumentList *pArguments,
    unsigned long iAttributeStartOffset)
{
    while (pArguments)
    {
        if (pArguments->Element->Value)
        {
            pArguments->Element->ValueStartPosition -= iAttributeStartOffset;
        }

        pArguments = pArguments->Next;
    }
}
#endif

ParseTree::VariableDeclarationIter::VariableDeclarationIter(_In_ ParseTree::VariableDeclarationStatement * pVariableDeclarationStatement)
{
    ThrowIfNull(pVariableDeclarationStatement);
    m_pCurrentVariableDeclarationStatement = pVariableDeclarationStatement;

    m_pNextDeclarationList = m_pCurrentVariableDeclarationStatement->Declarations;
}

ParseTree::VariableDeclaration*
ParseTree::VariableDeclarationIter::GetNext()
{
    ParseTree::VariableDeclarationList *pReturnValue = m_pNextDeclarationList;

    m_pNextDeclarationList = m_pNextDeclarationList ? m_pNextDeclarationList->Next : NULL;

    return pReturnValue ? pReturnValue->Element : NULL;
}

ParseTree::VariableDeclarationStatementStatementLambdaBodyInitializationIter::VariableDeclarationStatementStatementLambdaBodyInitializationIter
(
    _In_ ParseTree::VariableDeclarationStatement * pVariabledDeclarationStatement,
    _In_opt_ ParseTree::ArgumentList *pArgList,
    _In_opt_ ParseTree::InitializerList *pBracedList
)
:m_pVariableDeclarationIter(pVariabledDeclarationStatement),
 m_NewArgumentIter(pArgList),
 m_NewBracedInitializerIter(pBracedList)
{
}

ParseTree::LambdaBodyStatement*
ParseTree::VariableDeclarationStatementStatementLambdaBodyInitializationIter::GetNext()
{
    return GetNext(NULL);
}

ParseTree::LambdaBodyStatement*
ParseTree::VariableDeclarationStatementStatementLambdaBodyInitializationIter::GetNext(_In_opt_ Location *pLocation)
{
    ParseTree::LambdaBodyStatement *pLambdaBodyFromNewVariableDeclaration = NULL;
    
    if (pLambdaBodyFromNewVariableDeclaration = m_NewArgumentIter.GetNext())
    {
        return pLambdaBodyFromNewVariableDeclaration;
    }

    if (pLambdaBodyFromNewVariableDeclaration = m_NewBracedInitializerIter.GetNext())
    {
        return pLambdaBodyFromNewVariableDeclaration;
    }   
    
    ParseTree::VariableDeclaration *pDeclaration = NULL;

    while (pDeclaration = m_pVariableDeclarationIter.GetNext())
    {        
        if (pDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer)
        {
            ParseTree::Expression *pExpr = ParserHelper::DigToInitializerValue(pDeclaration->AsInitializer()->InitialValue);

            ParseTree::Expression *pResult = ParseTreeSearcher::FindFirstExpressionWithArgument(pExpr, pLocation, ParseTree::LambdaExpression::GetIsStatementLambda);

            if (pResult)
            {
                return pResult->AsLambda()->GetStatementLambdaBody();
            }
            
        }
        else if (pDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew)
        {
            if (pDeclaration->AsNew()->Arguments.Values)
            {
                m_NewArgumentIter.SetList(pDeclaration->AsNew()->Arguments.Values);
            }
            else if (pDeclaration->AsNew()->ObjectInitializer &&
                pDeclaration->AsNew()->ObjectInitializer->BracedInitializerList &&
                pDeclaration->AsNew()->ObjectInitializer->BracedInitializerList->InitialValues)
            {
                m_NewBracedInitializerIter.SetList(pDeclaration->AsNew()->ObjectInitializer->BracedInitializerList->InitialValues);
            }
            else if (pDeclaration->AsNew()->CollectionInitializer &&
                pDeclaration->AsNew()->CollectionInitializer->InitialValues)
            {
                m_NewBracedInitializerIter.SetList(pDeclaration->AsNew()->CollectionInitializer->InitialValues);
            }

            if (pLambdaBodyFromNewVariableDeclaration = m_NewArgumentIter.GetNext())
            {
                return pLambdaBodyFromNewVariableDeclaration;
            }

            if (pLambdaBodyFromNewVariableDeclaration = m_NewBracedInitializerIter.GetNext())
            {
                return pLambdaBodyFromNewVariableDeclaration;
            }       
        }
    }

    return NULL;
}

template <class ElementType, class ListType>
ParseTree::ListIter<ElementType, ListType>::ListIter( _In_ List<ElementType, ListType> *pList)
{
    m_pNextListElement = pList;
}

template <class ElementType, class ListType> 
ElementType *ParseTree::ListIter<ElementType, ListType>::GetNext()
{
    List<ElementType, ListType> *pReturnValue = m_pNextListElement;

    m_pNextListElement = m_pNextListElement ? m_pNextListElement->Next : NULL;

    return pReturnValue ? pReturnValue->Element : NULL;        
}
       
template<class ElementType, class ListType>
ParseTree::LambdaBodyListIter<ElementType, ListType>::LambdaBodyListIter( _In_ ParseTree::List<ElementType, ListType> *pList)
:m_ListIter(pList)
{
}

template<class ElementType, class ListType>
ParseTree::LambdaBodyStatement *
ParseTree::LambdaBodyListIter<ElementType, ListType>::GetNext()
{
    return GetNext(NULL);
}

template<class ElementType, class ListType>
ParseTree::LambdaBodyStatement *
ParseTree::LambdaBodyListIter<ElementType, ListType>::GetNext(_In_opt_ Location *pLocation)
{
    ElementType *pElem = NULL;

    while (pElem = m_ListIter.GetNext())
    {
        ParseTree::Expression *pResult = ParseTreeSearcher::FindFirstExpressionWithArgument(pElem, pLocation, ParseTree::LambdaExpression::GetIsStatementLambda);

        if (pResult)
        {
            return pResult->AsLambda()->GetStatementLambdaBody();
        }
    }
    return NULL;
}

 Location *ParseTree::Initializer::GetTextSpan()
 {
     ParseTree::Initializer *pInit = this;
     while (pInit != NULL)
     {
         switch (pInit->Opcode)
         {
             case ParseTree::Initializer::Deferred:
                 pInit = pInit->AsDeferred()->Value;
                 break;
 
             case ParseTree::Initializer::Assignment:
                 return &pInit->AsAssignment()->TextSpan;
           
             case ParseTree::Initializer::Expression:
                 return &pInit->AsExpression()->Value->TextSpan;
                 
             default:
                return NULL;
         }
     }
     
     return NULL;
}



