//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The ParserHelper Class is designed to create parse tree elements during
//  the semantic phase of the compiler. This is used to transform a SQL
//  statement into a function call. Creating the Parse tree is much easier
//  than the Symbol tree and the final IL code is the same.
//
//-------------------------------------------------------------------------------------------------

#pragma once

// $

static unsigned m_ParseHelperTempHiddenLambdaCounter = 0;

class ParseTreeService; // forward declaration

namespace ILTree
{
    struct ExpressionWithChildren;
    
};
class ParserHelper
{
public:
    ParserHelper(
        _In_ NorlsAllocator * TreeStorage,
        const Location TextSpan)
    {
        VSASSERT(TreeStorage != NULL, "required value");
        m_TreeStorage = TreeStorage;
        m_TextSpan = TextSpan;
    }

    ParserHelper(_In_ NorlsAllocator * TreeStorage)
    {
        VSASSERT(TreeStorage != NULL, "required value");
        m_TreeStorage = TreeStorage;
        m_TextSpan.SetLocationToHidden();
    }

    NorlsAllocator * GetTreeStorage()
    {
        return m_TreeStorage;
    }

    Location GetTextSpan()
    {
        return m_TextSpan;
    }

    void SetTextSpan(const Location & textSpan)
    {
        m_TextSpan = textSpan;
    }

    //
    // Expressions
    //

    ParseTree::CollectionInitializerExpression * 
    ParserHelper::CreateCollectionInitializer
    (
        ParseTree::NewExpression * pNewExpression, 
        ParseTree::PunctuatorLocation fromLocation, 
        ParseTree::BracedInitializerList * pInitializer
    );
    

    ParseTree::IntegralLiteralExpression * ParserHelper::CreateIntConst(int val);

    ParseTree::BooleanLiteralExpression * ParserHelper::CreateBoolConst(bool val);

    ParseTree::StringLiteralExpression * ParserHelper::CreateStringConst(_In_z_ STRING * Value);

    ParseTree::StringLiteralExpression * ParserHelper::CreateStringConst(
        _In_count_(Length + 1)_Pre_z_ WCHAR * Value,
        size_t Length);

    ParseTree::Expression * ParserHelper::CreateNothingConst();

    ParseTree::Expression * ParserHelper::CreateGlobalNameSpaceExpression();

    ParseTree::Name * ParserHelper::CreateGlobalNameSpaceName();

    ParseTree::Expression * ParserHelper::CreateMeReference();

    ParseTree::Expression * ParserHelper::CreateMyBaseReference();

    ParseTree::Expression * ParserHelper::CreateGroupReference(Location &TextSpan);

    ParseTree::Expression * ParserHelper::CreateMyClassReference();

    ParseTree::Expression * ParserHelper::CreateConversion(
        _In_opt_ ParseTree::Expression * value,
        _In_opt_ ParseTree::Type * targetType,
        ParseTree::Expression::Opcodes conversionKind = ParseTree::Expression::Conversion);

    ParseTree::NewArrayInitializerExpression * ParserHelper::CreateNewArray(
        _In_opt_ ParseTree::Type * BaseType);

    void ParserHelper::AddElementInitializer(
        _In_opt_ ParseTree::ArrayInitializerExpression * TheArray,
        _In_opt_ ParseTree::Expression * NewValue);

    ParseTree::NewObjectInitializerExpression * CreateNewObjectInitializer(
        _In_opt_ ParseTree::NewExpression * newExpression,
        ParseTree::BracedInitializerList * initialValues,
        _In_ const Location &textSpan);

    ParseTree::BracedInitializerList * CreateBracedInitializerList(
        _In_ ParseTree::InitializerList * initialValues,
        _In_ const Location &textSpan);

    ParseTree::NewObjectInitializerExpression * CreateAnonymousTypeInitializer(
        _In_ ParseTree::InitializerList * initialValues,
        _In_ const Location &textSpan,
        bool NoWithScope,
        bool QueryErrorMode);

    ParseTree::InitializerList * CreateInitializerListFromExpression(
        _In_ ParseTree::Expression * NewValue,
        bool FieldIsKey = true);

    ParseTree::InitializerList * AddExpressionInitializer(
        ParseTree::NewObjectInitializerExpression * objInitializer,
        _In_ ParseTree::Expression * NewValue,
        bool FieldIsKey = true);

    ParseTree::InitializerList * AppendInitializerList(
        ParseTree::NewObjectInitializerExpression * objInitializer,
        _In_ ParseTree::InitializerList * ListToAppend);

