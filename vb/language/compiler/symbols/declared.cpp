//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Builds the symbol table from the parse trees
//  
//  












#include "StdAfx.h"

// Keep this define - fixed one bug where the string and the string used to figure this length 
// were out of sync.
#define PLUS_ONE_STRING WIDE( "+1" )

//-------------------------------------------------------------------------------------------------
//
// Constructor
//
Declared::Declared(
    NorlsAllocator *Allocator,
    SourceFile *Source,
    LineMarkerTable* pLineMarkerTable,
    ErrorTable *pErrorTable,
    CompilerHost *pCompilerHost,
    CompilationCaches *pCompilationCaches) :
    m_SymbolAllocator( pCompilerHost->GetCompiler(), Allocator, pLineMarkerTable ),
    m_ErrorTable( pErrorTable ),
    m_CompilerInstance( pCompilerHost->GetCompiler() ),
    m_CompilerHost( pCompilerHost ),
    m_CompilationCaches(pCompilationCaches),
    m_SourceFile( Source ),
    m_ScratchAllocator( NORLSLOC ),
    m_OrphanedXMLDocCommentBlocks( Allocator )
{
    m_CurrentNamespaceContext = STRING_CONST(m_CompilerInstance, EmptyString);
    m_FileLevelSymbolList = NULL;

    m_TypeList = NULL;
    m_ExpressionListHead = NULL;
    m_ExpressionListTail = NULL;

    m_ContainerContextStack = NULL;

    if (Source && Source->SymbolStorage() == Allocator)
    {
        m_SymbolAllocator.SetGenericBindingCache(Source->GetCurrentGenericBindingCache());
    }
}

/*****************************************************************************
;MakeDeclared

Because Declared isn't really set up as a class, what people do is call this
static function that creates an instance of Declared and then invokes Declared
to do its thing.
*****************************************************************************/
void Declared::MakeDeclared
(
    CompilerHost *CompilerHost, // [in]  instance of the compiler to use for allocations
    NorlsAllocator *Allocator, // [in]  the allocator to use for declarations
    SourceFile *SourceFile, // [in]  the file that is going to declared - contains the trees
    LineMarkerTable* LineMarkers,
    ParseTree::FileBlockStatement *FileTree,  // [in] the parse tree for the file
    ErrorTable *ErrorLog, // [in]  the error table to log errors in
    SymbolList *FileSymbolList  // [out] a list of all file level symbols
)
{
    if ( !FileTree ) return; // Nothing to do

    // Run the decls.
    VSASSERT( FileSymbolList->GetCount() == 0, "We haven't built any symbols yet - the File's symbol list should be empty." );

    // This is a static function - create declared and build the symbols
    Declared declared( Allocator, SourceFile, LineMarkers, ErrorLog, CompilerHost, NULL );
    declared.DoMakeDeclared( FileTree, FileSymbolList );
}

/*****************************************************************************
;MakeReturnLocal

Make a return local for a function
*****************************************************************************/
BCSYM_Variable *Declared::MakeReturnLocal
(
    Compiler        *CompilerInstance,
    NorlsAllocator  *Allocator,
    BCSYM_Proc      *OwningProc,
    SymbolList      *ListOfBuiltLocals
)
{
    Symbols SymbolFactory( CompilerInstance, Allocator, NULL );
    BCSYM_Variable *BuiltVar = NULL;

    STRING *VariableName = OwningProc->GetName();

    if ( OwningProc->IsPropertyGet())
    {
        VSASSERT( wcslen( CLS_PROPERTY_GET_PREFIX ) == 4, "prefix changed!" ); // Microsoft: CONSDER - use the define for 4 (could probably get rid of assert in that case)
        VariableName = CompilerInstance->AddString( VariableName + 4 );
    }
    else if (OwningProc->IsPropertySet())
    {
        VSASSERT( wcslen( CLS_PROPERTY_SET_PREFIX ) == 4, "prefix changed!" ); // Microsoft: CONSDER - use the define for 4 (could probably get rid of assert in that case)
        VSASSERT( wcslen( CLS_PROPERTY_PUT_PREFIX ) == 4, "prefix changed!" );
        VariableName = CompilerInstance->AddString( VariableName + 4 );
    }
    else if (OwningProc->IsUserDefinedOperatorMethod())
    {
        // Operators don't need function return variables.  However, because we cannot guarantee all paths return a value,
        // we need a variable that codegen can use.  Create a temporary variable using the CLS name for the operator.
        VariableName = CompilerInstance->ConcatStrings(VBTemporaryPrefix, VariableName);
    }

    // Create the symbol.
    BuiltVar = SymbolFactory.AllocVariable( OwningProc->HasLocation(), false );

    // Set its information.
    SymbolFactory.GetVariable( OwningProc->GetLocation(),
                               VariableName,
                               VariableName,
                               DECLF_Dim | DECLF_Public,
                               VAR_FunctionResult,
                               OwningProc->GetRawType(),
                               NULL,
                               ListOfBuiltLocals,
                               BuiltVar );

    if (OwningProc->IsUserDefinedOperatorMethod())
    {
        // Operators don't need function return variables.  However, because we cannot guarantee all paths return a value,
        // we need a variable that codegen can use.  Make sure to mark this variable as a temporary.
        //
        BuiltVar->SetIsTemporary();
    }

    return BuiltVar;
}

void Declared::MakeLocalsFromParams
(
    Symbols *SymbolFactory,
    BCSYM_Param *ParamList,
    SymbolList *ListOfBuiltLocals,
    BCSYM_Variable **HeadOfBuiltLocals
)
{
    ThrowIfNull(SymbolFactory);
    ThrowIfNull(ListOfBuiltLocals);

    BCSYM_Variable *BuiltVar = NULL;

    //
    // Walk the list of parameters, turning them into locals.
    //

    for ( BCSYM_Param *CurrentParam = ParamList; CurrentParam; CurrentParam = CurrentParam->GetNext())
    {
        // Create the symbol.
        BuiltVar = SymbolFactory->AllocVariable( CurrentParam->HasLocation(), false /* no initializer */);

        Location *ParamLoc = CurrentParam->HasLocation() ? CurrentParam->GetLocation() : NULL;
        STRING *ParamName = CurrentParam->GetName();

        // Set its information.
        SymbolFactory->GetVariable( ParamLoc,
                                    ParamName,
                                    ParamName,
                                    DECLF_Dim | DECLF_Public,
                                    VAR_Param,
                                    CurrentParam->GetRawType(),
                                    NULL, // don't send initializer info
                                    ListOfBuiltLocals,
                                    BuiltVar );
    }

    if (HeadOfBuiltLocals)
    {
        *HeadOfBuiltLocals = BuiltVar;
    }
}

/*****************************************************************************
;MakeLocalsFromParams

Process the trees the locals variables in a function and produce symbols for them
*****************************************************************************/
void Declared::MakeLocalsFromParams
(
    Compiler        *CompilerInstance, // [in] instance of the compiler
    NorlsAllocator  *Allocator, // [in] symbols are allocated from here
    BCSYM_Proc      *OwningProc, // [in] the procedure that owns the parameters
    ErrorTable      *ErrorLog, // [in] put errors in here
    bool            GenerateReturnTypeLocal, // [in] Generate Return Type Local Variable
    BCSYM_Variable  **HeadOfBuiltLocals, // [out] the head of the list of built locals
    BCSYM_Hash      **HashOfLocals // [out] locals for the params put in this hash
)
{
    SymbolList ListOfBuiltLocals;
    Symbols SymbolFactory( CompilerInstance, Allocator, NULL );
    BCSYM_Variable *BuiltVar = NULL;

    MakeLocalsFromParams(&SymbolFactory, OwningProc->GetFirstParam(), &ListOfBuiltLocals, &BuiltVar);

    // Create the node for the return type.

    if (GenerateReturnTypeLocal && OwningProc->GetRawType())
    {
        BuiltVar = MakeReturnLocal(CompilerInstance, Allocator, OwningProc, &ListOfBuiltLocals);
    }

    //
    // Add the locals to the local hash table.
    //

    *HashOfLocals = SymbolFactory.GetHashTable( NULL /* no hash name */, OwningProc /* proc owns the hash */, true /* set parents */, 16, &ListOfBuiltLocals );

    if ( HeadOfBuiltLocals )
    {
        *HeadOfBuiltLocals = BuiltVar;
    }
}

/**************************************************************************************************
;MakeParamsFromLambda

Build parameter symbols for a lambda expression.
***************************************************************************************************/
void Declared::MakeParamsFromLambda
(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator, // [in] allocator to use
    SourceFile *File, // [in] file the expression is in
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorLog, // [in] error table to put errors in
    ParseTree::ParameterList *ParameterList, // [in] the list of parameters to build
    DECLFLAGS    MethodFlags,                  // [in] the flags for the method that owns these parameters
    BCSYM_Proc *OwningProcedure, // [in] the built parameter symbols
    BCSYM_Hash *LocalVarHash, // [in] built symbols for locals are put in here
    TypeResolutionFlags TypeResolutionFlags, // [in] specifically for propagating the flag which enables Obsolete checking
    BCSYM_Param **OutParamSymbols, // [out] the built parameter symbols
    BCSYM_Param **OutParamSymbolsLast // [out] tail of the built parameter symbols (may be null)
)
{
    // Create the instance of Declared that will do the work
    Declared declared( Allocator, File, NULL, ErrorLog, CompilerHost, IDECompilationCaches);

    // Run the decls.
    declared.MakeParams(
        NULL,
        ParameterList,
        MethodFlags | DECLF_LambdaArguments,
        OwningProcedure,
        0,
        OutParamSymbols,
        OutParamSymbolsLast,
        NULL,
        NULL);

    // Resolve type types
    declared.ResolveTypeLists( LocalVarHash, TypeResolutionFlags | TypeResolveDontMarkContainingProcAsBad);

}

/**************************************************************************************************
;MakeLocalsFromTrees

When semantics interprets a block, it builds a tree of local variable declarations which it passes
to this function to build BCSYM_Variables for them.  Catch variables are also built in here.

The argument pSourceFile will only be used if OwingProc is NULL. Also if pSourceFile is NULL, then OwningProc must 
provide a SourceFile.
***************************************************************************************************/
void Declared::MakeLocalsFromTrees
(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator, // [in] allocator to use
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorLog, // [in] error table to put errors in
    BCSYM_Class *OwningClass, // [in] class that owns the proc that declares these symbols
    SourceFile *pSourceFile, // [in] proc in which locals are defined
    BCSYM_Proc *OwningProc, // [in] proc in which locals are defined
    BCSYM_Hash *LocalVarHash, // [in] built symbols for locals are put in here
    ParseTree::VariableDeclarationStatement *VarDeclarationTree, // [in] the DIM tree
    bool CalledFromParseTreeService, // [in] whether we are being called in a situation where we may be in cs_nostate
    TypeResolutionFlags Flags, // [in] specifically for propagating the flag which enables Obsolete checking
    BCSYM_NamedRoot **CatchVariable, // [out] When provided, set to point to the local created for the catch
    bool lambdaMember
)
{

    // The following assertion ensures that if pSourceFile is NULL then OwningProc must provide one.
    // One of the primary cases this can happen is when OwningProc comes from Metadata.  Typically this 
    // happens when an expression is evaluated in the EE
    AssertIfFalse(pSourceFile != NULL || (OwningProc != NULL && OwningProc->GetSourceFile() != NULL));

    // Create the instance of Declared that will do the work
    Declared declared( Allocator, 
                       OwningProc && OwningProc->GetSourceFile() ? OwningProc->GetSourceFile() : pSourceFile,
                       NULL, 
                       ErrorLog, 
                       CompilerHost, 
                       IDECompilationCaches );

    // Run the decls.
    declared.DoMakeLocalsFromTrees( 
        VarDeclarationTree, 
        LocalVarHash, 
        OwningProc, 
        CatchVariable, 
        CalledFromParseTreeService, 
        Flags & TypeResolvePerformObsoleteChecks, // Always check obsolete for both var and type or neither of them.
        lambdaMember);

    // Evaluate them.
    declared.ResolveTypeLists( LocalVarHash, Flags );
}

// static member that makes the locals from a single declaration instead of a full declaration statement
// The argument pSourceFile will only be used if OwingProc is NULL. Also if pSourceFile is NULL, then OwningProc must 
// provide a SourceFile.
void Declared::MakeLocalsFromDeclarationTrees
(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator, // [in] allocator to use
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorLog, // [in] error table to put errors in
    // BCSYM_Class *OwningClass, // [in] class that owns the proc that declares these symbols //Note: actually the class is not used.
    SourceFile *pSourceFile,
    BCSYM_Proc *OwningProc, // [in] proc in which locals are defined
    BCSYM_Hash *LocalVarHash, // [in] built symbols for locals are put in here
    ParseTree::VariableDeclarationStatement *VarDeclarationTree, // [in] the DIM tree that contains all the declrations
    ParseTree::VariableDeclaration  *Declaration,               // [in] the declaration from the tree for which the local are created
    bool CalledFromParseTreeService, // [in] whether we are being called in a situation where we may be in cs_nostate
    TypeResolutionFlags Flags, // [in] specifically for propagating the flag which enables Obsolete checking
    DECLFLAGS SpecifierFlagsForAll,  // [in] the specifiers for all the declaration
    BCSYM_NamedRoot **CatchVariable, // [out] When provided, set to point to the local created for the catch
    bool lambdaMember // [in] Is this local a member of a lambda, FALSE by default.
)
{

    // The following assertion ensures that if pSourceFile is NULL then OwningProc must provide one. 
    // One of the primary cases this can happen is when OwningProc comes from Metadata.  Typically this 
    // happens when an expression is evaluated in the EE
    AssertIfFalse(pSourceFile != NULL || (OwningProc != NULL && OwningProc->GetSourceFile() != NULL));


    //DECLFLAGS SpecifierFlagsForAll = MapSpecifierFlags( VarDeclarationTree->Specifiers );
    // Create the instance of Declared that will do the work
    Declared declared( Allocator, 
                       OwningProc && OwningProc->GetSourceFile() ? OwningProc->GetSourceFile() : pSourceFile,
                       NULL, 
                       ErrorLog, 
                       CompilerHost, 
                       IDECompilationCaches);

    // Run the decls.
    declared.MakeLocalsFromDeclaration( 
        VarDeclarationTree, 
        Declaration, 
        OwningProc, 
        LocalVarHash,  
        CatchVariable, 
        CalledFromParseTreeService, 
        SpecifierFlagsForAll,
        Flags & TypeResolvePerformObsoleteChecks, // Always check obsolete for both var and type or neither of them.
        lambdaMember);

    // Evaluate them.
    declared.ResolveTypeLists( LocalVarHash, Flags );
}

/*****************************************************************************
;MakeLocalFromCatch

Build a local variable symbol from a Catch tree
*****************************************************************************/
void Declared::MakeLocalFromCatch(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator, // [in] allocator to use for the symbol
    ParseTree::CatchStatement *Catch, // [in] the catch statement that we are generating a local for
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorTable, // [in] put errors in here
    BCSYM_Class *Container, // [in] container for the catch variable
    SourceFile *pSourceFile, // [in] 
    BCSYM_Proc *ContainingProc, // [in] the proc that owns the local
    BCSYM_Hash *Lookup, // [in]
    Compiler *CompilerInstance, // [in] instance of the compiler
    TypeResolutionFlags Flags, // [in] specifically for propagating the flag which enables Obsolete checking
    BCSYM_NamedRoot **CatchSymbol // [optional out] the built catch symbol goes here if supplied
)
{
    ParseTree::Declarator *CatchDeclarator = (ParseTree::Declarator *) Allocator->Alloc( sizeof( ParseTree::Declarator ));
    CatchDeclarator->Name = Catch->Name;
    CatchDeclarator->TextSpan = Catch->Name.TextSpan;

    ParseTree::DeclaratorList *Declarators = (ParseTree::DeclaratorList*) Allocator->Alloc( sizeof( ParseTree::DeclaratorList ));
    Declarators->Element = CatchDeclarator;

    ParseTree::VariableDeclaration *CatchDeclaration = (ParseTree::VariableDeclaration *) Allocator->Alloc( sizeof( ParseTree::VariableDeclaration ));
    CatchDeclaration->Opcode = ParseTree::VariableDeclaration::NoInitializer;
    CatchDeclaration->Type = Catch->Type;
    CatchDeclaration->Variables = Declarators;
    CatchDeclaration->TextSpan = Catch->TextSpan;

    ParseTree::VariableDeclarationList *Declarations = (ParseTree::VariableDeclarationList*) Allocator->Alloc( sizeof( ParseTree::VariableDeclarationList ));
    Declarations->Element = CatchDeclaration;

    ParseTree::VariableDeclarationStatement *CatchDeclarationStatement = (ParseTree::VariableDeclarationStatement *) Allocator->Alloc( sizeof( ParseTree::VariableDeclarationStatement ));
    CatchDeclarationStatement->Opcode = ParseTree::Statement::VariableDeclaration;
    CatchDeclarationStatement->TextSpan = Catch->TextSpan;
    CatchDeclarationStatement->Declarations = Declarations;

    Declared::MakeLocalsFromTrees(
        CompilerHost,
        Allocator,
        IDECompilationCaches,
        ErrorTable,
        Container,
        pSourceFile,
        ContainingProc,
        Lookup,
        CatchDeclarationStatement,
        false, // not being called from ParseTreeService
        Flags,
        CatchSymbol );
}

/**************************************************************************************************
;MakeReturnTypeParamAndAttachAttributesFromTrees

Build parameter symbols for a lambda expression.
***************************************************************************************************/
void Declared::MakeReturnTypeParamAndAttachAttributesFromTrees
(
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator, // [in] allocator to use
    SourceFile *File, // [in] file the expression is in
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorLog, // [in] error table to put errors in
    Procedure *pOwningProc, //[in] procedure that owns lambda
    ParseTree::AttributeSpecifierList *pReturnTypeAttributes, //[in] attributes being attached to param
    BCSYM *pReturnTypeSymbol,//[in] symbol for return type
    BCSYM_Container *pContainer, //[in] symbol for lambda's container
    BCSYM_Param **ppReturnTypeParameter //[inout] parameter we are attaching attributes to.
)
{
    // Create the instance of Declared that will do the work
    Declared declared( Allocator, File, NULL, ErrorLog, CompilerHost, IDECompilationCaches);

    declared.SetContainerContext(pContainer);

    // Run the decls.
    declared.BuildReturnTypeParamAndAttachAttributes( ppReturnTypeParameter, 
                                                      pOwningProc, 
                                                      pReturnTypeAttributes, 
                                                      pReturnTypeSymbol);

}

/*****************************************************************************
 ;ResolveType
*****************************************************************************/
BCSYM *Declared::ResolveType
(
    NorlsAllocator *Allocator,
    SourceFile *Source,
    CompilationCaches *IDECompilationCaches,
    ErrorTable *ErrorLog,
    CompilerHost *CompilerHost,
    ParseTree::Type *TypeTree,
    BCSYM_NamedRoot *Context,
    BCSYM_Hash *TargetHash,
    TypeResolutionFlags Flags,
    bool IsAppliedAttributeContext
)
{
    BCSYM *Result = NULL;
    Declared declared( Allocator, Source, NULL, ErrorLog, CompilerHost, IDECompilationCaches);

    Result = declared.MakeType( TypeTree, NULL, Context, declared.GetTypeList(), 0, IsAppliedAttributeContext );

    declared.ResolveTypeLists( TargetHash, Flags );

    return Result;
}

/*========================================================================

Type name resolution routines

========================================================================*/

/*****************************************************************************
;MakeType

Build a type symbol from a parse tree.  Either a name tree or a type tree may be provided.
If you provide a name tree, the type is determined by looking for type characters in the name
and is variant if none are found.  If you provide a type tree, the type is derived from that.
*****************************************************************************/
BCSYM * // The built type
Declared::MakeType
(
 ParseTree::Type *TypeTree, // [in] the type for the symbol ( may be null in which case we look at TypeIdentifier to build the type )
 ParseTree::IdentifierDescriptor *TypeIdentifier, // [in] the name of the type ( may be null in which case we look at TypeTree to build the type )
 BCSYM_NamedRoot *Context, // [in] the context in which the type is defined
 BCSYM_NamedType **TypeList, // [in] the type list to add named types to
 DECLFLAGS DeclFlags, // [optional-in] if context->IsProc(), this can be used to determine if a FUNCTION is being built vs. a PROC
 //                if context->Isvar(), this can be used to determine if variable is a const
 bool IsAppliedAttributeContext, // [optional-in] Indicates whether this type is being created in the context of an applied attribute.
 bool IsForControlVarDeclaration // [optional-in] Indicates whether this type is being created for a For loop control var.
 )
{
    BCSYM *BuiltTypeSymbol = NULL;

    // If there is no explicit type, look at the name for a type character ( e.g. a$, a% ) to figure out what the type should be.
    // Failing all else, the thing will be made Object
    if ( !TypeTree )
    {
        if ( !TypeIdentifier || TypeIdentifier->IsBad ) // Since we weren't sent a type, you must send a name or the thing is horked
        {
            return m_SymbolAllocator.GetGenericBadNamedRoot();
        }

        // We've got a name but no type tree - see if we can deduce the type from the name from a typechar ( a$ indicating a string instance )
        typeChars TypeCharacter = TypeIdentifier->TypeCharacter;
        if ( TypeCharacter != chType_NONE ) // Is there a typechar?
        {
            BuiltTypeSymbol = m_CompilerHost->GetFXSymbolProvider()->GetType( VtypeOfTypechar( TypeCharacter ));

            if (TypeIdentifier->IsNullable)
            {
                ParserHelper ph(m_SymbolAllocator.GetNorlsAllocator());

                BuiltTypeSymbol =
                    MakeNamedType
                    (
                        NULL,
                        Context,
                        TypeList,
                        false/*IsTHISType*/,
                        false /*IsAppliedAttributeContext*/,
                        ph.CreateBoundType
                        (
                            BuiltTypeSymbol,
                            TypeIdentifier->TextSpan
                        )
                    );
            }
        }
        else if (!(DeclFlags & DECLF_LambdaArguments))// the type is object
        {
            // Generate warning or error here when As is missing only when the declaration doesn't have an initializer.  When it has one
            // the error or warning will be reported in Semantics after the initializer is evaluated.
            // When Option Infer is on or we are using this for Lambda expressions , we should allow this scenario.
            // We used to ignore constants when inferring types using Option Infer, but we are removing that
            // exception as per bug 400490.
            VSASSERT(!Context || m_SourceFile, "Please inform the VB compiler that you have a repro for DevDiv 806843");
            if (Context &&
                ( !(
                ( m_SourceFile && (m_SourceFile->GetOptionFlags() & OPTION_OptionInfer) ) &&
                ( IsForControlVarDeclaration || Context->IsVariableWithValue())
                )))
            {
                // Breaking change - catch dim x, function bob(), const x = 42, when Option Strict is on
                if ( m_SourceFile && (m_SourceFile->GetOptionFlags() & OPTION_OptionStrict) )
                {
                    ERRID errid;
                    if( Context && Context->IsProc() )
                    {
                        if( DeclFlags & DECLF_HasRetval )
                        {
                            errid = ERRID_StrictDisallowsImplicitProc;
                        }
                        else
                        {
                            errid = ERRID_StrictDisallowsImplicitArgs;
                        }
                    }
                    else
                    {
                        errid = ERRID_StrictDisallowImplicitObject;
                    }
                    ReportError( errid, &TypeIdentifier->TextSpan );
                }
                else
                {
                    // only warn
                    StringBuffer buf;
                    if (Context->IsStaticLocalBackingField())
                    {
                        ReportError(WRNID_ObjectAssumedVar1,
                                    &TypeIdentifier->TextSpan,
                                    ResLoadString(WRNID_StaticLocalNoInference, &buf) );
                    }
                    else if (!Context->IsProc() || (DeclFlags & DECLF_HasRetval)==0)
                    {
                        ReportError(WRNID_ObjectAssumedVar1,
                                    &TypeIdentifier->TextSpan,
                                    ResLoadString(WRNID_MissingAsClauseinVarDecl, &buf) );
                    }
                    else if (Context->IsProperty())
                    {
                        ReportError(WRNID_ObjectAssumedProperty1,
                                    &TypeIdentifier->TextSpan,
                                    ResLoadString(WRNID_MissingAsClauseinProperty, &buf) );
                    }
                    else
                    {   
                        ReportError(WRNID_ObjectAssumed1,
                                    &TypeIdentifier->TextSpan,
                                    ResLoadString(Context->IsUserDefinedOperator() ?
                                                  WRNID_MissingAsClauseinOperator : WRNID_MissingAsClauseinFunction, &buf));
                    }
                }
            }
            if (Context && Context->IsVariable() && (m_SourceFile && (m_SourceFile->GetOptionFlags() & OPTION_OptionInfer)) &&
                !Context->IsVariableWithValue() && !Context->IsVariableWithArraySizes()
                && TypeIdentifier->IsNullable)
            {
                ReportError( ERRID_NullableImplicit, &TypeIdentifier->TextSpan );
            }

            BuiltTypeSymbol = m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
        }
    }
    else  // A Type was provided
    {
        // If a name was provided along with the type, make sure that the name doesn't have a typechar
        // Queries will report these errors themselves when building the anonymous types.
        if ( TypeIdentifier && TypeIdentifier->TypeCharacter != chType_NONE && !(DeclFlags & DECLF_LambdaArguments))
        {
            ReportError( ERRID_TypeCharWithType1, &TypeIdentifier->TextSpan, WszTypeChar( TypeIdentifier->TypeCharacter ));
        }

        if ( TypeIdentifier && TypeIdentifier->IsNullable )
        {
            if (TypeTree->Opcode == ParseTree::Type::Nullable)
            {
                ReportError( ERRID_CantSpecifyNullableOnBoth, &TypeTree->TextSpan );
                BuiltTypeSymbol = m_SymbolAllocator.GetGenericBadNamedRoot();
            }
            else
            {
                BuiltTypeSymbol = MakeNamedType( NULL, Context, TypeList, false/*IsTHISType*/, false /*IsAppliedAttributeContext*/, TypeTree /*ArgumentNullable*/);
            }
        }
        else
        {
            switch ( TypeTree->Opcode )
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

                BuiltTypeSymbol = m_CompilerHost->GetFXSymbolProvider()->GetType( MapTypeToVType( TypeTree->Opcode ));
                break;

            case ParseTree::Type::Object:

                BuiltTypeSymbol = m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
                break;

            case ParseTree::Type::SyntaxError:

                BuiltTypeSymbol = m_SymbolAllocator.GetGenericBadNamedRoot();
                break;

            case ParseTree::Type::Named:

                if (( TypeTree->AsNamed()->TypeName->IsQualified() && TypeTree->AsNamed()->TypeName->AsQualified()->Qualifier.IsBad ) ||
                    ( TypeTree->AsNamed()->TypeName->IsSimple() && TypeTree->AsNamed()->TypeName->AsSimple()->ID.IsBad ))
                {
                    BuiltTypeSymbol = m_SymbolAllocator.GetGenericBadNamedRoot();
                }
                else
                {

                    BuiltTypeSymbol = MakeNamedType( TypeTree->AsNamed()->TypeName, Context, TypeList, false/*IsTHISType*/, IsAppliedAttributeContext, NULL /*ArgumentNullable*/ );
                }
                break;

            case ParseTree::Type::ArrayWithSizes:
            case ParseTree::Type::ArrayWithoutSizes:
                {
                    ParseTree::ArrayType *ArrayTypeTree = TypeTree->AsArray();
                    unsigned ArrayRank = ArrayTypeTree->Rank;
                    BuiltTypeSymbol = m_SymbolAllocator.GetArrayType( ArrayRank, MakeType( ArrayTypeTree->ElementType, TypeIdentifier, Context, TypeList, 0, IsAppliedAttributeContext ));

                    if ( ArrayRank > ArrayRankLimit )
                    {
                        ReportError( ERRID_ArrayRankLimit, &ArrayTypeTree->TextSpan );
                    }
                    break;
                }
            case ParseTree::Type::Nullable:
                {
                    if ( TypeIdentifier && TypeIdentifier->IsNullable )
                    {
                        ReportError( ERRID_CantSpecifyNullableOnBoth, &TypeIdentifier->TextSpan);
                    }
                    BuiltTypeSymbol = MakeNamedType(
                        NULL, Context,
                        TypeList,
                        false/*IsTHISType*/,
                        false /*IsAppliedAttributeContext*/,
                        TypeTree /*IsNullable*/);

                    break;
                }

            case ParseTree::Type::AlreadyBound:
            case ParseTree::Type::AlreadyBoundDelayCalculated:
            {
                //This is just a placeholder for a bound type that was already interpreted.

                BCSYM *pBoundType = TypeTree->Opcode == ParseTree::Type::AlreadyBound ?
                    TypeTree->AsAlreadyBound()->BoundType :
                    TypeTree->AsAlreadyBoundDelayCalculated()->GetBoundType();

                if (pBoundType == NULL && TypeTree->Opcode == ParseTree::Type::AlreadyBoundDelayCalculated)
                {
                    // Microsoft
                    // To fix 446866, what's happening is that in a scenario where an event is
                    // exposed in an interface, and a class implements this event but provides a
                    // wrong signature, bindable still produces code for this even though the delegate
                    // does not get hooked up. This will result in the pBoundType being NULL.
                    pBoundType = Symbols::GetGenericBadNamedRoot();
                }
                else
                {
                    AssertIfTrue(TypeHelpers::IsBadType(pBoundType));

                    if(TypeHelpers::IsBadType(pBoundType))
                    {
                        // to prevent us from going into code gen
                        ReportError(ERRID_InternalCompilerError, &TypeTree->TextSpan);
                    }
                }

                BCSYM_NamedType *pNamedType = m_SymbolAllocator.GetNamedType(&TypeTree->TextSpan, Context, 0, NULL);
                pNamedType->SetIsUsedInAppliedAttributeContext(IsAppliedAttributeContext);
                pNamedType->SetSymbol(pBoundType);

                BuiltTypeSymbol = pNamedType;

                break;
            }

            default:
                VSFAIL( "Unknown type tree." );
            }
        }
    }

    return BuiltTypeSymbol;
}

void Declared::FillGenericArgumentsArray(
    unsigned ArgumentsIndex,
    unsigned ArgumentsArrayLength,
    NameTypeArguments ***ArgumentsArrayIn, // [in/out] allocated as necessary
    ParseTree::Name *NamedTree, // [in] the qualified name tree
    ParseTree::GenericArguments *ArgumentsTree,
    BCSYM_NamedRoot *Context, // [in] the class where the named type is defined
    BCSYM_NamedType **TypeList // [in] the type list to add it to
)
{
    bool IsBad = false;
    NameTypeArguments **ArgumentsArray = *ArgumentsArrayIn;

    if (ArgumentsArray == NULL)
    {
        ArgumentsArray = (NameTypeArguments **)m_SymbolAllocator.GetNorlsAllocator()->Alloc(
            VBMath::Multiply(
            sizeof(NameTypeArguments *), 
            ArgumentsArrayLength));
        *ArgumentsArrayIn = ArgumentsArray;
    }

    ParseTree::TypeList *ArgumentTree;
    unsigned ArgumentCount = 0;
    for ( ArgumentTree = ArgumentsTree->Arguments; ArgumentTree; ArgumentTree = ArgumentTree->Next )
    {
        ArgumentCount++;
    }

    BCSYM **Arguments = (BCSYM **)m_SymbolAllocator.GetNorlsAllocator()->Alloc(VBMath::Multiply(
        sizeof(BCSYM *), 
        ArgumentCount));
    unsigned NonNamedTypeArgumentCount = 0;

    bool TypesForTypeArgumentsSpecified = false;
#if DEBUG
    bool TypesForTypeArgumentsUnSpecified = false;
#endif

    unsigned ArgumentIndex = 0;
    for ( ArgumentTree = ArgumentsTree->Arguments; ArgumentTree; ArgumentTree = ArgumentTree->Next )
    {
        BCSYM *ArgumentType = NULL;

        // ArgumentTree->Element can be NULL for unspecified type arguments
        // which is a valid scenario for types in getype expressions.
        // Eg: GetType(C(Of ,))

        if (ArgumentTree->Element)
        {
            VSASSERT(!TypesForTypeArgumentsUnSpecified, "Inconsistency in type arguments parse tree!!!");

            ArgumentType = MakeType( ArgumentTree->Element,
                                     NULL,
                                     Context,
                                     TypeList );

            if (ArgumentType->IsBad())
            {
                IsBad = true;
            }

            if (ArgumentType->IsNamedType())
            {
                ArgumentType->PNamedType()->SetIsTypeArgumentForGenericType(true);
            }
            else
            {
                if (ArgumentType->IsArrayType())
                {
                    BCSYM *ArrayElementType = ArgumentType->PArrayType()->ChaseToNamedType();

                    if (ArrayElementType && ArrayElementType->IsNamedType())
                    {
                        ArrayElementType->PNamedType()->SetIsTypeArgumentForGenericType(true);
                    }
                }

                NonNamedTypeArgumentCount++;
            }

            TypesForTypeArgumentsSpecified = true;
        }
        else
        {
            NonNamedTypeArgumentCount++;

#if DEBUG
            // Either all or none should be specified.

            VSASSERT(!TypesForTypeArgumentsSpecified, "Inconsistency in type arguments parse tree!!!");

            TypesForTypeArgumentsUnSpecified = true;
#endif
        }

        Arguments[ArgumentIndex++] = ArgumentType;
    }


    // Preserve the locations of the non-named type arguments - essentially primitives
    // and arrays

    Location *NonNamedTypeArgumentLocations = NULL;

    if (NonNamedTypeArgumentCount > 0)
    {
        NonNamedTypeArgumentLocations = (Location *)m_SymbolAllocator.GetNorlsAllocator()->Alloc(
            VBMath::Multiply(
            sizeof(Location), 
            NonNamedTypeArgumentCount));

        ArgumentIndex = 0;
        unsigned NonNamedTypeArgumentIndex = 0;
        for ( ArgumentTree = ArgumentsTree->Arguments; ArgumentTree; ArgumentTree = ArgumentTree->Next )
        {
            if (!Arguments[ArgumentIndex] || !Arguments[ArgumentIndex]->IsNamedType())
            {
                VSASSERT(NonNamedTypeArgumentIndex < NonNamedTypeArgumentCount, "Inconsistency in non-namedtype type argument count!!!");

                if (ArgumentTree->Element)
                {
                    NonNamedTypeArgumentLocations[NonNamedTypeArgumentIndex] = ArgumentTree->Element->TextSpan;
                }
                else
                {
                    // For unspecified types, use the punctuator location because no type has been specified.
                    // eg: GetType(C1(Of ,)

                    if (ArgumentTree->Next)
                    {
                        // If the node is not the last node in the list, then limit the location to the
                        // punctuator. Note that the list node's end location points to the end of the
                        // entire list and not to the end of the current node.
                        //
                        NonNamedTypeArgumentLocations[NonNamedTypeArgumentIndex].
                            SetLocation( ArgumentTree->TextSpan.m_lBegLine,
                                         ArgumentTree->TextSpan.m_lBegColumn,
                                         ArgumentTree->TextSpan.m_lBegLine + ArgumentTree->Punctuator.Line,
                                         ArgumentTree->Punctuator.Column);
                    }
                    else
                    {
                        NonNamedTypeArgumentLocations[NonNamedTypeArgumentIndex] = ArgumentTree->TextSpan;
                    }
                }

                NonNamedTypeArgumentIndex++;
            }

            ArgumentIndex++;
        }
    }

    NameTypeArguments *ArgumentsHolder = (NameTypeArguments *)m_SymbolAllocator.GetNorlsAllocator()->Alloc(sizeof(NameTypeArguments));
    ArgumentsHolder->m_Arguments = Arguments;
    ArgumentsHolder->m_ArgumentCount = (unsigned __int32)ArgumentCount;
    ArgumentsHolder->m_BindingTextSpan = NamedTree->TextSpan;
    ArgumentsHolder->m_TextSpansForNonNamedTypeArguments = NonNamedTypeArgumentLocations;
    ArgumentsHolder->m_TypesForTypeArgumentsSpecified = TypesForTypeArgumentsSpecified;

    ArgumentsArray[ ArgumentsIndex ] = ArgumentsHolder;

}

/*****************************************************************************
;FillQualifiedNameArray

Fills an array of strings that represent a simple or qualified name from a
name parse tree
*****************************************************************************/
void Declared::FillQualifiedNameArray(
    unsigned NumDotDelimitedNames, // [in] how many delimited names, e.g. "hello.world" = 2
    ParseTree::Name *NamedTree, // [in] the qualified name tree
    _Out_cap_(NumDotDelimitedNames) STRING **NameArray, // [in/out] the pre-allocated array ( should have room for NumDotDelimitedNames STRING *'s ) to fill in
    NameTypeArguments ***ArgumentsArray, // [out] allocated if any names have generic arguments
    BCSYM_NamedRoot *Context, // [in] the class where the named type is defined -- can be null if no names can have generic arguments
    BCSYM_NamedType **TypeList, // [in] the type list to add it to -- can be null if no names can have generic arguments
    bool    *isGlobalNameSpaceBased // [out] set true if the name starts with "GLobalNameSpace" keyword.
                                     // Only the out flag is set, the keyword "GLobalNameSpace" is not copied
)
{
    if (ArgumentsArray)
    {
        *ArgumentsArray = NULL;
    }

    if ( NumDotDelimitedNames > 0 ) // Is there anything to do?
    {
        unsigned CurrentNameCount = 1; // The number of names is the number of dots plus one.

        // Peel off the Qualified portions of the name
        // The number of names is the number of dots plus one.
        while ( NamedTree->IsQualified() && CurrentNameCount <= NumDotDelimitedNames)
        {
            NameArray[ NumDotDelimitedNames - CurrentNameCount ] = NamedTree->AsQualified()->Qualifier.Name;

            if ( NamedTree->Opcode == ParseTree::Name::QualifiedWithArguments )
            {
                FillGenericArgumentsArray( NumDotDelimitedNames - CurrentNameCount,
                                           NumDotDelimitedNames,
                                           ArgumentsArray,
                                           NamedTree,
                                           &NamedTree->AsQualifiedWithArguments()->Arguments,
                                           Context,
                                           TypeList );
            }

            CurrentNameCount++;

            NamedTree = NamedTree->AsQualified()->Base; // work down to the SimpleName
        }

        // Peel off the Simple name portion of the name
        if ( NamedTree->IsSimple() )
        {
            if (CurrentNameCount <= NumDotDelimitedNames)
            {
                NameArray[ NumDotDelimitedNames - CurrentNameCount ] = NamedTree->AsSimple()->ID.Name;

                if ( NamedTree->Opcode == ParseTree::Name::SimpleWithArguments )
                {
                    FillGenericArgumentsArray( NumDotDelimitedNames - CurrentNameCount,
                                               NumDotDelimitedNames,
                                               ArgumentsArray,
                                               NamedTree,
                                               &NamedTree->AsSimpleWithArguments()->Arguments,
                                               Context,
                                               TypeList );
                }
            }
        }
        else
        {
            VSASSERT( NamedTree->Opcode == ParseTree::Name::GlobalNameSpace, "A name starts with a simple or Global Name Space");
            //NameArray[ NumDotDelimitedNames - CurrentNameCount ] = STRING_CONST(this->m_CompilerInstance, GlobalNameSpace);
            if( isGlobalNameSpaceBased )
            {
                *isGlobalNameSpaceBased = true;
            }
        }
    }
    else if ( NamedTree->Opcode == ParseTree::Name::GlobalNameSpace )
    {   // Qualified names that consists of "Global" only have NumDotDelimitedNames = 0
            if( isGlobalNameSpaceBased )
            {
                *isGlobalNameSpaceBased = true;
            }
    }
}


/**************************************************************************************************
;DetermineIfBadName

See if any parts of a name tree are marked as bad
***************************************************************************************************/
bool // true - name is bad / false it is good
Declared::DetermineIfBadName(
    ParseTree::Name *Name
)
{
    while ( Name->IsQualified() )
    {
        if ( Name->AsQualified()->Qualifier.IsBad == true ) return true;
        Name = Name->AsQualified()->Base; // work down to the SimpleName
    }
    if ( Name->IsSimple() )
    {
        if ( Name->AsSimple()->ID.IsBad == true ) return true;
    }
    return false;
}

/*****************************************************************************
;MakeNamedType

Build a named type symbol from a parse tree.
*****************************************************************************/
BCSYM_NamedType * // the built type
Declared::MakeNamedType
(
    ParseTree::Name *NamedTypeTreeParm, // [in] The name for the named type symbol
    BCSYM_NamedRoot *Context, // [in] the class where the named type is defined
    BCSYM_NamedType **TypeList, // [in] the type list to add it to
    bool IsTHISType,            // [optional-in] Does the named type correspond to the THIS/Me type of the current context ?
    bool IsAppliedAttributeContext, // [optional-in] Indicates whether this type is being created in the context of an applied attribute.
    ParseTree::Type* ArgumentNullable    // [optional-in] Used for null propagating types like "dim x as integer?".
                                        // if this param is not null, ignore NamedTypeTree and make a named type Nullable(Of ArgumentNullable)
)
{
    ParseTree::Name *NamedTypeTree = NamedTypeTreeParm;
    VSASSERT((NamedTypeTree != NULL && ArgumentNullable == NULL) ||
             (NamedTypeTree == NULL && (IsTHISType || ArgumentNullable != NULL )),
             "Wrong call of MakeNamedType using nullable argument");
    BCSYM_NamedType *BuiltNamedRootSymbol = NULL;

    Location TempLoc;
    Location *NamedTypeLocation;
    unsigned CountDotDelimitedNames;

    // A fake parse tree data used to represent nullable types (e.g. "Integer?").
    struct ParseTreeSnippet : ZeroInit<ParseTreeSnippet>
    {
        ParseTree::QualifiedWithArgumentsName Name;
        ParseTree::TypeList TypeList;
        ParseTree::QualifiedName base;
        ParseTree::Name global;
    } snippet;


    if (ArgumentNullable != NULL)
    {
        // Wrap the input ArgumentNullable into a "Nullable(Of ArgumentNullable)"

        Location &TypeTextSpan = ArgumentNullable->TextSpan;
        if (ArgumentNullable->Opcode == ParseTree::Type::Nullable)
        {
            ArgumentNullable = ArgumentNullable->AsNullable()->ElementType;
        }

        snippet.global.Opcode = ParseTree::Name::GlobalNameSpace;
        snippet.global.TextSpan = TypeTextSpan;

        //memset(&base, 0, sizeof (base));
        snippet.base.Opcode = ParseTree::Name::Qualified;
        snippet.base.Base = &snippet.global;
        snippet.base.TextSpan = TypeTextSpan;
        snippet.base.Qualifier.Name= STRING_CONST(m_CompilerInstance, System);
        snippet.base.Qualifier.TextSpan = TypeTextSpan;

        snippet.TypeList.Element = ArgumentNullable;
        snippet.TypeList.TextSpan = ArgumentNullable->TextSpan;

        snippet.Name.TextSpan = TypeTextSpan;
        snippet.Name.Opcode = ParseTree::Name::QualifiedWithArguments;
        snippet.Name.Base = &snippet.base;
        snippet.Name.Qualifier.Name = m_CompilerInstance->AddString(WIDE("Nullable"));
        snippet.Name.Qualifier.TextSpan = TypeTextSpan;
        snippet.Name.Arguments.Opcode = ParseTree::GenericArguments::WithTypes;
        snippet.Name.Arguments.Arguments = &snippet.TypeList;

        NamedTypeTree = &snippet.Name;

        // this error is not going to be seen on regular installations. It is for platforms with mscorlib.dll missing Nullable
        if (!this->m_CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
        {
            ReportError( ERRID_NoNullableType, &TypeTextSpan );
            snippet.Name.Qualifier.IsBad = true;
        }
    }

    if (IsTHISType)
    {
        VSASSERT( !NamedTypeTree, "Non-null named unexpected when making a THIS type!!!");
        VSASSERT( ArgumentNullable == NULL, "wrong call of MakeNamedType using nullable argument and IsTHISTYpe");

        TempLoc = Location::GetHiddenLocation();
        NamedTypeLocation = &TempLoc;
        CountDotDelimitedNames = 0;
    }
    else
    {
        NamedTypeLocation = &NamedTypeTree->TextSpan;
        CountDotDelimitedNames = NamedTypeTree->GetDelimitedNameCount();
    }

    // Allocate the name.
    //
    // Note that this ordering here of allocating the named type for the generic before the named types
    // for its type arguments is particularly important. Particularly, we want the type arguments to
    // come before the generics itself in the typelist. This ordering ensures that the type arguments
    // are resolved before the generics itself when binding. This order of resolving makes it possible
    // for the generic bindings caching mechanism to work for source file.
    //
    BuiltNamedRootSymbol = m_SymbolAllocator.GetNamedType( NamedTypeLocation,
                                                           Context,
                                                           CountDotDelimitedNames,
                                                           TypeList );  // the built type is added to this list

    if (IsTHISType)
    {
        VSASSERT(!IsAppliedAttributeContext, "Unexpected flags for named type!!!");
        BuiltNamedRootSymbol->SetIsTHISType(true);
    }
    else
    {
        // Put the name in the symbol
        CheckForTypeCharacter( NamedTypeTree );
        bool isGlobalNameSpaceBased = false;
        NameTypeArguments ** tmp = BuiltNamedRootSymbol->PNamedType()->GetArgumentsArray();
        FillQualifiedNameArray( CountDotDelimitedNames,
                                NamedTypeTree,
                                BuiltNamedRootSymbol->PNamedType()->GetNameArray(),
                                &tmp,
                                Context,
                                TypeList,
                                &isGlobalNameSpaceBased);
        BuiltNamedRootSymbol->PNamedType()->SetArgumentsArray(tmp);

        BuiltNamedRootSymbol->SetIsGlobalNameSpaceBased(isGlobalNameSpaceBased);  // flag the symbol that starts with "GlobalNameSpace..."

        BuiltNamedRootSymbol->SetIsUsedInAppliedAttributeContext(IsAppliedAttributeContext);
    }

    return BuiltNamedRootSymbol;
}

/*****************************************************************************
;ResolveTypeLists

Resolve all named type and constant expressions for a SourceFile
*****************************************************************************/
void Declared::ResolveTypeLists
(
    BCSYM_Hash *LocalsHash,
    TypeResolutionFlags ResolutionFlags
)
{
    Bindable::ResolveAllNamedTypes( m_TypeList,
                                    m_SymbolAllocator.GetNorlsAllocator(),
                                    m_ErrorTable,
                                    m_CompilerInstance,
                                    m_CompilerHost,
                                    m_SourceFile,
                                    ResolutionFlags,
                                    m_CompilationCaches);

    Bindable::EvaluateConstantDeclarations(
        *GetExpressionListHead(),
        m_SymbolAllocator.GetNorlsAllocator(),
        m_ErrorTable,
        m_CompilerInstance,
        m_CompilerHost,
        m_CompilationCaches);

    Bindable::CheckAllGenericConstraints(
        m_TypeList,
        m_ErrorTable,
        m_CompilerHost,
        m_CompilerInstance,
        &m_SymbolAllocator,
        m_CompilationCaches);
}

/*****************************************************************************
;CloneParameters

Makes a copy of a parameter list. This is used by MakeProperty to clone
parameters from the Property to the Set/Get methods as well as MakeDelegateMethods
to share the parameters between the Delegate and the Invoke method.
*****************************************************************************/
BCSYM_Param *Declared::CloneParameters
(
    BCSYM_Param *ParameterSymbol,
    BCSYM_Param **LastParameterSymbol
)
{
    return
        CloneParameters(
            ParameterSymbol,
            false, // Default behavior is to include ByVal parameters
            LastParameterSymbol);
}

BCSYM_Param *Declared::CloneParameters
(
    BCSYM_Param *ParameterSymbol,
    bool IgnoreByValParameters,
    BCSYM_Param **LastParameterSymbol
)
{
    return CloneParameters(ParameterSymbol, IgnoreByValParameters, LastParameterSymbol, &m_SymbolAllocator);
}

BCSYM_Param *Declared::CloneParameters
(
    BCSYM_Param *ParameterSymbol,
    bool IgnoreByValParameters,
    BCSYM_Param **LastParameterSymbol,
    Symbols * pSymbols
)
{
    BCSYM_Param *BuiltParamSymbolsHead  = NULL;
    BCSYM_Param *BuiltLastParamSymbol   = NULL;

    for ( ; ParameterSymbol ; ParameterSymbol = ParameterSymbol->GetNext())
    {
        if (IgnoreByValParameters && !ParameterSymbol->IsByRefKeywordUsed())
        {
            // In certain cases, such as building the parameters for EndInvoke,
            // all ByVal parameters are ignored.
            continue;
        }

        // Assume it doesn't have a value when building the symbol.  When we copy it over we will set the kind to the right thing.
        BCSYM_Param *BuiltParamSymbol = pSymbols->AllocParameter( ParameterSymbol->HasLocation(), false );
        memcpy( BuiltParamSymbol, ParameterSymbol, sizeof( BCSYM_Param ));

        // The location does not get copied with a memcopy (since it's actually outside the symbol)
        if ( ParameterSymbol->HasLocation())
        {
            BuiltParamSymbol->SetLocation( ParameterSymbol->GetLocation());
        }

        BuiltParamSymbol->SetNext(NULL); // this is a copy - don't reuse the next pointer of the original

        if ( BuiltLastParamSymbol )
        {
            BuiltLastParamSymbol->SetNext(BuiltParamSymbol);
        }
        else
        {
            BuiltParamSymbolsHead = BuiltParamSymbol;
        }

        BuiltLastParamSymbol = BuiltParamSymbol;
    }

    if (LastParameterSymbol)
    {
        *LastParameterSymbol = BuiltLastParamSymbol;
    }

    return BuiltParamSymbolsHead;
}


/*========================================================================

Routines to map parse flags to decl flags

========================================================================*/

/*****************************************************************************
;MapSpecifierFlags

Convert parse tree flags to declaration flags
*****************************************************************************/
DECLFLAGS // the mapped declaration flags
Declared::MapSpecifierFlags
(
    ParseTree::SpecifierList *specifiers // [in] specifier flags from the parser that we want to map to DECLFLAGS
)
{
    DECLFLAGS SpecifierFlags = 0;
    for ( ParseTree::SpecifierList *CurrentSpecifier = specifiers;
          CurrentSpecifier;
          CurrentSpecifier = CurrentSpecifier->Next )
    {
        switch ( CurrentSpecifier->Element->Opcode )
        {
            case ParseTree::Specifier::Private:
            {
                if (!(SpecifierFlags & DECLF_AccessFlags))
                {
                    SpecifierFlags |= DECLF_Private;
                }
                continue;
            }
            case ParseTree::Specifier::Protected:
            {
                if (SpecifierFlags & DECLF_Friend)
                {
                    SpecifierFlags &= ~DECLF_Friend;
                    SpecifierFlags |= DECLF_ProtectedFriend;
                }
                else
                {
                    if (!(SpecifierFlags & DECLF_AccessFlags))
                    {
                        SpecifierFlags |= DECLF_Protected;
                    }
                }
                continue;
            }
            case ParseTree::Specifier::Public:
            {
                if (!(SpecifierFlags & DECLF_AccessFlags))
                {
                    SpecifierFlags |= DECLF_Public;
                }
                continue;
            }
            case ParseTree::Specifier::Friend:
            {
                if ( SpecifierFlags & DECLF_Protected )
                {
                    SpecifierFlags &= ~DECLF_Protected;
                    SpecifierFlags |= DECLF_ProtectedFriend;
                }
                else
                {
                    if (!(SpecifierFlags & DECLF_AccessFlags))
                    {
                        SpecifierFlags |= DECLF_Friend;
                    }
                }
                continue;
            }
            case ParseTree::Specifier::NotOverridable:
            {
                SpecifierFlags |= DECLF_NotOverridable;
                continue;
            }
            case ParseTree::Specifier::Overridable:
            {
                SpecifierFlags |= DECLF_Overridable;
                continue;
            }
            case ParseTree::Specifier::MustOverride:
            {
                SpecifierFlags |= DECLF_MustOverride;
                continue;
            }
            case ParseTree::Specifier::Static:
            {
                SpecifierFlags |= DECLF_Static;
                continue;
            }
            case ParseTree::Specifier::Shared:
            {
                SpecifierFlags |= DECLF_Shared;
                continue;
            }
            case ParseTree::Specifier::Overloads:
            {
                SpecifierFlags |= DECLF_OverloadsKeywordUsed;
                continue;
            }
            case ParseTree::Specifier::Overrides:
            {
                SpecifierFlags |= DECLF_OverridesKeywordUsed;
                continue;
            }
            case ParseTree::Specifier::Const:
            {
                SpecifierFlags |= DECLF_Const;
                continue;
            }
            case ParseTree::Specifier::ReadOnly:
            {
                SpecifierFlags |= DECLF_ReadOnly;
                continue;
            }
            case ParseTree::Specifier::WriteOnly:
            {
                SpecifierFlags |= DECLF_WriteOnly;
                continue;
            }
            case ParseTree::Specifier::Dim:
            {
                SpecifierFlags |= DECLF_Dim;
                continue;
            }
            case ParseTree::Specifier::Default:
            {
                SpecifierFlags |= DECLF_Default;
                continue;
            }
            case ParseTree::Specifier::MustInherit:
            {
                SpecifierFlags |= DECLF_MustInherit;
                continue;
            }
            case ParseTree::Specifier::NotInheritable:
            {
                SpecifierFlags |= DECLF_NotInheritable;
                continue;
            }
            case ParseTree::Specifier::Partial:
            {
                SpecifierFlags |= DECLF_Partial;
                continue;
            }
            case ParseTree::Specifier::Shadows:
            {
                SpecifierFlags |= DECLF_ShadowsKeywordUsed;
                continue;
            }
            case ParseTree::Specifier::WithEvents:
            {
                SpecifierFlags |= DECLF_WithEvents;
                continue;
            }
            case ParseTree::Specifier::Widening:
            {
                SpecifierFlags |= DECLF_Widening;
                continue;
            }
            case ParseTree::Specifier::Narrowing:
            {
                SpecifierFlags |= DECLF_Narrowing;
                continue;
            }
            case ParseTree::Specifier::Custom:
            {
                // Ignore, only used for parse error scenarios. Error should already
                // have been reported in the parser.
                continue;
            }
            case ParseTree::Specifier::Async:
            {
                SpecifierFlags |= DECLF_Async;
                continue;
            }
            case ParseTree::Specifier::Iterator:
            {
                SpecifierFlags |= DECLF_Iterator;
                continue;
            }
            default:
                VSFAIL("surprising specifier");

        } // switch
    } // loop through specifier list

    return SpecifierFlags;
}


/*****************************************************************************
;MapVarDeclFlags

Map parse tree flags for a declaration to declared flags.  Note that the specifier flags
are not mapped - this is done by MapSpecifierFlags() because it is beneficial in various
situations to have that part factored out.
*****************************************************************************/
DECLFLAGS  // the declared flags that mapped from parse flags
Declared::MapVarDeclFlags
(
    ParseTree::Declarator *VariableDecl, // [in] the variable to map flags for
    ParseTree::NewVariableDeclaration *DeclaredAsNew // [in] if set, we have a variable declared As New
)
{
    DECLFLAGS VarFlags = 0;

    if ( DeclaredAsNew )
    {
        VarFlags |= DECLF_New;

        // Because 'New' isn't a specifier, I would have to special case dozens of places to figure out
        // what the location information for the 'New' portion of a declaration is.  Delightful.  So instead, I'll
        // post the information for the last symbol built ( which should be the next symbol tested )
        // and GetDeclFlagInfo() can use it when calculating location information for the New specifier in
        // the event an error needs to be logged about the usage of 'New' on this member.

        // Microsoft: 



        m_AsNewLocation.m_lBegLine = VariableDecl->Name.TextSpan.m_lBegLine + DeclaredAsNew->New.Line;
        m_AsNewLocation.m_lEndLine = m_AsNewLocation.m_lBegLine;
        m_AsNewLocation.m_lBegColumn = DeclaredAsNew->New.Column;
        m_AsNewLocation.m_lEndColumn = m_AsNewLocation.m_lBegColumn + 2; // Length of string "New" ( adjusted since location is 1 based )
    }

    return VarFlags;
}

/*****************************************************************************
;MapProcDeclFlags

Map parse tree flags to declared flags
*****************************************************************************/
DECLFLAGS // the declared flags for this method
Declared::MapProcDeclFlags
(
    ParseTree::MethodSignatureStatement *MethodTree // [in] the method declaration to map flags on
)
{
    // Map the specifier flags.
    DECLFLAGS ProcFlags = MapSpecifierFlags( MethodTree->Specifiers );

    // Map the declaration flags.

    if ( MethodTree->Opcode == ParseTree::Statement::ProcedureDeclaration ||
         MethodTree->Opcode == ParseTree::Statement::ConstructorDeclaration ||
         MethodTree->Opcode == ParseTree::Statement::DelegateProcedureDeclaration )
    {
        ProcFlags |= DECLF_Function;
    }

    if ( MethodTree->Opcode == ParseTree::Statement::FunctionDeclaration ||
         MethodTree->Opcode == ParseTree::Statement::DelegateFunctionDeclaration )
    {
        ProcFlags |= DECLF_Function | DECLF_HasRetval;
    }

    if ( MethodTree->Opcode == ParseTree::Statement::OperatorDeclaration )
    {
        ProcFlags |= DECLF_Function | DECLF_HasRetval | DECLF_SpecialName | DECLF_DisallowByref;
    }

    return ProcFlags;
}

/*****************************************************************************
;MapDeclareDeclFlags

Map parse tree flags for a DLLDeclare statement to declared flags
*****************************************************************************/
DECLFLAGS // the mapped flags
Declared::MapDeclareDeclFlags
(
    ParseTree::ForeignMethodDeclarationStatement *DllDeclareTree // [in] the DLLDeclare statement tree to map flags for
)
{
    DECLFLAGS DeclareFlags = 0;

    // Map the specifier flags.

    DeclareFlags |= MapSpecifierFlags( DllDeclareTree->Specifiers );

    // Map the declaration flags.

    if ( DllDeclareTree->Opcode == ParseTree::Statement::ProcedureDeclaration )
    {
        DeclareFlags |= DECLF_Function;
    }
    else if ( DllDeclareTree->Opcode == ParseTree::Statement::ForeignProcedureDeclaration )
    {
        DeclareFlags |= DECLF_Function;
    }
    else if ( DllDeclareTree->Opcode == ParseTree::Statement::ForeignFunctionDeclaration )
    {
        DeclareFlags |= DECLF_Function | DECLF_HasRetval;
    }

    if ( DllDeclareTree->StringMode == ParseTree::ForeignMethodDeclarationStatement::Unicode )
    {
        DeclareFlags |= DECLF_Unicode;
    }
    else if ( DllDeclareTree->StringMode == ParseTree::ForeignMethodDeclarationStatement::ANSI )
    {
        DeclareFlags |= DECLF_Ansi;
    }
    else if ( DllDeclareTree->StringMode == ParseTree::ForeignMethodDeclarationStatement::Auto )
    {
        DeclareFlags |= DECLF_Auto;
    }

    return DeclareFlags;
}

/*========================================================================

Routines to the real work of declared

========================================================================*/

/*****************************************************************************
;DoMakeDeclared

Produce symbol table symbols for all the declarations in the file
*****************************************************************************/
void
Declared::DoMakeDeclared
(
    ParseTree::FileBlockStatement *FileBlockTree, // [in] the parse tree for the file
    SymbolList *OwningSymbolList  // [in] symbol list to add to
)
{
#if !IDE
    if ( FileBlockTree )
    {
        // Do the #ExternalSource directives
        m_SourceFile->SetMappedLineInfo( FileBlockTree->SourceDirectives );
    }
#endif

    // Get Project options that apply to the file
    OPTION_FLAGS OptionFlags = m_SourceFile->GetProject()->OptionCompareText() ? OPTION_OptionText : 0; // default
    OptionFlags |= m_SourceFile->GetProject()->OptionStrictOff() ? 0 : OPTION_OptionStrict;
    OptionFlags |= m_SourceFile->GetProject()->OptionExplicitOff() ? 0 : OPTION_OptionExplicit;
    OptionFlags |= m_SourceFile->GetProject()->OptionInferOff() ? 0 : OPTION_OptionInfer;
    m_SourceFile->SetOptionFlags( OptionFlags );
    m_SourceFile->SetOptionStrictOffSeenOnFile(false);
    m_FileLevelSymbolList = OwningSymbolList;

    // Build the symbols for the file.
    DriveMakeNamespace( FileBlockTree, NamespaceNesting::Project );
    VSASSERT( m_SourceFile->GetUnnamedNamespace() != NULL, "How did we get away without building a namespace for the file?" );

    // Report all errors for XMLDoc comment blocks that could not be associated with any valid language element.
    ReportAllOrphanedXMLDocCommentErrors();
}

/**************************************************************************************************
;DoMakeLocalsFromTrees

Builds local variable symbols from a parse tree.
Verifies that a duplicate local variable isn't defined.
This puny function exists because people are calling a static function (MakeLocalsFromTrees())
which requires class state.  So MakeLocalsFromTrees() hacks up an instance of declared and calls
this function to kick off making locals.  Sigh.
***************************************************************************************************/
void Declared::DoMakeLocalsFromTrees
(
    ParseTree::VariableDeclarationStatement *DeclarationTree, // [in] the parse tree for the declaration
    BCSYM_Hash *LocalsHash, // [in] hash to put built symbols into / contains locals built from last tree sent through here
    BCSYM_Proc *OwningProcedure, // [in] the procedure we are making locals for
    BCSYM_NamedRoot **CatchVariable, // [out] will point to the variable created for the catch
    bool CalledFromParseTreeService, // [in] whether we are being called from a possible CS_NoState situation
    bool PerfomObsoleteCheck,
    bool lambdaMember
)
{
    MakeLocals(
        DeclarationTree, 
        OwningProcedure, 
        LocalsHash, 
        CatchVariable, 
        CalledFromParseTreeService,
        PerfomObsoleteCheck, 
        lambdaMember);
}

/*========================================================================

Routines to build namespaces

========================================================================*/

/*****************************************************************************
;DriveMakeNamespace

Drives the building of namespaces.  For instance, if we are building a.b.c,
this creates the list of namespaces that need to be build (a, b, and c) and
calls MakeNamespace for each, in the right order.  Also makes sure that the
created namespaces are created within the default namespace specified by the
project.
*****************************************************************************/
void Declared::DriveMakeNamespace(
    ParseTree::BlockStatement *NamespaceTree, // [in] the tree for the namespace block.  This may come as a FileBlockStatement or a TypeStatement
    NamespaceNestingEnum Nesting              // [in] Project / File / Nested
)
{
    NamespaceNameList *NamespacesToMake = NULL; // a list of namespaces we need to build, e.g. Namespace A.B.C -> produces a,b,c

    // Derive the list of namespaces to build (handles qualified namespace names) from the parse tree
    Location *NamespaceNameLocation = NULL;
    if ( NamespaceTree && NamespaceTree->Opcode == ParseTree::Statement::Namespace )
    {
        NamespaceNameLocation = &NamespaceTree->AsNamespace()->Name->TextSpan;
        BuildNamespaceList( NamespaceTree->AsNamespace()->Name, &NamespacesToMake, false );
    }

    // Next level of namespace nesting is ...
    NamespaceNestingEnum NextLevel;

    // If this is the first time we've been called for the source file, we are building the outermost namespace
    // which needs to be whatever the project supplies for a default namespace with the unnamed namespace around that
    switch ( Nesting )
    {
    case NamespaceNesting::Project:
        {
            STRING *DefaultNamespace = m_SourceFile->GetProject()->GetDefaultNamespace(); // The default namespace the project supplies.  Set in \vs\src\vsproject\vb\vbbuild\vbcompiler.cpp CVbBldCompilerOptions::Init(CVbBldCompiler *pCompiler)
            if ( DefaultNamespace && wcscmp( DefaultNamespace, L"" ) != 0 ) // Did the project supply a default namespace?  harish: 
            {
                // Project supplied a namespace name - may be qualified
                bool Error;

                Parser NameParser(
                    &m_ScratchAllocator,
                    m_CompilerInstance,
                    m_CompilerHost,
                    false,
                    m_SourceFile->GetProject()->GetCompilingLanguageVersion()
                    );
                ParseTree::Name *DefaultNamespaceTree = NameParser.ParseName( Error, MakeSafeQualifiedName( m_CompilerInstance, DefaultNamespace ));

                if ( !Error )
                {
                    BuildNamespaceList( 
                        DefaultNamespaceTree, 
                        &NamespacesToMake, 
                        true  /* building the default namespace */
                        ); // Prepends the default namespace to NamespacesToMake
                }
                // error reported by project
            }

            // And the global (unnamed) namespace wraps all
            PrependToNamespaceList( &NamespacesToMake, STRING_CONST( m_CompilerInstance, EmptyString ), NULL );
        }
        NextLevel = NamespaceNesting::File;
        break;

    case NamespaceNesting::Nested:
        // Disallow Global namespace nested in another namespace
        if ( NamespacesToMake && NamespaceNameLocation && NamespacesToMake->Global )
        {
            ReportError( ERRID_InvalidGlobalNamespace, NamespaceNameLocation );
        }
        NextLevel = NamespaceNesting::Nested;
        break;

    default:
         VSASSERT( false, "Unknown NamespaceNesting kind" );

         // fall through

    case NamespaceNesting::File:
        NextLevel = NamespaceNesting::Nested;
        break;
    }

    // Build the symbols for the namespaces in the list
    MakeNamespace( NamespacesToMake, NamespaceTree, NextLevel, NamespaceNameLocation );
}


STRING *Declared::PushNamespaceContext(_In_ NamespaceNameList *NamespaceList)
{
    VSASSERT(NamespaceList, "Need non-null namespace list");

    STRING *OuterContext = m_CurrentNamespaceContext;
    if ( NamespaceList->Global )
    {
        m_CurrentNamespaceContext = STRING_CONST( m_CompilerInstance, EmptyString );
    }
    else
    {
        m_CurrentNamespaceContext = ConcatNameSpaceAndName(m_CompilerInstance, OuterContext, NamespaceList->Name);
    }
    return OuterContext;
}

/*****************************************************************************
;MakeNamespace

Build all of the symbols inside of a namespace
*****************************************************************************/
void
Declared::MakeNamespace(
    NamespaceNameList *NamespaceList,         // [in] The namespace we are to build, e.g. A.B.C as the list A->B->C
    ParseTree::BlockStatement *NamespaceTree, // [in] the tree for the namespace block.  This may come as a FileBlockStatement or a TypeStatement
    Declared::NamespaceNestingEnum Nesting,   // [in] true - we are processing a nested namespace statement, false - we are building the root namespace for the file
    Location *Loc                             // [in] location information for the name
)
{
    VSASSERT(m_CurrentNamespaceContext, "Must always have a valid namespace context.");

    if ( !NamespaceList || !NamespaceTree)
    {
        VSFAIL("Somehow we ended up without any namespaces to build");
        return;
    }

    SymbolList NamespaceChildren; // symbols that belong to the namespace are captured here
    ImportedTarget *ImportsList = NULL; // We build this list on a namespace by namespace basis
    bool FoundAssemblyAttribute = false; // Have we seen an assembly- or module-level custom attribute?

    // Save the current namespace context.
    // The same namespace can be spread across several files or projects.
    // To ensure uniform casing for these partial namespaces, the name of the new
    // namespace context should match the name of identical namespaces available to
    // the current project.

    STRING *OuterNamespaceContext = PushNamespaceContext(NamespaceList);

    // 




















    const unsigned EmptyHash = 0; //0xFFFFFFFF;

    BCSYM_Namespace *BuiltNamespace =
        StringPool::StringLength(m_CurrentNamespaceContext) == 0 ?
            m_CompilerInstance->GetUnnamedNamespace( m_SourceFile, EmptyHash, EmptyHash ) :
            m_CompilerInstance->GetNamespace( m_CurrentNamespaceContext, m_SourceFile, Loc, EmptyHash, EmptyHash );


    if (NamespaceList->pLocation)
    {
        SpellingInfo * pSpelling = (SpellingInfo *)m_SourceFile->m_nraSymbols.Alloc(sizeof(SpellingInfo));
        pSpelling->m_Location.SetLocation(NamespaceList->pLocation);
        pSpelling->m_pSpelling = NamespaceList->Name;
        BuiltNamespace->AddSpelling(pSpelling);

        if (m_SourceFile->GetLineMarkerTable())
        {
#if IDE
            m_SourceFile->GetLineMarkerTable()->AddDeclLocation(&(pSpelling->m_Location), BuiltNamespace);
#endif
        }
    }

    // set the current container context
    SetContainerContext(BuiltNamespace);

    if ( NamespaceList && NamespaceList->Next )
    {   // What I'm doing here is dealing with the creation of qualified namespaces, e.g. A.B
        // I've created the context for A (SymbolList NamespaceChildren above), but first we need to build B - go do it.
        MakeNamespace( NamespaceList->Next, NamespaceTree, Nesting, Loc );
    }
    else // Build this namespace
    {
        // If not nested namespace, then it is the root name space.
        // Set the Root Namespace on the file.
        if ( Nesting != NamespaceNesting::Nested )
        {
            m_SourceFile->SetRootNamespace(BuiltNamespace);
        }

        bool SeenOptionCompare = false;
        bool SeenOptionStrict = false;
        bool SeenOptionExplicit = false;
        bool SeenOptionInfer = false;

        for ( ParseTree::StatementList *Statement = NamespaceTree->Children;
              Statement;
              Statement = Statement->NextInBlock )
        {
            switch ( Statement->Element->Opcode )
            {
                // OPTION
                case ParseTree::Statement::OptionUnknown:
                case ParseTree::Statement::OptionCompareNone:
                case ParseTree::Statement::OptionCompareText:
                case ParseTree::Statement::OptionCompareBinary:
                case ParseTree::Statement::OptionStrictOn:
                case ParseTree::Statement::OptionStrictOff:
                case ParseTree::Statement::OptionExplicitOn:
                case ParseTree::Statement::OptionExplicitOff:
                case ParseTree::Statement::OptionInferOn:
                case ParseTree::Statement::OptionInferOff:
                    MakeOption( Statement->Element, SeenOptionCompare, SeenOptionStrict, SeenOptionExplicit, SeenOptionInfer );
                    continue;

                // INTERFACE
                case ParseTree::Statement::Interface:

                    MakeInterface( Statement,
                                   false, // not defined in a class
                                   true,  // defined at namespace level
                                   false, // no generic parent
                                   &NamespaceChildren,
                                   0 );   // no container flags
                    continue;

                // IMPORTS
                case ParseTree::Statement::Imports:

                    MakeImports( Statement->Element->AsImports(), &ImportsList );
                    continue;

                // NAMESPACE
                case ParseTree::Statement::Namespace:

                    DriveMakeNamespace( Statement->Element->AsBlock(), Nesting );
                    continue;

                // CLASS
                case ParseTree::Statement::Class:

                    MakeClass( Statement,
                               false, // not nested
                               false, // not a std module
                               false, // not defined in an interface
                               false, // no generic parent
                               0, // no container flags
                               &NamespaceChildren );
                    continue;

                // MODULE
                case ParseTree::Statement::Module:

                    MakeClass( Statement,
                               false, // not nested
                               true,  // build a std module
                               false, // not defined in an interface
                               false, // no generic parent
                               0, // no container flags
                               &NamespaceChildren );
                    continue;

                // ENUM
                case ParseTree::Statement::Enum:

                    MakeEnum( Statement,
                              false, // not nested
                              false, // not defined in a std module
                              false, // no generic parent
                              0, // no container flags
                              &NamespaceChildren );
                    continue;

                // STRUCTURE
                case ParseTree::Statement::Structure:

                    MakeStructure( Statement,
                                   false, // not nested
                                   false, // not defined in an interface
                                   false, // no generic parent
                                   0, // no container flags
                                   &NamespaceChildren );
                    continue;

                // DELEGATE
                case ParseTree::Statement::DelegateProcedureDeclaration:
                case ParseTree::Statement::DelegateFunctionDeclaration:

                    MakeDelegate( Statement,
                                  false, // not nested
                                  false, // not inside a standard module
                                  &NamespaceChildren,
                                  FALSE // not being created for an event
                                );
                    continue;

                // ATTRIBUTE (assembly- or module-level)
                case ParseTree::Statement::Attribute:

                    // Remember that we saw this and process later
                    FoundAssemblyAttribute = true;
                    continue;

                case ParseTree::Statement::CCConst:
                case ParseTree::Statement::CCIf:
                case ParseTree::Statement::CCElseIf:
                case ParseTree::Statement::CCElse:
                case ParseTree::Statement::CCEndIf:
                case ParseTree::Statement::File:
                case ParseTree::Statement::Empty:
                case ParseTree::Statement::SyntaxError:
                case ParseTree::Statement::Region:
                    continue;

                case ParseTree::Statement::CommentBlock:

                    if ( Statement->Element->AsCommentBlock()->IsXMLDocComment == true )
                    {
                        AddCommentBlock( Statement->Element->AsCommentBlock() );
                    }

                    continue;

                default:
                    // Don't report errors on 'End *' constructs or horked stuff.
                    if ( !Statement->Element->IsEndConstruct() &&
                         Statement->Element->Opcode != ParseTree::Statement::OptionInvalid &&
                         Statement->Element->HasSyntaxError == false )
                    {
                        ReportError( ERRID_InvalidInNamespace, &Statement->Element->TextSpan );
                    }
                    continue;
            } // switch
        }// loop through namespace parse tree children
    } // build namespace

    m_CurrentNamespaceContext = OuterNamespaceContext;

    // Create Namespace hashes if not already built
    // Create the hash table for the namespace
    if (!BuiltNamespace->GetHashRaw())
    {
        unsigned NumOfBindableChildren =
            NamespaceChildren.GetCount() > 17 ? NamespaceChildren.GetCount() : 17;

        BuiltNamespace->SetHash(
            (BCSYM_Hash *)m_SymbolAllocator.AllocHash(NumOfBindableChildren));

        // Fill in the hash table.
        BuiltNamespace->GetHashRaw()->SetName(BuiltNamespace->GetName());
        BuiltNamespace->GetHashRaw()->SetImmediateParent(BuiltNamespace);
        BuiltNamespace->GetHashRaw()->SetBindingSpace((BindspaceType)(BINDSPACE_Normal | BINDSPACE_Type));
        BuiltNamespace->GetHashRaw()->PHash()->SetCBuckets(NumOfBindableChildren);
    }

    SymbolList *UnBindableChildren = GetUnBindableChildrenSymbolList();
    if (!BuiltNamespace->GetUnBindableChildrenHashRaw())
    {
        unsigned NumOfUnBindableChildren =
            UnBindableChildren->GetCount() > 11 ? UnBindableChildren->GetCount() : 11;

        BuiltNamespace->SetUnBindableChildrenHash(
            (BCSYM_Hash *)m_SymbolAllocator.AllocHash(NumOfUnBindableChildren) );

        // Fill in the hash table.
        BuiltNamespace->GetUnBindableChildrenHashRaw()->SetName(BuiltNamespace->GetName());
        BuiltNamespace->GetUnBindableChildrenHashRaw()->SetImmediateParent(BuiltNamespace);
        BuiltNamespace->GetUnBindableChildrenHashRaw()->SetBindingSpace((BindspaceType)(BINDSPACE_IgnoreSymbol));
        BuiltNamespace->GetUnBindableChildrenHashRaw()->PHash()->SetCBuckets(NumOfUnBindableChildren);

        BuiltNamespace->GetUnBindableChildrenHashRaw()->SetIsUnBindableChildrenHash();
    }


    // Dev10 #832844: Mark namespace if it contains Public or Friend types.
    // Need to do this before calling AddSymbolListToHash because it will destroy content of the NamespaceChildren list.
    BCSYM_NamedRoot *pnamed, *pnamedNext;

    for (pnamed = NamespaceChildren.GetFirst(); pnamed; pnamed = pnamedNext)
    {
        pnamedNext = pnamed->GetNextInSymbolList();

        AssertIfTrue(pnamed->IsNamespace());

        if (pnamed->IsContainer() && !pnamed->IsNamespace())
        {
            switch (pnamed->GetAccess())
            {
                case ACCESS_Public:
                    BuiltNamespace->SetContainsPublicType(true);
                    break;

                case ACCESS_Friend:
                    BuiltNamespace->SetContainsFriendType(true);
                    break;
            }
        }
    }

    // Add the Bindable children to the namespace
    Symbols::AddSymbolListToHash( BuiltNamespace->GetHashRaw(), &NamespaceChildren, TRUE );

    // Add the UnBindable children to the namespace
    Symbols::AddSymbolListToHash( BuiltNamespace->GetUnBindableChildrenHashRaw(), UnBindableChildren, TRUE );

    // Stow the imports for this namespace
    // File-level imports go in the unnamed namespace, not the default namespace

    if ( ImportsList )
    {
        ( Nesting == NamespaceNesting::Nested ? BuiltNamespace : m_SourceFile->GetUnnamedNamespace() )->SetImports( ImportsList );
    }

    // Process assembly-level attributes.  Need to do this here because m_SourceFile->GetUnnamedNamespace() is NULL in the switch statement.
    // 
    if ( FoundAssemblyAttribute )
    {
        for ( ParseTree::StatementList *Statement = NamespaceTree->Children;
              Statement;
              Statement = Statement->NextInBlock )
        {
            // Microsoft: only do this if this attribute does not have a syntax error.

            if ( Statement->Element->Opcode == ParseTree::Statement::Attribute &&
                 !Statement->Element->HasSyntaxError )
            {
                // Attach attributes to this SourceFile's root namespace
                ProcessAttributeList( Statement->Element->AsAttribute()->Attributes, m_SourceFile->GetRootNamespace());
            }
        }
    }

    // We allow global nested namespace ("Global." prefix). Do not add those to the 'container'.
    if ( !NamespaceList->Global )
    {
        AddContainerContextToSymbolList();
    }
    PopContainerContext();
}


/*****************************************************************************
;MakeOption

Build the symbol for an Option statement and set the resulting class flags accordingly
*****************************************************************************/
void Declared::MakeOption
(
    ParseTree::Statement *OptionStatementTree, // [in] the tree representing the option statement
    bool &SeenOptionCompare, // [out] whether we built Option Compare
    bool &SeenOptionStrict, // [out] whether we built Option Strict
    bool &SeenOptionExplicit, // [out] whether we built Option Explicit
    bool &SeenOptionInfer // [out] whether we built Option Infer
)
{
    ParseTree::Statement::Opcodes OptionKind = OptionStatementTree->Opcode;

    // Figure out the option kind for this.
    switch ( OptionKind )
    {
        case ParseTree::Statement::OptionInvalid:
        case ParseTree::Statement::OptionUnknown:
        case ParseTree::Statement::OptionCompareNone:

            break; // Can't do much with these, ignore them.

        // case OPTION_CMP: OptionCompareNone?
        case ParseTree::Statement::OptionCompareText:
        case ParseTree::Statement::OptionCompareBinary:
            // Report a duplicate option statement.
            if ( SeenOptionCompare )
            {
                ReportError( ERRID_DuplicateOption1, &OptionStatementTree->TextSpan, m_CompilerInstance->TokenToString( tkCOMPARE ));
            }
            else if ( OptionKind == ParseTree::Statement::OptionCompareText )
            {
                m_SourceFile->CombineOptionFlags( OPTION_OptionText );
            }
            else if ( OptionKind == ParseTree::Statement::OptionCompareBinary )
            {
                m_SourceFile->StripOptionFlags( OPTION_OptionText );
            }

            SeenOptionCompare = true;
            break;

        case ParseTree::Statement::OptionStrictOn:
        case ParseTree::Statement::OptionStrictOff:

            if ( SeenOptionStrict ) // can't have multiples of these
            {
                ReportError( ERRID_DuplicateOption1, &OptionStatementTree->TextSpan, m_CompilerInstance->TokenToString( tkSTRICT ));
                break;
            }
            SeenOptionStrict = true;

            if ( OptionKind == ParseTree::Statement::OptionStrictOn )
            {
                m_SourceFile->CombineOptionFlags( OPTION_OptionStrict );
            }
            else if ( OptionKind == ParseTree::Statement::OptionStrictOff )
            {
                m_SourceFile->StripOptionFlags( OPTION_OptionStrict );
                m_SourceFile->SetOptionStrictOffSeenOnFile(true);
            }
            break;

        case ParseTree::Statement::OptionExplicitOn:
        case ParseTree::Statement::OptionExplicitOff:

            if ( SeenOptionExplicit ) // can't have multiples of these
            {
                ReportError( ERRID_DuplicateOption1, &OptionStatementTree->TextSpan, m_CompilerInstance->TokenToString( tkEXPLICIT ));
                break;
            }
            SeenOptionExplicit = true;

            if ( OptionKind == ParseTree::Statement::OptionExplicitOn )
            {
                m_SourceFile->CombineOptionFlags( OPTION_OptionExplicit );
            }
            else if ( OptionKind == ParseTree::Statement::OptionExplicitOff )
            {
                m_SourceFile->StripOptionFlags( OPTION_OptionExplicit );
            }
            break;

        case ParseTree::Statement::OptionInferOn:
        case ParseTree::Statement::OptionInferOff:

            if ( SeenOptionInfer ) // can't have multiples of these
            {
                ReportError( ERRID_DuplicateOption1, &OptionStatementTree->TextSpan, m_CompilerInstance->TokenToString( tkINFER ));
                break;
            }
            SeenOptionInfer = true;

            if ( OptionKind == ParseTree::Statement::OptionInferOn )
            {
                m_SourceFile->CombineOptionFlags( OPTION_OptionInfer );
            }
            else if ( OptionKind == ParseTree::Statement::OptionInferOff )
            {
                m_SourceFile->StripOptionFlags( OPTION_OptionInfer );
            }
            break;

        default:

            VSASSERT( false, "Unknown optionkind" );
    };
}

/*****************************************************************************
;MakeImports

Build symbols for an imports statement
*****************************************************************************/
void Declared::MakeImports
(
    ParseTree::ImportsStatement *ImportsTree, // [in] the Imports parse tree
    ImportedTarget **ImportsList // [in/out] the list of imports that we tag this built import on to
)
{
    // Loop over the list of imported namespaces.  Imports bob.sally = alias, jane.dawn = alias2, etc.
    for ( ParseTree::ImportDirectiveList *ImportsListElement = ImportsTree->Imports;
           ImportsListElement;
           ImportsListElement = ImportsListElement->Next )
    {
        // Walk over the names defined off this statement and create an ImportedNamespace on the importednamespaces list.
        ImportedTarget *ThisImport = BuildOneImportedTarget( ImportsListElement->Element, m_SourceFile->GetCompilerProject(), false );

        if (!ThisImport)
        {
            continue;
        }

        // Put it on the list of imports
        ThisImport->m_pNext = *ImportsList;
        *ImportsList = ThisImport;
    }
}

ImportedTarget* Declared::BuildOneImportedTarget
(
    ParseTree::ImportDirective *ImportDirective, // [in] the Imports parse tree
    CompilerProject *Project,
    bool isProjectLevelImport
)
{
    ImportedTarget *ThisImport;

    if (ImportDirective->Mode == ParseTree::ImportDirective::XmlNamespace)
    {
        ParseTree::Expression *NamespaceDeclaration = ImportDirective->AsXmlNamespace()->NamespaceDeclaration;
        ParseTree::IdentifierDescriptor Prefix;
        ILTree::StringConstantExpression *NamespaceExpr;

        // Bind the expression, and report any semantic errors
        Semantics Analyzer(&m_ScratchAllocator, m_ErrorTable, m_CompilerInstance, m_CompilerHost, m_SourceFile, NULL, false);
        if (!Analyzer.ExtractXmlNamespace(NamespaceDeclaration, Prefix, NamespaceExpr)) return NULL;

        // Only validate the prefix if one exists
        if (StringPool::StringLength(Prefix.Name) != 0)
        {
            // Validate prefix
            if (!ValidateXmlName(Prefix.Name, Prefix.TextSpan, m_ErrorTable)) return NULL;
        }

        // Validate the namespace. Pass true so that special import validation rules are used because
        // imports allows xmlns:p="" which is not legal in an xml literal.
        if (Prefix.Name != STRING_CONST(m_CompilerInstance, XmlNs) || !isProjectLevelImport)
        {
            if (!ValidateXmlNamespace(Prefix, NamespaceExpr, m_ErrorTable, m_CompilerInstance, true)) return NULL;
        }

        ThisImport = m_SymbolAllocator.AllocImportedTarget(0);
        ThisImport->m_IsXml = true;
        ThisImport->m_pstrAliasName = Prefix.Name;
        ThisImport->m_pstrQualifiedName = m_CompilerInstance->AddStringWithLen(NamespaceExpr->Spelling, NamespaceExpr->Length);

        // File-level imports have tracked locations and therefore must be declared now.  Wait until bound phase to create project-level import symbol.
        if (!isProjectLevelImport)
            ThisImport->m_pTarget = m_SymbolAllocator.GetXmlNamespaceDeclaration(ThisImport->m_pstrQualifiedName, m_SourceFile, Project, &ImportDirective->TextSpan, true);
    }
    else
    {
        // Imports bob.sally = alias, jane.dawn = alias2, etc.
        // Walk over the names defined off this statement and create an ImportedNamespace on the importednamespaces list.
        // Skip those that are bad.
        if (DetermineIfBadName(ImportDirective->AsNamespace()->ImportedName)) return NULL;

        // Allocate and populate the import structure.
        unsigned NumDotDelimitedNames = ImportDirective->AsNamespace()->ImportedName->GetDelimitedNameCount();
        ThisImport = m_SymbolAllocator.AllocImportedTarget(NumDotDelimitedNames);

        if (ImportDirective->Mode != ParseTree::ImportDirective::Namespace)
        {
            ThisImport->m_pstrAliasName = ImportDirective->AsAlias()->Alias.Name; // Grab the alias name
        }

        ThisImport->m_NumDotDelimitedNames = NumDotDelimitedNames;

#if IDE
        // In case of a file level import, we need to get unnamed namespace chunk, which corresponds to the source file
        // we are building. Otherwise, we'll get symbols with context belonging to another file, and the context can be 
        // deleted while the symbol is still valid (decompilation of a single file in the project). 
        // This is a serious issue when a file level import target is a generic type.
        BCSYM_Namespace* pUnnamedNamespace;

        if (isProjectLevelImport || m_SourceFile == NULL)
        {
            pUnnamedNamespace = m_CompilerInstance->GetUnnamedNamespace(Project);
        }
        else
        {
            pUnnamedNamespace = m_CompilerInstance->GetUnnamedNamespace(m_SourceFile);
        }

        FillQualifiedNameArray(NumDotDelimitedNames,
                               ImportDirective->AsNamespace()->ImportedName,
                               &ThisImport->m_DotDelimitedNames[ 0 ],
                               &ThisImport->m_ppTypeArguments, // ArgumentsArray
                               pUnnamedNamespace,    // Context
                               NULL,                           // Don't put any named types created during this
                                                               // process in the normal per container named types
                                                               // list. Imports are special and handled differently.
                               false    // the parser already signaled an error if the name is GLobalNameSpace based,
                                        // ignore IsGlobalNameSpaceBased flag and the "GlobalNameSpace" keyword
                               );
#else

        FillQualifiedNameArray(NumDotDelimitedNames,
                       ImportDirective->AsNamespace()->ImportedName,
                       &ThisImport->m_DotDelimitedNames[ 0 ],
                       &ThisImport->m_ppTypeArguments, // ArgumentsArray
                       m_CompilerInstance->GetUnnamedNamespace(Project),    // Context
                       NULL,                           // Don't put any named types created during this
                                                       // process in the normal per container named types
                                                       // list. Imports are special and handled differently.
                       false    // the parser already signaled an error if the name is GLobalNameSpace based,
                                // ignore IsGlobalNameSpaceBased flag and the "GlobalNameSpace" keyword
                       );
#endif

        // 


        ThisImport->m_pstrQualifiedName = m_CompilerInstance->ConcatStringsArray( NumDotDelimitedNames, (const WCHAR **)ThisImport->m_DotDelimitedNames, WIDE("."));
    }

    ThisImport->m_loc.SetLocation(&(ImportDirective->TextSpan));
    ThisImport->m_IsProjectLevelImport = isProjectLevelImport;

    return ThisImport;
}

ImportedTarget*
Declared::BuildProjectLevelImport
(
    ParseTree::ImportDirective *ImportDirective,
    NorlsAllocator *Allocator,
    ErrorTable *ErrorLog,
    CompilerProject *Project
)
{
    Declared Declared(
        Allocator,
        NULL,
        NULL,
        ErrorLog,
        Project->GetCompilerHost(),
        NULL);

    ImportedTarget *Import = Declared.BuildOneImportedTarget( ImportDirective, Project, true );

    return Import;

}

/*========================================================================

Routines to deal with attributes

========================================================================*/

/*****************************************************************************
;ProcessAttributeList

Stores the custom attributes from the trees into the symbol.
*****************************************************************************/
void Declared::ProcessAttributeList
(
    ParseTree::AttributeSpecifierList *Attributes, // [in] the parse tree for the attributes
    BCSYM *Symbol, // [in] the symbol the attribute is defined on
    BCSYM_NamedRoot *Context // [optional-in] the context in which this attribute was found, e.g. if symbol is a param then this would be the owning proc, etc.
)
{
    VSASSERT( Symbol->IsNamedRoot() || Symbol->IsParam(), "Bad param!" );

    for ( ; Attributes; Attributes = Attributes->Next)
    {
        ParseTree::AttributeSpecifier *AttributeBlock = Attributes->Element;
        ParseTree::AttributeList *AttributeList;

        for ( AttributeList = AttributeBlock->Values; AttributeList; AttributeList = AttributeList->Next )
        {
            ParseTree::Attribute * Attribute = AttributeList->Element;
            BCSYM_NamedType * AttributeName;
            BCSYM_ApplAttr * AttributeSymbol;

            // NamedRoots must not pass context; Params must
            VSASSERT((Symbol->IsNamedRoot() && Context == NULL) ||
                    (Symbol->IsParam()     && Context != NULL), "Bad attribute context!" );

            if (Attribute->DeferredRepresentationAsCall == NULL)
            {
                // Some syntax error was reported on this attribute. Don't put
                // it in the symbol table and don't hang it off of Symbol.
                continue;
            }

            // Create the Named Type symbol for the name of the attribute.
            AttributeName = MakeNamedType( AttributeList->Element->Name,
                                           Context ? Context : Symbol->PNamedRoot(),
                                           GetTypeList(),
                                           0,       //IsTHISTYpe
                                           true,    // Applied attr context
                                           NULL);  // ArgumentNullable

            AttributeName->SetIsAttributeName(true);

            // Finally create the ApplAttr class to store the attribute information and attach it to the symbol.

            AttributeSymbol = m_SymbolAllocator.GetApplAttr( AttributeName,
                                                    &Attribute->TextSpan,
                                                    Attribute->IsAssembly,
                                                    Attribute->IsModule);


            AttributeSymbol->SetExpression( m_SymbolAllocator.GetConstantExpression( 
                                                    //&Attribute->TextSpan, //Fix for bug B32565, when Assembly or module the loc of DeferredExpr 
                                                    // is different than Atribute location               
                                                    &Attribute->DeferredRepresentationAsCall->TextSpan,
                                                    Symbol->IsNamedRoot() ? Symbol->PNamedRoot() : NULL, // context
                                                    NULL, // no const expr list
                                                    NULL, // no const expr list
                                                    NULL, // no fake string
                                                    Attribute->DeferredRepresentationAsCall->Text, // the expression
                                                    -1 ) // figure the size out based on what we pass in for the expression
                                            );
            if ( !Symbol->GetPAttrVals())
            {
                VSASSERT(m_SourceFile != NULL, "How can the source file be NULL?");
                Symbol->AllocAttrVals(m_SymbolAllocator.GetNorlsAllocator(), CS_Declared, m_SourceFile, GetCurrentContainerContext());
            }
            else
            {
                AttributeSymbol->SetNext(Symbol->GetPAttrVals()->GetPNon----edData()->m_psymApplAttrHead);
            }

            Symbol->GetPAttrVals()->GetPNon----edData()->m_psymApplAttrHead = AttributeSymbol;
        }

    }

    // If this parameter has attributes attached, record the context so we can process the attribute in that context later.
    if ( Context && ( Symbol->GetPAttrVals() != NULL ))
    {
        Symbol->GetPAttrVals()->GetPNon----edData()->m_psymParamContext = Context;
    }
}

/*========================================================================

Routines to deal with XMLDoc comments

========================================================================*/

/*****************************************************************************
;ExtractXMLDocComments

    Extracts any XMLDoc comments that are attached to this statementlist, parses it
    and validates that it is a valid XML for this element. Appropriate erros are
    generated here.
*****************************************************************************/
void Declared::ExtractXMLDocComments
(
    ParseTree::StatementList *StatementListTree,
    BCSYM_NamedRoot *pNamedElement
)
{
    VSASSERT( StatementListTree && StatementListTree->Element && pNamedElement, "Bad params!" );

    // Only do this if XMLDoc comment feature is turned on.
    if (m_SourceFile && m_SourceFile->GetProject() && m_SourceFile->GetProject()->IsXMLDocCommentsOn())
    {
        bool SeenXMLDocComment = false;         // Only one XMLDoc comment per code element
        ParseTree::StatementList *pPreviousBlock = StatementListTree->PreviousInBlock;
        ParseTree::Statement     *pStatementElement = NULL;

        while (pPreviousBlock && pPreviousBlock->Element->Opcode != ParseTree::Statement::CommentBlock && pPreviousBlock->Element->IsPlacebo())
        {
            pPreviousBlock = pPreviousBlock->PreviousInBlock;
        }

        // We walk backwards from the statement representing the code element. This allows us to
        // use the first XMLDoc comment block and to flag any others as errors.
        if (pPreviousBlock)
        {
            pStatementElement = pPreviousBlock->Element;
        }

        if (pStatementElement && pStatementElement->Opcode == ParseTree::Statement::CommentBlock &&
            pStatementElement->AsCommentBlock()->IsXMLDocComment)
        {

            // There appears to be a valid XMLDoc comment block attached to this code element, go ahead and build
            // the XML comment info.
            TrackedLocation *pTrackedLocation = m_SourceFile->GetXMLDocFile()->AddXMLDocComment(pStatementElement->AsCommentBlock(), pNamedElement);

            // Don't report any errors on this CommentBlock because it is a good one.
            RemoveCommentBlock(pStatementElement->AsCommentBlock());

#if IDE
            if (pTrackedLocation && m_SourceFile && m_SourceFile->GetLineMarkerTable())
            {
                m_SourceFile->GetLineMarkerTable()->AddDeclLocation(pTrackedLocation, NULL);
            }
#endif IDE

            SeenXMLDocComment = true;       // Any other XMLdoc comments are considered errors.

            pPreviousBlock = pPreviousBlock->PreviousInBlock;
        }

        if (pPreviousBlock)
        {
            pStatementElement = pPreviousBlock->Element;
        }

        // Walk backwards looking for all comment blocks
        while (pPreviousBlock && pStatementElement)
        {
            if (pStatementElement->Opcode == ParseTree::Statement::CommentBlock)
            {
                if (pStatementElement->AsCommentBlock()->IsXMLDocComment)
                {
                    Location XMLDocCommentBlockLocation = GetDefaultXMLDocErrorLocation(pStatementElement->AsCommentBlock()->BodyTextSpan);

                    if (SeenXMLDocComment)
                    {
                        // This is not the first XMLDoc comment block, report an error
                        ReportError(WRNID_XMLDocMoreThanOneCommentBlock, &XMLDocCommentBlockLocation);

                        // Don't report any errors on this CommentBlock because we already did.
                        RemoveCommentBlock(pStatementElement->AsCommentBlock());
                    }
                    else
                    {
                        // We have seen something else first other than an XMLDoc comment block, so this block must be in error.
                        ReportError(WRNID_XMLDocBadXMLLine, &XMLDocCommentBlockLocation);

                        // Don't report any errors on this CommentBlock because we already did.
                        RemoveCommentBlock(pStatementElement->AsCommentBlock());

                        SeenXMLDocComment = true;
                    }
                }
            }
            else
            {
                if (!pStatementElement->IsPlacebo())
                {
                    break;
                }
            }

            pPreviousBlock = pPreviousBlock->PreviousInBlock;

            if (pPreviousBlock)
            {
                pStatementElement = pPreviousBlock->Element;
            }
        }
    }
}

/*****************************************************************************
;AddCommentBlock

    Adds an XML comment block to the list of comment blocks seen.
*****************************************************************************/
void Declared::AddCommentBlock
(
    ParseTree::CommentBlockStatement *AddedCommentBlock
)
{
    m_OrphanedXMLDocCommentBlocks.Insert(&(AddedCommentBlock->BodyTextSpan));
}

/*****************************************************************************
;RemoveCommentBlock

    Removes an XML comment block from the list of comment blocks seen.
*****************************************************************************/
void Declared::RemoveCommentBlock
(
    ParseTree::CommentBlockStatement *RemovedCommentBlock
)
{
    bool WasRemoved = m_OrphanedXMLDocCommentBlocks.Remove(&(RemovedCommentBlock->BodyTextSpan));

    VSASSERT(WasRemoved, "XMLDoc comment node does not exist in the RedBlack tree!");
}


/*****************************************************************************
;ReportAllOrphanedXMLDocCommentErrors

    Report all errors for XMLDoc comment blocks that could not be associated with any valid language elements.
*****************************************************************************/
void Declared::ReportAllOrphanedXMLDocCommentErrors
(
)
{
    RedBlackTreeT<Location, LocationOperations>::Iterator TreeIterator(&m_OrphanedXMLDocCommentBlocks);
    RedBlackTreeT<Location, LocationOperations>::RedBlackNode *pTreeNode = NULL;

    while (pTreeNode = TreeIterator.Next())
    {
        ReportError(WRNID_XMLDocWithoutLanguageElement, &GetDefaultXMLDocErrorLocation(pTreeNode->key));
    }
}

/*========================================================================

Routines dealing with generics

========================================================================*/

void Declared::MakeGenericParameters
(
    ParseTree::GenericParameterList *GenericParamsTree,
    BCSYM_GenericParam **GenericParams,
    BCSYM_NamedRoot *Context,
    _In_opt_z_ STRING *ContextName,
    bool ContextIsFunction
)
{
    BCSYM_GenericParam **ExistingParameters = GenericParams;
    unsigned ParameterIndex = 0;

    for ( ParseTree::GenericParameterList *ParamList = GenericParamsTree; ParamList; ParamList = ParamList->Next )
    {
        ParseTree::GenericParameter *ParamTree = ParamList->Element;

        // Ignore params with bad or no names so that they don't create problems down the line
        //
        if (ParamTree->Name.IsBad)
        {
            continue;
        }

        VSASSERT(ParamTree->Name.Name, "How can a good param not have a name ?");

        if ( ParamTree->Name.TypeCharacter != chType_NONE )
        {
            ReportError( ERRID_TypeCharOnGenericParam, &ParamTree->Name.TextSpan );
        }

        // For functions, type params cannot clash with the return variable name which is the same as the function
        // name. Note that this is not a n issue for subs
        //
        if ( ContextIsFunction &&
             ContextName &&
             StringPool::IsEqual( ParamTree->Name.Name, ContextName))
        {
            ReportError( ERRID_TypeParamNameFunctionNameCollision, &ParamTree->Name.TextSpan );

            // Don't set bad - only the name is horked and it would be interesting to do overload/overriding/etc semantics on it
        }

        Variance_Kind Variance = Variance_None;
        switch (ParamTree->Variance)
        {
            case ParseTree::GenericParameter::Variance_Out:
                Variance=Variance_Out;
                break;
            case ParseTree::GenericParameter::Variance_In:
                Variance=Variance_In;
                break;
            case ParseTree::GenericParameter::Variance_None:
                Variance=Variance_None;
                break;
            default:
                VSFAIL("Unexpected variance in parse-tree");
        }

        BCSYM_GenericParam *Param =
            m_SymbolAllocator.GetGenericParam( &ParamTree->Name.TextSpan,
                                               ParamTree->Name.Name,
                                               ParameterIndex++,
                                               Context->IsProc(),
                                               Variance);

        BCSYM_GenericConstraint **Constraints = Param->GetConstraintsTarget();
        bool fNewConstraintSeen = false;
        bool fReferenceConstraintSeen = false;
        bool fValueConstraintSeen = false;

        for ( ParseTree::ConstraintList *ConstraintList = ParamTree->Constraints; ConstraintList; ConstraintList = ConstraintList->Next )
        {
            ParseTree::Constraint *Constraint = ConstraintList->Element;
            BCSYM_GenericConstraint *ConstraintSymbol = NULL;

            // Build and link the constraint symbol

            switch (Constraint->Opcode)
            {
                case ParseTree::Constraint::New :
                {
                    if (fNewConstraintSeen)
                    {
                        ReportError(ERRID_MultipleNewConstraints, &Constraint->TextSpan);
                    }
                    else if (fValueConstraintSeen)
                    {
                        ReportError(ERRID_NewAndValueConstraintsCombined, &Constraint->TextSpan);
                    }
                    else
                    {
                        fNewConstraintSeen = true;

                        ConstraintSymbol =
                            m_SymbolAllocator.GetGenericNonTypeConstraint( &Constraint->TextSpan,
                                                                           BCSYM_GenericNonTypeConstraint::ConstraintKind_New );
                    }

                    break;
                }

                case ParseTree::Constraint::Class :
                {
                    if (fReferenceConstraintSeen)
                    {
                        ReportError(ERRID_MultipleReferenceConstraints, &Constraint->TextSpan);
                    }
                    else if (fValueConstraintSeen)
                    {
                        ReportError(ERRID_RefAndValueConstraintsCombined, &Constraint->TextSpan);
                    }
                    else
                    {
                        fReferenceConstraintSeen = true;

                        ConstraintSymbol =
                            m_SymbolAllocator.GetGenericNonTypeConstraint( &Constraint->TextSpan,
                                                                           BCSYM_GenericNonTypeConstraint::ConstraintKind_Ref );
                    }

                    break;
                }

                case ParseTree::Constraint::Struct :
                {
                    if (fValueConstraintSeen)
                    {
                        ReportError(ERRID_MultipleValueConstraints, &Constraint->TextSpan);
                    }
                    else if (fNewConstraintSeen)
                    {
                        ReportError(ERRID_NewAndValueConstraintsCombined, &Constraint->TextSpan);
                    }
                    else if (fReferenceConstraintSeen)
                    {
                        ReportError(ERRID_RefAndValueConstraintsCombined, &Constraint->TextSpan);
                    }
                    else
                    {
                        fValueConstraintSeen = true;

                        ConstraintSymbol =
                            m_SymbolAllocator.GetGenericNonTypeConstraint( &Constraint->TextSpan,
                                                                           BCSYM_GenericNonTypeConstraint::ConstraintKind_Value );
                    }

                    break;
                }

                case ParseTree::Constraint::Type:
                {
                    VSASSERT(Constraint->Opcode == ParseTree::Constraint::Type, "Unexpected constraint kind!!!");

                    ParseTree::Type *ConstraintTypeTree = Constraint->AsType()->Type;

                    BCSYM *ConstraintType =
                        MakeType( ConstraintTypeTree,
                                NULL,
                                Context,
                                GetTypeList(),
                                0);

                    ConstraintSymbol = m_SymbolAllocator.GetGenericTypeConstraint( &ConstraintTypeTree->TextSpan, ConstraintType );

                    break;
                }

                default:
                    VSFAIL("Unexpected constraint kind!!!");
            }

            if (ConstraintSymbol != NULL)
            {
                *Constraints = ConstraintSymbol;
                Constraints = ConstraintSymbol->GetNextConstraintTarget();
            }
        }


        // Verify that this parameter does not have the same name as another.
        // (This is an N-squared algorithm, but the expected number of generic parameters is small.)

        for ( BCSYM_GenericParam *CheckParam = *ExistingParameters; CheckParam; CheckParam = CheckParam->GetNextParam())
        {
            if ( !CheckParam->IsBad() && StringPool::IsEqual( CheckParam->GetName(), ParamTree->Name.Name ))
            {
                ReportError( ERRID_DuplicateTypeParamName1, &ParamTree->Name.TextSpan, ParamTree->Name.Name );
                // Don't set bad - only the name is horked and it would be interesting to do overload/overriding/etc semantics on it
                // Once we've logged an error on one param, there is no need to log an error on all other params.
                break;
            }
        }

        *GenericParams = Param;
        GenericParams = Param->GetNextParamTarget();
    }
}

/*========================================================================

Routines to build types

========================================================================*/

/*****************************************************************************
;MakeClassChildren

Given Class x or Module x, this builds all the symbols that belong to the class/module
*****************************************************************************/
void Declared::MakeClassChildren
(
    ParseTree::BlockStatement *ClassTree, // [in] the tree for the module
    DECLFLAGS ContainerDeclFlags,    // [in] the flags for the container where these children are defined
    bool &HasMustOverrideMethod, // [in/out] whether we've seen a must override method
    bool &SeenConstructor,       // [in/out] whether we've seen a constructor
    bool &SeenSharedConstructor, // [in/out] whether we've seen a shared constructor
    bool &NeedSharedConstructor, // [in/out] whether a shared constructor is needed
    bool HasGenericParent,       // [in] whether any of the class children's enclosing types in a generic
    BCSYM_Class *OwningClass,    // [in] class that owns the symbols being built
    SymbolList *OwningSymbolList,// [in] symbol list to add the class children to
    SymbolList *BackingFieldsForStaticsList, // [in] we put class fields that back local statics on here
    BCSYM **BaseClassSymbol,     // [out] the base class, i.e. the class this one inherits from
    BCSYM_Implements **ImplementsSymbolList, // [out] the implements list for this class
    BCSYM_Property **DefaultProperty // [out] set if we built a default property
)
{
    bool SeenInherits = false; // whether we have encountered an Inherits statement in the class
    bool TooLateForInherits = false; // Inherits can't come after data members are defined
    bool SeenImplements = false; // Inherits can't come after data members, but must come after Inherits


    // Microsoft: 



    // Iterate through all the declaration trees in this module/class and build symbols for them
    for ( ParseTree::StatementList *CurrentStatement = ClassTree->Children;
           CurrentStatement;
           CurrentStatement = CurrentStatement->NextInBlock )
    {
        switch ( CurrentStatement->Element->Opcode )
        {
            case ParseTree::Statement::Inherits:

                if ( ClassTree->Opcode != ParseTree::Statement::Structure ) // Only classes can inherit
                {
                    if ( SeenInherits )
                    {
                        ReportError( ERRID_MultipleExtends, &CurrentStatement->Element->TextSpan );
                    }
                    else if ( TooLateForInherits || SeenImplements )
                    {
                        ReportError( ERRID_InheritsStmtWrongOrder, &CurrentStatement->Element->TextSpan );
                    }
                    else
                    {
                        // Inherits is not allowed in a standard module.
                        if ( ContainerDeclFlags & DECLF_Module )
                        {
                            ReportError( ERRID_ModuleCantInherit, &CurrentStatement->Element->TextSpan );
                        }
                        else // Build the inherits statement
                        {
                            MakeInherits( CurrentStatement->Element->AsTypeList(), OwningClass, BaseClassSymbol );
                            // If we built a type for an intrinsic symbol - there isn't any location information for it.  Cobble some up.
                            if ( !(*BaseClassSymbol)->HasLocation())
                            {
                               OwningClass->SetInheritsLocation( &CurrentStatement->Element->AsTypeList()->Types->TextSpan, m_SymbolAllocator.GetNorlsAllocator());
                            }
                        }
                        SeenInherits = true;
                    }
                }
                else
                {
                    ReportError( ERRID_StructCantInherit, &CurrentStatement->Element->TextSpan );
                }
                continue;

            case ParseTree::Statement::Implements:

                // Implements is not allowed in a standard module.
                if ( ContainerDeclFlags & DECLF_Module )
                {
                    ReportError( ERRID_ModuleCantImplement, &CurrentStatement->Element->TextSpan );
                }
                else
                {
                    if ( TooLateForInherits )
                    {
                        ReportError( ERRID_ImplementsStmtWrongOrder, &CurrentStatement->Element->TextSpan );
                    }
                    MakeClassImplements( CurrentStatement->Element->AsTypeList(), OwningClass, ImplementsSymbolList );
                }
                SeenImplements = true;
                continue;

            case ParseTree::Statement::VariableDeclaration:

                MakeMemberVar( CurrentStatement,
                               ContainerDeclFlags,
                               NeedSharedConstructor,
                               OwningSymbolList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::EventDeclaration:
            case ParseTree::Statement::BlockEventDeclaration:

                MakeEvent( CurrentStatement,
                           false, // not in an interface
                           ContainerDeclFlags,
                           OwningSymbolList,
                           BackingFieldsForStaticsList,
                           NeedSharedConstructor );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::OperatorDeclaration:

                MakeOperator( CurrentStatement,
                              OwningClass,
                              ContainerDeclFlags,
                              NeedSharedConstructor,
                              OwningSymbolList,
                              BackingFieldsForStaticsList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::ProcedureDeclaration:
            case ParseTree::Statement::ConstructorDeclaration:

                MakeMethodImpl( CurrentStatement,
                                ContainerDeclFlags,
                                false,
                                HasMustOverrideMethod,
                                SeenConstructor,
                                SeenSharedConstructor,
                                NeedSharedConstructor,
                                OwningSymbolList,
                                BackingFieldsForStaticsList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::DelegateFunctionDeclaration:
            case ParseTree::Statement::DelegateProcedureDeclaration:

                MakeDelegate( CurrentStatement, true, ContainerDeclFlags, OwningSymbolList, NULL );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::ForeignFunctionDeclaration:
            case ParseTree::Statement::ForeignProcedureDeclaration:

                MakeDllDeclared( CurrentStatement, ContainerDeclFlags, OwningSymbolList );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::AutoProperty:
            case ParseTree::Statement::Property:
            {
                BCSYM_Property *Prop = MakeProperty( CurrentStatement,
                                                     ContainerDeclFlags,
                                                     false, // not defined in an interface
                                                     OwningSymbolList,
                                                     NeedSharedConstructor,
                                                     BackingFieldsForStaticsList );

                if ( !*DefaultProperty && Prop && Prop->IsDefault())
                {
                    *DefaultProperty = Prop; // Keep track of the first default property we find (bindable will deal with duplicates)
                }
                TooLateForInherits = true;
                continue;
            }
            case ParseTree::Statement::Interface:

                MakeInterface( CurrentStatement,
                               true /* defined within a class */,
                               false /* not at namespace level */,
                               HasGenericParent,
                               OwningSymbolList,
                               ContainerDeclFlags );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Class:

                MakeClass( CurrentStatement,
                           true, // nested,
                           false, // don't build a std module
                           false, // not defined in an interface
                           HasGenericParent,
                           ContainerDeclFlags,
                           OwningSymbolList );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Structure:

                MakeStructure( CurrentStatement,
                               true, // nested
                               false, // not defined within an interface
                               HasGenericParent,
                               ContainerDeclFlags,
                               OwningSymbolList );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Enum:

                MakeEnum( CurrentStatement,
                          true, // nested
                          false, // no Sam I am, we aren't defining it in an interface
                          HasGenericParent,
                          ContainerDeclFlags,
                          OwningSymbolList );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::CommentBlock:

                if ( CurrentStatement->Element->AsCommentBlock()->IsXMLDocComment == true )
                {
                    AddCommentBlock( CurrentStatement->Element->AsCommentBlock() );
                }

                continue;

            default:

                continue; // Every other kind of statement we don't care about building symbols for ( e.g. all the End_* trees, etc. )
        }
    }
}

/*****************************************************************************
;MakeInherits

Build the symbol for Inherits from the tree
*****************************************************************************/
void Declared::MakeInherits(
    ParseTree::TypeListStatement *InheritsStatementTree, // [in] the tree representing the inherits statement
    BCSYM_Class *ContainingClass,
    BCSYM **InheritsSymbol // [out] the built inherits symbol
)
{
    // We want to use this Inherits statement only if it is the first one we have seen or if it is an Inherits statement defined in hidden
    // code that overrides one defined in user code.

    ParseTree::TypeList *InheritsList;
    InheritsList = InheritsStatementTree->Types;

    // Build the type for this Inherits - We will verify that this is the correct type after we bind the named types.
    *InheritsSymbol = MakeType( InheritsList->Element, NULL, ContainingClass, NULL );

    // We only allow single inheritance - check to see if there are more classes in the inheritance list
    if ( InheritsList->Next )
    {
        ReportError( ERRID_MultipleExtends, &InheritsStatementTree->TextSpan );
    }
}

/*****************************************************************************
;MakeClassImplements

Process a class level Implements statement
*****************************************************************************/
void Declared::MakeClassImplements
(
    ParseTree::TypeListStatement *ImplementsTree, // [in] the tree representing the implements statement
    BCSYM_Class *ContainingClass, // [in] the class doing the Implementing
    BCSYM_Implements **ImplementsSymbolList // [out] the built implements symbol is appended to this list
)
{
    // Loop over the list of implemented interfaces.
    for ( ParseTree::TypeList * InheritsListElement = ImplementsTree->Types;
          InheritsListElement;
          InheritsListElement = InheritsListElement->Next )
    {
        // Get the type for this statement.
        BCSYM *TypeOfImplements = MakeType( InheritsListElement->Element, NULL, ContainingClass, GetTypeList() );

        if ( TypeOfImplements->IsBad()) // Ignore interfaces that have problems
        {
            continue;
        }
        else if ( !TypeOfImplements->IsNamedType()) // This must be a named type symbol.
        {
            ReportError( ERRID_BadImplementsType, &InheritsListElement->Element->TextSpan );
        }
        else
        {
            Location loc;
            loc.SetLocation( &InheritsListElement->Element->TextSpan );

            // Create the implements symbol.
            m_SymbolAllocator.GetImplements( &loc,
                                             TypeOfImplements->PNamedType()->GetHashingName(),
                                             TypeOfImplements->PNamedType(),
                                             ImplementsSymbolList ); // created Implements symbol gets tacked on to the end of the ImplementsSymbolList
        }
    } // loop list of interfaces
}

/*****************************************************************************
;MakeClass

Builds a Class/Module symbol from parse trees
*****************************************************************************/
void Declared::MakeClass
(
    ParseTree::StatementList *StatementListTree, // [in] the Class or Module parse tree
    bool IsNested, // [in] is this a nested type?
    bool BuildStdModule, // [in] are we supposed to build a std. module?
    bool DefinedInInterface, // [in] is this class defined in an interface?
    bool HasGenericParent,   // [in] whether the class has an enclosing generic type
    DECLFLAGS ContainerFlags, // [in] the flags for the container where this class is defined
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    BCSYM *Inherits = NULL;
    DECLFLAGS DeclFlags = 0;
    BCSYM_Implements *ImplementsSymbolList = NULL;
    bool SeenConstructor = false;
    bool SeenSharedConstructor = false;
    bool NeedSharedConstructor = false;
    bool NeedMain = false;
    bool BuiltInsideStdModule = ContainerFlags & DECLF_Module;
    bool DefinedInStructure = ContainerFlags & DECLF_Structure; // class is defined in a structure?
    SymbolList ClassChildrenSymbolList; // symbols that belong to the class are captured in here
    BCSYM_GenericParam *FirstGenericParam = NULL;
    SymbolList *BackingFieldsForStaticsList = (SymbolList *) m_SymbolAllocator.GetNorlsAllocator()->Alloc( sizeof( SymbolList )); // statics for all methods in the class stored here.
    ParseTree::TypeStatement *ClassTree = StatementListTree->Element->AsType();
    bool IsTypeAccessExplictlySpecified = true;
    bool IsBaseExplicitlySpecified = true;

    // Create an empty module symbol to use as our context.
    BCSYM_Class *Class = m_SymbolAllocator.AllocClass( TRUE /* has location */ );

    // If we don't have a name, there isn't much to do.
    if ( ClassTree->Name.IsBad ) return;

    bool HasMustOverrideMethod = false;
    STRING *ClassName = ClassTree->Name.Name;
    if ( ClassTree->Name.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, &ClassTree->Name.TextSpan );
    }

    DeclFlags = MapSpecifierFlags( ClassTree->Specifiers ); // Get the specifiers from the parse tree

    if ( DefinedInInterface && ( DeclFlags & DECLF_InvalidInterfaceMemberFlags ))
    {
        ReportDeclFlagError( ERRID_BadInterfaceClassSpecifier1, DeclFlags & DECLF_InvalidInterfaceMemberFlags, ClassTree->Specifiers );
        DeclFlags &= ~DECLF_InvalidInterfaceMemberFlags;
    }

    // Only access flags are legal.
    if ( BuildStdModule )
    {
        const DECLFLAGS DECLF_ValidStdModuleFlags = ( DECLF_AccessFlags );

        if ( DeclFlags & ~DECLF_ValidStdModuleFlags )
        {
            ReportDeclFlagError( ERRID_BadModuleFlags1, DeclFlags & ~DECLF_ValidStdModuleFlags, ClassTree->Specifiers );
            DeclFlags &= DECLF_ValidStdModuleFlags;
        }
    }
    else // This is a normal class
    {
        const DECLFLAGS DECLF_ValidClassFlags = ( DECLF_AccessFlags | DECLF_MustInherit | DECLF_NotInheritable | DECLF_ShadowsKeywordUsed | DECLF_Partial );

        if ( DeclFlags & ~DECLF_ValidClassFlags )
        {
            ReportDeclFlagError( ERRID_BadClassFlags1, DeclFlags & ~DECLF_ValidClassFlags, ClassTree->Specifiers );
            DeclFlags &= DECLF_ValidClassFlags;
        }
    }

    // Set the current container context
    SetContainerContext(Class);

    // Can't be NotInheritable and MustInherit at the same time
    if (( DeclFlags & DECLF_MustInherit ) && ( DeclFlags & DECLF_NotInheritable ))
    {
        ReportDeclFlagComboError( DECLF_MustInherit | DECLF_NotInheritable, ClassTree->Specifiers, DECLF_MustInherit );
        DeclFlags &= ~DECLF_MustInherit;
    }

    // Only nested classes can be Protected.
    if (( DeclFlags & ( DECLF_Protected | DECLF_ProtectedFriend )) && ( !IsNested || DefinedInStructure ))
    {
        ReportError( ERRID_ProtectedTypeOutsideClass, &( ClassTree->Name.TextSpan ));
        DeclFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
    }

    // Only nested classes can be Private
    if (( DeclFlags & DECLF_Private ) && !IsNested )
    {
        ReportError( ERRID_PrivateTypeOutsideType, &( ClassTree->Name.TextSpan ));
    }

    // Only nested classes can be marked as shadows.
    if (( DeclFlags & ( DECLF_ShadowsKeywordUsed )) && !IsNested )
    {
        ReportError( ERRID_ShadowingTypeOutsideClass1, &( ClassTree->Name.TextSpan ), m_CompilerInstance->TokenToString(tkCLASS) );
        DeclFlags &= ~DECLF_ShadowsKeywordUsed;
    }

    // Classes built inside modules can't have certain specifiers
    if ( BuiltInsideStdModule )
    {
        if ( DeclFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseTypeSpecifier1, DeclFlags & DECLF_InvalidStdModuleMemberFlags, ClassTree->Specifiers, NULL );
            DeclFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }
    }

    // Nested Classes default to Public access if no access flags are set (Friend if they are at namespace level)
    if ( !( DeclFlags & DECLF_AccessFlags ))
    {
        DeclFlags |= (( IsNested ) ? DECLF_Public : DECLF_Friend );
        IsTypeAccessExplictlySpecified = false;
    }

    if ( BuildStdModule )
    {
        DeclFlags |= DECLF_Module;
    }

    MakeGenericParameters( ClassTree->GenericParameters,
                            &FirstGenericParam,
                            Class );

    BCSYM_Property *DefaultProperty = NULL; // keep track of the 1st discovered default property for the interface

    // Build the members of the class/module
    MakeClassChildren( ClassTree,
                       DeclFlags,
                       HasMustOverrideMethod,
                       SeenConstructor,
                       SeenSharedConstructor,
                       NeedSharedConstructor,
                       HasGenericParent || FirstGenericParam,
                       Class,
                       &ClassChildrenSymbolList, // puts the fields of the class on this list
                       BackingFieldsForStaticsList,
                       &Inherits,
                       &ImplementsSymbolList,
                       &DefaultProperty );

    if ( HasMustOverrideMethod )
    {
        DeclFlags |= DECLF_HasMustOverrideMethod;
    }

    // If this class doesn't inherit from anything, make it inherit from OBJECT
    if ( !Inherits )
    {
        Inherits = m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType);
        IsBaseExplicitlySpecified = false;
    }


    // Ok, this ----s, but here it is: If this class is listed as the startup object and it
    // doesn't have anything called main, generate one. It may be the case that the user
    // specified a non-Form class, but we can't actually check that until bindable.
    // So we'll defer until then and hope for the best

    if (StringPool::IsEqual(
            ConcatNameSpaceAndName(
                m_CompilerInstance,
                m_CurrentNamespaceContext,
                ClassName),
            m_SourceFile->GetProject()->GetStartupType()))
    {
        NeedMain = true;
        bool SeenAConstructor = false;
        bool IsValidConstructor = false;

        for ( BCSYM_NamedRoot *CurrentChild = ClassChildrenSymbolList.GetFirst();
        CurrentChild;
        CurrentChild = CurrentChild->GetNextInSymbolList())
        {
            if ( StringPool::IsEqual( STRING_CONST(m_CompilerInstance, Main), CurrentChild->GetName()))
            {
                NeedMain = false;
                break;
            }

            // Since the synthetic Sub Main hard codes a call to the constructor, e.g.:
            // System.Windows.Forms.Application.Run(New Form1))
            // we need to make sure that the constructor is parameterless, otherwise we are going to
            // generate an error message for the the body of the synthetic Main.
            //
            // Note that if the Form doesn't have it's own constructor, but inherits from another Form that does,
            // then we are going to generate a default paramerterless constructor for the derived form, so we should
            // never hit this case. See VS266648.
            if ( CurrentChild->IsProc() && !CurrentChild->IsBad() && CurrentChild->PProc()->IsAnyConstructor())
            {
                SeenAConstructor = true;

                if ( !CurrentChild->PProc()->GetFirstParam() || CurrentChild->PProc()->GetFirstParam()->IsOptional())
                {
                    IsValidConstructor = true;
                }
            }
        }

        // For the synthetic Sub Main to work, we should either not have a constructor at all in this class, or
        // at least one of the constructors in the class should take no parameters or takes optional parameters.
        // For any other case, we can't generate the synthatic Sub Main, and the user will need to provide their own.
        if ( SeenAConstructor && !IsValidConstructor)
        {
            NeedMain = false;
        }
    }

    // Create constructors, sub main, With Event symbols, etc. as appropriate
    VSASSERT( 0 == (DeclFlags & DECLF_Structure), "We do not expect to find a structure here" );
    CreateSynthesizedSymbols( &ClassChildrenSymbolList,
                            DeclFlags,
                            SeenConstructor,
                            SeenSharedConstructor,
                            NeedSharedConstructor,
                            NeedMain,
                            &ClassChildrenSymbolList);

    // Create the class symbol. For nested classes this is created in the enclosing scope while for file-level classes this is created at the project level
    m_SymbolAllocator.GetClass( ClassTree ? &ClassTree->Name.TextSpan : NULL,
                                ClassName, // class name
                                GetEmittedTypeNameFromActualTypeName( ClassName, FirstGenericParam, m_CompilerInstance ), // emitted name for class
                                m_CurrentNamespaceContext,
                                m_SourceFile,
                                Inherits,
                                ImplementsSymbolList,
                                DeclFlags,
                                t_bad,
                                m_SymbolAllocator.AllocVariable(false, false),
                                &ClassChildrenSymbolList, // the children symbols for the class will be added to the class container hash
                                GetUnBindableChildrenSymbolList(),
                                FirstGenericParam,
                                OwningSymbolList, // put the created class in the enclosing class or namespace
                                Class );

    Class->SetTypeAccessExplictlySpecified(IsTypeAccessExplictlySpecified);
    Class->SetBaseExplicitlySpecified(IsBaseExplicitlySpecified);

    Symbols::SetDefaultProperty( Class, DefaultProperty );
    Class->StowBackingFieldsForStatics( BackingFieldsForStaticsList, &m_SymbolAllocator );

    if ( ClassTree )
    {  // Store away the custom attributes.
       ProcessAttributeList( ClassTree->Attributes, Class );
    }

    AddContainerContextToSymbolList();
    PopContainerContext();

    // If we build a class symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this class symbol.
    if ( Class )
    {
        ExtractXMLDocComments( StatementListTree, Class );
    }
}

/*****************************************************************************
;MakeStructure

Build a Structure symbol from a parse tree
*****************************************************************************/
void Declared::MakeStructure
(
    ParseTree::StatementList *StatementListTree, // [in] the structure tree
    bool IsNested, // [in] is this a nested type?
    bool DefinedInInterface, // [in] is this being defined within an interface?
    bool HasGenericParent,   // [in] indicates whether any of its enclosing types is a generic
    DECLFLAGS ContainerDeclFlags, //  [in] the declaration flags for the container the structure is defined within
    SymbolList *OwningSymbolList  // [in] symbol list to add built structure to
)
{
    BCSYM *Inherits = NULL;
    DECLFLAGS DeclFlags = 0;
    STRING *StructName = NULL;
    SymbolList StructChildrenSymbolList;
    BCSYM_GenericParam *FirstGenericParam = NULL;
    SymbolList *BackingFieldsForStaticsList = (SymbolList *) m_SymbolAllocator.GetNorlsAllocator()->Alloc( sizeof( SymbolList )); // statics for all methods in the struct stored here.
    BCSYM_Implements *ImplementsSymbolList = NULL;
    bool SeenConstructor = false;
    bool HasMustOverrideMethod = false;
    bool SeenSharedConstructor = false;
    bool NeedSharedConstructor = false;
    bool DefinedInStdModule = ContainerDeclFlags & DECLF_Module; // [in] is the structure being defined within a standard module?
    bool DefinedInStruct = ContainerDeclFlags & DECLF_Structure;
    ParseTree::TypeStatement *StructTree = StatementListTree->Element->AsType();
    bool IsTypeAccessExplictlySpecified = true;

    // Save the current decls list and point to the new one.

    VSASSERT( StructTree, "You must provide a parse tree for the structure..." );

    // If we don't have a name, there isn't much to do.
    if ( StructTree->Name.IsBad ) return;

    // Create an empty class symbol to use as our context (structures are essentially classes - with a flag set that gets set below)
    BCSYM_Class *Structure = m_SymbolAllocator.AllocClass( TRUE /* has location */ );
    Structure->SetIsStruct(true); // we need to know this is a struct when building member vars. Set it now
    // By defualt Struct is SequentialLayout.

    StructName = StructTree->Name.Name;
    if ( StructTree->Name.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, &StructTree->Name.TextSpan );
    }

    DeclFlags = MapSpecifierFlags( StructTree->Specifiers ); // Get the specifiers from the parse tree

    // Only access flags (except Protected - dealt with below) and shadows are legal.
    const DECLFLAGS DECLF_ValidStructFlags = DECLF_AccessFlags | DECLF_ShadowsKeywordUsed | DECLF_Partial; // protected case taken care of below
    if ( DeclFlags & ~DECLF_ValidStructFlags )
    {
        ReportDeclFlagError( ERRID_BadRecordFlags1, DeclFlags & ~DECLF_ValidStructFlags, StructTree->Specifiers );
        DeclFlags &= DECLF_ValidStructFlags;
    }

    // set the current container context
    SetContainerContext(Structure);

    if ( DefinedInInterface && ( DeclFlags & DECLF_InvalidInterfaceMemberFlags ))
    {
        ReportDeclFlagError( ERRID_BadInterfaceStructSpecifier1, DeclFlags & DECLF_InvalidInterfaceMemberFlags, StructTree->Specifiers );
        DeclFlags &= ~DECLF_InvalidInterfaceMemberFlags;
    }

    if ( DefinedInStruct )
    {
        if ( DeclFlags & DECLF_InvalidFlagsInNotInheritableClass )
        {
            ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1, DeclFlags & DECLF_InvalidFlagsInNotInheritableClass, StructTree->Specifiers );
            DeclFlags &= ~DECLF_InvalidFlagsInNotInheritableClass;
        }
    }

    // Errors if the structure isn't nested
    if ( !IsNested )
    {
        // Only nested structures (not nested in a struct, nested in a class, etc. ) can be Protected.
        if (( DeclFlags & ( DECLF_Protected | DECLF_ProtectedFriend )))
        {
            ReportError( ERRID_ProtectedTypeOutsideClass, &( StructTree->Name.TextSpan ));
            DeclFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
        }

        // Only nested structures can be Private
        if ( DeclFlags & DECLF_Private )
        {
            ReportError( ERRID_PrivateTypeOutsideType, &( StructTree->Name.TextSpan ));
        }

        // Only nested structures can be marked as shadows.
        if ( DeclFlags & DECLF_ShadowsKeywordUsed )
        {
            ReportError( ERRID_ShadowingTypeOutsideClass1, &( StructTree->Name.TextSpan ), m_CompilerInstance->TokenToString( tkSTRUCTURE ));
            DeclFlags &= ~DECLF_ShadowsKeywordUsed;
        }
    }

    // Structs defined in a module can't have certain specifiers
    if ( DefinedInStdModule )
    {
        if ( DeclFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseTypeSpecifier1, DeclFlags & DECLF_InvalidStdModuleMemberFlags, StructTree->Specifiers, NULL );
            DeclFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }
    }

    // Structures default to Public access if no access flags are set (Friend if they are at namespace level)
    if ( !( DeclFlags & DECLF_AccessFlags ))
    {
        IsTypeAccessExplictlySpecified = false;
        DeclFlags |= (( IsNested ) ? DECLF_Public : DECLF_Friend );
    }

    MakeGenericParameters( StructTree->GenericParameters,
                            &FirstGenericParam,
                            Structure );

    DeclFlags |= DECLF_NotInheritable | DECLF_Structure; // All structures have these flags

    // Build the members of the structure
    BCSYM_Property *DefaultProperty = NULL; // keep track of the 1st discovered default property for the interface

    MakeClassChildren( StructTree,
                       DeclFlags,
                       HasMustOverrideMethod,
                       SeenConstructor,
                       SeenSharedConstructor,
                       NeedSharedConstructor,
                       HasGenericParent || FirstGenericParam,
                       Structure,
                       &StructChildrenSymbolList,
                       BackingFieldsForStaticsList,
                       &Inherits,
                       &ImplementsSymbolList,
                       &DefaultProperty );

    // Structures may not explicitly inherit from anything (they implicitly inherit from System.ValueType)
    VSASSERT( !Inherits, "MakeClassChildren was supposed to catch the fact that Structures can't inherit" );
    VSASSERT( !HasMustOverrideMethod, "MakeClassChildren was supposed to catch the fact that Structures can't have override methods" );

    // Structures implicitly inherit from System.ValueType
    Inherits = m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType);

    // Create any synthesized symbols
    VSASSERT( 0 == (DeclFlags & DECLF_Module), "Did not expect to create symbols for a module here" );
    VSASSERT( 0 != (DeclFlags & DECLF_Structure), "Expected to create symbols for a structure" );
    CreateSynthesizedSymbols( &StructChildrenSymbolList,
                              DeclFlags,
                              SeenConstructor,
                              SeenSharedConstructor,
                              NeedSharedConstructor,
                              false,
                              &StructChildrenSymbolList );

    // Create the structure symbol. For nested structures, this is created in the enclosing scope.  For file-level classes, this is created at project level
    m_SymbolAllocator.GetClass( StructTree ? &StructTree->Name.TextSpan : NULL,
                                StructName,
                                GetEmittedTypeNameFromActualTypeName( StructName, FirstGenericParam, m_CompilerInstance ),
                                m_CurrentNamespaceContext,
                                m_SourceFile,
                                Inherits,
                                ImplementsSymbolList,
                                DeclFlags,
                                t_bad,
                                m_SymbolAllocator.AllocVariable(false, false),
                                &StructChildrenSymbolList,
                                GetUnBindableChildrenSymbolList(),
                                FirstGenericParam,
                                OwningSymbolList, // put the created class on symbol list for its container - there is always a namespace container or containing class container
                                Structure );

    Structure->SetTypeAccessExplictlySpecified(IsTypeAccessExplictlySpecified);
    Symbols::SetDefaultProperty( Structure, DefaultProperty );
    Structure->StowBackingFieldsForStatics( BackingFieldsForStaticsList, &m_SymbolAllocator );

    // Store away the custom attributes.
    ProcessAttributeList( StructTree->Attributes, Structure );

    AddContainerContextToSymbolList();
    PopContainerContext();

    // If we build a structure symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this structure symbol.
    if ( Structure )
    {
        ExtractXMLDocComments( StatementListTree, Structure );
    }
}

/*****************************************************************************
;MakeEnum

Creates the symbol for an Enum symbol
*****************************************************************************/
void Declared::MakeEnum
(
    ParseTree::StatementList *StatementListTree,  // [in] the parse tree representing the enumeration
    bool IsNested, // [in] is this a nested enum?
    bool DefinedInInterface, // [in] in this enum defined in an interface?
    bool HasGenericParent,   // [in] indicates whether any of its enclosing types is a generic
    DECLFLAGS ContainerFlags, //  [in] the declaration flags for the container the structure is defined within
    SymbolList *OwningSymbolList  // [in] symbol list to add to
)
{
    SymbolList EnumMemberList;
    _int64 EnumMemberValue = 0;
    bool HaveSeenExpression = false;
    bool DefinedInStdModule = ContainerFlags & DECLF_Module; // is this enum defined in a std module?
    bool DefinedInStruct = ContainerFlags & DECLF_Structure; // is this enum defined in a struct?
    ParseTree::EnumTypeStatement *EnumTree = StatementListTree->Element->AsEnumType();
    bool IsTypeAccessExplictlySpecified = true;

    // If we don't have a name, there isn't much to do.
    if ( EnumTree->Name.IsBad )
    {
        return;
    }

    if ( !m_CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::EnumType))
    {
        m_CompilerHost->GetFXSymbolProvider()->ReportTypeMissingErrors(FX::EnumType, m_ErrorTable, EnumTree->Name.TextSpan);
        return;
    }

    // Get the enum symbol (we need it for the elements)
    BCSYM_Class *EnumSymbol = m_SymbolAllocator.AllocClass( EnumTree->Name.TextSpan.IsValid() /* has location? */ );

    // set the current container context
    SetContainerContext(EnumSymbol);

    // Bug 17864: we don't allow typeChars to follow an enum identifier
    if ( EnumTree->Name.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, &EnumTree->Name.TextSpan );
    }

    STRING *EnumName = EnumTree->Name.Name;

    // Set the flags.
    DECLFLAGS EnumFlags = MapSpecifierFlags( EnumTree->Specifiers );

    // Check for legal flags
    const DECLFLAGS ValidEnumFlags = DECLF_AccessFlags | DECLF_ShadowsKeywordUsed;
    if ( EnumFlags & ~ValidEnumFlags )
    {
        ReportDeclFlagError( ERRID_BadEnumFlags1, EnumFlags & ~ValidEnumFlags, EnumTree->Specifiers );
        EnumFlags &= ValidEnumFlags;
    }

    if ( DefinedInInterface && ( EnumFlags & DECLF_InvalidInterfaceMemberFlags ))
    {
        ReportDeclFlagError( ERRID_BadInterfaceEnumSpecifier1, EnumFlags & DECLF_InvalidInterfaceMemberFlags, EnumTree->Specifiers );
        EnumFlags &= ~DECLF_InvalidInterfaceMemberFlags;
    }

    // Only nested enums can be Protected.
    if ( EnumFlags & ( DECLF_Protected | DECLF_ProtectedFriend ))
    {
        ERRID errid = 0;

        if ( IsNested )
        {
            if ( DefinedInStdModule )
            {
                errid = ERRID_ModuleCantUseMemberSpecifier1;
            }
            else if ( DefinedInStruct )
            {
                errid = ERRID_StructCantUseVarSpecifier1;
            }
        }
        else
        {
            errid = ERRID_ProtectedTypeOutsideClass;
        }

        if ( errid )
        {
            ReportDeclFlagError( errid, EnumFlags & ( DECLF_Protected | DECLF_ProtectedFriend ), EnumTree->Specifiers, NULL );
            EnumFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
            EnumFlags |= DECLF_Public;
        }
    }

    // Only nested classes can be Private
    if (( EnumFlags & DECLF_Private ) && !IsNested )
    {
        ReportError( ERRID_PrivateTypeOutsideType, &( EnumTree->Name.TextSpan ));
    }

    // Only nested enums can be marked as shadows.
    if (( EnumFlags & ( DECLF_ShadowsKeywordUsed )) && !IsNested )
    {
        ReportError( ERRID_ShadowingTypeOutsideClass1, &( EnumTree->Name.TextSpan ), m_CompilerInstance->TokenToString(tkENUM) );
        EnumFlags &= ~DECLF_ShadowsKeywordUsed;
    }

    // If no access flags are set, set to a default.
    if ( !( EnumFlags & DECLF_AccessFlags ))
    {
        // Nested Enums default to Public access if no access flags are set (Friend if they are at namespace level)
        EnumFlags |= (( IsNested ) ? DECLF_Public : DECLF_Friend );
        IsTypeAccessExplictlySpecified = false;
    }

    if ( DefinedInStdModule )
    {
        if ( EnumFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseTypeSpecifier1, EnumFlags & DECLF_InvalidStdModuleMemberFlags, EnumTree->Specifiers, NULL );
            EnumFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }
    }

    Vtypes EnumUnderlyingVType;
    BCSYM *EnumUnderlyingType;

    // Resolve the underlying type of the enum
    if ( EnumTree->UnderlyingRepresentation &&
         EnumTree->UnderlyingRepresentation->Opcode != ParseTree::Type::SyntaxError )
    {
        if (ParseTree::Type::Nullable == EnumTree->UnderlyingRepresentation->Opcode )
        {
            ReportError(ERRID_NullableOnEnum, &(EnumTree->UnderlyingRepresentation->TextSpan));
            EnumUnderlyingType = m_SymbolAllocator.GetGenericBadNamedRoot();
        }
        else
        {
            EnumUnderlyingType = MakeType( EnumTree->UnderlyingRepresentation, NULL, EnumSymbol, GetTypeList() );
        }
        EnumUnderlyingVType = EnumUnderlyingType->GetVtype();
    }
    else
    {
        EnumUnderlyingType = m_CompilerHost->GetFXSymbolProvider()->GetType( t_i4 );
        EnumUnderlyingVType = t_i4;
    }


    Vtypes EnumMemberVtype;
    BCSYM *EnumMemberType;
    bool DelayEvaluatingAllEnumMemberValues;

    if (EnumUnderlyingType->IsBad())
    {

        EnumMemberType = EnumUnderlyingType;
        EnumMemberVtype =  t_i4;;
        DelayEvaluatingAllEnumMemberValues = true;
    }
    else if ( IsIntegralType(EnumUnderlyingVType) && TypeHelpers::IsIntrinsicType(EnumUnderlyingType) )
    {
        EnumMemberType = EnumUnderlyingType->ChaseToType();
        EnumMemberVtype = EnumUnderlyingVType;
        DelayEvaluatingAllEnumMemberValues = false;
    }
    else
    {
        if ( !EnumUnderlyingType->IsNamedType() )
        {
            // Note that arrays are caught in the parser itself
            //
            // Non-integral intrinsics etc. are caught here
            //
            ReportError( ERRID_InvalidEnumBase, &EnumTree->UnderlyingRepresentation->TextSpan );
            EnumUnderlyingType = m_SymbolAllocator.GetGenericBadNamedRoot();
        }

        EnumMemberType = EnumUnderlyingType;
        EnumMemberVtype = t_i4;
        DelayEvaluatingAllEnumMemberValues = true;
    }


    BCSYM *EnumTHISType =
        HasGenericParent ?
            (BCSYM *)MakeNamedTypeForTHISType(EnumSymbol) :
            EnumSymbol;

    //
    // Build symbols for the members of the ENUM
    //

    for ( ParseTree::StatementList *EnumMember = EnumTree->Children;
          EnumMember;
          EnumMember = EnumMember->NextInBlock )
    {
        // Ignore any errors and at this level.
        ParseTree::EnumeratorStatement *ThisEnumMember;
        if ( EnumMember->Element->Opcode == ParseTree::Statement::Enumerator )
        {
            ThisEnumMember = EnumMember->Element->AsEnumerator();
        }
        else if ( EnumMember->Element->Opcode == ParseTree::Statement::EnumeratorWithValue )
        {
            ThisEnumMember = EnumMember->Element->AsEnumeratorWithValue();
        }
        else if ( EnumMember->Element->Opcode == ParseTree::Statement::CommentBlock &&
                  EnumMember->Element->AsCommentBlock()->IsXMLDocComment == true )
        {
            // Add any XMLComment blocks to the list of blocks seen so far.
            AddCommentBlock( EnumMember->Element->AsCommentBlock() );
            continue;
        }
        else // if not one of the two cases above, there was an error which the parser should have logged so we just skip
        {
            continue;
        }

        if ( ThisEnumMember->Name.IsBad )
        {
            continue; // Not much to do if there isn't a name for the member.
        }

        STRING *MemberName = ThisEnumMember->Name.Name;
        if ( ThisEnumMember->Name.TypeCharacter != chType_NONE )
        {
            ReportError( ERRID_TypecharNotallowed, &ThisEnumMember->Name.TextSpan );
        }

        // Set up the context to be the enumeration member we are building
        BCSYM_Variable *CurrentVariable = m_SymbolAllocator.AllocVariable( ThisEnumMember->Name.TextSpan.IsValid() /* has location */, true /* has value */);

        //
        // Process the expression.
        //

        // If there is an expression tree, use it
        BCSYM_Expression *EnumMemberExpressionSymbol = NULL;
        if ( ThisEnumMember->Opcode == ParseTree::Statement::EnumeratorWithValue )
        {
            MakeConstantExpression
            (
                ThisEnumMember->AsEnumeratorWithValue()->Value,
                true, // epxression is pre-evaluated
                true, // Enums are always explictly typed.
                CurrentVariable, // the context for the built EnumMemberExpressionSymbol
                &EnumMemberType, // the type of the enum (typically I4)
                &EnumMemberExpressionSymbol
            ); // the created const expression symbol

            HaveSeenExpression = true;
        }
        else if ( !HaveSeenExpression && !DelayEvaluatingAllEnumMemberValues )
        {
            ConstantValue Value;
            Value.TypeCode = EnumMemberVtype;
            Value.Integral = EnumMemberValue;

            // Make sure that if we have an intrinsic value that we aren't overflowing the bounds of the enum type.
            // For those enum values that have to be evaluated during bindable, we will catch those overflows during
            // Bindable::EvaluateDeclaredExpression()
            bool Overflow = false;
            NarrowIntegralResult(Value.Integral, EnumMemberType, EnumMemberType, Overflow);

            if (Overflow)
            {
                ReportError( ERRID_ExpressionOverflow1, &ThisEnumMember->Name.TextSpan, EnumMemberType->PNamedRoot()->GetName() );
            }
            else
            {
                EnumMemberExpressionSymbol = m_SymbolAllocator.GetFixedExpression( &Value );
            }
            ++EnumMemberValue;
        }
        else  // We need to synthesize an initializer expression string
        {
            if (!HaveSeenExpression)
            {
                // We need to synthesize an initializer expression string "0"

                VSASSERT(DelayEvaluatingAllEnumMemberValues,
                            "Why synthesize an expression for the first enum member when type is already known ?");

                EnumMemberExpressionSymbol = m_SymbolAllocator.GetConstantExpression( &ThisEnumMember->Name.TextSpan,
                                                                                    CurrentVariable,
                                                                                    GetExpressionListHead(),
                                                                                    GetExpressionListTail(),
                                                                                    NULL,
                                                                                    WIDE("0"),   // the initializer
                                                                                    1);     // length of the initializer string in WCHARS
                EnumMemberExpressionSymbol->SetIsSyntheticExpression();
                HaveSeenExpression = true;
            }
            else
            {
                // We need to synthesize an initializer expression string that represents the last initializer expression + 1
                // This will happen in Bindable::EvaluateDeclaredExpression
                EnumMemberExpressionSymbol = m_SymbolAllocator.GetConstantExpression( &ThisEnumMember->Name.TextSpan,
                                                                                    CurrentVariable,
                                                                                    GetExpressionListHead(),
                                                                                    GetExpressionListTail(),
                                                                                    NULL,
                                                                                    NULL, // no expression needed since we'll store the actual member
                                                                                    0);   // no string length

                EnumMemberExpressionSymbol->SetPrevousEnumMember(EnumMemberList.GetFirst()->PVariableWithValue());
            }

            EnumMemberExpressionSymbol->SetForcedType( !EnumMemberType->IsObject() ? EnumMemberType : NULL );
        }

        //
        // Finish allocating the symbol.
        //

        DECLFLAGS ThisEnumMemberFlags = DECLF_Public | DECLF_Const | (ThisEnumMember->Name.IsBracketed == true ? DECLF_Bracketed : 0);

        m_SymbolAllocator.GetVariable( ThisEnumMember->Name.TextSpan.IsValid() ? &ThisEnumMember->Name.TextSpan : NULL, // location info
                                       MemberName, // name of the enum member
                                       MemberName, // use it for the emitted name as well
                                       ThisEnumMemberFlags,
                                       VAR_Const, // variable kind
                                       EnumTHISType, // variable type
                                       EnumMemberExpressionSymbol, // initializer expression
                                       &EnumMemberList, // hang built enum member on this list
                                       CurrentVariable ); // CurrentVariable will get filled out with the above info

        // Store away the custom attributes.
        ProcessAttributeList( ThisEnumMember->Attributes, CurrentVariable );

        // If we build an enummember symbol, go ahead and process any XMLDoc comments that may need to be attached
        // to this symbol.
        if ( EnumSymbol )
        {
            ExtractXMLDocComments( EnumMember, CurrentVariable );
        }
    } // loop through the enum members

    // Is the enum empty?
    if ( EnumMemberList.GetCount() == 0 )
    {
        ReportError( ERRID_BadEmptyEnum1, &EnumTree->Name.TextSpan, EnumName );
    }

    // Make the instance variable
    BCSYM_Variable *EnumInstanceVar = m_SymbolAllocator.AllocVariable( false, false );

    m_SymbolAllocator.GetVariable( NULL, // No location
                           STRING_CONST(m_CompilerInstance, EnumValueMember),  // 'value__'
                           STRING_CONST(m_CompilerInstance, EnumValueMember),  // 'value__'
                           DECLF_Public | DECLF_SpecialName,
                           VAR_Member,
                           EnumMemberType,
                           NULL,
                           &EnumMemberList,
                           EnumInstanceVar );

    //
    // Create the enum symbol.
    //

    m_SymbolAllocator.GetClass( EnumTree->Name.TextSpan.IsValid() ? &EnumTree->Name.TextSpan : NULL,
                       EnumName,
                       GetEmittedTypeNameFromActualTypeName( EnumName, NULL, m_CompilerInstance ),
                       m_CurrentNamespaceContext,
                       m_SourceFile,
                       m_CompilerHost->GetFXSymbolProvider()->GetType(FX::EnumType),
                       NULL,
                       EnumFlags | DECLF_Enum,
                       EnumMemberVtype,
                       NULL,
                       &EnumMemberList,
                       GetUnBindableChildrenSymbolList(),
                       NULL, // No generic parameters.
                       OwningSymbolList,
                       EnumSymbol );

    EnumSymbol->SetUnderlyingTypeForEnum(EnumUnderlyingType);
    EnumSymbol->SetTypeAccessExplictlySpecified(IsTypeAccessExplictlySpecified);

    // Store away the custom attributes.
    ProcessAttributeList( EnumTree->Attributes, EnumSymbol );

    AddContainerContextToSymbolList();
    PopContainerContext();

    // If we build an enum symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this enum symbol.
    if ( EnumSymbol )
    {
        ExtractXMLDocComments( StatementListTree, EnumSymbol );
    }
}

/*****************************************************************************
;MakeInterface

Creates the symbol for an Interface symbol and hooks up the implementing methods
to the interface members they implement.
*****************************************************************************/
void Declared::MakeInterface
(
    ParseTree::StatementList *StatementListTree, // [in] the parse tree for the Interface
    bool NestedInClass,           // [in] whether we are defining an interface inside a class/module
    bool DefinedAtNamespaceLevel, // [in] whether this interface was defined at namespace level
    bool HasGenericParent,        // [in] indicates whether any of its enclosing types is a generic
    SymbolList *OwningSymbolList, // [in] symbol list to add the built interface to
    DECLFLAGS ContainerDeclFlags  // [in] the declaration flags for the container the interface is defined within
)
{
    bool DefinedInStdModule = ContainerDeclFlags & DECLF_Module; // is this interface defined inside a standard module?
    bool DefinedInStructure = ContainerDeclFlags & DECLF_Structure; // is this interface defined inside a structure?
    SymbolList InterfaceChildrenSymbolList; // will hold the list of methods, events, etc. in the interface
    BCSYM_GenericParam *FirstGenericParam = NULL;
    BCSYM_Implements *ListOfInheritedInterfaceSymbols = NULL;
    bool TooLateForInherits = false;
    const DECLFLAGS ValidInterfaceFlags = DECLF_AccessFlags | DECLF_ShadowsKeywordUsed;
    ParseTree::TypeStatement *InterfaceTree = StatementListTree->Element->AsType();

    // Get the name - If we don't have a name, there isn't much to do.
    if ( InterfaceTree->Name.IsBad ) return;

    // Microsoft: consider so much of the initial blocks of makeinterface,makeclass,makestructure,makeenum are the same.  Should really factor that stuff out.
    STRING *InterfaceName = InterfaceTree->Name.Name;
    Location *InterfaceNameLocation = &InterfaceTree->Name.TextSpan;
    if ( InterfaceTree->Name.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, InterfaceNameLocation );
    }
    BCSYM_Interface *Interface = m_SymbolAllocator.AllocInterface( InterfaceNameLocation != NULL ); // The context is now the Delegate Class symbol

    // set the current container context
    SetContainerContext(Interface);

    // Set the flags.
    DECLFLAGS InterfaceFlags = MapSpecifierFlags( InterfaceTree->Specifiers );

    // Only access flags are legal.
    if ( InterfaceFlags & ~ValidInterfaceFlags )
    {
        ReportDeclFlagError( ERRID_BadInterfaceFlags1, InterfaceFlags & ~ValidInterfaceFlags, InterfaceTree->Specifiers, NULL );
        InterfaceFlags &= ValidInterfaceFlags;
    }

    // Only nested interfaces can be Protected.
    if ( InterfaceFlags & ( DECLF_Protected | DECLF_ProtectedFriend ) && ( !NestedInClass || DefinedInStructure ))
    {
        ReportError( ERRID_ProtectedTypeOutsideClass, &( InterfaceTree->Name.TextSpan ));

        InterfaceFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
        InterfaceFlags |= DECLF_Public;
    }

    // Only nested interfaces can be Private
    if (( InterfaceFlags & DECLF_Private ) && !NestedInClass )
    {
        ReportError( ERRID_PrivateTypeOutsideType, &( InterfaceTree->Name.TextSpan ));
    }

    // Only nested interfaces can be marked as shadows.
    if (( InterfaceFlags & ( DECLF_ShadowsKeywordUsed )) && DefinedAtNamespaceLevel )
    {
        ReportError( ERRID_ShadowingTypeOutsideClass1, &( InterfaceTree->Name.TextSpan ), m_CompilerInstance->TokenToString(tkINTERFACE) );
        InterfaceFlags &= ~DECLF_ShadowsKeywordUsed;
    }

    if ( DefinedInStdModule )
    {
        if ( InterfaceFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseTypeSpecifier1, InterfaceFlags & DECLF_InvalidStdModuleMemberFlags, InterfaceTree->Specifiers, NULL );
            InterfaceFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }
    }

    // Nested Interfaces default to Public access if no access flags are set ( Friend if they are at namespace level )
    if ( !( InterfaceFlags & DECLF_AccessFlags ))
    {
        InterfaceFlags |= DefinedAtNamespaceLevel ? DECLF_Friend : DECLF_Public;
    }

    MakeGenericParameters( InterfaceTree->GenericParameters,
                           &FirstGenericParam,
                           Interface );

    STRING *EmittedName = InterfaceName; // Emitted name is the same as the interface name
    BCSYM_Property *DefaultProperty = NULL;

    //
    // Build the symbols for the body of the Interface definition
    //

    for ( ParseTree::StatementList * CurrentInterfaceMember = InterfaceTree->Children;
          CurrentInterfaceMember;
          CurrentInterfaceMember = CurrentInterfaceMember->NextInBlock )
    {
        Location *InterfaceMemberTextSpan = &CurrentInterfaceMember->Element->TextSpan;

        switch ( CurrentInterfaceMember->Element->Opcode )
        {
            case ParseTree::Statement::Empty:
            case ParseTree::Statement::Region:
            case ParseTree::Statement::EndRegion:
            case ParseTree::Statement::EndCommentBlock:
            case ParseTree::Statement::EndProperty:
            case ParseTree::Statement::EndSub:
            case ParseTree::Statement::EndFunction:
            case ParseTree::Statement::EndOperator:
            case ParseTree::Statement::EndGet:
            case ParseTree::Statement::EndSet:
            case ParseTree::Statement::EndStructure:
            case ParseTree::Statement::EndClass:
            case ParseTree::Statement::EndInterface:
            case ParseTree::Statement::EndEnum:
            case ParseTree::Statement::EndUnknown:
            case ParseTree::Statement::EndInvalid:
            case ParseTree::Statement::SyntaxError:
                continue; // skip over these

            case ParseTree::Statement::Inherits: // Looking at an Inherits statement?
            {
                // If we've seen either a sub or an event, this is in error.
                if ( TooLateForInherits )
                {
                    ReportError( ERRID_BadInterfaceOrderOnInherits, InterfaceMemberTextSpan );
                }

                // Run through all the interfaces listed in the inherits statement and build their symbols
                for ( ParseTree::TypeList *InheritedInterface = CurrentInterfaceMember->Element->AsTypeList()->Types;
                      InheritedInterface;
                      InheritedInterface = InheritedInterface->Next )
                {
                    if ( InheritedInterface->Element->Opcode == ParseTree::Type::SyntaxError )
                    {
                        continue; // don't process stuff that is horked
                    }

                    // Add it to the front of the list of inherited interfaces.
                    m_SymbolAllocator.GetImplements( &InheritedInterface->Element->TextSpan, NULL, MakeType( InheritedInterface->Element, NULL, Interface, NULL ), &ListOfInheritedInterfaceSymbols );

                } // loop through inherited interfaces
                continue;
            }

            case ParseTree::Statement::ConstructorDeclaration:

                ReportError( ERRID_NewInInterface, InterfaceMemberTextSpan );
                continue;

            case ParseTree::Statement::FunctionDeclaration:
            case ParseTree::Statement::ProcedureDeclaration:
            {
                TooLateForInherits = true;

                ParseTree::MethodDefinitionStatement * MethodDefinition = CurrentInterfaceMember->Element->AsMethodDefinition();
                if ( MethodDefinition->Name.IsBad )
                {
                    continue; // If there is no name, there isn't much for us to do.
                }

                DECLFLAGS ProcDeclarationFlags = DECLF_AllowOptional;
                BCSYM_MethodDecl *Method = m_SymbolAllocator.AllocMethodDecl( true /* has location information */ ); // context is the decl symbol
                Location *MemberTextSpan = &MethodDefinition->Name.TextSpan;

                // Check the flags.
                ProcDeclarationFlags |= MapProcDeclFlags( MethodDefinition );

                if ( ProcDeclarationFlags & ~DECLF_ValidMethodFlags )
                {
                    if ( ProcDeclarationFlags & DECLF_Static )
                    {
                        ReportError( ERRID_ObsoleteStaticMethod, MemberTextSpan );
                    }

                    if ( ProcDeclarationFlags  & ~(DECLF_ValidMethodFlags & DECLF_Static))
                    {
                        ReportDeclFlagError( ERRID_BadMethodFlags1, ProcDeclarationFlags & ~(DECLF_ValidMethodFlags | DECLF_Static), MethodDefinition->Specifiers );
                    }
                    ProcDeclarationFlags  &= DECLF_ValidMethodFlags;
                }

                if ( ProcDeclarationFlags & ~DECLF_ValidInterfaceMethodFlags )
                {
                    ReportDeclFlagError( ERRID_BadInterfaceMethodFlags1, ProcDeclarationFlags & ~DECLF_ValidInterfaceMethodFlags, MethodDefinition->Specifiers );
                    ProcDeclarationFlags &= DECLF_ValidInterfaceMethodFlags;
                }

                // Can't be OVERLOADS and SHADOWS
                if (( ProcDeclarationFlags & DECLF_ShadowsKeywordUsed ) && ( ProcDeclarationFlags & DECLF_OverloadsKeywordUsed ))
                {
                    ReportDeclFlagComboError( DECLF_ShadowsKeywordUsed | DECLF_OverloadsKeywordUsed, MethodDefinition->Specifiers );
                    ProcDeclarationFlags &= ~DECLF_OverloadsKeywordUsed;
                }

                // Interface methods can't claim to implement anything
                if ( MethodDefinition->Implements )
                {
                    ReportError( ERRID_BadInterfaceMethodFlags1, &MethodDefinition->Implements->TextSpan, m_CompilerInstance->TokenToString( tkIMPLEMENTS ));
                }

                if ( MethodDefinition->Handles )
                {
                    ReportError( ERRID_BadInterfaceMethodFlags1, &MethodDefinition->Handles->TextSpan, m_CompilerInstance->TokenToString( tkHANDLES ));
                }

                // Interface methods are always public and always MustOverride.
                ProcDeclarationFlags |= DECLF_Public | DECLF_MustOverride;

                // Create the symbol - it will get added to InterfaceChildrenSymbolList.
                MakeProc( MethodDefinition->Opcode,
                          &MethodDefinition->Name,
                          NULL, // indicates that the name should be pulled from the tree
                          CurrentInterfaceMember->Element->AsMethodDeclaration()->Parameters,
                          CurrentInterfaceMember->Element->AsMethodDeclaration()->ReturnType,
                          CurrentInterfaceMember->Element->AsMethodDeclaration()->ReturnTypeAttributes,
                          NULL, // no lib
                          NULL, // no alias
                          CurrentInterfaceMember->Element->AsMethodDeclaration()->GenericParameters,
                          ProcDeclarationFlags,
                          true,
                          ( CodeBlockLocation* ) NULL, // no code block location
                          ( CodeBlockLocation* ) NULL, // no proc block location
                          Method,
                          &InterfaceChildrenSymbolList, // attatch proc to this list
                          NULL, // don't care about constructors
                          NULL, // don't care about shared constructors
                          MethodDefinition->Specifiers,
                          InterfaceFlags, // Not declared inside a standard module
                          NULL,           // Not a property
                          NULL );         // No need to get back the list of parameters built

                // Store away the custom attributes.
                ProcessAttributeList( MethodDefinition->Attributes, Method );

                // Go ahead and process any XMLDoc comments that may be need to be attached to this method symbol.
                ExtractXMLDocComments( CurrentInterfaceMember, Method );

                if ( CurrentInterfaceMember->Element->HasSyntaxError )
                {
                    Method->SetIsBad(m_SourceFile);
                }

                continue;
            }

            case ParseTree::Statement::EventDeclaration: // Looking at an Event?
                                                         // Not handling block events here because they are disallow in interfaces.
            {
                TooLateForInherits = true;

                bool NeedSharedConstructor = false;
                MakeEvent( CurrentInterfaceMember,
                           true, // in an interface
                           0, // container flags don't matter (not in struct,module,etc.)
                           &InterfaceChildrenSymbolList,
                           NULL,    // No list for static backing fields
                           NeedSharedConstructor );
                continue;
            }

            case ParseTree::Statement::Property:
            {
                TooLateForInherits = true;

                bool NeedSharedConstructor = false;
                BCSYM_Property *Prop = MakeProperty( CurrentInterfaceMember,
                                                     0, // no class decl flags
                                                     true, // defined in an interface
                                                     &InterfaceChildrenSymbolList,
                                                     NeedSharedConstructor,
                                                     NULL ); // no list for static backing fields

                if ( Prop && Prop->IsDefault() && !DefaultProperty )
                {
                    DefaultProperty = Prop; // Just keep track of the first one - makes for better errors to have ahold of the first one we came across
                }

                VSASSERT( !NeedSharedConstructor, "Interfaces cannot have shared constructors." );
                continue;
            }
            case ParseTree::Statement::DelegateFunctionDeclaration:
            case ParseTree::Statement::DelegateProcedureDeclaration:

                TooLateForInherits = true;
                MakeDelegate( CurrentInterfaceMember,
                              true /* is nested */, 0 /* no container flags */ , &InterfaceChildrenSymbolList,
                              NULL, // no parameters symbol list
                              false, // not for an event
                              true, //  use attributes portion of tree if present
                              NULL, // no optional name for delegate
                              NULL, // no hook for built symbol
                              true /* defined in an interface */ );
                continue;

             case ParseTree::Statement::Interface:

                MakeInterface( CurrentInterfaceMember,
                               false,  // not nested in a class
                               false,  // not at namespace level
                               HasGenericParent || FirstGenericParam,
                               &InterfaceChildrenSymbolList,
                               InterfaceFlags );
                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Class:

                MakeClass( CurrentInterfaceMember,
                           true, // is nested,
                           false, // don't build a std module,
                           true, // defined in an interface
                           HasGenericParent || FirstGenericParam,
                           InterfaceFlags,
                           &InterfaceChildrenSymbolList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Structure:

                MakeStructure( CurrentInterfaceMember,
                               true, // is nested
                               true, // defined in an interface
                               HasGenericParent || FirstGenericParam,
                               0, // no container flags
                               &InterfaceChildrenSymbolList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::Enum:

                MakeEnum( CurrentInterfaceMember,
                          true, // it's nested
                          true, // it is defined in an interface
                          HasGenericParent || FirstGenericParam,
                          0, // no container flags
                          &InterfaceChildrenSymbolList );

                TooLateForInherits = true;
                continue;

            case ParseTree::Statement::CCIf:
            case ParseTree::Statement::CCEndIf:
            case ParseTree::Statement::CCElse:
            case ParseTree::Statement::CCElseIf:
                continue; // Ignore conditional compilation directives in all instances

            case ParseTree::Statement::CommentBlock:

                if ( CurrentInterfaceMember->Element->AsCommentBlock()->IsXMLDocComment == true )
                {
                    AddCommentBlock( CurrentInterfaceMember->Element->AsCommentBlock() );
                }

                continue;

            default: // anything else is an generic error.
                ReportError( ERRID_BadInterfaceMember, InterfaceMemberTextSpan );
                continue;
        } // switch ( CurrentInterfaceMember->Opcode )
    } // loop through members of interface

    //
    // Create the interface symbol.
    //

    m_SymbolAllocator.GetInterface( InterfaceNameLocation,
                            InterfaceName,
                            GetEmittedTypeNameFromActualTypeName( InterfaceName, FirstGenericParam, m_CompilerInstance ),
                            m_CurrentNamespaceContext,
                            m_SourceFile,
                            InterfaceFlags,
                            ListOfInheritedInterfaceSymbols, // the list of interfaces inherited by the one we are creating
                            &InterfaceChildrenSymbolList, // the list of events, methods, etc. discovered in the interface
                            GetUnBindableChildrenSymbolList(),
                            FirstGenericParam,
                            OwningSymbolList, // add this interface to this list of symbols
                            Interface); // the created interface lives here

    Symbols::SetDefaultProperty( Interface, DefaultProperty );

    // Store away the custom attributes.
    ProcessAttributeList( InterfaceTree->Attributes, Interface );

    AddContainerContextToSymbolList();
    PopContainerContext();

    // If we build a interface symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this interface symbol.
    if ( Interface )
    {
        ExtractXMLDocComments( StatementListTree, Interface );
    }
}

/*****************************************************************************
;MakeDelegateMethods

Creates the methods for a delegate symbol:

    Invoke( <delegate signature> )
    IAsyncResult BeginInvoke(<delegate signature>, AsyncCallback, Object)
    <delegate rettype> EndInvoke(<non-byval params>, IAsyncResult)
    .ctor(TargetObject as Object, TargetMember as IntPointer)

These methods are interesting because they aren't synthetic - thus there is
no code generated for them.  So you wonder why we create them in the first place.
Turns out that these are magical functions that the RunTime expects to find
and will do the hookup for us to the things that actually implement the respective
functionality.
*****************************************************************************/
void Declared::MakeDelegateMethods
(
    ParseTree::MethodSignatureStatement *DelegateTree, // [in] the signature of the delegate
    SymbolList *DelegateChildren, // [out] the built methods for the delegate will be attached here
    bool ForAnEvent, // [in] whether we are creating a delegate for an event
    BCSYM_Param *ParameterSymbolList,  // [in] Params for the delegate methods (if any) to be used instead of DelegateTree parameters in order to avoid logging duplicate errors
    DECLFLAGS DeclFlags  // [in] The Delegate's DECLFLAGS.
)
{
    BCSYM *ReturnType = NULL;
    // Delegate methods created for an event are marked as hidden
    DECLFLAGS DeclFlagsHidden = ForAnEvent ? DECLF_Hidden : 0;

    //
    // Create the invoke method.
    //

    // We need to get the list of parameters back from MakeProc for the Invoke method
    // so that we can simply make a copy of that list and use it for the delegate
    // parameters. This make it possible to generate one set of errors for the parameter,
    // should we decide to generate errors for them.
    //
    // Also, if we are building the delegate for an event, then the list of parameters has already been built,
    // and we wouldn't want to recreate a new set of parameters becasue of the duplicate error reporting issue
    BCSYM_Param *ParameterSymbolListForInvokeMethod = ParameterSymbolList ? CloneParameters(ParameterSymbolList, NULL) : NULL;

    BCSYM_MethodDecl *InvokeMethod = m_SymbolAllocator.AllocMethodDecl( true /* Invoke method has location of Delegate declaration for error reporting */ );

    BCSYM *DelegateReturnTypeSymbol = NULL;                        // The same ReturnType symbol is shared among the Invoke and EndInvoke methods

    MakeProc( DelegateTree->Opcode,
              &DelegateTree->Name, // The actual name will be overriden by "Invoke", but we need this in the event there is a return type character
              STRING_CONST(m_CompilerInstance, DelegateInvoke), // "Invoke"
              ParameterSymbolListForInvokeMethod ? NULL : DelegateTree->Parameters,
              DelegateTree->ReturnType,
              DelegateTree->ReturnTypeAttributes,
              NULL, // no lib
              NULL, // no alias
              NULL, // no generic params
              DECLF_Function | DECLF_Public | DECLF_Overridable | DECLF_AllowDupParam | // DevDiv Bugs #22935  Allow duplicate param names
                  ( ForAnEvent ? DECLF_AllowOptional : 0 ) | ( DeclFlags & DECLF_HasRetval ) | DeclFlagsHidden,
              false,
              ( CodeBlockLocation* ) NULL,
              ( CodeBlockLocation* ) NULL,
              InvokeMethod,
              DelegateChildren,
              NULL, // don't care about constructors
              NULL, // don't care about shared constructors
              DelegateTree->Specifiers,
              DeclFlags | DECLF_Delegate,    // Indicate declared in delegate
              &DelegateReturnTypeSymbol,
              &ParameterSymbolListForInvokeMethod );         // No need to get back the list of parameters built

    BCSYM_Param *LastParam;
    BCSYM_Param *FirstParam;
    BCSYM_MethodDecl *MethodDeclSymbol;

    // Public WinRT Delegates shouldn't have BeginInvoke/EndInvoke.
    if (!(m_SourceFile->GetCompilerProject() && 
          m_SourceFile->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj &&
          (DeclFlags & DECLF_Public)) &&
          m_CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::AsyncCallbackType) &&
          m_CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncResultType))
    {
        // Asynchronous begin Invoke
        // IAsyncResult BeginInvoke(<params>, AsyncCallback, Object)
        MethodDeclSymbol = m_SymbolAllocator.AllocMethodDecl( false );

        FirstParam = CloneParameters(ParameterSymbolListForInvokeMethod, &LastParam);

        // Now, tack on DelegateCallback & DelegateAsyncState parameters
        m_SymbolAllocator.GetParam( NULL,
                            STRING_CONST(m_CompilerInstance, DelegateCallback), // "DelegateCallback"
                            m_CompilerHost->GetFXSymbolProvider()->GetType(FX::AsyncCallbackType),
                            0,
                            NULL,
                            &FirstParam,
                            &LastParam,
                            false);  // Not a return type param

        m_SymbolAllocator.GetParam( NULL,
                            STRING_CONST(m_CompilerInstance, DelegateAsyncState), // "DelegateAsyncState
                            m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType),
                            0,
                            NULL,
                            &FirstParam,
                            &LastParam,
                            false);  // Not a return type param

        m_SymbolAllocator.GetProc( NULL, // Build BeginInvoke()
                           STRING_CONST(m_CompilerInstance, DelegateBeginInvoke),
                           STRING_CONST(m_CompilerInstance, DelegateBeginInvoke),
                           ( CodeBlockLocation* ) NULL,
                           ( CodeBlockLocation* ) NULL,
                           DECLF_Function | DECLF_Public | DECLF_Overridable | DeclFlagsHidden,
                           m_CompilerHost->GetFXSymbolProvider()->GetType(FX::IAsyncResultType),
                           FirstParam,
                           NULL,
                           NULL,
                           NULL,
                           SYNTH_None,
                           NULL, // no generic params
                           DelegateChildren,
                           MethodDeclSymbol );

        // Asynchronous end Invoke
        // <rettype> EndInvoke(<non-byval params>, IAsyncResult)
        // EndInvoke takes only byref params because they need to be copied-out when the async call finishes

        FirstParam = LastParam = NULL;
        MethodDeclSymbol = m_SymbolAllocator.AllocMethodDecl( false );

        // First, make the regular parameters
        FirstParam =
            CloneParameters(
                ParameterSymbolListForInvokeMethod,
                true, // All ByVal parameters are ignored for EndInvoke
                &LastParam);

        // Now, tack on the IAsyncResult parameter and make the proc
        m_SymbolAllocator.GetParam( NULL,
                            STRING_CONST(m_CompilerInstance, DelegateAsyncResult),
                            m_CompilerHost->GetFXSymbolProvider()->GetType(FX::IAsyncResultType),
                            0,
                            NULL,
                            &FirstParam,
                            &LastParam,
                            false);  // Not a return type param

        if ( DeclFlags & DECLF_HasRetval)
        {
            ReturnType = DelegateReturnTypeSymbol;
        }

        m_SymbolAllocator.GetProc( NULL,
                           STRING_CONST( m_CompilerInstance, DelegateEndInvoke ),
                           STRING_CONST( m_CompilerInstance, DelegateEndInvoke ),
                           (CodeBlockLocation*) NULL,
                           (CodeBlockLocation*) NULL,
                           DECLF_Function | (DeclFlags & DECLF_HasRetval) | DECLF_Public | DECLF_Overridable | DeclFlagsHidden,
                           ReturnType,
                           FirstParam,
                           NULL,
                           NULL,
                           NULL,
                           SYNTH_None,
                           NULL, // no generic params
                           DelegateChildren,
                           MethodDeclSymbol );
    }

    // Main constructor, <object, IntPointer>
    FirstParam = LastParam = NULL;

    m_SymbolAllocator.GetParam( NULL,
                        STRING_CONST(m_CompilerInstance, DelegateTargetObject), // "TargetObject"
                        m_CompilerHost->GetFXSymbolProvider()->GetType(FX::ObjectType), // param type will be "object"
                        0,
                        NULL,
                        &FirstParam,
                        &LastParam,
                        false);  // Not a return type param

    m_SymbolAllocator.GetParam( NULL,
                        STRING_CONST(m_CompilerInstance, DelegateTargetMethod), // "TargetMethod"
                        m_CompilerHost->GetFXSymbolProvider()->GetType(FX::IntPtrType),  // param is type Integer Pointer
                        0,
                        NULL,
                        &FirstParam,
                        &LastParam,
                        false);  // Not a return type param

    MethodDeclSymbol = m_SymbolAllocator.AllocMethodDecl( false );

    m_SymbolAllocator.GetProc( NULL,
                       STRING_CONST(m_CompilerInstance, Constructor), // .ctor
                       STRING_CONST(m_CompilerInstance, Constructor), // .ctor
                       ( CodeBlockLocation* ) NULL,
                       ( CodeBlockLocation* ) NULL,
                       DECLF_Function | DECLF_Public | DECLF_Constructor | DeclFlagsHidden | DECLF_SpecialName,
                       NULL,
                       FirstParam,
                       NULL,
                       NULL,
                       NULL,
                       SYNTH_None,
                       NULL, // no generic params
                       DelegateChildren,
                       MethodDeclSymbol );
}

/*****************************************************************************
;MakeDelegate

Creates a Delegate symbol from a parse tree
*****************************************************************************/
void Declared::MakeDelegate
(
    ParseTree::StatementList *StatementListTree, // [in] the delegate tree
    bool IsNested, // [in] delegate is defined inside a class/module/interface
    DECLFLAGS ContainerFlags, // [in] used to determine what kind of container the delegate is defined in
    SymbolList *OwningSymbolList, // [in] symbol list to add to
    BCSYM_Param *ParameterSymbolList,    // [in] Params for the delegate methods (if any) to be used instead of DelegateTree parameters in order to avoid logging duplicate errors
    bool ForAnEvent, // [optional in] whether we are creating a delegate for an event
    bool UseAttributes, // [optional in] Use attributes from DelegateTree->Attributes when building delegate?
    _In_opt_z_ STRING *OptionalDelegateName, // [optional in] the name of the delegate (we use this instead of the parse tree name when building a delegate for an event)
    BCSYM_Class **BuiltDelegateSymbol, // [optional out] the built delegate symbol which we can provide if ForAnEvent is true
    bool DefinedInInterface // [optional in] whether the delegate was declared within an interface
)
{
#if DEBUG
    if ( ForAnEvent )
    {
        VSASSERT( OptionalDelegateName, "Always pass in the delegate name when calling MakeDelegate for events." );
        VSASSERT( BuiltDelegateSymbol,  "Always pass in the result delegate when calling MakeDelegate for events." );
    }
    else
    {
        VSASSERT( !OptionalDelegateName, "Only pass in the delegate name when calling MakeDelegate for events." );
        VSASSERT( !BuiltDelegateSymbol,  "Only pass in the result delegate when calling MakeDelegate for events." );
        VSASSERT( !ParameterSymbolList,  "MakeDelegate() should not be given a ParameterSymbolList unless the Delegate is being built for an Event");
    }
#endif // DEBUG

    ParseTree::MethodDeclarationStatement *DelegateTree = StatementListTree->Element->AsMethodDeclaration();

    BCSYM_GenericParam *FirstGenericParam = NULL;

    bool DefinedInStdModule = ContainerFlags & DECLF_Module;    // variable is defined in a standard module
    bool DefinedInStructure = ContainerFlags & DECLF_Structure; // variable is defined in a structure
    bool IsTypeAccessExplictlySpecified = true;

    //========================================================================
    // A delegate is represented as a class with a constructor ( Object, I4 )
    // and an invoke method with the same signature as the delegate.  Neither method should be implemented.
    //========================================================================

    BCSYM_Class *Delegate = m_SymbolAllocator.AllocClass( true /* has location info */); // The context is now the Delegate Class symbol

    // If there is no name, there isn't much for us to do.
    if ( DelegateTree->Name.IsBad ) return;

    // The name of the delegate can't be "New"
    if ( StringPool::IsEqual( STRING_CONST( m_CompilerInstance, New ), DelegateTree->Name.Name ) &&
         !DelegateTree->Name.IsBracketed )
    {
        ReportError( ERRID_InvalidUseOfKeyword, &DelegateTree->Name.TextSpan );
        return;
    }

    // set the current container context
    SetContainerContext(Delegate);

    MakeGenericParameters( DelegateTree->GenericParameters,
                           &FirstGenericParam,
                           Delegate );

    // Delegates can't handle events or implement interface memebers
    if ( DelegateTree->Handles )
    {
        ReportError( ERRID_DelegateCantHandleEvents, &DelegateTree->Handles->TextSpan );
    }

    if ( DelegateTree->Implements )
    {
        ReportError( ERRID_DelegateCantImplement, &DelegateTree->Implements->TextSpan );
    }

    //
    // Check the flags.
    //

    DECLFLAGS DelegateFlags = MapProcDeclFlags( DelegateTree );

    // do not report errors on the flags if the error is in hidden code
    if (( DelegateFlags & ~DECLF_ValidDelegateFlags ) && !ForAnEvent )
    {
        ReportDeclFlagError( ERRID_BadDelegateFlags1, DelegateFlags & ~DECLF_ValidDelegateFlags, DelegateTree->Specifiers );
        DelegateFlags &= DECLF_ValidDelegateFlags;
    }

    // special semantics on delegates in interfaces (don't report error if in hidden code)
    if ( DefinedInInterface && !ForAnEvent )
    {
        if ( DelegateFlags & DECLF_InvalidInterfaceMemberFlags )
        {
            ReportDeclFlagError( ERRID_BadInterfaceDelegateSpecifier1, DelegateFlags & DECLF_InvalidInterfaceMemberFlags, DelegateTree->Specifiers );
            DelegateFlags &= ~DECLF_InvalidInterfaceMemberFlags;
        }
    }

    if ( DefinedInStructure )
    {
        if ( DelegateFlags & DECLF_InvalidFlagsInNotInheritableClass )
        {
            ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1, DelegateFlags & DECLF_InvalidFlagsInNotInheritableClass, DelegateTree->Specifiers );
            DelegateFlags &= ~DECLF_InvalidFlagsInNotInheritableClass;
        }
    } // Only nested delegates can be Protected.
    else if ( DelegateFlags & ( DECLF_Protected | DECLF_ProtectedFriend ) && !IsNested )
    {
        ReportError( ERRID_ProtectedTypeOutsideClass, &( DelegateTree->Name.TextSpan ));

        DelegateFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
        DelegateFlags |= DECLF_Public;
    }

    if ( !IsNested )
    {
        // Only nested classes can be Private
        if ( DelegateFlags & DECLF_Private )
        {
            ReportError( ERRID_PrivateTypeOutsideType, &( DelegateTree->Name.TextSpan ));
        }

        // Only nested delegates can be marked as shadows.
        if ( DelegateFlags & DECLF_ShadowsKeywordUsed )
        {
            ReportError( ERRID_ShadowingTypeOutsideClass1, &( DelegateTree->Name.TextSpan ), m_CompilerInstance->TokenToString(tkDELEGATE) );
            DelegateFlags &= ~DECLF_ShadowsKeywordUsed;
        }
    }

    if ( DefinedInStdModule )
    {
        if ( DelegateFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseTypeSpecifier1, DelegateFlags & DECLF_InvalidStdModuleMemberFlags, DelegateTree->Specifiers );
            DelegateFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }
    }

    // Delegates default to Public access if no access flags are set (Friend if they are at namespace level)
    if ( !( DelegateFlags & DECLF_AccessFlags ))
    {
        DelegateFlags |= ( IsNested ? DECLF_Public : DECLF_Friend );
        IsTypeAccessExplictlySpecified = false;
    }

    //
    // Get the trees we need.
    //

    SymbolList DelegateChildren;
    MakeDelegateMethods( DelegateTree, &DelegateChildren, ForAnEvent, ParameterSymbolList, DelegateFlags );

    //
    // Figure out what the name should be
    //

    STRING *DelegateName = ForAnEvent ? OptionalDelegateName : DelegateTree->Name.Name;

    //
    // Create the class symbol for the delegate
    //

    Location *DelegateTextSpan = &DelegateTree->Name.TextSpan;
    m_SymbolAllocator.GetClass( DelegateTextSpan,
                        DelegateName,
                        GetEmittedTypeNameFromActualTypeName( DelegateName, FirstGenericParam, m_CompilerInstance ),
                        m_CurrentNamespaceContext,
                        m_SourceFile,
                        m_CompilerHost->GetFXSymbolProvider()->GetType(FX::MultiCastDelegateType),
                        NULL,
                        ( DelegateFlags & DECLF_ValidDelegateFlags ) | DECLF_NotInheritable | DECLF_Delegate,
                        t_bad,
                        NULL,
                        &DelegateChildren,
                        GetUnBindableChildrenSymbolList(),
                        FirstGenericParam,
                        OwningSymbolList,
                        Delegate);

    Delegate->SetTypeAccessExplictlySpecified(IsTypeAccessExplictlySpecified);

    // Store away the custom attributes.
    if ( UseAttributes )
    {
        ProcessAttributeList( DelegateTree->Attributes, Delegate );
    }

    // When creating a delegate for an event, return the result
    if ( ForAnEvent )
    {
        *BuiltDelegateSymbol  = Delegate->PClass();
    }

    AddContainerContextToSymbolList();
    PopContainerContext();

    // If we build a delegate symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this delegate symbol. Notice that we only do this for delegates that exists in the souce code, not for
    // delegates that we generate for events decls.
    if ( Delegate && !ForAnEvent )
    {
        ExtractXMLDocComments( StatementListTree, Delegate );
    }
}

/**************************************************************************************************
;ContainsArrayWithSizes

Determines whether a variable in a Declarator list is declared with array sizes
***************************************************************************************************/
static bool // true - yes, a variable in the list is declared with array sizes / false - none are
ContainsArrayWithSizes
(
    ParseTree::DeclaratorList *DeclaratorList
)
{
    for (ParseTree::DeclaratorList *Declarators = DeclaratorList;
         Declarators;
         Declarators = Declarators->Next)
    {
        if (Declarators->Element->ArrayInfo &&
            Declarators->Element->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes)
        {
            return true;
        }
    }

    return false;
}

/**************************************************************************************************
;MakeArrayType

Makes a type of an array declaration
***************************************************************************************************/
BCSYM *
Declared::MakeArrayType
(
    ParseTree::Type *ArrayInfo,
    BCSYM *ElementType
)
{
    BCSYM *CurrentElementType = NULL;
    if (ArrayInfo->AsArray()->ElementType)
    {
        VSASSERT(ArrayInfo->Opcode == ParseTree::Type::ArrayWithoutSizes ||
                 ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes,
                 "unexpected!");
        CurrentElementType = MakeArrayType(ArrayInfo->AsArray()->ElementType, ElementType);
    }
    else
    {
        CurrentElementType = ElementType;
    }
    unsigned ArrayRank = ArrayInfo->AsArray()->Rank;
    if ( ArrayRank > ArrayRankLimit )
    {
        ReportError( ERRID_ArrayRankLimit, &ArrayInfo->TextSpan );
    }
    return m_SymbolAllocator.GetArrayType( ArrayRank, CurrentElementType);
}

/*========================================================================

Routines to build type members

========================================================================*/

/*****************************************************************************
;MakeImplementsList

Process an Implements list
*****************************************************************************/
BCSYM_ImplementsList * // The implements list for the method
Declared::MakeImplementsList(
    ParseTree::NameList * ImplementsTree, // [in] the Implements tree
    BCSYM_Proc *Procedure // [in] the procedure that is doing the implementing
)
{
    BCSYM_ImplementsList *ImplementsListHead = NULL, *ImplementsListTail = NULL;

    // haul through the implements entries in the list and create Implements symbols for them
    for ( ; ImplementsTree; ImplementsTree = ImplementsTree->Next )
    {
        // It is a syntax error for a non-qualified name to occur here. The parser
        // will detect the error, but can produce a tree with another sort of name.

        if ( ImplementsTree->Element->IsQualified() &&
             !ImplementsTree->Element->AsQualified()->Qualifier.IsBad )
        {
            // Add the function and type where it is defined (e.g. for Implements Class2.Foo.Bob pass the type for Class2.Foo and the name for 'Bob')
            ParseTree::QualifiedName * QualifiedName = ImplementsTree->Element->AsQualified();
            BCSYM_ImplementsList * Implements = m_SymbolAllocator.GetImplementsList( &QualifiedName->TextSpan, // location info for the entire qualified name
                                                                                     MakeNamedType( QualifiedName->Base, Procedure, GetTypeList()), // the type of the func, e.g. Class2.Foo. /*IsTHISType=false, IsAppliedAttributeContext=false, ArgumentNullable = NULL */
                                                                                     QualifiedName->Qualifier.Name); // the function name 'bob'


            // Build the list in the same order the user typed it in so errors in bindable make more sense
            if ( ImplementsListTail )
            {
                ImplementsListTail->SetNext(Implements);
                ImplementsListTail = Implements;
            }
            else
            {
                ImplementsListHead = Implements;
                ImplementsListTail = Implements;
            }
        }
    } // haul through list of implements entries

    return ImplementsListHead;
}

/*****************************************************************************
;MakeHandlesList

Build symbol list for a Handles statement
*****************************************************************************/
BCSYM_HandlesList * // The built list of handles clauses
Declared::MakeHandlesList(
    ParseTree::NameList *HandlesList, // [in] the Implements tree
    BCSYM_MethodImpl *HandlingMethod,
    DECLFLAGS ClassFlags // [in] the flags on the class that owns this method symbol
)
{
    // constructors cannot have handles clauses
    //
    if (HandlingMethod->IsAnyConstructor())
    {
        ReportError( ERRID_NewCannotHandleEvents,
                        &HandlesList->TextSpan );

        return NULL;
    }

    BCSYM_HandlesList *pHandlesListSymbol = NULL;
    BCSYM_HandlesList **ppHandlesListSymbol = &pHandlesListSymbol;

    // haul through the Handles entries in the list and create Handles symbols for them
    for ( ; HandlesList; HandlesList = HandlesList->Next )
    {
        *ppHandlesListSymbol = MakeHandlesClause( HandlesList->Element, HandlingMethod, ClassFlags );

        if ( *ppHandlesListSymbol )
        {
            // what this does is make it so the next created HandlesList symbol is created already chained in the list
            //
            ppHandlesListSymbol = ( *ppHandlesListSymbol )->GetNextHandlesTarget();
        }
    } // haul through list of handles entries

    return pHandlesListSymbol;
}

BCSYM_HandlesList*
Declared::MakeHandlesClause
(
    ParseTree::Name *HandlesEntry,
    BCSYM_MethodImpl *HandlingMethod,
    DECLFLAGS ClassFlags // [in] the flags on the class that owns this method symbol
)
{
    // It is a syntax error for a non-qualified name to occur here.
    //
    if ( !HandlesEntry ||
         !HandlesEntry->IsQualified() ||
         HandlesEntry->AsQualified()->Qualifier.IsBad )
    {
        return NULL;
    }

    bool IsContainerContextStdModule = ClassFlags & DECLF_Module;
    ERRID HandlesSyntaxError;
    if ( ClassFlags & DECLF_Module )
    {
        HandlesSyntaxError = ERRID_HandlesSyntaxInModule;
    }
    else
    {
        HandlesSyntaxError = ERRID_HandlesSyntaxInClass;
    }

    ParseTree::QualifiedName *QualifiedName = HandlesEntry->AsQualified();
    // Given Handles WithEventVar.EventName, build a handles list entry

    // 


    int NumberOfQualifiedNames = 0;
    STRING *NamesList[3] = { NULL, NULL, NULL };

    ParseTree::Name *WithEventName;
    for (WithEventName = QualifiedName;
          WithEventName->IsQualified() && NumberOfQualifiedNames < 3;
          WithEventName = WithEventName->AsQualified()->Base )
    {
        if ( WithEventName->AsQualified()->Qualifier.IsBad )
        {
            return NULL;
        }

        NumberOfQualifiedNames++;

        NamesList[3 - NumberOfQualifiedNames] = WithEventName->AsQualified()->Qualifier.Name;

        // Syntax error to have more than 3 names i.e. more than 2 qualified names
        // eg: handles A.B.C.D - error, 3 qualified names - B, C, D
        //
        if ( NumberOfQualifiedNames > 2 )
        {
            ReportError( HandlesSyntaxError,
                            &HandlesEntry->TextSpan );

            return NULL;
        }

    }

    if ( !WithEventName )
    {
        return NULL;
    }

    if ( WithEventName->Opcode != ParseTree::Name::Simple )
    {
        ReportError( HandlesSyntaxError,
                        &HandlesEntry->TextSpan );
        return NULL;
    }

    if ( WithEventName->AsSimple()->ID.IsBad )
    {
        return NULL;
    }

    ParseTree::SimpleName *LeadingQualifier = WithEventName->AsSimple();
    bool HandlesMyBaseEvent = false;
    bool HandlesMyClassEvent = false;
    bool HandlesMeEvent = false;

    if ( !LeadingQualifier->ID.IsBracketed )
    {
        if ( StringPool::IsEqual( LeadingQualifier->ID.Name,
                      STRING_CONST( m_CompilerInstance, MyBase )))
        {
            HandlesMyBaseEvent = true;
        }
        else if (StringPool::IsEqual( LeadingQualifier->ID.Name,
                    STRING_CONST( m_CompilerInstance, MyClass )))
        {
            HandlesMyClassEvent = true;
        }
        else if (StringPool::IsEqual( LeadingQualifier->ID.Name,
                    STRING_CONST( m_CompilerInstance, Me )))
        {
            HandlesMeEvent = true;
        }
    }

    if ( HandlesMyBaseEvent ||
         HandlesMyClassEvent ||
         HandlesMeEvent)
    {
        if ( NumberOfQualifiedNames > 1 ||
             IsContainerContextStdModule )      // Handles in Modules cannot have Mybase, Myclass and Me
        {
            ReportError( HandlesSyntaxError,
                            &HandlesEntry->TextSpan );

            return NULL;
        }
    }
    else
    {
        NamesList[0] = LeadingQualifier->ID.Name;
    }

    BCSYM_HandlesList *HandlesListSymbol =
        m_SymbolAllocator.GetHandlesList( &HandlesEntry->TextSpan,                  // location info for the whole qualified name in the handles
                                            &WithEventName->AsSimple()->TextSpan,  // location info for WithEventVar
                                            &QualifiedName->Qualifier.TextSpan,    // location info for EventName
                                            NamesList[0],                          // WithEvents Var
                                            NamesList[1],                          // Event Source Property
                                            NamesList[2],                          // Event
                                            HandlesMyBaseEvent,
                                            HandlesMyClassEvent,
                                            HandlesMeEvent,
                                            HandlingMethod );

    return HandlesListSymbol;
}


/*****************************************************************************
;MakeConstantExpression

Creates a constant expression symbol from a parse tree
*****************************************************************************/
void Declared::MakeConstantExpression
(
    ParseTree::Expression *ExpressionTree, // [in]  the expression we want to make a constant expression from
    bool PreEvaluatedExpression, // [in]  determines whether we will need to evaluate the expression in ExpressionTree
    bool IsExplicitlyTyped, // [in]  whether the result of the expression should be cast to a specified type.
    BCSYM_NamedRoot *Context, // [in] the context we're in
    BCSYM **TypeOfConstant, // [in/out] - the type of this constant. May be passed in to coerce result / else reflects type of built BCSYM_Expression
    BCSYM_Expression **BuiltConstantExprSymbol // [out] the created constant expression symbol that contains the result of evaluating ExpressionTree
)
{
    ConstantValue Value;
    bool HasValue = false;

    if ( !ExpressionTree || ExpressionTree->Opcode != ParseTree::Expression::Deferred )
    {
        *BuiltConstantExprSymbol = NULL;
        return;
    }

    // Do not evaluate the expression here. Evaluating would require loading metadata which cannot happen
    // while in declared state (DevDivBugs #26494). Expression evaluation is deferred until bound state.


    if ( !HasValue ) // If this doesn't have a value, we have to create an expression symbol that can be evaluated when going to Bindable state.
    {
        *BuiltConstantExprSymbol = m_SymbolAllocator.GetConstantExpression( &ExpressionTree->TextSpan, // location info
                                                                            Context, // symbol the expression is being created for
                                                                            GetExpressionListHead(), GetExpressionListTail(), // head and tail of expression list so we can tack this one on
                                                                            NULL, // no STRING * expression string
                                                                            ExpressionTree->AsDeferred()->Text, // expression text
                                                                            ExpressionTree->AsDeferred()->TextLengthInCharacters );

        (*BuiltConstantExprSymbol)->SetForcedType(( IsExplicitlyTyped && !(*TypeOfConstant)->IsObject()) ? *TypeOfConstant : NULL );
    }
    else // We do have a value - create a fixed expression symbol.
    {
        *BuiltConstantExprSymbol = m_SymbolAllocator.GetFixedExpression( &Value );
    }
}

/*****************************************************************************
;MakeParams

Build the symbols for a parameter list
*****************************************************************************/
void Declared::MakeParams
(
    _In_opt_z_ STRING *ReturnValueName,    // [in] May be null - used to prevent hiding the function name through a parameter name
    ParseTree::ParameterList *ParameterList,    // [in] the list of parameters to build
    DECLFLAGS    MethodFlags,                  // [in] the flags for the method that owns these parameters
    BCSYM_Proc   *OwningProcedure,              // [in] the procedure symbol we are building parameters for
    DECLFLAGS    ContainerFlags,                // [in] flags for the container where the owning procedure is in
    BCSYM_Param **OutParamSymbols,              // [out] the built parameter symbols
    BCSYM_Param **OutParamSymbolsLast,          // [out] tail of the built parameter symbols (may be null)
    BCSYM        *ImplicitParamTypeSymbol,      // [in] if these are params for a Property Set(), this is the type of the implicit parameter
    BCSYM_Param  *ExistingParameterSymbolList  // [in] existing parameters list to append new parameters to (may be null)
)
{
    BCSYM_Param *TailOfExistingParamList = ExistingParameterSymbolList;
    bool IsBadParameter = false;

    // Point TailOfExistingParamList to the last parameter
    if ( TailOfExistingParamList )
    {
        for ( ; TailOfExistingParamList->GetNext() != NULL; TailOfExistingParamList = TailOfExistingParamList->GetNext());
    }

    // Determine whether all parms should explicitly specify a type (if one is marked explictily as a type, they all must be)
    // VSW # 576826 - we should check each parameter instead of only the first parameter, because any parameter may
    // contain a type, which would force all the others to contain a type as well.
    bool TypeRequiredOnParams = false;
    for ( ParseTree::ParameterList *Param = ParameterList; Param != NULL; Param = Param->Next )
    {
        if ( Param->Element->Type || Param->Element->Name->Name.TypeCharacter != chType_NONE )
        {
            TypeRequiredOnParams = true;
            break;
        }
    }

    // Haul through the parameter list and build the params
    for ( ; ParameterList; ParameterList = ParameterList->Next )
    {
        ParseTree::Parameter *CurrentParameter = ParameterList->Element;
        BCSYM_Expression *ConstExpressionParamSymbol = NULL;

        // Get the name for the parameter.
        ParseTree::IdentifierDescriptor *ParamNameDescriptor = &CurrentParameter->Name->Name;
        if ( CurrentParameter->Name->Name.IsBad ) continue; // no name - not much to do
        STRING *ParamName = ParamNameDescriptor->Name;
        Location *ParamTextSpan = &ParamNameDescriptor->TextSpan;

        if (CurrentParameter->Name->Name.IsNullable && !CurrentParameter->Type&& ! CurrentParameter->Name->Name.TypeCharacter)
        {
            ReportError(ERRID_NullableParameterMustSpecifyType, ParamTextSpan);
            IsBadParameter = true;
        }


        if ( OwningProcedure && OwningProcedure->IsProperty() && StringPool::IsEqual( ParamName, STRING_CONST( m_CompilerInstance, Value )))
        {
            ReportError( ERRID_PropertySetParamCollisionWithValue, ParamTextSpan );

            // We can't allow a Property to have a "Value" parameter because if we do, we are going to collide with
            // Property Set's "Value". Therefore, build the symbol, then we mark it as bad.
            IsBadParameter = true;
        }
        else
        {
            // Does it collide with the name of any other parameter, type parameter or the name of the function?
            if ( !( MethodFlags & DECLF_AllowDupParam ))
            {
                if ( OwningProcedure &&
                     !OwningProcedure->IsUserDefinedOperator() &&  // Operators have no function return variable, so name collision doesn't matter.
                     StringPool::IsEqual( ReturnValueName, ParamName ))
                {
                    ReportError( ERRID_ParamNameFunctionNameCollision, ParamTextSpan );
                    // Don't set bad - only the name is horked and it would be interesting to do overload/overriding/etc semantics on it
                }
                // Does it collide with the name of any type parameter?
                else if ( OwningProcedure &&
                          OwningProcedure->GetGenericParamsHash() &&
                          OwningProcedure->GetGenericParamsHash()->SimpleBind( ParamName ))
                {
                    ReportError( ERRID_NameSameAsMethodTypeParam1, ParamTextSpan, ParamName );
                    // Don't set bad - only the name is horked and it would be interesting to do overload/overriding/etc semantics on it
                }

                for ( BCSYM_Param *CheckParam = ExistingParameterSymbolList; CheckParam; CheckParam = CheckParam->GetNext())
                {
                    if ( !CheckParam->IsBadParam() && StringPool::IsEqual( CheckParam->GetName(), ParamName ))
                    {
                        ReportError( ERRID_DuplicateParamName1, ParamTextSpan, ParamName );
                        // Don't set bad - only the name is horked and it would be interesting to do overload/overriding/etc semantics on it
                        // Once we've logged an error on one param, there is no need to log an error on all other params.
                        break;
                    }
                }
            }
        }

        // Map the flags.
        unsigned ParamFlags = 0;
        for ( ParseTree::ParameterSpecifierList *CurrentSpecifier = CurrentParameter->Specifiers;
              CurrentSpecifier;
              CurrentSpecifier = CurrentSpecifier->Next )
        {
            switch ( CurrentSpecifier->Element->Opcode )
            {
                case ParseTree::ParameterSpecifier::ParamArray:

                    if (!(MethodFlags & DECLF_AllowOptional))
                    {
                        if (MethodFlags & (DECLF_EventAddMethod | DECLF_EventRemoveMethod | DECLF_EventFireMethod))
                        {
                            // Paramarray parameters are illegal on AddHandler, RemoveHandler and RaiseEvent.
                            ReportError(
                                ERRID_EventMethodOptionalParamIllegal1,
                                &CurrentSpecifier->Element->TextSpan,
                                m_CompilerInstance->TokenToString(tkPARAMARRAY));
                        }
                        else
                        {
                            // Optional parameters are illegal on events, delegates, and operators.
                            ReportError(
                                ERRID_ParamArrayIllegal1,
                                &CurrentSpecifier->Element->TextSpan,
                                ContainerFlags & DECLF_Delegate ?
                                    m_CompilerInstance->TokenToString(tkDELEGATE) :
                                    MethodFlags & DECLF_LambdaArguments ?
                                        STRING_CONST(m_CompilerInstance, Lambda) :
                                        StringOfSymbol(m_CompilerInstance, OwningProcedure));
                        }
                    }
                    else
                    {
                        ParamFlags |= PARAMF_ParamArray;
                    }
                    continue;

                case ParseTree::ParameterSpecifier::Optional:

                    if (!(MethodFlags & DECLF_AllowOptional))
                    {
                        if (MethodFlags & (DECLF_EventAddMethod | DECLF_EventRemoveMethod | DECLF_EventFireMethod))
                        {
                            // Optional parameters are illegal on AddHandler, RemoveHandler and RaiseEvent.
                            ReportError(
                                ERRID_EventMethodOptionalParamIllegal1,
                                &CurrentSpecifier->Element->TextSpan,
                                m_CompilerInstance->TokenToString(tkOPTIONAL));
                        }
                        else
                        {
                            // Optional parameters are illegal on events, delegates, and operators.
                            ReportError(
                                ERRID_OptionalIllegal1,
                                &CurrentSpecifier->Element->TextSpan,
                                ContainerFlags & DECLF_Delegate ?
                                    m_CompilerInstance->TokenToString(tkDELEGATE) :
                                    MethodFlags & DECLF_LambdaArguments ?
                                         STRING_CONST(m_CompilerInstance, Lambda) :
                                         StringOfSymbol(m_CompilerInstance, OwningProcedure));
                        }
                    }
                    else
                    {
                        ParamFlags |= PARAMF_Optional;
                    }
                    continue;

                case ParseTree::ParameterSpecifier::ByVal:

                    if (!(ParamFlags & PARAMF_ByRef))
                    {
                        ParamFlags |= PARAMF_ByVal;
                    }
                    continue;

                case ParseTree::ParameterSpecifier::ByRef:

                    // ByRef parameters are illegal in properties and operators
                    if (MethodFlags & DECLF_DisallowByref)
                    {
                        // Properties and operator cannot exist in delegates. So this restriction
                        // can never be applicable for delegates.
                        VSASSERT(!(ContainerFlags & DECLF_Delegate), "Unexpected restriction for delegate!!!");

                        // Note that byref parameters are allowed for an event's fire method.
                        VSASSERT(!(MethodFlags & DECLF_EventFireMethod), "Byref params should be allowed for event fire methods.");

                        if (MethodFlags & (DECLF_EventAddMethod | DECLF_EventRemoveMethod))
                        {
                            // Byref parameters are not allowed for an event's add or remove methods.
                            // Note that byref parameters are allowed for an event's fire method.
                            ReportError(
                                ERRID_EventAddRemoveByrefParamIllegal,
                                &CurrentSpecifier->Element->TextSpan);
                        }
                        else
                        {
                            ReportError(
                                ERRID_ByRefIllegal1,
                                &CurrentSpecifier->Element->TextSpan,
                                StringOfSymbol(m_CompilerInstance, OwningProcedure));
                        }
                    }
                    else if (!(ParamFlags & PARAMF_ByVal))
                    {
                        ParamFlags |= PARAMF_ByRef;
                    }
                    continue;

                default:
                    VSFAIL("unexpected parameter specifier");
            } // switch
        } // loop specifiers

        if ( !( ParamFlags & PARAMF_ByVal ) && !( ParamFlags & PARAMF_ByRef )) // If neither byval or byref is specified, default to byval.
        {
            ParamFlags |= PARAMF_ByVal;
        }

        BCSYM *SymbolForParamType;

        // Make the type symbol for the parameter.
        // If the parameter has no location, this means that it's an implicit param, which tells us to use the param type passed in.
        if ( (CurrentParameter->TextSpan.IsValid() || MethodFlags & DECLF_LambdaArguments)&& OwningProcedure )
        {
            SymbolForParamType = MakeType(
                CurrentParameter->Type,
                ParamNameDescriptor,
                OwningProcedure,
                GetTypeList(),
                (MethodFlags & DECLF_LambdaArguments)? DECLF_LambdaArguments : 0);
        }
        else
        {
            VSASSERT( ImplicitParamTypeSymbol || !OwningProcedure, "We should have a type for default property Set parameter" );
            SymbolForParamType = ImplicitParamTypeSymbol;
        }

        if ((CurrentParameter->Name->ArrayInfo && CurrentParameter->Type && CurrentParameter->Type->Opcode == ParseTree::Type::Nullable) ||
            (CurrentParameter->Name->Name.IsNullable && CurrentParameter->Type &&
            ( CurrentParameter->Type->Opcode == ParseTree::Type::ArrayWithSizes ||
              CurrentParameter->Type->Opcode == ParseTree::Type::ArrayWithoutSizes )))
        {
            ReportError( ERRID_CantSpecifyArrayAndNullableOnBoth, &CurrentParameter->Type->TextSpan );
            SymbolForParamType = m_SymbolAllocator.GetGenericBadNamedRoot();
        }
        else if ( CurrentParameter->Name->ArrayInfo )
        {
            if ( CurrentParameter->Type &&
                 ( CurrentParameter->Type->Opcode == ParseTree::Type::ArrayWithSizes ||
                   CurrentParameter->Type->Opcode == ParseTree::Type::ArrayWithoutSizes ))
            {
                ReportError( ERRID_CantSpecifyArraysOnBoth, &CurrentParameter->Type->TextSpan );
                SymbolForParamType = m_SymbolAllocator.GetGenericBadNamedRoot();
            }
            else if (SymbolForParamType)
            {
                SymbolForParamType = MakeArrayType(CurrentParameter->Name->ArrayInfo, SymbolForParamType);
            }
            else if (MethodFlags & DECLF_LambdaArguments)
            {
                // In lambda parameters's we don't allow array specifiers without the type since
                // inference can become ambiguous, see bug dd:80897
                ReportError(ERRID_CantSpecifyParamsOnLambaParamNoType, &CurrentParameter->Name->TextSpan );
                SymbolForParamType = m_SymbolAllocator.GetGenericBadNamedRoot();
                IsBadParameter = true;
            }
        }

        if ( ParamFlags & PARAMF_ParamArray && SymbolForParamType)
        {
            // A param array needs to be a one-dimensional array of something...
            if ( !SymbolForParamType->IsArrayType())
            {
                SymbolForParamType = m_SymbolAllocator.GetGenericBadNamedRoot();
                ReportError( ERRID_ParamArrayNotArray, ParamTextSpan );
                ParamFlags &= ~PARAMF_ParamArray;
                if (OwningProcedure && !(MethodFlags & DECLF_LambdaArguments))
                {
                    OwningProcedure->SetIsBad( m_SourceFile ); // Microsoft: why do we have to do this (use this particular setIsBad?)
                }
            }
            else if ( SymbolForParamType->PArrayType()->GetRank() != 1 )
            {
                ReportError( ERRID_ParamArrayRank, ParamTextSpan );
            }
        }

        // If any parameter has an explicit type, then all parameters must have an explicit type.
        if ( !( MethodFlags & DECLF_PropSet ) && // because we synthesize this method, and the trailing Value as <>, we need to ignore this rule because the user may not have explicitly typed his args in the property def
                !( MethodFlags & DECLF_AllowParamTypingInconsistency))
        {
            if ( TypeRequiredOnParams && !( CurrentParameter->Type || CurrentParameter->Name->Name.TypeCharacter != chType_NONE ) )
            {
                ReportError( ERRID_ParamTypingInconsistency, ParamTextSpan );
                if (OwningProcedure && !(MethodFlags & DECLF_LambdaArguments))
                {
                    OwningProcedure->SetIsBad( m_SourceFile );
                }
            }
        }

        // If we have attributes on Lambda parameters, report an error.
        if ((MethodFlags & DECLF_LambdaArguments) && CurrentParameter->Attributes)
        {
            Location *ErrorLocation = ParamTextSpan;
            if (CurrentParameter->Attributes->Element)
            {
                ErrorLocation = &CurrentParameter->Attributes->Element->TextSpan;
            }
            ReportError( ERRID_LambdasCannotHaveAttributes, ErrorLocation );
            IsBadParameter = true;
        }

        // Get the value, if we have one.
        if ( ParamFlags & PARAMF_Optional )
        {
            if ( CurrentParameter->AsOptional()->DefaultValue->Opcode == ParseTree::Type::SyntaxError )
            {
                ParamFlags &= ~PARAMF_Optional;
            }
            else
            {
                // MakeConstantExpression can mutate the passed-in type.
                // Introduce a temporary to prevent corrupting the parameter type.

                BCSYM *ParameterValueType = SymbolForParamType;

                MakeConstantExpression( CurrentParameter->AsOptional()->DefaultValue,
                                        false, // expression isn't pre-evaluated
                                        // If the type of the parameter is Object, let the default
                                        // value be of any type. Forcing the type to Object
                                        // can generate bogus errors about converting from
                                        // intrinsic types to Object in constant expressions.
                                        ParameterValueType && !ParameterValueType->IsObject(), // IsExplicitlyTyped
                                        OwningProcedure,
                                        &ParameterValueType, // the type it should be
                                        &ConstExpressionParamSymbol ); // pick up the new symbol, here
                // OPTIONAL: In the above call,
                // "Optional ByVal x As Integer = 1", "Optional ByVal x as Integer = Nothing" -- ParameterValueType is Integer
                // "Optional ByVal x as Object = Nothing" -- ParameterValueType is Object, but we pass IsExplicitlyTyped=false.
                // The effect of IsExplicitlyTyped=False is to return a ConstExpressionSymbol with ForcedType==NULL.
                // So, in EvaluateDeclaredExpression, it sets CastType==NULL.
                // So, in Semantics::InterpretConstantExpression, it gets TargetType=NULL.
                // In other words, the effect of our "IsExplicitlyTyped==false" here is to encode the
                // target type of Object as "NULL" inside InterpretConstantExpression, which then has
                // to decode it back as Object.
            }
        }

        // Make this byref, if needed regenerate
        if ( !( ParamFlags & PARAMF_ByVal ) && SymbolForParamType)
        {
            SymbolForParamType = m_SymbolAllocator.MakePtrType( SymbolForParamType );
        }

        // Create the parameter.
        BCSYM_Param *ParamJustCreated = m_SymbolAllocator.GetParam( ParamTextSpan,
                                                                    ParamName,
                                                                    SymbolForParamType,
                                                                    ParamFlags,
                                                                    ConstExpressionParamSymbol,
                                                                    &ExistingParameterSymbolList, // tack the parameter onto this list
                                                                    &TailOfExistingParamList,
                                                                    false );

        // Mark the parameter if it represents a Query iteration variable
        AssertIfTrue(!CurrentParameter->IsQueryIterationVariable && CurrentParameter->IsQueryRecord);
        ParamJustCreated->SetIsQueryIterationVariable(CurrentParameter->IsQueryIterationVariable);

        if(ParamJustCreated->IsQueryIterationVariable())
        {
            ParamJustCreated->SetIsQueryRecord(CurrentParameter->IsQueryRecord);
        }

        if
        (
            IsBadParameter ||
            (
                ParamJustCreated &&
                ParamJustCreated->GetRawType() &&
                ParamJustCreated->GetRawType()->IsBad()
            )
        )
        {
            ParamJustCreated->SetIsBad();
            if (OwningProcedure && !(MethodFlags & DECLF_LambdaArguments))
            {
                OwningProcedure->SetIsBad();
            }
        }

        if ( !(MethodFlags & DECLF_LambdaArguments))
        {
            // Store away the custom attributes.
            ProcessAttributeList( CurrentParameter->Attributes, ParamJustCreated, OwningProcedure );
        }

        // Check for OptionCompareAttribute
        // NOTE: Microsoft - The attributes at this point have not been processed, so the GetOptionCompareData has
        //                not been set.  This only affects callers of function args marked with <OptionCompareAttribute>
        //                when called within the same project.  If we don't want to open this attribute to developers
        //                then it isn't necessary to process it here.
        // if (ParamJustCreated->GetPAttrVals()->GetOptionCompareData())
        // {
        //     ParamJustCreated->SetIsOptionCompare(true);
        // }
    } // haul through parameter list

    *OutParamSymbols = ExistingParameterSymbolList;
    if ( OutParamSymbolsLast )
    {
        *OutParamSymbolsLast = TailOfExistingParamList;
    }
}

/*****************************************************************************
;MakeMemberVar

Create a member variable for a class / module that was declared using Const,
WithEvents, Dim, Private|Public|Friend|etc.
*****************************************************************************/
void Declared::MakeMemberVar
(
    ParseTree::StatementList *StatementListTree, // [in] the variables in the parse trees
    DECLFLAGS ClassDeclFlags, // [in] the flags for the class in which the member var is defined
    bool &NeedSharedConstructor,  // [out] whether a shared constructor is needed
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    bool DefinedInStdModule = ClassDeclFlags & DECLF_Module;    // variable is defined in a standard module
    bool DefinedInStructure = ClassDeclFlags & DECLF_Structure; // variable is defined in a structure
    bool FirstVariable = true;      // XMLDoc comments is only generated for the first valid variable.
    ParseTree::VariableDeclarationStatement *VariableDeclarationStatement = StatementListTree->Element->AsVariableDeclaration();

    // if ( VariableDeclarationStatement->HasSyntaxError ) return; // prevent ourselves from creating possibly malformed symbols #261003

    DECLFLAGS SpecifierFlags = MapSpecifierFlags( VariableDeclarationStatement->Specifiers );

    // Given dim a,b as integer, c,d as string - haul through the integer decls, then the string decls
    for ( ParseTree::VariableDeclarationList *DeclarationList = VariableDeclarationStatement->Declarations;
          DeclarationList;
          DeclarationList = DeclarationList->Next )
    {
        ParseTree::VariableDeclaration *VariableDeclaration = DeclarationList->Element;
        ParseTree::Type *VariableType = VariableDeclaration->Type; // All variables in the list are of this type

        // you can't have dim a,b as string = "x"
        if ( VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer && VariableDeclaration->Variables->Next )
        {
            ReportError( ERRID_InitWithMultipleDeclarators, &VariableDeclaration->TextSpan );
        }

        ParseTree::Declarator *Declarator = NULL;
        DECLFLAGS VariableFlags = 0;

        // Haul through all the variables declared for a type, e.g. a,b as integer
        for ( ParseTree::DeclaratorList *DeclaratorList = VariableDeclaration->Variables; // the list of variables being declared
              DeclaratorList;
              DeclaratorList = DeclaratorList->Next )
        {
            Declarator = DeclaratorList->Element;

            // If this tree doesn't have a name tree then there isn't much point in continuing...
            if ( Declarator->Name.IsBad )
            {
                continue;
            }

            // Convert parse tree flags into declaration flags (private, withevents, etc.)
            VariableFlags = SpecifierFlags | MapVarDeclFlags( Declarator, VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew ?
                                                              VariableDeclaration->AsNew() : NULL );

            // If we are going to generate synthetic code with names longer that the maximum allowable identifier length, generate an error here
            if ((VariableFlags & DECLF_WithEvents) && (StringPool::StringLength(Declarator->Name.Name) + MAXIMUM_WITHEVENTS_PREFIX_LEN > MaxIdentifierLength))
            {
                ReportError(ERRID_WithEventsNameTooLong, &Declarator->Name.TextSpan);
                continue;
            }

            // Allocate the variable
            BCSYM_Variable *Variable;
            if ( Declarator->ArrayInfo && Declarator->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes ) // allocate array var
            {
                Variable = m_SymbolAllocator.AllocVariableWithArraySizes( Declarator->ArrayInfo->AsArrayWithSizes()->Rank );
            }
            else
            {
                bool HasInitializer = VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew ||
                                      VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer;

                Variable = m_SymbolAllocator.AllocVariable( true, // hasLocation
                                                            HasInitializer || ( VariableFlags & DECLF_Const )); // whether this variable has a value
            }

            // Semantics on initializers
            if ( Declarator->ArrayInfo && VariableFlags & DECLF_New ) // Can't "As New" an array
            {
                ReportDeclFlagError( ERRID_AsNewArray, DECLF_New, VariableDeclarationStatement->Specifiers );
                VariableFlags &= ~DECLF_New;
                Variable->SetIsBad( m_SourceFile );
            }

            if ( Declarator->ArrayInfo &&
                 Declarator->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes &&
                 VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer )
            {
                ReportError( ERRID_InitWithExplicitArraySizes, &Declarator->TextSpan );
            }

            //
            // Make sure that the flags on this variable declaration are correct.
            //

            VARIABLEKIND BuiltVariableKind;

            // CONST checks
            if ( VariableFlags & DECLF_Const ) // Check for CONST errors first, since those errors preclude other types of errors
            {
                BuiltVariableKind = VAR_Const;
                ConstantVarSemantics( VariableDeclarationStatement,
                                      Declarator,
                                      VariableDeclaration,
                                      false, // not a local, this is a const field
                                      &SpecifierFlags,
                                      Variable );
            }
            // WITHEVENTS checks
            else if ( VariableFlags & DECLF_WithEvents ) // WithEvents errors are next on the error precedence list.
            {
                BuiltVariableKind = VAR_WithEvents;

                if ( !VariableDeclaration->Type ) // make sure they didn't do something like 'WithEvents Ev1' and then forget to do the 'As Class' part ( # 37185 )
                {
                    ReportError( ERRID_WithEventsRequiresClass, &Declarator->Name.TextSpan );
                    Variable->SetIsBad( m_SourceFile );
                }
                else if ( Declarator->ArrayInfo != NULL )
                {
                    ReportError( ERRID_EventSourceIsArray, &VariableDeclaration->Type->TextSpan );
                    Variable->SetIsBad( m_SourceFile );
                }
                else if ( VariableFlags & (~DECLF_ValidWithEventsFlags )) // check valid modifiers that can precede WithEvent declarations
                {
                    ReportDeclFlagError( ERRID_BadWithEventsFlags1, VariableFlags & (~DECLF_ValidWithEventsFlags ), VariableDeclarationStatement->Specifiers );
                    VariableFlags &= DECLF_ValidWithEventsFlags;
                }
            }
            else // We're looking at a DIM, NEW, DEFAULT, SHARED, HIDDEN, PRIVATE, PROTECTED, PUBLIC, FRIEND declaration
            {
                BuiltVariableKind = VAR_Member;
                if ( VariableFlags & ( ~DECLF_ValidGeneralDeclFlags )) // check for for invalid flags that universally apply to DIM, NEW, DEFAULT, SHARED, HIDDEN, PRIVATE, PROTECTED, PUBLIC, FRIEND statements
                {
                    ReportDeclFlagError( ERRID_BadDimFlags1, VariableFlags & ( ~DECLF_ValidGeneralDeclFlags ), VariableDeclarationStatement->Specifiers );
                    VariableFlags &= DECLF_ValidGeneralDeclFlags;
                }
            }

            //
            // --- Various edge semantic checks ( typically deals with certain combinations of things that are bad ) ---
            //

            // Semantics on member vars inside a standard module
            if ( DefinedInStdModule )  // In a shared class, all the members are marked as shared - but they shouldn't be explicitly marked that way or have other modifier flags
            {
                if ( VariableFlags & DECLF_InvalidStdModuleMemberFlags )
                {
                    ReportDeclFlagError( ERRID_ModuleCantUseVariableSpecifier1, VariableFlags & DECLF_InvalidStdModuleMemberFlags, VariableDeclarationStatement->Specifiers );
                    VariableFlags &= ~DECLF_InvalidStdModuleMemberFlags;
                }

                // Although the user shouldn't Explicitly mark these as shared, we mark them as shared since member vars in std modules are implicitly shared
                VariableFlags |= DECLF_Shared;
            }

            // Semantics on member vars inside a structure
            if ( DefinedInStructure )
            {
                DECLFLAGS BadStructMemberFlags = DECLF_Protected | DECLF_ProtectedFriend | DECLF_WithEvents;
                if ( VariableFlags & BadStructMemberFlags )
                {
                    ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1, VariableFlags & BadStructMemberFlags, VariableDeclarationStatement->Specifiers );
                    VariableFlags &= ~BadStructMemberFlags;
                }

                // Non-shared structure members cannot have initializers
                //
                if ( ( VariableFlags & ( DECLF_Shared | DECLF_New )) == DECLF_New )
                {
                    ReportDeclFlagError( ERRID_SharedStructMemberCannotSpecifyNew, VariableFlags & DECLF_New, VariableDeclarationStatement->Specifiers );
                    VariableFlags &= ~DECLF_New;
                }
            }

            // get rid of any error causing flags from the common flags so that we report
            // the errors on these only once although multiple variables are declared in
            // the same declaration statement
            //
            SpecifierFlags &= VariableFlags;

            // If no access flags are set, make the variable private or public depending on the container
            if ( !( VariableFlags & DECLF_AccessFlags ))
            {
                if ( DefinedInStdModule ) // fields of std modules are private by default
                {
                    VariableFlags |= DECLF_Private;
                }
                else
                {
                    if ( DefinedInStructure ) // fields of structs are public by default
                    {
                        VariableFlags |= DECLF_Public;
                    }
                    else // fields of classes are private by default
                    {
                        VariableFlags |= DECLF_Private;
                    }
                }
            }

            // Fill in the variable symbol.
            // Microsoft  consider: the type is going to be the same for all the entries in the list so it seems to make sense to
            // build the type once outside this loop and pass it in.  The trouble is type-characters.  I need to go through the
            // logic in MakeType() inside MakeVariable() each time because the name may have a type character or there may
            // be an explicit type AND a type character, etc. and we need to catch those errors
            MakeVariable( VariableDeclarationStatement->Attributes, Declarator, VariableDeclaration, VariableFlags, BuiltVariableKind, false, Variable, OwningSymbolList );

            if ( VariableDeclarationStatement->HasSyntaxError )
            {
                Variable->SetIsBad( m_SourceFile ); // prevent ourselves from creating possibly malformed symbols #261003
            }

            // Do we need to create a shared initializer for this variable?
            if ( (VariableFlags & DECLF_Shared &&
                  ( VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer ||
                    VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew ||
                    ContainsArrayWithSizes( DeclaratorList ))) ||
                 (VariableFlags & DECLF_Const &&
                   Variable->GetRawType()->IsIntrinsicType() &&
                   (Variable->GetType()->GetVtype() == t_decimal ||
                    Variable->GetType()->GetVtype() == t_date)))

            {
                NeedSharedConstructor = true;
            }

            // If we build a variable symbol, go ahead and process any XMLDoc comments that may be need to be attached
            // to this variable symbol. Notice that we only do this for the first valid (with symbol) variable, so the
            // XMLDoc comment only applies to that variable.
            if ( Variable && FirstVariable )
            {
                ExtractXMLDocComments( StatementListTree, Variable );
                FirstVariable = false;
            }
        } // while ( ParseTree::DeclaratorList *DeclaratorList = VariableDeclaration->Variables )

        if ( DefinedInStructure && !( VariableFlags & DECLF_Shared ) )
        {
            if ( Declarator && Declarator->ArrayInfo && Declarator->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes )
            {
                ReportError( ERRID_ArrayInitInStruct, &Declarator->TextSpan );
            }
            else if ( VariableDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer && !( VariableFlags & DECLF_Const ))
            {
                ReportError( ERRID_InitializerInStruct, &Declarator->Name.TextSpan );
            }
        }
    } // while ( ParseTree::VariableDeclarationList *DeclarationList = VariableDeclarationStatement->Declarations )
}

UserDefinedOperators
MapToUserDefinedOperator
(
    tokens TokenType,
    int ParameterCount,
    DECLFLAGS Flags
)
{
    UserDefinedOperators Result;

    switch (TokenType)
    {
        case tkCTYPE:      Result = (Flags & DECLF_Widening) ? OperatorWiden : OperatorNarrow; break;
        case tkISTRUE:     Result = OperatorIsTrue; break;
        case tkISFALSE:    Result = OperatorIsFalse; break;
        case tkMinus:      Result = (ParameterCount == 1) ? OperatorNegate : OperatorMinus; break;
        case tkNOT:        Result = OperatorNot; break;
        case tkPlus:       Result = (ParameterCount == 1) ? OperatorUnaryPlus : OperatorPlus; break;
        case tkMult:       Result = OperatorMultiply; break;
        case tkDiv:        Result = OperatorDivide; break;
        case tkPwr:        Result = OperatorPower; break;
        case tkIDiv:       Result = OperatorIntegralDivide; break;
        case tkConcat:     Result = OperatorConcatenate; break;
        case tkShiftLeft:  Result = OperatorShiftLeft; break;
        case tkShiftRight: Result = OperatorShiftRight; break;
        case tkMOD:        Result = OperatorModulus; break;
        case tkOR:         Result = OperatorOr; break;
        case tkXOR:        Result = OperatorXor; break;
        case tkAND:        Result = OperatorAnd; break;
        case tkLIKE:       Result = OperatorLike; break;
        case tkEQ:         Result = OperatorEqual; break;
        case tkNE:         Result = OperatorNotEqual; break;
        case tkLT:         Result = OperatorLess; break;
        case tkLE:         Result = OperatorLessEqual; break;
        case tkGE:         Result = OperatorGreaterEqual; break;
        case tkGT:         Result = OperatorGreater; break;
        default:
            VSFAIL("unexpected operator token");
            Result = OperatorUNDEF;
    }

    VSASSERT(TokenType == OperatorTokenTypes(Result), "inconsistent mapping");
    return Result;
}

/*****************************************************************************
;MakeOperator

Create an operator implementation for a class/structure

Operators exist in the symbol table as two symbols:
    BCSYM_UserDefinedOperator called the "Operator symbol".
    BCSYM_MethodImpl called the "Operator Method symbol".

The Operator symbol is the symbol with the true name of the operator as well
as an enum value specifying the operator kind.

The Operator method symbol exists to allow users to call operators as methods
using the CLS name.  E.g., Foo.op_Addition(), Foo.op_Subtraction(), etc.

All Bindable semantics occurs on the Operator symbol.
All method body semantics and metaemit work occurs on the Operator Method symbol.

Example declaration:

    "Shared Operator +(x As Foo, y As Foo) As Foo"

    Operator Symbol:
        Name                  == "+"
        Emitted Name          == "op_Addition"
        Binding space         == Ignore
        Container hash        == Unbindable hash
        IsSpecialName         == YES
        Operator              == OperatorAddition
        AssociatedMethod      == <points to Operator Method symbol>

    Operator Method Symbol:
        Name                  == "op_Addition"
        Emitted Name          == "op_Addition"
        Binding space         == Normal
        Container hash        == members hash
        IsSpecialName         == YES
        AssociatedOperatorDef == <points to Operator symbol>

Both symbols shared the same param list and xml doc comments.

*****************************************************************************/
void Declared::MakeOperator
(
    ParseTree::StatementList *StatementListTree, // [in] the parse tree for the method
    BCSYM_Class *OwningClass, // [in] class that owns the symbols being built
    DECLFLAGS ClassFlags, // [in] the flags on the class that owns this method symbol
    bool &NeedSharedConstructor,  // [out] whether a shared constructor is needed
    SymbolList *OwningSymbolList, // [in] we put the built method symbol on this list
    SymbolList *BackingStaticFieldsList // [in] class fields to back statics put on here
)
{
    ParseTree::OperatorDefinitionStatement *MethodTree = StatementListTree->Element->AsOperatorDefinition();

    if (MethodTree->Name.IsBad) // If there is no name, there isn't much for us to do.
    {
        return;
    }

    DECLFLAGS MethodFlags = MapProcDeclFlags(MethodTree);
    Location *MethodNameLocation = &MethodTree->Name.TextSpan;

    int ParameterCount = 0;
    ParseTree::ParameterList *Cursor = MethodTree->Parameters;
    while (Cursor)
    {
        ParameterCount++;
        Cursor = Cursor->Next;
    }

    // Determine the operator kind, which is partially determined by the number of parameters in the declaration.
    tokens OperatorTokenType = MethodTree->OperatorTokenType;

    UserDefinedOperators Operator = MapToUserDefinedOperator(OperatorTokenType, ParameterCount, MethodFlags);

    // Operators must be Public.
    if ((MethodFlags & DECLF_AccessFlags) && !(MethodFlags & DECLF_Public))
    {
        ReportDeclFlagError(ERRID_OperatorMustBePublic, MethodFlags & DECLF_AccessFlags, MethodTree->Specifiers);
        MethodFlags &= ~DECLF_AccessFlags;
    }

    // Operators must be Shared.
    if (!(MethodFlags & DECLF_Shared))
    {
        ReportError(ERRID_OperatorMustBeShared, MethodNameLocation);
    }

    // Widening and Narrowing can be specified only on conversion operators.
    if (!IsConversionOperator(Operator) &&
        (MethodFlags & (DECLF_Widening | DECLF_Narrowing)))
    {
        ReportDeclFlagError(
            ERRID_InvalidSpecifierOnNonConversion1,
            MethodFlags & (DECLF_Widening | DECLF_Narrowing),
            MethodTree->Specifiers);
        MethodFlags &= ~(DECLF_Widening | DECLF_Narrowing);
    }
    // Conversion operators must specify either Widening or Narrowing.
    else if (IsConversionOperator(Operator) &&
             !(MethodFlags & (DECLF_Widening | DECLF_Narrowing)))
    {
        ReportError(ERRID_ConvMustBeWideningOrNarrowing, MethodNameLocation);
    }

    // Can't be OVERLOADS and SHADOWS
    if ((MethodFlags & DECLF_ShadowsKeywordUsed) && (MethodFlags & DECLF_OverloadsKeywordUsed))
    {
        ReportDeclFlagComboError(DECLF_ShadowsKeywordUsed | DECLF_OverloadsKeywordUsed, MethodTree->Specifiers);
        MethodFlags &= ~DECLF_OverloadsKeywordUsed;
    }

    // Generic flag problems caught here
    if (MethodFlags & ~DECLF_ValidOperatorFlags)
    {
        ReportDeclFlagError(ERRID_BadOperatorFlags1, MethodFlags & ~DECLF_ValidOperatorFlags, MethodTree->Specifiers);
        MethodFlags &= DECLF_ValidOperatorFlags;
    }

    // Operators cannot be declared in Modules.
    if (ClassFlags & DECLF_Module)
    {
        ReportError(ERRID_OperatorDeclaredInModule, MethodNameLocation);
    }

    VSASSERT(MethodTree->Implements == NULL && MethodTree->Handles == NULL, "parser should have rejected implements and handles");

    // Done with most checks.  Make sure the operator is Shared and Public.
    MethodFlags |= DECLF_Public | DECLF_Shared;

    OwningClass->SetHasUserDefinedOperators(true);

    //
    // Create the operator symbol.
    //
    BCSYM_UserDefinedOperator *OperatorDecl = m_SymbolAllocator.AllocUserDefinedOperator(true);
    BCSYM *ReturnTypeSymbol = NULL;
    BCSYM_Param *ParameterSymbolList = NULL;

    MakeProc(MethodTree->Opcode,
             &MethodTree->Name,
             NULL,
             MethodTree->Parameters,
             MethodTree->ReturnType,
             MethodTree->ReturnTypeAttributes,
             NULL, // no lib
             NULL, // no alias
             MethodTree->GenericParameters,
             MethodFlags,
             false,  // Operators can't be declared in Interfaces.
             &MethodTree->BodyLocation,
             &MethodTree->EntireMethodLocation,
             OperatorDecl,
             GetUnBindableChildrenSymbolList(), // attach proc to this list
             NULL,
             NULL,
             MethodTree->Specifiers,
             ClassFlags,
             &ReturnTypeSymbol,     // Use the same NamedType symbol that was created for the property
             &ParameterSymbolList,  // Use the list of parameters already built
             Operator);

    OperatorDecl->SetBindingSpace(BINDSPACE_IgnoreSymbol);

    // Store away the custom attributes.
    ProcessAttributeList(MethodTree->Attributes, OperatorDecl);

    //
    // Create the procedure symbol which represents the actual operator method.
    //
    BCSYM_MethodImpl *OperatorMethodSymbol = m_SymbolAllocator.AllocMethodImpl(true);

    MakeProc(ParseTree::Statement::FunctionDeclaration,
             &MethodTree->Name,
             OperatorDecl->GetEmittedName(),
             NULL, // params already built
             NULL, // return type already built
             NULL, // return type attributes already built
             NULL, // no lib
             NULL, // no alias
             MethodTree->GenericParameters,
             MethodFlags & ~(DECLF_Widening | DECLF_Narrowing),
             false,  // Operators can't be declared in Interfaces.
             &MethodTree->BodyLocation,
             &MethodTree->EntireMethodLocation,
             OperatorMethodSymbol,
             OwningSymbolList, // attach proc to this list
             NULL,
             NULL,
             MethodTree->Specifiers,
             ClassFlags,
             &ReturnTypeSymbol,
             &ParameterSymbolList);

    OperatorMethodSymbol->SetAssociatedOperatorDef(OperatorDecl);
    OperatorDecl->SetOperatorMethod(OperatorMethodSymbol);

    // Store away the custom attributes.
    ProcessAttributeList(MethodTree->Attributes, OperatorMethodSymbol);

    // Creating backing fields for any static vars
    CreateBackingFieldsForStatics(
        MethodTree->StaticLocalDeclarations,
        OperatorMethodSymbol,
        ClassFlags,
        NeedSharedConstructor,
        BackingStaticFieldsList);

    // Go ahead and process any XMLDoc comments that may be need to be attached to this method symbol.
    ExtractXMLDocComments(StatementListTree, OperatorDecl);
    // Both the operator and its associated method symbol point to the same xmldoc strcuture.
    OperatorMethodSymbol->SetXMLDocNode(OperatorDecl->GetXMLDocNode());

    // The rest of the compiler has strict assumptions about the number of parameters for operators.
    // If the param count does not match, mark the operator bad.  In most cases, generate an error.
    //
    bool OperatorIsBad = false;

    if (OperatorDecl->GetParameterCount() != ParameterCount)
    {
        // If the number of parameter symbols does not match the number of parse tree param nodes,
        // then that means the parse tree had bad nodes and an error has already been reported by the parser.
        // Just mark the operator bad.
        OperatorIsBad = true;
    }
    else if (IsUnaryOperator(Operator))
    {
        if (ParameterCount != 1)
        {
            ReportError(ERRID_OneParameterRequired1, MethodNameLocation, m_CompilerInstance->OperatorToString(Operator));
            OperatorIsBad = true;
        }
    }
    else
    {
        VSASSERT(IsBinaryOperator(Operator), "must be binary - how can it be otherwise?");

        if (ParameterCount != 2)
        {
            ERRID ErrorId =
                OperatorTokenType == tkPlus || OperatorTokenType == tkMinus ?
                    ERRID_OneOrTwoParametersRequired1 :
                    ERRID_TwoParametersRequired1;

            ReportError(ErrorId, MethodNameLocation, m_CompilerInstance->OperatorToString(Operator));
            OperatorIsBad = true;
        }
    }

    if (OperatorIsBad)
    {
        OperatorDecl->SetIsBad(m_SourceFile);
        OperatorMethodSymbol->SetIsBad(m_SourceFile);
    }
}


/*****************************************************************************
;MakeMethodImpl

Create a method implementation (vs. just a declaration as for an interface)
for a class/structure/module
*****************************************************************************/
void Declared::MakeMethodImpl
(
    ParseTree::StatementList *StatementListTree, // [in] the parse tree for the method
    DECLFLAGS ClassFlags, // [in] the flags on the class that owns this method symbol
    bool IsInterface, // [in] is this method being defined in an interface?
    bool &HasMustOverrideMethod, // [in/out] whether we have a must override method
    bool &SeenConstructor, // [in/out] whether we've seen a constructor
    bool &SeenSharedConstructor, // [in/out] whether we've seen a shared constructor
    bool &NeedSharedConstructor,  // [out] whether a shared constructor is needed
    SymbolList *OwningSymbolList, // [in] we put the built method symbol on this list
    SymbolList *BackingStaticFieldsList // [in] class fields to back statics put on here
)
{
    ParseTree::MethodDefinitionStatement *MethodTree = StatementListTree->Element->AsMethodDefinition();

    if ( MethodTree->Name.IsBad ) // If there is no name, there isn't much for us to do.
    {
        return;
    }

    BCSYM_HandlesList *HandlesList = NULL;
    BCSYM_ImplementsList *ImplementsList = NULL;
    DECLFLAGS MethodFlags = DECLF_AllowOptional;
    BCSYM_MethodImpl *MethodImpl = m_SymbolAllocator.AllocMethodImpl( true ); // The context is now the method we are creating
    Location *MethodNameLocation = &MethodTree->Name.TextSpan;

    // Check the flags.
    MethodFlags |= MapProcDeclFlags( MethodTree );

    // Catch specifier errors on Structures / NotInheritable classes
    if ( ClassFlags & DECLF_NotInheritable )
    {
        // Semantics for members declared in classes that are final, i.e. 'NotInheritable'
        DECLFLAGS InvalidNotInheritableFlagsMinusProtected = DECLF_InvalidFlagsInNotInheritableClass & ~( DECLF_Protected | DECLF_ProtectedFriend ); // Protected is dealt with in bindable

        if ( MethodFlags & InvalidNotInheritableFlagsMinusProtected )
        {
            ERRID error = ( ClassFlags & DECLF_Structure) ? ERRID_StructCantUseVarSpecifier1 : ERRID_BadFlagsInNotInheritableClass1;
            ReportDeclFlagError( error, MethodFlags & InvalidNotInheritableFlagsMinusProtected, MethodTree->Specifiers );
            MethodFlags &= ~InvalidNotInheritableFlagsMinusProtected;
        }
    }

    // Can't be PRIVATE and NOTOVERRIDABLE
    if (( MethodFlags & DECLF_NotOverridable ) && ( MethodFlags & DECLF_Private ))
    {
        ReportDeclFlagComboError( DECLF_NotOverridable | DECLF_Private, MethodTree->Specifiers );
    }
    // Can't be OVERLOADS and SHADOWS
    if (( MethodFlags & DECLF_ShadowsKeywordUsed ) && ( MethodFlags & DECLF_OverloadsKeywordUsed ))
    {
        ReportDeclFlagComboError( DECLF_ShadowsKeywordUsed | DECLF_OverloadsKeywordUsed, MethodTree->Specifiers );
        MethodFlags &= ~DECLF_OverloadsKeywordUsed;
    }
    // If it is NOTOVERRIDABLE, it must also be marked OVERRIDES
    if (( MethodFlags & DECLF_NotOverridable ) && !( MethodFlags & DECLF_OverridesKeywordUsed ))
    {
        ReportDeclFlagError( ERRID_NotOverridableRequiresOverrides, MethodFlags & DECLF_NotOverridable, MethodTree->Specifiers );
    }

    // Partal method declarations can only be applied to private methods.

    if (( MethodFlags & DECLF_Partial ) )
    {
        // These flags can't be used with partial methods.

        DECLFLAGS badFlags =
            DECLF_Protected |
            DECLF_Public |
            DECLF_Friend |
            DECLF_NotOverridable |
            DECLF_MustOverride |
            DECLF_Overridable |
            DECLF_OverridesKeywordUsed |
            DECLF_MustInherit |
            DECLF_ProtectedFriend;

        // We must explicitly have private.

        if( MethodFlags & badFlags )
        {
            ReportDeclFlagError( ERRID_OnlyPrivatePartialMethods1, MethodFlags & badFlags, MethodTree->Specifiers );
        }
        else if( MethodFlags & DECLF_Async)
        {
            ReportError( ERRID_PartialMethodsMustNotBeAsync1, MethodNameLocation, MethodTree->Name );
        }
        else if( MethodTree->Opcode == ParseTree::Statement::FunctionDeclaration )
        {
            ReportError( ERRID_PartialMethodsMustBeSub1, MethodNameLocation, MethodTree->Name );
        }
        else if( !(MethodFlags & DECLF_Private))
        {
            ReportDeclFlagError( ERRID_PartialMethodsMustBePrivate, DECLF_Partial, MethodTree->Specifiers );
        }
    }

    // Generic flag problems caught here
    if ( MethodFlags & ~DECLF_ValidMethodFlags )
    {
        if ( MethodFlags & DECLF_Static )
        {
            ReportError( ERRID_ObsoleteStaticMethod, MethodNameLocation );
        }

        if ( MethodFlags & ~( DECLF_ValidMethodFlags & DECLF_Static ))
        {
            ReportDeclFlagError( ERRID_BadMethodFlags1, MethodFlags & ~(DECLF_ValidMethodFlags | DECLF_Static ), MethodTree->Specifiers );
        }
        MethodFlags &= DECLF_ValidMethodFlags;
    }

    // If there is a MustOverrides method in the class, keep track of that information so we get an idea about whether the class containing the method is abstract
    if ( MethodFlags & DECLF_MustOverride )
    {
        HasMustOverrideMethod = true;
    }

    // bug 19283 : a shared procedure cannot be marked as overrides, notoverridable or mustoverride
    // Do this check before we set flags to shared for a module below so that we catch the case where they
    // explicitly mark the method as shared along with overrides
    if ( MethodFlags & DECLF_Shared )
    {
        if ( MethodFlags & DECLF_InvalidFlagsOnShared )
        {
            ReportDeclFlagError( ERRID_BadFlagsOnSharedMeth1, MethodFlags & DECLF_InvalidFlagsOnShared, MethodTree->Specifiers );
        }
    }

    // All members in a Standard modules are marked as Shared.
    if ( ClassFlags & DECLF_Module )
    {
        if ( MethodFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseMethodSpecifier1, MethodFlags & DECLF_InvalidStdModuleMemberFlags, MethodTree->Specifiers );
            MethodFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }

        if (MethodFlags & DECLF_OverloadsKeywordUsed)
        {
            ReportDeclFlagError(ERRID_OverloadsModifierInModule, DECLF_OverloadsKeywordUsed, MethodTree->Specifiers );
        }

        // We implicitly mark anything inside a module as Shared
        MethodFlags |= DECLF_Shared;
    }

    if ( ClassFlags & DECLF_Structure )
    {
        // Protected and Protected Friend can actually be specified in a structure, but only if the method is overriding something
        if (  ( MethodFlags & ( DECLF_Protected | DECLF_ProtectedFriend )) &&
             !( MethodFlags & DECLF_OverridesKeywordUsed ))
        {
            ReportDeclFlagError( ERRID_StructureCantUseProtected, MethodFlags & (DECLF_Protected | DECLF_ProtectedFriend), MethodTree->Specifiers );
            MethodFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
        }
    }

    if ( !MethodTree->Name.IsBracketed )
    {
        // Class_Terminate and Class_Initialize are no longer valid
        if ( StringPool::IsEqual( STRING_CONST( m_CompilerInstance, ClassInitialize ), MethodTree->Name.Name ))
        {
            ReportError( WRNID_ObsoleteClassInitialize, MethodNameLocation );
        }
        if ( StringPool::IsEqual( STRING_CONST( m_CompilerInstance, ClassTerminate ), MethodTree->Name.Name ))
        {
            ReportError( WRNID_ObsoleteClassTerminate, MethodNameLocation );
        }
    }

    // Static initializers cannot be declared with any access specifier and are always private.
    if (( MethodTree->Opcode == ParseTree::Statement::ConstructorDeclaration && ( MethodFlags & DECLF_Shared )))
    {
        const DECLFLAGS DECLF_InvalidStaticInitFlags = ( DECLF_AccessFlags | DECLF_NotOverridable | DECLF_MustOverride | DECLF_Overridable );
        if ( MethodFlags & DECLF_InvalidStaticInitFlags )
        {
            ReportDeclFlagError( ERRID_SharedConstructorIllegalSpec1, MethodFlags & DECLF_InvalidStaticInitFlags, MethodTree->Specifiers );
            MethodFlags &= ~DECLF_InvalidStaticInitFlags;
        }
        MethodFlags |= DECLF_Private;
    }
    else
    {
        // Methods default to public if no flags are set
        if ( !( MethodFlags & DECLF_AccessFlags ))
        {
            MethodFlags |= DECLF_Public;
        }
    }

    // Process the IMPLEMENTS lists
    if ( MethodTree->Implements )
    {
        // Methods in Std Modules can't implement interfaces
        if ( ClassFlags & DECLF_Module )
        {
            ReportModuleMemberCannotImplementError( &MethodTree->TextSpan, &MethodTree->HandlesOrImplements );
        }
        // Don't allow implements on a constructor.
        else if ( MethodTree->Opcode == ParseTree::Statement::ConstructorDeclaration )
        {
            ReportError( ERRID_ImplementsOnNew, MethodNameLocation );
        }
        // Don't allow implements on partial method declarations.
        else if (( MethodFlags & DECLF_Partial ))
        {
            ReportError( ERRID_PartialDeclarationImplements1, MethodNameLocation, MethodTree->Name.Name );
        }
        else
        {
            // Implementing with shared methods is illegal.
            const DECLFLAGS DECLF_InvalidOnImplementsFlags = DECLF_Shared;
            if ( MethodFlags & DECLF_InvalidOnImplementsFlags )
            {
                ReportDeclFlagError( ERRID_SharedOnProcThatImpl, MethodFlags & DECLF_InvalidOnImplementsFlags, MethodTree->Specifiers );
                // I don't clear the Shared flag because then you get weird errors from semantics about needing an object reference to the non-shared member, etc.
            }

            ImplementsList = MakeImplementsList( MethodTree->Implements, MethodImpl );
            MethodFlags |= DECLF_Implementing;
        }
    }

    // Process the HANDLES lists
    if ( MethodTree->Handles )
    {
        // Constructors can't handle events
        if ( MethodTree->Opcode == ParseTree::Statement::ConstructorDeclaration )
        {
            ReportError( ERRID_NewCannotHandleEvents, MethodNameLocation );
        }
        else if ( ClassFlags & DECLF_Structure ) // Structures can't have Handles statements
        {
            ReportError( ERRID_StructsCannotHandleEvents, MethodNameLocation );
        }
        else if ( MethodTree->GenericParameters )
        {
            ReportError( ERRID_HandlesInvalidOnGenericMethod, MethodNameLocation );
        }
        else
        {
            HandlesList = MakeHandlesList( MethodTree->Handles, MethodImpl, ClassFlags );
        }
    }

    Symbols::SetHandlesAndImplLists( MethodImpl, HandlesList, ImplementsList );

    //
    // Create the procedure symbol.
    //

    MakeProc( MethodTree->Opcode,
              &MethodTree->Name,
              NULL, // don't need to send a name string since it is in the tree arg above
              MethodTree->Parameters,
              MethodTree->ReturnType,
              MethodTree->ReturnTypeAttributes,
              NULL, // no lib
              NULL, // no alias
              MethodTree->GenericParameters,
              MethodFlags,
              IsInterface,
              &MethodTree->BodyLocation,
              &MethodTree->EntireMethodLocation,
              MethodImpl,
              OwningSymbolList, // attach proc to this list
              &SeenConstructor,
              &SeenSharedConstructor,
              MethodTree->Specifiers,
              ClassFlags,
              NULL,           // Not a property
              NULL );         // No need to get back the list of parameters built

    // Store away the custom attributes.
    ProcessAttributeList( MethodTree->Attributes, MethodImpl );

    // Structures can't have parameterless instance constructors (they may have parameterized class constructors)
    if (( ClassFlags & DECLF_Structure)
          && MethodImpl->IsInstanceConstructor()
          && MethodImpl->GetRequiredParameterCount() == 0 )
    {
        ReportError( ERRID_NewInStruct, MethodNameLocation );
        MethodImpl->SetIsBad( m_SourceFile );
    }

    // Creating backing fields for any static vars
    CreateBackingFieldsForStatics( MethodTree->StaticLocalDeclarations, MethodImpl, ClassFlags,
                                   NeedSharedConstructor, BackingStaticFieldsList );

    // Go ahead and process any XMLDoc comments that may be need to be attached to this method symbol.
    ExtractXMLDocComments( StatementListTree, MethodImpl );
}


/*****************************************************************************
;MakeEvent

Process Event foo(). See GenerateEventHookupSymbols() for a list of synthetic
symbols built in response to a Event foo() declaration.  Can also use
/dump.syntheticcode to see these at compile time as well
*****************************************************************************/
void Declared::MakeEvent
(
    ParseTree::StatementList *StatementListTree,    // [in] the parse tree of the Event declaration
    bool DefinedInInterface,                        // [in] are we defining an Event inside an interface?
    DECLFLAGS ContainerDeclFlags,                   // [in] the flags for the container where the event is defined
    SymbolList *OwningSymbolList,                   // [in] symbol list to add to
    SymbolList *BackingStaticFieldsList,            // [in] the symbol list to add backing fields to
    bool &NeedSharedConstructor                     // [out] whether a shared constructor is needed
)
{
    VSASSERT( StatementListTree->Element->Opcode == ParseTree::Statement::EventDeclaration ||
              StatementListTree->Element->Opcode == ParseTree::Statement::BlockEventDeclaration,
                    "Non event statement unexpected!!!" );

    ParseTree::EventDeclarationStatement *EventTree =
        StatementListTree->Element->Opcode == ParseTree::Statement::EventDeclaration ?
            StatementListTree->Element->AsEventDeclaration() :
            StatementListTree->Element->AsBlockEventDeclaration()->EventSignature;

    if ( EventTree->Opcode == ParseTree::Statement::SyntaxError || EventTree->Name.IsBad )
    {
        return; // Don't even try if we know the event declaration is horked - Bug #34508
    }

    bool DefinedInStruct = ContainerDeclFlags & DECLF_Structure;
    bool DefinedInStdModule = ContainerDeclFlags & DECLF_Module;
    Location *EventLocation = &EventTree->Name.TextSpan;

    // If we are going to generate synthetic code with names longer that the maximum allowable identifier length, generate an error here
    if (StringPool::StringLength(EventTree->Name.Name) + MAXIMUM_EVENT_PREFIX_LEN > MaxIdentifierLength)
    {
        ReportError(ERRID_EventNameTooLong, EventLocation);
        return;
    }

    if ( EventTree->Name.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, EventLocation );
        EventTree->Name.TypeCharacter = chType_NONE; // so we don't get an error on the proc we build for the event
    }

    // Interface events can't claim to implement anything
    if ( DefinedInInterface && EventTree->Implements )
    {
        ReportError( ERRID_InterfaceEventCantUse1, &EventTree->Implements->TextSpan, m_CompilerInstance->TokenToString( tkIMPLEMENTS ));
        return;
    }

    // Create the Event Foo() Declaration for the symbol table
    BCSYM_EventDecl *Event = m_SymbolAllocator.AllocEventDecl( true );

    //
    // Check the flags.
    //

    DECLFLAGS EventDeclFlags = DECLF_AllowDupParam;
    EventDeclFlags |= MapSpecifierFlags( EventTree->Specifiers );
    EventDeclFlags |= DECLF_Function; // Events are functions.

    // if there are decl problems with this event, then all synthetic code will be marked as having decl errors as well
    if ( EventDeclFlags & ~DECLF_ValidEventFlags )
    {
        ReportDeclFlagError( ERRID_BadEventFlags1, EventDeclFlags & ~DECLF_ValidEventFlags, EventTree->Specifiers );
        EventDeclFlags &= DECLF_ValidEventFlags;
        Event->SetIsBad( m_SourceFile );
    }

    if ( DefinedInStdModule )  // In a shared class, all the members are implicitly marked as shared - but they shouldn't be explicitly marked that way or have other modifier flags.
    {
        if ( EventDeclFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseEventSpecifier1, EventDeclFlags & DECLF_InvalidStdModuleMemberFlags, EventTree->Specifiers );
            EventDeclFlags &= ~DECLF_InvalidStdModuleMemberFlags;
            Event->SetIsBad( m_SourceFile );
        }

        // We mark them as shared since member vars in a standard modules are implicitly shared
        EventDeclFlags |= DECLF_Shared;
    }

    if ( DefinedInInterface ) // special semantics on events in interfaces
    {
        if ( EventDeclFlags & DECLF_InvalidInterfaceMemberFlags )
        {
            ReportDeclFlagError( ERRID_InterfaceCantUseEventSpecifier1, EventDeclFlags & DECLF_InvalidInterfaceMemberFlags, EventTree->Specifiers );
            EventDeclFlags &= ~DECLF_InvalidInterfaceMemberFlags;
            Event->SetIsBad( m_SourceFile );
        }
    }

    if ( DefinedInStruct ) // special semantics on events defined in structures
    {
        if ( EventDeclFlags & DECLF_InvalidFlagsInNotInheritableClass )
        {
            ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1, EventDeclFlags & DECLF_InvalidFlagsInNotInheritableClass, EventTree->Specifiers );
            EventDeclFlags &= ~DECLF_InvalidFlagsInNotInheritableClass;
            Event->SetIsBad( m_SourceFile );
        }
    }

    // If no access flags are set, default the event function to public access
    if ( !( EventDeclFlags & DECLF_AccessFlags ))
    {
       EventDeclFlags |= DECLF_Public;
    }

    // Process the IMPLEMENTS lists
    if ( EventTree->Implements )
    {
        if ( DefinedInStdModule )
        {
            ReportModuleMemberCannotImplementError( &EventTree->TextSpan, &EventTree->HandlesOrImplements );
        }
        else
        {
            const DECLFLAGS DECLF_InvalidOnImplementsFlags = DECLF_Shared;
            if ( EventDeclFlags & DECLF_InvalidOnImplementsFlags )
            {
                //Bug 314537 - Implementing with shared events is illegal.
                ReportDeclFlagError( ERRID_SharedOnProcThatImpl, EventDeclFlags & DECLF_InvalidOnImplementsFlags, EventTree->Specifiers );
            }

            BCSYM_ImplementsList *ImplementsList = NULL;
            ImplementsList = MakeImplementsList( EventTree->Implements, Event );
            EventDeclFlags |= DECLF_Implementing;
            Event->SetImplementsList( ImplementsList );
        }
    }

    VSASSERT(EventTree->GenericParameters == NULL, "Parser generated an event declaration with type parameters.");

    // Fill out the base class portion of the Event Declaration symbol that is sitting in Event
    // e.g. given <Event Access> Event Foo(<signature>) this builds <Event Access> Foo(<signature>)
    // The created proc has the same name as the event name, e.g. Bob if the declaration was Event bob().
    //
    // Note -- the parameters here may be NULL if its the "Dim x as delegate" syntax because we won't know
    // what the delegate is until bindable.

    BCSYM_Param *ParameterSymbolListForEventHookupSymbols = NULL;  // We need to get the list of parameters back from MakeProc
                                                                   // so that we can simply make a copy of that list and use it
                                                                   // for the delegate parameters. This make it possible to generate
                                                                   // one set of errors for the parameter,

                                                                   // should we decide to generate errors for them.
    MakeProc( EventTree->Opcode,
              &EventTree->Name,
              NULL, // proc's name found in EventTree->Name, above
              EventTree->Parameters,
              NULL, // proc has no return type
              NULL, // no return type attributes
              NULL, // proc has no lib
              NULL, // proc has no alias
              EventTree->GenericParameters,
              EventDeclFlags, // use the flags that were on the Event statement
              DefinedInInterface,
              (CodeBlockLocation*) NULL,
              (CodeBlockLocation*) NULL,
              Event, // the context
              OwningSymbolList, // attach event decl to this list
              NULL, // don't care about constructors
              NULL, // don't care about shared constructors
              EventTree->Specifiers,
              ContainerDeclFlags,
              NULL,           // no return type for an event.
              &ParameterSymbolListForEventHookupSymbols );       // Get the list of parameters back to use on the event hookup symbols

    if ( StatementListTree->Element->Opcode == ParseTree::Statement::BlockEventDeclaration )
    {
        Event->SetIsBlockEvent();
    }

    // Store away the custom attributes.
    ProcessAttributeList( EventTree->Attributes, Event );

    // Generate the delegate and Add,Remove functions, etc.
    GenerateEventHookupSymbols( StatementListTree,
                                EventDeclFlags,
                                DefinedInInterface,
                                Event,
                                OwningSymbolList,
                                DefinedInStdModule,
                                ParameterSymbolListForEventHookupSymbols ,
                                ContainerDeclFlags,
                                BackingStaticFieldsList,
                                NeedSharedConstructor );


    VSASSERT( !Event->IsBlockEvent() || !Event->GetDelegateVariable(), "Delegate variable unexpected for block events!!!" );

    VSASSERT( !Event->IsBlockEvent() ||
                ( ( !Event->GetProcAdd() || !Event->GetProcAdd()->IsSyntheticMethod() ) &&
                  ( !Event->GetProcRemove() || !Event->GetProcRemove()->IsSyntheticMethod() ) &&
                  ( !Event->GetProcFire() || !Event->GetProcFire()->IsSyntheticMethod() )),
                        "Synthetic event methods unexpected for block events!!!" );

    // If we build a event symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this event symbol.
    if ( Event )
    {
        ExtractXMLDocComments( StatementListTree, Event );
    }
}

/*****************************************************************************
;GenerateEventHookupSymbols

The following are created when processing Event Foo(<signature>):
  1. <Event access> Delegate Sub FooEventHandler(<signature>)
  2. PRIVATE FooEvent as FooEventHandler
  3. <Event access> sub add_Foo( ByVal obj as FooEventHandler )
  4. <Event access> sub remove_Foo( ByVal obj as FooEventHandler )
*****************************************************************************/
void Declared::GenerateEventHookupSymbols
(
    ParseTree::StatementList *StatementListTree,    // [in] the parse tree of the Event declaration, e.g. Event Foo(<signature>)
    DECLFLAGS EventDeclFlags,                       // [in] flags for the event declaration
    bool DefinedInInterface,                        // [in] are we defining in an interface?
    BCSYM_EventDecl *Event,                         // [in] the Event declaration symbol being built
    SymbolList *OwningSymbolList,                   // [in] symbol list to add to
    bool DefinedInStdModule,                        // [in] whether the event was defined in a std. module or not
    BCSYM_Param *ParameterSymbolList,               // [in] Params for the delegate methods (if any) to be used instead of DelegateTree parameters in order to avoid logging duplicate errors
    DECLFLAGS ContainerDeclFlags,                   // [in] the flags for the containing class
    SymbolList *BackingStaticFieldsList,            // [in] the symbol list to add backing fields to
    bool &NeedSharedConstructor                     // [out] whether a shared constructor is needed
)
{
#if DEBUG
    if ( VSFSWITCH( fDumpSyntheticCode ))
    {
        DebPrintf( "[Event Declaration for %S ]\n", Event->GetName() );
    }
#endif

    // 1. <EventAccess> <[Shadows]> Delegate Sub FooEventHandler(<signature>)
    //
    BuildEventDelegate( StatementListTree,
                        Event,
                        EventDeclFlags,
                        OwningSymbolList,
                        DefinedInStdModule,
                        ParameterSymbolList);


    // 2. PRIVATE [Shadows] FooEvent as FooEventHandler (i.e. the delegate built in 1)
    //  Only build this if the Event was NOT declared on an interface and only for
    //  non-block events
    //
    if ( !DefinedInInterface &&
         StatementListTree->Element->Opcode != ParseTree::Statement::BlockEventDeclaration )
    {
        // Synthesize an event field
        BuildEventField( Event, EventDeclFlags, OwningSymbolList );
    }


    // 3. <Event access> sub add_Foo( byval obj as FooEventHandler )
    // 4. <Event Access> sub remove_Foo( byval obj as FooEventHandler )
    // 5. <Event Access> sub fire_Foo( <Delegate Signature> )
    //
    BuildEventMethods( StatementListTree->Element,
                       Event,
                       EventDeclFlags,
                       GetUnBindableChildrenSymbolList(),   // Add, Remove and Fire cannot be bound to, so put them in unbindable list
                       DefinedInInterface,
                       ContainerDeclFlags,
                       BackingStaticFieldsList,
                       NeedSharedConstructor );

}

/*****************************************************************************
;BuildEventDelegate

Builds a delegate for an event, synthesizing one if required
*****************************************************************************/
void
Declared::BuildEventDelegate
(
    ParseTree::StatementList *EventStatementListTree,   // [in] the parsetree for the event
    BCSYM_EventDecl *Event,                             // [in] the owning event
    DECLFLAGS EventDeclFlags,                           // [in] the DECLFFLAGS of the event
    SymbolList *OwningSymbolList,                       // [in] symbol list to add to
    bool DefinedInStdModule,                            // [in] indicates whether the owning event is declared in a std module
    BCSYM_Param *DelegateParamSymbolList                // [in] Params for the delegate methods (if any) to be used instead of
                                                        //      DelegateTree parameters in order to avoid logging duplicate errors
)
{
    // <EventAccess> <[Shadows]> Delegate Sub FooEventHandler(<signature>)

    STRING *EventName = Event->GetName();

    ParseTree::EventDeclarationStatement *EventTree =
        EventStatementListTree->Element->Opcode == ParseTree::Statement::EventDeclaration ?
            EventStatementListTree->Element->AsEventDeclaration() :
            EventStatementListTree->Element->AsBlockEventDeclaration()->EventSignature;

    // For events implementing interface events, this may never get set if the host interface is bad (bug #307340)
    BCSYM *DelegateSymbol = Symbols::GetGenericBadNamedRoot();

    // Create the event delegate name "<EventName>EventHandler"
    STRING *DelegateName = m_CompilerInstance->ConcatStrings( EventName, WIDE( "EventHandler" ));


    // Build the delegate

    if ( EventTree->ReturnType )
    {
        DelegateSymbol = MakeType( EventTree->ReturnType, NULL, Event, GetTypeList() );
        Event->SetParametersObtainedFromDelegate(true);
    }
    else
    {
        VSASSERT( EventStatementListTree->Element->Opcode != ParseTree::Statement::BlockEventDeclaration,
                        "Block event without a delegate type unexpected!!!" );

        // Create the delegate - unless we're implementing in which case the delegate will be hooked up from the interface later.
        //
        if ( !EventTree->Implements )
        {

            if (m_SourceFile->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj)
            {
                ReportError(ERRID_WinRTEventWithoutDelegate, Event->GetLocation());
            }
            else
            {
                
                MakeDelegate( EventStatementListTree,
                              true,                                 // Delegate is nested inside another type
                              DefinedInStdModule,
                              OwningSymbolList,
                              DelegateParamSymbolList,
                              true,                                 // An Event Delegate
                              false,                                // UseAttributes
                              DelegateName,
                              (BCSYM_Class **)&DelegateSymbol );    // built delegate attatched to OwningSymbolList


                if ( DelegateSymbol )
                {
                    DelegateSymbol->PNamedRoot()->SetEventThatCreatedSymbol( Event );

                    // Set the hidden flag if we made the Delegate from the Event
                    DelegateSymbol->PNamedRoot()->SetIsHidden( true );

                    // Propagate badness
                    //
                    if ( Event->IsBad())
                    {
                        DelegateSymbol->PNamedRoot()->SetIsBad( m_SourceFile );
                    }

                    if ( !DelegateSymbol->IsBad() && DelegateSymbol->IsClass() )
                    {
                        // Replace a direct reference to the delegate class with a named
                        // type. This will allow for correct synthesis of type arguments
                        // if the event is within a generic class.

                        BCSYM_NamedType *DelegateType = m_SymbolAllocator.GetNamedType( &EventTree->TextSpan, Event, 1, GetTypeList() );
                        DelegateType->GetNameArray()[0] = DelegateSymbol->PClass()->GetName();

                        DelegateSymbol = DelegateType;
                    }
                }
            }
        }
    }

    // connect the delegate class defined for this Event with the Event Declaration symbol
    Event->SetDelegate( DelegateSymbol );

#if IDE
    Event->SetDeclaredDelegate(DelegateSymbol);
#endif 


#if DEBUG
    if ( VSFSWITCH( fDumpSyntheticCode ))
    {
        if ( DelegateSymbol &&  !DelegateSymbol->IsNamedType())
        {
            StringBuffer DelegateString;
            DelegateSymbol->GetBasicRep( m_CompilerInstance, DelegateSymbol->PClass()->GetContainingClass(), &DelegateString, NULL, DelegateSymbol->PNamedRoot()->GetName(), false );
            DebPrintf( "  [Synth Delegate] %S\n", DelegateString.GetString());
        }
        else
        {
            DebPrintf("  [Synth Delegate] - NONE\n" );
        }
    }
#endif DEBUG
}

/*****************************************************************************
;BuildEventField

Builds a synthetic backing field for an event
*****************************************************************************/
void
Declared::BuildEventField
(
    BCSYM_EventDecl *Event,         // [in] the owning event
    DECLFLAGS EventDeclFlags,       // [in] the DECLFFLAGS of the event
    SymbolList *OwningSymbolList    // [in] symbol list to add to
)
{
    // PRIVATE [Shadows] FooEvent as FooEventHandler

    bool EventHasLocation = Event->HasLocation();
    STRING *EventName = Event->GetName();
    BCSYM *DelegateSymbol = Event->GetRawDelegate();

    DECLFLAGS EventFieldFlags =
             ( ( EventDeclFlags &
                    ( DECLF_Shared | DECLF_ShadowsKeywordUsed )) |
               DECLF_Dim |
               DECLF_Private |
               DECLF_Hidden |
               DECLF_Bracketed);

    // Create the event field name "<EventName>Event"
    STRING *EventFieldName = m_CompilerInstance->ConcatStrings( EventName, WIDE( "Event" ));


    // Create the BCSYM_Variable corresponding to the event field

    BCSYM_Variable *EventFieldSymbol = m_SymbolAllocator.AllocVariable( EventHasLocation, false );

    m_SymbolAllocator.GetVariable( NULL,            // no location
                                   EventFieldName,  // Name
                                   EventFieldName,  // EmittedName
                                   EventFieldFlags ,// DECLFlags
                                   VAR_Member,      // Indicates that the variable is a type member as opposed to a local, etc.
                                   DelegateSymbol,  // the type of this variable is the delegate that the event is typed to
                                   NULL,            // no expression
                                   OwningSymbolList,
                                   EventFieldSymbol );

    Event->SetDelegateVariable( EventFieldSymbol );
    EventFieldSymbol->SetEventThatCreatedSymbol( Event );

    // Propagte the event location to the synthesized field in order to guarantee deterministic
    // ordering in the emitted assembly.
    //
    if ( EventHasLocation )
    {
        EventFieldSymbol->SetLocation( Event->GetLocation(),
                                       true );      // Location is inherited from a source symbol
    }

    // Propagate badness
    //
    if ( Event->IsBad())
    {
        EventFieldSymbol->SetIsBad( m_SourceFile );
    }


#if DEBUG
    if ( VSFSWITCH( fDumpSyntheticCode ))
    {
        if ( EventFieldSymbol->PMember()->GetRawType() )
        {
            DebPrintf( "  [Synth Delegate Var] Private Dim %S As %S\n",
                       EventFieldSymbol->GetName(),
                       EventFieldSymbol->PMember()->GetRawType()->GetErrorName(m_CompilerInstance));
        }
        else
        {
            DebPrintf( "  [Synth Delegate Var] Unknown\n");
        }
    }
#endif DEBUG
}


/*****************************************************************************
;BuildEventMethods

Builds the Add, Remove and optionally the Fire methods for an event,
synthesizing them if required
*****************************************************************************/
void
Declared::BuildEventMethods
(
    ParseTree::Statement *EventTree,                    // [in] the parsetree for the event
    BCSYM_EventDecl *Event,                             // [in] the owning event
    DECLFLAGS EventDeclFlags,                           // [in] the DECLFFLAGS of the event
    SymbolList *OwningSymbolList,                       // [in] symbol list to add to
    bool DefinedInInterface,                            // [in] indicates whether the owning event is declared in a std module
    DECLFLAGS ContainerDeclFlags,                       // [in] the flags for the containing class
    SymbolList *BackingStaticFieldsList,                // [in] the symbol list to add backing fields to
    bool &NeedSharedConstructor                         // [out] whether a shared constructor is needed
)
{
    // Event Hookups ( Add_Foo, Remove_Foo ) get the same access as the Event declaration, but Fire_Foo
    // gets Private
    //
    // Disallow byref by passing the DECLF_DisallowByref flag. Disallow Optional and Paramarray by not
    // passing the DECLF_AllowOptional flag.
    //
    DECLFLAGS EventMethodFlags = (EventDeclFlags & ( DECLF_AccessFlags | DECLF_ShadowsKeywordUsed )) |
                                 DECLF_Function |
                                 DECLF_SpecialName |
                                 DECLF_DisallowByref;

    if ( DefinedInInterface )
    {
        // The event hookup code methods should be virtual / abstract if we are building this for an event
        // declared on an interface
        EventMethodFlags |= DECLF_MustOverride;
    }
    else
    {
        // Fix for bug VisualStudio7 - 312488 - Event add/remove methods should not be virtual by default,
        // but should be virtual final when event is implementing another event

        EventMethodFlags |= ( EventDeclFlags & ( DECLF_Shared | DECLF_Implementing ));
    }

    if ( EventTree->Opcode == ParseTree::Statement::BlockEventDeclaration )
    {
        VSASSERT( !DefinedInInterface, "Block events in interfaces should have been rejected in the parser!!!");

        BuildUserDefinedEventMethods( EventTree->AsBlockEventDeclaration(),
                                      Event,
                                      EventMethodFlags,
                                      OwningSymbolList,
                                      ContainerDeclFlags,
                                      BackingStaticFieldsList,
                                      NeedSharedConstructor );
    }
    else
    {
        // Build the Add method
        BuildSyntheticEventMethod( SYNTH_AddEvent, Event, EventMethodFlags | DECLF_EventAddMethod, OwningSymbolList );

        // Build the Remove method
        BuildSyntheticEventMethod( SYNTH_RemoveEvent, Event, EventMethodFlags | DECLF_EventRemoveMethod, OwningSymbolList );

        // No synthetic Fire method needed
    }
}

/*****************************************************************************
;BuildSyntheticEventMethod

Builds the Add or Remove methods for an event based on the synthetic context
passed in
*****************************************************************************/
void
Declared::BuildSyntheticEventMethod
(
    SyntheticKind MethodKind,                           // [in] the synthetic method kind to indicate Add or Remove
    BCSYM_EventDecl *Event,                             // [in] the owning event
    DECLFLAGS EventMethodFlags,                         // [in] the flags to use for the event method to be built
    SymbolList *OwningSymbolList                        // [in] symbol list to add to
)
{
    STRING *EventName = Event->GetName();
    bool EventHasLocation = Event->HasLocation();
    BCSYM *DelegateSymbol = Event->GetRawDelegate();

    STRING *EventMethodName = NULL;
    STRING *ParamName = m_CompilerInstance->AddString( WIDE( "obj" ));
    BCSYM_Param *ParamSymbolList = NULL, *ParamSymbolListTail = NULL;

    if ( MethodKind == SYNTH_AddEvent )
    {
        // Create the name for the event Add method Add_<EventName>
        EventMethodName = m_CompilerInstance->ConcatStrings( ATTACH_LISTENER_PREFIX, EventName );
    }
    else
    {
        VSASSERT( MethodKind == SYNTH_RemoveEvent, "Unexpected event synthetic method kind!!!");

        // Create the name for the event Remove method Remove_<EventName>
        EventMethodName = m_CompilerInstance->ConcatStrings( REMOVE_LISTENER_PREFIX, EventName );
    }

    // Build the parameter "obj as Foo" for the event method
    //
    m_SymbolAllocator.GetParam( NULL,               // no location
                                ParamName,          // "obj"
                                DelegateSymbol,     // the type is the delegate foo created above
                                PARAMF_ByVal,
                                NULL,               // no expression
                                &ParamSymbolList,
                                &ParamSymbolListTail,
                                false);             // Not a return type param

     // allocate the synthetic method we will use to add event handlers
    BCSYM_SyntheticMethod *EventMethod = m_SymbolAllocator.AllocSyntheticMethod(EventHasLocation);

    // Fill in: <Event Decl Access> NotOverridable Hidden Add_Foo/Remove_Foo( byval obj as Foo )
    //
    m_SymbolAllocator.GetProc( NULL,                        // no location
                               EventMethodName,             // Add_Foo is the name of the proc
                               EventMethodName,             // Add_Foo for emitted name
                               (CodeBlockLocation*) NULL,
                               (CodeBlockLocation*) NULL,
                               EventMethodFlags,
                               NULL,                        // no type ( it's a sub )
                               ParamSymbolList,             // the "obj" parameter
                               NULL,
                               NULL,                        // no lib name
                               NULL,                        // no alias name
                               MethodKind,
                               NULL,                        // no generic params
                               OwningSymbolList,
                               EventMethod );

    if ( EventHasLocation )
    {
        EventMethod->SetLocation( Event->GetLocation(),
                                  true );                   // Location is inherited from a source symbol
    }

    // Propagate badness
    //
    if ( Event->IsBad())
    {
        EventMethod->SetIsBad( m_SourceFile );
    }

    // Connect the Add_Foo method to the Event Declaration

    if ( MethodKind == SYNTH_AddEvent )
    {
        Event->SetProcAdd( EventMethod );
    }
    else
    {
        VSASSERT( MethodKind == SYNTH_RemoveEvent, "Unexpected event synthetic method kind!!!");

        Event->SetProcRemove( EventMethod );
    }

    EventMethod->SetEventThatCreatedSymbol( Event );
}


/*****************************************************************************
;BuildUserDefinedEventMethods

Builds the Add, Remove and Fire methods for an event
*****************************************************************************/
void
Declared::BuildUserDefinedEventMethods
(
    ParseTree::BlockEventDeclarationStatement *EventTree,   // [in] the parsetree for the event
    BCSYM_EventDecl *Event,                                 // [in] the owning event
    DECLFLAGS EventMethodFlags,                             // [in] the DECLFFLAGS to use for the event method to be built
    SymbolList *OwningSymbolList,                           // [in] symbol list to add to
    DECLFLAGS ContainerDeclFlags,                           // [in] the flags for the container of the owning event
    SymbolList *BackingStaticFieldsList,                    // [in] the symbol list to add backing fields to
    bool &NeedSharedConstructor                             // [out] whether a shared constructor is needed
)
{
    // Build the Add/Remove/Fire members of the event block

    // Block events are disallowed in interfaces
    VSASSERT( !( EventMethodFlags & DECLF_MustOverride ), "Block event unexpected in an interface!!!");

    bool MarkEventBad = false;
    STRING *EventName = Event->GetName();

    BCSYM_Proc *AddMethod = NULL;
    BCSYM_Proc *RemoveMethod = NULL;
    BCSYM_Proc *FireMethod = NULL;

    for ( ParseTree::StatementList *CurrentEventMember = EventTree->Children;
          CurrentEventMember;
          CurrentEventMember = CurrentEventMember->NextInBlock )
    {
        if ( CurrentEventMember->Element->HasSyntaxError )
        {
            continue;  // don't process trees for horked guys
        }

        if ( CurrentEventMember->Element->Opcode == ParseTree::Statement::CommentBlock )
        {
            // XML doc comments are not allowed on Add, Remove and Fire of events, so we add them
            // to the list of seen comment blocks before we continue.
            //
            if ( CurrentEventMember->Element->AsCommentBlock()->IsXMLDocComment == true )
            {
                AddCommentBlock( CurrentEventMember->Element->AsCommentBlock() );
            }

            continue;
        }

        STRING *EventMethodName = NULL;
        DECLFLAGS CurrentEventMethodFlags = EventMethodFlags;
        BCSYM_Proc **CurrentEventMethod = NULL;

        switch ( CurrentEventMember->Element->Opcode )
        {
            case ParseTree::Statement::AddHandlerDeclaration:
            {
                if ( AddMethod )
                {
                    // 'AddHandler' is already declared
                    ReportError( ERRID_DuplicateAddHandlerDef, &CurrentEventMember->Element->TextSpan);

                    // don't build a second Add
                    continue;
                }

                MarkEventBad |= !ValidateEventAddRemoveMethodParams( CurrentEventMember->Element->AsMethodDefinition() );

                CurrentEventMethodFlags |= DECLF_EventAddMethod;
                CurrentEventMethod = &AddMethod;
                EventMethodName = m_CompilerInstance->ConcatStrings( ATTACH_LISTENER_PREFIX, EventName );
                break;
            }

            case ParseTree::Statement::RemoveHandlerDeclaration:
            {
                if ( RemoveMethod )
                {
                    // 'RemoveHandler' is already declared
                    ReportError( ERRID_DuplicateRemoveHandlerDef, &CurrentEventMember->Element->TextSpan);

                    // don't build a second Remove
                    continue;
                }

                MarkEventBad |= !ValidateEventAddRemoveMethodParams(CurrentEventMember->Element->AsMethodDefinition());

                CurrentEventMethodFlags |= DECLF_EventRemoveMethod;
                CurrentEventMethod = &RemoveMethod;
                EventMethodName = m_CompilerInstance->ConcatStrings( REMOVE_LISTENER_PREFIX, EventName );
                break;
            }

            case ParseTree::Statement::RaiseEventDeclaration:
            {
                if ( FireMethod )
                {
                    // 'RaiseEvent' is already declared
                    ReportError( ERRID_DuplicateRaiseEventDef, &CurrentEventMember->Element->TextSpan);

                    // don't build a second Fire
                    continue;
                }


                // - RaiseEvent should always have private access
                // - Even when the event is implementing another event, RaiseEvent does not implement any method
                // - RaiseEvent parameters can be byref as long as they match the parameters of the event's delegate.
                CurrentEventMethodFlags = ( ( CurrentEventMethodFlags &
                                                ~( DECLF_Implementing | DECLF_AccessFlags | DECLF_DisallowByref )) |
                                             DECLF_Private |
                                             DECLF_EventFireMethod);

                CurrentEventMethod = &FireMethod;
                EventMethodName = m_CompilerInstance->ConcatStrings( FIRE_LISTENER_PREFIX, EventName );
                break;
            }

            default:
                continue;
        }

        // Build the event method

        VSASSERT( EventMethodName && CurrentEventMethod, "Expected non-NULL event method !!!" );

        ParseTree::MethodDefinitionStatement *EventMethodTree = CurrentEventMember->Element->AsMethodDefinition();

        *CurrentEventMethod = m_SymbolAllocator.AllocMethodImpl( true );

        MakeProc( ParseTree::Statement::ProcedureDeclaration,
                  NULL,                                         // NULL parse tree for name, since name is explicitly passed in
                  EventMethodName,
                  EventMethodTree->Parameters,
                  NULL,                                         // NULL return type because event methods are subs
                  NULL,                                         // No attributes on return type
                  NULL,                                         // no lib
                  NULL,                                         // no alias
                  NULL,                                         // no generic params
                  CurrentEventMethodFlags,
                  false,                                        // No defined in interface - Block events cannot be defined in an interface
                  &EventMethodTree->BodyLocation,
                  &EventMethodTree->EntireMethodLocation,
                  *CurrentEventMethod,
                  OwningSymbolList,
                  NULL,                                         // don't care about constructors
                  NULL,                                         // don't care about shared constructors
                  NULL,                                         // no specifiers allowed on event methods
                  ContainerDeclFlags,
                  NULL,                                         // only valid for properties
                  NULL );                                       // no need to get back the list of parameters built

        (*CurrentEventMethod)->SetLocation( &EventMethodTree->TextSpan );

        // Connect the event method to its owning event
        (*CurrentEventMethod)->SetEventThatCreatedSymbol( Event );

        // Store away the custom attributes.
        ProcessAttributeList( EventMethodTree->Attributes, *CurrentEventMethod );

        // Creating backing fields for any static vars
        CreateBackingFieldsForStatics( EventMethodTree->StaticLocalDeclarations,
                                       *CurrentEventMethod,
                                       ContainerDeclFlags,
                                       NeedSharedConstructor,
                                       BackingStaticFieldsList );

        MarkEventBad |= (*CurrentEventMethod)->IsBad();

        // Propgate any already existing event declaration badness to the event method
        //
        if ( Event->IsBad() )
        {
            (*CurrentEventMethod)->SetIsBad( m_SourceFile );
        }
    }


    // Give errors in any of the Add, Remove or Fire methods are missing
    // and connect the event to the event methods if they do exist.

    if ( AddMethod )
    {
        Event->SetProcAdd( AddMethod );
    }
    else
    {
        // 'AddHandler' definition missing for event '|1'.

        ReportError( ERRID_MissingAddHandlerDef1, Event->GetLocation(), EventName );
        MarkEventBad = true;
    }

    if ( RemoveMethod )
    {
        Event->SetProcRemove( RemoveMethod );
    }
    else
    {
        // 'RemoveHandler' definition missing for event '|1'.

        ReportError( ERRID_MissingRemoveHandlerDef1, Event->GetLocation(), EventName );
        MarkEventBad = true;
    }

    if ( FireMethod )
    {
        Event->SetProcFire( FireMethod );
    }
    else
    {
        // 'RaiseEvent' definition missing for event '|1'.

        ReportError( ERRID_MissingRaiseEventDef1, Event->GetLocation(), EventName );
        MarkEventBad = true;
    }

    if ( MarkEventBad )
    {
        Event->SetIsBad( m_SourceFile );
    }
}

/*****************************************************************************
;ValidateEventAddRemoveMethodParams

Validates event add or remove method's params. Returns true if no errors have
been found, else return false.
*****************************************************************************/
bool
Declared::ValidateEventAddRemoveMethodParams
(
    ParseTree::MethodDefinitionStatement *EventMethodTree
)
{
    VSASSERT( EventMethodTree->Opcode == ParseTree::Statement::AddHandlerDeclaration ||
              EventMethodTree->Opcode == ParseTree::Statement::RemoveHandlerDeclaration,
                    "Unexpected event method to validate!!!");

    ParseTree::ParameterList *EventMethodParameter = EventMethodTree->Parameters;

    if ( !EventMethodParameter || EventMethodParameter->Next )
    {
        // 'AddHandler' or 'RemoveHandler' method must have exactly one parameter.

        ReportError( ERRID_EventAddRemoveHasOnlyOneParam,
                     EventMethodParameter ? &EventMethodParameter->TextSpan : &EventMethodTree->TextSpan );

        return false;
    }

    return true;
}

/*****************************************************************************
;MakeProperty

Builds a symbol for a property declaration
*****************************************************************************/
BCSYM_Property * // the createed property symbol
Declared::MakeProperty
(
    ParseTree::StatementList     *StatementListTree, // [in] the property tree
    DECLFLAGS                    ClassDeclFlags,    // [in] the flags for the class
    bool                         DefinedInInterface, // [in] property is defined in an interface
    SymbolList                   *OwningSymbolList, // [in] symbol list to add to
    bool                         &NeedSharedConstructor,  // [out] whether a shared constructor is needed
    SymbolList                   *BackingStaticFieldsList // [in] the symbol list to add backing fields to
)
{
    VSASSERT( StatementListTree &&
              StatementListTree->Element &&
              (StatementListTree->Element->Opcode == ParseTree::Statement::Property) ||
              (StatementListTree->Element->Opcode == ParseTree::Statement::AutoProperty),
              "Declared::MakeProperty only accepts a Property or an AutoProperty" );
    
    ParseTree::PropertyStatement *PropertyTree; 

    if (StatementListTree->Element->Opcode == ParseTree::Statement::Property )
    {
        PropertyTree = StatementListTree->Element->AsProperty();
    }
    else
    {
        PropertyTree = StatementListTree->Element->AsAutoProperty();
    }
    
    // If there is no name or the property is horked, there isn't much for us to do
    if ( PropertyTree->Name.IsBad || PropertyTree->HasSyntaxError )
    {
        return NULL;
    }

    // Property semantic checkings
    // Some of these checkings are replicated for the accessor's flags.
    // Any code change in this area might need to be replicated bellow.
    bool DefinedInStdModule = ClassDeclFlags & DECLF_Module; // property defined in a module?
    bool DefinedInStruct    = ClassDeclFlags & DECLF_Structure; // property defined in a structure?

    const DECLFLAGS ValidPropertyFlags = DECLF_AccessFlags | DECLF_NotOverridable | DECLF_Overridable | DECLF_MustOverride | DECLF_Shared | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed| DECLF_ReadOnly | DECLF_WriteOnly | DECLF_Default | DECLF_ShadowsKeywordUsed | DECLF_Iterator;
    Location *PropertyLocation = &PropertyTree->Name.TextSpan;
    DECLFLAGS PropertyFlags = MapSpecifierFlags( PropertyTree->Specifiers );
    DECLFLAGS InvalidNotInheritableFlagsMinusProtected = DECLF_InvalidFlagsInNotInheritableClass & ~( DECLF_Protected | DECLF_ProtectedFriend ); // Protected is dealt with specially below


    if ( PropertyFlags & ~ValidPropertyFlags )
    {
        ReportDeclFlagError( ERRID_BadPropertyFlags1, PropertyFlags & ~ValidPropertyFlags, PropertyTree->Specifiers );
        PropertyFlags &= ValidPropertyFlags;
    }

    // Can't be auto-property and iterator
    if (PropertyTree->Opcode == ParseTree::Statement::AutoProperty && (PropertyFlags & DECLF_Iterator)!=0)
    {
        ReportDeclFlagError( ERRID_BadAutoPropertyFlags1, DECLF_Iterator, PropertyTree->Specifiers );
        PropertyFlags &= ~DECLF_Async;
    }

    // Catch specifier errors on Structures / NotInheritable classes
    if ( ClassDeclFlags & DECLF_NotInheritable )
    {
        // Semantics for members declared in classes that are final, i.e. 'NotInheritable'

        if ( PropertyFlags & InvalidNotInheritableFlagsMinusProtected )
        {
            ERRID error = ( ClassDeclFlags & DECLF_Structure) ? ERRID_StructCantUseVarSpecifier1 : ERRID_BadFlagsInNotInheritableClass1;
            ReportDeclFlagError( error, PropertyFlags & InvalidNotInheritableFlagsMinusProtected, PropertyTree->Specifiers );
            PropertyFlags &= ~InvalidNotInheritableFlagsMinusProtected;
        }
    }

    // Can't be PRIVATE and NOTOVERRIDABLE
    if (( PropertyFlags & DECLF_NotOverridable ) && ( PropertyFlags & DECLF_Private ))
    {
        ReportDeclFlagComboError( DECLF_NotOverridable | DECLF_Private, PropertyTree->Specifiers );
    }
    // Can't be NotOverridable and not Overrides
    if (( PropertyFlags & DECLF_NotOverridable ) && !( PropertyFlags & DECLF_OverridesKeywordUsed))
    {
        ReportDeclFlagError( ERRID_NotOverridableRequiresOverrides, PropertyFlags & DECLF_NotOverridable, PropertyTree->Specifiers );
    }
    // Can't be OVERLOADS and SHADOWS
    if (( PropertyFlags & DECLF_ShadowsKeywordUsed ) && ( PropertyFlags & DECLF_OverloadsKeywordUsed ))
    {
        ReportDeclFlagComboError( DECLF_ShadowsKeywordUsed | DECLF_OverloadsKeywordUsed, PropertyTree->Specifiers );
        PropertyFlags &= ~DECLF_OverloadsKeywordUsed;
    }
    // Can't be ITERATOR and WRITEONLY
    if (( PropertyFlags & DECLF_Iterator ) && ( PropertyFlags & DECLF_WriteOnly ))
    {
        ReportDeclFlagComboError( DECLF_Iterator | DECLF_WriteOnly, PropertyTree->Specifiers, DECLF_Iterator );
        PropertyFlags &= ~DECLF_Iterator;
    }

    if ( DefinedInStruct )
    {
        if ( PropertyFlags & DECLF_InvalidFlagsInNotInheritableClass )
        {
            ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1, PropertyFlags & DECLF_InvalidFlagsInNotInheritableClass, PropertyTree->Specifiers );
            PropertyFlags &= DECLF_InvalidFlagsInNotInheritableClass;
        }
    }

    if ( DefinedInInterface && ( PropertyFlags & DECLF_InvalidInterfaceMemberFlags ))
    {
        ReportDeclFlagError( ERRID_BadInterfacePropertyFlags1, PropertyFlags & DECLF_InvalidInterfaceMemberFlags, PropertyTree->Specifiers );
        PropertyFlags &= ~DECLF_InvalidInterfaceMemberFlags;
    }

    // A shared property cannot be marked as overrides, notoverridable or mustoverride
    // Do this check before we set flags to shared for a module below so that we catch the case where they
    // explicitly mark the property as shared along with overrides
    if (( PropertyFlags & DECLF_Shared ) && ( PropertyFlags & DECLF_InvalidFlagsOnShared ))
    {
        ReportDeclFlagError( ERRID_BadFlagsOnSharedProperty1, PropertyFlags & DECLF_InvalidFlagsOnShared, PropertyTree->Specifiers );
        PropertyFlags &= ~DECLF_InvalidFlagsOnShared;
    }

    if ( PropertyTree->Implements || DefinedInInterface )
    { // Properties on Interfaces, or properties that implement properties, must be marked as Virtual (do after check for shared properties above)
        if ( DefinedInInterface )
        { // Properties on interfaces must also be marked as abstract
            PropertyFlags |= DECLF_MustOverride;
        }
    }

    // Properties default to public if no flags are set
    if ( !( PropertyFlags & DECLF_AccessFlags ))
    {
        PropertyFlags |= DECLF_Public;
    }

    if (( PropertyFlags & DECLF_Default ) && ( PropertyFlags & DECLF_InvalidFlagsOnDefault ))
    {
        ReportDeclFlagError( ERRID_BadFlagsWithDefault1, PropertyFlags & DECLF_InvalidFlagsOnDefault, PropertyTree->Specifiers );
    }

    // Property inside Std Module semantics
    // Do these after the checks above because we mask in 'Shared' and don't want to do semantics on it - #191039
    if ( DefinedInStdModule )
    {
        if ( PropertyFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_BadFlagsOnStdModuleProperty1, PropertyFlags & DECLF_InvalidStdModuleMemberFlags, PropertyTree->Specifiers );
            PropertyFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }

        // Standard modules have all of their members marked as being shared.
        PropertyFlags |= DECLF_Shared;
        if (PropertyFlags & DECLF_OverloadsKeywordUsed)
        {
            ReportDeclFlagError(ERRID_OverloadsModifierInModule, DECLF_OverloadsKeywordUsed, PropertyTree->Specifiers);
        }
    }


    // Get the signature of the Property block
    BCSYM_Property *PropertySymbol = m_SymbolAllocator.AllocProperty( PropertyLocation != NULL );
    BCSYM *PropertyReturnTypeSymbol = NULL;         // The same ReturnType symbol is used for both the Get and the Set
    BCSYM_Param *ParameterSymbolList = NULL;        // The list of parameters for the property. This list is first built
                                                    // when procesing the propert itself, and then we make two copies of
                                                    // it, one for the Get and other for the Set. the reason we do this
                                                    // is that we don't want to reprocess the same parameters more than
                                                    // once in bindable (gives repeating errors, more space, time, etc.)
                                                    // so once we create the list of params once, we make copies of it and use the copies
                                                    // for the Get and the Set. In the Get case, we don't need to modify
                                                    // that list, however, in the Set case, we append 'Value' to that list.

    // Builds the proc into the base class portion of the PropertySymbol
    MakeProc( ParseTree::Statement::FunctionDeclaration,
              &PropertyTree->Name,
              NULL, // don't need to send the name since we pass it from the tree above
              PropertyTree->Parameters,
              PropertyTree->PropertyType,
              PropertyTree->PropertyTypeAttributes,
              NULL, // no lib
              NULL, // no alias
              NULL, // no generic params
              PropertyFlags | DECLF_HasRetval | DECLF_Function | DECLF_DisallowByref | DECLF_AllowOptional,
              DefinedInInterface,
              NULL, // no body location
              NULL, // no method location
              PropertySymbol,
              NULL, // don't add the symbol to a symbol list
              NULL, // don't care about constructors
              NULL, // don't care about shared constructors
              PropertyTree->Specifiers,
              ClassDeclFlags,
              &PropertyReturnTypeSymbol,    // Get the type of the Property back
              &ParameterSymbolList );       // Get the list of parameters back

    DECLFLAGS ErrorCausingFlags = PropertyFlags & ~PropertySymbol->GetDeclFlags();

    // Build the Implements list and store it in the Property
    if ( PropertyTree->Implements )
    {
        // Implementing with module members is illegal.
        if ( ClassDeclFlags & DECLF_Module )
        {
            ReportModuleMemberCannotImplementError( &PropertyTree->TextSpan, &PropertyTree->Impl );
        }
        else
        {
            // Implementing with shared properties is illegal.
            const DECLFLAGS DECLF_InvalidOnImplementsFlags = DECLF_Shared;
            if ( PropertyFlags & DECLF_InvalidOnImplementsFlags )
            {
                ReportDeclFlagError( ERRID_SharedOnProcThatImpl, 
                    PropertyFlags & DECLF_InvalidOnImplementsFlags, 
                    PropertyTree->Specifiers );
                // I don't clear the Shared flag because then you get weird errors from semantics about needing an object reference to the non-shared member, etc.
            }

            BCSYM_ImplementsList *ImplList = MakeImplementsList( PropertyTree->Implements, 
                                                                 PropertySymbol );
            PropertyFlags |= DECLF_Implementing;
            PropertySymbol->SetImplementsList( ImplList );
        }
    }

    // Process an auto property
    if ( PropertyTree->Opcode == ParseTree::Statement::AutoProperty )
    {
            MakeAutoProperty( PropertyTree->AsAutoProperty(),
                              PropertySymbol,
                              PropertyLocation,
                              DefinedInStruct,
                              DefinedInStdModule,
                              NeedSharedConstructor,
                              PropertyFlags,
                              OwningSymbolList );
            
        }

    else
    {
        MakeExpandedProperty(PropertyTree, 
                             PropertySymbol,  
                             PropertyLocation,
                             DefinedInStruct, 
                             DefinedInInterface, 
                             DefinedInStdModule, 
                             PropertyFlags,
                             ErrorCausingFlags,
                             ClassDeclFlags,
                             PropertyReturnTypeSymbol,
                             ParameterSymbolList,
                             OwningSymbolList,
                             NeedSharedConstructor,
                             BackingStaticFieldsList );
    }
    // Store away the custom attributes.
    ProcessAttributeList( PropertyTree->Attributes, PropertySymbol );

    // If we build a property symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this property symbol.
    if ( PropertySymbol )
    {
        ExtractXMLDocComments( StatementListTree, PropertySymbol );
    }

    return PropertySymbol;
}

void Declared::MakeAutoProperty
(
    _In_ ParseTree::AutoPropertyStatement * pAutoPropertyTree,
    _Inout_ BCSYM_Property *pAutoPropertySymbol,
    _In_ Location * pAutoPropertyLocation,
    bool definedInStruct,
    bool definedInStdModule,
    bool &needSharedConstructor,
    DECLFLAGS autoPropertyFlags,
    _Inout_ SymbolList *pOwningSymbolList // symbol list to add to
)
{
    // Auto properties initialized in structures must be marked 'Shared'.
    if ( definedInStruct &&
         !( autoPropertyFlags & DECLF_Shared ) &&
         (pAutoPropertyTree->AsAutoProperty()->pAutoPropertyDeclaration != NULL ))
    {
        ReportError( ERRID_AutoPropertyInitializedInStructure, pAutoPropertyLocation);
    }

    if ((autoPropertyFlags & DECLF_Shared) &&
        pAutoPropertyTree->AsAutoProperty()->pAutoPropertyDeclaration)
    {
        needSharedConstructor = true;
    }

    if ( pAutoPropertyLocation )
    {
        VSASSERT( pAutoPropertySymbol->HasLocation(), "Expected AutoProperty to have location");
        pAutoPropertySymbol->SetLocation(pAutoPropertyLocation);
    }
                        
    pAutoPropertySymbol->SetIsAutoProperty(true);
                
    MakeBackingField(pAutoPropertySymbol, 
                     definedInStdModule, 
                     pOwningSymbolList, 
                     pAutoPropertyTree->pAutoPropertyDeclaration,
                     pAutoPropertyTree->PropertyTypeAttributes);                
        
}

//-------------------------------------------------------------------------------------------------
//
// MakeExpandedProperty
//
// Fills in the expanded property and its getter and setter.
//-------------------------------------------------------------------------------------------------
void Declared::MakeExpandedProperty
(
    _In_ ParseTree::PropertyStatement *pPropertyTree, // the property tree
    _Inout_ BCSYM_Property *pPropertySymbol, // Symbol for the expanded property
    _In_ Location* pPropertyLocation,
    bool definedInStruct, // is property defined in a struct?
    bool definedInInterface, // is the property defined in an interface?
    bool definedInStdModule, // is the property defined in a standard module?
    DECLFLAGS propertyFlags, // flags for the property
    DECLFLAGS ErrorCausingFlags, // flags that are causing errors
    DECLFLAGS classDeclFlags, // flags for the class
    _Inout_ BCSYM *pPropertyReturnTypeSymbol, // The same ReturnType symbol is used for both the Get and the Set
    _Inout_ BCSYM_Param *pParameterSymbolList, // The list of parameters for the property.
    _Inout_ SymbolList *pOwningSymbolList, // symbol list to add to
    bool &needSharedConstructor,  // whether a shared constructor is needed
    _Inout_ SymbolList *pBackingStaticFieldsList) // the symbol list to add backing fields to
{
    BCSYM_Proc *GetProperty = NULL, *SetProperty = NULL;
    
    if ( propertyFlags & DECLF_Default && !pPropertySymbol->GetRequiredParameterCount())
    {
        ReportError( ERRID_DefaultPropertyWithNoParams, &pPropertyTree->Name.TextSpan );
        pPropertySymbol->SetIsBad( m_SourceFile );
    }

    DECLFLAGS AccessorFlags = propertyFlags & ~DECLF_Default & ~ErrorCausingFlags;
    bool getHasAccessFlag = false;
    bool setHasAccessFlag = false;

    if ( !definedInInterface && !( propertyFlags & DECLF_MustOverride )) // Properties declared outside of interfaces and non abstract properties have bodies - build the MethodImpls
    {
        // Build the Get/Set members of the property block
        for ( ParseTree::StatementList *CurrentPropertyMember = pPropertyTree->Children;
            CurrentPropertyMember;
            CurrentPropertyMember = CurrentPropertyMember->NextInBlock )
        {
            if ( CurrentPropertyMember->Element->HasSyntaxError )
            {
                continue;  // don't process trees for horked guys
            }

            DECLFLAGS AccessorSpecificFlags = 0;
            DECLFLAGS AllowableAccessorFlags = DECLF_AccessFlags;

            if( CurrentPropertyMember->Element->Opcode == ParseTree::Statement::PropertyGet ||
                CurrentPropertyMember->Element->Opcode == ParseTree::Statement::PropertySet )
            {
                ParseTree::MethodDefinitionStatement *AccessorTree = 
                    CurrentPropertyMember->Element->AsMethodDefinition();
                
                AccessorSpecificFlags = MapSpecifierFlags( AccessorTree->Specifiers );

                // Check that we only have allowable flags.
                if ( AccessorSpecificFlags & ~AllowableAccessorFlags )
                {
                    ReportDeclFlagError( ERRID_BadPropertyAccessorFlags, 
                        AccessorSpecificFlags & ~AllowableAccessorFlags, 
                        AccessorTree->Specifiers );

                    AccessorSpecificFlags &= AllowableAccessorFlags;
                }

                ACCESS AccessOfPropertyAccessor = Symbols::AccessOfFlags(AccessorSpecificFlags);
                ACCESS AccessOfProperty = Symbols::AccessOfFlags(AccessorFlags);

                // Note that the following check is actually wrong in the case
                // that neither the property nor the accessor have any explicit
                // access modifiers. In this case, both will have public-level
                // access, and we will call ReportDeclFlagError even though
                // there is no actual error.
                // Needless to say, this is ----, but it works in the sense
                // that no actual error is produced. ReportDeclFlagError will
                // report one error per flag passed in, and since there are no
                // access modifier flags in this particular case, the method
                // does nothing.
                if (AccessOfProperty <= AccessOfPropertyAccessor ||
#pragma prefast(suppress: 26010 26011, "If access is outside of the array limits, then the whole symbol is busted")
                    MapAccessToAccessOutsideAssembly[AccessOfProperty] < MapAccessToAccessOutsideAssembly[AccessOfPropertyAccessor])
                {
                    ReportDeclFlagError( ERRID_BadPropertyAccessorFlagsRestrict,
                        AccessorSpecificFlags & DECLF_AccessFlags, AccessorTree->Specifiers );

                    AccessorSpecificFlags &= ~DECLF_AccessFlags;
                }

                // some of the general property semantics must be redone here as the accessor flags can restrict the access.
                // Any change in this code is suceptible to be replicated above

                // Again: catch specifier errors on Structures / NotInheritable classes
                if ( classDeclFlags & DECLF_NotInheritable )
                {
                    // Semantics for members declared in classes that are final, i.e. 'NotInheritable'
                    DECLFLAGS InvalidNotInheritableFlagsMinusProtected = 
                        DECLF_InvalidFlagsInNotInheritableClass & 
                        ~( DECLF_Protected | DECLF_ProtectedFriend ); // Protected is dealt with specially
                        
                    if ( AccessorSpecificFlags & InvalidNotInheritableFlagsMinusProtected )
                    {
                        ERRID error = ( classDeclFlags & DECLF_Structure) ? 
                            ERRID_StructCantUseVarSpecifier1 : ERRID_BadFlagsInNotInheritableClass1;
                        
                        ReportDeclFlagError( error,
                            AccessorSpecificFlags & InvalidNotInheritableFlagsMinusProtected,
                            AccessorTree->Specifiers );
                        AccessorSpecificFlags &= ~InvalidNotInheritableFlagsMinusProtected;
                    }
                }

                // Can't be PRIVATE accessor in a NOTOVERRIDABLE property
                if (( propertyFlags & DECLF_NotOverridable ) && ( AccessorSpecificFlags & DECLF_Private ))
                {
                    ReportDeclFlagError( ERRID_BadPropertyAccessorFlags1,
                        DECLF_Private,
                        AccessorTree->Specifiers );
                    AccessorSpecificFlags &= ~DECLF_Private;
                }

                if ( definedInStruct )
                {
                    if ( AccessorSpecificFlags & DECLF_InvalidFlagsInNotInheritableClass )
                    {
                        ReportDeclFlagError( ERRID_StructCantUseVarSpecifier1,
                            AccessorSpecificFlags & DECLF_InvalidFlagsInNotInheritableClass,
                            AccessorTree->Specifiers );
                        AccessorSpecificFlags &= ~DECLF_InvalidFlagsInNotInheritableClass;
                    }
                }

                if (( propertyFlags & DECLF_Default ) && ( AccessorSpecificFlags & DECLF_InvalidFlagsOnDefault ))
                {

                    ReportDeclFlagError( ERRID_BadPropertyAccessorFlags2,
                        AccessorSpecificFlags & DECLF_InvalidFlagsOnDefault,
                        AccessorTree->Specifiers );
                    AccessorSpecificFlags &= ~DECLF_InvalidFlagsOnDefault;
                }

                // Property inside Std Module semantics
                if ( definedInStdModule )
                {
                    if ( AccessorSpecificFlags & DECLF_InvalidStdModuleMemberFlags )
                    {
                        ReportDeclFlagError( ERRID_BadFlagsOnStdModuleProperty1,
                            AccessorSpecificFlags & DECLF_InvalidStdModuleMemberFlags,
                            AccessorTree->Specifiers );
                        AccessorSpecificFlags &= ~DECLF_InvalidStdModuleMemberFlags;
                    }
                }
                // End of semantic check duplicate

            }
            else
            {
                // XML doc comments are not allowed on Get and Set members of properties, so we add them
                // to the list of seen comment blocks before we continue.
                if ( CurrentPropertyMember->Element->Opcode == ParseTree::Statement::CommentBlock &&
                     CurrentPropertyMember->Element->AsCommentBlock()->IsXMLDocComment == true )
                {
                    AddCommentBlock( CurrentPropertyMember->Element->AsCommentBlock() );
                }

                continue;
            }

            switch ( CurrentPropertyMember->Element->Opcode )
            {
                case ParseTree::Statement::PropertyGet:
                {

                    if ( GetProperty )
                    {
                        ReportError( ERRID_DuplicatePropertyGet, &CurrentPropertyMember->Element->TextSpan);

                        // don't build a second getter
                        continue;
                    }
                    if((AccessorSpecificFlags & DECLF_AccessFlags) != 0 && setHasAccessFlag )
                    {
                        ReportError( ERRID_OnlyOneAccessorForGetSet, &CurrentPropertyMember->Element->TextSpan);
                        AccessorSpecificFlags = 0;
                    }
                    if((propertyFlags & DECLF_ReadOnly) && (AccessorSpecificFlags & DECLF_AccessFlags) != 0)
                    {
                        ReportError( ERRID_ReadOnlyOnlyNoAccesorFlag, &CurrentPropertyMember->Element->TextSpan);
                        AccessorSpecificFlags = 0;
                    }

                    DECLFLAGS GetAccessorFlags = 0;
                    if(AccessorSpecificFlags != 0)
                    {
                        // replace the access flag with the specific one
                        VSASSERT( (AccessorSpecificFlags & ~AllowableAccessorFlags) == 0, "Why is there an invalid flag here");
                        GetAccessorFlags = (AccessorFlags & ~DECLF_AccessFlags) | AccessorSpecificFlags;
                        getHasAccessFlag = true;
                    }
                    else
                    {
                        GetAccessorFlags = AccessorFlags;
                    }

                    GetProperty = m_SymbolAllocator.AllocMethodImpl( true ); // Set the context to be the property get method we are building

                    // Make a copy of the property params, and pass it to MakeProc for the Property Get() to use instead of building a whole
                    // new list of parameters for the Get. This avoids the generation of multiple errors when the params go through bindable since
                    // the user just typed them once in the property declaration anyway.
                    BCSYM_Param *ParameterSymbolListForGet = CloneParameters(pParameterSymbolList, NULL);

                    MakeProc( ParseTree::Statement::FunctionDeclaration,
                              &pPropertyTree->Name,
                              NULL, // don't need to send the name since it is in the tree arg above
                              NULL, // params already built for the Get
                              pPropertyTree->PropertyType,
                              pPropertyTree->PropertyTypeAttributes,
                              NULL, // no lib
                              NULL, // no alias
                              NULL, // no generic params
                              GetAccessorFlags | DECLF_HasRetval | DECLF_PropGet | DECLF_AllowOptional | DECLF_SpecialName | (pPropertySymbol->IsIteratorKeywordUsed() ? DECLF_Iterator : 0),
                              definedInInterface,
                              &CurrentPropertyMember->Element->AsMethodDefinition()->BodyLocation,
                              &CurrentPropertyMember->Element->AsMethodDefinition()->EntireMethodLocation,
                              GetProperty,
                              GetUnBindableChildrenSymbolList(),
                              NULL, // don't care about constructors
                              NULL, // don't care about shared constructors
                              pPropertyTree->Specifiers,
                              classDeclFlags,
                              &pPropertyReturnTypeSymbol,       // Use the same NamedType symbol that was created for the property
                              &ParameterSymbolListForGet );    // Use the list of parameters already built

                    GetProperty->SetLocation( &CurrentPropertyMember->Element->TextSpan );

                    // Store away the custom attributes.
                    ProcessAttributeList( CurrentPropertyMember->Element->AsMethodDefinition()->Attributes, GetProperty ); // Microsoft: Consider - you can't have attributes on the GET so why do this?

                    // Creating backing fields for any static vars
                    CreateBackingFieldsForStatics( CurrentPropertyMember->Element->AsMethodDefinition()->StaticLocalDeclarations,
                                                   GetProperty, classDeclFlags, needSharedConstructor, pBackingStaticFieldsList );
                    continue;
                }
                case ParseTree::Statement::PropertySet:
                {
                    if ( SetProperty )
                    {
                        ReportError( ERRID_DuplicatePropertySet, &CurrentPropertyMember->Element->TextSpan);

                        // don't build a second setter
                        continue;
                    }

                    if(AccessorSpecificFlags != 0 && getHasAccessFlag )
                    {
                        ReportError( ERRID_OnlyOneAccessorForGetSet, &CurrentPropertyMember->Element->TextSpan);
                        AccessorSpecificFlags = 0;
                    }
                    if((propertyFlags & DECLF_WriteOnly) && AccessorSpecificFlags != 0)
                    {
                        ReportError( ERRID_WriteOnlyOnlyNoAccesorFlag, &CurrentPropertyMember->Element->TextSpan);
                        AccessorSpecificFlags = 0;
                    }


                    DECLFLAGS SetAccessorFlags = 0;
                    if(AccessorSpecificFlags != 0)
                    {
                        // replace the access flag with the specific one
                        VSASSERT( (AccessorSpecificFlags & ~DECLF_AccessFlags) == 0, " why a non access flag is here");
                        SetAccessorFlags = (AccessorFlags & ~DECLF_AccessFlags) | AccessorSpecificFlags;
                        setHasAccessFlag = true;
                    }
                    else
                    {
                        SetAccessorFlags = AccessorFlags;
                    }

                    SetProperty = m_SymbolAllocator.AllocMethodImpl( true ); // Set the context to be the property set we are building

                    // Make a copy of the property params, and pass it to MakeProc for the Property Set() to use instead of building a whole
                    // new list of parameters for the Get. This avoids the generation of multiple errors when the params go through bindable since
                    // the user just typed them once in the property declaration anyway.
                    BCSYM_Param *ParameterSymbolListForSet = CloneParameters(pParameterSymbolList, NULL);
                    BuildPropertySet( pPropertyTree, CurrentPropertyMember, SetAccessorFlags,
                                      definedInInterface, SetProperty, classDeclFlags, pOwningSymbolList,
                                      &pPropertyReturnTypeSymbol, &ParameterSymbolListForSet );

                    // Store away the custom attributes.
                    ProcessAttributeList( CurrentPropertyMember->Element->AsMethodDefinition()->Attributes, SetProperty ); // Microsoft: Consider - you can't have attributes on the SET so why do this?

                    // Creating backing fields for any static vars
                    CreateBackingFieldsForStatics( CurrentPropertyMember->Element->AsMethodDefinition()->StaticLocalDeclarations, SetProperty, classDeclFlags, needSharedConstructor, pBackingStaticFieldsList );
                    continue;
                }
                default:
                    VSFAIL("unreachable code");
                    continue; // skip the EndGet/EndSet, comments, etc.
            } // switch
        }   // for
    } // !DefinedIninterface
    else // Properties declared in interfaces have implicit getters/setters - build them here so we will have something to mark as 'implemented' when the property is implemented
    {
        if ( !( propertyFlags & DECLF_WriteOnly )) // 'Manufacture' a Get
        {
            GetProperty = m_SymbolAllocator.AllocMethodDecl( true ); // Set the context to be the property get method we are building

            // Make a copy of the property params, and pass it to MakeProc for the Property Get() to use instead of building a whole
            // new list of parameters for the Get. This avoids the generation of multiple errors when the params go through bindable since
            // the user just typed them once in the property declaration anyway.
            BCSYM_Param *ParameterSymbolListForGet = CloneParameters(pParameterSymbolList, NULL);

            MakeProc( ParseTree::Statement::FunctionDeclaration,
                      &pPropertyTree->Name,
                      NULL, // don't need to send the name since it is in the tree arg above
                      NULL, // params already built for the Get
                      pPropertyTree->PropertyType,
                      pPropertyTree->PropertyTypeAttributes,
                      NULL, // no lib
                      NULL, // no alias
                      NULL, // no generic params
                      AccessorFlags | DECLF_HasRetval | DECLF_PropGet | DECLF_AllowOptional | DECLF_SpecialName,
                      definedInInterface,
                      NULL, // no method body
                      NULL, // no method location
                      GetProperty,
                      GetUnBindableChildrenSymbolList(),
                      NULL, // don't care about constructors
                      NULL, // don't care about shared constructors
                      pPropertyTree->Specifiers,
                      classDeclFlags,
                      &pPropertyReturnTypeSymbol,       // Use the same NamedType symbol that was created for the property
                      &ParameterSymbolListForGet );    // Use the list of parameters already built

            // The get doesn't technically have a location, but we need one in case we've
            // got overloads and we need to do them in declaration order for COM interop (VS 550459)
            GetProperty->SetLocation( &pPropertyTree->TextSpan, TRUE /* Location is Inherited */  );

        }

        if ( !( propertyFlags & DECLF_ReadOnly )) // 'Manufacture' a Set
        {
            SetProperty = m_SymbolAllocator.AllocMethodDecl( true ); // Set the context to be the property set we are building

            // Make a copy of the property params, and pass it to MakeProc for the Property Set() to use instead of building a whole
            // new list of parameters for the Get. This avoids the generation of multiple errors when the params go through bindable since
            // the user just typed them once in the property declaration anyway.
            BCSYM_Param *ParameterSymbolListForSet = CloneParameters(pParameterSymbolList, NULL);
            BuildPropertySet( pPropertyTree, NULL, AccessorFlags, definedInInterface, SetProperty,
                              classDeclFlags, pOwningSymbolList, &pPropertyReturnTypeSymbol, &ParameterSymbolListForSet );

            if ( GetProperty )
            {
                Symbols::SetOverloads( GetProperty );
                Symbols::SetOverloads( SetProperty );
            }
        }
    }

    // Build the property symbol
    m_SymbolAllocator.GetProperty( pPropertyLocation,
                                   pPropertyTree->Name.Name,
                                   propertyFlags,
                                   GetProperty,
                                   SetProperty,
                                   pPropertySymbol,
                                   pOwningSymbolList ); // add this property to the list of symbols

    // If a Property isn't explicitly marked as ReadOnly or WriteOnly...
    if ( !( propertyFlags & ( DECLF_ReadOnly | DECLF_WriteOnly )))
    {
         // then it must provide both a getter and setter
        if ( !GetProperty || !SetProperty )
        {
            ReportError( ERRID_PropMustHaveGetSet, &pPropertyTree->Name.TextSpan );
            pPropertySymbol->SetIsBad( m_SourceFile );
        }
    }
    else if (( propertyFlags & DECLF_WriteOnly ))  // WriteOnly property semantics
    {
        if ( GetProperty != NULL ) // If a property is marked WriteOnly, it can't have a Get
        {
            ReportError( ERRID_WriteOnlyHasGet, GetProperty->GetLocation());
            pPropertySymbol->SetIsBad( m_SourceFile );
        }
        if ( SetProperty == NULL ) // A WriteOnly property must have a set, though...
        {
            ReportError( ERRID_WriteOnlyHasNoWrite, &pPropertyTree->Name.TextSpan );
            pPropertySymbol->SetIsBad( m_SourceFile );
        }
    }
    else if (( propertyFlags & DECLF_ReadOnly ))  // ReadOnly property semantics
    {
        if ( SetProperty != NULL ) // If a property is marked ReadOnly, it can't have a Set
        {
            ReportError( ERRID_ReadOnlyHasSet, SetProperty->GetLocation());
            pPropertySymbol->SetIsBad( m_SourceFile );
        }
        if ( GetProperty == NULL ) // A ReadOnly property must have a get, though...
        {
            ReportError( ERRID_ReadOnlyHasNoGet, &pPropertyTree->Name.TextSpan);
            pPropertySymbol->SetIsBad( m_SourceFile );
        }
    }
}

/*****************************************************************************
;BuildPropertySet

Create a Property Set method symbol.
*****************************************************************************/
void Declared::BuildPropertySet(
    ParseTree::PropertyStatement *PropertyTree, // [in] the tree for the property declaration
    ParseTree::StatementList *CurrentPropertyMember, // [in] the method impl information for the current get/set being worked on ( may be NULL )
    DECLFLAGS PropertyFlags,    // [in] the declaration flags for the property
    bool DefinedInInterface,    // [in] are we building in an interface?
    BCSYM_Proc *PropertySet,    // [in] the MethodImpl that will be
    DECLFLAGS ContainerFlags,   // [in] the flags that are set on the container of this property
    SymbolList *OwningSymbolList,        // [in] symbol list to add to
    BCSYM **PropertyReturnTypeSymbol,    // [in] the return type symbol for implicit param of the set property (same as the type of the property itself).
    BCSYM_Param **ParameterSymbolList    // [in] Params for the  property Set we are building. This list should have already been built when
                                         // building the params list for the property, and is used here to append 'Value' to the end of this list
)
{
    VSASSERT( PropertySet->IsMethodDecl() || PropertySet->IsMethodImpl(), "BuildPropertySet assumes that the context was already allocated to hold the set" );
    VSASSERT( PropertyReturnTypeSymbol && *PropertyReturnTypeSymbol, "Bad Property return type symbol" );

    // Cobble up a parse tree for the synthetically generated trailing argument to the set - It is: byval Value as (whatever the property type is)

    // The parameter
    ParseTree::Parameter *HackedParm;

    if ( CurrentPropertyMember && CurrentPropertyMember->Element->AsMethodDefinition()->Parameters )
    {
        ParseTree::ParameterList *CurrentParameter = CurrentPropertyMember->Element->AsMethodDefinition()->Parameters;

        HackedParm = CurrentParameter->Element;

        if ( ParseTree::ParameterSpecifierList *BadSpecifierNode =
                HackedParm->Specifiers->HasSpecifier(
                    ParseTree::ParameterSpecifier::ParamArray |
                    ParseTree::ParameterSpecifier::ByRef |
                    ParseTree::ParameterSpecifier::Optional ))
        {
            WCHAR *BadSpecifier = NULL;

            if ( BadSpecifierNode->Element->Opcode == ParseTree::ParameterSpecifier::ParamArray )
            {
                BadSpecifier = m_CompilerInstance->TokenToString( tkPARAMARRAY );
            }
            else if ( BadSpecifierNode->Element->Opcode == ParseTree::ParameterSpecifier::ByRef )
            {
                BadSpecifier = m_CompilerInstance->TokenToString( tkBYREF );
            }
            else
            {
                VSASSERT( BadSpecifierNode->Element->Opcode == ParseTree::ParameterSpecifier::Optional,
                                "How can it be anything other than Optional ?");

                BadSpecifier = m_CompilerInstance->TokenToString( tkOPTIONAL );
            }

            ReportError( ERRID_SetHasToBeByVal1, &BadSpecifierNode->Element->TextSpan, BadSpecifier );
        }

        if ( CurrentParameter->Next )
        {
            ReportError( ERRID_SetHasOnlyOneParam, &CurrentPropertyMember->Element->AsMethodDefinition()->Parameters->TextSpan );
        }
    }
    else
    {
        HackedParm = ( ParseTree::Parameter *) m_ScratchAllocator.Alloc( sizeof( ParseTree::Parameter ));

            // The Param Declarator
        ParseTree::Declarator *HackedDeclarator = (ParseTree::Declarator *) m_ScratchAllocator.Alloc( sizeof( ParseTree::Declarator ));
        HackedDeclarator->Name.Name = STRING_CONST( m_CompilerInstance, Value );
        HackedDeclarator->Name.TypeCharacter = chType_NONE;

            // The parameter specifier
        ParseTree::ParameterSpecifier *HackedSpecifier = (ParseTree::ParameterSpecifier *) m_ScratchAllocator.Alloc( sizeof( ParseTree::ParameterSpecifier ));
        HackedSpecifier->Opcode = ParseTree::ParameterSpecifier::ByVal;

            // The parameter specifier list
        ParseTree::ParameterSpecifierList *HackedSpecifierList = (ParseTree::ParameterSpecifierList *) m_ScratchAllocator.Alloc( sizeof( ParseTree::ParameterSpecifierList ));
        HackedSpecifierList->Element = HackedSpecifier;

            // Fill out the parameter
        HackedParm->Name = HackedDeclarator;
        HackedParm->Type = PropertyTree->PropertyType;
        HackedParm->Specifiers = HackedSpecifierList;
    }

        // The parameter list
    ParseTree::ParameterList *HackedList = (ParseTree::ParameterList *) m_ScratchAllocator.Alloc( sizeof( ParseTree::ParameterList ));
    HackedList->Element = HackedParm;

    MakeProc( ParseTree::Statement::FunctionDeclaration,
              &PropertyTree->Name,
              NULL, // don't need to send the name since it is in the tree arg above
              HackedList,
              NULL, // no property type since it doesn't return anything
              NULL, // no return type attributes
              NULL, // no lib
              NULL, // no alias
              NULL, // no generic params
              PropertyFlags | DECLF_PropSet | DECLF_AllowOptional | DECLF_SpecialName,
              DefinedInInterface,
              CurrentPropertyMember ? &CurrentPropertyMember->Element->AsMethodDefinition()->BodyLocation : NULL,
              CurrentPropertyMember ? &CurrentPropertyMember->Element->AsMethodDefinition()->EntireMethodLocation : NULL,
              PropertySet, // context information for the property set
              GetUnBindableChildrenSymbolList(),
              NULL, // don't care about constructors
              NULL, // don't care about shared constructors
              PropertyTree->Specifiers,
              ContainerFlags,
              PropertyReturnTypeSymbol,    // Use the same NamedType symbol that was created for the property
              ParameterSymbolList );       // Use the parameter list already built when processing the property


    if ( CurrentPropertyMember )
    {
        PropertySet->SetLocation( &CurrentPropertyMember->Element->TextSpan );
    }
    else
    {
        // The set doesn't technically have a location, but we need one in case we've
        // got overloads and we need to do them in declaration order for COM interop (VS 550459)
        PropertySet->SetLocation( &PropertyTree->TextSpan, TRUE /* Location is Inherited */ );
    }
}

/*****************************************************************************
 ;MakeDllDeclared

Build a DLLDeclare symbol
*****************************************************************************/
void Declared::MakeDllDeclared
(
    ParseTree::StatementList *StatementListTree, // [in] the dlldeclare tree
    DECLFLAGS ClassFlags, // [in] the flags on the class that owns this dll decl symbol
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    ParseTree::ForeignMethodDeclarationStatement *DllDeclareTree = StatementListTree->Element->AsForeignMethodDeclaration();
    DECLFLAGS DllDeclareFlags = DECLF_AllowOptional | DECLF_AllowDupParam | MapDeclareDeclFlags( DllDeclareTree );
    Location * DeclareLocationInfo = &DllDeclareTree->TextSpan;
    bool DefinedInStdModule = ClassFlags & DECLF_Module; // property defined in a module?

    //
    // Change the context to be the DllDeclare symbol
    //

    BCSYM_DllDeclare *DllDeclare = m_SymbolAllocator.AllocDllDeclare( true ); // the new context is the DLLDeclare symbol  Microsoft: move to after the check for bad below.

    // If we don't have a name, there isn't much we can do.
    if ( DllDeclareTree->Name.IsBad ) return;

    //
    // Check the flags.
    //

    if ( DllDeclareFlags & ~DECLF_ValidDeclareFlags )
    {
        ReportDeclFlagError( ERRID_BadDeclareFlags1, DllDeclareFlags & ~DECLF_ValidDeclareFlags, DllDeclareTree->Specifiers );
        DllDeclareFlags &= DECLF_ValidDeclareFlags;
    }

    if ( DefinedInStdModule )
    {
        if ( DllDeclareFlags & DECLF_InvalidStdModuleMemberFlags )
        {
            ReportDeclFlagError( ERRID_ModuleCantUseDLLDeclareSpecifier1, DllDeclareFlags & DECLF_InvalidStdModuleMemberFlags, DllDeclareTree->Specifiers );
            DllDeclareFlags &= ~DECLF_InvalidStdModuleMemberFlags;
        }

        if (DllDeclareFlags & DECLF_OverloadsKeywordUsed)
        {
            ReportDeclFlagError(ERRID_OverloadsModifierInModule, DECLF_OverloadsKeywordUsed, DllDeclareTree->Specifiers);
        }
    }

    if ( ClassFlags & DECLF_Structure )
    {
        if ( DllDeclareFlags & ( DECLF_Protected | DECLF_ProtectedFriend ))
        {
            ReportDeclFlagError( ERRID_StructCantUseDLLDeclareSpecifier1, DllDeclareFlags & (DECLF_Protected | DECLF_ProtectedFriend), DllDeclareTree->Specifiers );
            DllDeclareFlags &= ~( DECLF_Protected | DECLF_ProtectedFriend );
        }
    }

    //
    // UV Support:
    // When linking against Starlite, Declare keyword shouldn't support Auto, Ansi or Unicode modifiers
    //
    if ( m_CompilerHost->IsStarliteHost() &&
          ( DllDeclareFlags & (DECLF_Ansi | DECLF_Auto | DECLF_Unicode )))
    {
        ReportDeclFlagError( ERRID_StarliteBadDeclareFlags, DllDeclareFlags & (DECLF_Ansi | DECLF_Auto | DECLF_Unicode), DllDeclareTree->Specifiers, DeclareLocationInfo );
        DllDeclareFlags &= ~(DECLF_Ansi | DECLF_Auto | DECLF_Unicode);
    }

    if (( DllDeclareFlags & DECLF_ShadowsKeywordUsed ) && ( DllDeclareFlags & DECLF_OverloadsKeywordUsed ))
    {
        ReportDeclFlagComboError( DECLF_ShadowsKeywordUsed | DECLF_OverloadsKeywordUsed, DllDeclareTree->Specifiers );
        DllDeclareFlags &= ~DECLF_OverloadsKeywordUsed;
    }

    // If no access flags are set, set to a default.
    if ( !( DllDeclareFlags & DECLF_AccessFlags ))
    {
        DllDeclareFlags |= DECLF_Public;
    }

    // Declares are always static.
    DllDeclareFlags |= DECLF_Shared;

    //
    // Create the symbol.
    //

    MakeProc( DllDeclareTree->Opcode,
              &DllDeclareTree->Name,
              NULL, // we don't need to send the string version of the name since it is contained in the DllDeclareTree->Name
              DllDeclareTree->Parameters,
              DllDeclareTree->ReturnType,
              DllDeclareTree->ReturnTypeAttributes,
              DllDeclareTree->Library,
              DllDeclareTree->Alias,
              NULL, // no generic params
              DllDeclareFlags, // the declaration flags
              false, // can't be in an interface
              ( CodeBlockLocation* ) NULL,
              ( CodeBlockLocation* ) NULL,
              DllDeclare,
              OwningSymbolList, // attatch proc to this list
              NULL, // don't care about constructors
              NULL, // don't care about shared constructors
              DllDeclareTree->Specifiers,
              ClassFlags,
              NULL,           // Not a property
              NULL );         // No parameter list

    // Store away the custom attributes.
    ProcessAttributeList( DllDeclareTree->Attributes, DllDeclare );

    // If we build a proc symbol, go ahead and process any XMLDoc comments that may be need to be attached
    // to this proc symbol.
    if ( DllDeclare )
    {
        ExtractXMLDocComments( StatementListTree, DllDeclare );
    }
}

/*****************************************************************************
;MakeProc

Build a procedure symbol and attach it to the symbol list
Any constant expressions encountered while building types for the parameters or return
value are put on the expression list (m_ExpressionListHead)

The proc or methodimpl should already be allocated and the current context should
point to it.

If OutParameterSymbolList is not NULL, we use this list as the list of parameters
for the new proc (and possibly add to it if ParameterList is not NULL). However,
if OutParameterSymbolList is NULL, then we build our new list of parameters from
ParameterList and return it in OutParameterSymbolList.
The reason we do this is for those cases like properties where we want to use
the same parameters for the get/set method (i.e. we don't want to manufacture
a duplicate set for each of these that will log duplicate errors when they
hit bindable, etc. etc.)
*****************************************************************************/
void Declared::MakeProc
(
    ParseTree::Statement::Opcodes MethodType,               // [in] what kind of method we are building
    ParseTree::IdentifierDescriptor *MethodNameDescriptor,  // [in] optional name tree for the proc - may be null in which case MethodName is used
    _In_opt_z_ STRING *MethodName,                                     // [in] optional name for the proc - may be NULL iff ptreeName is supplied, instead.
    ParseTree::ParameterList *ParameterList,                // [in] the parameters to the method
    ParseTree::Type *ReturnTypeTree,                        // [in] the return type of the method ( NULL if building a SUB )
    ParseTree::AttributeSpecifierList *ReturnTypeAttributes,    // [in] attributes on return type ( NULL if no attributes )
    ParseTree::Expression * LibraryExpression,              // [in] For a DLL Declare
    ParseTree::Expression * AliasExpression,                // [in] For a DLL Declare
    ParseTree::GenericParameterList *GenericParameters,     // [in] the type parameters of the method
    DECLFLAGS MethodFlags,                  // [in] the declaration flags for the method
    bool DefinedOnInterface,                // [in] is this in an interface?
    CodeBlockLocation *CodeBlock,           // [in] the location information for just the code within the proc
    CodeBlockLocation *ProcBlock,           // [in] the location for the proc
    BCSYM_Proc *Procedure,                  // [in/Out] the proc we need to build
    SymbolList *OwningSymbolList,           // [in] the symbol list that we will attach the built proc to - may be NULL
    bool *SeenConstructor,                  // [in/out] whether we've seen a constructor or not
    bool *SeenSharedConstructor,            // [in/out] whether we've seen a shared constructor or not
    ParseTree::SpecifierList *Specifiers,   // [in] specifier location information
    DECLFLAGS ContainerFlags,               // [in] flags for the container where this property is
    BCSYM **PropertyReturnTypeSymbol,       // [in/out] the symbol for the return type for properties.  If this is supplied, then it is used, otherwise,
                                            // a NamedType symbol is built, added to the NamedTypes list, and retunred in this [in/out] param
    BCSYM_Param **OutParameterSymbolList,   // [in/out] Params for the method we are building
    UserDefinedOperators Operator           // [in, optional] the kind of operator to create - default is OperatorUNDEF
)
{
    // Everything we have to gather.
    BCSYM *ReturnTypeSymbol = NULL;
    BCSYM_Param *ParameterSymbolList = NULL;
    BCSYM_Param *ReturnTypeParameter = NULL;

    // We need any constant expressions to be put on the constant expression list in order, but we evaluate
    // parameters in reverse order.  Do a little munging to get around this problem.

    BCSYM_Expression *ExpressionListHead = *GetExpressionListHead();
    BCSYM_Expression *ExpressionListTail = *GetExpressionListTail();
    SetExpressionListHead(NULL);
    SetExpressionListTail(NULL);

    // If we have no name, there isn't much to do.
    VSASSERT( MethodName || MethodNameDescriptor, "A name must be provided" );

    if ( !MethodName )
    {
        MethodName = MethodNameDescriptor->Name;
    }
    // keeping this out of the !MethodName check leads to an interesting effect - some synthetic code will point at the thing
    // that generated the proc.  That may be odd sometimes, sometimes is good (delegates), but in general is always better than pointing at nothing...
    Location *MethodNameTextSpan = NULL;
    if ( MethodNameDescriptor &&
        ( !DefinedOnInterface || !(( MethodFlags & DECLF_PropGet ) || ( MethodFlags & DECLF_PropSet ))))
    {
        MethodNameTextSpan = &MethodNameDescriptor->TextSpan;
    }

    // Sub's can't have type characters
    if ( !( MethodFlags & DECLF_HasRetval ) && MethodNameDescriptor && ( MethodNameDescriptor->TypeCharacter != chType_NONE ) && !( MethodFlags & DECLF_PropSet ))
    {
        ReportError( ERRID_TypeCharOnSub, MethodNameTextSpan );
    }

    // Note that it is important to make the type parameters before the normal parameters,
    // so that we can name clash errors for normal parameters with same names as the type
    // parameters.
    //
    BCSYM_GenericParam *FirstGenericParam = NULL;

    MakeGenericParameters( GenericParameters,
                           &FirstGenericParam,
                           Procedure,
                           MethodName,
                           MethodFlags & DECLF_HasRetval );

    // Need to set this early for procs, so that the type param hash is available when
    // looking for clashes between type param and normal param names.
    //
    m_SymbolAllocator.SetGenericParams( FirstGenericParam, Procedure );

    STRING *EmittedName = MethodName;

    // Check the flags on constructors
    if ( MethodType == ParseTree::Statement::ConstructorDeclaration )
    {
        if ( DefinedOnInterface )
        { // you can't declare a constructor from within an interface
            ReportError( ERRID_NewInInterface, MethodNameTextSpan );
        }
        else
        {
            if ( MethodFlags & DECLF_OverloadsKeywordUsed )
            {
                ReportDeclFlagError( ERRID_BadFlagsOnNewOverloads, DECLF_OverloadsKeywordUsed, Specifiers );
                MethodFlags &= ~DECLF_OverloadsKeywordUsed;
            }

            if ( MethodFlags & DECLF_OverridesKeywordUsed )
            {
                ReportDeclFlagError( ERRID_CantOverrideConstructor, DECLF_OverridesKeywordUsed, Specifiers );
                MethodFlags &= ~DECLF_OverridesKeywordUsed;
            }

            const DECLFLAGS DECLF_InvalidFlagsOnNew = DECLF_Overridable | DECLF_MustOverride | DECLF_NotOverridable | DECLF_Static | DECLF_ShadowsKeywordUsed;
            if ( MethodFlags & DECLF_InvalidFlagsOnNew )
            {

                ReportDeclFlagError( ERRID_BadFlagsOnNew1, MethodFlags & DECLF_InvalidFlagsOnNew, Specifiers );
                MethodFlags &= ~DECLF_InvalidFlagsOnNew;
            }

            if ( !( MethodFlags & DECLF_Shared ))
            {
                if ( SeenConstructor )
                {
                    *SeenConstructor = true; // Don't allow both a non-shared constructor and an initialize event.
                }
                EmittedName = STRING_CONST( m_CompilerInstance, Constructor ); // Set the emitted name for a constructor
            }
            else
            {
                // Remember that we've seen this.
                if ( SeenSharedConstructor )
                {
                    *SeenSharedConstructor = true;
                }

                // A shared constructor cannot have any parameters
                if ( ParameterList && ParameterList->Element )
                {
                    ReportError( ERRID_SharedConstructorWithParams, MethodNameTextSpan );
                }

                // Set the name and emitted name for a shared constructor - this is a hidden name as shared constructors are not callable directly.
                MethodName = STRING_CONST( m_CompilerInstance, SharedConstructor );
                EmittedName = MethodName;
            }
            MethodFlags |= DECLF_Constructor | DECLF_SpecialName; // mark this method as being a constructor
        }
    }
    else if ( MethodType == ParseTree::Statement::OperatorDeclaration )
    {
        EmittedName = m_CompilerInstance->GetOperatorCLSName(Operator);
        Procedure->PUserDefinedOperator()->SetOperator(Operator);
    }
    else if ( !( MethodFlags & DECLF_Function )) // Map the emitted name for property functions.
    {
        const WCHAR *PrefixString;
        switch ( MethodFlags & DECLF_InvokeFlags )
        {
            case DECLF_PropGet:
                PrefixString = CLS_PROPERTY_GET_PREFIX;
                break;

            case DECLF_PropSet:
                if (m_SourceFile &&
                    m_SourceFile->GetCompilerProject() && 
                    m_SourceFile->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj)
                {
                    PrefixString = CLS_PROPERTY_PUT_PREFIX;
                }
                else
                {
                    PrefixString = CLS_PROPERTY_SET_PREFIX;
                }
                break;

            default:
                VSFAIL( "Bad invokekind." );
                break;
        }

        EmittedName = m_CompilerInstance->ConcatStrings( PrefixString, MethodName );
        MethodName = EmittedName;
    }

    // Get the method's return type.
    if ( MethodFlags & DECLF_HasRetval )
    {
        if ( PropertyReturnTypeSymbol && *PropertyReturnTypeSymbol )
        {
            ReturnTypeSymbol = *PropertyReturnTypeSymbol;
        }
        else
        {
            ReturnTypeSymbol = MakeType( ReturnTypeTree, MethodNameDescriptor, Procedure, GetTypeList(), MethodFlags );
        }

        if ( ReturnTypeAttributes )
        {
             // Create a param for the return type and attach the attributes to it.
            BuildReturnTypeParamAndAttachAttributes( &ReturnTypeParameter, 
                                                      Procedure, 
                                                      ReturnTypeAttributes, 
                                                      ReturnTypeSymbol);
        }
    }

    // bug 10265: no private on MustOverride functions
    if ( MethodFlags & DECLF_MustOverride )
    {
        if ( MethodFlags & DECLF_Private )
        {
            ReportDeclFlagComboError( DECLF_MustOverride | DECLF_Private, Specifiers );
            MethodFlags &= ~DECLF_Private;
        }

        // Nor async
        if ( MethodFlags & DECLF_Async )
        {
            ReportDeclFlagComboError( DECLF_MustOverride | DECLF_Async, Specifiers );
            MethodFlags &= ~DECLF_Async;
        }
    }

    // Special cases on Overridable functions
    if ( MethodFlags & DECLF_Overridable )
    {
        // They can't be private
        if ( MethodFlags & DECLF_Private )
        {
            if ( ( MethodFlags & DECLF_PropGet ) || ( MethodFlags & DECLF_PropSet ))
            {
                ReportDeclFlagError(ERRID_BadPropertyAccessorFlags3, DECLF_Overridable, Specifiers );
            }
            else
            {
                ReportDeclFlagComboError( DECLF_Overridable | DECLF_Private, Specifiers, DECLF_Overridable );
            }
            MethodFlags &= ~DECLF_Overridable;
        }
        // Overrides implies Overridable
        else if ( MethodFlags & DECLF_OverridesKeywordUsed )
        {
            ReportDeclFlagError( ERRID_OverridesImpliesOverridable, DECLF_Overridable, Specifiers );
            MethodFlags &= ~DECLF_Overridable;
        }
    }

    // Special cases on Overriding functions
    if ( MethodFlags & DECLF_OverridesKeywordUsed )
    {
        // They can't shadow - what would you override in that case?
        if ( MethodFlags & DECLF_ShadowsKeywordUsed )
        {
            ReportDeclFlagComboError( DECLF_OverridesKeywordUsed | DECLF_ShadowsKeywordUsed, Specifiers );
            MethodFlags &= ~DECLF_OverridesKeywordUsed;
        }
    }

    // Is this method name declared with brackets?
    if ( MethodNameDescriptor && MethodNameDescriptor->IsBracketed == true )
    {
        MethodFlags |= DECLF_Bracketed;
    }

#if DEBUG
    // Build the parameter symbols
    if ( ( MethodFlags & DECLF_PropGet ) || ( MethodFlags & DECLF_PropSet ))
    {
        // If the proc is a Property Get or Set, then the list of parameters was already built when we
        // processed the property itself, so we should use that list to get the types for each parameter.
        // Make sure that there is such a list.
        VSASSERT( OutParameterSymbolList, "OutParameterSymbolList must not be NULL for a property Get or Set" );
    }
#endif DEBUG

    if ( ParameterList )
    {
        MakeParams( (MethodFlags & DECLF_HasRetval) ?
                        MethodName : NULL, // the name of the retval is the name of the method - if we have a ret val
                    ParameterList,
                    MethodFlags,
                    Procedure,
                    ContainerFlags,
                    &ParameterSymbolList,
                    NULL,
                    PropertyReturnTypeSymbol ?
                        *PropertyReturnTypeSymbol : NULL,
                    OutParameterSymbolList ?
                        *OutParameterSymbolList : NULL);
    }
    else
    {
        OutParameterSymbolList ? ParameterSymbolList = *OutParameterSymbolList : ParameterSymbolList = NULL;
    }

    // Process Lib.
    STRING *LibraryString = LibraryExpression && LibraryExpression->Opcode == ParseTree::Expression::StringLiteral ?
                            m_CompilerInstance->AddString( LibraryExpression->AsStringLiteral()->Value ) :
                            NULL;

    // Process Alias.
    STRING *AliasString   = AliasExpression && AliasExpression->Opcode == ParseTree::Expression::StringLiteral ?
                            m_CompilerInstance->AddString( AliasExpression->AsStringLiteral()->Value ) :
                            NULL;

    // Create the procedure.

    // The built symbol is hung off of OwningSymbolList
    m_SymbolAllocator.GetProc( MethodNameTextSpan, MethodName, EmittedName,
                               CodeBlock,
                               ProcBlock,
                               MethodFlags,
                               ReturnTypeSymbol, // return type of the proc
                               ParameterSymbolList, // parameters to the proc
                               ReturnTypeParameter,
                               LibraryString, // library name
                               AliasString, // alias name
                               SYNTH_None, // synthetic kind
                               FirstGenericParam,
                               OwningSymbolList,  // built proc will be added to this list
                               Procedure ); // the context - an empty proc - becomes the built proc symbol

    // Any expressions created while evaluating this proc are on the list in reverse order.  Put them back on the main list.
    BCSYM_Expression *NewExpressionListHead = *GetExpressionListHead();
    if (NewExpressionListHead)
    {
        BCSYM_Expression *ReverseMe = NULL, *NewTail = *GetExpressionListHead();

        // Reverse the list.
        while ( NewExpressionListHead )
        {
            BCSYM_Expression *NextExpression = NewExpressionListHead->GetNext();
            Symbols::SetNextExpression( NewExpressionListHead, ReverseMe );
            ReverseMe = NewExpressionListHead;
            NewExpressionListHead = NextExpression;
        }

        // Link it on.
        if ( !ExpressionListTail )
        {
            ExpressionListHead = ReverseMe;
            ExpressionListTail = NewTail;
        }
        else
        {
            Symbols::SetNextExpression( ExpressionListTail, ReverseMe );
            ExpressionListTail = NewTail;
        }
    }

    // Keep track of the additions to the expression list
    SetExpressionListHead(ExpressionListHead);
    SetExpressionListTail(ExpressionListTail);

    // Send the Return type back to the caller.
    if ( PropertyReturnTypeSymbol && ReturnTypeSymbol )
    {
        *PropertyReturnTypeSymbol = ReturnTypeSymbol;
    }

    // Return the list back to the caller.
    if ( OutParameterSymbolList )
    {
        *OutParameterSymbolList = ParameterSymbolList;
    }
}

/////////////////////////////////////////
//
// BuildReturnTypeParamAndAttachAttributes
//
// Creates a BCSYM_Param that represents the return type and attaches
// the given attributes to it.
//
void Declared::BuildReturnTypeParamAndAttachAttributes
(
    _Out_ BCSYM_Param **ppReturnTypeParam,
    _Inout_ BCSYM_Proc *pProc,
    _Inout_ ParseTree::AttributeSpecifierList * pReturnTypeAttributes,
    _In_ BCSYM *pReturnType
)
{
    ThrowIfNull(pProc);
    ThrowIfNull(pReturnTypeAttributes);
    ThrowIfNull(pReturnType);
    ThrowIfNull(ppReturnTypeParam);
    
    // Load localized string to name this pseudo-param.  The return type param needs a name so we can report errors against it.
    WCHAR wszFunctionReturnType[512];
    if (FAILED(ResLoadString(STRID_FunctionReturnType, wszFunctionReturnType, DIM(wszFunctionReturnType))))
    {
        wcscpy_s(wszFunctionReturnType, _countof(wszFunctionReturnType), L"function return type");
    }

    // 
    *ppReturnTypeParam = m_SymbolAllocator.GetParam( NULL,
                                                      m_CompilerInstance->AddString(wszFunctionReturnType),
                                                      pReturnType,
                                                      0, // no flags
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      true);  // Is a return type param

    ProcessAttributeList( pReturnTypeAttributes, *ppReturnTypeParam, pProc );
}

/*========================================================================
Synthesized type members
========================================================================*/

/*****************************************************************************
;CreateSynthesizedSymbols

Generates the project-wide synthesized symbols for this module.
When generating names for synthesized symbols, don't use the bracket notation
( e.g. [Decimal] ) because brackets are always removed by the time a symbol gets
created.  By sneaking them in here, binding will get messed up.  It is the actual
synthetic code generation that should put the brackets on when appropriate,
but for symbol names, they shouldn't be put on.
*****************************************************************************/
void Declared::CreateSynthesizedSymbols(
    SymbolList * ClassChildren, // [in] We go through this list looking for WithEvent symbols & data members
    DECLFLAGS DeclFlags, // [in] declaration flags for this symbol
    bool SeenConstructor, // [in] whether a constructor has been seen
    bool SeenSharedConstructor, // [in] whether a shared constructor has been seen
    bool NeedSharedConstructor, // [in] whether a shared constructor is required
    bool NeedMain, // [in] whether we've seen main
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    CreateStartAndTerminateSymbols( DeclFlags, SeenConstructor, SeenSharedConstructor, NeedSharedConstructor, NeedMain, OwningSymbolList );

    if ( ClassChildren->GetCount() == 0 ) // if the symbol list is empty, then we are done
    {
        return;
    }

    // The following actions are done for all modules, even the hidden ones
    // NOTE: By the time that Declared::CreateSynthesizedSymbols is called, ClassChildren is already initialized & points to a valid class

    for ( BCSYM_NamedRoot *CurrentClassMember = ClassChildren->GetFirst(); CurrentClassMember != NULL; CurrentClassMember = CurrentClassMember->GetNextInSymbolList())
    {
        // If we aren't on a WithEvent variable, move on to the next symbol
        if ( !CurrentClassMember->IsVariable() || CurrentClassMember->PVariable()->GetVarkind() != VAR_WithEvents || CurrentClassMember->IsBad())
        {
            continue;
        }

        // Generate symbols for WithEvent variables
        MakeBackingField( CurrentClassMember->PVariable(), 0 != (DeclFlags & DECLF_Module), OwningSymbolList );
    }
}

/*****************************************************************************
;CreateStartAndTerminateSymbols

Generate synthetic Main and a constructor if there isn't one in the class.
*****************************************************************************/
void Declared::CreateStartAndTerminateSymbols(
    DECLFLAGS DeclFlags, // [in] information about the symbol
    bool SeenConstructor, // [in] have we seen a constructor?
    bool SeenSharedConstructor, // [in] have we seen a shared constructor?
    bool NeedSharedConstructor, // [in] do we need to a shared constructor if there isn't one?
    bool NeedMain, // [in] do we need to create a main?
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    // Generate a constructor if none exists in this class.
    if ( !SeenConstructor && 0 == (DeclFlags & DECLF_Module) && 0 == (DeclFlags & DECLF_Structure))
    {
        SyntheticConstructorTypeEnum makeWhat = (0 == (DeclFlags & DECLF_MustInherit)) ?
            SyntheticConstructorType::Instance : SyntheticConstructorType::MustInherit;
        BuildSyntheticConstructor( &m_SymbolAllocator, m_CompilerInstance, OwningSymbolList, makeWhat );
    }

    // Generate a class constructor if needed.
    if ( !SeenSharedConstructor && NeedSharedConstructor )
    {
        BuildSyntheticConstructor( &m_SymbolAllocator,
                                   m_CompilerInstance,
                                   OwningSymbolList,
                                   SyntheticConstructorType::Shared );
    }

    if ( NeedMain )
    {
        STRING *NameOfMain = STRING_CONST(m_CompilerInstance, Main);
        BCSYM_SyntheticMethod *SyntheticMain = m_SymbolAllocator.AllocSyntheticMethod();
        m_SymbolAllocator.GetProc( NULL,
                                   NameOfMain,
                                   NameOfMain,
                                   ( CodeBlockLocation* ) NULL,
                                   ( CodeBlockLocation* ) NULL,
                                   DECLF_Function | DECLF_Public | DECLF_Shared | DECLF_OverloadsKeywordUsed,
                                   NULL,
                                   NULL, // no parameters
                                   NULL,
                                   NULL,
                                   NULL,
                                   SYNTH_FormMain,
                                   NULL, // no generic params
                                   OwningSymbolList,
                                   SyntheticMain );
    }
}

BCSYM_Proc* Declared::BuildSyntheticConstructor
(
    Symbols *SymbolAllocator,
    Compiler *CompilerInstance,
    SymbolList *OwningSymbolList, // [in] symbol list to add to
    SyntheticConstructorTypeEnum makeWhat
)
{
    return Declared::BuildSyntheticConstructor(
        SymbolAllocator,
        CompilerInstance,
        NULL,
        OwningSymbolList,
        makeWhat
        );
}

BCSYM_Proc* Declared::BuildSyntheticConstructor
(
    Symbols *SymbolAllocator,
    Compiler *CompilerInstance,
    BCSYM_Param *Parameters,
    SymbolList *OwningSymbolList, // [in] symbol list to add to
    SyntheticConstructorTypeEnum makeWhat
)
{
    DECLFLAGS DeclFlags = DECLF_Constructor | DECLF_Function | DECLF_SpecialName;
    STRING *pstrName;
    STRING *pstrEmittedName;
    SyntheticKind Kind;

    switch (makeWhat)
    {
        case SyntheticConstructorType::Shared:
        {
            pstrName = STRING_CONST( CompilerInstance, SharedConstructor );
            pstrEmittedName = CompilerInstance->AddString( COR_CCTOR_METHOD_NAME_W );

            DeclFlags = DeclFlags | DECLF_Private | DECLF_Shared;
            Kind = SYNTH_SharedNew;
            break;
        }

        default:
            VSASSERT( false, "Undefined synthetic constructor type" );

        case SyntheticConstructorType::Instance:
        {
            pstrName = STRING_CONST( CompilerInstance, Constructor );
            pstrEmittedName = CompilerInstance->AddString( COR_CTOR_METHOD_NAME_W );

            DeclFlags = DeclFlags | DECLF_Public;
            Kind = SYNTH_New;
            break;
        }

        case SyntheticConstructorType::MustInherit:
        {
            pstrName = STRING_CONST( CompilerInstance, Constructor );
            pstrEmittedName = CompilerInstance->AddString( COR_CTOR_METHOD_NAME_W );

            DeclFlags = DeclFlags | DECLF_Protected;
            Kind = SYNTH_New;
            break;
        }
    }

    BCSYM_SyntheticMethod *pmeth = SymbolAllocator->AllocSyntheticMethod();

    SymbolAllocator->GetProc( NULL,
                        pstrName,
                        pstrEmittedName,
                        ( CodeBlockLocation* ) NULL,
                        ( CodeBlockLocation* ) NULL,
                        DeclFlags,
                        NULL,
                        Parameters,
                        NULL,
                        NULL,
                        NULL,
                        Kind,
                        NULL, // no generic params
                        OwningSymbolList,
                        pmeth );

    return pmeth;
}


//-------------------------------------------------------------------------------------------------
// MakeBackingField
// 
// MakeBackingField creates the hidden variable for both WithEvent variables and automatic
// properties.
//
//
// For "[access] dim withevents x as class1" we create:
//
//    1. PRIVATE _x as class1
//    2. [access] Property x() as class1
//    2a.                Set( _o as class1 )
//    2b.                Get
//                End Property
//-------------------------------------------------------------------------------------------------
void Declared::MakeBackingField(
    _Inout_ BCSYM_Member *pMember, // the current symbol we are supposed to generate accessors for
                                   // pMember is only modified if it is an auto property
    bool definedInStdModule, // whether the member was defined within a standard module
    _Inout_ SymbolList *pOwningSymbolList, // symbol list to add to
    _In_opt_ ParseTree::AutoPropertyInitialization *pAutoPropertyDeclaration,
    _In_opt_ ParseTree::AttributeSpecifierList *pAutoPropertyReturnTypeAttributes
)
{

    VSASSERT( pMember && pOwningSymbolList, "Inputs to Declared::MakeBackingField cannot be NULL" );
    VSASSERT( (pMember->IsVariable() && (pMember->PVariable()->GetVarkind() == VAR_WithEvents || pMember->PVariable()->IsWithEvents()) && 
                    !pAutoPropertyDeclaration && !pAutoPropertyReturnTypeAttributes ) ||
        (pMember->IsProperty() && pMember->PProperty()->IsAutoProperty()),
        "Declared::MakeBackingField only accepts WithEvent variables or auto properties" );

    bool hasLocation = pMember->HasLocation();

    // 1. PRIVATE _x as class1

    STRING *pMemberName = pMember->GetName();
    // NOTE: metaemit.cpp depends on this name being _WithEventVarName
    STRING *pUnderscoreMemberName = m_CompilerInstance->ConcatStrings( WIDE("_"), pMemberName );
    BCSYM_Variable *pMemberVar = m_SymbolAllocator.AllocVariable( hasLocation, pAutoPropertyDeclaration != NULL ? true : false);
    BCSYM_Expression *InitializerExpression = NULL;

    if (pMember->IsProperty() && pMember->PProperty()->IsAutoProperty())
    {
#if IDE 
        pMember->PProperty()->SetAutoPropertyBackingField(pMemberVar);
#endif

        // Get the initializer value, if we have one.
        if (pAutoPropertyDeclaration != NULL)        
            InitializerExpression = MakeAutoPropertyInitializerExpression(pAutoPropertyDeclaration, 
                                                                          pMember->PProperty());  
    }

    m_SymbolAllocator.GetVariable( NULL,
                                   pUnderscoreMemberName,
                                   pUnderscoreMemberName,
                                   DECLF_Dim | DECLF_Private | DECLF_Hidden |
                                   (pMember->IsShared() ? DECLF_Shared : 0) |
                                   (pMember->IsShadowsKeywordUsed() ? DECLF_ShadowsKeywordUsed : 0) |
                                   ((pMember->IsVariable() && pMember->PVariable()->IsWithEvents()) ?
                                     DECLF_WithEvents : 0 ) |
                                   ((pMember->IsProperty() && pMember->PProperty()->IsAutoProperty()) ?
                                     DECLF_AutoProperty: 0 ) |  
                                   ((pAutoPropertyDeclaration != NULL) && 
                                     (pAutoPropertyDeclaration->Opcode == ParseTree::AutoPropertyInitialization::WithNew) ?
                                     DECLF_New : 0),
                                   VAR_Member,
                                   pMember->GetRawType(),
                                   InitializerExpression,
                                   pOwningSymbolList,
                                   pMemberVar );

    if ( hasLocation )
    {
        pMemberVar->SetLocation( pMember->GetLocation(), true /* Location is inherited from a source symbol */ );
    }

    pMemberVar->SetMemberThatCreatedSymbol( pMember );

    if ( pMember->IsVariable() && (pMember->PVariable()->GetVarkind() == VAR_WithEvents || pMember->PVariable()->IsWithEvents() ) )
    {
            
#if DEBUG
        if ( VSFSWITCH( fDumpSyntheticCode ))
        {
            DebPrintf( "  [Synth WithEvents var] Private Hidden WithEvents %S%S As %S\n\n", 
                       pMember->PVariable()->IsShared() ? "Shared " : "", 
                       pMemberVar->GetName(), 
                       pMember->GetRawType()->PNamedType()->GetNameArray()[0] );
        }
#endif DEBUG

        ProcessAttributeList( pMember->PVariable()->GetAttributesTree(), pMemberVar );  // #149166 - Push attribute info onto synthetic withevent var

        // Create the property
        BCSYM_Property *WithEventsProperty = BuildWithEventsProperty( 
                        pMember->PVariable(), 
                        NULL, 
                        &m_SymbolAllocator, 
                        false /* !overrides */, 
                        definedInStdModule, 
                        m_CompilerInstance, 
                        pOwningSymbolList, 
                        GetUnBindableChildrenSymbolList() );
        
        WithEventsProperty->SetMemberThatCreatedSymbol( pMember );
    }
    
    else // pMember is an auto property
    {        
        // Set flag describing whether or not the property is new.
        pMember->PProperty()->SetNewAutoProperty(pMemberVar->IsNew());  

        BuildAutoPropertyMethods(pMember->PProperty(), 
            pAutoPropertyReturnTypeAttributes,
            &m_SymbolAllocator, 
            definedInStdModule, 
            m_CompilerInstance, 
            pOwningSymbolList, 
            GetUnBindableChildrenSymbolList() );
    }
        
}

BCSYM_Expression*
Declared::MakeAutoPropertyInitializerExpression
(
    _In_ ParseTree::AutoPropertyInitialization *pAutoPropertyDeclaration,
    _In_ BCSYM_Property *pPropertySymbol
)
{
    AssertIfNull(pAutoPropertyDeclaration);
    AssertIfNull(pPropertySymbol);
    
    BCSYM_Expression *pInitializerExpression = NULL;

    if ( pAutoPropertyDeclaration->Opcode == ParseTree::AutoPropertyInitialization::WithInitializer &&
             pAutoPropertyDeclaration->AsInitializer()->InitialValue->Opcode == ParseTree::Initializer::Deferred )
        {
            ParseTree::DeferredInitializer *DeferredInit = pAutoPropertyDeclaration->AsInitializer()->InitialValue->AsDeferred();

            // Encode the text of the entire initializer. Though an initializer is not, strictly speaking, an "expression",
            // the expression symbol mechanism works to supply the initializer to semantic analysis.

            // Make sure that bindable does not find the expression symbol as a constant to evaluate--don't put in on the list that
            // bindable traverses. There are several reasons for this:
            //      1) An aggregate initializer is not an an expression
            //      2) If the initial value of a variable is an evaluated expression, semantic analysis will assume that it
            //         is something other than an initializer.
            // 

            AssertIfFalse(ParseTree::Initializer::Expression == DeferredInit->Value->Opcode);

            pInitializerExpression = m_SymbolAllocator.GetConstantExpression(
                    &DeferredInit->Value->AsExpression()->Value->TextSpan,
                    pPropertySymbol, // current context
                    NULL, NULL, // head and tail of expression list -- don't want this symbol on the list of constants to evaluate
                    NULL, // no expression string
                    DeferredInit->Text, // initializer text
                    DeferredInit->TextLengthInCharacters );

        }

        else if ( pAutoPropertyDeclaration->Opcode == ParseTree::AutoPropertyInitialization::WithNew &&
                  pAutoPropertyDeclaration->AsNew()->DeferredInitializerText )
        {
            // See the explanation above about deferring initializers.

            pInitializerExpression = 
                BuildNewDeferredInitializerExpression
                (
                    pPropertySymbol, 
                    pAutoPropertyDeclaration->AsNew()->DeferredInitializerText, 
                    pAutoPropertyDeclaration->AsNew()->ObjectInitializer, 
                    pAutoPropertyDeclaration->AsNew()->Arguments,
                    pAutoPropertyDeclaration->AsNew()->CollectionInitializer,
                    pAutoPropertyDeclaration->TextSpan,
                    &pAutoPropertyDeclaration->AsNew()->From
                );
        }

        return pInitializerExpression;
}

/**************************************************************************************************
;BuildWithEventsProperty

Builds the property for MakeWithEventsSymbols().  It is static so it can be called from Bindable
***************************************************************************************************/
BCSYM_Property * // the built property symbol
Declared::BuildWithEventsProperty
(
    BCSYM_Variable *WithEventVar, // [in] the current symbol we are supposed to generate accessors for
    BCSYM_GenericBinding *GenericBindingContext, // [in] the context used to bind generic parameters of the type of the WithEvents field
    Symbols *SymbolAllocator, // [in] used to allocate the symbols for the synthetic methods/property
    bool MarkAsOverrides, // [in] whether to mark the created property as overrides or not and flag the get/set as CallBaseFirst()
    bool DefinedInStdModule, // [in] whether the WithEvents variable was defined within a standard module
    Compiler *CompilerInstance, // [in] instance of the compiler
    SymbolList *OwningSymbolList, // [in-optional] symbol list to add to
    SymbolList *OwningUnBindableSymbolList // [in-optional] symbol list to add the get and set to
)
{
    STRING *WithEventVarName = WithEventVar->GetName();
    bool WithEventHasLocation;

    if (!MarkAsOverrides && WithEventVar->HasLocation())
    {
        WithEventHasLocation = true;
    }
    else
    {
        WithEventHasLocation = false;
    }

    BCSYM *WithEventVarType = WithEventVar->GetRawType();

    if ( GenericBindingContext )
    {
        // This call has come through Bindable and named types have been bound, so chasing through the type is OK
        // (as well as being necessary).

        WithEventVarType = ReplaceGenericParametersWithArguments ( WithEventVarType->DigThroughAlias(),
                                                                   GenericBindingContext,
                                                                   *SymbolAllocator );
    }

    // 2a. [access] property set x( _o as class1 )

    // The _o parameter in property set x( _o as class1 )
    BCSYM_Param *ParamFirst = NULL, *LastParam = NULL;
    BCSYM_SyntheticMethod *SetMethod = SymbolAllocator->AllocSyntheticMethod(WithEventHasLocation   /* HasLocation ?*/);
    SymbolAllocator->GetParam( NULL, // no location
                               CompilerInstance->AddString( WIDE( "WithEventsValue" )), // arg name  Microsoft: Consider Should this name be Value? Does it affect the CLR or COM Interop?
                               WithEventVarType,
                               NULL,
                               NULL,
                               &ParamFirst,
                               &LastParam,
                               false);  // Not a return type param

    // build the property set

    DECLFLAGS DeclarationFlags = DECLF_Hidden;
    DeclarationFlags |= Symbols::MapAccessToDeclFlags( WithEventVar->GetAccess());
    DeclarationFlags |= WithEventVar->IsShadowsKeywordUsed() ? DECLF_ShadowsKeywordUsed : 0;
    if ( DefinedInStdModule || WithEventVar->IsShared())
    {
        DeclarationFlags |= DECLF_Shared;
    }
    else
    {
        DeclarationFlags |= DECLF_Overridable;
    }

    if ( MarkAsOverrides )
    {
        DeclarationFlags |= DECLF_OverridesKeywordUsed;
    }

    STRING *WithEventSetName = NULL;
    if (CompilerInstance && 
        CompilerInstance->GetProjectBeingCompiled() &&
        CompilerInstance->GetProjectBeingCompiled()->GetOutputType() == OUTPUT_WinMDObj)
    {
        WithEventSetName = CompilerInstance->ConcatStrings( CLS_PROPERTY_PUT_PREFIX, WithEventVarName );
    }
    else
    {
        WithEventSetName = CompilerInstance->ConcatStrings( CLS_PROPERTY_SET_PREFIX, WithEventVarName );
    }

    SymbolAllocator->GetProc( NULL,
                              WithEventSetName,
                              WithEventSetName, // the emitted name
                              (CodeBlockLocation*) NULL,
                              (CodeBlockLocation*) NULL,
                              DECLF_PropSet | DECLF_SpecialName | DeclarationFlags,
                              NULL,
                              ParamFirst, // _o as class1
                              NULL,
                              NULL,
                              NULL,
                              SYNTH_WithEventsSet,
                              NULL, // no generic params
                              OwningUnBindableSymbolList,
                              SetMethod );


    if (WithEventHasLocation)
    {
        SetMethod->SetLocation(WithEventVar->GetLocation(), TRUE /* Location is inherited from the withevents variable */);
    }

    if ( MarkAsOverrides )
    {
        SetMethod->SetCallBaseFirst();
    }

    // 2b. get

    STRING *WithEventGetName = CompilerInstance->ConcatStrings( CLS_PROPERTY_GET_PREFIX, WithEventVarName );
    BCSYM_SyntheticMethod *GetMethod = SymbolAllocator->AllocSyntheticMethod(WithEventHasLocation   /* HasLocation ?*/);
    SymbolAllocator->GetProc( NULL,
                             WithEventGetName,
                             WithEventGetName, // the emitted name
                             ( CodeBlockLocation* ) NULL,
                             ( CodeBlockLocation* ) NULL,
                             DECLF_PropGet | DECLF_SpecialName | DeclarationFlags,
                             WithEventVarType,
                             NULL, // no parameters
                             NULL,
                             NULL, // no lib name
                             NULL, // no alias name
                             SYNTH_WithEventsGet,
                             NULL, // no generic params
                             OwningUnBindableSymbolList,
                             GetMethod );


    if (WithEventHasLocation)
    {
        GetMethod->SetLocation(WithEventVar->GetLocation(), TRUE /* Location is inherited from the withevents variable */);
    }

    if ( MarkAsOverrides )
    {
        GetMethod->SetCallBaseFirst();
    }

    // 2. [Property] symbol that holds the get/set
    BCSYM_Property *PropertySymbol = SymbolAllocator->AllocProperty(WithEventHasLocation   /* HasLocation ?*/);

    // Builds the proc into the base class portion of our PropertySymbol
    SymbolAllocator->GetProc( NULL,
                       WithEventVarName,
                       WithEventVarName, // the emitted name
                       ( CodeBlockLocation* ) NULL,
                       ( CodeBlockLocation* ) NULL,
                       DeclarationFlags | DECLF_Overridable | DECLF_Function | DECLF_HasRetval |
                           (WithEventVar->IsBracketed() == true ? DECLF_Bracketed : 0),
                       WithEventVarType,
                       NULL, // no parameters
                       NULL,
                       NULL, // not lib name
                       NULL, // no alias name
                       SYNTH_None,
                       NULL, // no generic params
                       NULL, // don't hang it off a symbol list - it belongs to the base class of the property symbol
                       PropertySymbol->GetPropertySignature()); // Fills out the base class of the BCSYM_Property

    if (WithEventHasLocation)
    {
        PropertySymbol->SetLocation(WithEventVar->GetLocation(), TRUE /* Location is inherited from the withevents variable */);
    }

    SymbolAllocator->GetProperty( NULL,
                                  WithEventVarName,
                                  DeclarationFlags, // Since it has both a getter/setter, there is no DECLF_* flag to send to indicate WriteOnly or ReadOnly
                                  GetMethod,
                                  SetMethod,
                                  PropertySymbol,
                                  OwningSymbolList ); // add this property to the list of symbols

    return PropertySymbol;
}

//-------------------------------------------------------------------------------------------------
// BuildAutoPropertyMethods
//
// Builds the methods for the auto property from  MakeBackingField(). The methods are then linked
// to the auto property symbol.
//-------------------------------------------------------------------------------------------------
void Declared::BuildAutoPropertyMethods
( 
    _Inout_ BCSYM_Property *pAutoProperty, // the current auto property symbol
    _In_opt_ ParseTree::AttributeSpecifierList *pReturnTypeAttributes, 
    _In_ Symbols *pSymbolAllocator, // used to allocate the symbols for the synthetic methods
    bool definedInStdModule, // whether the auto property was defined in a standard module
    _In_ Compiler *pCompilerInstance, // instance of the compiler
    _Inout_ SymbolList *pOwningSymbolList, // symbol list to add
    _Inout_ SymbolList *pOwningUnBindableSymbolList // symbol list to add the get and set to
)
{
    STRING *pAutoPropertyName = pAutoProperty->GetName();
    bool hasLocation = pAutoProperty->HasLocation();

    BCSYM *pAutoPropertyType = pAutoProperty->GetRawType();

    DECLFLAGS declarationFlags = DECLF_Hidden;
    declarationFlags |= Symbols::MapAccessToDeclFlags( pAutoProperty->GetAccess());
    declarationFlags |= pAutoProperty->IsShadowsKeywordUsed() ? DECLF_ShadowsKeywordUsed : 0;
    declarationFlags |= pAutoProperty->IsOverridesKeywordUsed() ? DECLF_OverridesKeywordUsed : 0;
    declarationFlags |= pAutoProperty->IsOverridableKeywordUsed() ? DECLF_Overridable : 0;
    declarationFlags |= pAutoProperty->GetImplementsList() ? DECLF_Implementing : 0;
    declarationFlags |= pAutoProperty->IsReadOnly() ? DECLF_ReadOnly : 0;
    
    if ( definedInStdModule || pAutoProperty->IsShared())
    {
       declarationFlags |= DECLF_Shared;
    }

    BCSYM_SyntheticMethod *pSetMethod = NULL;
    BCSYM_SyntheticMethod *pGetMethod = NULL;

    VSASSERT( !pAutoProperty->IsReadOnly(), "We cannot have ReadOnly AutoProperties");


    // 2a. [access] property set x( _o as class1 )

    // The _o parameter in property set x( _o as class1 )
    BCSYM_Param *pParamFirst = NULL, *pLastParam = NULL;
    pSetMethod = pSymbolAllocator->AllocSyntheticMethod(hasLocation );
    pSymbolAllocator->GetParam( NULL, // no location
                               pCompilerInstance->AddString( WIDE( "AutoPropertyValue" )), // arg name  Microsoft: Consider Should this name be Value? Does it affect the CLR or COM Interop?
                               pAutoPropertyType,
                               NULL, // param flags
                               NULL, // initial value
                               &pParamFirst,
                               &pLastParam,
                               false);  // Not a return type param

    // build the property set

    STRING *pAutoPropertySetName = NULL;
    if (m_SourceFile &&
        m_SourceFile->GetCompilerProject() && 
        m_SourceFile->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj)
    {
        pAutoPropertySetName = pCompilerInstance->ConcatStrings( CLS_PROPERTY_PUT_PREFIX, pAutoPropertyName );
    }
    else
    {
        pAutoPropertySetName = pCompilerInstance->ConcatStrings( CLS_PROPERTY_SET_PREFIX, pAutoPropertyName );
    }

    pSymbolAllocator->GetProc( NULL, // location
                              pAutoPropertySetName, // proc name
                              pAutoPropertySetName, // the emitted name
                              (CodeBlockLocation*) NULL,
                              (CodeBlockLocation*) NULL,
                              DECLF_PropSet | DECLF_SpecialName | declarationFlags,
                              NULL, // return type
                              pParamFirst, // _o as class1
                              NULL, // return paramater
                              NULL, // library name
                              NULL, // alias name
                              SYNTH_AutoPropertySet,
                              NULL, // no generic params
                              pOwningUnBindableSymbolList,
                              pSetMethod );


    if (hasLocation)
    {
        pSetMethod->SetLocation(pAutoProperty->GetLocation(), 
                                TRUE /* Location is inherited from the withevents variable */);
    }

    // 2b. get

    STRING *pAutoPropertyGetName = pCompilerInstance->ConcatStrings( CLS_PROPERTY_GET_PREFIX, 
                                                                     pAutoPropertyName );
    
    pGetMethod = pSymbolAllocator->AllocSyntheticMethod(hasLocation);

    BCSYM_Param *pReturnTypeParameter = NULL;

    if ( pReturnTypeAttributes )
    {
        // Create a return type param and attach attributes to it if necessary.
        BuildReturnTypeParamAndAttachAttributes( &pReturnTypeParameter, 
                                                 pGetMethod, 
                                                 pReturnTypeAttributes, 
                                                 pAutoPropertyType);
    }

    pSymbolAllocator->GetProc( NULL, // location
                             pAutoPropertyGetName,
                             pAutoPropertyGetName, // the emitted name
                             ( CodeBlockLocation* ) NULL,
                             ( CodeBlockLocation* ) NULL,
                             DECLF_PropGet | DECLF_SpecialName | declarationFlags,
                             pAutoPropertyType,
                             NULL, // no parameters
                             pReturnTypeParameter, // return paramater
                             NULL, // no lib name
                             NULL, // no alias name
                             SYNTH_AutoPropertyGet,
                             NULL, // no generic params
                             pOwningUnBindableSymbolList,
                             pGetMethod );


    if (hasLocation)
    {
        pGetMethod->SetLocation(pAutoProperty->GetLocation(), TRUE /* Location is inherited from the withevents variable */);
    }

    pSymbolAllocator->GetProperty( NULL,
                                  pAutoPropertyName,
                                  declarationFlags, 
                                  pGetMethod,
                                  pSetMethod,
                                  pAutoProperty,
                                  pOwningSymbolList ); // add this property to the list of symbols

}
/*========================================================================

Error routines

========================================================================*/

/*****************************************************************************
 ;ReportError

Add an error
*****************************************************************************/
void __cdecl Declared::ReportError
(
    ERRID errid, // the error code
    Location *location, // text location of the error
    ... // variable arguments
)
{
    if ( m_ErrorTable )
    {
        va_list ap;
        va_start( ap, location ); // Get stack pointer.
        // Defer to a helper.
        m_ErrorTable->CreateErrorArgs( errid, location, NULL, ap );
    }
}

/**************************************************************************************************
;ReportDeclFlagComboError

Reports the error that two specifiers can't be used together
***************************************************************************************************/
void Declared::ReportDeclFlagComboError
(
    DECLFLAGS DeclarationFlags, // [in] the two offending specifers to log an error about
    ParseTree::SpecifierList *Specifiers // [in] has the location information for the specifiers
)
{
    Location *SpecifierLocation;
    WCHAR *DeclFlagName1 = GetDeclFlagInfo( &DeclarationFlags, Specifiers, &SpecifierLocation ); // Note: DeclarationFlags is getting its bits stripped as GetDeclFlagInfo() handles each one
    WCHAR *DeclFlagName2 = GetDeclFlagInfo( &DeclarationFlags, Specifiers, NULL ); // Note: DeclarationFlags is getting its bits stripped as GetDeclFlagInfo() handles each one
    ReportError( ERRID_BadSpecifierCombo2, SpecifierLocation, DeclFlagName1, DeclFlagName2 );
}

/**************************************************************************************************
;ReportDeclFlagComboError

Reports the error that two specifiers can't be used together
***************************************************************************************************/
void Declared::ReportDeclFlagComboError
(
    DECLFLAGS DeclarationFlags, // [in] the two offending specifers to log an error about
    ParseTree::SpecifierList *Specifiers, // [in] has the location information for the specifiers
    DECLFLAGS SpecifierOnWhichToReportError
)
{
    Location *SpecifierLocation;

    WCHAR *DeclFlagName1 = GetDeclFlagInfo( &DeclarationFlags, Specifiers, &SpecifierLocation ); // Note: DeclarationFlags is getting its bits stripped as GetDeclFlagInfo() handles each one

    WCHAR *DeclFlagName2;
    if (~DeclarationFlags & SpecifierOnWhichToReportError)
    {
        DeclFlagName2 = GetDeclFlagInfo( &DeclarationFlags, Specifiers, NULL );
    }
    else
    {
        DeclFlagName2 = GetDeclFlagInfo( &DeclarationFlags, Specifiers, &SpecifierLocation );
    }

    ReportError( ERRID_BadSpecifierCombo2, SpecifierLocation, DeclFlagName1, DeclFlagName2 );
}

/*****************************************************************************
;ReportDeclFlagError

Adds an error for an invalid use of a declaration flag for each bit set in the
DeclarationFlags.
*****************************************************************************/
void Declared::ReportDeclFlagError
(
    ERRID ErrorId, // [in] the error code
    DECLFLAGS DeclarationFlags, // [in] the offending flags to log errors on
    ParseTree::SpecifierList *Specifiers, // [in] has the location information for the specifiers
    Location *AlternateLocation // [in-optional] where the error occured (only provide if you want to override the location info found for the specifier)
)
{
    while ( DeclarationFlags )
    {
        Location *SpecifierLocation;
        WCHAR *DeclFlagName = GetDeclFlagInfo( &DeclarationFlags, Specifiers, &SpecifierLocation ); // Note: DeclarationFlags is getting its bits stripped as GetDeclFlagInfo() handles each one
        ReportError( ErrorId, AlternateLocation ? AlternateLocation : SpecifierLocation, DeclFlagName );
    }
}

/*****************************************************************************
;GetDeclFlagInfo

Given a set of DECLF_xxxx flags, returns a string for one of the bits.  That
bit is then cleared from the DeclFlag that is passed in.  The idea is to call
this function repeatedly until the declFlag is 0 - each time getting a string
back for one of the bits on in DeclFlag
If you provide Specifiers and LocationInfo, then the location for the specifier
is also provided
*****************************************************************************/
WCHAR * // A text string representing one bit in a DECLFLAG
Declared::GetDeclFlagInfo(
    DECLFLAGS *DeclFlag, // [in/out] The declaration flag bits to return a string for / On return, the bit for which a string is returned is cleared
    ParseTree::SpecifierList *Specifiers, // [optional in] has the location information for the relevant specifiers
    Location **LocationInfo // [optional-out] The location for the specifier bit we generated a string for
)
{
    // There are instances where you may have a specifier but no LocationInfo ( DECLF_WithEvents, for instance )

    Location *DummyLocation = NULL;
    if ( !LocationInfo ) LocationInfo = &DummyLocation; // Do this so clients that don't care about location information don't have to send in dummies

    if ( *DeclFlag & DECLF_Private )
    {
        *DeclFlag &= ~DECLF_Private;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Private, Specifiers );
        return m_CompilerInstance->TokenToString( tkPRIVATE );
    }
    else if ( *DeclFlag & DECLF_ProtectedFriend )
    {
        *DeclFlag &= ~DECLF_ProtectedFriend;
        *LocationInfo = GetLocationOfSpecifier( DECLF_ProtectedFriend, Specifiers );
        VSASSERT((*LocationInfo), "This should never be NULL.");
        return m_CompilerInstance->ConcatStrings( m_CompilerInstance->TokenToString( tkPROTECTED ), L" ", m_CompilerInstance->TokenToString( tkFRIEND ));
    }
    else if ( *DeclFlag & DECLF_Protected )
    {
        *DeclFlag &= ~DECLF_Protected;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Protected, Specifiers );
        return m_CompilerInstance->TokenToString( tkPROTECTED );
    }
    else if ( *DeclFlag & DECLF_Public )
    {
        *DeclFlag &= ~DECLF_Public;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Public, Specifiers );
        return m_CompilerInstance->TokenToString( tkPUBLIC );
    }
    else if ( *DeclFlag & DECLF_Friend )
    {
        *DeclFlag &= ~DECLF_Friend;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Friend, Specifiers );
        return m_CompilerInstance->TokenToString( tkFRIEND );
    }
    else if ( *DeclFlag & DECLF_NotOverridable )
    {
        *DeclFlag &= ~DECLF_NotOverridable;
        *LocationInfo = GetLocationOfSpecifier( DECLF_NotOverridable, Specifiers );
        return m_CompilerInstance->TokenToString( tkNOTOVERRIDABLE );
    }
    else if ( *DeclFlag & DECLF_Overridable )
    {
        *DeclFlag &= ~DECLF_Overridable;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Overridable, Specifiers );
        return m_CompilerInstance->TokenToString( tkOVERRIDABLE );
    }
    else if ( *DeclFlag & DECLF_MustOverride )
    {
        *DeclFlag &= ~DECLF_MustOverride;
        *LocationInfo = GetLocationOfSpecifier( DECLF_MustOverride, Specifiers );
        return m_CompilerInstance->TokenToString( tkMUSTOVERRIDE );
    }
    else if ( *DeclFlag & DECLF_Static )
    {
        *DeclFlag &= ~DECLF_Static;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Static, Specifiers );
        return m_CompilerInstance->TokenToString( tkSTATIC );
    }
    else if ( *DeclFlag & DECLF_Shared )
    {
        *DeclFlag &= ~DECLF_Shared;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Shared, Specifiers );
        return m_CompilerInstance->TokenToString( tkSHARED );
    }
    else if ( *DeclFlag & DECLF_OverloadsKeywordUsed )
    {
        *DeclFlag &= ~DECLF_OverloadsKeywordUsed;
        *LocationInfo = GetLocationOfSpecifier( DECLF_OverloadsKeywordUsed, Specifiers );
        return m_CompilerInstance->TokenToString( tkOVERLOADS );
    }
    else if ( *DeclFlag & DECLF_OverridesKeywordUsed )
    {
        *DeclFlag &= ~DECLF_OverridesKeywordUsed;
        *LocationInfo = GetLocationOfSpecifier( DECLF_OverridesKeywordUsed, Specifiers );
        return m_CompilerInstance->TokenToString( tkOVERRIDES );
    }
    else if ( *DeclFlag & DECLF_WithEvents )
    {
        *DeclFlag &= ~DECLF_WithEvents;
        *LocationInfo = GetLocationOfSpecifier( DECLF_WithEvents, Specifiers );
        return m_CompilerInstance->TokenToString( tkWITHEVENTS );
    }
    else if ( *DeclFlag & DECLF_New )
    {
        *DeclFlag &= ~DECLF_New;

        // NOTE: Because New isn't a specifier, we have to jump through some hoops to get the location
        // information we need for it (it is in a really inconvienient spot in the trees with regards to how
        // we log specifier errors.)  So when a member is declared 'AsNew', the location of the 'new' is cached
        // during MapVarFlags() and then used here.  Note that this isn't thread safe but we only run the compiler on one thread
        *LocationInfo = &m_AsNewLocation;
        return m_CompilerInstance->TokenToString( tkNEW );
    }
    else if ( *DeclFlag & DECLF_Default )
    {
        *DeclFlag &= ~DECLF_Default;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Default, Specifiers );
        return m_CompilerInstance->TokenToString( tkDEFAULT );
    }
    else if ( *DeclFlag & DECLF_Dim )
    {
        *DeclFlag &= ~DECLF_Dim;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Dim, Specifiers );
        return m_CompilerInstance->TokenToString( tkDIM );
    }
    else if ( *DeclFlag & DECLF_Const )
    {
        *DeclFlag &= ~DECLF_Const;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Const, Specifiers );
        return m_CompilerInstance->TokenToString( tkCONST );
    }
    else if ( *DeclFlag & DECLF_ReadOnly )
    {
        *DeclFlag &= ~DECLF_ReadOnly;
        *LocationInfo = GetLocationOfSpecifier( DECLF_ReadOnly, Specifiers );
        return m_CompilerInstance->TokenToString( tkREADONLY );
    }
    else if ( *DeclFlag & DECLF_WriteOnly )
    {
        *DeclFlag &= ~DECLF_WriteOnly;
        *LocationInfo = GetLocationOfSpecifier( DECLF_WriteOnly, Specifiers );
        return m_CompilerInstance->TokenToString( tkWRITEONLY );
    }
    else if ( *DeclFlag & DECLF_Unicode )
    {
        *DeclFlag &= ~DECLF_Unicode;
        *LocationInfo = NULL; // no location info avail for this symbol
        return m_CompilerInstance->TokenToString( tkUNICODE );
    }
    else if ( *DeclFlag & DECLF_Ansi )
    {
        *DeclFlag &= ~DECLF_Ansi;
        *LocationInfo = NULL; // no location info avail for this symbol
        return m_CompilerInstance->TokenToString( tkANSI );
    }
    else if ( *DeclFlag & DECLF_Auto )
    {
        *DeclFlag &= ~DECLF_Auto;
        *LocationInfo = NULL; // no location info avail for this symbol
        return m_CompilerInstance->TokenToString( tkAUTO );
    }
    else if ( *DeclFlag & DECLF_HasRetval )
    {
        *LocationInfo = NULL; // no location info avail for this symbol
        *DeclFlag &= ~DECLF_HasRetval;
        return m_CompilerInstance->TokenToString( tkFUNCTION );
    }
    else if ( *DeclFlag & DECLF_MustInherit )
    {
        *DeclFlag &= ~DECLF_MustInherit;
        *LocationInfo = GetLocationOfSpecifier( DECLF_MustInherit, Specifiers );
        return m_CompilerInstance->TokenToString( tkMUSTINHERIT );
    }
    else if ( *DeclFlag & DECLF_NotInheritable )
    {
        *DeclFlag &= ~DECLF_NotInheritable;
        *LocationInfo = GetLocationOfSpecifier( DECLF_NotInheritable, Specifiers );
        return m_CompilerInstance->TokenToString( tkNOTINHERITABLE );
    }
    else if ( *DeclFlag & DECLF_Partial )
    {
        *DeclFlag &= ~DECLF_Partial;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Partial, Specifiers );
        return m_CompilerInstance->TokenToString( tkPARTIAL );
    }
    else if ( *DeclFlag & DECLF_ShadowsKeywordUsed )
    {
        *DeclFlag &= ~DECLF_ShadowsKeywordUsed;
        *LocationInfo = GetLocationOfSpecifier( DECLF_ShadowsKeywordUsed, Specifiers );
        return m_CompilerInstance->TokenToString( tkSHADOWS );
    }
    else if ( *DeclFlag & DECLF_Widening )
    {
        *DeclFlag &= ~DECLF_Widening;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Widening, Specifiers );
        return m_CompilerInstance->TokenToString( tkWIDENING );
    }
    else if ( *DeclFlag & DECLF_Narrowing )
    {
        *DeclFlag &= ~DECLF_Narrowing;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Narrowing, Specifiers );
        return m_CompilerInstance->TokenToString( tkNARROWING );
    }
    else if ( *DeclFlag & DECLF_CompilerControlled )
    {
        *DeclFlag &= ~DECLF_CompilerControlled;
        *LocationInfo = GetLocationOfSpecifier( DECLF_CompilerControlled, Specifiers );
        return m_CompilerInstance->AddString(WIDE("Compiler"));
    }
    else if ( *DeclFlag & DECLF_Async )
    {
        *DeclFlag &= ~DECLF_Async;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Async, Specifiers);
        return m_CompilerInstance->TokenToString( tkASYNC );
    }
    else if ( *DeclFlag & DECLF_Iterator )
    {
        *DeclFlag &= ~DECLF_Iterator;
        *LocationInfo = GetLocationOfSpecifier( DECLF_Iterator, Specifiers);
        return m_CompilerInstance->TokenToString( tkITERATOR );
    }
    else
    {
        VSFAIL( "Decl Flag not handled for this bit." );
        *DeclFlag = DECLF_NoFlags;
        *LocationInfo = NULL;
        return L"Internal compiler error";
    }
}

/**************************************************************************************************
;GetLocationOfSpecifier

Finds the location information for a given specifier
***************************************************************************************************/
Location *
Declared::GetLocationOfSpecifier(
    DECLFLAGS DeclFlag, // [in] The declaration flag specifying which location information we want
    ParseTree::SpecifierList *Specifiers // [in] has the location information for the specifiers
)
{
    if ( Specifiers )
    {
        Location *LocationOfProtected = NULL;
        Location *LocationOfFriend = NULL;

        for ( ParseTree::SpecifierList *CurrentSpecifier = Specifiers;
              CurrentSpecifier;
              CurrentSpecifier = CurrentSpecifier->Next )
        {
            Location *location = &(CurrentSpecifier->Element->TextSpan);

            switch ( CurrentSpecifier->Element->Opcode )
            {
                case ParseTree::Specifier::Private:
                {
                    if ( DeclFlag == DECLF_Private )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::ProtectedFriend:
                {
                    if ( DeclFlag == DECLF_ProtectedFriend )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Protected:
                {
                    if ( DeclFlag == DECLF_Protected )
                    {
                        return location;
                    }
                    else if ( DeclFlag == DECLF_ProtectedFriend )
                    {
                        if (LocationOfFriend)
                        {
                            Location *LocationOfProtectedFriend = new(m_ScratchAllocator) Location;

                            LocationOfProtectedFriend->SetLocation(
                                LocationOfFriend->m_lBegLine,
                                LocationOfFriend->m_lBegColumn,
                                location->m_lEndLine,
                                location->m_lEndColumn);

                            return LocationOfProtectedFriend;
                        }
                        else
                        {
                            LocationOfProtected = location;
                        }
                    }
                    continue;
                }
                case ParseTree::Specifier::Public:
                {
                    if ( DeclFlag == DECLF_Public )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Friend:
                {
                    if ( DeclFlag == DECLF_Friend )
                    {
                        return location;
                    }
                    else if ( DeclFlag == DECLF_ProtectedFriend )
                    {
                        if (LocationOfProtected)
                        {
                            Location *LocationOfProtectedFriend = new(m_ScratchAllocator) Location;

                            LocationOfProtectedFriend->SetLocation(
                                LocationOfProtected->m_lBegLine,
                                LocationOfProtected->m_lBegColumn,
                                location->m_lEndLine,
                                location->m_lEndColumn);

                            return LocationOfProtectedFriend;
                        }
                        else
                        {
                            LocationOfFriend = location;
                        }
                    }
                    continue;
                }
                case ParseTree::Specifier::NotOverridable:
                {
                    if ( DeclFlag == DECLF_NotOverridable )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Overridable:
                {
                    if ( DeclFlag == DECLF_Overridable )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::MustOverride:
                {
                    if ( DeclFlag == DECLF_MustOverride )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Static:
                {
                    if ( DeclFlag == DECLF_Static )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Shared:
                {
                    if ( DeclFlag == DECLF_Shared )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Overloads:
                {
                    if ( DeclFlag == DECLF_OverloadsKeywordUsed )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Overrides:
                {
                    if ( DeclFlag == DECLF_OverridesKeywordUsed )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Const:
                {
                    if ( DeclFlag == DECLF_Const )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::ReadOnly:
                {
                    if ( DeclFlag == DECLF_ReadOnly )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::WriteOnly:
                {
                    if ( DeclFlag == DECLF_WriteOnly )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Dim:
                {
                    if ( DeclFlag == DECLF_Dim )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Default:
                {
                    if ( DeclFlag == DECLF_Default )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::MustInherit:
                {
                    if ( DeclFlag == DECLF_MustInherit )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::NotInheritable:
                {
                    if ( DeclFlag == DECLF_NotInheritable )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Partial:
                {
                    if ( DeclFlag == DECLF_Partial )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Shadows:
                {
                    if ( DeclFlag == DECLF_ShadowsKeywordUsed )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::WithEvents:
                {
                    if ( DeclFlag == DECLF_WithEvents )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Widening:
                {
                    if ( DeclFlag == DECLF_Widening )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Narrowing:
                {
                    if ( DeclFlag == DECLF_Narrowing )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Async:
                {
                    if ( DeclFlag == DECLF_Async )
                    {
                        return location;
                    }
                    continue;
                }
                case ParseTree::Specifier::Iterator:
                {
                    if ( DeclFlag == DECLF_Iterator )
                    {
                        return location;
                    }
                    continue;
                }
                // No default intentional
            } // switch
        } // loop through specifier list
    } // if ( Specifiers )
    return NULL;
}

/*========================================================================

Routines to make locals and general variables

========================================================================*/

/*****************************************************************************
;MakeLocals

Builds local variable symbols from a parse tree.  Checks to make sure if we've
got a function that the variable isn't named the same as the func, etc.
Verifies that a duplicate local variables aren't defined.
*****************************************************************************/
void Declared::MakeLocals
(
    ParseTree::VariableDeclarationStatement *DeclarationTree, // [in] the tree for the variable declaration
    BCSYM_Proc *OwningProcedure, // [in] the procedure within which these local variables are defined
    BCSYM_Hash *LocalsHash, // [in/out] Locals discovered so far by semantics are in here.  We add those being built for the tree
    BCSYM_NamedRoot **CatchSymbol, // [out] hack: the catch variable is tacked on here if this is !NULL on entry
    bool CalledFromParseTreeService, // [in] whether we are being called from a possible CS_NoState situation
    bool PerformObsoleteCheck,
    bool lambdaMember
)
{
    // No tree, no locals.
    if ( !DeclarationTree || DeclarationTree->HasSyntaxError ) return;

    DECLFLAGS SpecifierFlagsForAll = MapSpecifierFlags( DeclarationTree->Specifiers );

    for ( ParseTree::VariableDeclarationList *DeclarationList = DeclarationTree->Declarations;
          DeclarationList;
          DeclarationList = DeclarationList->Next )
    {
        ParseTree::VariableDeclaration *Declaration = DeclarationList->Element;
        MakeLocalsFromDeclaration(
            DeclarationTree, 
            Declaration, 
            OwningProcedure, 
            LocalsHash, 
            CatchSymbol, 
            CalledFromParseTreeService,
            SpecifierFlagsForAll, 
            PerformObsoleteCheck,
            lambdaMember);
    }
}

void Declared::MakeLocalsFromDeclaration
(
    ParseTree::VariableDeclarationStatement *DeclarationTree, // [in] the tree for the variable declaration
    ParseTree::VariableDeclaration *Declaration, // [in] the declaration for the variable declaration
    BCSYM_Proc *OwningProcedure, // [in] the procedure within which these local variables are defined, can be NULL if creating locals for a lambda
    BCSYM_Hash *LocalsHash, // [in/out] Locals discovered so far by semantics are in here.  We add those being built for the tree
    BCSYM_NamedRoot **CatchSymbol, // [out] hack: the catch variable is tacked on here if this is !NULL on entry
    bool CalledFromParseTreeService, // [in] whether we are being called from a possible CS_NoState situation
    DECLFLAGS SpecifierFlagsForAll,   // [in] the flags for all the declaration statement this declaration is nested in
    bool PerformObsoleteCheck,
    bool lambdaMember
)
{
    ParseTree::Type *VariableType = Declaration->Type; // All variables in the list are of this type

    bool HasMultipleInitializers = false;
    if ( Declaration->Opcode == ParseTree::VariableDeclaration::WithInitializer && Declaration->Variables->Next )
    {   // This is really ----y because Semantics catches it for locals, MakeMemberVar() catches it on fields, but we have to catch it in
        // here ONLY for CONST declarations.  So we do some hocus pocus to log the error if we are working through a block of CONST declarations
        // that have multiple initializers but not for anything else.
        HasMultipleInitializers = true;
    }

    // Wind through each variable declarator in the list
    // Microsoft: consider - don't refigure the type out every time - only need to do it once.
    // The complication is the way MakeType works with arrays...
    VARIABLEKIND KindOfVariable = VAR_Local;
    for ( ParseTree::DeclaratorList *DeclaratorList = Declaration->Variables;
            DeclaratorList;
            DeclaratorList = DeclaratorList->Next )
    {
        ParseTree::Declarator *VariableDeclarator = DeclaratorList->Element;

        if ( VariableDeclarator->Name.IsBad ) continue; // this var was horked - move on to the next

        // Is the owning proc a Function and if so, did they name the local after the function?
        if (OwningProcedure && // OwningProcedure is null for lambdas, ok because lambdas do not have names
            OwningProcedure->IsProcedure() && // Don't consider properties
            OwningProcedure->GetRawType() && // functions provide a return type
            !OwningProcedure->IsUserDefinedOperatorMethod() && // Don't consider operators either
            StringPool::IsEqual( OwningProcedure->GetName(), VariableDeclarator->Name.Name ))
        {
            ReportError( ERRID_LocalSameAsFunc, &DeclaratorList->Element->TextSpan, DeclaratorList->Element->Name.Name );
            continue; // this var was horked - move on to the next
        }

        // SPECIFIERS

        Location *VarLocation = &VariableDeclarator->Name.TextSpan;
        DECLFLAGS SpecifiersForThisVar = SpecifierFlagsForAll | MapVarDeclFlags( VariableDeclarator, Declaration->Opcode == ParseTree::VariableDeclaration::WithNew ?
                                                                                    Declaration->AsNew() : NULL );
        // Allocate this variable.
        BCSYM_Variable *VariableSymbol;
        if ( VariableDeclarator->ArrayInfo && VariableDeclarator->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes ) // allocate array var
        {
            VariableSymbol = m_SymbolAllocator.AllocVariableWithArraySizes( VariableDeclarator->ArrayInfo->AsArrayWithSizes()->Rank );
        }
        else
        {
            VariableSymbol = m_SymbolAllocator.AllocVariable( true, // hasLocation
                                                                Declaration->Opcode == ParseTree::VariableDeclaration::WithInitializer ||
                                                                Declaration->Opcode == ParseTree::VariableDeclaration::WithNew ||
                                                                ( SpecifiersForThisVar & DECLF_Const )); // has a value
        }

        VariableSymbol->SetIsLambdaMember(lambdaMember);

        // Check for Duplicates

        bool NotADuplicate = TRUE;
        BCSYM_NamedRoot *PreviouslyDefinedLocal = LocalsHash->SimpleBind( VariableDeclarator->Name.Name );
     
        if ( PreviouslyDefinedLocal &&
                PreviouslyDefinedLocal->DigThroughAlias()->IsVariable() &&
                // duplicates statics caught in CreateBackingFieldsForStatics() because of block scoping issues
                !( SpecifiersForThisVar & DECLF_Static && PreviouslyDefinedLocal->DigThroughAlias()->IsStaticLocalBackingField()))
        {
            NotADuplicate = FALSE;
            // MakeLocalsFromParams() is called before we do MakeLocalsFromTrees() so we will have the symbols for the parameters in the hash
            // figure out if we collided with another local variable or with a parameter
            bool CollisionWithParam = PreviouslyDefinedLocal->DigThroughAlias()->PVariable()->GetVarkind() == VAR_Param ? true : false;
            
            ReportError( CollisionWithParam ? ERRID_LocalNamedSameAsParam1 : ERRID_DuplicateLocals1, VarLocation, VariableDeclarator->Name.Name );
            VariableSymbol->DigThroughAlias()->PNamedRoot()->SetIsBad( m_SourceFile );
        }
        else if (OwningProcedure && //Owning procedure null for lambdas, ok because lambdas cannot have generic params.
                 OwningProcedure->GetGenericParamsHash() &&
                 OwningProcedure->GetGenericParamsHash()->SimpleBind( VariableDeclarator->Name.Name ))
        {
            ReportError( ERRID_NameSameAsMethodTypeParam1, VarLocation, VariableDeclarator->Name.Name );
            VariableSymbol->DigThroughAlias()->PNamedRoot()->SetIsBad( m_SourceFile );
        }

        // Decl semantics for the local

        // Can't have static locals in Structures
        if ( SpecifiersForThisVar & DECLF_Static )
        {
            if (OwningProcedure == NULL)
            {
                ReportError(ERRID_StaticInLambda, &Declaration->TextSpan);
                continue;
            }
            
            if ( OwningProcedure->GetContainingClass()->IsStruct())
            {
                ReportDeclFlagError( ERRID_BadStaticLocalInStruct, DECLF_Static, DeclarationTree->Specifiers );
                SpecifiersForThisVar &= ~DECLF_Static;
                VariableSymbol->SetIsBad( m_SourceFile );
            }
            else if ( OwningProcedure->IsGeneric())
            {
                ReportDeclFlagError( ERRID_BadStaticLocalInGenericMethod, DECLF_Static, DeclarationTree->Specifiers );
                SpecifiersForThisVar &= ~DECLF_Static;
                VariableSymbol->SetIsBad( m_SourceFile );
            }
        }

        if ( NotADuplicate )
        {
            // Check errors on CONST variables
            if ( SpecifiersForThisVar & DECLF_Const )
            {
                ConstantVarSemantics( DeclarationTree,
                                    VariableDeclarator,
                                    Declaration,
                                    true, // this is for a local
                                    &SpecifiersForThisVar,
                                    VariableSymbol );
                KindOfVariable = VAR_Const;


                // We only want to log the multiple initializer error if we have a constant block with this problem
                // Semantics catches it for locals, MakeMemberVar() catches it on fields, but we have to catch it just on Const declarations
                if ( HasMultipleInitializers )
                {
                    ReportError( ERRID_InitWithMultipleDeclarators, &Declaration->TextSpan );
                    HasMultipleInitializers = false; // clear so we don't log repeatedly
                }
            }
            else // A straight dim var or static var statement
            {
                KindOfVariable = SpecifiersForThisVar & DECLF_Static ? VAR_Member : VAR_Local;

                if ( SpecifiersForThisVar & ( ~DECLF_ValidLocalDimFlags ))
                {
                    ReportDeclFlagError( ERRID_BadLocalDimFlags1, SpecifiersForThisVar & (~DECLF_ValidLocalDimFlags ), DeclarationTree->Specifiers );
                    SpecifiersForThisVar &= DECLF_ValidLocalDimFlags;
                }

                // Variables allocated as New shouldn't be arrays
                if ( VariableDeclarator->ArrayInfo && SpecifiersForThisVar & DECLF_New )
                {
                    ReportDeclFlagError( ERRID_AsNewArray, DECLF_New, DeclarationTree->Specifiers );
                    SpecifiersForThisVar &= ~DECLF_New;
                    VariableSymbol->SetIsBad( m_SourceFile );
                }
            }
        } // if ( NotADuplicate )

        // We always want to build a symbol (except for the var named same as function error) so that semantics won't log errors on things like:
        // dim a as string, a as long: a = 42 <-- if a bad symbol for a as long exists, no semantic error generated about assign type mismatch

        // Locals are always public. So remove any other access if specified and mark as public.
        SpecifiersForThisVar &= ~DECLF_AccessFlags;
        SpecifiersForThisVar |= DECLF_Public;

        // get rid of any error causing flags from the common flags so that we report
        // the errors on these only once although multiple variables are declared in
        // the same declaration statement
        //
        SpecifierFlagsForAll &= SpecifiersForThisVar;

        // Fill in the variable symbol
        SymbolList OwningSymbolList;
        if ( SpecifiersForThisVar & DECLF_Static &&
                !VariableSymbol->IsBad() &&
                !CalledFromParseTreeService ) // We only have the backing fields if we are coming up through declared normally - we can get called in NOSTATE from the parsetree service
        {
            if (OwningProcedure == NULL)
            {
                ReportError(ERRID_StaticInLambda, &Declaration->TextSpan);
                continue;
            }
            MakeAliasToStaticVar( VariableDeclarator, Declaration, SpecifiersForThisVar, OwningProcedure, &OwningSymbolList, PerformObsoleteCheck);
        }
        else
        {
            MakeVariable( DeclarationTree->Attributes, VariableDeclarator, Declaration, SpecifiersForThisVar, KindOfVariable, true, VariableSymbol, &OwningSymbolList );
        }

        // Add the variable to the hash of locals (add it now so we can do dupe checking against it as we iterate through the locals to build)
        VSASSERT( OwningSymbolList.GetCount() == 1, "Just making sure there is only one guy on the list" );
        if ( CatchSymbol )
        { // this is hokey - the only time we are called with this set is when there is a single element in the list by MakeLocalFromCatch(), but
            // if there ever were this would break.
            *CatchSymbol = OwningSymbolList.GetFirst();
        }
        Symbols::AddSymbolListToHash( LocalsHash, &OwningSymbolList, true /* set parent */ );
    } // loop variables in declarator list
}

/*****************************************************************************
;MakeVariable

Fill in the guts of a variable declaration. The caller should have already
verified that the flags on this variable are correct and have allocated the
correct kind of variable.
*****************************************************************************/
void Declared::MakeVariable
(
    ParseTree::AttributeSpecifierList *Attributes, // [in] the tree for the attributes of the declaration statement
    ParseTree::Declarator *VariableTree, // [in] the tree for the variable symbol
    ParseTree::VariableDeclaration *Declaration, // [in] contains the type information, etc. for the variable
    DECLFLAGS VariableFlags, // [in] declaration flags for the variable symbol
    VARIABLEKIND VariableKind, // [in] whether this is a const variable, member var, param, etc.
    bool IsLocal, // [in] is this a local variable?
    BCSYM_Variable *Variable, // [in] the pre-allocated symbol for the variable we are building
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    VSASSERT( !VariableTree->Name.IsBad, "Can't build a variable symbol from a bad tree." );

    BCSYM_Expression *InitializerExpression = NULL;
    ParseTree::IdentifierDescriptor *NameDescriptor = &VariableTree->Name;
    STRING *VarName = NameDescriptor->Name;

    Variable->SetHasNullableDecoration(NameDescriptor->IsNullable);

    // Is this variable typed via a type character? A$, for instance, would be a string
    if ( !Declaration->Type && NameDescriptor->TypeCharacter == chType_NONE )
    {
        VariableFlags |= DECLF_NotTyped;
    }

    // Is this variable declared with brackets?
    if ( NameDescriptor->IsBracketed == true )
    {
        VariableFlags |= DECLF_Bracketed;
    }
    BCSYM *VarType;
    if ((VariableTree->ArrayInfo && Declaration->Type && Declaration->Type->Opcode == ParseTree::Type::Nullable) ||
        (NameDescriptor->IsNullable && Declaration->Type &&
        (   Declaration->Type->Opcode == ParseTree::Type::ArrayWithSizes ||
            Declaration->Type->Opcode == ParseTree::Type::ArrayWithoutSizes ||
            Declaration->Opcode == ParseTree::VariableDeclaration::WithNew )))
    {
        ReportError( Declaration->Opcode == ParseTree::VariableDeclaration::WithNew ?
                        ERRID_CantSpecifyAsNewAndNullable :
                        ERRID_CantSpecifyArrayAndNullableOnBoth,
                     &Declaration->TextSpan);
        VarType = m_SymbolAllocator.GetGenericBadNamedRoot();
    }
    else
    {
        // Generate the type symbol for this variable, e.g. given "dim Decimal as CommandButton", generate the type for CommandButton
        VarType = MakeType( Declaration->Type, NameDescriptor, Variable, GetTypeList(), VariableFlags, false, VariableTree->IsForControlVarDeclaration);
    }

    // Dev10#489103: in Orcas, if VarType was a NamedType pointing to a bad thing, then IsBad used to return FALSE!!!
    // so we'd go ahead and make an array of elements of type "NamedTypeThatPointsToSomethingBad".
    // That seems fraught with danger. So in Dev10 we don't do that any more.
    if ( !VarType->IsBad() && VariableTree->ArrayInfo )
    {
        if ( Declaration->Type &&
             ( Declaration->Type->Opcode == ParseTree::Type::ArrayWithSizes ||
               Declaration->Type->Opcode == ParseTree::Type::ArrayWithoutSizes ))
        {
            ReportError( ERRID_CantSpecifyArraysOnBoth, &Declaration->Type->TextSpan );
            VarType = m_SymbolAllocator.GetGenericBadNamedRoot();
        }
        else
        {
            VarType = MakeArrayType(VariableTree->ArrayInfo, VarType);
        }
    }

    bool IgnoreInitializer = VariableKind == VAR_Local || Variable->IsStaticLocalBackingField();

    // Get the initializer value, if we have one.
    if ( Declaration->Opcode == ParseTree::VariableDeclaration::WithInitializer &&
         Declaration->AsInitializer()->InitialValue->Opcode == ParseTree::Initializer::Deferred )
    {
        ParseTree::DeferredInitializer *DeferredInit = Declaration->AsInitializer()->InitialValue->AsDeferred();

        if ( VariableFlags & DECLF_Const )
        {
            if ( DeferredInit->Value->Opcode == ParseTree::Initializer::Expression )
            {
                ParseTree::Expression *InitExpr = DeferredInit->Value->AsExpression()->Value;

                if ( ParseTree::Expression::ArrayInitializer == InitExpr->Opcode ||
                     ParseTree::Expression::NewArrayInitializer == InitExpr->Opcode )
                {
                    ReportError( ERRID_ConstAggregate, &InitExpr->TextSpan );
                }
                else
                {
                    MakeConstantExpression( InitExpr,
                                            false, // not pre-evaluated
                                            !( VariableFlags & DECLF_NotTyped ), // whether to explicitly type it
                                            Variable,
                                            &VarType, // the built type goes here
                                            &InitializerExpression ); // the built expression symbol goes here
                }
            }
        }
        else if ( !IgnoreInitializer )
        {
            // Encode the text of the entire initializer. Though an initializer is not, strictly speaking, an "expression",
            // the expression symbol mechanism works to supply the initializer to semantic analysis.

            // Make sure that bindable does not find the expression symbol as a constant to evaluate--don't put in on the list that
            // bindable traverses. There are several reasons for this:
            //      1) An aggregate initializer is not an an expression
            //      2) If the initial value of a variable is an evaluated expression, semantic analysis will assume that it
            //         is something other than an initializer.
            // 

            AssertIfFalse(ParseTree::Initializer::Expression == DeferredInit->Value->Opcode);

            InitializerExpression = m_SymbolAllocator.GetConstantExpression(
                    &DeferredInit->Value->AsExpression()->Value->TextSpan,
                    Variable, // current context
                    NULL, NULL, // head and tail of expression list -- don't want this symbol on the list of constants to evaluate
                    NULL, // no expression string
                    DeferredInit->Text, // initializer text
                    DeferredInit->TextLengthInCharacters );
        }
    }
    else if ( Declaration->Opcode == ParseTree::VariableDeclaration::WithNew &&
              Declaration->AsNew()->DeferredInitializerText )
    {
        if ( !IgnoreInitializer )
        {
            // See the explanation above about deferring initializers.

            InitializerExpression = 
                BuildNewDeferredInitializerExpression
                (
                    Variable, 
                    Declaration->AsNew()->DeferredInitializerText, 
                    Declaration->AsNew()->ObjectInitializer,
                    Declaration->AsNew()->Arguments,
                    Declaration->AsNew()->CollectionInitializer,
                    Declaration->TextSpan,
                    &Declaration->AsNew()->From
                );
        }
    }
    else if ( VariableTree->ArrayInfo &&
              VariableTree->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes &&
              !IgnoreInitializer &&
              Variable->IsVariableWithArraySizes())
    {
        // Fill in the array sizes.

        BCSYM *SizeType = m_CompilerHost->GetFXSymbolProvider()->GetType(t_i4);
        unsigned Dimension = 0;

        for ( ParseTree::ArrayDimList *Dims = VariableTree->ArrayInfo->AsArrayWithSizes()->Dims;
              Dims;
              Dims = Dims->Next )
        {
            BCSYM_Expression *SizeExpression = NULL;

            // Encode the text of the entire initializer. Though an initializer is not, strictly speaking, an "expression",
            // the expression symbol mechanism works to supply the initializer to semantic analysis.

            // Make sure that bindable does not find the expression symbol as a constant to evaluate--don't put in on the list that
            // bindable traverses. There are several reasons for this:
            //      1) An aggregate initializer is not an an expression
            //      2) If the initial value of a variable is an evaluated expression, semantic analysis will assume that it
            //         is something other than an initializer.
            // 

            if (Dims->Element && Dims->Element->upperBound->Opcode == ParseTree::Expression::Deferred)
            {
                ParseTree::DeferredExpression *DeferredBoundInit = Dims->Element->upperBound->AsDeferred();

                SizeExpression =
                    m_SymbolAllocator.GetConstantExpression(
                        &(DeferredBoundInit->Value->TextSpan),
                        Variable, // current context
                        NULL, NULL, // head and tail of expression list -- don't want this symbol on the list of constants to evaluate
                        NULL, // no expression string
                        DeferredBoundInit->Text, // initializer text
                        DeferredBoundInit->TextLengthInCharacters );

                SizeExpression->SetForcedType(SizeType);
            }
            else
            {
                SizeExpression = NULL;
            }

            Variable->PVariableWithArraySizes()->SetArraySize(Dimension, SizeExpression);
            Dimension++;
        }
    }

    //
    // Fill out the variable symbol
    //

    m_SymbolAllocator.GetVariable( &NameDescriptor->TextSpan,
                                   VarName,
                                   VarName, // EmittedName is the same as the var name,
                                   VariableFlags,
                                   VariableKind,
                                   VarType,
                                   InitializerExpression,
                                   OwningSymbolList,
                                   Variable);


    if ( IsLocal && Attributes )
    {
        // Locals can't have custom attributes.
        ReportError( ERRID_LocalsCannotHaveAttributes, &VariableTree->Name.TextSpan );
    }
    else
    {
        Variable->StowAttributeTree( Attributes ); // hang on to these so we can put them on the synthetic code we generate for events
        // Store away the custom attributes.
        ProcessAttributeList( Attributes, Variable );
    }
}

BCSYM_Expression *
Declared::BuildNewDeferredInitializerExpression
(
    _Inout_ BCSYM_NamedRoot* pContext,
    _In_ ParseTree::DeferredText *pDeferredInitializerText,
    _In_opt_ ParseTree::ObjectInitializerList *pObjectInitializer,
    ParseTree::ParenthesizedArgumentList arguments,
    _In_opt_ ParseTree::BracedInitializerList * pCollectionInitializer,
    Location textSpan, // This is the location of the entire expression
    _In_opt_ ParseTree::PunctuatorLocation *FromLoc // the punctuator location of the FROM token (if present)
)
{
    BCSYM_Expression *pInitializerExpression = NULL;

    // See the explanation above about deferring initializers.

    Location deferredInitializerTextSpan;
    deferredInitializerTextSpan.Invalidate();

    // Start location
    if (pObjectInitializer && !arguments.Values)
    {
        // used for object initializers, start at the "With"
        deferredInitializerTextSpan.m_lBegLine = pObjectInitializer->With.m_lBegLine;
        deferredInitializerTextSpan.m_lBegColumn = pObjectInitializer->With.m_lBegColumn;
    }
    else if (!arguments.TextSpan.IsInvalid())
    {
        // used for normal arguments "new Fred(1,2,3)" starts at the "(1,2,3)"
        // also used for collection initializers "new Fred() From {x}", again starts at the "()"
        deferredInitializerTextSpan.m_lBegLine = arguments.TextSpan.m_lBegLine;
        deferredInitializerTextSpan.m_lBegColumn = arguments.TextSpan.m_lBegColumn;
    }
    else if (pCollectionInitializer && FromLoc != NULL && 
             FromLoc->Column != 0 || FromLoc->Line !=0 ) // Make sure FromLoc is valid
    {
        /* Dev10_630929 
        Collection initializers are stored as a deferred initializer.  Given an expression like: 
        
            Dim x As New System.Collections.Generic.List(Of Foo) From {bar}
           
        the deferred expression text is going to be: "From {bar}"  But sadly the location information 
        we have been given is for the entire expression, e.g. "x as New ... From {bar}" So we need to
        figure out where the "From {bar}" text actually starts.
        Fortunately, we have the FROM punctuator which marks where the deferred text we will have 
        captured actually starts.  Remembering that a punctuator provides an absolute column, but a 
        line relative to the expression that contains it, we calculate our deferredInitializerTextSpan 
        to reflect where the "From {bar}" portion of the expression starts */
        deferredInitializerTextSpan.m_lBegLine = textSpan.m_lBegLine + FromLoc->Line; // Punctuator lines are an offset (rather than absolute) so we add it
        deferredInitializerTextSpan.m_lBegColumn = FromLoc->Column;
    }
    else
    {
        deferredInitializerTextSpan = textSpan;
    }

    // End location
    if (pObjectInitializer)
    {
        if (pObjectInitializer->BracedInitializerList)
        {
            deferredInitializerTextSpan.m_lEndLine = pObjectInitializer->BracedInitializerList->TextSpan.m_lEndLine;
            deferredInitializerTextSpan.m_lEndColumn = pObjectInitializer->BracedInitializerList->TextSpan.m_lEndColumn;
        }
        else
        {
            deferredInitializerTextSpan.m_lEndLine = pObjectInitializer->With.m_lEndLine;
            deferredInitializerTextSpan.m_lEndColumn = pObjectInitializer->With.m_lEndColumn;
        }
    }
    else if (pCollectionInitializer)
    {
        deferredInitializerTextSpan.m_lEndLine = pCollectionInitializer->TextSpan.m_lEndLine;
        deferredInitializerTextSpan.m_lEndColumn = pCollectionInitializer->TextSpan.m_lEndColumn;
    }
    else if (arguments.Values)
    {
         deferredInitializerTextSpan.m_lEndLine = arguments.TextSpan.m_lEndLine;
         deferredInitializerTextSpan.m_lEndColumn = arguments.TextSpan.m_lEndColumn;
    }

    pInitializerExpression = m_SymbolAllocator.GetConstantExpression(
            &deferredInitializerTextSpan,
            pContext, // current context
            NULL, NULL, // head and tail of expression list -- don't want this symbol on the list of constants to evaluate
            NULL, // no STRING * expression string
            pDeferredInitializerText->Text, // arguments text
            pDeferredInitializerText->TextLengthInCharacters );
    
    return pInitializerExpression;
}
/*****************************************************************************
;PrependToNamespaceList

Builds a NamespaceNameList node and prepends it to the supplied list
*****************************************************************************/
void Declared::PrependToNamespaceList(
    NamespaceNameList **List,    // [in] the head of the list we want to prepend to
    _In_z_ STRING *Name,        // [in] the name of this namespace node
    Location * pLocation        // [in] the location of the name
)
{
    *List = new (m_ScratchAllocator) NamespaceNameList(Name, pLocation, *List);
}

/*****************************************************************************
;BuildNamespaceList

Given a ParseTree for a name, builds a list, in order, of namespaces we
need to build it.  E.g. given a parse tree for "A.B.C", this creates the
namespace list: A,B,C
*****************************************************************************/
void Declared::BuildNamespaceList(
    ParseTree::Name *NameTree,                // [in] parse tree for the qualified namespace name
    NamespaceNameList **NamespacesToMake,    // [in/out] We prepend the list of namespaces to build to this list
    bool isDefaultNamespace                    // Is this the default namespace for the project.  
)
{
    CheckForTypeCharacter( NameTree ); // Can't have type characters in namespace names

    STRING *NamespaceName;
    Location *pLocation;

    // The parse tree for a name comes backwards, e.g. A.B.C comes as C, B, A.  Reverse it.
    while ( NameTree->IsQualified() )
    {
        ParseTree::QualifiedName * pQualified = NameTree->AsQualified();
        pLocation = isDefaultNamespace 
            ? NULL 
            : &pQualified->Qualifier.TextSpan;
        NamespaceName = pQualified->Qualifier.IsBad ? STRING_CONST(m_CompilerInstance, BadNamespace) : pQualified->Qualifier.Name;
        // Add this guy to the front of the list of namespaces to build
        PrependToNamespaceList( NamespacesToMake, NamespaceName, pLocation );
        NameTree = NameTree->AsQualified()->Base;
    }

    switch ( NameTree->Opcode )
    {
    case ParseTree::Name::Simple:
    case ParseTree::Name::SimpleWithArguments:
        {
            ParseTree::SimpleName *Simple = NameTree->AsSimple();

            // If the name is bad, punt processing the construct.
            NamespaceName = Simple->ID.IsBad ? STRING_CONST( m_CompilerInstance, BadNamespace ) :
                                                             Simple->ID.Name;
            pLocation = isDefaultNamespace 
                ? NULL 
                : &Simple->ID.TextSpan;

            PrependToNamespaceList( NamespacesToMake, NamespaceName, pLocation );
        }
        break;

    case ParseTree::Name::GlobalNameSpace:
        // The global (unnamed) namespace
        PrependToNamespaceList( NamespacesToMake, STRING_CONST( m_CompilerInstance, EmptyString ), isDefaultNamespace ? NULL : &NameTree->TextSpan );
        (*NamespacesToMake)->Global = true;
        break;

    default:
        VSASSERT ( false, "Unexpected namespace name. Need Global or Simple" );
    }
}

/*****************************************************************************
;LookupInLinkedNamespaces

Determine if a symbol is defined in any of the namespaces of a particular name
*****************************************************************************/
BCSYM_NamedRoot * // the symbol by the same name as SymbolToLookFor in Namespace
Declared::LookupInLinkedNamespaces(
    BCSYM_NamedRoot *SymbolToLookFor, // [in] we wan't to know if a symbol by this name is already defined in Namespace
    BCSYM_Namespace *Namespace // [in] the namespace (and all its linked namespaces) to check for a previous definition
)
{
    VSASSERT( Namespace != NULL, "Must be called with a valid namespace" );

    BCSYM_Namespace *CurrentNamespace = Namespace;
    do
    {
        if ( CurrentNamespace->IsBasic())
        {
            for ( BCSYM_NamedRoot *Dupe = CurrentNamespace->SimpleBind( NULL, SymbolToLookFor->GetName());
                  Dupe;
                  Dupe = Dupe->GetNextOfSameName())
            {
                if ( !Dupe->IsBad() &&
                     Dupe != SymbolToLookFor /* if you found yourself this isn't a dupe */ )
                {
                    return Dupe;
                }
            }
        }
        CurrentNamespace = CurrentNamespace->GetNextNamespace();
    } while ( CurrentNamespace != Namespace ); // we know we've visited all the namespaces by this name when we loop back to where we started
    return NULL;
}

/**************************************************************************************************
;CreateBackingFieldsForStatics

Static variables are implemented by creating backing fields on the class and then aliasing the local
variables to those fields.  At this point, we are building MethodImpls and we have a parse tree for
the static locals in the method.  We go through the statics and create the backing fields here.

I postpone semantics for these things until MakeLocals() when the statics come through during method
body compilation and we are hooking up the fields.  There are a lot of similarities between this
function, MakeLocals(), and MakeMemberVar().  It would probably be worthwhile sometime to see what
commonality could be factored out.
***************************************************************************************************/
void Declared::CreateBackingFieldsForStatics(
    ParseTree::StatementList *StaticLocalDeclarations, // [in] the static locals found within OwningProc
    BCSYM_Proc *OwningProc, // [in] the proc where the static local was defined
    DECLFLAGS ClassFlags, // [in] the flags on the class that owns the method symbol
    bool &NeedSharedConstructor,  // [out] whether a shared constructor is needed
    SymbolList *ListOfBackingFields // [in] the backing fields are put on this list
)
{
    NorlsAllocator ScratchAllocator( NORLSLOC);
    HashTable<16> StaticsSoFar( &ScratchAllocator );
    while ( StaticLocalDeclarations )
    {
        ParseTree::VariableDeclarationStatement *StaticsDeclarationTree = StaticLocalDeclarations->Element->AsVariableDeclaration();

        for ( ParseTree::VariableDeclarationList *StaticDeclarationList = StaticsDeclarationTree->Declarations;
              StaticDeclarationList;
              StaticDeclarationList = StaticDeclarationList->Next )
        {
            ParseTree::VariableDeclaration *StaticDeclaration = StaticDeclarationList->Element;
            ParseTree::Type *VariableType = StaticDeclaration->Type; // All statics in the current list are of this type

            // The parser does not report syntax errors for declarations of static locals
            // until it is parsing method bodies. Therefore, it is not sound to quietly
            // make anything bad here--no errors will have been generated before metadata
            // generation runs. Therefore, if there is a syntax error in the type don't
            // make any symbols.
            // DDB 128554: check the decl for errors, because generic type argument errors don't surface
            // to the top (see the code in FindEndProcs that handle statics).

            if ( (VariableType && VariableType->Opcode == ParseTree::Type::SyntaxError) ||
                 StaticDeclaration->HasSyntaxError)
            {
                continue;
            }

            // Wind through each static variable in the list
            // Microsoft: consider changing it so that we don't refigure the type out every time - only need to do it once.
            // The complication is the way MakeType works with arrays and type characters.
            for ( ParseTree::DeclaratorList *DeclaratorList = StaticDeclaration->Variables;
                  DeclaratorList;
                  DeclaratorList = DeclaratorList->Next )
            {
                ParseTree::Declarator *Declarator = DeclaratorList->Element;

                if ( Declarator->Name.IsBad ) continue; // this var was horked - move on to the next

                // You can't block scope static locals of the same name - only one static local of a given name allowed per function because we don't have
                // a way for the debugging team to determine which backing field goes with which scope, etc.
                STRING_INFO *StringInfo = StringPool::Pstrinfo( Declarator->Name.Name );
                if ( StaticsSoFar.Find( StringInfo ))
                {
                    ReportError( ERRID_DuplicateLocalStatic1, &Declarator->Name.TextSpan, Declarator->Name.Name );
                    continue; // don't build the field for this static
                }
                StaticsSoFar.Add( StringInfo, StringInfo /* it doesn't matter what we add - the template requires something so I'll send this */ );

                // Allocate a class field for this static variable
                BCSYM_StaticLocalBackingField *Variable = m_SymbolAllocator.AllocStaticLocalBackingField(true);

                DECLFLAGS VarFlags = DECLF_Hidden | DECLF_Private;
                // If statics are in shared procedures or in shared classes, they are shared
                if ( OwningProc->IsShared() || ( ClassFlags & DECLF_Module ))
                {
                    VarFlags |= DECLF_Shared;
                }

                MakeVariable( StaticsDeclarationTree->Attributes, // tree for the attributes
                              Declarator, // tree for the var symbol
                              StaticDeclaration, // tree containing type info
                              VarFlags,
                              VAR_Member,
                              false, // not a local var (it's a field)
                              Variable, // The symbol
                              ListOfBackingFields ); // attach the built variable to this list

                Variable->SetNew(StaticDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew);
                Variable->SetProcDefiningStatic( OwningProc );

                // If the static local needs initialization, allocate a corresponding init flag field for the backing field.

                if ( StaticDeclaration->Opcode == ParseTree::VariableDeclaration::WithInitializer ||
                     StaticDeclaration->Opcode == ParseTree::VariableDeclaration::WithNew ||
                     ( Declarator->ArrayInfo && Declarator->ArrayInfo->Opcode == ParseTree::Type::ArrayWithSizes ))
                {
                    if (VarFlags & DECLF_Shared)
                    {
                        NeedSharedConstructor = true;
                    }

                    BCSYM_StaticLocalBackingField *InitFlag = m_SymbolAllocator.AllocStaticLocalBackingField(false);
                    STRING *InitFlagName = m_CompilerInstance->ConcatStrings( Declarator->Name.Name, STRING_CONST( m_CompilerInstance, BackingFieldInitFlag ));

                    // Synthesize a symbol for the named type "Microsoft.VisualBasic.CompilerServices.StaticLocalInitFlag" which is the type of the init flag.

                    BCSYM_NamedType *InitFlagType = m_SymbolAllocator.GetNamedType( &Declarator->TextSpan,
                                                                                    InitFlag,
                                                                                    4,
                                                                                    GetTypeList());
                    InitFlagType->SetIsGlobalNameSpaceBased(true);
                    STRING **InitFlagTypeNames = InitFlagType->GetNameArray();
                    InitFlagTypeNames[0] = STRING_CONST(m_CompilerInstance, Microsoft);
                    InitFlagTypeNames[1] = STRING_CONST(m_CompilerInstance, VisualBasic);
                    InitFlagTypeNames[2] = STRING_CONST(m_CompilerInstance, CompilerServices);
                    InitFlagTypeNames[3] = STRING_CONST(m_CompilerInstance, BackingFieldInitFlagClass);

                    m_SymbolAllocator.GetVariable( NULL,
                                                   InitFlagName,
                                                   InitFlagName, // EmittedName is the same as the var name,
                                                   VarFlags,
                                                   VAR_Member,
                                                   InitFlagType,
                                                   NULL,
                                                   ListOfBackingFields,
                                                   InitFlag);

                    // The init flags are treated as backing fields so that metaemit will decorate their names.

                    InitFlag->SetProcDefiningStatic( OwningProc );
                    Variable->SetInitFlag( InitFlag );
                }
            }
        }
        StaticLocalDeclarations = StaticLocalDeclarations->NextInBlock;
    }
}

/**************************************************************************************************
;MakeAliasToStaticVar

When Semantics calls us to build the local variables, as we come across Static variables, we need
to find the backing field on the class that represents the static variable (built during MakeMethodImpl())
and create an alias for the static variable that points to the backing field
***************************************************************************************************/
void
Declared::MakeAliasToStaticVar(
    ParseTree::Declarator *VariableTree, // [in] the tree for the variable symbol
    ParseTree::VariableDeclaration *Declaration, // [in] contains the type information, etc. for the variable
    DECLFLAGS SpecifiersForThisVar, // [in] specifiers for the static variable
    BCSYM_Proc *OwningProcedure, // [in] the procedure in which the static is defined
    SymbolList *OwningSymbolList, // [in] symbol list to add alias to
    bool PerformObsoleteCheck
)
{
    BCSYM_Class *OwningClass = OwningProcedure->GetContainer()->PClass();
    ParseTree::IdentifierDescriptor *NameDescriptor = &VariableTree->Name;
    STRING *VarName = NameDescriptor->Name;

    BCSYM_Hash *BackingFieldsHash = OwningClass->GetBackingFieldsForStatics();
    VSASSERT( BackingFieldsHash != NULL, "How did we get a static local that we didn't previously build a hash for?");

    BCSYM_NamedRoot *BackingField = BackingFieldsHash->SimpleBind( VarName );

    while ( BackingField )
    {
        // 


        if ( BCSYM::CompareProcs( OwningProcedure, (GenericBinding *)NULL, BackingField->PStaticLocalBackingField()->GetProcDefiningStatic(), (GenericBinding *)NULL, &m_SymbolAllocator) == EQ_Match )
        {
            break;
        }
        BackingField = BackingField->GetNextOfSameName();
    }

    VSASSERT( BackingField != NULL, "How did we get a static local that we didn't previously build a backing field for?");

    // This static variable is just going to be an alias to the field
    BCSYM_Alias *AliasToBackingField = m_SymbolAllocator.GetAlias( VarName, SpecifiersForThisVar, BackingField, OwningSymbolList ); // built Alias attached to OwningSymbolList

    // Is the static local's type obsolete?
    // NOTE:  we check here because Static Local type resolution follows a different code path than other local vars.
    //
    if (!BackingField->IsBad() && PerformObsoleteCheck)
    {
        ObsoleteChecker::CheckObsolete(
            BackingField->PMember()->GetType()->ChaseToType(),
            &Declaration->Type->TextSpan,
            BackingField->GetContainer(),
            BackingField->GetContainer(),
            m_ErrorTable,
            m_SourceFile,
            OwningProcedure);
    }

    // check for restricted types. no fields of restricted type.
    CheckRestrictedType(
            ERRID_RestrictedType1,
            BackingField->PMember()->GetType()->ChaseToType(),
            &Declaration->Type->TextSpan,
            m_SourceFile->GetCompilerHost(),
            m_ErrorTable);

}


/**************************************************************************************************
;ConstantVarSemantics

Does the declaration semantics for the definition of a single Constant variable
***************************************************************************************************/
void Declared::ConstantVarSemantics(
    ParseTree::VariableDeclarationStatement *VariableDeclarationStatement, // [in] needed for specifiers
    ParseTree::Declarator *Declarator, // [in] to detect if this is an Array type
    ParseTree::VariableDeclaration *VariableDeclaration, // [in] needed for the type of the thing
    bool ForLocal, // [in] true - this is a const local variable. false - this is a const field
    DECLFLAGS *SpecifiersForThisVar, // [in/out]
    BCSYM_Variable *VariableSymbol // [in/out] the
)
{
    ERRID ErrorToLog;
    DECLFLAGS LegalSpecifiersForThisVar;
    if ( ForLocal )
    {
        LegalSpecifiersForThisVar = DECLF_ValidLocalConstFlags;
        ErrorToLog = ERRID_BadLocalConstFlags1;
    }
    else
    {
        LegalSpecifiersForThisVar = DECLF_ValidConstFlags;
        ErrorToLog = ERRID_BadConstFlags1;
    }

    if ( VariableDeclaration->Type )
    {
         if ( VariableDeclaration->Type->Opcode == ParseTree::Type::ArrayWithoutSizes ||
              VariableDeclaration->Type->Opcode == ParseTree::Type::ArrayWithSizes )
        {
            ReportError( ERRID_ConstAsNonConstant, &Declarator->Name.TextSpan, Declarator->Name.Name );
            VariableSymbol->SetIsBad( m_SourceFile );
        }
    }

    if ( Declarator->ArrayInfo ) // Arrays can't be constant.
    {
        ReportError( ERRID_ConstAsNonConstant, &Declarator->Name.TextSpan, Declarator->Name.Name );
        VariableSymbol->SetIsBad( m_SourceFile );
    }

    // Constants must have a value
    if ( !VariableSymbol->IsBad() && VariableDeclaration->Opcode != ParseTree::VariableDeclaration::WithInitializer ) // Make sure the constant has a value.
    {
        ReportError( ERRID_ConstantWithNoValue, &Declarator->Name.TextSpan );
        VariableSymbol->SetIsBad( m_SourceFile );
    }

    // Can only have certain specifiers
    if ( *SpecifiersForThisVar & ( ~LegalSpecifiersForThisVar ))
    {
        ReportDeclFlagError( ErrorToLog, *SpecifiersForThisVar & (~LegalSpecifiersForThisVar), VariableDeclarationStatement->Specifiers );
        *SpecifiersForThisVar &= LegalSpecifiersForThisVar;
        VariableSymbol->SetIsBad( m_SourceFile );
    }

    // We catch whether the Const variable was not of an intrinsic type/enum type in Bindable::ResolveNamedType()
}

/**************************************************************************************************
;CheckForTypeCharacter

Determine if a name contains a type character.  Also catches type characters in qualified names,
e.g. Foo.Bar$ or Foo$.Bar$, etc.
Takes care of logging the appropriate error
***************************************************************************************************/
bool // true - name contains a type character, false - doesn't
Declared::CheckForTypeCharacter(
    ParseTree::Name *NameTree // [in] the name you want to check to see if it has a type character
)
{
    bool FoundTypeChar = false;

    while ( NameTree->IsQualified() )
    {
        if ( NameTree->AsQualified()->Qualifier.IsBad ) return FoundTypeChar; // the thing is bad so we just punt  Microsoft:  

        if ( NameTree->AsQualified()->Qualifier.TypeCharacter != chType_NONE )
        {
            ReportError( ERRID_TypecharNotallowed, &NameTree->AsQualified()->Qualifier.TextSpan );
            FoundTypeChar = true; // Microsoft 
        }
        NameTree = NameTree->AsQualified()->Base;
    }

    VSASSERT( NameTree->IsSimple() ||NameTree->Opcode == ParseTree::Name::GlobalNameSpace , "Unexpected name tree kind" );

    if ( NameTree->IsSimple() && NameTree->AsSimple()->ID.IsBad ) return FoundTypeChar; // the thing is bad so we just punt  Microsoft:  

    if ( NameTree->IsSimple() && NameTree->AsSimple()->ID.TypeCharacter != chType_NONE )
    {
        ReportError( ERRID_TypecharNotallowed, &NameTree->AsSimple()->ID.TextSpan );
        FoundTypeChar = true;
    }

    return FoundTypeChar; // there is a type character
}


void Declared::SetContainerContext
(
    BCSYM_Container *Container
)
{
    if (!m_ContainerContextStack)
    {
        m_ContainerContextStack = new Stack<ContainerContextInfo *>();
    }

    ContainerContextInfo *ContextInfo = new (zeromemory) ContainerContextInfo(Container);
    m_ContainerContextStack->Push(ContextInfo);
}

void Declared::PopContainerContext()
{
    VSASSERT( m_ContainerContextStack, "Not initialized properly ?");
    VSASSERT( !m_ContainerContextStack->Empty(), "Why are we trying to pop an empty context ?");

    delete m_ContainerContextStack->Top();
    m_ContainerContextStack->Pop();

    if (m_ContainerContextStack->Empty())
    {
        delete m_ContainerContextStack;
        m_ContainerContextStack = NULL;
    }
}

BCSYM_Container* Declared::GetCurrentContainerContext()
{
    VSASSERT( m_ContainerContextStack, "Not initialized properly ?");

    return m_ContainerContextStack->Top()->m_Container;
}

void Declared::AddContainerContextToSymbolList()
{
    VSASSERT( m_ContainerContextStack &&
              m_ContainerContextStack->Count() > 0 &&
              m_ContainerContextStack->Top()->m_Container, "Container context not yet set!!!");

    BCSYM_Container *Container = m_ContainerContextStack->Top()->m_Container;

    if (Container->HasBeenAddedToDeclaredSymbolList())
    {
        return;
    }

    if (m_ContainerContextStack->Count() > 1)
    {
        BCSYM_Container *Parent =
            m_ContainerContextStack->Element(m_ContainerContextStack->Count() - 2)->m_Container;

        VSASSERT( Parent, "Non-NULL parent expected!!!");

        Parent->AddToNestedTypesList(Container);

        // Only types need to be added to the file level type list
        //
        if (!Container->IsNamespace() && m_FileLevelSymbolList)
        {
            // Create an alias that points to this type.
            //
            m_FileLevelSymbolList->AddToFront(
                m_SymbolAllocator.GetAliasOfSymbol(
                    Container,
                    Container->GetAccess(),
                    NULL));
        }
    }
    else
    {
        VSASSERT(m_FileLevelSymbolList == NULL ||
                 (Container->IsNamespace() &&
                  StringPool::IsEqual(
                    Container->GetName(),
                    STRING_CONST(m_CompilerInstance, EmptyString))), "Unnamed Namespace expected!!!");
    }

    Container->SetHasBeenAddedToDeclaredSymbolList(true);
}

// Now the ExpressionList is per container.
BCSYM_Expression** Declared::GetExpressionListHead()
{
    if (m_ContainerContextStack)
    {
        return GetCurrentContainerContext()->GetExpressionListAddress();
    }

    // This could occur when the types for Method locals are being
    // evaluated. There are a few other cases too.
    return &m_ExpressionListHead;
}

// Now the ExpressionList is per container.
void Declared::SetExpressionListHead(BCSYM_Expression *ExpressionListHead)
{
    if (m_ContainerContextStack)
    {
        GetCurrentContainerContext()->SetExpressionList(ExpressionListHead);
        return;
    }

    // This could occur when the types for Method locals are being
    // evaluated. There are a few other cases too.
    m_ExpressionListHead = ExpressionListHead;
}

// Now the ExpressionList is per container.
BCSYM_Expression** Declared::GetExpressionListTail()
{
    if (m_ContainerContextStack)
    {
        return &m_ContainerContextStack->Top()->m_ExpressionListTail;
    }

    // This could occur when the types for Method locals are being
    // evaluated. There are a few other cases too.
    return &m_ExpressionListTail;
}

void Declared::SetExpressionListTail(BCSYM_Expression *ExpressionListTail)
{
    if (m_ContainerContextStack)
    {
        m_ContainerContextStack->Top()->m_ExpressionListTail = ExpressionListTail;
        return;
    }

    // This could occur when the types for Method locals are being
    // evaluated. There are a few other cases too.
    m_ExpressionListTail = ExpressionListTail;
}

BCSYM_NamedType** Declared::GetTypeList()
{
    if (m_ContainerContextStack)
    {
        return GetCurrentContainerContext()->GetNamedTypesListReference();
    }

    // This could occur when the types for Method locals are being
    // evaluated. There are a few other cases too.
    return &m_TypeList;
}

SymbolList* Declared::GetUnBindableChildrenSymbolList()
{
    VSASSERT( m_ContainerContextStack,
                    "How can there be any unbindable members outside of container ?");

    return &m_ContainerContextStack->Top()->m_UnBindableChildren;
}

// This method returns the passed in Location buffer if there is an implements clause else returns NULL
//
Location *Declared::GetImplementsLocation
(
    Location *ImplementorLocation,                              // [in] Location of the implementing member
    ParseTree::PunctuatorLocation *ImplementsRelativeLocation,  // [in] location of the implements clause
                                                                //          relative to the implementing member
    Location *LocationBuffer                                    // [out] The location buffer to fill with the
                                                                //          location of the implements clause.
)
{
    VSASSERT( ImplementorLocation && ImplementsRelativeLocation, "Expected non-NULL locations!!!");
    VSASSERT( LocationBuffer, "Expected non-NULL location buffer!!!");

    Location LocationTemp =
        {
            ImplementsRelativeLocation->Line + ImplementorLocation->m_lBegLine,     // Start line
            ImplementsRelativeLocation->Line + ImplementorLocation->m_lBegLine,     // End line
            ImplementsRelativeLocation->Column,                                     // Start column
            ImplementsRelativeLocation->Column +                                    // End column
            (long)StringPool::StringLength(m_CompilerInstance->TokenToString(tkIMPLEMENTS)) - 1
        };

    *LocationBuffer = LocationTemp;

    return LocationBuffer;
}

// This method reports an error saying that a module member cannot implement other members.
//
void Declared::ReportModuleMemberCannotImplementError
(
    Location *ImplementorLocation,                              // [in] Location of the implementing member
    ParseTree::PunctuatorLocation *ImplementsRelativeLocation   // [in] location of the implements clause
                                                                //          relative to the implementing member
)
{
    VSASSERT( ImplementorLocation && ImplementsRelativeLocation, "Expected non-NULL locations!!!");

    Location ImplementsLocation;


    ReportError( ERRID_ModuleMemberCantImplement,
                    GetImplementsLocation(ImplementorLocation, ImplementsRelativeLocation, &ImplementsLocation));
}

// Builds the THIS type - it is the same as the ClassOrInterface for non-generic types,
// but is the open binding corresponding to the ClassOrInterface when the ClassOrInterface
// is either generic or is nested ina generic type.
//
BCSYM_NamedType *Declared::MakeNamedTypeForTHISType
(
    BCSYM_NamedRoot *Context
)
{
    VSASSERT( GetCurrentContainerContext()->IsClass(), "Class expected!!!");

    return
        MakeNamedType(
            NULL,
            Context,
            GetTypeList(),
            true);  // Is THIS type,  IsAppliedAttributeContext=false, ArgumentNullable = NULL */
}
