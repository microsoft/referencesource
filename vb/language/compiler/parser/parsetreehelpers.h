//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements helpers for processing parsetree statements.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#if IDE 

#define COMPILER_ARG(argName) Compiler * argName = (Compiler *)GetCompilerPackage()

#else

#define COMPILER_ARG(argName) Compiler * argName

#endif


namespace ParseTreeHelpers
{
    bool GetWholeStatementLocation(
        ParseTree::Statement *pPreviousStatement,
        ParseTree::Statement *pCurrentStatement,
        Location *pLoc,
        bool lookAtBlockSpanForUnterminatedBlocks = false);

    ParseTree::Statement * GetStatementContainingLocation(
        ParseTree::BlockStatement *pCurrentBlock, // [In] Can be NULL.
        Location *pLoc, //[In] Cannot be NULL
        bool lookAtBlockSpanForUnterminatedBlocks = false,
        ParseTree::StatementList **ppCachedResult = NULL);

    bool IsAssignment(ParseTree::Statement *pStatement);

    bool IsMethodSignature(ParseTree::Statement *pStatement);

    bool IsMethodDeclaration(ParseTree::Statement *pStatement);

    bool IsTypeStatement(ParseTree::Statement *pStatement);

    bool IsVariableDeclarationStatement(ParseTree::Statement *pStatement);

    bool IsArrayType(ParseTree::Type *pType);

    bool IsIntrinsicType(ParseTree::Type *pType);

    bool IsEnumeratorStatement(ParseTree::Statement *pStatement);

    bool IsPartialType(ParseTree::Statement *pStatement);

    bool CanHaveReturnType(ParseTree::Statement *pStatement);

    bool IsConstructorDeclaration(ParseTree::Statement *pStatement);

    bool IsOperatorDeclaration(ParseTree::Statement *pStatement);

    bool IsEventDeclaration(ParseTree::Statement *pStatement);

    bool IsEventDeclaration(ParseTree::Statement::Opcodes Opcode);

    bool IsPropertyAccessor(ParseTree::Statement *pStatement);

    bool IsEventAccessor(ParseTree::Statement *pStatement);

    bool IsEventAccessor(ParseTree::Statement::Opcodes Opcode);

    bool IsDeclareContext(ParseTree::Statement::Opcodes Opcode);

    bool IsMethodBody(ParseTree::Statement *pStatement);

    bool IsWithinMethod(
        ParseTree::Statement *pStatement,
        ParseTree::MethodBodyStatement **ppContainingMethodBody = NULL);

    ParseTree::LambdaBodyStatement *IsWithinMultilineLambda(_In_ ParseTree::Statement *pStatement, _In_ bool fUltimateParent);

    bool IsConstantExpression(ParseTree::Expression::Opcodes Opcode);

    bool IsOperatorAssignment(ParseTree::Statement::Opcodes Opcode);

    bool IsFunctionDeclaration(ParseTree::Statement::Opcodes Opcode);

    bool IsProcedureDeclaration(ParseTree::Statement::Opcodes Opcode);

    void GetMethodKindPunctuator(
        ParseTree::Statement *pStatement,
        ParseTree::PunctuatorLocation *plocPunctuator);

    void GetMethodKindLocation
    (
        ParseTree::Statement *pStatement,
        Location *pLoc,
        COMPILER_ARG(pCompiler)
    ); //[In] Cannot be NULL

    void GetLocationAfterAttributesAndSpecifiers(
        ParseTree::Statement *pStatement,
        Location *pLoc); //[In] Cannot be NULL

    bool GetEndConstructFromOpcode
    (
        ParseTree::Statement::Opcodes opcode,
        StringBuffer *psbEndConstruct,
        COMPILER_ARG(pCompiler)
    );

    tokens
    MapOpcodeToTokenType
    (
        ParseTree::Statement::Opcodes opcode,
        _In_opt_ ParseTree::LambdaExpression *pLambda=NULL
    );

    tokens MapOpcodeToEndTokenType(ParseTree::Statement::Opcodes opcode);

    STRING * MapSpecifierOpcodeToSTRING(
        ParseTree::Specifier::Specifiers opcode,
        Compiler *pCompiler);

    long MapSpecifierOpcodeToIndex(ParseTree::Specifier::Specifiers opcode);

    ParseTree::Specifier::Specifiers MapAccessToSpecifierOpcode(ACCESS access);

    STRING * MapParamSpecifierOpcodeToSTRING(
        ParseTree::ParameterSpecifier::Specifiers opcode,
        Compiler *pCompiler);

    long MapParamSpecifierOpcodeToIndex(
        ParseTree::ParameterSpecifier::Specifiers opcode);

    tokens MapToTokenType(ParseTree::Specifier::Specifiers specifierOpcode);

    tokens MapToTokenType(
        ParseTree::ForeignMethodDeclarationStatement::StringModes stringMode);