    void ParserHelper::ChangeToExpressionInitializer(
        ParseTree::InitializerList * NewListElement,
        _In_ ParseTree::Expression * NewValue,
        bool FieldIsKey = true);

    ParseTree::Expression * ParserHelper::CreateExpressionTreeNameExpression(
        Compiler * Compiler,
        _In_z_ STRING * Name);

    ParseTree::Expression * ParserHelper::CreateXmlNameExpression(
        Compiler * Compiler,
        _In_ const Location &TextSpan,
        unsigned NameCount,
        ...);

    ParseTree::Expression * ParserHelper::CreateXmlNameExpression(
        Compiler * Compiler,
        unsigned NameCount,
        ...);

    ParseTree::Expression * ParserHelper::CreateXmlNameExpression(
        Compiler * Compiler,
        _In_ const Location &TextSpan,
        unsigned NameCount,
        va_list NameList);

    ParseTree::NameExpression *
    ParserHelper::CreateNameExpression
    (
        _In_z_ STRING * Name
    );    

    ParseTree::NameExpression * ParserHelper::CreateNameExpression(
        _In_z_ STRING * Name,
        _In_ const Location &TextSpan);

    ParseTree::NameExpression * ParserHelper::CreateNameExpression(
        _In_ ParseTree::IdentifierDescriptor &Identifier);

    ParseTree::Expression * ParserHelper::CreateNameExpression(
        unsigned NameCount,
        ...);

    ParseTree::Expression * ParserHelper::CreateNameExpression(
        const Location &TextSpan,
        unsigned NameCount,
        ...);

    ParseTree::Expression * ParserHelper::CreateQualifiedNameExpression(
        ParseTree::Expression * Base,
        unsigned NameCount,
        ...);

    ParseTree::Expression * ParserHelper::CreateNameExpressionEx(
        _In_ const Location &TextSpan,
        unsigned NameCount,
        va_list NameList,
        _In_opt_ ParseTree::Expression * PrefixName = NULL);

    ParseTree::Expression * ParserHelper::CreateNameExpressionEx(
        unsigned NameCount,
        va_list NameList,
        _In_opt_ ParseTree::Expression * PrefixName = NULL);

    ParseTree::AlreadyBoundExpression * ParserHelper::CreateBoundExpression(
        ILTree::Expression * BoundExpression);

    ParseTree::UnaryExpression * ParserHelper::CreateUnaryExpression(
        ParseTree::Expression::Opcodes UnaryType,
        _In_ ParseTree::Expression * Operand) ;

    ParseTree::BinaryExpression * ParserHelper::CreateBinaryExpression(
        ParseTree::Expression::Opcodes UnaryType,
        _In_ ParseTree::Expression * Left,
        _In_ ParseTree::Expression * Right) ;

    ParseTree::NewExpression * ParserHelper::CreateNewObject(
        _In_opt_ ParseTree::Type * ObjectType,
        ParseTree::ArgumentList * ConstructorArgs = NULL);

    ParseTree::InitializerList * ParserHelper::CreateInitializerListFromExpression(
        ParseTree::IdentifierDescriptor * Name,
        ParseTree::Expression * NewValue,
        bool FieldIsKey = true);

    ParseTree::InitializerList * ParserHelper::CreateInitializerListFromExpression(
        ParseTree::IdentifierDescriptor * Name,
        ParseTree::Expression * NewValue,
        const Location &TextSpan,
        bool FieldIsKey = true);

    ParseTree::InitializerList * ParserHelper::AddAssignmentInitializer(
        _In_ ParseTree::ObjectInitializerExpression * TheObject,
        _In_ ParseTree::IdentifierDescriptor * Name,
        _In_ ParseTree::Expression * NewValue,
        bool FieldIsKey = true);

    ParseTree::InitializerList * ParserHelper::AddAssignmentInitializer(
        _In_ ParseTree::ObjectInitializerExpression * TheObject,
        _In_ ParseTree::IdentifierDescriptor * Name,
        _In_ ParseTree::Expression * NewValue,
        _In_ const Location &TextSpan,
        bool FieldIsKey = true);

    ParseTree::QueryOperatorCallExpression * ParserHelper::CreateQueryOperatorCall(
        ParseTree::CallOrIndexExpression * MethodCall);

