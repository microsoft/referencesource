//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to move a module from NoState to Declared state.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class Declared
{
    // forward decls
    struct NamespaceNameList;
    typedef unsigned __int64 TypeResolutionFlags;

public:

    //
    // Main entrypoints.
    //

    // Bring a source file to declared state
    static void MakeDeclared( CompilerHost *CompilerHost,
                              NorlsAllocator *pnra,
                              SourceFile *pfile,
                              LineMarkerTable* pLineMarkerTable,
                              ParseTree::FileBlockStatement *ptreeModule,
                              ErrorTable *pErrorTable,
                              SymbolList *psymlist );

    static void MakeLocalsFromParams(Symbols *SymbolFactory,
                                     BCSYM_Param *ParamList,
                                     SymbolList *ListOfBuiltLocals,
                                     BCSYM_Variable **HeadOfBuiltLocals = NULL);

    // Build a set of locals (and return variable) from a procedure's parameters
    static void MakeLocalsFromParams( Compiler *pCompiler,
                                      NorlsAllocator *pnra,
                                      BCSYM_Proc* pproc,
                                      ErrorTable *ErrorLog,
                                      bool GenerateReturnTypeLocal,
                                      BCSYM_Variable **ppvarReturn,
                                      BCSYM_Hash** ppbchashLocal );

    static BCSYM_Variable *MakeReturnLocal(Compiler *CompilerInstance,
                                                 NorlsAllocator *Allocator,
                                                 BCSYM_Proc *OwningProc,
                                                 SymbolList *ListOfBuiltLocals);

    static void MakeLocalFromCatch( CompilerHost *CompilerHost,
                                    NorlsAllocator *Allocator,
                                    ParseTree::CatchStatement *Catch,
                                    CompilationCaches *IDECompilationCaches,
                                    ErrorTable *ErrorTable,
                                    BCSYM_Class *Container,
                                    SourceFile *pSourceFile,
                                    BCSYM_Proc *ContainingProc,
                                    BCSYM_Hash *Lookup,
                                    Compiler *CompilerInstance,
                                    TypeResolutionFlags Flags,
                                    BCSYM_NamedRoot **CatchSymbol = NULL);

    static void MakeReturnTypeParamAndAttachAttributesFromTrees
    (
        CompilerHost *CompilerHost,
        NorlsAllocator *Allocator, 
        SourceFile *File, 
        CompilationCaches *IDECompilationCaches,
        ErrorTable *ErrorLog, 
        Procedure *pOwningProc,
        ParseTree::AttributeSpecifierList *pReturnTypeAttributes, 
        BCSYM *pReturnTypeSymbol,
        BCSYM_Container *pContainer,
        BCSYM_Param **ppReturnTypeParameter 
    );
    
    // Build a set of locals from a procedure's declarations
    static void MakeLocalsFromTrees
    (
        CompilerHost *CompilerHost,
        NorlsAllocator *Allocator, // [in] allocator to use
        CompilationCaches *IDECompilationCaches,
        ErrorTable *ErrorLog, // [in] error table to put errors in
        BCSYM_Class *OwningClass, // [in] class that owns the proc that declares these symbols
        SourceFile *pSourceFile, 
        BCSYM_Proc *OwningProc, // [in] proc in which locals are defined
        BCSYM_Hash *LocalVarHash, // [in] built symbols for locals are put in here
        ParseTree::VariableDeclarationStatement *VarDeclarationTree, // [in] the DIM tree
        bool CalledFromParseTreeService, // [in] whether we are being called in a situation where we may be in cs_nostate
        TypeResolutionFlags Flags, // [in] specifically for propagating the flag which enables Obsolete checking
        BCSYM_NamedRoot **CatchVariable, // [out] When provided, set to point to the local created for the catch
        bool lambdaMember = false
    );

    // Build a set of locals from a declaration member of declaration statement
    static void MakeLocalsFromDeclarationTrees( CompilerHost *CompilerHost,
                                    NorlsAllocator *Allocator,
                                    CompilationCaches *IDECompilationCaches,
                                    ErrorTable *ErrorLog,
                                    //BCSYM_Class *OwningClass,
                                    SourceFile *pSourceFile,
                                    BCSYM_Proc *OwningProc,
                                    BCSYM_Hash *LocalVarHash,
                                    ParseTree::VariableDeclarationStatement *VarDeclarationTree,
                                    ParseTree::VariableDeclaration  *Declaration,
                                    bool CalledFromParseTreeService,
                                    TypeResolutionFlags Flags,
                                    DECLFLAGS SpecifierFlagsForAll,   // [in] the specifiers for all the declaration
                                    BCSYM_NamedRoot **CatchVariable = NULL,
                                    bool lambdaMember = false);

    // Build a set of parameters -- needed for lambda expressions
    static void MakeParamsFromLambda( CompilerHost *CompilerHost,
                                      NorlsAllocator *Allocator,
                                      SourceFile *File,
                                      CompilationCaches *IDECompilationCaches,
                                      ErrorTable *ErrorLog,
                                      ParseTree::ParameterList *ParameterList,
                                      DECLFLAGS    MethodFlags,
                                      BCSYM_Proc *OwningProcedure, // [in] the built parameter symbols
                                      BCSYM_Hash   *LocalVarHash,
                                      TypeResolutionFlags TypeResolutionFlags,
                                      BCSYM_Param **OutParamSymbols,
                                      BCSYM_Param ** OutParamSymbolsLast);

    // Resolve a type
    static BCSYM *ResolveType( NorlsAllocator *Allocator,
                              SourceFile *pfile,
                              CompilationCaches *IDECompilationCaches,
                              ErrorTable *ErrorLog,
                              CompilerHost *CompilerHost,
                              ParseTree::Type *TypeTree,
                              BCSYM_NamedRoot *Context,
                              BCSYM_Hash *TargetHash,
                              TypeResolutionFlags Flags,
                              bool IsAppliedAttributeContext = false);

    // Helper to map parse tree flags to decl. flags
    static DECLFLAGS MapSpecifierFlags( ParseTree::SpecifierList * );

    // Helper to build the property symbol that will be used to do dynamic event hookup for WithEvents variables
    static BCSYM_Property *BuildWithEventsProperty
    (
        BCSYM_Variable *WithEventVar, 
        BCSYM_GenericBinding *GenericBindingContext, 
        Symbols *SymbolAllocator, 
        bool MarkAsOverrides, 
        bool InStdModule, 
        Compiler *CompilerInstance, 
        SymbolList *OwningSymbolList, 
        SymbolList *OwningUnBindableSymbolList
    );

    // Helper to build synthetic symbols for auto properties and link them to the auto property symbol
    void BuildAutoPropertyMethods
    (
        _Inout_ BCSYM_Property *pAutoProperty,
        _In_opt_ ParseTree::AttributeSpecifierList *pReturnTypeAttributes,
        _In_ Symbols *pSymbolAllocator,
        bool definedInStdModule,
        _In_ Compiler *pCompilerInstance,
        _Inout_ SymbolList *pOwningSymbolList,
        _Inout_ SymbolList *pOwningUnBindableSymbolList
    );

    // Helper to synthesize constructors both in declared and in bindable during handles hookup
    //
    DECLARE_ENUM(SyntheticConstructorType)
        Instance,
        Shared,
        MustInherit
    END_ENUM(SyntheticConstructorType)

    static BCSYM_Proc* BuildSyntheticConstructor(Symbols *SymbolAllocator,
                                                 Compiler *CompilerInstance,
                                                 SymbolList *OwningSymbolList,
                                                 SyntheticConstructorTypeEnum type);

    static BCSYM_Proc* BuildSyntheticConstructor(Symbols *SymbolAllocator,
                                                 Compiler *CompilerInstance,
                                                 BCSYM_Param *Parameters,
                                                 SymbolList *OwningSymbolList,
                                                 SyntheticConstructorTypeEnum type);

    // Helper to build project level imports
    //
    static ImportedTarget* BuildProjectLevelImport(ParseTree::ImportDirective *ImportDirective,
                                                   NorlsAllocator *Allocator,
                                                   ErrorTable *ErrorLog,
                                                   CompilerProject *Project);

    static BCSYM_Param * CloneParameters
    (
        BCSYM_Param *ParameterSymbol,
        bool IgnoreByValParameters,
        BCSYM_Param **LastParameterSymbol,
        Symbols * pSymbols
    );

private:

    //
    // Constructor
    //
    Declared
    (
        NorlsAllocator *pnra,
        SourceFile *pfile,
        LineMarkerTable* pLineMarkerTable,
        ErrorTable *pErrorTable,
        CompilerHost *pCompilerHost,
        CompilationCaches *pCompilationCaches);

    //
    // Type name resolution
    //
    BCSYM *MakeType( ParseTree::Type *pTypeTree, ParseTree::IdentifierDescriptor *pIdentifier, BCSYM_NamedRoot *Context, BCSYM_NamedType **TypeList, DECLFLAGS MethodFlags = 0, bool IsAppliedAttributeContext = false, bool IsForControlVarDeclaration = false);
    void FillQualifiedNameArray( unsigned NumDotDelimitedNames, ParseTree::Name *pNameTree, _Out_cap_(NumDotDelimitedNames) STRING **ppNameArray, NameTypeArguments ***ArgumentsArray, BCSYM_NamedRoot *Context, BCSYM_NamedType **TypeList, bool *isGlobalNameSpaceBased );
    void FillGenericArgumentsArray( unsigned ArgumentsIndex, unsigned ArgumentsArrayLength, NameTypeArguments ***ArgumentsArray, ParseTree::Name *NamedTree, ParseTree::GenericArguments *ArgumentsTree, BCSYM_NamedRoot *Context, BCSYM_NamedType **TypeList );
    BCSYM_NamedType *MakeNamedType( ParseTree::Name *pNameTree, BCSYM_NamedRoot *Context, BCSYM_NamedType **TypeList, bool IsTHISType = false, bool IsAppliedAttributeContext = false, ParseTree::Type* ArgumentNullable = NULL );
    void ResolveTypeLists(BCSYM_Hash *phashLocals, TypeResolutionFlags);
    BCSYM *Declared::MakeArrayType(ParseTree::Type *ArrayInfo, BCSYM *ElementType);

    //
    // Makes a copy of a parameter list. This is used by MakeProperty to clone parameters from the Property to the Set/Get methods
    //
    BCSYM_Param *CloneParameters( BCSYM_Param *ParameterSymbol, BCSYM_Param **LastParameterSymbol );
    BCSYM_Param *CloneParameters( BCSYM_Param *ParameterSymbol, bool IgnoreByValParameters, BCSYM_Param **LastParameterSymbol );
    //
    // Routines to map parse flags to decl flags
    //
    DECLFLAGS MapVarDeclFlags( ParseTree::Declarator *VarDecl, ParseTree::NewVariableDeclaration *DeclaredAsNew );
    DECLFLAGS MapProcDeclFlags( ParseTree::MethodSignatureStatement *MethodTree );
    DECLFLAGS MapDeclareDeclFlags( ParseTree::ForeignMethodDeclarationStatement *DllDeclareTree  );

    //
    // Routines to do the real work of declared
    //
    void DoMakeDeclared(ParseTree::FileBlockStatement *FileTree,
                        SymbolList *OwningSymbolList);

    void DoMakeLocalsFromTrees( ParseTree::VariableDeclarationStatement *DeclarationTree,
                                BCSYM_Hash *Hash,
                                BCSYM_Proc *Procedure,
                                BCSYM_NamedRoot **FirstCreatedLocal,
                                bool CalledFromParseTreeService,
                                bool PerfomObsoleteCheck,
                                bool lambdaMember = false);

    //
    // Routines to build namespaces
    //
    void BuildNamespaceList( ParseTree::Name *NameTree, NamespaceNameList **NamespacesToMake, bool isDefaultNamespace );
    void PrependToNamespaceList( NamespaceNameList **List, _In_z_ STRING * Name, Location * pLocation );

    DECLARE_ENUM(NamespaceNesting)
        Project,    // Project level namespace - root
        File,       // Top level namespace in the file
        Nested      // Namespace nested in another namespace
    END_ENUM(NamespaceNesting)

    void DriveMakeNamespace( ParseTree::BlockStatement *NamespaceTree, NamespaceNestingEnum Nesting );
    void MakeNamespace( NamespaceNameList *NamespaceList, ParseTree::BlockStatement *NamespaceTree, NamespaceNestingEnum Nesting, Location *Loc );
    void FindDupesInNamespace( BCSYM_Namespace *Namespace );
    BCSYM_NamedRoot *LookupInLinkedNamespaces( BCSYM_NamedRoot *SymbolToLookFor, BCSYM_Namespace *Namespace );
    void MakeOption( ParseTree::Statement *pOptionStmtTree, bool &SeenOptionCompare, bool &SeenOptionStrict, bool &SeenOptionExplicit, bool &SeenOptionInfer );
    void MakeImports( ParseTree::ImportsStatement * pImportsTree, ImportedTarget **ImportsList );
    ImportedTarget* BuildOneImportedTarget( ParseTree::ImportDirective *ImportDirective, CompilerProject *Project, bool isProjectLevelImport );

    //
    // Routines to deal with attributes
    //
    void ProcessAttributeList( ParseTree::AttributeSpecifierList *Attributes, BCSYM *Symbol, BCSYM_NamedRoot *Context = NULL );

    //
    // Routines to deal with XMLDoc comments
    //
    void ExtractXMLDocComments( ParseTree::StatementList *StatementListTree, BCSYM_NamedRoot *NamedElement );

    //
    // Adds an XML comment block to the list of comment blocks seen.
    //
    void AddCommentBlock( ParseTree::CommentBlockStatement *AddedCommentBlock );

    //
    // Removes an XML comment block from the list of comment blocks seen.
    //
    void RemoveCommentBlock( ParseTree::CommentBlockStatement *RemovedCommentBlock );

    // Report all errors for XMLDoc comment blocks that could not be associated with any valid language elements.
    void ReportAllOrphanedXMLDocCommentErrors();

    //
    // Routines to build types
    //
    void MakeClassChildren(ParseTree::BlockStatement *ModuleTree,
                          DECLFLAGS ClassFlags,
                          bool &HasMustOverrideMethod,
                          bool &SeenConstructor,
                          bool &SeenSharedConstructor,
                          bool &NeedSharedConstructor,
                          bool HasGenericParent,
                          BCSYM_Class *OwningClass,
                          SymbolList *OwningSymbolList,
                          SymbolList *BackingFieldsForStaticsList,
                          BCSYM ** BaseClass,
                          BCSYM_Implements ** ImplementsList,
                          BCSYM_Property **DefaultProperty);
    void MakeInherits(ParseTree::TypeListStatement *pInheritsStmtTree, BCSYM_Class *ContainingClass, BCSYM **ppInheritsSymbol );
    void MakeClassImplements(ParseTree::TypeListStatement *pImplementsTree, BCSYM_Class *ContainingClass, BCSYM_Implements **ppImplementsSymbol );
    void MakeClass(ParseTree::StatementList *StatementListTree, bool IsNested, bool BuildStdModule, bool DefinedInInterface, bool HasGenericParent, DECLFLAGS ContainerDeclFlags, SymbolList *OwningSymbolList );
    void MakeStructure( ParseTree::StatementList *StatementListTree, bool IsNested, bool DefinedInInterface, bool HasGenericParent, DECLFLAGS ContainerFlags, SymbolList *OwningSymbolList );
    void MakeEnum( ParseTree::StatementList *StatementListTree, bool IsNested, bool DefinedInStdModule, bool HasGenericParent, DECLFLAGS ContainerFlags, SymbolList *OwningSymbolList );
    void MakeInterface( ParseTree::StatementList *StatementListTree, bool NestedInClass, bool DefinedAtNamespaceLevel, bool HasGenericParent, SymbolList *OwningSymbolList, DECLFLAGS ContainerDeclFlags );
    void MakeDelegateMethods(ParseTree::MethodSignatureStatement *DelegateTree,
                             SymbolList *DelegateChildren,
                             bool ForAnEvent,
                             BCSYM_Param *ParameterSymbolList,
                             DECLFLAGS DeclFlags);
    void MakeDelegate( ParseTree::StatementList *StatementListTree,
                       bool IsNested,
                       DECLFLAGS ContainerFlags,
                       SymbolList *OwningSymbolList,
                       BCSYM_Param *ParameterSymbolList,
                       bool ForAnEvent = false,
                       bool UseAttributes = true,
                       _In_opt_z_ STRING * OptionalDelegateName = NULL,
                       BCSYM_Class ** BuiltDelegateSymbol = NULL,
                       bool DefinedInInterface = false );
    void MakeGenericParameters( ParseTree::GenericParameterList *GenericParamsTree,
                                BCSYM_GenericParam **GenericParams,
                                BCSYM_NamedRoot *Context,
                                _In_opt_z_ STRING *ContextName = NULL,
                                bool ContextIsFunction = false );

    // Builds the THIS type - it is the same as the ClassOrInterface for non-generic types,
    // but is the open binding corresponding to the ClassOrInterface when the ClassOrInterface
    // is either generic or is nested in a generic type.
    //
    BCSYM_NamedType *MakeNamedTypeForTHISType( BCSYM_NamedRoot *Context );

    //
    // Routines to build type members
    //
    BCSYM_ImplementsList *MakeImplementsList( ParseTree::NameList * pImplementsList, BCSYM_Proc *Procedure);
    BCSYM_HandlesList *MakeHandlesList( ParseTree::NameList * pHandlesList, BCSYM_MethodImpl *HandlingMethod, DECLFLAGS ClassFlags );
    BCSYM_HandlesList* MakeHandlesClause( ParseTree::Name *HandlesEntry, BCSYM_MethodImpl *HandlingMethod, DECLFLAGS ClassFlags );
    void MakeConstantExpression( ParseTree::Expression *ExpressionTree, bool ExpressionPreEvaluated,
                                bool IsExplictlyTyped, BCSYM_NamedRoot *Context, BCSYM **TypeOfConstant, BCSYM_Expression **BuiltConstantExprSymbol );
    void MakeParams(_In_opt_z_ STRING *ReturnValueName, ParseTree::ParameterList *ParameterList, DECLFLAGS MethodFlags, BCSYM_Proc *OwningProcedure, DECLFLAGS    ContainerFlags, BCSYM_Param **OutParamSymbols, BCSYM_Param ** OutParamSymbolsLast, BCSYM *EmplicitParamTypeSymbol, BCSYM_Param *ExistingParameterSymbolList);

    void ConstantVarSemantics( ParseTree::VariableDeclarationStatement * VariableDeclarationStatement, ParseTree::Declarator *Declarator, ParseTree::VariableDeclaration *VariableDeclaration,
                               bool ForLocal, DECLFLAGS *SpecifiersForThisVar, BCSYM_Variable *VariableSymbol );
    void MakeMemberVar( ParseTree::StatementList *StatementListTree, DECLFLAGS ClassFlags, bool &NeedSharedConstructor, SymbolList *OwningSymbolList );
    void MakeOperator( ParseTree::StatementList *StatementListTree, BCSYM_Class *OwningClass, DECLFLAGS ClassFlags, bool &NeedSharedConstructor, SymbolList *OwningSymbolList, SymbolList *BackingStaticFieldsList );
    void MakeMethodImpl( ParseTree::StatementList *StatementListTree, DECLFLAGS ClassFlags, bool DefinedInInterface, bool &HasMustOverrideMethod, bool &SeenConstructor, bool &SeenSharedConstructor, bool &NeedSharedConstructor, SymbolList *OwningSymbolList, SymbolList *BackingStaticFieldsList );

    void MakeEvent
    (
        ParseTree::StatementList *StatementListTree,
        bool DefinedInInterface,
        DECLFLAGS ContainerDeclFlags,
        SymbolList *OwningSymbolList,
        SymbolList *BackingStaticFieldsList,
        bool &NeedSharedConstructor
    );

    void GenerateEventHookupSymbols
    (
        ParseTree::StatementList *StatementListTree,
        DECLFLAGS EventDeclFlags,
        bool DefinedInInterface,
        BCSYM_EventDecl *Event,
        SymbolList *OwningSymbolList,
        bool DefinedInStdModule,
        BCSYM_Param *ParameterSymbolList,
        DECLFLAGS ContainerDeclFlags,
        SymbolList *BackingStaticFieldsList,
        bool &NeedSharedConstructor
    );

    void
    BuildEventDelegate
    (
        ParseTree::StatementList *EventStatementListTree,
        BCSYM_EventDecl *Event,
        DECLFLAGS EventDeclFlags,
        SymbolList *OwningSymbolList,
        bool DefinedInStdModule,
        BCSYM_Param *DelegateParamSymbolList
    );

    void
    BuildEventField
    (
        BCSYM_EventDecl *Event,
        DECLFLAGS EventDeclFlags,
        SymbolList *OwningSymbolList
    );

    void
    BuildEventMethods
    (
        ParseTree::Statement *EventTree,
        BCSYM_EventDecl *Event,
        DECLFLAGS EventDeclFlags,
        SymbolList *OwningSymbolList,
        bool DefinedInInterface,
        DECLFLAGS ContainerDeclFlags,
        SymbolList *BackingStaticFieldsList,
        bool &NeedSharedConstructor
    );

    void
    BuildSyntheticEventMethod
    (
        SyntheticKind MethodKind,
        BCSYM_EventDecl *Event,
        DECLFLAGS EventMethodFlags,
        SymbolList *OwningSymbolList
    );

    void
    BuildUserDefinedEventMethods
    (
        ParseTree::BlockEventDeclarationStatement *EventTree,
        BCSYM_EventDecl *Event,
        DECLFLAGS EventMethodFlags,
        SymbolList *OwningSymbolList,
        DECLFLAGS ContainerDeclFlags,
        SymbolList *BackingStaticFieldsList,
        bool &NeedSharedConstructor
    );

    bool
    ValidateEventAddRemoveMethodParams
    (
        ParseTree::MethodDefinitionStatement *EventMethodTree
    );

    void BuildPropertySet( ParseTree::PropertyStatement *PropertyTree, ParseTree::StatementList * CurrentPropertyMember, DECLFLAGS PropertyFlags, bool DefinedInInterface, BCSYM_Proc *PropertySet, DECLFLAGS ContainerFlags, SymbolList *OwningSymbolList, BCSYM **PropertyReturnTypeSymbol, BCSYM_Param **ParameterSymbolList );
    
    BCSYM_Property *MakeProperty
    ( 
        ParseTree::StatementList *StatementListTree, 
        DECLFLAGS ClassDeclFlags, 
        bool DefinedInInterface, 
        SymbolList *OwningSymbolList, 
        bool &NeedSharedConstructor, 
        SymbolList *BackingStaticFieldsList 
    );
    
    void MakeAutoProperty
    (
        _In_ ParseTree::AutoPropertyStatement * pAutoPropertyTree,
        _Inout_ BCSYM_Property *pAutoPropertySymbol,
        _In_ Location * pAutoPropertyLocation,
        bool definedInStruct,
        bool definedInStdModule,
        bool &needSharedConstructor,
        DECLFLAGS autoPropertyFlags,
        _Inout_ SymbolList *pOwningSymbolList // symbol list to add to
    );

    void MakeExpandedProperty
    (
        _In_ ParseTree::PropertyStatement *pPropertyTree, 
        _Inout_ BCSYM_Property *pPropertySymbol,       
        _In_ Location *pPropertyLocation,
        bool definedInStruct, 
        bool definedInInterface,
        bool definedInStdModule,
        DECLFLAGS propertyFlags,
        DECLFLAGS ErrorCausingFlags,
        DECLFLAGS classDeclFlags,
        _Inout_ BCSYM *pPropertyReturnTypeSymbol,
        _Inout_ BCSYM_Param *pParameterSymbolList, 
        _Inout_ SymbolList *pOwningSymbolList,
        bool &needSharedConstructor,
        _Inout_ SymbolList *pBackingStaticFieldsList
    );
    
    void MakeDllDeclared( ParseTree::StatementList *StatementListTree, DECLFLAGS ClassFlags, SymbolList *OwningSymbolList );
    void MakeProc( ParseTree::Statement::Opcodes MethodType,
                   ParseTree::IdentifierDescriptor *MethodNameDescriptor,
                   _In_opt_z_ STRING *MethodName,
                   ParseTree::ParameterList *ParameterList,
                   ParseTree::Type *ReturnTypeTree,
                   ParseTree::AttributeSpecifierList *ReturnTypeAttributes,
                   ParseTree::Expression * LibraryExpression,
                   ParseTree::Expression * AliasExpression,
                   ParseTree::GenericParameterList *GenericParameters,
                   DECLFLAGS methodFlags,
                   bool DefinedInInterface,
                   CodeBlockLocation *CodeBlock,
                   CodeBlockLocation *ProcBlock,
                   BCSYM_Proc *Procedure,
                   SymbolList *OwningSymbolList,
                   bool *SeenConstructor,
                   bool *SeenSharedConstructor,
                   ParseTree::SpecifierList *Specifiers,
                   DECLFLAGS ContainerFlags,
                   BCSYM **PropertyReturnTypeSymbol,
                   BCSYM_Param **OutParameterSymbolList,
                   UserDefinedOperators Operator = OperatorUNDEF);

    void BuildReturnTypeParamAndAttachAttributes
    (
        _Out_ BCSYM_Param **ppReturnTypeParam,
        _Inout_ BCSYM_Proc *pProc,
        _Inout_ ParseTree::AttributeSpecifierList * pReturnTypeAttributes,
        _In_ BCSYM *pReturnType
    );

    //
    // Synthesized type members
    //
    void CreateSynthesizedSymbols(SymbolList * psymlistChildren, DECLFLAGS DeclFlags, bool SeenConstructor, bool SeenSharedConstructor, bool NeedSharedConstructor, bool NeedMain, SymbolList *OwningSymbolList );
    void CreateStartAndTerminateSymbols( DECLFLAGS DeclFlags, bool SeenConstructor, bool SeenSharedConstructor, bool NeedSharedConstructor, bool NeedMain, SymbolList *OwningSymbolList );
    
    void MakeBackingField
    ( 
        _Inout_ BCSYM_Member *pMember, bool inStdModule, 
        _Inout_ SymbolList *pOwningSymbolList,
        _In_opt_ ParseTree::AutoPropertyInitialization *pAutoPropertyDeclaration = NULL,
        _In_opt_ ParseTree::AttributeSpecifierList *pAutoPropertyReturnTypeAttributes = NULL
    );

    BCSYM_Expression* MakeAutoPropertyInitializerExpression
    (
        _In_ ParseTree::AutoPropertyInitialization *pAutoPropertyDeclaration,
        _In_ BCSYM_Property *pPropertySymbol
    );

    //
    // Error routines
    //
    void __cdecl ReportError( ERRID errid, Location *loc, ... );
    void ReportDeclFlagError( ERRID errid, DECLFLAGS DeclarationFlags, ParseTree::SpecifierList *Specifiers, Location *AlternateLocation = NULL );
    void ReportDeclFlagComboError( DECLFLAGS DeclarationFlags, ParseTree::SpecifierList *Specifiers );
    void ReportDeclFlagComboError( DECLFLAGS DeclarationFlags, ParseTree::SpecifierList *Specifiers, DECLFLAGS SpecifierOnWhichToReportError );
    WCHAR * GetDeclFlagInfo( DECLFLAGS * DeclFlag, ParseTree::SpecifierList *Specifiers = NULL, Location **LocationInfo = NULL );
    Location * GetLocationOfSpecifier( DECLFLAGS DeclFlag, ParseTree::SpecifierList *Specifiers );

    //
    // Routines to make locals and general variables
    //
    void CreateBackingFieldsForStatics( ParseTree::StatementList *StaticLocalDeclarations, BCSYM_Proc *OwningProc, DECLFLAGS ClassFlags, bool &NeedSharedConstructor, SymbolList *OwningSymbolList );
    void 
    MakeLocals
    (
        ParseTree::VariableDeclarationStatement *DeclarationTree, 
        BCSYM_Proc *OwningProcedure, 
        BCSYM_Hash *LocalsHash, 
        BCSYM_NamedRoot **CatchSymbol, 
        bool CalledFromParseTreeService, 
        bool PerformObsoleteCheck,
        bool lambdaMember=false
    );
    
    void 
    MakeLocalsFromDeclaration
    (
        ParseTree::VariableDeclarationStatement *DeclarationTree, 
        ParseTree::VariableDeclaration *Declaration, 
        BCSYM_Proc *OwningProcedure, 
        BCSYM_Hash *LocalsHash, 
        BCSYM_NamedRoot **CatchSymbol, 
        bool CalledFromParseTreeService, 
        DECLFLAGS SpecifierFlagsForAll,
        bool PerformObsoleteCheck,
        bool lambdaMember = false
    );

    void 
    MakeAliasToStaticVar
    ( 
        ParseTree::Declarator *VariableTree, 
        ParseTree::VariableDeclaration *Declaration, 
        DECLFLAGS SpecifiersForThisVar, 
        BCSYM_Proc *OwningProcedure, 
        SymbolList *OwningSymbolList,
        bool PerformObsoleteCheck
    );
    void MakeVariable( ParseTree::AttributeSpecifierList *Attributes, ParseTree::Declarator * VariableTree, ParseTree::VariableDeclaration * Declaration, DECLFLAGS varFlags,
                       VARIABLEKIND VarKind, bool IsLocal, BCSYM_Variable *Variable, SymbolList *OwningSymbolList );
    BCSYM_Expression *
    Declared::BuildNewDeferredInitializerExpression
    (
        _Inout_ BCSYM_NamedRoot* pContext,
        _In_ ParseTree::DeferredText *pDeferredInitializerText,
        _In_opt_ ParseTree::ObjectInitializerList *pObjectInitializer,
        ParseTree::ParenthesizedArgumentList arguments,
        _In_opt_ ParseTree::BracedInitializerList * pCollectionInitializer,
        Location textSpan,
        _In_opt_ ParseTree::PunctuatorLocation *FromLoc
    );

    // Helpers.

    bool DetermineIfBadName( ParseTree::Name *Name );
    bool CheckForTypeCharacter( ParseTree::Name *Name );

    // Helpers to make declared container based
    void SetContainerContext(BCSYM_Container *Container);
    void PopContainerContext();
    void AddContainerContextToSymbolList();

    BCSYM_Container* GetCurrentContainerContext();
    BCSYM_Expression** GetExpressionListHead();
    void SetExpressionListHead(BCSYM_Expression *ExpressionListHead);
    BCSYM_Expression** GetExpressionListTail();
    void SetExpressionListTail(BCSYM_Expression *ExpressionListTail);

    BCSYM_NamedType** GetTypeList();
    SymbolList* GetUnBindableChildrenSymbolList();

    // This method returns the passed in Location buffer if there is an implements clause else returns NULL
    //
    Location *GetImplementsLocation
    (
        Location *ImplementorLocation,                              // [in] Location of the implementing member
        ParseTree::PunctuatorLocation *ImplementsRelativeLocation,  // [in] location of the implements clause
                                                                    //          relative to the implementing member
        Location *LocationBuffer                                    // [out] The location buffer to fill with the
                                                                    //          location of the implements clause.
    );

    // This method reports an error saying that a module member cannot implement other members.
    //
    void ReportModuleMemberCannotImplementError
    (
        Location *ImplementorLocation,                              // [in] Location of the implementing member
        ParseTree::PunctuatorLocation *ImplementsRelativeLocation   // [in] location of the implements clause
                                                                    //          relative to the implementing member
    );

    STRING *PushNamespaceContext(_In_ NamespaceNameList *NamespaceList);

    //========================================================================
    // Data members.
    //========================================================================

    Location m_AsNewLocation; // tracks AsNew location info for last variable declared AsNew
    Compiler *m_CompilerInstance; // Compiler Context
    CompilerHost *m_CompilerHost; // CompilerHost
    CompilationCaches *m_CompilationCaches;
    Symbols m_SymbolAllocator; // Allocator
    NorlsAllocator m_ScratchAllocator; // Scratch allocator
    SourceFile *m_SourceFile; // Source file providing the parse trees
    ErrorTable *m_ErrorTable; // Table of errors.
    STRING *m_CurrentNamespaceContext; // Cache of the current namespace context
    SymbolList *m_FileLevelSymbolList;   // Any type declarations are added to this list

    // By the end of Declared, this tree will have all XMLDoc comments that didn't get associated with
    // any language element and don't have any errors reported. We use this tree to report errors for each
    // one of these XMLDoc comment blocks.
    RedBlackTreeT<Location, LocationOperations> m_OrphanedXMLDocCommentBlocks;

    // list of all types and expression created outside container by this instance
    // of declared. This is needed when resolving locals. We don't have a container
    // context and also don't want to tie the named types and expressions created
    // in this scenario to the container.
    //
    BCSYM_NamedType *m_TypeList;
    BCSYM_Expression *m_ExpressionListHead; // The expression list must be kept in-order.
    BCSYM_Expression *m_ExpressionListTail;

    struct NamespaceNameList // Used to keep track of the current namespace
    {
        STRING *const Name;
        Location *const pLocation;
        NamespaceNameList *const Next;
        bool Global;                    // The namespace is rooted in the global (as opposed to Project root) namespace

        NamespaceNameList(_In_z_ STRING *name, Location *location, NamespaceNameList *next)
            : Name(name),
              pLocation(location),
              Next(next),
              Global(false)
        {
        }
    };

    // This structure is used to store container specific information required only during
    // declared.
    //
    struct ContainerContextInfo
    {
        BCSYM_Container *const m_Container;
        SymbolList m_UnBindableChildren;
        BCSYM_Expression *m_ExpressionListTail;

        ContainerContextInfo(BCSYM_Container *Container)
            : m_Container(Container),
              m_ExpressionListTail(NULL)
        {
        }
    };

    Stack<ContainerContextInfo *> *m_ContainerContextStack;
};
