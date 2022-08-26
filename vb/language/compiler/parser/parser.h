//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB compiler parser.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class Scanner;
struct Token;
class Compiler;
enum LexicalState;

//-------------------------------------------------------------------------------------------------
//
// The Parser class implements all VB source code parsing. There are several contexts
// in which parsing occurs:
//
//    -- In the first visit to a source file, the compiler parses all module-level
//       declarations, and skips method bodies. (ParseDecls implements this for an
//       entire module.)
//
//    -- In the second visit to a source file, the compiler parses method bodies.
//       (ParseMethodBody implements this for an individual method.)
//
//    -- At project startup, the compiler parses the conditional compilation constant
//       declarations in the project file. (ParseProjectLevelCondCompDecls implements
//       this.)
//
//    -- During the semantic processing of a constant declaration, the compiler parses
//       the text for the expression giving the constant's value. (ParseOneExpression
//       implements this.)
//
//    -- During compilation of an expression for the hosted compiler, the compiler
//       parses the input expression text and needs to ensures that after the
//       expression has been parsed, besides newline characters there is no other
//       text. (ParseOneExpressionForHostedCompiler implements this)
//
//    -- IntelliSense requires the parsing of an individual line within a specific
//       syntactic context. (ParseOneLine implements this.)
//
class Parser : ZeroInit<Parser>
{
    friend class CVBDbgee_Parser;
    friend class SaveErrorStateGuard;
    friend class SetupConditionalConstantsScopeForPartialParse;
    friend class MissingTypeArgumentVisitor;

public:
    Parser(
        NorlsAllocator *TreeStorage,
        Compiler *TheCompiler,
        CompilerHost *TheCompilerHost,
        bool IsXMLDocEnabled,
        LANGVERSION langVersion,
        bool InterpretationUnderDebugger = false,
        bool InterpretationUnderIntelliSense = false,
        bool InitializingFields = false);

    // Create trees for the module-level declarations (everything except method bodies)
    // in a source module.
    HRESULT ParseDecls(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        SourceFile *InputFile,
        _Deref_out_ ParseTree::FileBlockStatement **Module,
        _Inout_ NorlsAllocator *ConditionalCompilationSymbolsStorage,
        BCSYM_Container *ProjectLevelCondCompScope,
        _Out_opt_ BCSYM_Container **ConditionalCompilationConstants,
        _In_ LineMarkerTable *LineMarkerTableForConditionals);

    // Create trees for the body of an individual method.
    HRESULT ParseMethodBody(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        BCSYM_Container *ProjectLevelCondCompScope,
        BCSYM_Container *ConditionalCompilationConstants,
        ParseTree::Statement::Opcodes MethodBodyKind,
        _Out_opt_ ParseTree::MethodBodyStatement **Result);

    // Create a tree for an individual expression.
    HRESULT ParseOneExpression(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _Deref_out_ ParseTree::Expression **Result,
        _Out_opt_ bool*   ErrorInConstruct = NULL,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope = NULL,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants = NULL);

#if HOSTED

    // Creates a tree for an individual expression and also ensures
    // that the parsing of the expression has consumed the entire
    // input stream. If the stream has not ended (besides tkEOL) after
    // the expression has been parsed, then a parse error is reported.
    //
    // Note that this same method could be invoked by the expression
    // editor too to ensure parse time compat.
    //
    HRESULT ParseOneExpressionForHostedCompiler(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _Deref_out_ ParseTree::Expression **Result,
        _Out_opt_ bool*   ErrorInConstruct = NULL);

#endif HOSTED

    // Create a tree for an individual initializer.
    ParseTree::Initializer * ParseOneInitializer(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants);

    // Create a tree for an individual argument list.
    ParseTree::ParenthesizedArgumentList ParseParenthesizedArguments(
        _In_ Scanner *InputStream,
        ErrorTable *Errors);

    // Create a tree for an individual argument list
    // and
    // Create a tree for an individual object initializer list
    void ParseParenthesizedArgumentsAndObjectInitializer
    (
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _Out_ ParseTree::ParenthesizedArgumentList *&ArgumentList,
        _Deref_out_ ParseTree::ObjectInitializerList *&ObjectInitializer,
        _Deref_out_ ParseTree::BracedInitializerList * & CollectionInitializer,
        _Out_ ParseTree::PunctuatorLocation & FromLocation,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants
    );

    // Create a tree for an individual Addhandler statemeent.
    // used by semantics.
    ParseTree::HandlerStatement * ParseHandlerStatement(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _Out_ bool &ErrorInConstruct);

    // Parse and process the project-wide conditional compilation
    // Constant declarations.
    HRESULT ParseProjectLevelCondCompDecls(
        _In_ Scanner *InputStream,
        _In_ Symbols *SymbolTable,
        _Inout_ BCSYM_Hash *HashTable,
        ErrorTable *Errors);

    // Create trees for an individual logical line in a particular syntactic context.
    void ParseOneLine(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _In_ ParseTree::BlockStatement *Context,
        _Out_ ParseTree::StatementList **Result,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope = NULL,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants = NULL);

    void ParseOnePropertyOrEventProcedureDefinition(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _In_ ParseTree::BlockStatement *Context,
        _Out_ ParseTree::StatementList **Result,
        _In_opt_ BCSYM_Container *pProjectLevelCondCompScope,
        _In_opt_ BCSYM_Container *pConditionalCompilationConstants);

    ParseTree::Name * ParseName(
        _Out_ bool &ParseError,
        _In_z_ const WCHAR *Name,
        bool AllowGlobalNameSpace = false,
        bool AllowGenericArguments = false,
        bool *pHasTrailingTokens = NULL);

    ParseTree::Type * Parser::ParseTypeName(
        _Out_ bool &ParseError,
        _In_z_ const WCHAR *NameToParse);

    ParseTree::Name * ParseName(
        _In_ Location *Loc,
        _In_count_(cchName) const WCHAR *Name,
        size_t cchName,
        _Out_opt_ bool *ParseError,
        bool AllowGlobalNameSpace,
        bool AllowGenericArguments);

    ParseTree::Type * Parser::ParseGeneralType(
        _In_ Location *Loc,
        _In_count_(cchTypeText) const WCHAR *TypeText,
        size_t cchTypeText,
        _Out_opt_ bool *ParseError,
        bool AllowEmptyGenericArguments = false);

    HRESULT ParseDeclsAndMethodBodies(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _In_ LineMarkerTable *LineMarkers,
        _Out_ ParseTree::FileBlockStatement **Result,
        _Inout_ NorlsAllocator *ConditionalCompilationSymbolsStorage,
        BCSYM_Container *ProjectLevelCondCompScope,
        _Out_ BCSYM_Container **ConditionalCompilationConstants);

    ParseTree::Expression::Opcodes GetBinaryOperator(
        _In_ Token *T,
        _Out_opt_ OperatorPrecedence *Precedence ); // [out] - can be NULL if not interested in the precedence

    // Create a tree for an Imports statement. Used to parse project level imports.
    ParseTree::ImportDirective * ParseOneImportsDirective(
        _In_ Scanner *InputStream,
        ErrorTable *Errors,
        _Out_ bool &ErrorInConstruct);


public: // Local parser types

    static void SetPunctuator
    (
        _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
        Token *Start,
        _In_opt_ Token *Punctuator
    );

    static void SetPunctuator
    (
        _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
        const Location &Start,
        _In_opt_ Token *Punctuator
    );
   

    struct ParseError
    {
        unsigned Errid;
        Location TextSpan;
    };

    // A ConditionalCompilationDescriptor describes the state of one #if/#elseif/#else
    // sequence.

    struct ConditionalCompilationDescriptor
    {
        ConditionalCompilationDescriptor ();

#if IDE
        bool AnyBranchTaken;
#endif
        bool ElseSeen;
        bool IsBad;