    ParseTree::CrossJoinExpression * ParserHelper::CreateCrossJoinExpression(
        ParseTree::LinqExpression * Source,
        _In_opt_ ParseTree::LinqExpression * JoinTo,
        _In_ Location &TextSpan);

    ParseTree::FromItem * ParserHelper::CreateLetFromItem(
        _In_ ParseTree::VariableDeclaration * VariableDeclaration,
        _In_ ParseTree::Expression * Value,
        _In_ Location &TextSpan);

    ParseTree::FromExpression * ParserHelper::CreateFromExpression(
        _In_ ParseTree::FromItem * FromItem);

    ParseTree::SelectExpression * ParserHelper::CreateSelectExpression(
        ParseTree::LinqExpression * Source,
        _In_opt_ ParseTree::InitializerList * Projection,
        _In_ Location &TextSpan);

    ParseTree::QueryAggregateGroupExpression * ParserHelper::CreateQueryAggregateGroupExpression(
        _In_ ParseTree::LinqExpression * Group);

    ParseTree::CallOrIndexExpression * ParserHelper::CreateMethodCall(
        _In_ ParseTree::Expression * MethodName,
        ParseTree::ArgumentList * Args = NULL);

    ParseTree::CallOrIndexExpression * ParserHelper::CreateMethodCall(
        _In_ ParseTree::Expression * MethodName,
        _In_ ParseTree::ArgumentList * Args,
        _In_ const Location &TextSpan);

    ParseTree::CallOrIndexExpression * ParserHelper::CreateMethodCallOnAlreadyResolvedTarget(
        _In_ ParseTree::Expression * MethodName,
        _In_ ParseTree::ArgumentList * Args,
        _In_ const Location &TextSpan);

	ParseTree::CallOrIndexExpression * ParserHelper::CreateMethodCallFromAggregation(
        _In_opt_ ParseTree::Expression * BaseOfTarget,
        _In_ ParseTree::AggregationExpression * Aggregation);

    ParseTree::Expression * ParserHelper::CreateSyntaxErrorExpression(
        _In_ const Location &TextSpan);

    ParseTree::Expression * ParserHelper::CreateQualifiedExpression(
        _In_opt_ ParseTree::Expression * Base,
        _In_ ParseTree::Expression * Name,
        ParseTree::Expression::Opcodes Opcode = ParseTree::Expression::DotQualified);

    ParseTree::Expression * ParserHelper::CreateQualifiedExpression(
        _In_ ParseTree::Expression * Base,
        _In_ ParseTree::Expression * Name,
        _In_ const Location &TextSpan,
        ParseTree::Expression::Opcodes Opcode = ParseTree::Expression::DotQualified);

    ParseTree::GenericQualifiedExpression * ParserHelper::CreateGenericQualifiedExpression(
        _In_opt_ ParseTree::Expression * Base,
        ParseTree::TypeList * Args);

    ParseTree::LambdaExpression * CreateSingleLineLambdaExpression(
        ParseTree::ParameterList * Parameters,
        _In_opt_ ParseTree::Expression * Value,
        bool functionLambda);

    ParseTree::LambdaExpression * CreateSingleLineLambdaExpression(
        ParseTree::ParameterList * Parameters,
        _In_opt_ ParseTree::Expression * Value,
        _In_ const Location &TextSpan,
        bool functionLambda );

    //
    // Parameters
    //

    // Port SP1 CL 2929782 to VS10

    ParseTree::Parameter * CreateParameter(
        ParseTree::IdentifierDescriptor Name,
        ParseTree::Type * Type,
        bool IsQueryIterationVariable = false,
        bool IsQueryRecord = false);

    ParseTree::Parameter * CreateParameter(
        ParseTree::IdentifierDescriptor Name,
        ParseTree::Type * Type,
        _In_ const Location &TextSpan,
        bool IsQueryIterationVariable = false,
        bool IsQueryRecord = false,
        ParseTree::ParameterSpecifierList *Specifiers = NULL  // the structure we return has a pointer to this list
    );


    // ParameterSpecifierLists (includes things like ByVal, ByRef, Optional, ParamArray)

    ParseTree::ParameterSpecifierList *
    CreateParameterSpecifierList
    (    
        unsigned SpecifierCount,
        ... // a list of ParseTree::ParameterSpecifier* pointers, probably constructed by CreateParameterSpecifier
    );

    ParseTree::ParameterSpecifierList *
    CreateParameterSpecifierList
    (
        _In_ const Location & TextSpan,
        unsigned SpecifierCount,
        ... // a list of ParseTree::ParameterSpecifier* pointers, probably constructed by CreateParameterSpecifier
    );