    ParseTree::MethodSignatureStatement * GetEventDeclaration(
        ParseTree::Statement *pStatement);

    ParseTree::ParameterList * GetParameters(
        ParseTree::Statement *pStatement);

    ParseTree::GenericParameterList * GetGenericParameters(
        ParseTree::Statement *pStatement);

    ParseTree::GenericArguments * GetGenericArguments(
        ParseTree::Type *pPossibleGenericType);

    ParseTree::GenericArguments * GetGenericArguments(
        ParseTree::Name *pPossibleGenericName);

    int GetGenericArgumentCountInCallTarget(
        ParseTree::CallOrIndexExpression *pCallExpression);

    int GetGenericArgumentCount(
        ParseTree::GenericArguments *pGenericArguments);

    int GetArity(ParseTree::Statement *pStatement);

    int GetArity(ParseTree::Name *pName);

    int GetTypeArity(ParseTree::Type *pType);

    ParseTree::Name * GetTypeName(ParseTree::Type *pPossibleNamedType);

    bool DoesTypeNameReferToGenericArguments(ParseTree::Type *pType);

    ParseTree::TypeListStatement * FindFirstImplements(
        ParseTree::TypeStatement *pType);

    ParseTree::TypeListStatement * FindFirstInherits(
        ParseTree::TypeStatement *pType);

    ParseTree::TypeListStatement * FindFirstMatchingTypeList(
        ParseTree::TypeStatement *pType,
        ParseTree::Statement::Opcodes Opcode);

    ParseTree::TypeListStatement * FindNextMatchingTypeList(
        ParseTree::Statement *pStatement,
        ParseTree::Statement::Opcodes Opcode);

    ParseTree::MethodDefinitionStatement * FindFirstPropertySet(
        ParseTree::PropertyStatement *pProperty);

    bool HasPropertyGet(ParseTree::PropertyStatement *pProperty);

    bool HasPropertySet(ParseTree::PropertyStatement *pProperty);

    ParseTree::SpecifierList * GetSpecifierList(ParseTree::Statement *pStatement);

    ParseTree::Statement * GetPreviousNonEmptyInBlock(ParseTree::Statement *pStatement);

    ParseTree::StatementList * SkipOverPlacebos(ParseTree::StatementList *pStatementList);

    bool GetEnumIntializerReplaceLocation(
        ParseTree::EnumeratorStatement *pEnumerator,
        Location *pLoc);

    bool GetParameterIntializerReplaceLocation(
        ParseTree::Parameter *pParameter,
        Location *pLoc);

    bool GetParameterSpecifiersReplaceLocation(
        ParseTree::Parameter *pParameter,
        Location *pLoc);

    bool GetVariableIntializerReplaceLocation(
        ParseTree::VariableDeclaration *pVariable,
        Location *pLoc);

    tokens GetOperatorTokenFromOperatorAssignment(ParseTree::Statement::Opcodes opcode);

    bool IsExprPartOfIdentifier(ParseTree::Expression *pExpression);

    ParseTree::StatementList * GetLastStatementInBlock(ParseTree::BlockStatement *pBlock);

    bool GetPunctuatorLocation(
        const Location *pBaseLocation,
        const ParseTree::PunctuatorLocation *pPunctuator,
        Location *pRealLocation,
        // if you want the location to be set to the entire punctuator, pass in the length
        int PunctuatorLength = 0,
        bool fAllowColumn0 = false      // allow a PunctLoc of Line=0, col = 0 to be valid
        );

    bool GetStringFromIdentifierDescriptor(
        ParseTree::IdentifierDescriptor *pIdentifier,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb);