        ParseTree::CCBranchStatement *IntroducingStatement;
    };

    //---------------------------------------------------------------------------------------------
    //
    // A ConditionalCompilationStack implements a stack of ConditionaCompilationDescriptors,
    // necessary because conditional compilation constructs nest.
    //
    class ConditionalCompilationStack
    {
    public:

        void Init (NorlsAllocator *ElementStorage);
        void ShallowCopyInto( _In_ ConditionalCompilationStack *newCloneStack );
        ConditionalCompilationDescriptor *Top ();
        ConditionalCompilationDescriptor *Pop ();

        void Push (ConditionalCompilationDescriptor NewTop);

        bool IsEmpty();

    protected:

        ConditionalCompilationDescriptor InitialElements[10];
        ConditionalCompilationDescriptor *Elements;
        unsigned MaxElements;
        unsigned Depth;
        NorlsAllocator *ElementOverflowStorage;
    };

    StringPool* GetStringPool()
    {
        return m_pStringPool;
    }

    void SetStringPool( _In_ StringPool* pStringPool )
    {
        ThrowIfNull(pStringPool);
        m_pStringPool = pStringPool;
    }

    typedef void (Parser::*StatementParsingMethod) (bool &ErrorInConstruct);

#if IDE
    // This flag is used by IDE to start Xml Language Service early and allow user to get 
    // Xml Intellisense on first invocation. It is also set to true if imports of Xml namespace
    // is encountered
    bool GetSeenXmlLiterals()
    {
        return m_SeenXmlLiterals;
    }
#endif

private:

    void InitParserFromScannerStream(
        _In_ Scanner *InputStream,
        _In_ ErrorTable *Errors
    );

    //
    //============ Methods for parsing general syntactic constructs. =======
    //

    void 
    ParseReturnOrYieldStatement
    (
        Token* Start, 
        _Inout_ bool &ErrorInConstruct, 
        bool ImplicitReturn = false
    );


    void
    ParseDeclarationStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseStatementInMethodBody
    (
        _Inout_ bool &ErrorInConstruct
    );

    // Parse a line within a known context definition.
    void
    ParseLine
    (
        StatementParsingMethod pmStatementParser
    );

    //
    //============ Methods for parsing declaration constructs ============
    //