    ParseTree::ParameterSpecifierList *
    CreateParameterSpecifierList
    (
        _In_ const Location & TextSpan,
        unsigned SpecifierCount,
        va_list SpecifierList // a list of ParseTree::ParameterSpecifier* pointers, probably constructed by CreateParameterSpecifier
    );

    ParseTree::ParameterSpecifierList *
    AddParameterSpecifier
    (
        ParseTree::ParameterSpecifierList * List, // must be the last element of the list
        ParseTree::ParameterSpecifier * Specifier,
        _In_ const Location & TextSpan
    );

    ParseTree::ParameterSpecifier *
    CreateParameterSpecifier
    (
        ParseTree::ParameterSpecifier::Specifiers SpecifierOpcode,
        _In_ const Location & TextSpan
    );

    // ParameterList

    ParseTree::ParameterList * CreateParameterList(
        unsigned ParameterCount,
        ...);

    ParseTree::ParameterList * CreateParameterList(
        _In_ const Location &TextSpan,
        unsigned ParameterCount,
        ...);

    ParseTree::ParameterList * CreateParameterList(
        _In_ const Location &TextSpan,
        unsigned ParameterCount,
        va_list ParameterExpressionList);

    ParseTree::ParameterList * AddParameter(
        ParseTree::ParameterList * List,
        ParseTree::Parameter * Parameter,
        _In_ const Location &TextSpan);

    //
    // Arguments
    //

    ParseTree::ArgumentList *
    CreateBoundArgList    
    (
        ILTree::ExpressionWithChildren * pArguments
    );


    ParseTree::ArgumentList * ParserHelper::CreateArgList(
        _In_ const Location &TextSpan,
        unsigned ArgCount,
        ...);

    ParseTree::ArgumentList * ParserHelper::CreateArgList(
        unsigned ArgCount,
        ...);

    ParseTree::ArgumentList * ParserHelper::CreateArgList(
        _In_ const Location &TextSpan,
        unsigned ArgCount,
        va_list ArgExpressionList);

    ParseTree::ArgumentList * ParserHelper::CreateArgList(ParseTree::Expression * Argument);

    ParseTree::ArgumentList * ParserHelper::AddArgument(
        ParseTree::ArgumentList * List,
        ParseTree::Expression * Argument);

    ParseTree::ArgumentList * ParserHelper::AddArgument(
        ParseTree::ArgumentList * List,
        ParseTree::Expression * Argument,
        _In_ const Location &TextSpan);

    //
    // Names
    //

    ParseTree::Name * ParserHelper::CreateName(
        unsigned NameCount,
        ...);

    ParseTree::Name * ParserHelper::CreateName(
        ParseTree::Name * BaseName,
        unsigned NameCount,
        ...);

    ParseTree::Name * ParserHelper::CreateNameWithTypeArguments(
        ParseTree::TypeList * TypeArgs,
        unsigned NameCount,
        ...);

    ParseTree::Name * ParserHelper::CreateNameEx(
        unsigned NameCount,
        va_list NameList,
        _In_opt_ ParseTree::Name * BaseName = NULL,
        ParseTree::TypeList * TypeArgs = NULL);

    //
    // Types
    //

    ParseTree::Type * ParserHelper::CreateType(ParseTree::Name * TypeName);

    ParseTree::TypeList * ParserHelper::CreateTypeList(
        unsigned TypeCount,
        ...);

    ParseTree::TypeList * ParserHelper::CreateTypeList(
        unsigned TypeCount,
        va_list TypeList);

    ParseTree::TypeList * ParserHelper::CreateTypeList(ParseTree::Type * Type);

    ParseTree::AlreadyBoundType * ParserHelper::CreateBoundType(Type * BoundType);

    ParseTree::AlreadyBoundType * ParserHelper::CreateBoundType(
        Type * BoundType,
        _In_ const Location &TextSpan);

    ParseTree::NullableType * ParserHelper::CreateNullableType(
        _In_ ParseTree::Type * ElementType,
        _In_ const Location &TextSpan);

    ParseTree::AlreadyBoundDelayCalculatedType * ParserHelper::CreateBoundDelayCalculatedType(
        BCSYM *(* DelayedCalculation)(void *),
        void * Parameter);

    ParseTree::ImplicitConversionExpression * ParserHelper::CreateImplicitConversionExpression(
        _In_ ParseTree::Expression * Value,
        Type * TargetType);