    bool GetStringFromGenericArguments
    (
        ParseTree::GenericArguments *pGenericArguments,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    bool GetStringFromGenericParameters
    (
        ParseTree::GenericParameterList *pGenericParameters,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    bool GetStringFromName
    (
        ParseTree::Name *pName,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    bool GetStringFromNameWithoutQualifier
    (
        ParseTree::Name *pName,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    bool GetStringFromArrayType
    (
        ParseTree::ArrayType *pArrayType,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    bool GetStringFromType
    (
        ParseTree::Type *pType,
        bool IncludeGenericParameters,
        bool IncludeArrayBounds,
        StringBuffer *pSb,
        COMPILER_ARG(pCompiler)
    );

    void GetFullyQualifiedName
    (
        CompilerProject *pCompilerProject,
        ParseTree::Statement *pStatement,
        bool IncludeGenericParameters,
        StringBuffer *pSbName,
        COMPILER_ARG(pCompiler)
    );

    ParseTree::Statement * GetStatementOnOrAfterLocation(
        ParseTree::BlockStatement *pCurrentBlock,
        Location *pLoc);

    ParseTree::Statement * GetStatementOnOrBeforeLocation(
        ParseTree::BlockStatement *pCurrentBlock,
        Location *pLoc);

    STRING * ExtractDeclarationName(
        Compiler *pCompiler,
        ParseTree::Statement *pStatement,
        Location *pLoc,
        Location *pLocFoundName,
        bool fIncludeBrackets = true);

    STRING * ExtractDeclarationNameFromGenericParams(
        Compiler *pCompiler,
        ParseTree::GenericParameterList *pList,
        Location *pLoc,
        Location *pLocFoundName,
        bool fIncludeBrackets = true);

    STRING * ExtractDeclarationNameFromParams(
        Compiler *pCompiler,
        ParseTree::ParameterList *pList,
        Location *pLoc,
        Location *pLocFoundName,
        bool fIncludeBrackets = true);

    STRING * ExtractDeclarationFromID(
        Compiler *pCompiler,
        ParseTree::IdentifierDescriptor *pID,
        Location *pLoc,
        Location *pLocFoundName,
        bool fIncludeBrackets = true);

    bool IsInferredDeclaration(
        ParseTree::Statement *pStatement,
        SourceFile           *pFile,
        Location             *pLoc);

    bool IsPrefixDotExpression(
        ParseTree::Expression *pExpression);

    bool GetAttributesAndSpecifiers(
        ParseTree::Statement *pStatement,
        ParseTree::AttributeSpecifierList **ppAttributes = NULL,
        Location *plocBeforeAttributes = NULL,
        Location *plocAfterAttributes = NULL,
        ParseTree::SpecifierList **ppSpecifiers = NULL,
        Location *plocBeforeSpecifiers = NULL,
        Location *plocAfterSpecifiers = NULL,
        Location *plocAfterAttributesAndSpecifiers = NULL);

    bool GetSpecifiers(
        ParseTree::Statement *pStatement,
        DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,
        Location *plocBefore = NULL,
        Location *plocAfter = NULL);

    bool HasSpecifier(
        DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,
        ParseTree::Specifier::Specifiers Opcode,
        ULONG *pulFoundSpecifier1 = NULL,
        ULONG *pulFoundSpecifier2 = NULL);

    bool GetFirstMatchingSpecifier(
        DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,
        ParseTree::Specifier::Specifiers Opcode,
        ULONG *pulFoundSpecifier = NULL);

    bool GetAllMatchingSpecifiers(
        DynamicArray<ParseTree::Specifier *> *pdaSpecifiers,
        ParseTree::Specifier::Specifiers Opcode,
        DynamicArray<ULONG> *pdaFoundSpecifiers);

    void GetLocationOfObjectInitializer(
        ParseTree::VariableDeclaration *pVariableDeclaration,
        Location *pLoc);

    void GetLocationOfObjectInitializer(
        _In_ ParseTree::AutoPropertyStatement *pAutoPropertyStatement,
        _Out_ Location *pLoc);

    ParseTree::Statement * GetRegionParent(ParseTree::Statement *pStatement);

    ParseTree::Initializer * DigThroughDeferredInitializer(
        ParseTree::Initializer *pInitializer);

    ParseTree::Expression * DigThroughDeferredExpression(ParseTree::Expression *pExpression);

    bool CanBePartialMethodImplementation(ParseTree::Statement *pStatement);

    bool IsLocationAfterFunctionOrSubKeywordInLambda(
        Location *pLocation,
        ParseTree::LambdaExpression *pLambdaExpression);

    bool IsLocationAfterLeftParenInLambda(
        Location *pLocation,
        ParseTree::LambdaExpression *pLambdaExpression);

    // Checks to see if the provided location is after the right parenthesis of the
    // parameter list in the lambda expression
    bool IsLocationAfterLambdaParameters(
        _In_ Location *pLocation,
        _In_ ParseTree::LambdaExpression *pLambdaExpression,
        _Out_opt_ Location *pLocRParen = NULL    // in case the user wants the location of the RParen
        );

    unsigned GetParameterCount(ParseTree::ParameterList *pParameterList);

    void GetStartLocationOfQueryExpression(
        ParseTree::Expression *pExpression,
        Location *plocStart);

    bool ContainsAssemblyOrModuleAttributes(ParseTree::AttributeStatement *pStatement);

    tokens MapOptionKindToTokenType(ParseTree::Statement *pOption);

    class VariableDeclaratorIterator
    {
    public:
        VariableDeclaratorIterator(ParseTree::VariableDeclarationStatement *pStatement) :
            m_pStatement(pStatement),
            m_pCurrentDeclaration(NULL),
            m_pCurrentDeclarator(NULL)
        {
            Reset();
        }

        ParseTree::Declarator *Next(ParseTree::VariableDeclaration **ppCurrentDeclaration = NULL);
        void Reset();

    private:
        ParseTree::VariableDeclarationStatement *m_pStatement;
        ParseTree::VariableDeclarationList *m_pCurrentDeclaration;
        ParseTree::DeclaratorList *m_pCurrentDeclarator;
    };
};