    ParseTree::Statement *
    ParseSpecifierDeclaration
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::EnumTypeStatement *
    ParseEnumStatement
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseEnumGroupStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseEnumMember
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::TypeStatement *
    ParseTypeStatement
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ReportGenericParamsDisallowedError
    (
        ERRID errid,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ReportGenericArgumentsDisallowedError
    (
        ERRID errid,
        _Inout_ bool &ErrorInConstruct
    );

    void
    RejectGenericParametersForMemberDecl
    (
        _In_ bool &ErrorInConstruct
    );

    ParseTree::NamespaceStatement *
    ParseNamespaceStatement
    (
        _In_ Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseInterfaceGroupStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseInterfaceMember
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParsePropertyOrEventGroupStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::CommentBlockStatement *
    CreateCommentBlockStatement
    (
        _In_ Token *Start
    );

    void
    ParseGroupEndStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement::Opcodes
    ParseEndClause
    (
    );

    ParseTree::Statement *
    CreateEndConstruct
    (
        _In_ Token *StartOfEndConstruct,
        ParseTree::Statement::Opcodes EndType,
        bool linkStatement = true
    );

    ParseTree::Statement *
    CreateEndConstructWithOperand
    (
        _In_ Token *StartOfEndConstruct,
        ParseTree::Statement::Opcodes EndType,
        _In_opt_ ParseTree::Expression *Operand
    );

    ParseTree::SpecifierList *
    ParseSpecifiers
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    Parser::ParseForLoopControlVariable
    (
        ParseTree::BlockStatement *ForBlock,
        _In_ Token *ForStart,
        _Out_ ParseTree::VariableDeclarationStatement **Decl,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    Parser::ParseForLoopVariableDeclaration
    (
        ParseTree::BlockStatement *ForBlock,
        _In_ Token *ForStart,
        _Out_ ParseTree::VariableDeclarationStatement **Decl,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    Parser::ParseContinueStatement
    (
        _In_ Token *StmtStart,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseVarDeclStatement
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::VariableDeclarationList *
    Parser::ParseVariableDeclaration
    (
        _Inout_ ParseTree::VariableDeclarationList *Declarations,
        _Out_ bool &CurrentStatementInError,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ObjectInitializerList *
    ParseObjectInitializerList
    (
        _Inout_ bool &ErrorInConstruct
    );

    static bool
    SetLambdaOwner( _In_ ParseTree::Expression *pExpr, _In_ ParseTree::Statement *pStatement);

    bool
    VariableDeclaratorsContinue();

    bool
    VariableDeclarationsContinue();

    ParseTree::BracedInitializerList *
    ParseInitializerList
    (
        _Inout_ bool &ErrorInConstruct,
        bool AllowExpressionInitializers,
        bool AllowAssignmentInitializers,
        bool AnonymousTypeInitializer = false,
        bool RequireAtleastOneInitializer = false
    );

    bool
    InitializersContinue();

    ParseTree::AssignmentInitializer *
    ParseAssignmentInitializer
    (
        _Inout_ bool &ErrorInConstruct,
        bool MakeExpressionsDeferred
    );

    ParseTree::Initializer *
    ParseInitializer
    (
        _Inout_ bool &ErrorInConstruct,
        bool MakeExpressionsDeferred,
        bool AllowExpressionInitializer = true,
        bool AllowAssignmentInitializer = false
    );

    ParseTree::Initializer *
    ParseDeferredInitializer
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseDeclarator
    (
        bool AllowExplicitArraySizes,
        // The result has been created by the caller.
        _Out_ ParseTree::Declarator *Result,
        _Inout_ bool &ErrorInConstruct
    );

    static bool
    IsIntrinsicType
    (_In_
        Token *Token
    );

    static bool
    CanTokenStartTypeName
    (_In_
        Token *Token
    );

    ParseTree::Type *
    ParseTypeName
    (
        _Inout_ bool &ErrorInConstruct,
        bool AllowEmptyGenericArguments = false,
        _Out_opt_ bool *AllowedEmptyGenericArguments = NULL
    );

    ParseTree::Type *
    ParseGeneralType
    (
        _Inout_ bool &ErrorInConstruct,
        bool AllowEmptyGenericArguments = false
    );

    void
    ParseGenericArguments
    (
        Token *Start,
        ParseTree::GenericArguments &Arguments,
        _Inout_ bool &AllowEmptyGenericArguments,
        _Inout_ bool &AllowNonEmptyGenericArguments,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::TypeList *
    ParseGenericArguments
    (
        _Out_ Token *&Of,
        _Out_ Token *&LeftParen,
        _Out_ Token *&RightParen,
        _Inout_ bool &AllowEmptyGenericArguments,
        _Inout_ bool &AllowNonEmptyGenericArguments,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ArrayType *
    ParseArrayDeclarator
    (
        _In_opt_ Token *TypeStart,
        _In_opt_ ParseTree::Type *ElementType,
        bool AllowExplicitSizes,
        bool InnerArrayType,
        _Inout_ bool &ErrorInConstruct
    );

    bool
    ArrayBoundsContinue
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseProcedureDefinition
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParsePropertyOrEventProcedureDefinition
    (
        _In_ Token *StmtStart,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Name *
    ParseName
    (
        bool RequireQualification,
        _Inout_ bool &ErrorInConstruct,
        bool AllowGlobalNameSpace,
        bool AllowGenericArguments,
        bool DisallowGenericArgumentsOnLastQualifiedName = false,
        bool AllowEmptyGenericArguments = false,
        _Out_opt_ bool *AllowedEmptyGenericArguments = NULL,
        bool AllowGlobalOnly = false
    );

    bool
    AreGenericsArgumentsOnLastName
    (
        _In_ Token *Start
    );

    ParseTree::NameList *
    ParseImplementsList
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::NameList *
    ParseHandlesList
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::MethodDeclarationStatement *
    ParseSubDeclaration
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        bool IsDelegate,
        _Inout_ bool &ErrorInConsruct
    );

    ParseTree::MethodDeclarationStatement *
    ParseFunctionDeclaration
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        bool IsDelegate,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::MethodDeclarationStatement *
    ParseOperatorDeclaration
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        bool IsDelegate,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::PropertyStatement *
    ParsePropertyDefinition
    (
        _In_ ParseTree::AttributeSpecifierList *Attributes,
        _In_ ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct, 
        _In_opt_ bool propertyDefinedInInterface
    ); 

    void ParsePropertyType(
        _Inout_ bool &errorInConstruct,
        _Inout_ Token ** ppAsKeyword,
        _Inout_ Token ** ppNewKeyword,
        _Inout_ Token ** ppFromKeyword,
        _Inout_ Token ** ppWithKeyword,
        _Inout_ ParseTree::AttributeSpecifierList **ppPropertyTypeAttributes,
        _Inout_ ParseTree::Type **ppPropertyType,
        _Inout_ long &deferredInitializationTextStart,
        _Inout_ ParseTree::ParenthesizedArgumentList &PropertyTypeConstructorArguments,
        _Inout_ ParseTree::BracedInitializerList ** ppCollectionInitializer,
        _Inout_ ParseTree::ObjectInitializerList ** ppObjectInitializerList
    );

    ParseTree::PropertyStatement *
    BuildPropertyTree
    (
        bool mustBeAnExpandedProperty, // tells us whether we are making an expanded property or an autoproperty
        _In_ Token *pStart, // Provides location info
        _In_ Token *pIdentifierStart, 
        _In_ Token *pIdentifierEnd,
        _In_ Token *pLeftParen,
        _In_ Token *pRightParen,
        _In_ Token *pEquals,
        _In_ Token *pAsKeyword,
        _In_ Token *pNewKeyword,
        _In_ Token *pWithKeyword,
        _In_ Token *pFromKeyword,
        _In_ Token *pImplementsKeyword,
        _In_ Token *pPropertyPunctuatorEnd,
        long deferredInitializationTextStart,
        _In_ ParseTree::AttributeSpecifierList * pAttributes,
        _In_ ParseTree::SpecifierList *pSpecifiers,
        _In_ ParseTree::IdentifierDescriptor &propertyName,
        _In_ ParseTree::ParameterList *pPropertyParameters,
        _In_ ParseTree::Type *pPropertyType,
        _In_ ParseTree::AttributeSpecifierList *pPropertyTypeAttributes,
        _In_ ParseTree::ParenthesizedArgumentList &PropertyTypeConstructorArguments,
        _In_ ParseTree::Initializer *pInitialValue,
        _In_ ParseTree::BracedInitializerList * pCollectionInitializer,
        _In_ ParseTree::ObjectInitializerList * pObjectInitializerList,
        _In_ ParseTree::NameList * pImplementsList,
        _Inout_ bool &errorInConstruct
    );

    bool DoesPropertyHaveExpandedPropertyMembers
    (
        _In_    Scanner & InputStream,
        _In_    ErrorTable & Errors,
        _In_    BCSYM_CCContainer *pConditionalConstants,
        _In_    Symbols *pConditionalCompilationSymbolsSource,
        _In_    ConditionalCompilationStack &ConditionalsStack,
        _Inout_ bool &errorInConstruct
    );

    ParseTree::MethodDeclarationStatement *
    ParseDelegateStatement
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct
    );

    unsigned
    GetMissingEndProcError
    (
        tokens ProcedureKind
    );

    // FindEndProc searches for the end of a method definition. The return value is
    // true if the method definition ends with an end statement or an EOF, and
    // false if it ends in an error condition (in which case the current token
    // can be assumed to be at the start of another declaration).
    bool
    FindEndProc
    (
        tokens ProcedureKind,
        _In_ Token* StmtStart,
        _In_ ParseTree::MethodDefinitionStatement *Procedure,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::GenericParameterList *
    ParseGenericParameters
    (
        _Out_ Token *&Of,
        _Out_ Token *&LeftParen,
        _Out_ Token *&RightParen,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ParameterList *
    Parser::ParseParameters
    (
        _Inout_ bool &ErrorInConstruct,
        _Out_ Token *&LeftParen,
        _Out_ Token *&RightParen,
        _Inout_ ParseTree::ParseTreeNode * pParent
    );

    ParseTree::ParameterList *
    ParseParameters
    (
        _Inout_ bool &ErrorInConstruct,
        _Out_ Token *&LeftParen,
        _Out_ Token *&RightParen
    );

    ParseTree::ParameterSpecifierList *
    ParseParameterSpecifiers
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Parameter *
    ParseParameter
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::ParameterSpecifierList *Specifiers,
        _In_ Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Parameter *
    ParseOptionalParameter
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::ParameterSpecifierList *Specifiers,
        _In_ Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseOptionalParameters
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::ParameterSpecifierList *Specifiers,
        _In_ Token *ParameterStart,
        ParseTree::ParameterList **&ParamListTarget,
        _Inout_ bool &ErrorInConstruct
    );

    bool
    DoneParamList
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ImportsStatement *
    ParseImportsStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ImportDirective*
    ParseOneImportsDirective
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseInheritsImplementsStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseOptionStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ForeignMethodDeclarationStatement *
    ParseProcDeclareStatement
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseDeclareLibClause
    (
        _Deref_out_ ParseTree::Expression **Library,
        _Deref_out_ ParseTree::Expression **AliasAs,
        _Deref_out_ Token *&Lib,
        _Deref_out_ Token *&Alias,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Statement *
    ParseEventDefinition
    (
        ParseTree::AttributeSpecifierList *Attributes,
        ParseTree::SpecifierList *Specifiers,
        _In_ Token *StatementStart,
        _Inout_ bool &ErrorInConstruct
    );

    enum ExpectedAttributeKind
    {
        FileAttribute,
        NotFileAttribute
    };

    ParseTree::AttributeSpecifierList *
    ParseAttributeSpecifier
    (
        ExpectedAttributeKind Expected,
        _Inout_ bool &ErrorInConstruct
    );

    bool
    BeginsDeclaration
    (
        _In_ Token *Start
    );

    //
    //============ Methods for parsing specific executable statements
    //

    // ParseIfConstruct handles the parsing of block and line if statements and
    // block and line elseif statements, setting *IsLineIf as appropriate.
    // For a line if/elseif, parsing consumes through the first statement (if any)
    // in the then clause. The returned tree is the block created for the if or else if.
    ParseTree::ExpressionBlockStatement *
    ParseIfConstruct
    (
        ParseTree::IfStatement *IfContainingElseIf,
        _Out_ bool *IsLineIf,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseLabelReference
    (
        _Out_ ParseTree::LabelReferenceStatement *LabelReference,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseGotoStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseOnErrorStatement
    (
        _Inout_ bool &ErrorInConstruct
    );


    ParseTree::Initializer *
    ParseSelectListInitializer
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::InitializerList *
    ParseSelectList
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseAggregationExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    bool
    CheckForEndOfExpression
    (
        Token * Start,
        bool &ErrorInConstruct
    );

    ParseTree::Initializer *
    ParseAggregateListInitializer
    (
        bool AllowGroupName,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::InitializerList *
    ParseAggregateList
    (
        bool AllowGroupName,
        bool IsGroupJoinProjection,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::FromList *
    ParseFromList
    (
        bool AssignmentList,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::CrossJoinExpression *
    ParseCrossJoinExpression
    (
        ParseTree::LinqExpression * Source,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::InnerJoinExpression *
    ParseInnerJoinExpression
    (
        _In_ Token * beginSourceToken,
        _In_opt_ ParseTree::LinqExpression * Source,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::GroupJoinExpression *
    ParseGroupJoinExpression
    (
        _In_ Token * beginSourceToken,
        _In_opt_ ParseTree::LinqExpression * Source,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::LinqExpression *
    ParseJoinSourceExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::LinqSourceExpression *
    ParseJoinControlVarExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseJoinPredicateExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression * 
    ParsePotentialQuery
    (
        _Out_ bool &ParsedQuery, // [out] true = that we were on a query and that we handled it / false = not a query
        _Out_ bool &ErrorInConstruct
    );

    ParseTree::LinqExpression *
    ParseFromExpression
    (
        bool StartingQuery,
        bool ImplicitFrom,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::OrderByList *
    ParseOrderByList
    (
        _Inout_ bool &ErrorInConstruct
    );

    bool
    IsContinuableQueryOperator
    (
        Token * pToken
    );


    ParseTree::LinqExpression *
    ParseFromQueryExpression
    (
        bool ImplicitFrom,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::AggregateExpression *
    ParseAggregateQueryExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseResumeStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseCallStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::CallStatement *
    CreateCallStatement
    (
        _In_ Token *Start,
        _In_ ParseTree::Expression *Variable
    );

    void
    ParseRaiseEventStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseRedimStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseHandlerStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::HandlerStatement*
    ParseHandlerWithoutLinkingStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseExpressionBlockStatement
    (
        ParseTree::Statement::Opcodes Opcode,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseAssignmentStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseTry
    (
    );

    void
    ParseCatch
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseFinally
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseErase
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseMid
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseAwaitStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    //
    //============ Methods for parsing portions of executable statements ==
    //

    ParseTree::Expression *
    ParseDeferredExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    // Eat the new line and reflect the continuation situation for that line
    void EatNewLine();

    // Eat the new line but only if the next line starts with the specified token
    void EatNewLineIfFollowedBy
    (    
        tokens tokenType,
        bool isQueryContext = false
    );

    void Parser::EatNewLineIfNotFollowedBy
    (
        tokens tokenType, // the token we want to check for on start of the next line
        bool isQueryOp // [in] whether the tokenType represents a query operator token
    );

    void EatNewLineIfNotFirst(bool & first);

    bool Parser::NextLineStartsWith
    (
        tokens tokenType, // the token we want to check for on start of the next line
        bool isQueryOp // [in] whether the tokenType represents a query operator token
    );
    
    ParseTree::Expression *
    ParseExpression
    (
        OperatorPrecedence PendingPrecedence,
        _Inout_ bool &ErrorInConstruct,
        //EatLeadingNewLine: Indicates that the parser should
        //dig through a leading EOL token when it tries
        //to parse the expression.
        bool EatLeadingNewLine = false,   // Note: this flag is incompatible with BailIfFirstTokenRejected
        ParseTree::Expression * pContainingExpression = NULL,
        bool BailIfFirstTokenBad = false  // bail (return NULL) if the first token isn't a valid expression-starter, rather than reporting an error or setting ErrorInConstruct
    );

    ParseTree::Expression *
    ParseExpression
    (
        _Inout_ bool &ErrorInConstruct
    )
    {
        return ParseExpression(PrecedenceNone, ErrorInConstruct, false, NULL);
    }

    bool ShouldSkipLambdaBody();

    ParseTree::LambdaExpression*
    ParseStatementLambda
    ( 
        _In_ ParseTree::ParameterList *pParams, 
        Token *pStart,
        bool functionLambda, 
        bool isSingleLine,
        bool parseBody,  // normally true. Set this to false to prevent it trying to parse the body. That's only allowed when isSingleLine.
        bool &errorInConstruct
    );

    bool 
    IsMultiLineLambda();
    
    bool    
    EndOfMultilineLambda
    (
        _In_ Token *pCurrentLineToken, 
        _In_ ParseTree::LambdaBodyStatement *pLambdaBody
    );

    bool
    CatchOrFinallyBelongsToLambda( _In_ ParseTree::LambdaBodyStatement *pLambdaBody);

    ParseTree::Expression *
    ParseTerm
    (
        _Inout_ bool &ErrorInConstruct,
        bool BailIfFirstTokenBad = false  // bail (return NULL) if the first token isn't a valid expression-starter, rather than reporting an error or setting ErrorInConstruct
    );

    ParseTree::Expression *
    ParseGetType
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseGetXmlNamespace
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseCastExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseNewExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseTypeOf
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseVariable
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseQualifiedExpr
    (
        _In_ Token *Start,
        _In_opt_ ParseTree::Expression *Term,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseBracketedXmlQualifiedName
    (
        bool IsElement,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseParenthesizedExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ParenthesizedArgumentList
    ParseParenthesizedArguments
    (
        _Inout_ bool &ErrorInConstruct
    );


    ParseTree::Expression *
    ParseParenthesizedQualifier
    (
        _In_ Token *Start,
        _In_ ParseTree::Expression *Term,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseGenericQualifier
    (
        Token *Start,
        _In_opt_ ParseTree::Expression *Term,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseArguments
    (
        _In_ ParseTree::ArgumentList **Target,
        _Inout_ bool &ErrorInConstruct
    );

    bool

    ArgumentsContinue
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseNamedArguments
    (
        ParseTree::ArgumentList **Target,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Argument *
    ParseArgument
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseCType
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseSimpleExpressionArgument
    (
        ParseTree::ArgumentList **&Target,
        _Inout_ bool &ErrorInConstruct
    );

    void
    ParseVariableInList
    (
        ParseTree::ExpressionList **&Target,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ExpressionList *
    ParseVariableList
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlExpression
    (
        _Inout_ bool &ErrorInConstruct
    );


    ParseTree::XmlElementExpression *
    ParseXmlElement
    (
        _In_opt_ ParseTree::XmlElementExpression *Parent,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::XmlElementExpression *
    Parser::ParseXmlEndElement
    (
    _Inout_ ParseTree::XmlElementExpression *Element,
    Token *StartToken,
    _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ExpressionList *
    ParseXmlAttributes
    (
        bool AllowNameAsExpression,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlAttribute
    (
        bool AllowNameAsExpression,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlAttributeValue
    (
        _Out_ ParseTree::XmlAttributeExpression *Attribute,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlQualifiedName
    (
        bool AllowExpr,
        bool IsBracketed,
        bool IsElementName,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlDocument
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::XmlDocumentExpression *
    ParseXmlDecl
    (
         _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ExpressionList **
    ParseXmlMisc
    (
        _Inout_ ParseTree::ExpressionList **Prev,
        bool IsProlog,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlMarkup
    (
         _In_opt_ ParseTree::XmlElementExpression *Parent,
         bool AllowVBExpression,
         _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ExpressionList *
    ParseXmlContent
    (
        _Inout_ ParseTree::XmlElementExpression *Parent,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlPI
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlCData
    (
        _Inout_ bool &ErrorInConstruct
    );


    ParseTree::Expression *
    ParseXmlComment
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::ExpressionList *
    ParseXmlOtherData
    (
        tokens EndToken,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    Parser::ParseXmlString
    (
        _Out_ ParseTree::XmlAttributeExpression *Attribute,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    Parser::ParseXmlReference
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseXmlEmbedded
    (
        bool AllowEmbedded,
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::XmlReferenceExpression *
    Parser::MakeXmlCharReference
    (
    );

    ParseTree::XmlReferenceExpression *
    Parser::MakeXmlCharReference
    (
        _In_ const WCHAR *Value,
        size_t Length,
        bool CopyData
    );

    ParseTree::XmlReferenceExpression *
    Parser::MakeXmlEntityReference
    (
    );

    ParseTree::XmlCharDataExpression *
    Parser::MakeXmlStringLiteral
    (
    );

    bool
    Parser::SkipXmlWhiteSpace
    (
    );

    bool
    HandleUnexpectedToken
    (
        tokens ExpectedTokenType,
        _Inout_ bool &ErrorInConstruct
    );

    bool
    VerifyExpectedToken
    (
        tokens ExpectedTokenType,
        _Inout_ bool &ErrorInConstruct,
        bool EatNewLineAfterToken = false,
        ParseTree::ParseTreeNode * pNode = NULL
    );

    bool
    VerifyExpectedXmlToken
    (
        tokens ExpectedTokenType,
        _Inout_ bool &ErrorInConstruct
    );

    void
    VerifyTokenPrecedesExpression
    (
        tokens ExpectedTokenType,
        _Inout_ bool &ErrorInConstruct,
        bool EatNewLineAfterToken = false,
        ParseTree::ParseTreeNode * pNode = NULL
    );

    void
    VerifyTokenPrecedesVariable
    (
        tokens ExpectedTokenType,
        _Inout_ bool &ErrorInConstruct,
        bool EatNewLineAfterToken = false,
        ParseTree::ParseTreeNode * pNode = NULL        
    );

    ParseTree::Expression *
    ParseOptionalWhileUntilClause
    (
        _Out_ bool *IsWhile,
        _Inout_ bool &ErrorInConstruct
    );

    //
    //============ Methods for parsing syntactic terminals ===============
    //

    ParseTree::IdentifierDescriptor
    ParseIdentifier
    (
        _Inout_ bool &ErrorInConstruct,
        bool allowNullable=false
    );

    ParseTree::IdentifierDescriptor
    ParseIdentifierAllowingKeyword
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseIdentifierExpressionAllowingKeyword
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseIdentifierExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseIntLiteral
    (
    );

    ParseTree::Expression *
    ParseCharLiteral
    (
    );

    ParseTree::Expression *
    ParseStringLiteral
    (
        _Inout_ bool &ErrorInConstruct
    );

    ParseTree::Expression *
    ParseDecLiteral
    (
    );

    ParseTree::Expression *
    ParseFltLiteral
    (
    );

    ParseTree::Expression *
    ParseDateLiteral
    (
    );

    void
    ParseTrailingComments
    (
    );

    void
    CreateComment
    (
        _In_ Token *CommentToken
    );

    //
    //============ Methods related to managing text locations. =================
    //

    long
    TokenColumnBeg
    (
        _In_ Token *Tok
    );

    long
    TokenColumnEnd
    (
        _In_ Token *Tok
    );

    void
    CacheStmtLocation
    (
    );

    void
    SetStmtBegLocation
    (
        ParseTree::Statement *Statement,
        _In_ Token *Start
    );

    void
    SetStmtEndLocation
    (
        _In_ ParseTree::Statement *Statement,
        _In_ Token *FollowingEnd
    );

    void
    SetFileBlockLocation
    (
        ParseTree::FileBlockStatement *File,
        _In_ Token *LastToken
    );

    void Parser::SetLocationForSpanOfOneToken
    (
        _Out_ Location *pLoc,     // [out] we fill out Loc based on the Beg and End tokens
        _In_ Token *pToken        // [in] begin token for the construct (inclusive)
    );

    void
    SetLocation
    (
        _Out_ Location* Loc,     // [in] location to set
        _In_ Token *Beg,        // [in] begin token for the construct (inclusive)
        _In_ Token *End         // [in] end token for the construct (exclusive)
    );

    void
    SetLocationAndPunctuator
    (
        _Out_ Location *TextSpan,
        _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
        _In_ Token *Beg,
        _In_ Token *End,
        _In_opt_ Token *Punctuator
    );

    void
    SetExpressionLocation
    (
        _Out_ ParseTree::Expression *Expr,
        _In_ Token *Beg,
        _In_ Token *End,
        _In_opt_ Token *FirstPunctuator
    );

    // If T is the first token following a logical newline, return the EOL
    // (or comment, if one is present at the end of the line) token
    // following the nearest non-EOL token preceding T, and otherwise return T.
    // (This is used to set locations for constructs that actually end at a
    // newline but for which the following token is the first token of the
    // next line.)
    Token *
    BackupIfImpliedNewline
    (
        _In_ Token *T
    );

    // Set the location info for a block tree (the current context)
    // to the beginning of the construct that ends the block.
    void
    SetBlockEndLocationInfo
    (
        _In_ Token *StartOfEndConstruct
    );

    // ISSUE Microsoft. These template methods are not necessarily appropriate as inline
    // methods, but VC does not allow non-inline definitions of template member functions.

    // Lists are right heavy and parsed iteratively, so the end location info
    // for each list element isn't known until the entire list has been parsed.
    // FixupListEndLocations traverses the elements of a list, setting the end location
    // of each list element to be the end of the list.
    // e.g. each list location goes from its start to its end.  a,b,c goes from a to c 
    // 
    //             list  <-- need to set the span for this to be from a->c
    //            /    \
    //           a    list  <-- span for this is b->c
    //                /   \
    //               b   list  <-- span for this is c
    //                   /  \
    //                  c   NULL
    //

    template <class ElementType, class ListType> void
    FixupListEndLocations
    (
        _In_ ParseTree::List<ElementType, ListType> *List
    )
    {
        Token *LastTokenOfList = BackupIfImpliedNewline(m_CurrentToken)->m_Prev;

        ParseTree::List<ElementType, ListType> * pLast = List;

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
                LastTokenOfList->m_TokenType == tkEOL ||
                // Dev10 694189 because tkEOF tokens don't have 'normal' location info (having no location and ListElement->TextSpan->EndColumn = 0)
                // it would be deemed invalid even though it isn't so excuse it from this assert.
                LastTokenOfList->m_TokenType == tkEOF || 
                (ListElement->TextSpan.m_lEndLine == LastTokenOfList->m_StartLine &&
                ListElement->TextSpan.m_lEndColumn == LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1),
                "List ends at a surprising point.");

            // For statements with errors the last token of the list can end to some strange positions (see b73623)
            // This would produces ill formatted locations for the list elements with errors and subsequently could crashes the compiler.
            // For such cases make sure the end location equals the start location, at least.

            bool validEnd = (ListElement->TextSpan.m_lBegLine < LastTokenOfList->m_StartLine) ?
                                true :
                                ((ListElement->TextSpan.m_lBegLine == LastTokenOfList->m_StartLine) ?
                                    (ListElement->TextSpan.m_lBegColumn <= LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1) :
                                    false );

            if (validEnd)
            {
                // Microsoft:Consider - it is strange that we can have zero-width tokens yet we subract one from them anyway,
                // e.g. ListElement->TextSpan.m_lEndColumn == LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1
                // I tried patching this up but the IDE apparently has a number of places that accomodates this wierdness and
                // fixing it broke suites.  I have elected to leave it alone given that we are late in beta2.  For posterity, what I tried 
                // was: long LastTokenOfListEndColumn = LastTokenOfList->m_Width > 0 ? LastTokenOfList->m_Width - 1 : 0;
                // and then I used that in the places we are using LastTokenOfList->m_Width - 1.  Though it fixes the inverted
                // span problem, it breaks the IDE as they have adapted to this wierdness.
                // We can't VSASSERT( ListElement->TextSpan.m_lEndColumn >= LastTokenOfList->m_StartColumn, "Inverted span" );
                // here because we in fact return inverted spans in some cases which the IDE apparently relies on.
                ListElement->TextSpan.m_lEndLine = LastTokenOfList->m_StartLine;
                ListElement->TextSpan.m_lEndColumn = LastTokenOfList->m_StartColumn + LastTokenOfList->m_Width - 1;
            }

            pLast = ListElement;
        }
   }

    template <class ElementType, class ListType> void
    CreateListElement
    (
        _Out_ ListType **&Target,
        ElementType *Element,
        _In_ Token *Start,
        _In_ Token *End,
        _In_opt_ Token *ListPunctuator
    )
    {
        ThrowIfNull(Target);
        
        ListType *ListElement = new(m_TreeStorage) ListType ;

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

#if IDE 
    void
    FixAttributeArgumentValueOffsets
    (
        ParseTree::ArgumentList *pArguments,
        unsigned long iAttributeStartOffset
    );
#endif IDE

    //
    //============ Methods to manage internal state. ================================
    //

    // The general strategy for enabling/disabling state is to call a Disable/Enable method
    // to guarantee that the parser is in a certain state. The Enable/Disable method returns
    // a flag indicating whether or not the parser was already in that state. This flag is
    // passed to the method to undo the state request, in order to tell whether or not the
    // parser should remain in that state.

    // Disabling generation of error messages.

    bool
    DisableErrors
    (
    );

    void
    EnableErrors
    (
        bool ContinueDisabled
    );

    bool
    IsErrorDisabled
    (
    );

    // Enabling keeping count of errors.

    bool
    EnableErrorTracking
    (
    );

    void
    DisableErrorTracking
    (
        bool ContinueTracking
    );

    unsigned
    CountOfTrackedErrors
    (
    );

    // Disabling discarding of tokens for parsed text lines.

    bool
    DisableAbandonLines
    (
    );

    void
    EnableAbandonLines
    (
        bool ContinueDisabled
    );

    //
    //============ Methods to test properties of tokens. ====================
    //

    // IdentifierAsKeyword returns the token type of a identifier token,
    // interpreting non-bracketed indentifiers as (non-reserved) keywords as appropriate.
    tokens
    IdentifierAsKeyword
    (
        _In_ Token *T
    );

    // TokenAsKeyword returns the token type of a token, interpreting
    // non-bracketed identifiers as keywords as appropriate. The token need
    // not necessarily be a keyword or identifier.
    tokens
    TokenAsKeyword
    (
        _In_ Token *T
    );

    bool
    EqNonReservedToken
    (
        _In_ Token *T,
        tokens NonReservedTokenType
    );

    //
    //============ Methods related to creating error messages. ================
    //

    void
    AddError
    (
        ParseError *Error,
        _In_opt_z_ const WCHAR *Substitution1 = NULL
    );

    void
    AddError
    (
        ParseError *Error,
        _In_z_ const WCHAR *Substitution1,
        _In_z_ const WCHAR *Substitution2
    );

    void
    SetErrorLocation
    (
        _Inout_ ParseError *Error,
        _In_ Token *Start,
        _In_ Token *End
    );

    void
    ReportSyntaxError
    (
        unsigned Errid,
        _In_ Token *Beg,
        _In_ Token *End,
        bool MarkErrorStatementBad,
        _Inout_ bool &ErrorInConstruct
    );

    // Report a syntax error and mark the next statement to be linked with
    // the HasSyntaxError flag.

    void
    ReportSyntaxError
    (
        unsigned ErrorId,
        _Inout_ Token *Start,
        _Inout_ Token *End,
        _Inout_ bool &ErrorInConstruct
    );

    // Use this version for an error whose text span is known to be one token long.

    void
    ReportSyntaxError
    (
        unsigned ErrorId,
        _In_ Token *StartAndEnd,
        _Inout_ bool &ErrorInConstruct
    );

    // Generate a syntax error without entering a recovery state and without
    // disabling pretty listing.

    void
    ReportSyntaxErrorWithoutMarkingStatementBad
    (
        unsigned ErrorId,
        _In_ Token *Start,
        _In_ Token *End,
        bool ErrorInConstruct
    );

    void
    ReportSyntaxErrorWithoutMarkingStatementBad
    (
        unsigned ErrorId,
        _In_ Token *StartAndEnd,
        bool ErrorInConstruct
    );

    void
    ReportSyntaxErrorWithoutMarkingStatementBad
    (
        unsigned Errid,     
        _In_ Location *TextSpan,          
        _Inout_ bool &ErrorInConstruct,
        _In_opt_z_ const WCHAR *Substitution1 = NULL
    );

    void
    ReportSyntaxError
    (
        unsigned ErrorId,
        _In_ Location *TextSpan,
        _Inout_ bool &ErrorInConstruct,
        _In_opt_z_ const WCHAR *Substitution1 = NULL,
        _In_opt_ bool MarkStatementBad = true
    );

    ParseTree::Statement *
    ReportUnrecognizedStatementError
    (
        unsigned ErrorId,
        _Inout_ bool &ErrorInConstruct
    );

    // Report a syntax error and mark the last statement linked with
    // the HasSyntaxError flag.

    void
    ReportSyntaxErrorForPreviousStatement
    (
        unsigned ErrorId,
        _In_ Token *Start,
        _In_ Token *End,
        _Inout_ bool &ErrorInConstruct
    );

    // Report a syntax error because of a feature requested that does not
    // match the currently selected language.

public:
    void
    ReportSyntaxErrorForLanguageFeature
    (
        unsigned ErrorId,
        _In_ Token *Start,
        unsigned Feature,
        _In_opt_z_ const WCHAR *wszVersion
    );


    //
    //============ Methods to manage syntactic contexts ======================
    //

private:

    void
    AssignResumeIndices
    (
        _Inout_ ParseTree::Statement *Statement
    );

    void
    LinkStatement
    (
        _Out_ ParseTree::Statement *Statement
    );

    void
    EstablishContext
    (
        _Inout_ ParseTree::BlockStatement *Block
    );

    static bool TryAddMultilineLambdaStatementsToLexicalStatementList
    (
        _In_ ParseTree::Expression *pExpr,
        _In_ ParseTree::StatementList **ppListElement
    );
    
    void
    IterateAndAddMultilineLambdaToLexicalStatementList( _In_ ParseTree::LambdaBodyIter *pLambdaBodyIter);

    static ParseTree::StatementList*
    GetLastLexicalStatement(_In_ ParseTree::StatementList *pList);

    // PopContext establishes the parent of the current context as the current
    // context.
    void
    PopContext
    (
        bool ContextHasProperTermination,
        _In_ Token *StartOfEndConstruct
    );

    // PopBlockCommentContext establishes the parent of the current context as the current
    // context for comments blocks.
    void
    Parser::PopBlockCommentContext
    (
        Token *StartOfEndConstruct
    );

    // If the context comment block needs to be terminated, this function will do so, otherwise
    // it doesn't do a thing. Ending comment blocks is a little complicated because there is no
    // implicit end construct, this is why this is a seperate function.
    void
    Parser::EndCommentBlockIfNeeded
    (
    );

    // EndContext establishes the parent of the current context as the current
    // context, and generates an end construct for the removed context.
    ParseTree::Statement *
    EndContext
    (
        _In_ Token *StartOfEndConstruct,
        ParseTree::Statement::Opcodes EndOpcode,
        _In_opt_ ParseTree::Expression *Operand
    );

    // Returns the nearest enclosing non-region context.
    ParseTree::BlockStatement *
    RealContext
    (
    );

    // Returns the nearest containing Try, Catch, or Finally, or NULL if the
    // current context is not within one of these.
    ParseTree::BlockStatement *
    FindContainingHandlerContext
    (
    );

    // Return true if the target context is somewhere in the current context
    // hierarchy, and false otherwise.
    bool
    FindContainingContext
    (
        ParseTree::BlockStatement *TargetContext
    );

    // Return true if pExpectedParent is a parent of pChild.
    bool
    ConfirmParentChildRelationship
    (
        _In_ ParseTree::BlockStatement *pExpectedParent,
        _In_ ParseTree::Statement *pChild
    );

    // Return true if our immediate enclosing context is a single-line sub lambda
    bool
    IsParsingSingleLineLambda()
    {
        return m_pStatementLambdaBody!=NULL &&
               m_pStatementLambdaBody->pOwningLambdaExpression!=NULL &&
               m_pStatementLambdaBody->pOwningLambdaExpression->IsSingleLine;
    }

    //
    //============ Methods to encapsulate scanning ========================
    //

    bool
    IsValidStatementTerminator
    (
        _In_ Token *T
    );

    bool
    CanFollowStatement
    (
        _In_ Token *T
    );

    bool
    CanEndExecutableStatement
    (
        _In_ Token *T
    );

    bool
    CanFollowExpression
    (
        _In_ Token *T
    );

    bool
    BeginsEmptyStatement
    (
        _In_ Token *T
    );

    bool
    BeginsStatement
    (
        _In_ Token *T
    );

    // Returns the OF token if this begins a generic, else returns NULL
    //
    // Returning a token instead of a bool is useful to retain the optional
    // parantheses semantics in case the decision to make them mandatory is
    // ever changed.
    //
    Token*
    BeginsGeneric
    (
        _In_ Token *T
    );

    bool
    BeginsGenericWithPossiblyMissingOf
    (
        _In_ Token *T
    );

    bool
    BeginsEvent
    (
        _In_ Token *T
    );

    // Does this token force the end of a statement?
    bool
    MustEndStatement
    (
        _In_ Token *T
    );

    bool
    IsEndOfLine
    (
        _In_ Token *T
    );

    // Following returns the token following T, skipping over line ends.
    // If T's Next field is an end-of-line, the returned token is the first
    // token of the next line.
    Token *
    Following
    (
        _In_ Token *T
    );

    // GetNextLine gets the tokens for the next line, and does not process
    // any of them.

    Token *
    GetNextLine
    (
        tokens TokenType = tkNone
    );

    Token * PeekNextLine(Token * pAfter = NULL);

    void
    GoToEndOfLine
    (
        _Inout_ Token **ppInputToken
    );

    // GetTokensForNextLine throws away the tokens for the current line,
    // gets the tokens for the next line, and processes
    // line numbers and labels.
    void
    GetTokensForNextLine
    (
        bool WithinMethod
    );

    // GetNextToken advances the current token, skipping over line ends.
    Token *
    GetNextToken
    (
    );

    // GetNextToken advances the current token, skipping over line ends.
    Token *
    GetNextXmlToken
    (
    );

    tokens __cdecl
    PeekAheadFor
    (
        unsigned PeekaheadCount,
        ...
    );

    tokens __cdecl
    PeekAheadForComment
    (
    );

    Token* __cdecl
    Parser::PeekAheadForToken
    (
        _In_ Token *pInputToken,
        unsigned Count,        // [in] count of tokens to look for
        ...                     // [in] var_list of tokens (tk[]) to look for
    );

    tokens 
    ResyncAtInternal
    (
        bool TreatColonAsStatementTerminator,
        unsigned ResyncCount,
        va_list Tokens
    );

    tokens  __cdecl
    ResyncAt
    (
        unsigned ResyncCount,
        ...
    );

    tokens  __cdecl
    CCResyncAt
    (
        unsigned ResyncCount,
        ...
    );

    //
    //============ Methods related to conditional compilation. ==============
    //

    void
    ParseConditionalCompilationStatement
    (
        bool SkippingMethodBody
    );

    ParseTree::Expression*
    ParseConditionalCompilationExpression
    (
        _Inout_ bool &ErrorInConstruct
    );

    static bool
    StartsValidConditionalCompilationExpr
    (
        _In_ Token *T
    );

    static bool
    IsValidOperatorForConditionalCompilationExpr
    (
        _In_ Token *T
    );

    void
    MoveRegionChildrenToRegionParent
    (
        _Inout_ ParseTree::BlockStatement *EndedRegion,
        _In_opt_ ParseTree::Statement *EndRegionStatement,
        _Out_ ParseTree::StatementList *LastRegionBodyLink
    );

    void
    ParseConditionalCompilationLine
    (
        bool SkippingMethodBody
    );

    void
    InterpretConditionalCompilationConditional
    (
        _In_ ParseTree::ExpressionStatement *Directive
    );

    // Verify that a conditional compilation statement ends correctly.
    // If the statements ends at an end-of-line, the current token is
    // set the to EOL.
    void
    VerifyEndOfConditionalCompilationStatement
    (
        _Inout_ bool &ErrorInConstruct
    );

    // If m_CurrentToken is at a colon, then parse the next statement, and repeat
    void
    GreedilyParseColonSeparatedStatements
    (
        _Inout_ bool &ErrorInConstruct
    );

    void
    SkipConditionalCompilationSection
    (
        _Out_ Location& skippedLocation
    );

    bool
    EvaluateConditionalCompilationExpression
    (
        ParseTree::Expression *Expr,
        _Inout_ ConditionalCompilationDescriptor *Conditional
    );

    void
    RecoverFromMissingConditionalEnds
    (
    );

    //
    //============ Miscellaneous methods. ===================================
    //

    
    bool
    DoLookAheadToSeeIfPropertyIsExpanded();

    void
    AbandonPreviousLines
    (
    );

    void
    AbandonAllLines
    (
    );

    ParseTree::Expression *
    CreateBoolConst
    (
        bool Value
    );

    // Deal with text following a statement. The return value is true if all text from
    // a line has been digested, and false otherwise.
    // If the return value is true, the current token will be an EOL or EOF.
    bool
    VerifyEndOfStatement
    (
        _Inout_ bool &ErrorInConstruct,
        bool ignoreColon = false
    );

    void
    ParseMismatchedLoopClause
    (
        _In_ Token *Start,
        _Inout_ bool &ErrorInConstruct
    );

    void
    RecoverFromMismatchedEnd
    (
        _In_ Token *Start,
        ParseTree::Statement::Opcodes EndType,
        _Inout_ bool &ErrorInConstruct
    );

    void
    RecoverFromMissingEnds
    (
        ParseTree::BlockStatement *ContextToMatch
    );

    ParseTree::MethodDeclarationStatement *
    CreateMethodDeclaration
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
    );

    ParseTree::Expression *
    ExpressionSyntaxError
    (
    );

    ParseTree::IdentifierDescriptor
    CreateId
    (
        _In_ Token *IdToken
    );

    ParseTree::IdentifierDescriptor
    Parser::CreateXmlNameIdentifier
    (
        _In_ Token *NameToken,
        bool &ErrorInConstruct
    );

    void
    CreateZeroArgument
    (
        ParseTree::ArgumentList **&ArgumentsTarget
    );

    void
    UpdateStatementLinqFlags
    (
        _Out_ ParseTree::Statement *pStatement
    );

    void
    AssertLanguageFeature
    (
        unsigned feature,
        _Inout_ Token* Current
    );

    //
    //============ Begin Parser data fields. =======================
    //

    // Current compilation context.
    Compiler *m_Compiler;
    CompilerHost *m_CompilerHost;
    StringPool* m_pStringPool;


    // The input stream of tokens.
    Scanner *m_InputStream;

    // The current Token.
    Token *m_CurrentToken;

    // Allocator used for tree creation.
    NorlsAllocator &m_TreeStorage;

    // Table in which to place messages.
    ErrorTable *m_Errors;

    // Where the conditional compilation constants come from and live.
    Symbols *m_ConditionalCompilationSymbolsSource;
    BCSYM_CCContainer *m_ConditionalConstantsScope;

    // Indicates whether the current expression being evaluated is in
    // a conditional compilation statement context.
    bool m_EvaluatingConditionCompilationExpression;

    // The number of errors that have occurred since last resetting the count.
    unsigned short m_ErrorCount;

    // The enclosing syntactic context, which must be a block tree.
    ParseTree::BlockStatement *m_Context;

    // The tree for the method definition being parsed. NULL unless a method
    // body is being parsed.
    ParseTree::MethodBodyStatement *m_Procedure;

    // The body of the multiline lambda that is being parsed. Null unless a 
    // lambda is being parsed.
    ParseTree::LambdaBodyStatement* m_pStatementLambdaBody;

    ParseTree::LambdaBodyStatement *m_pFirstMultilineLambdaBody;

    // Bool that is true only when the parser is created in InitializeFields()
    bool m_InitializingFields;

    // The last label statement parsed in the current procedure.
    ParseTree::StatementList *m_LastLabel;

    // A stack of last statements that have been linked into contexts.
    ParseTree::StatementList **m_LastInContexts;
    ParseTree::StatementList *m_ScratchLastInContexts[100];
    int m_ContextIndex;
    int m_MaxContextDepth;

    // The last statement to be linked (into any context).
    ParseTree::StatementList *m_LastStatementLinked;

    // Only used to verify location information is consistent.
#if DEBUG
    long m_CurrentStatementStartLine;
    long m_CurrentStatementStartColumn;
#endif

    // The state of conditional compilation directives.
    ConditionalCompilationStack m_Conditionals;

    // If the context is within an ExternalSource directive, the enclosing directive,
    // otherwise NULL.
    struct ParseTree::ExternalSourceDirective *m_CurrentExternalSourceDirective;

    // If ExternalSourceDirectives are being collected, where to put the next one,
    // otherwise NULL.
    struct ParseTree::ExternalSourceDirective **m_ExternalSourceDirectiveTarget;

    // FirstTokenAfterNewline is the first token following a newline encountered
    // within a statement, and is NULL at the beginning of parsing a logical
    // line.
    Token *m_FirstTokenAfterNewline;

    // UndisposedComment is a first comment token that precedes the current
    // token but which has not been attached to a statement.
    Token *m_FirstUndisposedComment;

    // The root of an xml literal
    ParseTree::ExpressionList *m_XmlRoots;       // List of all Xml literals in the current statement
    ParseTree::ExpressionList **m_XmlRootsEnd;   // Pointer to the end of XmlRoot list

    // State for attaching lists of comments to statements.
    ParseTree::CommentList *m_CommentsForCurrentStatement;
    ParseTree::CommentList **m_CommentListTarget;

    //  Used to save the last linked statement becaue it may be changed if this statment contains a multi-line lambda
    //  otherwise m_pCurrentStatement and m_LastStatementLinked are the same.
    //  Comments are always attached to m_pCurrentStatement and syntax errors are reported on then 
    //  the m_pCurrentStatement and not the lastlinked statement.  In the case of a statment containing a multi-line lambda, 
    //  the last linked statement will point to the END SUB/END FUNCTION.
    // m_LastStatementList. See ParseTrailingComments().
    ParseTree::StatementList *m_pCurrentStatement;

    // the statement currently being parsed is redim or new. In this context lower bound '0 to' is
    // allowed in arguments
    bool m_RedimOrNew;

    /* Indicates whether we need to force method declaration line state when scanning inside PeekNextLine().  
       This context is necessary when we are doing look ahead in implicit line continuation cases because the
       scanner parses XML and it needs some context to know when it should parse XML vs. Attributes.  The context
       it needs to know in this case is whether we are processing a method declaration because XML is highly restricted
       in that case. But if line continuation is encountered the scanner doesn't know when it is inside the declaration 
       so we need the parser to tip it off so it can scan attributes vs. xml correctly inside the parameter list.

       We need to change the fact that the scanner parses xml like this.  Historically it was done that way because 
       the colorizer uses the scanner to colorize.  In Dev10 the colorizer should use the parser instead and XML
       parsing should move to the parser.  In the mean time, we need to give hints to the scanner when we are
       parsing parameters so it knows that XML is not allowed.  See dev10 #504604, #536130 for an example 
       where we mistake attributes on a parameter for XML. */
    bool m_ForceMethodDeclLineState;

    // The statement immediately following a Select must be a Case, comment,
    // or End. This flag indicates that the parser has seen a Select statement
    // but has not seen the first Case of the Select.
    bool m_SelectBeforeFirstCase;

    // Some aspects of the parser (e.g. conditional compilation) act differently
    // depending on whether the parse is for an individual line.
    bool m_IsLineParse;

    // Some aspects of the parser (e.g. parsing procedure definitions) act
    // differently when the parse is for decls including method bodies.
    bool m_IsDeclAndMethodBodiesParse;

    // A flag to indicate whether or not the parse is for/within a method body,
    // and a field to indicate what opcode to expect for an Exit statement
    // that exits the method.
    bool m_ParsingMethodBody;
    ParseTree::Statement::Opcodes m_ExpectedExitOpcode;

    // A flag to suppress error reporting.
    bool m_ErrorReportingDisabled;

    // A flag to keep track of whether or not to count errors as they occur.
    bool m_ErrorTrackingEnabled;

    // A flag to suppress discarding the tokens for lines as the parser advances.
    bool m_KeepLines;

    // A flag to indicate that the current statement contains a syntax error.
    bool m_CurrentStatementInError;

    // A flag to indicate that the current statement is the first statement on the
    // current logical line.
    bool m_FirstStatementOnLine;

    // A flag to indicate that a statement that can't precede an Option
    // statement has been parsed.
    bool m_ProhibitOptionStatement;

    // A flag to indicate that a statement that can't precede an Imports
    // statement has been parsed.
    bool m_ProhibitImportsStatement;

    // A flag to indicate that a statement that can't precede an Attribute
    // statement has been parsed.
    bool m_ProhibitAttributeStatement;

    // A flag to indicate that the parsing is in service of a debugger interpretation.
    bool m_InterpretationUnderDebugger;

    // A flag to indicate that the parsing is in service of IntelliSense.
    bool m_InterpretationUnderIntelliSense;

    // A flag to indicate if we interpret XMLDocs as normal comments or not.
    bool m_IsXMLDocEnabled;

    // A flag for parsing in the context of a line if
    bool m_IsLineIf;

#if LINQ_INTELLISENSE
    // A flag to indicate we are within a Select Expression
    bool m_IsSelectExpression;
#endif

    bool m_SeenAnonymousTypeInitialization;
    bool m_SeenQueryExpression;
    bool m_SeenLambdaExpression;

#if IDE
    bool m_SeenXmlLiterals;
#endif

    // keep track of # of multiline lambdas seen on a single statement within a method. 
    DynamicArray<ParseTree::LambdaBodyStatement *> m_LambdaBodyStatements;


    LANGVERSION m_CompilingLanguageVersion;

    // Dev10 #789970 
    // This flag affects how conditional compilation is handled, the assumption is that 
    // Conditional Constants Scope has been completely built already and errors related to conditional 
    // compilation directives have been reported.   
    bool m_DoLiteParsingOfConditionalCompilation;

    // Allow global namespace prefix
    bool m_AllowGlobalNamespace;
};

bool IsBinaryOperator(_In_ Token *T);
bool IsAssignmentOperator(_In_ Token *T);
bool IsRelationalOperator(_In_ Token *TokenToTest);

inline
void Parser::SetPunctuator
(
    _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
    Token *Start,
    _In_opt_ Token *Punctuator
)
{
    if (Punctuator)
    {
        PunctuatorSource->Line = Punctuator->m_StartLine - Start->m_StartLine;
        PunctuatorSource->Column = Punctuator->m_StartColumn;
    }
}


inline
void Parser::SetPunctuator
(
    _Out_ ParseTree::PunctuatorLocation *PunctuatorSource,
    const Location &Start,
    _In_opt_ Token *Punctuator
)
{
    if (Punctuator)
    {
        PunctuatorSource->Line = Punctuator->m_StartLine - Start.m_lBegLine;
        PunctuatorSource->Column = Punctuator->m_StartColumn;
    }
}