    //
    // VariableDeclarations
    //

    ParseTree::VariableDeclaration * ParserHelper::CreateVariableDeclaration(
        _In_ ParseTree::IdentifierDescriptor &Name);

    ParseTree::VariableDeclarationStatement * ParserHelper::CreateVariableDeclarationStatement(
        _In_z_ STRING * VariableName,
        _In_ ParseTree::Type * VariableType,
        _In_ ParseTree::Expression * InitialValue);

    //
    // IdentifierDescriptors
    //

    static
    ParseTree::IdentifierDescriptor ParserHelper::CreateIdentifierDescriptor(_In_z_ STRING * Name);

    static
    ParseTree::IdentifierDescriptor ParserHelper::CreateIdentifierDescriptor(
        _In_z_ STRING * Name,
        _In_ const Location &TextSpan);

    ParseTree::AddressOfExpression * ParserHelper::CreateAddressOf(
        ParseTree::Expression * MethodReference,
        bool UseLocationOfTargetMethodForStrict);

    ParseTree::Expression * ParserHelper::CreateBoundSymbol(Symbol * Symbol);
    ParseTree::Expression * ParserHelper::CreateBoundMemberSymbol(ParseTree::Expression * BaseReference, Symbol * Symbol);
    ParseTree::Expression * ParserHelper::CreateBoundSymbolOnNewInstance(ParseTree::Expression * BaseReference, Symbol * Symbol);

    // Statements


    ParseTree::StatementList * ParserHelper::AppendToStatementList(
        _Inout_ ParseTree::StatementList * LatestStatement,
        _In_opt_ ParseTree::Statement * CurrentStatement);

    ParseTree::StatementList * ParserHelper::CreateStatementList(
        _In_opt_ ParseTree::Statement * CurrentStatement);

    void ParserHelper::JoinStatementList(
        unsigned StatementCount,
        ...);

    void ParserHelper::JoinStatementList(
        unsigned StatementCount,
        va_list StatementList);

    ParseTree::StatementList * ParserHelper::CreateIfStatement(
        ParseTree::Expression * Condition,
        _Inout_opt_ ParseTree::StatementList * Then,
        ParseTree::StatementList * Else);

    ParseTree::HandlerStatement * ParserHelper::CreateAddHandler(
        _In_ ParseTree::Expression * EventReference,
        _In_opt_ ParseTree::Expression * DelegateReference);

    ParseTree::HandlerStatement * ParserHelper::CreateRemoveHandler(
        _In_ ParseTree::Expression * EventReference,
        _In_opt_ ParseTree::Expression * DelegateReference);

    ParseTree::AssignmentStatement * ParserHelper::CreateAssignment(
        _In_opt_ ParseTree::Expression * Target,
        _In_opt_ ParseTree::Expression * Source);

    static ParseTree::LambdaBodyStatement *
    ParserHelper::TryGetLambdaBodyFromParseTreeNode(
        _In_ ParseTree::ParseTreeNode *pParseTreeNode, 
        _In_ bool fIsStatement, // kind of pParseTreeNode: either ParseTree::Statement or ParseTree::Expression
        _In_opt_ Location *pTextSpan = NULL);

    static
    ParseTree::LambdaBodyStatement *
    GetLambdaBodyInStatement
    (
        _In_ ParseTree::Statement *pStatement, 
        _In_ Location *pTextSpan,
        _In_opt_ ParseTreeService *pParseTreeService = NULL);

    static ParseTree::Expression*
    ParserHelper::DigToInitializerValue(ParseTree::Initializer *pInitializer);

private:
    ParseTree::StatementList * ParserHelper::FindLastStatementAndPlaceInBlock(
        _In_opt_ ParseTree::StatementList * StartOfList,
        ParseTree::BlockStatement * ParentBlock);

    ParseTree::HandlerStatement * ParserHelper::CreateHandlerStatement(
        ParseTree::Statement::Opcodes OpCode,
        _In_ ParseTree::Expression * EventReference,
        _In_opt_ ParseTree::Expression * DelegateReference);

    ParseTree::StatementList * ParserHelper::JoinStatementList(
        _Inout_opt_ ParseTree::StatementList * Left,
        _Inout_opt_ ParseTree::StatementList * Right,
        ParseTree::BlockStatement * Parent = NULL);

private:
    NorlsAllocator * m_TreeStorage;
    Location m_TextSpan;
};
