//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ExpressionTreeSemantics:
//      Class that deals with the translation from a VB bound tree to a DLR tree
// that has the same VB semantics. This is templatized and can be used to
// generate ASTs for any abstraction. Currently this is used to
// produce:
//  1. Bound trees that produce DLR ASTs - for LINQ
//  2. DLR ASTs directly - for the hosted compiler scenario
//
//-------------------------------------------------------------------------------------------------


#pragma once

#include "ExpressionTreeGenerator.h"

typedef FixedSizeHashTable<64, Variable *, LambdaParamHandle *> LambdaParamsMap;

template<class TREE_TYPE>
class ExpressionTreeSemanticsVisitor
{
protected:

    TREE_TYPE* InvokeConvertMethod(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
#if DEBUG
        ,BILOP DebugOnlyDummyOpcode	= (BILOP)0	// Only used by the DebugExpressionTreeSemantics checker
#endif
        );

#define DECLARE_ABSTRACT_CONVERSION_FUNCTION(name) \
    virtual TREE_TYPE* name( \
        ILTree::Expression* Input, \
        ExpressionFlags Flags, \
        Type* TargetType \
        ) = 0;

    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertArrayIndex );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertArrayLength );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertAssignment );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertBinaryOperator );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertBooleanOperator );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertShortCircuitedBooleanOperator );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertIIF );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertIsTrue );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertCoalesce );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertCTypeOp );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertAnonymousType );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertCall );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertCast );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertConstantValue );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertInitStructure );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertIsType );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertLambda );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertLike );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertNegateNot );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertNew );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertCollectionInitializer );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertArrayLiteralToExpressionTree );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertNewArray );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertNewObjectInitializer );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertSEQ );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertXml );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertSymbol );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertWideningConversions );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertBadExpression );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertIf );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertSynclockCheck );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertLateBoundExpression );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertPlus );
    DECLARE_ABSTRACT_CONVERSION_FUNCTION( ConvertAdr );

    virtual TREE_TYPE* HandleUnexpectedExprKind(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
        ) = 0;

#undef DECLARE_ABSTRACT_CONVERSION_FUNCTION
};


template<class TREE_TYPE>
class ExpressionTreeSemantics : private ExpressionTreeSemanticsVisitor<TREE_TYPE>
{
protected:

    // Helper methods. These need to be protected so that they are accessible to derived classes too.


    static bool IsInitStructure(ILTree::Expression* Input);

    //-------------------------------------------------------------------------------------------------
    //
    // Check if the expression type is an object initializer.
    //
    static bool IsObjectInitializer(ILTree::Expression* Input);

    // ----------------------------------------------------------------------------
    // Find the right runtime member for casting from decimal to the target type.
    // Casts to bool still need to go through the VB runtime; other casts will
    // load the cast operators for expression trees, rather than calling
    // CodeGenerator::GetHelperFromDecimal, which returns constructors.
    // ----------------------------------------------------------------------------

#ifdef HOSTED
    // Dev10 bug 822565 - Need to make this method virtual for hosted compiler
    // so that its impl can be overridden in the hosted compiler for non-expr tree
    // lambda scenarios in order to ensure compat with core compiler for decimal
    // conversions.
    //
    virtual
#else
    static
#endif
    RuntimeMembers GetHelperCastFromDecimal(Vtypes vtypeTo);

    // ----------------------------------------------------------------------------
    // Find the right runtime member for casting from source type to decimal.
    // Casts to bool still need to go through the VB runtime; other casts will
    // load the cast operators for expression trees, rather than calling
    // CodeGenerator::GetHelperToDecimal, which returns constructors.
    // ----------------------------------------------------------------------------

    static RuntimeMembers GetHelperCastToDecimal(Vtypes vtypeFrom);

    // ----------------------------------------------------------------------------
    // Determine the runtime helper for conversions to boolean since the expression
    // tree API does not support conversions to boolean.
    // ----------------------------------------------------------------------------

    static RuntimeMembers GetHelperToBoolean(Vtypes vtypeFrom);

    // ----------------------------------------------------------------------------
    // For floating point conversions we go through Convert.ToIntXXX, in order for
    // DLINQ and the expression tree compiler to get the right semantics.
    // ----------------------------------------------------------------------------

    static RuntimeMembers GetHelperCastFromSingle(Vtypes vtypeTo);

    // ----------------------------------------------------------------------------
    // For floating point conversions we go through Convert.ToIntXXX, in order for
    // DLINQ and the expression tree compiler to get the right semantics.
    // ----------------------------------------------------------------------------

    static RuntimeMembers GetHelperCastFromDouble(Vtypes vtypeTo);

    // ----------------------------------------------------------------------------
    // Determine the runtime helper for the CTYPE. This will cause the expression
    // tree to generate a node with the method info for the runtime type.
    // ----------------------------------------------------------------------------

    // Dev10 bug 822565 - Need to make this method non-static for hosted compiler
    // due to the fix for this bug since one of the methods invoked by its impl has
    // been made a non-static (and virtual) as part of this bug fix.
    //
#ifndef HOSTED
    static
#endif
    RuntimeMembers DetermineRuntimeMemberForCType(
        Vtypes vtypeTo,
        Vtypes vtypeFrom,
        ILTree::Expression* Input,
        Type* Object
        );    


    // ----------------------------------------------------------------------------
    // Finds a runtime helper and returns its Procedure. If helper is not found
    // reports error and returns NULL.
    // ----------------------------------------------------------------------------
    Procedure *GetRuntimeMemberProcedure( RuntimeMembers Helper, const Location &Loc, bool fReportErrors = true );

    // ----------------------------------------------------------------------------
    // Determine whether this opcode is a boolean operator because boolean
    // nodes have extra parameters that we need to encode.
    // ----------------------------------------------------------------------------

    static bool IsBooleanOperator(BILOP bilop);

    // ----------------------------------------------------------------------------
    // This method returns true if the opcode is a binary operator.
    // ----------------------------------------------------------------------------

    static bool IsBinaryOperator(BILOP opcode);

    // ----------------------------------------------------------------------------
    // When SX_NOTHING is used with certain binary operators, the compiler
    // creates a new structure and uses this structure to do the operation. This
    // method returns true for the opcodes that do this.
    // ----------------------------------------------------------------------------

    static bool OpcodeHasSpecialStructureSemantics(BILOP opcode);

    // ----------------------------------------------------------------------------
    // This method walks through a tree and sees if we cast from SX_NOTHING.
    // ----------------------------------------------------------------------------

    static bool DigAndCheckIfItIsNothing(ILTree::Expression* PotentialNothing);

    // ----------------------------------------------------------------------------
    // Check whether the type is a small integral type.
    // ----------------------------------------------------------------------------

    static bool IsSmallIntegralType(Vtypes vt);

    // ----------------------------------------------------------------------------
    // Report the expression tree error.
    // ----------------------------------------------------------------------------

    void ReportErrorForExpressionTree(ERRID err, const Location &Loc);
    void ReportErrorForMissingHelper(const Location &Loc, RuntimeMembers Helper);

    // ----------------------------------------------------------------------------
    // Generate a cast to Int32 if the type is i1 or ui1.
    // ----------------------------------------------------------------------------

    TREE_TYPE *GenerateCastToInteger(
        Vtypes vtype,
        bool IsNullable,
        TREE_TYPE *TreeInput,
        const Location &Loc,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // If we did cast to i4, cast back.
    // ----------------------------------------------------------------------------

    TREE_TYPE *GenerateCastBackFromInteger(
        ILTree::Expression* Input,
        TREE_TYPE *TreeInput,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Generate a cast node for the enum conversion. This is required because the
    // expression tree API does not allow enum types in intrinsic operators; thus
    // we need to generate a cast to the underlying enum type. This assumes that
    // the TreeInput argument is already converted to an expression tree.
    // ----------------------------------------------------------------------------

    TREE_TYPE *GenerateEnumConversionIfNeeded(
        ILTree::Expression* Input,
        TREE_TYPE *TreeInput,
        Type* TargetType
        );

#if HOSTED
    virtual
#endif
    IReadOnlyList<TREE_TYPE*>* CreateArgumentListForExpressionTree(
        ExpressionList * ArgumentExpressions,
        ExpressionFlags Flags,
        Symbol * TargetType,
        BCSYM_Proc * TargetMethod,    
        const Location &Loc,
        NorlsAllocator &Allocator
        );

    ILTree::LambdaExpression* ConvertLambaExpressionFromSeqOp2(
        ILTree::Expression* assignment,
        ILTree::Expression* lambda,
        bool useInstance
        );

    // ----------------------------------------------------------------------------
    // Convert a method to the corresponding operator.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertUserDefinedOperatorToExpressionTree(
        ILTree::Expression* Input,
        ILTree::Expression* MethodTree,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert a method to the corresponding operator.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertUserDefinedOperatorToExpressionTree(
        ILTree::Expression* Input,
        ILTree::Expression* MethodTree,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        ILTree::Expression* SecondArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert a method to the corresponding operator.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertUserDefinedCastOperatorToExpressionTree(
        ILTree::Expression* Input,
        ILTree::SymbolReferenceExpression* MethodTree,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        Type* CastType,
        ExpressionFlags Flags,
        Type* TargetType
    );

    // ----------------------------------------------------------------------------
    // Check whether we should generate expression trees with checked semantics.
    // ----------------------------------------------------------------------------

    bool UseCheckedSemantics(ILTree::Expression* Input);

    // ----------------------------------------------------------------------------
    // If the opcode is one that VB allows conversions of nothing to new, default
    // structures, and we have a structure and a nothing, then return true.
    // ----------------------------------------------------------------------------

    bool ShouldConvertNothingToStructureType(
        BILOP opcode,
        ILTree::Expression* PotentialStructureType,
        ILTree::Expression* PotentialNothing
        );

    // ----------------------------------------------------------------------------
    // Convert a one-argument VBRuntime method call to an expression tree.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertRuntimeHelperToExpressionTree(
        RuntimeMembers Helper,
        const Location& Loc,
        Type* ResultType,
        ILTree::Expression* Argument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert a two-argument VBRuntime method call to an expression tree.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertRuntimeHelperToExpressionTree(
        RuntimeMembers Helper,
        const Location& Loc,
        Type* ResultType,
        ILTree::Expression* FirstArgument,
        ILTree::Expression* SecondArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert a three-argument VBRuntime method call to an expression tree.
    // This will generate operator nodes with the correct method info based on
    // the runtime helper if the runtime helper has a corresponding expression tree
    // representation. Otherwise, it will generate CALL nodes.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertRuntimeHelperToExpressionTree(
        RuntimeMembers Helper,
        const Location &Loc,
        Type* ResultType,
        ILTree::Expression* FirstArgument,
        ILTree::Expression* SecondArgument,
        ILTree::Expression* ThirdArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Cast operators have a different signature, so we have to special case
    // it here.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertCastRuntimeHelperToExpressionTree(
        ILTree::Expression* Input,
        RuntimeMembers Helper,
        const Location &Loc,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Boolean needs special handling.
    // 



    TREE_TYPE *ConvertBooleanRuntimeHelperToExpressionTree(
        ILTree::Expression* Input,
        RuntimeMembers Helper,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        ILTree::Expression* SecondArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert one argument runtime helpers to Expression trees.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertRuntimeHelperOperatorToExpressionTree(
        ILTree::Expression* Input,
        RuntimeMembers Helper,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Convert two-argument operators to expression trees.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertRuntimeHelperOperatorToExpressionTree(
        ILTree::Expression* Input,
        RuntimeMembers Helper,
        const Location &Loc,
        ILTree::Expression* FirstArgument,
        ILTree::Expression* SecondArgument,
        ExpressionFlags Flags,
        Type* TargetType
        );
    
    // ----------------------------------------------------------------------------
    // Create the lambda expression for the coalesce operator.
    // ----------------------------------------------------------------------------

    TREE_TYPE *CreateLambdaForCoalesce(
        TREE_TYPE* LambdaBody,
        LambdaParamHandle *LambdaParameter,
        Type* LeftHandType,
        Type* RightHandType,
        Type* TargetType,
        const Location& Loc,
        NorlsAllocator &Allocator
        );

    // ----------------------------------------------------------------------------
    // Given a method symbol and an argument, build a call expression for
    // this method.
    // ----------------------------------------------------------------------------

    TREE_TYPE *BuildCallExpressionFromMethod(
        Procedure* Method,
        ILTree::Expression* Operand,
        LambdaParamHandle *ExprTreeLambdaParameter,
        const Location& Loc,
        NorlsAllocator &Allocator
        );

    // ----------------------------------------------------------------------------
    // Return the VType of the tree.
    // ----------------------------------------------------------------------------

    Vtypes GetVtypeOfTree(ILTree::Expression* Input);

    // ----------------------------------------------------------------------------
    // For boolean conversions, we must find the matching signed types of the
    // unsigned type in order to do the proper negation semantics.
    // ----------------------------------------------------------------------------

    Type* GetTypeForBooleanConversion(Vtypes vt);

    // ----------------------------------------------------------------------------
    // Convert the constructor call of a delegate.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertDelegateConstructor(
        ILTree::Expression* Input,
        Type* DelegateType,
        ExpressionFlags Flags,
        Type* TargetType
        );

    // ----------------------------------------------------------------------------
    // Helper method that generates an expression tree call to create a new
    // structure type.
    // ----------------------------------------------------------------------------

    TREE_TYPE *CreateNewStructureType(
        Type* StructureType,
        const Location& Loc,
        Type* TargetType
        );

    TREE_TYPE *ConvertNewArrayInitializer(
        ILTree::Expression* Input,
        ILTree::Expression* Value,
        ExpressionFlags Flags,
        Type* TargetType
        );

    FX::FXSymbolProvider* GetFXSymbolProvider()
    {
        return m_Semantics->GetFXSymbolProvider();
    };

protected:

    // Non-helper conversion method that should be accessible to derived classes most
    // of which can also be overridden.

    // ----------------------------------------------------------------------------
    // Convert the VB expressions to an expression tree.
    // ----------------------------------------------------------------------------

    TREE_TYPE *ConvertInternalToExpressionTree(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
        );

    // Define the Conversion methods.

#define DECLARE_CONVERSION_FUNCTION(name) \
    virtual TREE_TYPE* name( \
        ILTree::Expression* Input, \
        ExpressionFlags Flags, \
        Type* TargetType \
        );

    DECLARE_CONVERSION_FUNCTION( ConvertArrayIndex );
    DECLARE_CONVERSION_FUNCTION( ConvertArrayLength );
    DECLARE_CONVERSION_FUNCTION( ConvertAssignment );
    DECLARE_CONVERSION_FUNCTION( ConvertBinaryOperator );
    DECLARE_CONVERSION_FUNCTION( ConvertBooleanOperator );
    DECLARE_CONVERSION_FUNCTION( ConvertShortCircuitedBooleanOperator );
    DECLARE_CONVERSION_FUNCTION( ConvertIIF );
    DECLARE_CONVERSION_FUNCTION( ConvertIsTrue );
    DECLARE_CONVERSION_FUNCTION( ConvertCoalesce );
    DECLARE_CONVERSION_FUNCTION( ConvertCTypeOp );
    DECLARE_CONVERSION_FUNCTION( ConvertAnonymousType );
    DECLARE_CONVERSION_FUNCTION( ConvertCall );
    DECLARE_CONVERSION_FUNCTION( ConvertCast );
    DECLARE_CONVERSION_FUNCTION( ConvertConstantValue );
    DECLARE_CONVERSION_FUNCTION( ConvertInitStructure );
    DECLARE_CONVERSION_FUNCTION( ConvertIsType );
    DECLARE_CONVERSION_FUNCTION( ConvertLambda );
    DECLARE_CONVERSION_FUNCTION( ConvertLike );
    DECLARE_CONVERSION_FUNCTION( ConvertNegateNot );
    DECLARE_CONVERSION_FUNCTION( ConvertNew );
    DECLARE_CONVERSION_FUNCTION( ConvertCollectionInitializer );
    DECLARE_CONVERSION_FUNCTION( ConvertArrayLiteralToExpressionTree );
    DECLARE_CONVERSION_FUNCTION( ConvertNewArray );
    DECLARE_CONVERSION_FUNCTION( ConvertNewObjectInitializer );
    DECLARE_CONVERSION_FUNCTION( ConvertSEQ );
    DECLARE_CONVERSION_FUNCTION( ConvertXml );
    DECLARE_CONVERSION_FUNCTION( ConvertSymbol );
    DECLARE_CONVERSION_FUNCTION( ConvertWideningConversions );
    DECLARE_CONVERSION_FUNCTION( ConvertBadExpression );
    DECLARE_CONVERSION_FUNCTION( ConvertIf );
    DECLARE_CONVERSION_FUNCTION( ConvertSynclockCheck );
    DECLARE_CONVERSION_FUNCTION( ConvertLateBoundExpression );
    DECLARE_CONVERSION_FUNCTION( ConvertPlus );
    DECLARE_CONVERSION_FUNCTION( ConvertAdr );

#undef DECLARE_CONVERSION_FUNCTION

    virtual TREE_TYPE* HandleUnexpectedExprKind(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
        );

    virtual ILTree::Expression* ExtractArgumentForByrefParam(
        ILTree::Expression *Argument
    );

public:

    // ----------------------------------------------------------------------------
    // Convert lambdas to expression trees.
    // ----------------------------------------------------------------------------

    virtual TREE_TYPE *ConvertLambdaToExpressionTree(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

    ExpressionTreeSemantics(
        Semantics *SemanticsContext,
        ExpressionTreeGenerator<TREE_TYPE> *ExprTreeGenerator,
        ExprTreeGenRuntimeValuesSupport<TREE_TYPE> *ExprTreeGenRuntimeValues	// can be NULL for hosted compiler scenarios
        ) :
            m_Semantics(SemanticsContext),
            m_Compiler(SemanticsContext->m_Compiler),
            m_CompilerHost(SemanticsContext->GetCompilerHost()),
            m_CompilationCaches(SemanticsContext->m_CompilationCaches),
            m_SymbolCreator(SemanticsContext->m_SymbolCreator),
            m_NoIntChecks(SemanticsContext->m_NoIntChecks),
            m_CoalesceLambdaParameter(NULL),
            m_CoalesceArgumentToBeReplaced(NULL),
            m_LastExprTreeErrID(NOERROR),
            m_ConvertingQualifiedExpression(false),
            m_ExprTreeGenerator(ExprTreeGenerator),
            m_ExprTreeGenRuntimeValues(ExprTreeGenRuntimeValues),
            m_CurrentLambdaParamsMap(NULL)
    {
        ThrowIfNull(SemanticsContext);
        ThrowIfNull(ExprTreeGenerator);
        
        // ExprTreeGeneratorRuntimeValues can be NULL for hosted compiler scenarios
        // ThrowIfNull(ExprTreeGeneratorRuntimeValues);
    }
    
protected:

    // Data members

    // Protected data members. Should be available to derived classes

    Semantics *m_Semantics;

    Compiler *m_Compiler;
    CompilerHost *m_CompilerHost;
    CompilationCaches *m_CompilationCaches;
    
    Symbols m_SymbolCreator;

    // project option that indicates no integer overflow
    bool m_NoIntChecks;

    // If these two are set, it means that ConvertInternalToExpressionTree will replace
    // the expression with a reference to the variable.

private:

    // Private data members. Should NOT be available to derived classes

    LambdaParamHandle *m_CoalesceLambdaParameter;
    ILTree::Expression *m_CoalesceArgumentToBeReplaced;

    // remembers the last expr tree error
    ERRID m_LastExprTreeErrID;

    bool m_ConvertingQualifiedExpression;

    ExpressionTreeGenerator<TREE_TYPE> *m_ExprTreeGenerator;

    // can be NULL for hosted compiler scenarios
    ExprTreeGenRuntimeValuesSupport<TREE_TYPE> *m_ExprTreeGenRuntimeValues;

    // Hash to map from labda parameter locals to the corresponding created
    // lambda parameters. Starts out as NULL.
#if HOSTED
    protected:
#endif
    LambdaParamsMap *m_CurrentLambdaParamsMap;
};

//-------------------------------------------------------------------------------------------------
// ArrayList_NorlsAllocBased is a simple wrapper for ArrayList that always uses no release allocators.
// Created this wrapper to avoid creating a NorlsAllocWrapper at every use site and also to avoid
// specifying the NorlsAllocWrapper (cannot specific typedefs for partially open templates) in the
// type specificication at every use site.
//
// Additionally this also implements IReadOnlyList so that the factory methods in ExpressionTreeGenerator
// don't need to specify ArrayList_NorlsAllocBased as the parameter type, but can instead use
// IReadOnlyList which is sufficient for the factory method since they only read from these lists.
//-------------------------------------------------------------------------------------------------

template<class T>
class ArrayList_NorlsAllocBased : public IReadOnlyList<T>
{
public:
    ArrayList_NorlsAllocBased(NorlsAllocator *Allocator)
        : m_NorlsAllocWrapper(Allocator),
          m_ArrayList(m_NorlsAllocWrapper, 0)
    {
    };

    void Add(const T &Item)
    {
        m_ArrayList.Add(Item);
    }

    unsigned long Count() const
    {
        return m_ArrayList.Count();
    }

    const T & operator[](const unsigned long Index) const
    {
        return m_ArrayList[Index];
    }

private:

    // Note that the order in which the members appear is important
    // since m_NorlsAllocWrapper is used to initialize m_ArrayList
    // and hence m_NorlsAllocWrapper needs to be initalized before
    // m_ArrayList. For this to happen, m_NorlsAllocWrapper needs
    // to be declared before m_ArrayList.
    //
    NorlsAllocWrapper m_NorlsAllocWrapper;
    ArrayList<T, NorlsAllocWrapper> m_ArrayList;
};

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemanticsVisitor<TREE_TYPE>::InvokeConvertMethod(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
#if DEBUG
    ,BILOP DebugOnlyDummyOpcode	// Only used by the DebugExpressionTreeSemantics checker
#endif
    )
{
    // This is purely a dispatch method and no ET conversion related
    // semantics should be handled in this method. Any ET conversion
    // semantics should be handled in the appropriate virtual Convert*
    // method so that the conversion semantics can then be overridden
    // in any specialization of ExpressionTreeSemantics.

#if !DEBUG
    ThrowIfNull(Input);
#endif

    BILOP Opcode =
#if DEBUG
        (Input? Input->bilop : DebugOnlyDummyOpcode);
#else
        (Input->bilop);
#endif

    switch (Opcode)
    {
    case SX_BAD:
        return ConvertBadExpression(Input, Flags, TargetType);

    case SX_SYM:
        return ConvertSymbol(Input, Flags, TargetType);

    case SX_INIT_STRUCTURE:
        return ConvertInitStructure(Input, Flags, TargetType);

    case SX_NEW:
        return ConvertNew(Input, Flags, TargetType);

    case SX_CALL:
        return ConvertCall(Input, Flags, TargetType);

    case SX_NEW_ARRAY:
        return ConvertNewArray(Input, Flags, TargetType);

    case SX_ARRAYLITERAL:
        return ConvertArrayLiteralToExpressionTree(Input, Flags, TargetType);

    case SX_COLINIT:
        return ConvertCollectionInitializer(Input, Flags, TargetType);

    case SX_ADR:
        return ConvertAdr(Input, Flags, TargetType);

    case SX_ASG:
    case SX_ASG_RESADR:
        return ConvertAssignment(Input, Flags, TargetType);

    case SX_INDEX:
        return ConvertArrayIndex(Input, Flags, TargetType);

    case SX_LATE:
    case SX_VARINDEX:
        return ConvertLateBoundExpression(Input, Flags, TargetType);

    case SX_ARRAYLEN:
        return ConvertArrayLength(Input, Flags, TargetType);

    case SX_SEQ:
    case SX_SEQ_OP1:
    case SX_SEQ_OP2:
        return ConvertSEQ(Input, Flags, TargetType);

    case SX_NOTHING:
    case SX_CNS_INT:
    case SX_CNS_DEC:
    case SX_CNS_FLT:
    case SX_CNS_STR:
        return ConvertConstantValue(Input, Flags, TargetType);

    case SX_METATYPE:
        VSASSERT(
            Input == NULL ||	// NULL in DebugExpressionTreeSemantics checker scenarios
            Input->AsExpressionWithChildren().Left->bilop == SX_SYM || Input->AsExpressionWithChildren().Left->bilop == SX_NOTHING,
            "How did this metatype node get here?"
            );
        return ConvertConstantValue(Input, Flags, TargetType);

    case SX_ISTYPE:
        return ConvertIsType(Input, Flags, TargetType);

    case SX_WIDE_COERCE:
        return ConvertWideningConversions(Input, Flags, TargetType);

    case SX_PLUS:
        return ConvertPlus(Input, Flags, TargetType);

    case SX_CTYPE:
    case SX_TRYCAST:
    case SX_DIRECTCAST:
        return ConvertCast(Input, Flags, TargetType);

    case SX_NEG:
    case SX_NOT:
        return ConvertNegateNot(Input, Flags, TargetType);

    case SX_LIKE:
        return ConvertLike(Input, Flags, TargetType);

    case SX_AND:
    case SX_OR:
    case SX_XOR:
    case SX_POW:
    case SX_MUL:
    case SX_ADD:
    case SX_SUB:
    case SX_DIV:
    case SX_MOD:
    case SX_IDIV:
    case SX_CONC:
    case SX_SHIFT_LEFT:
    case SX_SHIFT_RIGHT:
        return ConvertBinaryOperator(Input, Flags, TargetType);

    case SX_IS:
    case SX_ISNOT:
    case SX_EQ:
    case SX_NE:
    case SX_LE:
    case SX_GE:
    case SX_LT:
    case SX_GT:
        return ConvertBooleanOperator(Input, Flags, TargetType);

    case SX_ANDALSO:
    case SX_ORELSE:
        return ConvertShortCircuitedBooleanOperator(Input, Flags, TargetType);

    case SX_IIF:
        return ConvertIIF(Input, Flags, TargetType);

    case SX_ISTRUE:
        return ConvertIsTrue(Input, Flags, TargetType);

    case SX_IIFCoalesce:
        return ConvertCoalesce(Input, Flags, TargetType);

    case SX_CTYPEOP:
        return ConvertCTypeOp(Input, Flags, TargetType);

    case SX_ANONYMOUSTYPE:
        return ConvertAnonymousType(Input, Flags, TargetType);

    case SX_LAMBDA:
        return ConvertLambda(Input, Flags, TargetType);

    case SX_IF:
        return ConvertIf(Input, Flags, TargetType);

    case SX_SYNCLOCK_CHECK:
        return ConvertSynclockCheck(Input, Flags, TargetType);

    // Unexpected expression nodes

    case SX_NAME:				// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_APPL_ATTR:			// Should only occur when processing applied attributes
    case SX_CREATE_ARRAY:		// Should never appear in a tree meant for code-gen
    case SX_ARG:				// Should never appear as the head of an expression.
    case SX_ADDRESSOF:			// Should never appear in a tree meant for code-gen - should have already been converted to a delegate constructor call
    case SX_LIST:				// Should never appear as the head of an expression.
    case SX_BOGUS:				// Should never appear in a tree meant for code-gen
    case SX_DELEGATE_CTOR_CALL:	// Should be handled in SX_NEW and so should not appear as the head of an expression.
    case SX_PROPERTY_REFERENCE:	// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_LATE_REFERENCE:		// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_NAME_NOT_FOUND:		// Should only occur within semantic analysis for debugging scenarios
    case SX_OVERLOADED_GENERIC:	// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_EXTENSION_CALL:		// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_UNBOUND_LAMBDA:		// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_DEFERRED_TEMP:		// Should only occur temporarily to hold an intermediate state within semantic analysis
    case SX_ISFALSE:			// Should never appear in non-lowered bound trees and expression tree conversion is 
                                //     always done on non-lowered trees only
    case SX_NESTEDARRAYLITERAL:	// Should never appear as the head of an expression.
    case SX_COLINITELEMENT:		// Should never appear as the head of an expression.
    case SX_ASYNCSPILL:         // Should never appear in an expression tree.

    case SX_AWAIT:                      // isn't allowed in an expression-tree

        return HandleUnexpectedExprKind(Input, Flags, TargetType);

    default:
        // Unanticipated expression nodes - would occur if any new expression nodes are
        // introduced in the compiler and not handled in ET semantics

        // If this assert is ever hit, that indicates that a new expression node has been
        // added in the compiler without handing it wrt ExpressionTreeSemantics. For any new
        // nodes:
        // - if the new node is not expected to be seen by ET semantics due to reasons like
        //		"only occurs temporarily as an intermediate state within semantic analysis", then
        //		they should be added to the switch to invoke HandleUnexpectedExprKind.
        //
        // - if the new node is expected to be seen by ET semantics, then
        //		- a new Convert* visitor method needs to be added for this node
        //		- a corresponding case statement needs to be added to the above switch where
        //			the new visitor method should be invoked.
        //		
        //		The new visitor method can then be implemented in ExpressionTreeSemantics to
        //		either report a compile error or actually the new expression kind to an ET.
        //
        //		Note that all the derived classes of ExpressionTreeSemantics also need
        //		to be reviewed if they will need to override the visitor method for this new
        //		expression kind or the handling in ExpressionTreeSemantics is sufficient.
        //
#if DEBUG
        VSASSERT(false, "Unanticipated node kind in ExpressionTreeSemantics - support needs to be added either by adding conversion to ET or by reporting a compile error!");
    
        // This throw will help catch this scenario on compiler startup itself
        // using DebugETSemantics.

        IfFalseThrow(false);

        // Dummy return to prevent warning ("not all control paths return a value")
        // from compiler
        return NULL;
#else
        // Don't throw exception in release. Rather handle with an error.
        return HandleUnexpectedExprKind(Input, Flags, TargetType);
#endif DEBUG
    }
}

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::IsInitStructure(ILTree::Expression* Input)
{
    if( Input->bilop == SX_SEQ &&
        Input->AsExpressionWithChildren().Left->bilop == SX_ASG &&
        Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->bilop == SX_SEQ_OP2 &&
        Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left->bilop == SX_INIT_STRUCTURE )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

//-------------------------------------------------------------------------------------------------
//
// Check if the expression type is an object initializer.
//
template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::IsObjectInitializer(ILTree::Expression* Input)
{
    if( ( IsInitStructure( Input ) ||
            ( Input->AsExpressionWithChildren().Left->bilop == SX_ASG && Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right &&
              Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->bilop == SX_NEW )
        ) &&
        Input->AsExpressionWithChildren().Right && Input->AsExpressionWithChildren().Right->bilop == SX_LIST )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// Find the right runtime member for casting from decimal to the target type.
// Casts to bool still need to go through the VB runtime; other casts will
// load the cast operators for expression trees, rather than calling
// CodeGenerator::GetHelperFromDecimal, which returns constructors.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::GetHelperCastFromDecimal(Vtypes vtypeTo)
{
    switch( vtypeTo )
    {
    case t_bool:    return DecimalToBooleanMember;
    case t_i1:      return DecimalToSignedByteCast;
    case t_ui1:     return DecimalToByteCast;
    case t_i2:      return DecimalToShortCast;
    case t_ui2:     return DecimalToUnsignedShortCast;
    case t_i4:      return DecimalToIntegerCast;
    case t_ui4:     return DecimalToUnsignedIntegerCast;
    case t_i8:      return DecimalToLongCast;
    case t_ui8:     return DecimalToUnsignedLongCast;
    case t_single:  return DecimalToSingleCast;
    case t_double:  return DecimalToDoubleCast;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// ----------------------------------------------------------------------------
// Find the right runtime member for casting from source type to decimal.
// Casts to bool still need to go through the VB runtime; other casts will
// load the cast operators for expression trees, rather than calling
// CodeGenerator::GetHelperToDecimal, which returns constructors.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::GetHelperCastToDecimal(Vtypes vtypeFrom)
{
    switch( vtypeFrom )
    {
    case t_bool:    return BooleanToDecimalMember;
    case t_i1:
    case t_ui1:
    case t_i2:
    case t_ui2:
    case t_i4:      return IntegerToDecimalCast;
    case t_ui4:     return UnsignedIntegerToDecimalCast;
    case t_i8:      return LongToDecimalCast;
    case t_ui8:     return UnsignedLongToDecimalCast;
    case t_single:  return SingleToDecimalCast;
    case t_double:  return DoubleToDecimalCast;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// ----------------------------------------------------------------------------
// Determine the runtime helper for conversions to boolean since the expression
// tree API does not support conversions to boolean.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::GetHelperToBoolean(Vtypes vtypeFrom)
{
    switch( vtypeFrom )
    {
    // The IDE may create a bound tree with identity conversions, and we need to
    // handle this for overload resolution.
    case t_bool:    return UndefinedRuntimeMember;
    case t_i1:
    case t_ui1:
    case t_i2:
    case t_ui2:
    case t_i4:      return IntegerToBooleanMember;
    case t_ui4:     return UnsignedIntegerToBooleanMember;
    case t_i8:      return LongToBooleanMember;
    case t_ui8:     return UnsignedLongToBooleanMember;
    case t_single:  return SingleToBooleanMember;
    case t_double:  return DoubleToBooleanMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// ----------------------------------------------------------------------------
// For floating point conversions we go through Convert.ToIntXXX, in order for
// DLINQ and the expression tree compiler to get the right semantics.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::GetHelperCastFromSingle(Vtypes vtypeTo)
{
    switch( vtypeTo )
    {
    case t_i1:      return SingleToSignedByteMember;
    case t_ui1:     return SingleToByteMember;
    case t_i2:      return SingleToShortMember;
    case t_ui2:     return SingleToUnsignedShortMember;
    case t_i4:      return SingleToIntegerMember;
    case t_ui4:     return SingleToUnsignedIntegerMember;
    case t_i8:      return SingleToLongMember;
    case t_ui8:     return SingleToUnsignedLongMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// ----------------------------------------------------------------------------
// For floating point conversions we go through Convert.ToIntXXX, in order for
// DLINQ and the expression tree compiler to get the right semantics.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::GetHelperCastFromDouble(Vtypes vtypeTo)
{
    switch( vtypeTo )
    {
    case t_i1:      return DoubleToSignedByteMember;
    case t_ui1:     return DoubleToByteMember;
    case t_i2:      return DoubleToShortMember;
    case t_ui2:     return DoubleToUnsignedShortMember;
    case t_i4:      return DoubleToIntegerMember;
    case t_ui4:     return DoubleToUnsignedIntegerMember;
    case t_i8:      return DoubleToLongMember;
    case t_ui8:     return DoubleToUnsignedLongMember;

    default:
        VSFAIL("unexpected type");
        VbThrow(HrMake(ERRID_CodegenError));
        return UndefinedRuntimeMember;
    }
}

// ----------------------------------------------------------------------------
// Determine the runtime helper for the CTYPE. This will cause the expression
// tree to generate a node with the method info for the runtime type.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> RuntimeMembers
ExpressionTreeSemantics<TREE_TYPE>::DetermineRuntimeMemberForCType(
    Vtypes vtypeTo,
    Vtypes vtypeFrom,
    ILTree::Expression* Input,
    Type* Object
    )
{
    ThrowIfNull( Input );
    ThrowIfNull( Object );

    RuntimeMembers Helper = UndefinedRuntimeMember;

    if( vtypeTo == t_string &&
        ( vtypeFrom != t_ref ||
          Input->AsExpressionWithChildren().Left->ResultType != Object
        )
      )
    {
        Helper = CodeGenerator::GetHelperForStringCoerce( vtypeFrom );
    }
    else if( vtypeFrom == t_string && vtypeTo != t_ref )
    {
        Helper = CodeGenerator::GetHelperFromString( vtypeTo );
    }
    else if( vtypeTo == t_decimal && vtypeFrom != t_string && vtypeFrom != t_ref )
    {
        Helper = GetHelperCastToDecimal( vtypeFrom );
    }
    else if( vtypeFrom == t_decimal && vtypeTo != t_string && vtypeTo != t_ref )
    {
        Helper = GetHelperCastFromDecimal( vtypeTo );
    }
    else if( vtypeFrom == t_ref && vtypeTo != t_ref && vtypeTo != t_struct && vtypeTo != t_generic )
    {
        if( vtypeTo == t_array )
        {
            if( TypeHelpers::IsCharArrayRankOne( Input->AsExpressionWithChildren().ResultType ) )
            {
                Helper = ObjectToCharArrayMember;
            }
        }
        else
        {
            Helper = CodeGenerator::GetHelperFromRef( vtypeTo );
        }
    }
    else if( vtypeTo == t_bool )
    {
        AssertIfFalse( vtypeFrom != t_ref );
        AssertIfFalse( vtypeFrom != t_string );
        Helper = GetHelperToBoolean( vtypeFrom );
    }
    else if( vtypeFrom == t_single && IsIntegralType( vtypeTo ) )
    {
        Helper = GetHelperCastFromSingle( vtypeTo );
    }
    else if( vtypeFrom == t_double && IsIntegralType( vtypeTo ) )
    {
        Helper = GetHelperCastFromDouble( vtypeTo );
    }

    return( Helper );
}

template<class TREE_TYPE> Procedure *
ExpressionTreeSemantics<TREE_TYPE>::GetRuntimeMemberProcedure( RuntimeMembers Helper, const Location &Loc, bool fReportErrors = true )
{
    BCSYM_NamedRoot *Root = m_CompilerHost->GetSymbolForRuntimeMember( Helper, m_CompilationCaches );
    if ( !Root )
    {
        if (fReportErrors)
        {
            ReportErrorForMissingHelper( Loc, Helper );
        }
        return NULL;
    }

    return ViewAsProcedure( Root );
}

// ----------------------------------------------------------------------------
// Determine whether this opcode is a boolean operator because boolean
// nodes have extra parameters that we need to encode.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::IsBooleanOperator( BILOP bilop )
{
    if( bilop == SX_IS ||
        bilop == SX_ISNOT ||
        bilop == SX_EQ ||
        bilop == SX_NE ||
        bilop == SX_LE ||
        bilop == SX_GE ||
        bilop == SX_LT ||
        bilop == SX_GT
      )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// This method returns true if the opcode is a binary operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::IsBinaryOperator( BILOP opcode )
{
    if( opcode == SX_AND ||
        opcode == SX_OR ||
        opcode == SX_XOR ||
        opcode == SX_POW ||
        opcode == SX_MUL ||
        opcode == SX_ADD ||
        opcode == SX_SUB ||
        opcode == SX_DIV ||
        opcode == SX_MOD ||
        opcode == SX_IDIV ||
        opcode == SX_CONC ||
        opcode == SX_SHIFT_LEFT ||
        opcode == SX_SHIFT_RIGHT
      )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// When SX_NOTHING is used with certain binary operators, the compiler
// creates a new structure and uses this structure to do the operation. This
// method returns true for the opcodes that do this.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::OpcodeHasSpecialStructureSemantics( BILOP opcode )
{
    if( IsBinaryOperator( opcode ) || IsBooleanOperator( opcode ) )
    {
        // But not SX_IS, SX_ISNOT.

        if( opcode != SX_IS && opcode != SX_ISNOT )
        {
            return( true );
        }
    }

    return( false );
}

// ----------------------------------------------------------------------------
// This method walks through a tree and sees if we cast from SX_NOTHING.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::DigAndCheckIfItIsNothing( ILTree::Expression* PotentialNothing )
{
    ThrowIfNull( PotentialNothing );

    while(
        PotentialNothing->bilop == SX_CTYPE ||
        PotentialNothing->bilop == SX_NOTHING
        )
    {
        if( PotentialNothing->bilop == SX_NOTHING )
        {
            return( true );
        }
        else
        {
            PotentialNothing = PotentialNothing->AsExpressionWithChildren().Left;
        }
    }

    return( false );
}

// ----------------------------------------------------------------------------
// Check whether the type is a small integral type.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::IsSmallIntegralType( Vtypes vt )
{
    if( vt == t_i1 || vt == t_ui1 )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// Set the expression tree error.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> void
ExpressionTreeSemantics<TREE_TYPE>::ReportErrorForExpressionTree( ERRID err, const Location &Loc )
{
    if ( m_LastExprTreeErrID == 0 )
    {
        m_LastExprTreeErrID = err;
        m_Semantics->ReportSemanticError(err, Loc);
    }
}

template<class TREE_TYPE> void
ExpressionTreeSemantics<TREE_TYPE>::ReportErrorForMissingHelper(const Location &Loc, RuntimeMembers Helper)
{
    if ( m_LastExprTreeErrID == 0 )
    {
        m_LastExprTreeErrID = ERRID_MissingRuntimeHelper;

        StringBuffer NameOfMissingType;
        NameOfMissingType.AppendString( g_rgRTLangClasses[ g_rgRTLangMembers[ Helper ].rtParent ].wszClassName );
        NameOfMissingType.AppendChar( L'.' );
        NameOfMissingType.AppendString( g_rgRTLangMembers[ Helper ].wszName );
        m_Semantics->ReportSemanticError(
            ERRID_MissingRuntimeHelper,
            Loc,
            NameOfMissingType.GetString()
            );
    }
}


// ----------------------------------------------------------------------------
// Generate a cast to Int32 if the type is i1 or ui1.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::GenerateCastToInteger(
    Vtypes vtype,
    bool IsNullable,
    TREE_TYPE *TreeInput,
    const Location &Loc,
    Type* TargetType
    )
{
    ThrowIfNull( TreeInput );

    if( !IsSmallIntegralType( vtype ) )
    {
        return( TreeInput );
    }

    Type* intType = NULL;

    if( IsNullable )
    {
        intType = GetFXSymbolProvider()->GetNullableIntrinsicSymbol( t_i4 );
    }
    else
    {
        intType = GetFXSymbolProvider()->GetIntegerType();
    }

    return m_ExprTreeGenerator->CreateConvertExpr(
        TreeInput,
        intType,
        !m_NoIntChecks,
        TargetType,
        Loc
        );
}

// ----------------------------------------------------------------------------
// If we did cast to i4, cast back.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::GenerateCastBackFromInteger(
    ILTree::Expression* Input,
    TREE_TYPE *TreeInput,
    Type* TargetType
    )
{
    TREE_TYPE *Result = TreeInput;
    Vtypes vtypeOp = GetVtypeOfTree( Input );
    bool IsNullable = TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost );

    // If we casted up to i4, we need to cast back.

    if( IsSmallIntegralType( vtypeOp ) )
    {
        Type* typeToUse = NULL;

        switch( vtypeOp )
        {
        case t_i1:
            if( IsNullable )
            {
                typeToUse = GetFXSymbolProvider()->GetNullableIntrinsicSymbol( t_i1 );
            }
            else
            {
                typeToUse = GetFXSymbolProvider()->GetSignedByteType();
            }
            break;

        case t_ui1:
            if( IsNullable )
            {
                typeToUse = GetFXSymbolProvider()->GetNullableIntrinsicSymbol( t_ui1 );
            }
            else
            {
                typeToUse = GetFXSymbolProvider()->GetByteType();
            }
            break;
        }

        // A left or right shift operation's result cannot overflow according to spec. So we should just
        // cast the result back from integer without any check semantics.
        Result = m_ExprTreeGenerator->CreateConvertExpr(
            TreeInput,
            typeToUse,
            !(m_NoIntChecks || Input->bilop == SX_SHIFT_LEFT || Input->bilop == SX_SHIFT_RIGHT),
            TargetType,
            Input->Loc
            );
    }

    return( Result );
}

// ----------------------------------------------------------------------------
// Generate a cast node for the enum conversion. This is required because the
// expression tree API does not allow enum types in intrinsic operators; thus
// we need to generate a cast to the underlying enum type. This assumes that
// the TreeInput argument is already converted to an expression tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::GenerateEnumConversionIfNeeded(
    ILTree::Expression* Input,
    TREE_TYPE *TreeInput,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfNull( TreeInput );

    TREE_TYPE *Result = TreeInput;

    // Check if the symbol type is an enum; if it is, we must cast to the
    // underlying type. But don't do this if Input is a SX_WIDE_COERCE,
    // because SX_WIDE_COERCE is already converted to an expression tree.

    if( Input->bilop != SX_WIDE_COERCE &&
        Input->ResultType->IsClass() &&
        Input->ResultType->PClass()->IsEnum() )
    {
        BCSYM_NamedRoot * pUnderlyingType = NULL;

        // Find the underlying type.

        if( Input->ResultType->PClass()->IsIntrinsicType() )
        {
            pUnderlyingType = GetFXSymbolProvider()->GetType(
                Input->ResultType->PClass()->GetVtype());
        }
        else
        {
            BCSYM_NamedRoot* pValue = Input->ResultType->PClass()->SimpleBind(
                NULL,
                STRING_CONST( m_Compiler, EnumValueMember )
                );

            if( pValue != NULL && pValue->IsMember() && pValue->PMember()->GetRawType() )
            {
                BCSYM* ptyp = pValue->PMember()->GetType();
                if( ptyp != NULL && ptyp->IsNamedRoot() )
                {
                    pUnderlyingType = ptyp->PNamedRoot();
                }
            }
        }

        VSASSERT( pUnderlyingType != NULL, "Why can't we find the underlying type for this enum?" );
        VSASSERT( pUnderlyingType->IsIntrinsicType(), "Why is the underlying type not intrinsic?" );

        Result = m_ExprTreeGenerator->CreateCastExpr(
            TreeInput,
            pUnderlyingType,
            TargetType,
            Input->Loc
            );
    }

    ThrowIfNull( Result );
    return( Result );
}

// ----------------------------------------------------------------------------
// Convert an array index to an expression tree. This handles single dimension
// as well as multi-dimension.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertArrayIndex(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_INDEX );

    TREE_TYPE *Result = NULL;

    TREE_TYPE *Target = ConvertInternalToExpressionTree(
        ( ILTree::Expression* )Input->AsExpressionWithChildren().Left,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    ExpressionList* IndexExpressions = ( ExpressionList* )Input->AsExpressionWithChildren().Right;
    ThrowIfNull( IndexExpressions );

    ParseTree::Expression* IndexArray = NULL;

    // Single dimension index goes through a different constructor.

    if( IndexExpressions->AsExpressionWithChildren().Right == NULL )
    {
        Result = m_ExprTreeGenerator->CreateArrayIndexExpr(
            Target,
            ConvertInternalToExpressionTree(
                IndexExpressions->AsExpressionWithChildren().Left,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                ),
            TargetType,
            Input->Loc
            );                    
    }
    else
    {
        NorlsAllocator Scratch(NORLSLOC);
        ArrayList_NorlsAllocBased<TREE_TYPE *> Indices(&Scratch);

        while( IndexExpressions != NULL )
        {
            if( IndexExpressions->bilop != SX_LIST )
            {
                VSASSERT( false, "how did we get an SX_LIST in an array?" );
                return( false );
            }

            TREE_TYPE *Index = ConvertInternalToExpressionTree(
                IndexExpressions->AsExpressionWithChildren().Left,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                );

            Indices.Add(Index);
            IndexExpressions = IndexExpressions->AsExpressionWithChildren().Right;
        }

        Result = m_ExprTreeGenerator->CreateArrayIndexExpr(
            Target,
            &Indices,
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert array index calls.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertArrayLength(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ARRAYLEN );

    TREE_TYPE *Result = NULL;

    TREE_TYPE *ArrayExpr = ConvertInternalToExpressionTree(
        Input->AsExpressionWithChildren().Left,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    // For long length, we need to handle this specially by generating a call
    // expression instead of an ArrayLength.

    if( Input->ResultType == GetFXSymbolProvider()->GetLongType() )
    {
        Procedure* ArrayLongLengthProperty = m_Semantics->FindHelperMethod(
            STRING_CONST( m_Compiler, LongLength ),
            GetFXSymbolProvider()->GetRootArrayType()->PClass(),
            Input->Loc,
            false
            );

        ThrowIfNull( ArrayLongLengthProperty );

        Result = m_ExprTreeGenerator->CreateCallExpr(
            ArrayLongLengthProperty->PProperty()->GetProperty(),
            NULL,   // No generic binding context
            ArrayExpr,
            NULL,   // No Arguments for this method
            TargetType,
            Input->Loc);
    }
    else
    {
        Result = m_ExprTreeGenerator->CreateArrayLengthExpr(ArrayExpr, TargetType, Input->Loc);
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Assignments don't get generated, so just reference the value being
// assigned.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertAssignment(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse(
        Input->bilop == SX_ASG ||
        Input->bilop == SX_ASG_RESADR
        );

    Declaration* Target = Input->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().pnamed;

    if( IsVariable( Target ) && Target->PVariable()->IsTemporary() )
    {
        // Temporary assignment doesn't appear in the tree, just return the value
        // being assigned.

        return( ConvertInternalToExpressionTree( Input->AsExpressionWithChildren().Right, Flags, TargetType ) );
    }
    else
    {
        // SINGLESUB: An assignment "x=y" might now appear in an expression-tree, thanks to single-line subs...
        // And for all cases where we might return null, we're supposed to report why.
        ReportErrorForExpressionTree(ERRID_ExpressionTreeNotSupported, Input->Loc);
        return( NULL );
    }
}

// ----------------------------------------------------------------------------
// Convert the binary operators.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertBinaryOperator(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_AND ||
                  Input->bilop == SX_OR ||
                  Input->bilop == SX_XOR ||
                  Input->bilop == SX_POW ||
                  Input->bilop == SX_MUL ||
                  Input->bilop == SX_ADD ||
                  Input->bilop == SX_SUB ||
                  Input->bilop == SX_DIV ||
                  Input->bilop == SX_MOD ||
                  Input->bilop == SX_IDIV ||
                  Input->bilop == SX_CONC ||
                  Input->bilop == SX_SHIFT_LEFT ||
                  Input->bilop == SX_SHIFT_RIGHT
                  );

    RuntimeMembers Helper = UndefinedRuntimeMember;
    ILTree::Expression* Left = Input->AsExpressionWithChildren().Left;
    ILTree::Expression* Right = Input->AsExpressionWithChildren().Right;
    Vtypes vtypeOp = GetVtypeOfTree( Input );

    if( vtypeOp == t_ref )
    {
        Helper = CodeGenerator::GetHelperForObjBinOp( Input->bilop );
    }
    else if( vtypeOp == t_decimal )
    {
        Helper = CodeGenerator::GetHelperForDecBinOp( Input->bilop );
    }
    else
    {
        switch( Input->bilop )
        {
        case SX_CONC:
            Helper = StringConcatenationMember;
            break;

        case SX_POW:
            Helper = NumericPowerMember;
            break;

        default:
            break;
        }
    }

    if( Helper != UndefinedRuntimeMember )
    {
        return( ConvertRuntimeHelperOperatorToExpressionTree(
                Input,
                Helper,
                Input->Loc,
                Left,
                Right,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        TREE_TYPE *Result = NULL;
        Vtypes vtypeLeft = GetVtypeOfTree( Left );
        Vtypes vtypeRight = GetVtypeOfTree( Right );

        // For binary operators, we have to generate the cast to the enum's underlying
        // type for the expression tree to work. Some binary operators return an enum
        // type, so we need to cast back to an enum.

        // In addition, if the types are i1, i2, ui1, or ui2, we must generate a cast to
        // i4 or ui4.

        TREE_TYPE *ExprTreeLeft = GenerateCastToInteger(
            vtypeLeft,
            TypeHelpers::IsNullableType( Left->ResultType, m_CompilerHost ),
            GenerateEnumConversionIfNeeded(
                Left,
                ConvertInternalToExpressionTree( Left, Flags, NULL ),
                TargetType
                ),
            Input->Loc,
            TargetType
            );

        TREE_TYPE *ExprTreeRight = GenerateCastToInteger(
            vtypeRight,
            TypeHelpers::IsNullableType( Right->ResultType, m_CompilerHost ),
            GenerateEnumConversionIfNeeded(
                Right,
                ConvertInternalToExpressionTree( Right, Flags, NULL ),
                TargetType
                ),
            Input->Loc,
            TargetType
            );

        Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
            Input->bilop,
            ExprTreeLeft,
            ExprTreeRight,
            UseCheckedSemantics( Input ),
            TargetType,
            Input->Loc
            );

        // We need to generate the cast back.

        Result = GenerateCastBackFromInteger(
            Input,
            Result,
            TargetType
            );

        // If we are an enum, we need to cast the result back to the enum type,
        // because the expression tree API will return the underlying type as the
        // result of the operation.

        if( Input->ResultType->IsClass() &&
            Input->ResultType->PClass()->IsEnum() )
        {
            Result = m_ExprTreeGenerator->CreateCastExpr(
                Result,
                Input->ResultType,
                TargetType,
                Input->Loc
                );
        }

        return( Result );
    }
}

// ----------------------------------------------------------------------------
// Convert the boolean operators.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertBooleanOperator(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_IS ||
                  Input->bilop == SX_ISNOT ||
                  Input->bilop == SX_EQ ||
                  Input->bilop == SX_NE ||
                  Input->bilop == SX_LE ||
                  Input->bilop == SX_GE ||
                  Input->bilop == SX_LT ||
                  Input->bilop == SX_GT
                  );

    RuntimeMembers Helper = UndefinedRuntimeMember;
    BILOP bilop = Input->bilop;
    ILTree::Expression* Left = Input->AsExpressionWithChildren().Left;
    ILTree::Expression* Right = Input->AsExpressionWithChildren().Right;
    Vtypes vtypeOp = GetVtypeOfTree( Input );
    Vtypes vtypeLeft = GetVtypeOfTree( Left );
    Vtypes vtypeRight = GetVtypeOfTree( Right );

    RuntimeMembers CompareStringHelper = m_CompilerHost->GetSymbolForRuntimeClass(VBEmbeddedOperatorsClass, NULL)? EmbeddedCompareStringMember : CompareStringMember;
    // We will always lift to null unless we are using IS or ISNOT.

    bool bLiftToNull = true;

    if( Input->bilop == SX_IS || Input->bilop == SX_ISNOT )
    {
        bLiftToNull = false;
    }

    if( vtypeLeft == t_ref && Input->bilop != SX_IS && Input->bilop != SX_ISNOT )
    {
        return( ConvertBooleanRuntimeHelperToExpressionTree(
                Input,
                CodeGenerator::GetHelperForObjRelOp( bilop, vtypeOp ),
                Input->Loc,
                Left,
                Right,
                Flags,
                TargetType
                )
            );
    }
    else if( vtypeLeft == t_decimal )
    {
        // If we have a decimal, we must use the operators defined instead of
        // the Decimal.Compare method.

        switch( Input->bilop )
        {
        case SX_EQ:
            Helper = DecimalEqual;
            break;

        case SX_NE:
            Helper = DecimalInequality;
            break;

        case SX_LE:
            Helper = DecimalLessThanOrEqual;
            break;

        case SX_LT:
            Helper = DecimalLessThan;
            break;

        case SX_GE:
            Helper = DecimalGreaterThanOrEqual;
            break;

        case SX_GT:
            Helper = DecimalGreaterThan;
            break;

        default:
            // Nullable falls in here.
            break;
        }
    }
    else if( vtypeLeft == t_date )
    {
        // If we have a date time, we must use the operators defined instead
        // of the DateTime.Compare method.

        switch( Input->bilop )
        {
        case SX_EQ:
            Helper = DateTimeEqual;
            break;

        case SX_NE:
            Helper = DateTimeInequality;
            break;

        case SX_LE:
            Helper = DateTimeLessThanOrEqual;
            break;

        case SX_LT:
            Helper = DateTimeLessThan;
            break;

        case SX_GE:
            Helper = DateTimeGreaterThanOrEqual;
            break;

        case SX_GT:
            Helper = DateTimeGreaterThan;
            break;

        default:
            // Nullable falls in here.
            break;
        }
    }
    else if( vtypeLeft == t_string )
    {
        Helper = CompareStringHelper;
    }
    else if( vtypeLeft == t_bool )
    {
        // Because True is -1, we need to switch the comparisons.
        // Note that these calls are not converted to runtime helpers.

        if( bilop == SX_LT ) bilop = SX_GT;
        else if( bilop == SX_GT ) bilop = SX_LT;
        else if( bilop == SX_LE ) bilop = SX_GE;
        else if( bilop == SX_GE ) bilop = SX_LE;
    }

    if( Helper != UndefinedRuntimeMember )
    {
        return( ConvertBooleanRuntimeHelperToExpressionTree(
                Input,
                Helper,
                Input->Loc,
                Left,
                Right,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        bool bConvertToInteger = false;
        bool bLeftIsNullable = TypeHelpers::IsNullableType( Left->ResultType, m_CompilerHost );
        bool bRightIsNullable = TypeHelpers::IsNullableType( Right->ResultType, m_CompilerHost );
        TREE_TYPE *ExprTreeLeft = NULL;
        TREE_TYPE *ExprTreeRight = NULL;

        VSASSERT(
            bLeftIsNullable || bRightIsNullable || ( vtypeLeft == vtypeRight ),
            "How can we have different types for boolean operations?"
            );

        // For LT, LE, GT, and GE, if both the arguments are boolean arguments,
        // we must generate a conversion from the boolean argument to an integer.

        if( vtypeLeft == t_bool &&
            vtypeRight == t_bool &&
            ( Input->bilop == SX_LT || Input->bilop == SX_LE || Input->bilop == SX_GT || Input->bilop == SX_GE ) )
        {
            bConvertToInteger = true;
        }

        ExprTreeLeft = ConvertInternalToExpressionTree( Left, Flags, NULL );
        ExprTreeRight = ConvertInternalToExpressionTree( Right, Flags, NULL );

        // When encoding a boolean operation, enum types need to be casted to their
        // underlying types. In addition, SX_NOTHING comes in here with type mismatches
        // against boolean, so we must adjust.

        if( ( Input->bilop == SX_IS || Input->bilop == SX_ISNOT ) &&
            ( bLeftIsNullable || bRightIsNullable ) )
        {
            // Find the expression that is the nothing expression, and emit a cast
            // to the nullable type of the other expression.
            TREE_TYPE **NothingExpr = bLeftIsNullable ? &ExprTreeRight : &ExprTreeLeft;
            Type* NullableType = bLeftIsNullable ? Left->ResultType : Right->ResultType;
            ILTree::Expression* NothingLiteral = bLeftIsNullable ? Right : Left;

            AssertIfFalse(NothingLiteral->bilop == SX_NOTHING);
            ThrowIfFalse(NothingLiteral->bilop == SX_NOTHING);

            // Dev10 #468350 Use constant expression rather than conversion from Nothing to the target type.            
            *NothingExpr = m_ExprTreeGenerator->CreateNothingConstantExpr(NullableType, TargetType, NothingLiteral->Loc);
        }

        ExprTreeLeft = GenerateEnumConversionIfNeeded(
            Left,
            ExprTreeLeft,
            TargetType
            );

        ExprTreeRight = GenerateEnumConversionIfNeeded(
            Right,
            ExprTreeRight,
            TargetType
            );

        // Check if we need to convert the boolean arguments to Int32.

        if( bConvertToInteger )
        {
            ExprTreeLeft = m_ExprTreeGenerator->CreateConvertExpr(
                ExprTreeLeft,
                bLeftIsNullable ?
                    GetFXSymbolProvider()->GetNullableIntrinsicSymbol( t_i4 ) :
                    GetFXSymbolProvider()->GetIntegerType(),
                !m_NoIntChecks,
                TargetType,
                Input->Loc
                );

            ExprTreeRight = m_ExprTreeGenerator->CreateConvertExpr(
                ExprTreeRight,
                bRightIsNullable ?
                    GetFXSymbolProvider()->GetNullableIntrinsicSymbol( t_i4 ) :
                    GetFXSymbolProvider()->GetIntegerType(),
                !m_NoIntChecks,
                TargetType,
                Input->Loc
                );
        }

        // We must use the overload that allows us to set LiftToNull = true, even
        // though we will pass Nothing as the operator.

        return m_ExprTreeGenerator->CreateBinaryOperatorExpr(
            bilop,
            ExprTreeLeft,
            ExprTreeRight,
            UseCheckedSemantics( Input ),
            bLiftToNull,
            NULL,   // no helper method
            NULL,   // no generic binding context
            TargetType,
            Input->Loc
            );
    }
}

// ----------------------------------------------------------------------------
// Convert the short circuit operators.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertShortCircuitedBooleanOperator(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ANDALSO ||
                 Input->bilop == SX_ORELSE
        );

    ILTree::Expression* Left = Input->AsExpressionWithChildren().Left;
    ILTree::Expression* Right = Input->AsExpressionWithChildren().Right;
    Vtypes vtypeOp = GetVtypeOfTree( Input );

    // Check if this is a user defined operator.

    if( Input->IsUserDefinedOperator() )
    {
        VSASSERT( Input->AsUserDefinedBinaryOperatorExpression().OperatorMethod != NULL, "How can the method be missing?" );

        Procedure* TargetMethod = Input->AsUserDefinedBinaryOperatorExpression().OperatorMethod;
        if( TargetMethod->IsLiftedOperatorMethod() )
        {
            TargetMethod = TargetMethod->PLiftedOperatorMethod()->GetActualProc();
            ThrowIfNull( TargetMethod );
        }

        return( ConvertUserDefinedOperatorToExpressionTree(
                Input,
                m_Semantics->ReferToSymbol(
                    Input->Loc,
                    TargetMethod,
                    chType_NONE,
                    NULL,
                    Input->AsUserDefinedBinaryOperatorExpression().OperatorMethodContext,
                    ExprIsExplicitCallTarget
                    ),
                Input->Loc,
                Left,
                Right,
                Flags,
                TargetType
                )
            );
    }

    if( vtypeOp == t_ref )
    {
        Left = m_Semantics->ConvertWithErrorChecking(
            Left,
            GetFXSymbolProvider()->GetBooleanType(),
            ExprHasExplicitCastSemantics
            );

        Right = m_Semantics->ConvertWithErrorChecking(
            Right,
            GetFXSymbolProvider()->GetBooleanType(),
            ExprHasExplicitCastSemantics
            );
    }

    ThrowIfNull( Left );
    ThrowIfNull( Right );

    TREE_TYPE *ExprTreeLeft = ConvertInternalToExpressionTree(
        Left,
        Flags,
        NULL
        );

    TREE_TYPE *ExprTreeRight = ConvertInternalToExpressionTree(
        Right,
        Flags,
        NULL
        );

    TREE_TYPE *Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
        Input->bilop,
        ExprTreeLeft,
        ExprTreeRight,
        false,  // No checked semantics for short circuit operators
        TargetType,
        Input->Loc);

    // Now if we had an object expression, we should convert it back.

    if( vtypeOp == t_ref )
    {
        Result = m_ExprTreeGenerator->CreateCastExpr(
            Result,
            GetFXSymbolProvider()->GetObjectType(),
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert the IIF operator. This should only be called for a real IIF, not
// for the nullable coalesce operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertIIF(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_IIF );

    TREE_TYPE *ExprTreeCondition = ConvertInternalToExpressionTree(
        Input->AsIfExpression().condition,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *ExprTreeLeft = ConvertInternalToExpressionTree(
        Input->AsExpressionWithChildren().Left,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *ExprTreeRight = ConvertInternalToExpressionTree(
        Input->AsExpressionWithChildren().Right,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    return m_ExprTreeGenerator->CreateTernaryIfExpr(
        ExprTreeCondition,
        ExprTreeLeft,
        ExprTreeRight,
        TargetType,
        Input->Loc
        );
}

// ----------------------------------------------------------------------------
// This handles the specific IsTrue/IsFalse "fake" nodes that are created when
// interpreting condition operators for boolean types.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertIsTrue(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ISTRUE );

    TREE_TYPE *Result = NULL;

    // The strategy is to take the Left and build a Coalesce operator if the
    // operand is a boolean? type. If it is not, then we also need to encode
    // the method conversion for the coalesce operator.

    // Here's how it looks:
    // IsTrue: Convert(Coalesce(op,false), boolean)
    // If op is not a boolean?, then it must have IsTrue defined, and thus,
    // we pass this to the Coalesce operator, which will perform a left -> right
    // conversion. This method must be generated as a lambda.

    ILTree::Expression* Operand = m_Semantics->FindOperandForIsTrueOperator( Input );

    TREE_TYPE *ExprTreeOperand = ConvertInternalToExpressionTree(
        Operand,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *ExprTreeFalseLiteral = ConvertInternalToExpressionTree(
        m_Semantics->ProduceConstantExpression(
            0,
            Input->Loc,
            GetFXSymbolProvider()->GetBooleanType()
            IDE_ARG(0)
            ),
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    // Check if we have a operator.

    Procedure* IsTrue = m_Semantics->FindRealIsTrueOperator( Input );

    if( IsTrue != NULL )
    {
        // We found a real IsTrue operator. Build the lambda for this. The lambda takes you from
        // x to y, where y must be a boolean type.

        LambdaParamHandle *LambdaParameter = m_ExprTreeGenerator->CreateLambdaParameter(
            STRING_CONST( m_Compiler, CoalesceLambdaArg ),
            Operand->ResultType,
            Input->Loc
            );

        NorlsAllocator Scratch(NORLSLOC);

        // The body of this lambda is a call to the operator with the argument being the lambda
        // parameter.

        TREE_TYPE *LambdaBody = BuildCallExpressionFromMethod(
            IsTrue,
            Operand,
            LambdaParameter,
            Input->Loc,
            Scratch
            );

        // Now that we have the body of the lambda, we need to build the actual lambda
        // node on top of it.

        TREE_TYPE *LambdaExpression = CreateLambdaForCoalesce(
            LambdaBody,
            LambdaParameter,
            Operand->ResultType,
            GetFXSymbolProvider()->GetBooleanType(),
            TargetType,
            Input->Loc,
            Scratch
            );

        Result = m_ExprTreeGenerator->CreateCoalesceExpr(
            ExprTreeOperand,
            ExprTreeFalseLiteral,
            LambdaExpression,
            TargetType,
            Input->Loc
            );
    }
    else
    {
        Result = m_ExprTreeGenerator->CreateCoalesceExpr(
            ExprTreeOperand,
            ExprTreeFalseLiteral,
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert the Coalesce operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCoalesce(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_IIFCoalesce );

    // We should first load the elements of the coalesce operator. This is an API that breaks
    // up the arguments and generates the appropriate conversions. We pass true to this API
    // so that no temporaries are generated, and the conversions are specified in terms of
    // the operands. This will let us figure out how to build the appropriate lambda.

    ILTree::Expression* Condition = NULL;
    ILTree::Expression* TrueExpr = NULL;
    ILTree::Expression* FalseExpr = NULL;

    m_Semantics->LoadIIfElementsFromIIfCoalesce(
        Input,
        Condition,
        TrueExpr,
        FalseExpr,
        true
        );

    VSASSERT( Input->AsExpressionWithChildren().Left == Condition, "The condition should be the same as the Input in this case!" );
    VSASSERT( Condition, "How come no condition is generated?" );
    VSASSERT( TrueExpr, "How come no TrueExpr is generated?" );
    VSASSERT( FalseExpr, "How come no FalseExpr is generated?" );

    // The results of this API is that condition is the first operand to coalesce, and
    // FalseExpr is the second operand to coalesce. TrueExpr contains the conversions
    // necessary to convert from the first operand to the second operand.

    TREE_TYPE *ExprTreeCondition = ConvertInternalToExpressionTree(
        Condition,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *ExprTreeFalseExpr = ConvertInternalToExpressionTree(
        FalseExpr,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *Result = NULL;

    if( TrueExpr != Condition )
    {
        // Here, we must build a lambda that does this: (left) => convert(left).
        // This is the high level, the implementation is slightly different.

        // 1. We build the lambda parameter, which has the type of left.
        // 2. We have to walk the TrueExpr tree, find the sub part that is Condition, and
        //    replace that with a symbol to the lambda.
        // 3. We can convert the TrueExpr tree into an expression.
        // 4. We can then build lambda expression on top of that.

        LambdaParamHandle *LambdaParameter = m_ExprTreeGenerator->CreateLambdaParameter(
            STRING_CONST( m_Compiler, CoalesceLambdaArg ),
            Condition->ResultType,
            Input->Loc
            );

        // Now that we have the temporary, we have to replace the argument in the chain of
        // conversions with an SX_SYM that is the temporary. This will allow the normal
        // expression tree interpretation to work.
        // Unfortunately, the easiest way to do this is to set a special variable which
        // holds the condition, and then call ConvertInternalToExpressionTree. It will
        // look for this variable, and if it is non-null, replace it with the lambda
        // parameter that is set.

        BackupValue< LambdaParamHandle* > backup_variable( &m_CoalesceLambdaParameter );
        BackupValue< ILTree::Expression* > backup_expr( &m_CoalesceArgumentToBeReplaced );

        m_CoalesceLambdaParameter = LambdaParameter;
        m_CoalesceArgumentToBeReplaced = Condition;

        TREE_TYPE *LambdaBody = ConvertInternalToExpressionTree(
            TrueExpr,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        // Now that we have the body of the lambda, we need to build the actual lambda
        // node on top of it.

        NorlsAllocator Scratch(NORLSLOC);

        TREE_TYPE *LambdaExpression = CreateLambdaForCoalesce(
            LambdaBody,
            LambdaParameter,
            Condition->ResultType,
            FalseExpr->ResultType,
            TargetType,
            Input->Loc,
            Scratch
            );

        Result = m_ExprTreeGenerator->CreateCoalesceExpr(
            ExprTreeCondition,
            ExprTreeFalseExpr,
            LambdaExpression,
            TargetType,
            Input->Loc
            );
    }
    else
    {
        Result = m_ExprTreeGenerator->CreateCoalesceExpr(
            ExprTreeCondition,
            ExprTreeFalseExpr,
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert the SX_CTYPEOP node, which is a nullable conversion.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCTypeOp(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_CTYPEOP );

    // An SX_CTYPEOP is always a user-defined conversion. Thus, we must build
    // the method-accepting version of the call.

    VSASSERT( Input->AsUserDefinedUnaryOperatorExpression().OperatorMethod != NULL,
        "How come there is no method for SX_CTYPEOP?" );
    VSASSERT( TypeHelpers::IsNullableType( Input->AsExpressionWithChildren().Left->ResultType, m_CompilerHost ),
        "How come the operand for SX_CTYPEOP is not a nullable type?" );
    VSASSERT( TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost ),
        "How come the result for SX_CTYPEOP is not a nullable type?" );

    ILTree::Expression* ChainedConversions = NULL;

    // Use a helper to build a set of chained expressions, since the CTYPEOP node
    // only contains the main user-defined operators, and not the implicit conversions
    // around it.

    ChainedConversions = m_Semantics->ConvertUsingConversionOperatorWithNullableTypes(
        Input->AsExpressionWithChildren().Left,
        Input->ResultType,
        Input->AsUserDefinedUnaryOperatorExpression().OperatorMethod,
        Input->AsUserDefinedUnaryOperatorExpression().OperatorMethodContext,
        Flags
        );

    return( ConvertInternalToExpressionTree(
            ChainedConversions,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            )
        );

}

// ----------------------------------------------------------------------------
// Convert the SX_ANONYMOUSTYPE node.
// We do not convert anonymous types which have temps, because the expression
// tree does not support this. These are anonymous types in which an earlier
// field is used to initialize a later field. C# disallows this altogether
// in their anonymous type implementation.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertAnonymousType(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ANONYMOUSTYPE );

    ILTree::AnonymousTypeExpression* AnonymousType = &Input->AsAnonymousTypeExpression();

    // First, let's see if any of the nodes contain temps.

    bool ContainsTemp = false;

    for( ULONG i = 0; i < AnonymousType->BoundMembersCount; i += 1 )
    {
        if( AnonymousType->BoundMembers[ i ].Temp != NULL )
        {
            ContainsTemp = true;
            break;
        }
    }

    if( ContainsTemp )
    {
        ReportErrorForExpressionTree( ERRID_BadAnonymousTypeForExprTree, Input->Loc );
        return( NULL );
    }

    // Now that we know we have no temps, we can construct the anonymous type node.
    // We need to match expressions with their properties in the expression tree
    // constructor.

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<PropertyAssignmentInfo<TREE_TYPE>> Initializations(&Scratch);
    
    for( ULONG i = 0; i < AnonymousType->BoundMembersCount; i += 1 )
    {
        VSASSERT(
            AnonymousType->BoundMembers[ i ].Temp == NULL,
            "Oops, we should never have temps for Anonymous types in expression trees."
            );

        PropertyAssignmentInfo<TREE_TYPE> InitInfo;

        InitInfo.PropertyGetter = AnonymousType->BoundMembers[ i ].Property->GetProperty();
        InitInfo.BindingContext = AnonymousType->AnonymousTypeBinding;

        InitInfo.Value = ConvertInternalToExpressionTree(
            AnonymousType->BoundMembers[ i ].BoundExpression,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        Initializations.Add(InitInfo);
    }

    return m_ExprTreeGenerator->CreateNewExpr(
        AnonymousType->Constructor,
        AnonymousType->AnonymousTypeBinding->PGenericTypeBinding(),
        &Initializations,
        TargetType,
        Input->Loc
        );
}

template<class TREE_TYPE> IReadOnlyList<TREE_TYPE*>*
ExpressionTreeSemantics<TREE_TYPE>::CreateArgumentListForExpressionTree
(
    ExpressionList * ArgumentExpressions,
    ExpressionFlags Flags,
    Symbol * TargetType,
    BCSYM_Proc * TargetMethod,    
    const Location &Loc,
    NorlsAllocator &Allocator
)
{
    // We need to encode the arguments.

    ArrayList_NorlsAllocBased<TREE_TYPE*> *ArgumentList = NULL;

    Parameter* CurrentParameter = TargetMethod->GetFirstParam();
    while( ArgumentExpressions != NULL )
    {
        if( ArgumentExpressions->bilop != SX_LIST )
        {
            VSASSERT( false, "How did this node get here?" );
            return NULL;
        }

        // We must check here: it could be that this method takes a structure
        // type, and the argument is SX_NOTHING. We must replace this with a new structure
        // call, if this is the case.

        Type* RealTypeOfParameter = NULL;
        ILTree::Expression* Argument = NULL;

        if( CurrentParameter->GetType()->IsPointerType())
        {
            Argument = ExtractArgumentForByrefParam(ArgumentExpressions->AsExpressionWithChildren().Left);

            // Dev10 #468366 Grab type from expression tree, it will have generics type arguments properly substituted, whereas 
            // type from Parameter symbol may still contain unsubstituted generic type arguments. For example, method's type 
            // arguments.
            RealTypeOfParameter = Argument->ResultType->IsPointerType() ? 
                                                        Argument->ResultType->PPointerType()->GetRoot() : 
                                                        Argument->ResultType;
        }
        else
        {
            Argument = ArgumentExpressions->AsExpressionWithChildren().Left;

            // Dev10 #468366 Grab type from expression tree, it will have generics type arguments properly substituted, whereas 
            // type from Parameter symbol may still contain unsubstituted generic type arguments.
            RealTypeOfParameter = Argument->ResultType;
        }

        TREE_TYPE *ExprTreeArgument = NULL;

        if
        (
            RealTypeOfParameter->IsStruct() &&
            DigAndCheckIfItIsNothing(Argument) 
        )
        {
            ExprTreeArgument = CreateNewStructureType(
                RealTypeOfParameter,
                Loc,
                TargetType
                );
        }
        else
        {
            ExprTreeArgument = ConvertInternalToExpressionTree(
                ArgumentExpressions->AsExpressionWithChildren().Left,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                );
        }

        if (ArgumentList == NULL)
        {
            ArgumentList = new (Allocator) ArrayList_NorlsAllocBased<TREE_TYPE*>(&Allocator);
        }

        ArgumentList->Add(ExprTreeArgument);

        ArgumentExpressions = ArgumentExpressions->AsExpressionWithChildren().Right;
        CurrentParameter = CurrentParameter->GetNext();
    }

    return ArgumentList;
}

template<class TREE_TYPE> ILTree::Expression*
ExpressionTreeSemantics<TREE_TYPE>::ExtractArgumentForByrefParam
(
    ILTree::Expression *Argument
)
{
    ThrowIfNull(Argument);

    // If the argument has an assignment due to type mismatch or property get for byref
    // parameters, then ignore the assignment and directly return the actual value.
    //
    // The assignment is ignored in this way, since copy back is anyway not supported
    // for byref parameters for method calls that occur in lambdas that are converted
    // to expression trees.
    //
    return Argument->bilop == SX_ASG_RESADR ?
        Argument->AsExpressionWithChildren().Right :
        Argument;
}

// ----------------------------------------------------------------------------
// Convert a call tree. The new conversion goes through this code path as well.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCall
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_CALL );

    TREE_TYPE *Result = NULL;

    ILTree::SymbolReferenceExpression *Target = &Input->AsCallExpression().Left->AsSymbolReferenceExpression();
    ExpressionList* ArgumentExpressions = Input->AsCallExpression().Right;

    // First, re-map the call if necessary.

    Target->pnamed = m_CompilerHost->RemapRuntimeCall(
        ViewAsProcedure( Target->pnamed ),
        m_CompilationCaches
        );

    Procedure* TargetMethod = ViewAsProcedure( Target->pnamed );
    Parameter* CurrentParameter = TargetMethod->GetFirstParam();

    // Check to see if this is a user-defined operator.

    if( HasFlag32( Input, SXF_CALL_WAS_OPERATOR ) )
    {
        switch( Input->AsCallExpression().OperatorOpcode )
        {
        case SX_ADD:
        case SX_SUB:
        case SX_MUL:
        case SX_DIV:
        case SX_IDIV:
        case SX_MOD:
        case SX_POW:
        case SX_LE:
        case SX_LT:
        case SX_GT:
        case SX_GE:
        case SX_EQ:
        case SX_NE:
        case SX_AND:
        case SX_OR:
        case SX_XOR:
        case SX_SHIFT_LEFT:
        case SX_SHIFT_RIGHT:
            Result = ConvertUserDefinedOperatorToExpressionTree(
                Input,
                Target,
                Input->Loc,
                ArgumentExpressions->AsExpressionWithChildren().Left,
                ArgumentExpressions->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                );
            break;

        case SX_PLUS:
        case SX_NEG:
        case SX_NOT:
            Result = ConvertUserDefinedOperatorToExpressionTree(
                Input,
                Target,
                Input->Loc,
                ArgumentExpressions->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                );
            break;

        case SX_CTYPE:
        case SX_DIRECTCAST:
        case SX_TRYCAST:
            Result = ConvertUserDefinedCastOperatorToExpressionTree(
                Input,
                Target,
                Input->Loc,
                ArgumentExpressions->AsExpressionWithChildren().Left,
                Input->ResultType,
                Flags,
                TargetType
                );
            break;

        case SX_ANDALSO:
        case SX_ORELSE:
            VSASSERT( false, "How did this happen? AndAlso/OrElse should have been lowered later." );
            break;
        }

        if( Result != NULL )
        {
            return( Result );
        }
    }

    // Generate a standard call.

    // We need to encode the arguments.

    NorlsAllocator Scratch(NORLSLOC);

    IReadOnlyList<TREE_TYPE *> *ArgumentsList = CreateArgumentListForExpressionTree(
        ArgumentExpressions,
        Flags,
        TargetType,
        TargetMethod,    
        Input->Loc,
        Scratch
        );
    
    // Constructor calls.

    if( TargetMethod->IsInstanceConstructor() )
    {
        Result = m_ExprTreeGenerator->CreateNewExpr(
            TargetMethod,
            Target->GenericBindingContext->PGenericTypeBinding(),
            ArgumentsList,
            TargetType,
            Input->Loc
            );
    }
    else
    {
        TREE_TYPE *ThisExpression = Input->AsCallExpression().ptreeThis ?
            ConvertInternalToExpressionTree(
                ( ILTree::Expression* )Input->AsCallExpression().ptreeThis,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                ) :
            NULL;

        // Bug 491598
        // If ptreeThis is not null, and ThisExpression is NULL, it may be that the target
        // was a multiline lambda.

        if( Input->AsCallExpression().ptreeThis != NULL &&
            ThisExpression == NULL )
        {
            VSASSERT( m_LastExprTreeErrID != 0, "Error but no error set?" );
            return NULL;
        }

        // There are a few cases where we have to emit converts on the "this" constant.

        if( Input->AsCallExpression().ptreeThis != NULL )
        {
            ILTree::Expression* SymbolForThis = ( ILTree::Expression* )Input->AsCallExpression().ptreeThis;

            if( Input->AsCallExpression().ptreeThis->bilop == SX_ADR ||
                Input->AsCallExpression().ptreeThis->bilop == SX_ASG ||
                Input->AsCallExpression().ptreeThis->bilop == SX_ASG_RESADR )
            {
                SymbolForThis = Input->AsCallExpression().ptreeThis->AsExpressionWithChildren().Left;
            }

            // In the case of a method called on generic type parameter,
            // generate a convert to the parent of the method, which has already been analyzed
            // correctly.

            Type* targetType = NULL;

////////////////////////////////////////////////////
//
// The following lines have been commented out as a fix for TFS bug 367903 (PS #: 171027).
// The commented lines will be removed entirely once the C# team has made a fix in System.Core.dll so that
// we do not throw a runtime exception when we try to compile an Expression.Property().
//
// The commented code below causes to call conversion method to ensure that the method type
// and the argument types match. This is bad because it causes boxing when we do not want boxing 
// to occur. NOT performing this conversion is ok with Expression.Call, but not performing the conversion 
// with an Expression.Property causes an exception to be thrown by System.Core.dll.
/////////////////////////////////////////////////////
//            if( SymbolForThis->vtype == t_generic )
//            {
//                targetType = TargetMethod->PProc()->GetContainingClassOrInterface();
//                VSASSERT( targetType != NULL, "How can the containing class be NULL?" );
//            }
// NOTE: The If statement below (if (TypeHelpers...) should be an else if, if we decide to reintroduce
//           the above code.
//////////////////////////////////////////////////////

            // In the case where the method is one of the ones defined on Object or ValueType,
            // emit a cast to that symbol.

            if( TypeHelpers::IsValueType( SymbolForThis->ResultType ) &&
                TargetMethod->GetParent()->IsClass() &&
                ( TargetMethod->GetParent()->PClass()->IsCOMValueType() ||
                  BCSYM::AreTypesEqual(
                      TargetMethod->GetParent()->PClass(),
                        GetFXSymbolProvider()->GetType(FX::ObjectType) ) ) )
            {
                targetType = TargetMethod->GetParent()->PClass();
                VSASSERT( targetType != NULL, "How can the containing class be NULL?" );
            }

            if( targetType != NULL )
            {
                // Emit a cast.

                ThisExpression = m_ExprTreeGenerator->CreateCastExpr(
                    ThisExpression,
                    targetType,
                    TargetType,
                    Input->Loc
                    );
            }
        }

        if( TargetMethod->IsPropertyGet() || TargetMethod->IsPropertySet() )
        {
            Procedure* Proc = ViewAsProcedure( Target->pnamed );

#if HOSTED
            //It is valid for copy backs from by-ref or late binding, and member init expressions
#else
            VSASSERT( !TargetMethod->IsPropertySet(), "How can we have property setters in expressions?" );
#endif

            // If we have a non-virtual, no parameter property set, generate a special
            // expression tree. Otherwise, generate the call expression tree.

            if( Proc->IsPropertyGet() && !Input->AsCallExpression().Right )
            {
                Result = m_ExprTreeGenerator->CreatePropertyAccessExpr(
                    TargetMethod,
                    Target->GenericBindingContext,
                    ThisExpression,
                    TargetType,
                    Input->Loc
                    );
            }
        }

        // Check if this call was the result of resolving a implicit delegate
        // invoke into the Invoke method on System.Delegate. If it was, we
        // should encode a Invoke expression rather than a method call.

        if( Result == NULL && HasFlag32( Input, SXF_CALL_WAS_IMPLICIT_INVOKE ) )
        {
            Result = m_ExprTreeGenerator->CreateInvokeExpr(
                ThisExpression,
                ArgumentsList,
                TargetType,
                Input->Loc
                );
        }

        // If we haven't handled any of the special cases above, we should just
        // generate a method call expression.

        if( Result == NULL )
        {
            if( Target->uFlags & SXF_SYM_NONVIRT )
            {
                Result = m_ExprTreeGenerator->CreateCallExpr(
                    TargetMethod,
                    Target->GenericBindingContext,
                    ThisExpression,
                    ArgumentsList,
                    TargetType,
                    Input->Loc
                    );
            }
            else
            {
                // No separate way to emit virtual calls in ET ASTs. Call is sufficient
                // for virtual calls.

                Result = m_ExprTreeGenerator->CreateCallExpr(
                    TargetMethod,
                    Target->GenericBindingContext,
                    ThisExpression,
                    ArgumentsList,
                    TargetType,
                    Input->Loc
                    );
            }
        }
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert the cast operators.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCast(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_CTYPE ||
                  Input->bilop == SX_TRYCAST ||
                  Input->bilop == SX_DIRECTCAST
                  );

    RuntimeMembers Helper = UndefinedRuntimeMember;

    // We must build the lowered expression tree. But only do this if one of the source
    // is nullable and the target non-nullable, or vice versa.

    if( Input->bilop == SX_CTYPE &&
        (
         (
            !TypeHelpers::IsNullableType( Input->AsExpressionWithChildren().Left->ResultType, m_CompilerHost ) &&
            TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost )
         ) ||
         (
            TypeHelpers::IsNullableType( Input->AsExpressionWithChildren().Left->ResultType, m_CompilerHost ) &&
            !TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost )
         )
        ) &&
        !BCSYM::AreTypesEqual(
            TypeHelpers::GetElementTypeOfNullable( Input->AsExpressionWithChildren().Left->ResultType, m_CompilerHost ),
            TypeHelpers::GetElementTypeOfNullable( Input->ResultType, m_CompilerHost ) )
      )
    {
        ILTree::Expression* LoweredCType = m_Semantics->LowerCType( Input );

        // If we lower the ctype, we have to re-walk the tree, because this may produce call
        // expressions, and we need to run through that code.

        if( LoweredCType != Input )
        {
            TREE_TYPE *Result = ConvertInternalToExpressionTree(
                LoweredCType,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                );

            // Cannot re-enable this assert in a simple way since Result is now the
            // a template parameter type and not ILTree::Expression any more
            // VSASSERT( Result->bilop != SX_IIF, "How come we have IIF?" );

            return( Result );
        }
    }

    // 




#if 0

    if( vtypeTo == t_bool && vtypeFrom == t_ref )
    {
        switch( Input->AsExpressionWithChildren().Left->bilop )
        {
        case SX_EQ:
        case SX_NE:
        case SX_LE:
        case SX_LT:
        case SX_GE:
        case SX_GT:
            Input->AsExpressionWithChildren().Left->AsExpression().vtype = t_bool;
            return( ConvertBooleanOperator( Input->AsExpressionWithChildren().Left, Flags, TargetType ) );
            break;
        }
    }

#endif

    TREE_TYPE *ExpressionTree = NULL;

    switch( Input->bilop )
    {
    case SX_CTYPE:
    {
        // This will help us check if this cast expression should be represented
        // by a runtime method. But, don't do this, and fall through to a normal cast,
        // if we have a nothing casted to a nullable type.

        if( !( TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost ) &&
            Input->AsExpressionWithChildren().Left->bilop == SX_NOTHING ) )
        {
            Vtypes vtypeTo = GetVtypeOfTree( Input );
            Vtypes vtypeFrom = GetVtypeOfTree( Input->AsExpressionWithChildren().Left );

            // At this point, we could have a T? -> T conversion. So if the intrinsic types are the
            // same, don't find the runtime helper.

            if( vtypeTo != vtypeFrom )
            {
                Helper = DetermineRuntimeMemberForCType(
                    vtypeTo,
                    vtypeFrom,
                    Input,
                    GetFXSymbolProvider()->GetType(FX::ObjectType)
                    );
            }
        }

        // If it's not a runtime member, we fall through to a cast.

        if( Helper != UndefinedRuntimeMember )
        {
            break;
        }
    }

    __fallthrough;

    case SX_TRYCAST:
    case SX_DIRECTCAST:
    {
        Vtypes vtypeTo = GetVtypeOfTree( Input );
        Vtypes vtypeFrom = GetVtypeOfTree( Input->AsExpressionWithChildren().Left );
        
        // Dev10#731130: If we see the Nothing literal converted to a value
        // type, emit the default value. Otherwise, compiled LambdaExpression
        // will throw NullReference because when the Expression Tree is
        // compiled and run.
        //
        // If instead of the Nothing literal, we get a null value at runtime
        // it will result in a NullReference. We have to leave it that way,
        // because if we generate something other than a simple Convert node,
        // LINQ providers might not understand the resulting tree.
        //
        if (Input->AsExpressionWithChildren().Left->bilop == SX_NOTHING &&
            TypeHelpers::IsValueType(Input->ResultType) &&
            !TypeHelpers::IsNullableType(Input->ResultType, m_CompilerHost))
        {
            ExpressionTree = CreateNewStructureType(Input->ResultType, Input->Loc, TargetType);

            // we're done
            break;
        }

        // First, let's convert the argument to an expression tree.

        ExpressionTree = ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left,
            Flags,
            NULL
            );

        // Check codegen\expressions.cpp. In this case, we emit a box instruction.
        // For expression trees, just don't emit a cast instruction.

        if( m_ConvertingQualifiedExpression && vtypeFrom == t_generic && vtypeTo == t_ref )
        {
            // skip cast because of boxed generic
        }

        // If we are casting from boolean to an unsigned integer type, we have to dance a little
        // because we first need to cast it to the signed version of the type, negate it, then
        // convert back to the unsigned type. This is because in VB, bool is -1, and the expression
        // tree API does not like doing negations on unsigned types.
        // We also have to do this for SByte.

        else if( vtypeFrom == t_bool && ( IsUnsignedType( vtypeTo ) || vtypeTo == t_i1 ) )
        {
            Type* associatedType = GetTypeForBooleanConversion( vtypeTo );

            // Must use unsafe casts, because we know we will overflow.

            ExpressionTree = m_ExprTreeGenerator->CreateCastExpr(
                ExpressionTree,
                associatedType,
                TargetType,
                Input->Loc
                );

            // Now we need to emit the negate here, while we have the
            // signed type on the execution stack.

            ExpressionTree = m_ExprTreeGenerator->CreateNegateExpr(
                ExpressionTree,
                TargetType,
                Input->Loc
                );

            // For the final cast, we always want an unsafe cast here.

            ExpressionTree = m_ExprTreeGenerator->CreateCastExpr(
                ExpressionTree,
                Input->ResultType,
                TargetType,
                Input->Loc
                );
        }
        else
        {
            // Find the cast method, and use the checked cast if needed.

            if( Input->bilop == SX_TRYCAST )
            {
                ExpressionTree = m_ExprTreeGenerator->CreateTypeAsExpr(
                    ExpressionTree,
                    Input->ResultType,
                    TargetType,
                    Input->Loc
                    );
            }
            else
            {
                ExpressionTree = m_ExprTreeGenerator->CreateConvertExpr(
                    ExpressionTree,
                    Input->ResultType,
                    UseCheckedSemantics(Input),
                    TargetType,
                    Input->Loc
                    );
            }

            // Now, if the original cast was to a signed type, we have to emit
            // the negate here for conversions of boolean.

            if( vtypeFrom == t_bool && IsNumericType( vtypeTo ) )
            {
                // We have to emit a neg tree to make sure that we get
                // the right semantics for Boolean conversions
                // Note that we've already taken care of decimal

                ExpressionTree = m_ExprTreeGenerator->CreateNegateExpr(
                    ExpressionTree,
                    TargetType,
                    Input->Loc
                    );
            }
        }
        break;
    }
    }

    if( Helper != UndefinedRuntimeMember )
    {
        return ConvertCastRuntimeHelperToExpressionTree(
            Input,
            Helper,
            Input->Loc,
            Flags,
            TargetType
            );
    }
    else
    {
        ThrowIfNull( ExpressionTree );
        return ExpressionTree;
    }
}

// ----------------------------------------------------------------------------
// Convert the creation of constant values.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertConstantValue(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    // -----------------------------------Contract start------------------------------------
    // This contract check is very important/required since this method ConvertConstantValue
    // is overridden in derived classes of ExpressionTreeSemantics.h and and the derived
    // implementations only handle these node kinds.

    ThrowIfFalse(Input->bilop == SX_NOTHING ||
                 Input->bilop == SX_CNS_INT ||
                 Input->bilop == SX_CNS_FLT ||
                 Input->bilop == SX_CNS_DEC ||
                 Input->bilop == SX_CNS_STR ||
                 Input->bilop == SX_METATYPE ||
                 Input->bilop == SX_SYM);

    if (Input->bilop == SX_SYM)
    {
        ThrowIfFalse(Input->AsSymbolReferenceExpression().Symbol->IsVariable());
        
        VARIABLEKIND VarKind =
            Input->AsSymbolReferenceExpression().Symbol->PVariable()->GetVarkind();

        ThrowIfFalse(VarKind == VAR_Param ||
                     VarKind == VAR_Local ||
                     VarKind == VAR_Const);
    }

    // -----------------------------------Contract end------------------------------------

    // This method will/should be overridden for the specific hosted compiler scenarios
    // where m_ExprTreeGenRuntimeValues will be NULL.
    //
    ThrowIfNull(m_ExprTreeGenRuntimeValues);

    return m_ExprTreeGenRuntimeValues->CaptureRuntimeValueAsConstant(
        Input,
        TargetType
        );
}




// ----------------------------------------------------------------------------
// Convert the constructor call of a delegate.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertDelegateConstructor(
    ILTree::Expression* Input,
    Type* DelegateType,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfNull( DelegateType );
    ThrowIfFalse( Input->bilop == SX_DELEGATE_CTOR_CALL );

    // We will generate a call to Delegate.CreateDelegate, using
    // the (Type, Object, MethodInfo, Bool) overload.

    ILTree::Expression* TypeExpression = m_Semantics->AllocateExpression(
        SX_METATYPE,
        GetFXSymbolProvider()->GetTypeType(),
        m_Semantics->AllocateExpression(
            SX_NOTHING,
            DelegateType,
            Input->Loc
            ),
        Input->Loc
        );

    ILTree::Expression* ObjectExpression = m_Semantics->ConvertWithErrorChecking(
        ( ILTree::Expression* )Input->AsDelegateConstructorCallExpression().ObjectArgument,
        GetFXSymbolProvider()->GetObjectType(),
        ExprHasExplicitCastSemantics
        );

    ILTree::Expression* MethodInfoExpression = m_Semantics->AllocateExpression(
        SX_METATYPE,
        GetFXSymbolProvider()->GetMethodInfoType(),
        ( ILTree::Expression* )Input->AsDelegateConstructorCallExpression().Method,
        Input->Loc
        );

    ILTree::Expression* BooleanExpression = m_Semantics->ProduceConstantExpression(
        0,
        Input->Loc,
        GetFXSymbolProvider()->GetBooleanType()
        IDE_ARG(0)
        );

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<TREE_TYPE *> Arguments(&Scratch);
    TREE_TYPE* Instance = NULL;

    // Get the symbol for System.Reflection.MethodInfo.CreateDelegate if available. This will be present in .NET Core.
    Procedure *CreateDelegateProcedure = GetRuntimeMemberProcedure( CreateDelegateMemberFromMethodInfo, Input->Loc, false );
    if ( CreateDelegateProcedure )
    {

        Arguments.Add(
            ConvertInternalToExpressionTree(
                TypeExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );

        Arguments.Add(
            ConvertInternalToExpressionTree(
                ObjectExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );

        Instance = 
            ConvertInternalToExpressionTree(
                MethodInfoExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
            );
    }
    // else fallback to System.Delegate.CreateDelegate.
    else if (CreateDelegateProcedure = GetRuntimeMemberProcedure( CreateDelegateMember, Input->Loc, false ))
    {
        Arguments.Add(
            ConvertInternalToExpressionTree(
                TypeExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );

        Arguments.Add(
            ConvertInternalToExpressionTree(
                ObjectExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );

        Arguments.Add(
            ConvertInternalToExpressionTree(
                MethodInfoExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );

        Arguments.Add(
            ConvertInternalToExpressionTree(
                BooleanExpression,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                )
            );
    }
    // No member found.
    else
    {
		ReportErrorForMissingHelper( Input->Loc, CreateDelegateMemberFromMethodInfo );
        return NULL;
    }

    TREE_TYPE *Result = m_ExprTreeGenerator->CreateCallExpr(
        CreateDelegateProcedure,
        NULL,   // no generic binding context
        Instance,
        &Arguments,
        TargetType,
        Input->Loc
        );

    // Now, we need to generate a cast node to the delegate type, because
    // Delegate.CreateDelegate returns just a System.Delegate.

    Result = m_ExprTreeGenerator->CreateCastExpr(
        Result,
        DelegateType,
        TargetType,
        Input->Loc
        );

    return Result;
}

// ----------------------------------------------------------------------------
// Convert structure initialization.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertInitStructure(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_INIT_STRUCTURE );

    return CreateNewStructureType(
        Input->AsInitStructureExpression().StructureType,
        Input->Loc,
        TargetType
        );
}

// ----------------------------------------------------------------------------
// Convert the IsType opcode.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertIsType(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ISTYPE );

    TREE_TYPE *ExprOperand = ConvertInternalToExpressionTree(
        (ILTree::Expression* )Input->AsExpressionWithChildren().Left,
        Flags,
        NULL
        );

    Type *TypeOperand = Input->AsExpressionWithChildren().Right->ResultType;

    return m_ExprTreeGenerator->CreateTypeIsExpr(
        ExprOperand,
        TypeOperand,
        TargetType,
        Input->Loc
        );
}

// ----------------------------------------------------------------------------
// Convert lambda expressions.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertLambda
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_LAMBDA );

    ILTree::LambdaExpression* Lambda = &Input->AsLambdaExpression();


    // Don't allow statement lambdas to be converted to expression trees.
    if (Lambda->IsStatementLambda)
    {
        // Note that "Async Function() 1" is represented internally as a statement lambda "Async Function() : Return 1 : End Function"
        // We pick out the resumable cases here so as to give better error messages "can't convert async/iterator to expression tree"
        bool isResumable = Lambda->IsAsyncKeywordUsed || Lambda->IsIteratorKeywordUsed;
        // but otherwise just fall back to "can't convert statement lambda to expression tree"

        ReportErrorForExpressionTree( isResumable ? ERRID_ResumableLambdaInExpressionTree : ERRID_StatementLambdaInExpressionTree, Input->Loc );
        return NULL;
    }

    ILTree::Expression* pLambdaExpression = Lambda->GetExpressionLambdaBody();

    Parameter* CurrentParameter = Lambda->FirstParameter;

#if 0

    // We need to see whether the lambda expression is a sub or a func.
    // Turns out that C# will support Sub types and throw away the return.
    // Linked bug over to C#.

    Declaration* InvokeMethod = NULL;
    InvokeMethod = GetInvokeFromDelegate(Lambda->ResultType, m_Compiler);

    if( InvokeMethod != NULL )
    {
        if( GetReturnType( InvokeMethod->PProc() ) == NULL )
        {
            SetErrorForExpressionTree( ERRID_SubIllegalInExpressionTree );
            return( NULL );
        }
    }
#endif

    NorlsAllocator Scratch(NORLSLOC);
    LambdaParamsMap TempLambdaParamsMap(&Scratch);

    // Backup the current lambda params map so that it is restored before
    // exiting this function.
    BackupValue<LambdaParamsMap *> BackupLambdaParamsMap( &m_CurrentLambdaParamsMap );

    // Set the m_CurrentLambdaParamsMap only if it NULL. It should not be set when
    // it is non-NULL in order to ensure that the mapped parent lambda params are
    // available when converting nested lambdas.
    //
    if (m_CurrentLambdaParamsMap == NULL)
    {
        m_CurrentLambdaParamsMap = &TempLambdaParamsMap;
    }

    ArrayList_NorlsAllocBased<LambdaParamHandle *> LambdaParameters(&Scratch);

    while( CurrentParameter != NULL )
    {
        Variable* ParameterLocal = Lambda->GetLocalsHash()->SimpleBind(
            CurrentParameter->GetName(), SIMPLEBIND_IgnoreOtherPartialTypes
            )->PVariable();

        // Port SP1 CL 2935897 to VS 10
        // Port SP1 CL 2929997 to VS 10

        VSASSERT( ParameterLocal != NULL, "How can we not find the local for the lambda?" );

        if( (ParameterLocal != NULL) && (ParameterLocal->GetCompilerType()->IsPointerType()) )
        {
           ReportErrorForExpressionTree( ERRID_ByRefParamInExpressionTree, Input->Loc );
           return( NULL );
        }

        // We need to create the Parameter expression and store it in a local
        // and associate it with the parameter.

        LambdaParamHandle *LambdaParameter = m_ExprTreeGenerator->CreateLambdaParameter(
            CurrentParameter->GetName(),
            CurrentParameter->GetType(),
            Input->Loc
            );

        LambdaParameters.Add(LambdaParameter);

        if (ParameterLocal != NULL)
        {
            m_CurrentLambdaParamsMap->HashAdd(ParameterLocal, LambdaParameter);
        }

        CurrentParameter = CurrentParameter->GetNext();
    }

    TREE_TYPE *LambdaBody = ConvertInternalToExpressionTree(
        pLambdaExpression,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    TREE_TYPE *Result = m_ExprTreeGenerator->CreateLambdaExpr(
        &LambdaParameters,
        LambdaBody,
        Lambda->ResultType,
        TargetType,
        Input->Loc
        );

    return Result;
}

// ----------------------------------------------------------------------------
// Convert the "like" keyword.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertLike(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_LIKE );

    RuntimeMembers Helper = UndefinedRuntimeMember;
    ILTree::Expression* LikeType = m_Semantics->ProduceConstantExpression(
        ( Input->uFlags & SXF_RELOP_TEXT ) ? 1 : 0,
        Input->Loc,
        GetFXSymbolProvider()->GetIntegerType()
        IDE_ARG(0)
        );

    Vtypes vtypeLeft = GetVtypeOfTree( Input->AsExpressionWithChildren().Left );

    if( vtypeLeft == t_ref )
    {
        Helper = LikeObjectMember;
    }
    else if( vtypeLeft == t_string )
    {
        Helper = LikeStringMember;
    }
    else
    {
        VSASSERT( false, "How did we get a non-object, non-string LHS in 'like'?" );
    }

    return( ConvertRuntimeHelperToExpressionTree(
            Helper,
            Input->Loc,
            Input->ResultType,
            ( ILTree::Expression* )Input->AsExpressionWithChildren().Left,
            ( ILTree::Expression* )Input->AsExpressionWithChildren().Right,
            LikeType,
            Flags,
            TargetType
            )
        );
}

// ----------------------------------------------------------------------------
// Convert negation and not operators.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertNegateNot(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_NEG || Input->bilop == SX_NOT );

    RuntimeMembers Helper = UndefinedRuntimeMember;
    Vtypes vtypeOp = GetVtypeOfTree( Input );

    // If we have an object or a decimal, generate the helper for the runtime method.

    if( vtypeOp == t_ref )
    {
        Helper = CodeGenerator::GetHelperForObjBinOp( Input->bilop );
    }
    else if( vtypeOp == t_decimal )
    {
        Helper = CodeGenerator::GetHelperForDecBinOp( Input->bilop );
    }

    // We are either interpreting a runtime member call, or an expression tree call.

    if( Helper != UndefinedRuntimeMember )
    {
        return ConvertRuntimeHelperOperatorToExpressionTree(
            Input,
            Helper,
            Input->Loc,
            Input->AsExpressionWithChildren().Left,
            Flags,
            TargetType
            );
    }
    else
    {
        Vtypes vtypeLeft = GetVtypeOfTree( Input->AsExpressionWithChildren().Left );

        // For these operators, we must cast i1, i2, ui1, ui2 to i4.

        TREE_TYPE *CastExpr = GenerateCastToInteger(
            vtypeLeft,
            TypeHelpers::IsNullableType( Input->AsExpressionWithChildren().Left->ResultType, m_CompilerHost ),
            ConvertInternalToExpressionTree(
                Input->AsExpressionWithChildren().Left,
                Flags,
                NULL
                ),
            Input->Loc,
            TargetType
            );

        TREE_TYPE *Result = m_ExprTreeGenerator->CreateUnaryOperatorExpr(
            Input->bilop,
            CastExpr,
            UseCheckedSemantics(Input),
            TargetType,
            Input->Loc
            );

        // If we casted to i4, we must cast back to the original type.

        Result = GenerateCastBackFromInteger(
            Input,
            Result,
            TargetType
            );

        return Result;
    }
}

// ----------------------------------------------------------------------------
// Convert New, which calls the constructor.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertNew(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_NEW );

    TREE_TYPE *Result = NULL;

    // Just fix up the input and convert the call to the constructor.

    if( Input->AsNewExpression().ConstructorCall != NULL )
    {
        ILTree::Expression* ConstructorCall = Input->AsNewExpression().ConstructorCall;

        switch( Input->AsNewExpression().ConstructorCall->bilop )
        {
        case SX_CALL:
            Result = ConvertCall( ConstructorCall, Flags, TargetType );
            break;

        case SX_DELEGATE_CTOR_CALL:
            Result = ConvertDelegateConstructor( ConstructorCall, Input->ResultType, Flags, TargetType );
            break;

        default:
            VSASSERT( false, "What type of constructor is this?" );
            break;
        }
    }
    else if( Input->AsNewExpression().Class != NULL )
    {
        // This is the case of doing a new on a generic parameter.
        
        Result = m_ExprTreeGenerator->CreateNewExpr(
            Input->AsNewExpression().Class,
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCollectionInitializer
(
    ILTree::Expression *Input,
    ExpressionFlags flags, 
    Type * pTargetType
)
{
    ThrowIfNull(Input);
    IfFalseThrow(Input->bilop == SX_COLINIT);

    ILTree::ColInitExpression * pExpr = &Input->AsColInitExpression();

    //We want to bind to 
    //Expressions.
    TREE_TYPE *pConvertedNewExpr = ConvertInternalToExpressionTree(
        pExpr->NewExpression,
        flags,
        GetFXSymbolProvider()->GetNewExpressionType()
        );

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<ListElementInitInfo<TREE_TYPE>> Initializers(&Scratch);

    ILTree::ExpressionWithChildren * pElementList = pExpr->Elements;

    while (pElementList)
    {
        
        ThrowIfFalse(pElementList->Left && pElementList->Left->bilop == SX_COLINITELEMENT);
        ILTree::ColInitElementExpression * pElement = &pElementList->Left->AsColInitElementExpression();

        ThrowIfFalse(pElement->CallExpression && pElement->CallExpression->bilop == SX_CALL);
        ILTree::CallExpression * pCallExpression = & pElement->CallExpression->AsCallExpression();

        ThrowIfFalse(pCallExpression->Left && pCallExpression->Left->bilop == SX_SYM);
        
        // Fill the Init info structure
        ListElementInitInfo<TREE_TYPE> InitInfo;

        InitInfo.AddMethod =
            ViewAsProcedure(pCallExpression->Left->AsSymbolReferenceExpression().Symbol);

        InitInfo.Arguments = CreateArgumentListForExpressionTree(
            pCallExpression->Right, 
            flags, 
            GetFXSymbolProvider()->GetExpressionType(), 
            InitInfo.AddMethod,
            pElement->Loc,
            Scratch
            );

        InitInfo.BindingContext =
            pCallExpression->Left->AsSymbolReferenceExpression().GenericBindingContext;

        // Add the init info structure to the list of initializers
        Initializers.Add(InitInfo);

        pElementList = pElementList->Right ? &pElementList->Right->AsExpressionWithChildren() : NULL;
    }

    return m_ExprTreeGenerator->CreateListInitExpr(
        pConvertedNewExpr,
        &Initializers,
        pTargetType,
        pExpr->Loc
        );
}

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertArrayLiteralToExpressionTree
(
    ILTree::Expression *Input,
    ExpressionFlags flags,
    Type * pTargetType
)
{
    ThrowIfNull(Input);
    IfFalseThrow(Input->bilop == SX_ARRAYLITERAL);

    ILTree::ArrayLiteralExpression * pLiteral = &Input->AsArrayLiteralExpression();

    if (pLiteral->Rank > 1)
    {
        ReportErrorForExpressionTree( ERRID_ExprTreeNoMultiDimArrayCreation, Input->Loc );
        return NULL;
    }
    else
    {
        NorlsAllocator Scratch(NORLSLOC);
        ArrayList_NorlsAllocBased<TREE_TYPE *> ArrayElements(&Scratch);

        ILTree::ExpressionWithChildren * pElements = pLiteral->ElementList;

        while (pElements)
        {
            TREE_TYPE *ExprTreeElement = ConvertInternalToExpressionTree(
                pElements->AsExpressionWithChildren().Left,
                flags,
                GetFXSymbolProvider()->GetExpressionType()
                );

            ArrayElements.Add(ExprTreeElement);

            pElements = pElements->Right ? & pElements->Right->AsExpressionWithChildren() : NULL;
        }


        return m_ExprTreeGenerator->CreateNewArrayInitExpr(
            pLiteral->ResultType->PArrayType()->GetRoot(),
            &ArrayElements,
            pTargetType,
            pLiteral->Loc
            );
    }
}

// ----------------------------------------------------------------------------
// Convert the creation of arrays.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertNewArray(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_NEW_ARRAY );

    ExpressionList* IndexExpressions = ( ExpressionList* )Input->AsExpressionWithChildren().Left;

    // Convert the array initializers to expressions, and add them to the parse tree
    // that represents the expression construction call.

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<TREE_TYPE *> Bounds(&Scratch);

    while( IndexExpressions != NULL )
    {
        if( IndexExpressions->bilop != SX_LIST )
        {
            VSASSERT( false, "why do we not have a SX_LIST in the array?" );
            return( NULL );
        }

        TREE_TYPE *Bound = ConvertInternalToExpressionTree(
            IndexExpressions->AsExpressionWithChildren().Left,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        Bounds.Add(Bound);

        IndexExpressions = IndexExpressions->AsExpressionWithChildren().Right;
    }

    return m_ExprTreeGenerator->CreateNewArrayBoundsExpr(
        Input->ResultType->PArrayType()->GetRoot(),
        &Bounds,
        TargetType,
        Input->Loc
        );
}

// ----------------------------------------------------------------------------
// Convert an array initializer.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertNewArrayInitializer(
    ILTree::Expression* Input,
    ILTree::Expression* Value,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfNull( Value );
    ThrowIfFalse( Value->bilop == SX_NEW_ARRAY );

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<TREE_TYPE *> ArrayElements(&Scratch);

    ILTree::Expression* Initializers = ( ILTree::Expression* )Input->AsExpressionWithChildren().Right;

    while( Initializers != NULL )
    {
        ILTree::Expression* Initializer;

        if( Initializers->bilop == SX_SEQ )
        {
            Initializer = Initializers->AsExpressionWithChildren().Left;
            Initializers = Initializers->AsExpressionWithChildren().Right;
        }
        else
        {
            if( Initializers->bilop != SX_ASG )
            {
                return( NULL );
            }

            Initializer = Initializers;
            Initializers = NULL;
        }

        TREE_TYPE *Element = ConvertInternalToExpressionTree(
            Initializer->AsExpressionWithChildren().Right,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        ArrayElements.Add(Element);
    }

    return m_ExprTreeGenerator->CreateNewArrayInitExpr(
        Value->ResultType->PArrayType()->GetRoot(),
        &ArrayElements,
        TargetType,
        Input->Loc
        );
}

// ----------------------------------------------------------------------------
// Convert an object initializer.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertNewObjectInitializer(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( IsObjectInitializer( Input ) );

    // ATTENTION !!! If you change this code, make sure LinqOptimization is changed accordingly

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<MemberAssignmentInfo<TREE_TYPE>> Initializations(&Scratch);

    ExpressionList* Initializers = ( ExpressionList* )Input->AsExpressionWithChildren().Right;

    // The strategy is that for each initializer, we will create a binding expression. We
    // save these binding expressions in an array initializer itself. We then interpret
    // this array initializer to get the final expression for this object initializer.

    while( Initializers != NULL )
    {
        if( Initializers->bilop != SX_LIST )
        {
            return( NULL );
        }

        ILTree::Expression* Initializer = ( ILTree::Expression* )Initializers->AsExpressionWithChildren().Left;
        ILTree::Expression *InitValue = NULL;

        if( Initializer->bilop == SX_ASG )
        {
            InitValue = Initializer->AsExpressionWithChildren().Right;
        }
        else if( Initializer->bilop == SX_CALL )
        {
            Procedure *SetAccessor = ViewAsProcedure( Initializer->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().pnamed );

            // We used to need to GetAccessor.
            // Procedure *GetAccessor = SetAccessor->GetAssociatedPropertyDef()->GetProperty();

            Initializer->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().pnamed = SetAccessor;

            // MQ Bug 855193
            // New Outer { .Inner = New Inner(1,2)}, where Inner is a Structure with user defined constructor.
            // The initializer is different from Class, the InitInfo.Member should be MeArgument of Initializer, 
            // InitValue should be Initializer. 
            if (Initializer->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().pnamed->PProc()->IsAnyConstructor())
            {

                InitValue = Initializer;
            }
            else
            {
                InitValue = Initializer->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left;
            }
              
        }

        if( InitValue == NULL )
        {
            return( NULL );
        }

        // Add the members and the values to the lists

        ILTree::SymbolReferenceExpression *SymbolRef = NULL;

        // MQ Bug 855193
        if (Initializer->bilop == SX_CALL && 
            Initializer->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().pnamed->PProc()->IsAnyConstructor())
        {

            ILTree::Expression *pMeArgumentOfConstructor = Initializer->AsCallExpression().MeArgument;
            ILTree::Expression *pObjectAssignment = Input->AsExpressionWithChildren().Left;

            ThrowIfFalse( 
                pMeArgumentOfConstructor->bilop == SX_ADR &&
                pMeArgumentOfConstructor->AsExpressionWithChildren().Left != NULL &&
                pMeArgumentOfConstructor->AsExpressionWithChildren().Left->bilop == SX_SYM &&    
                pObjectAssignment != NULL &&
                pObjectAssignment->bilop == SX_ASG &&
                pObjectAssignment->AsExpressionWithChildren().Left != NULL &&
                pObjectAssignment->AsExpressionWithChildren().Left->bilop == SX_SYM);

            SymbolRef = &Initializer->AsCallExpression().MeArgument->AsExpressionWithChildren().Left->AsSymbolReferenceExpression();
        }
        else
        {
            SymbolRef = &Initializer->AsExpressionWithChildren().Left->AsSymbolReferenceExpression();
        }

        MemberAssignmentInfo<TREE_TYPE> InitInfo;

        InitInfo.Member = SymbolRef->pnamed->PMember();
        InitInfo.BindingContext = SymbolRef->GenericBindingContext;
        InitInfo.Value = ConvertInternalToExpressionTree(InitValue, Flags, NULL);
        
        Initializations.Add(InitInfo);

        Initializers = Initializers->AsExpressionWithChildren().Right;
    }

    // MemberInit expression requires a NewExpression, representing the constructor
    // call. It then takes an array of bindings.

    // A NewExpression requires a type or a constructor info, representing the
    // type to create. Find that now.

    TREE_TYPE *NewExpression = NULL;

    if( !IsInitStructure( Input ) )
    {
        ThrowIfFalse( Input->AsExpressionWithChildren().Left->bilop == SX_ASG &&
                      Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->bilop == SX_NEW );

        ILTree::Expression* Value = Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right;
        NewExpression = ConvertNew(Value, Flags, NULL);
    }
    else
    {
        NewExpression = ConvertInitStructure(
            Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left,
            Flags,
            NULL
            );
    }

    ThrowIfNull( NewExpression );

    return m_ExprTreeGenerator->CreateMemberInitExpr(
        NewExpression,
        &Initializations,
        TargetType,
        Input->Loc
        );
}

// ----------------------------------------------------------------------------
// Convert the SX_SEQ nodes. This will call other helpers to convert the
// specific scenarios.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertSEQ(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_SEQ ||
                  Input->bilop == SX_SEQ_OP1 ||
                  Input->bilop == SX_SEQ_OP2
                );

    if( Input->AsExpressionWithChildren().Right &&
        Input->AsExpressionWithChildren().Right->bilop == SX_SYM &&
        Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed &&
        IsVariable( Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed ) &&
        Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed->PVariable()->IsTemporary() )
    {
        // This should be the case where we're doing some kind of initialization to a
        // temporary and then returning the value itself. If so, ignore the return and
        // convert the initialization.

        // However, this could be the case of initializing multi-dimension arrays. We
        // must fail here.

        if( Input->ResultType->IsArrayType() &&
            Input->ResultType->PArrayType()->GetRank() > 1 )
        {
            ReportErrorForExpressionTree( ERRID_ExprTreeNoMultiDimArrayCreation, Input->Loc );
            return( NULL );
        }

        return( ConvertInternalToExpressionTree( Input->AsExpressionWithChildren().Left, Flags, TargetType ) );
    }
    else if( Input->AsExpressionWithChildren().Left->bilop == SX_CALL ||
             Input->AsExpressionWithChildren().Left->bilop == SX_NEW )
    {
        // This should be the case where copy-out assignments are appended on to a
        // call node (or a constructor). If so, ignore the copy-out and let the
        // expression tree consumer deal with it.

        return( ConvertInternalToExpressionTree( Input->AsExpressionWithChildren().Left, Flags, TargetType ) );
    }
    else if( Input->AsExpressionWithChildren().Left->bilop == SX_ASG &&
             Input->AsExpressionWithChildren().Right &&
             ( Input->AsExpressionWithChildren().Right->bilop == SX_SEQ || Input->AsExpressionWithChildren().Right->bilop == SX_ASG ) )
    {
        ILTree::Expression* Value = Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right;

        if( Value->bilop == SX_NEW_ARRAY )
        {
            return( ConvertNewArrayInitializer(
                    Input,
                    Value,
                    Flags,
                    TargetType
                    )
                );
        }
    }
    else if( IsObjectInitializer( Input ) )
    {
        return( ConvertNewObjectInitializer(
                Input,
                Flags,
                TargetType
                )
            );
    }
    else if (Input->bilop == SX_SEQ_OP2 &&
            Input->AsExpressionWithChildren().Left->bilop == SX_LAMBDA &&
            Input->AsExpressionWithChildren().Right == NULL)
    {
        // Lambda inside a lambda (needed to be wrapped in a dummy SEQ_OP)
        // See comment in assert in ClosureRoot::ConvertLambdaToProcedure for details.
        // Just recurse into the lambda
        return ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left,
            Flags,
            TargetType);
    }
    else if (Input->bilop == SX_SEQ_OP2 &&
             ( HasFlag32(Input, SXF_SEQ_OP2_RELAXED_LAMBDA_EXT) ||
               HasFlag32(Input, SXF_SEQ_OP2_RELAXED_LAMBDA_INST)))
    {
        VSASSERT(HasFlag32(Input, SXF_SEQ_OP2_RELAXED_LAMBDA_EXT) != HasFlag32(Input, SXF_SEQ_OP2_RELAXED_LAMBDA_INST),
            "Should never have two flags set at once");

        ILTree::LambdaExpression* FixedLambdaExpression = ConvertLambaExpressionFromSeqOp2(
            Input->AsExpressionWithChildren().Left,
            Input->AsExpressionWithChildren().Right,
            HasFlag32(Input, SXF_SEQ_OP2_RELAXED_LAMBDA_INST)); // true: as Instance, false: as extension method.

        if (FixedLambdaExpression == NULL)
        {
            VSASSERT(false, "Unexpected Sequence Expression the flag on seq_op should indicate this pattern....");

            ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
            return( NULL );
        }

        return ConvertLambda(
            FixedLambdaExpression,
            Flags,
            TargetType
            );
    }
    else if (Input->bilop == SX_SEQ_OP2 &&
             HasFlag32(Input->uFlags, SXF_SEQ_OP2_XML))
    {
        return ConvertXml(
            Input,
            Flags,
            TargetType
            );
    }
    else if( Input->AsExpressionWithChildren().Left->bilop == SX_LATE || (Input->AsExpressionWithChildren().Right && Input->AsExpressionWithChildren().Right->bilop == SX_LATE) )
    {
        ReportErrorForExpressionTree( ERRID_ExprTreeNoLateBind, Input->Loc );
        return( NULL );
    }

    ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
    return( NULL );
}


// This is not a general purpose visitor. If you like it, get your own, I am not sharing this one.
// This class has only one public method 'Visit' see its comment for details.
class ConvertLambaExpressionFromSeqOp2BoundTreeVisitor : private BoundTreeVisitor
{
private:    

    // Internal flag to indicate that the tree doesn't follow the pattern. 
    // Primary use within the class - cancel visiting operation as soon as we detected an error.   
    bool m_failed;  

    // Keeps pointer to the found call node
    // No guarantees about its value if m_failed == true
    ILTree::Expression * m_CallNode;

    // Keeps pointer to the found temp reference node
    // No guarantees about its value if m_failed == true
    ILTree::Expression * m_TempVarRefNode;

    // The symbol we are looking for
    Symbol* m_theTemp;

    ConvertLambaExpressionFromSeqOp2BoundTreeVisitor(Symbol* theTemp)
    {
        AssertIfFalse(theTemp);
        
        m_theTemp = theTemp;
        m_failed = (m_theTemp == NULL);
        m_CallNode = NULL;
        m_TempVarRefNode = NULL;
    }

public:    

    
    // Given an expression tree (pExpr) and a symbol for a temp var (theTemp), 
    // this method scans the tree and finds unique reference for the temp and a call node 
    // that uses the reference as the target instance (for an instance method) or as 
    // the first argument (for a shared method).
    // Returns true if nodes are found, false otherwise 
    static bool Visit(
        Symbol* theTemp, // the remp we are looking for 
        ILTree::Expression* pExpr, // the expression tree to look in
        ILTree::Expression * & pCallNode, // [Out] Found call node is saved here
        ILTree::Expression * & pTempRef // [Out] Found reference node is saved here
    )
    {
        AssertIfNull(pExpr);
        
        ConvertLambaExpressionFromSeqOp2BoundTreeVisitor visitor(theTemp);
        
        visitor.VisitExpression(&pExpr);

        if(!visitor.m_failed) 
        {
            AssertIfFalse(visitor.m_CallNode && visitor.m_TempVarRefNode);

            if(visitor.m_CallNode && visitor.m_TempVarRefNode)
            {
                pCallNode = visitor.m_CallNode;
                pTempRef = visitor.m_TempVarRefNode;
            }
            else
            {
                visitor.m_failed = true;
            }
        }

        if (visitor.m_failed)
        {
            pCallNode = NULL;
            pTempRef = NULL;
        }

        return !visitor.m_failed;
    }

    static bool IsSymbolHiddenBehindConversions(
        ILTree::Expression *pCallReceiver,
        Symbol *theTemp)
    {
        if (pCallReceiver && 
            (pCallReceiver->bilop == SX_WIDE_COERCE || 
             pCallReceiver->bilop == SX_CTYPE || 
             pCallReceiver->bilop == SX_TRYCAST || 
             pCallReceiver->bilop == SX_DIRECTCAST || 
             pCallReceiver->bilop == SX_ADR))
        {
            if(pCallReceiver->AsExpressionWithChildren().Left &&
               pCallReceiver->AsExpressionWithChildren().Left->bilop == SX_SYM &&
               pCallReceiver->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol == theTemp)
            {
                return true;
            }
        }
        return false;
    }
    
protected:

    // Record reference node for the variable we are looking for and the call node.
    // This method does all the work. As soon as it finds the call, it records both nodes. 
    // At the same time, it tracks references without a call and duplicate references. 
    // As soon as one is found, the failure is recorded and visitor is cancelled.
    virtual bool StartExpression(_Inout_ ILTree::Expression ** ppNode) 
    { 
        // continue visiting until we know that we failed.
        if (m_failed)
        {
            return false;
        }

        AssertIfNull(m_theTemp);
        
        if (BoundTreeVisitor::StartExpression(ppNode))
        {
            if (ppNode && (*ppNode))
            {
            
                ILTree::Expression * pNode = *ppNode;

                if (pNode->bilop == SX_SYM &&
                     pNode->AsSymbolReferenceExpression().Symbol == m_theTemp)
                {
                    // found a reference
                
                    if (m_CallNode == NULL)
                    {
                        m_failed = true; // found a reference, but haven't seen a call yet - an error
                        return false;
                    }

                    // we have a call node, must have a reference node too 
                    AssertIfFalse(m_TempVarRefNode != NULL);
                    
                    if (m_TempVarRefNode!=pNode)
                    {
                        m_failed = true; // do not expect more than one reference
                        return false;
                    }

                    AssertIfFalse(m_CallNode && m_TempVarRefNode && m_TempVarRefNode == pNode);
                }
                else if(m_CallNode == NULL && pNode->bilop == SX_CALL) 
                {
                    ILTree::CallExpression* pCall = &pNode->AsCallExpression();
                    ILTree::Expression *pCallReceiver = NULL;

                    if (pCall->MeArgument != NULL)
                    {
                        // instance call
                        pCallReceiver = pCall->MeArgument;                        
                    }
                    else
                    {
                        // shared call
                        AssertIfFalse(pCall->MeArgument == NULL);

                        if (pCall->AsExpressionWithChildren().Right != NULL &&
                            pCall->AsExpressionWithChildren().Right->bilop == SX_LIST)
                        {
                            ILTree::Expression* pArg = pCall->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left;
                            
                            pCallReceiver = pArg;
                        }
                    }

                    if (pCallReceiver)
                    {
                        if (pCallReceiver->bilop == SX_SYM &&
                            pCallReceiver->AsSymbolReferenceExpression().Symbol == m_theTemp)
                        {
                            m_CallNode = pCall;
                            m_TempVarRefNode = pCallReceiver;
                        }
                        else if (IsSymbolHiddenBehindConversions(pCallReceiver, m_theTemp))
                        {
                            m_CallNode = pCall;
                            m_TempVarRefNode = pCallReceiver->AsExpressionWithChildren().Left;
                        }
                    }

                }
            }
            
            return true; // continue visiting
        }

        return false;
    }

    // Cancel visiting operation as soon as we know that we failed.
    virtual bool StartBlock(ILTree::ExecutableBlock* pBlock) 
    { 
        return (!m_failed) && BoundTreeVisitor::StartBlock( pBlock); 
    }
    
    // Cancel visiting operation as soon as we know that we failed.
    virtual bool StartBlockStatements(ILTree::ExecutableBlock* pBlock) 
    { 
        return (!m_failed) && BoundTreeVisitor::StartBlockStatements(pBlock); 
    }
    
    // Cancel visiting operation as soon as we know that we failed.
    virtual bool StartStatement(ILTree::Statement* pStmt) 
    { 
        return (!m_failed) && BoundTreeVisitor::StartStatement(pStmt); 
    }
    
};

template<class TREE_TYPE> ILTree::LambdaExpression*
ExpressionTreeSemantics<TREE_TYPE>::ConvertLambaExpressionFromSeqOp2
(
    ILTree::Expression* assignment,
    ILTree::Expression* lambda,
    bool useInstance
)
{
    // The assignment and the lambda we are dealing with here are built by 
    // Semantics::CreateRelaxedDelegateLambda() function. 
    // 
    // When we are relaxing an AddressOf expression, something like:
    //
    //      Dim relaxed as Func(Of T) = AddressOf <expr>.SomeMethod
    //
    // Where SomeMethod returns value of type S.
    // 
    // The assignment looks like:
    //    
    //      temp = <expr>
    // 
    // For an instance method, the lambda looks like:
    //
    //      Function() CType(temp.SomeMethod(...), T)
    //
    // For an extension method, the lambda looks like:
    //
    //      Function() CType(SomeMethod(temp, ...), T)
    //
    // And when T and S happen to be different, but compatible delegate types (for example S is anonymous delegate type), 
    // the lambda will look like:
    // 
    //      Function() New T(temp.SomeMethod(...), S.Invoke)
    //
    // or
    //
    //      Function() New T(SomeMethod(temp, ...), S.Invoke)
    //
    // The main point I am trying to get across is that the lambda will have that call "buried"
    // inside its body, but not necessary as the top level expression.
    //
    // What this function is trying to do is to eliminate the need for the assignment because,
    // we are not able to represent assignments in expression trees yet. This may come at a price 
    // of broken semantics in terms of proper lifting and time when <expr> is evaluated because
    // we are patching the lambda body by replacing the temp reference with <expr>.

    if (assignment != NULL &&
        assignment->bilop == SX_ASG &&
        assignment->AsExpressionWithChildren().Left != NULL &&
        assignment->AsExpressionWithChildren().Left->bilop == SX_SYM &&
        assignment->AsExpressionWithChildren().Right != NULL)
    {
        Symbol* assignmentTarget = assignment->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol;
        if (assignmentTarget != NULL &&
            assignmentTarget->IsVariable() &&
            assignmentTarget->PVariable()->IsTemporary())
        {
            Variable* temporaryVariable = assignmentTarget->PVariable();
            ILTree::Expression* temporaryExpression = assignment->AsExpressionWithChildren().Right;

            if (lambda != NULL &&
                lambda->bilop == SX_LAMBDA &&
                lambda->AsLambdaExpression().GetExpressionLambdaBody() != NULL)
            {
                ILTree::Expression * body = lambda->AsLambdaExpression().GetExpressionLambdaBody();
                
                // Dev10 #496875 Look for our special nodes within the body.
                ILTree::Expression * pCall = NULL;
                ILTree::Expression * pReference = NULL;
                bool found = ConvertLambaExpressionFromSeqOp2BoundTreeVisitor::Visit(assignmentTarget, body, pCall, pReference);

                AssertIfFalse(found && pCall && pReference);

                if (found && pCall && pReference)
                {
                    AssertIfFalse(pCall->bilop == SX_CALL);
                    
                    if (pCall->bilop == SX_CALL)
                    {
                        ILTree::CallExpression* methodCall = &pCall->AsCallExpression();
                        ILTree::Expression *pCallReceiver = NULL;

                        if (useInstance)
                        {
                            // Instance call, fix up the this pointer.
                            VSASSERT(methodCall->MeArgument != NULL, "Extension method calls should be static");
                            pCallReceiver = methodCall->MeArgument;                            
                        }
                        else
                        {
                            // Extension Method Path.
                            VSASSERT(methodCall->MeArgument == NULL, "Extension method calls should be static");
                            if (methodCall->MeArgument == NULL &&
                                methodCall->AsExpressionWithChildren().Right != NULL &&
                                methodCall->AsExpressionWithChildren().Right->bilop == SX_LIST)
                            {
                                ILTree::ExpressionWithChildren* arguments = &methodCall->AsExpressionWithChildren().Right->AsExpressionWithChildren();
                                pCallReceiver = arguments->Left;
                            }
                        }

                        if (pCallReceiver)
                        {
                            if (pCallReceiver->bilop == SX_SYM &&
                                pCallReceiver->AsSymbolReferenceExpression().Symbol == assignmentTarget)
                            {
                                AssertIfFalse(pCallReceiver == pReference);
                                if (pCallReceiver == pReference)
                                {
                                    if (useInstance)
                                    {
                                        methodCall->MeArgument = temporaryExpression;
                                    }
                                    else
                                    {
                                        methodCall->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left = temporaryExpression;
                                    }
                                    return &lambda->AsLambdaExpression();
                                }
                            }
                            else if (ConvertLambaExpressionFromSeqOp2BoundTreeVisitor::IsSymbolHiddenBehindConversions(pCallReceiver, assignmentTarget))
                            {
                                AssertIfFalse(pCallReceiver->AsExpressionWithChildren().Left == pReference);
                                if (pCallReceiver->AsExpressionWithChildren().Left == pReference)
                                {
                                    pCallReceiver->AsExpressionWithChildren().Left = temporaryExpression;
                                    return &lambda->AsLambdaExpression();
                                }
                            }

                        }
                    }
                }
            }
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------------
// Convert Xml Expression
//    Xml code generation uses SX_SEQ_OP2.  Code looks like
//       SX_SEQ_OP2
//           SX_ASG
//              Local_Var = new XElement("x")
//           SX_SEQ_OP2
//              Local_Var.Add(Content1)
//              SX_SEQ_OP2
//                 ...
//                 SX_SEQ_OP2
//                   Local_Var.Add(ContentN)
//                   Local_Var
//
//    These SX_SEQ_OP2 aren't handled by query expressions. ConvertXml walks the SX_SEQ_OP2 and converts the procedural
//    code to functional code that looks like.
//
//        new XElement("x", Content1, ... ContentN)
//
//    Note, this funciton must be kept in sync with the xmlsemantics code generation.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertXml(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_SEQ_OP2 );
    ThrowIfFalse( HasFlag32(Input->uFlags, SXF_SEQ_OP2_XML) );

    // Get the constructor.  This will be either XElement or XDocument

    ILTree::Expression *Asg = Input->AsExpressionWithChildren().Left;
    ILTree::Expression *Constructor = Asg->AsExpressionWithChildren().Right;

    // Handle case of either constructor or function call.  If this
    // is a constructor then get the actual call.

    ThrowIfFalse( Constructor->bilop == SX_NEW );

    ILTree::Expression *Call = Constructor->AsNewExpression().ConstructorCall;

    // Clone the constructor's parameter list.

    ILTree::Expression *ParamList = Call->AsExpressionWithChildren().Right;
    ExpressionList *NewParamList = NULL;
    ExpressionList **Prev = &NewParamList;

    for (ExpressionList *Param = ParamList; Param; Param = Param->AsExpressionWithChildren().Right)
    {
        ExpressionList * Next = m_Semantics->AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
                m_Semantics->AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    Param->AsExpressionWithChildren().Left,
                    Param->Loc),
            NULL,
            Param->Loc);

        *Prev = Next;
        Prev = &Next->AsExpressionWithChildren().Right;
    }

    // Now add everything in the sequence operator as additional parameters to the
    // constructor. The contents of element is everything in the SX_SEQ_OP2 path to the right.

    ILTree::Expression *SequenceArgs = Input->AsExpressionWithChildren().Right;

    while (SequenceArgs)
    {
        if (SequenceArgs->bilop == SX_SEQ_OP2)
        {
            // The left is the call to Add
            ILTree::Expression *AddMethod = SequenceArgs->AsExpressionWithChildren().Left;

            // Get the parameter to Add.  AddMethod->AsExpressionWithChildren().Right is a list of one so Left is the actual parameter.

            ILTree::Expression *Param = AddMethod->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left;

            ExpressionList *Next = m_Semantics->AllocateExpression(
                SX_LIST,
                TypeHelpers::GetVoidType(),
                m_Semantics->AllocateExpression(
                    SX_ARG,
                    TypeHelpers::GetVoidType(),
                    Param,
                    Param->Loc),
                NULL,
                Param->Loc);

            *Prev = Next;
            Prev = &Next->AsExpressionWithChildren().Right;

            SequenceArgs = SequenceArgs->AsExpressionWithChildren().Right;
        }
        else
        {
            break;
        }
    }

    // Create a new constructor with the new parameter list.

    Constructor = m_Semantics->CreateConstructedInstance(
        Constructor->ResultType,
        Constructor->Loc,
        Constructor->Loc,
        NewParamList,
        NULL,
        Flags
        );

    // Now that the SX_SEQ_OP2 is gone.  Recursively call ConvertInternalToExpressionTree.

    return ConvertInternalToExpressionTree(
        Constructor,
        Flags,
        TargetType);
}

// ----------------------------------------------------------------------------
// Convert SX_SYM.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertSymbol(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_SYM );

    Declaration* Symbol = Input->AsSymbolReferenceExpression().pnamed;
    TREE_TYPE *Result = NULL;

    if( !IsVariable( Symbol ) )
    {
        return( NULL );
    }

    Variable* VariableSymbol = Symbol->PVariable();

    if( VariableSymbol->IsShared() )
    {
        Result = m_ExprTreeGenerator->CreateFieldExpr(
            VariableSymbol,
            Input->AsSymbolReferenceExpression().GenericBindingContext->PGenericTypeBinding(),
            NULL, // shared member
            TargetType,
            Input->Loc
            );
    }
    else if( Input->AsSymbolReferenceExpression().ptreeQual == NULL )
    {
        // Me parameters and constants are captured by value.

        if( ( VariableSymbol->GetVarkind() == VAR_Param && VariableSymbol->IsMe() ) ||
            VariableSymbol->GetVarkind() == VAR_Const )
        {
            Result = ConvertConstantValue( Input, Flags, TargetType );
        }
        else
        {
            if( VariableSymbol->GetVarkind() != VAR_Local &&
                VariableSymbol->GetVarkind() != VAR_Param )
            {
                ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
                return( NULL );
            }

            LambdaParamHandle **LambdaParam = NULL;

            // Port SP1 CL 2935897 to VS 10
            if(VariableSymbol->GetVarkind() == VAR_Param &&
                m_CurrentLambdaParamsMap != NULL &&
               (LambdaParam = m_CurrentLambdaParamsMap->HashFind(VariableSymbol)) &&
               *LambdaParam)
            {
                TREE_TYPE *LambdaParamReference = m_ExprTreeGenerator->CreateLambdaParamReferenceExpr(
                    *LambdaParam,
                    Input->Loc
                    );

                Result = LambdaParamReference;
            }
            else
            {
                Result = ConvertConstantValue( Input, Flags, TargetType );
            }
        }
    }
    else
    {
        BackupValue<bool> backupConvertingQualifiedExpression( &m_ConvertingQualifiedExpression );
        m_ConvertingQualifiedExpression = true;

        Result = m_ExprTreeGenerator->CreateFieldExpr(
            VariableSymbol,
            Input->AsSymbolReferenceExpression().GenericBindingContext->PGenericTypeBinding(),
            ConvertInternalToExpressionTree(
                (ILTree::Expression*)Input->AsSymbolReferenceExpression().ptreeQual,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                ),
            TargetType,
            Input->Loc
            );
    }

    ThrowIfNull( Result );

    return Result;
}

// ----------------------------------------------------------------------------
// Expression trees require widening conversions to be written in the tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertWideningConversions(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_WIDE_COERCE );

    TREE_TYPE *Result = NULL;

    if( Input->AsExpressionWithChildren().Left->bilop == SX_LAMBDA )
    {
        // This can be a nested lambda.
        // Save the converted expression for quoting below.
        Result = ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left,
            Flags,
            NULL   // no specific target type
            );

        // Only lambdas can be quoted, and they are always surrounded by SX_WIDE_COERCE.
        // The quoting happens here, and here only, for both quoting of lambda passed to
        // a method accepting Expression(Of T) as an argument, as well as direct cast
        // CType(Function () ..., Expression(Of T))

        if( m_Semantics->IsOrInheritsFrom(
                Input->ResultType,
                GetFXSymbolProvider()->GetGenericExpressionType()
                )
          )
        {
            Result = m_ExprTreeGenerator->CreateQuoteExpr(
                Result,
                TargetType,
                Input->Loc
                );
        }
    }
    else
    {
        Result = m_ExprTreeGenerator->CreateConvertExpr(
            ConvertInternalToExpressionTree(
                Input->AsExpressionWithChildren().Left,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                ),
            Input->ResultType,
            UseCheckedSemantics(Input),
            TargetType,
            Input->Loc
            );
    }

    return( Result );
}

// ----------------------------------------------------------------------------
// Check whether we should generate expression trees with checked semantics.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::UseCheckedSemantics(ILTree::Expression* Input)
{
    ThrowIfNull( Input );

    if( IsIntegralType( GetVtypeOfTree( Input ) ) && !m_NoIntChecks )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// If the opcode is one that VB allows conversions of nothing to new, default
// structures, and we have a structure and a nothing, then return true.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> bool
ExpressionTreeSemantics<TREE_TYPE>::ShouldConvertNothingToStructureType(
    BILOP opcode,
    ILTree::Expression* PotentialStructureType,
    ILTree::Expression* PotentialNothing
    )
{
    ThrowIfNull( PotentialStructureType );
    ThrowIfNull( PotentialNothing );

    if( OpcodeHasSpecialStructureSemantics( opcode ) &&
        PotentialStructureType->ResultType->IsStruct() &&
        DigAndCheckIfItIsNothing( PotentialNothing ) )
    {
        return( true );
    }
    else
    {
        return( false );
    }
}

// ----------------------------------------------------------------------------
// Helper method that generates an expression tree call to create a new
// structure type.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::CreateNewStructureType(
    Type* StructureType,
    const Location& Loc,
    Type* TargetType
    )
{
    ThrowIfNull( StructureType );
    VSASSERT(
        StructureType->IsStruct() || StructureType->IsGenericParam(),
        "Must be structure type or generic param!" );

    return m_ExprTreeGenerator->CreateNewExpr(
        StructureType,
        TargetType,
        Loc);
}

// ----------------------------------------------------------------------------
// Convert a one-argument VBRuntime method call to an expression tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertRuntimeHelperToExpressionTree(
    RuntimeMembers Helper,
    const Location& Loc,
    Type* ResultType,
    ILTree::Expression* Argument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Argument );
    return( ConvertRuntimeHelperToExpressionTree(
            Helper,
            Loc,
            ResultType,
            Argument,
            NULL,
            Flags,
            TargetType
            )
        );
}

// ----------------------------------------------------------------------------
// Convert a two-argument VBRuntime method call to an expression tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertRuntimeHelperToExpressionTree(
    RuntimeMembers Helper,
    const Location& Loc,
    Type* ResultType,
    ILTree::Expression* FirstArgument,
    ILTree::Expression* SecondArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( FirstArgument );
    return( ConvertRuntimeHelperToExpressionTree(
            Helper,
            Loc,
            ResultType,
            FirstArgument,
            SecondArgument,
            NULL,
            Flags,
            TargetType
            )
        );
}

// ----------------------------------------------------------------------------
// Convert a three-argument VBRuntime method call to an expression tree.
// This will generate operator nodes with the correct method info based on
// the runtime helper if the runtime helper has a corresponding expression tree
// representation. Otherwise, it will generate CALL nodes.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertRuntimeHelperToExpressionTree(
    RuntimeMembers Helper,
    const Location &Loc,
    Type* ResultType,
    ILTree::Expression* FirstArgument,
    ILTree::Expression* SecondArgument,
    ILTree::Expression* ThirdArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( FirstArgument );

    TREE_TYPE *Result = NULL;

    Procedure* Method = GetRuntimeMemberProcedure( Helper, Loc );
    if ( !Method )
    {
        return NULL;
    }

    Parameter* CurrentParameter = Method->GetFirstParam();

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<TREE_TYPE *> ArgumentsList(&Scratch);

    TREE_TYPE *Argument = ConvertInternalToExpressionTree(
        m_Semantics->ConvertWithErrorChecking(
            FirstArgument,
            CurrentParameter->GetType(),
            ExprHasExplicitCastSemantics
            ),
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    ArgumentsList.Add(Argument);

    if( SecondArgument != NULL )
    {
        CurrentParameter = CurrentParameter->GetNext();

        Argument = ConvertInternalToExpressionTree(
            m_Semantics->ConvertWithErrorChecking(
                SecondArgument,
                CurrentParameter->GetType(),
                ExprHasExplicitCastSemantics
                ),
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        ArgumentsList.Add(Argument);
    }

    if( ThirdArgument != NULL )
    {
        CurrentParameter = CurrentParameter->GetNext();

        Argument = ConvertInternalToExpressionTree(
            m_Semantics->ConvertWithErrorChecking(
                ThirdArgument,
                CurrentParameter->GetType(),
                ExprHasExplicitCastSemantics
                ),
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        ArgumentsList.Add(Argument);
    }

    if( Method->IsInstanceConstructor() )
    {
        Result = m_ExprTreeGenerator->CreateNewExpr(
            Method,
            NULL,   // no generic binding context
            &ArgumentsList,
            TargetType,
            Loc
            );
    }
    else
    {
        Result = m_ExprTreeGenerator->CreateCallExpr(
            Method,
            NULL,   // no generic binding context
            NULL,   // no instance
            &ArgumentsList,
            TargetType,
            Loc
            );
    }

    // Check if we need to cast from the result type of the method to the type given.
    // We only do this for nullable types. The only opcode that should do this is
    // SX_POW, until we get support for exponentiation in expression trees.

    if( Method->GetType() != NULL &&
        !BCSYM::AreTypesEqual( Method->GetType(), ResultType ) &&
        TypeHelpers::IsNullableType( ResultType, m_CompilerHost ) )
    {
        Result = m_ExprTreeGenerator->CreateCastExpr(
            Result,
            ResultType,
            NULL,   // no specific target type
            Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Cast operators have a different signature, so we have to special case
// it here.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertCastRuntimeHelperToExpressionTree(
    ILTree::Expression* Input,
    RuntimeMembers Helper,
    const Location &Loc,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_CTYPE ||
                  Input->bilop == SX_DIRECTCAST ||
                  Input->bilop == SX_TRYCAST );

    bool bFoundOperator = false;

    ILTree::Expression* FirstArgument = Input->AsExpressionWithChildren().Left;
    Type* CastType = Input->ResultType;

    Procedure* Method = GetRuntimeMemberProcedure(Helper, Input->Loc);
    if ( !Method )
    {
        return NULL;
    }

    Parameter* FirstParameter = Method->GetFirstParam();

    // This checks whether the cast is a lifted cast.

    bool IsLifted = TypeHelpers::IsNullableType( FirstArgument->ResultType, m_CompilerHost ) &&
        TypeHelpers::IsNullableType( CastType, m_CompilerHost ) &&
        !TypeHelpers::IsNullableType( FirstParameter->GetType(), m_CompilerHost );

    // 

    if( !Method->IsInstanceConstructor() && Input != NULL )
    {
        bFoundOperator = m_ExprTreeGenerator->DoesBilopMapToOperatorNode(
            Input->bilop,
            UseCheckedSemantics(Input)
            );
    }

    if( !bFoundOperator )
    {
        // Just generate the runtime method.

        return ConvertRuntimeHelperToExpressionTree(
            Helper,
            Loc,
            Input->ResultType,
            FirstArgument,
            Flags,
            TargetType
            );
    }
    else
    {
        // There are cases where the tree represents a cast from, say, byte to decimal,
        // but the runtime method that we use converts int to decimal. Thus, we must
        // encode a cast here as well. But we only do this if this is not a lifted
        // runtime operator. However, we can just check Method, because Method is obtained
        // in code gen, rather than through the IsLiftedOperator symbol. Thus, we have to
        // do this unfortunate hack.
        //
        // The real way to fix this is to have semantics bind to the runtime helper and thus
        // encode the correct arguments to the runtime helper.

        if( IsLifted &&
            BCSYM::AreTypesEqual( TypeHelpers::GetElementTypeOfNullable( FirstArgument->ResultType, m_CompilerHost ),
                FirstParameter->GetType() ) )
        {
            // In this case, we have a method that takes a T, and the argument is a T?. We don't want to
            // convert T? to T, because that will throw an exception, and we really want this operator
            // to be lifted.
        }
        else if( BCSYM::AreTypesEqual( FirstArgument->ResultType, FirstParameter->GetType() ) )
        {
            // In this case, the argument matches directly. So don't cast anything here.
        }
        else
        {
            // There could be a situation where we have a cast from short? -> bool?, and the method
            // being used is int -> bool. In this case, we need to convert short? to int?, not to int.

            Type* CastTo = NULL;

            if( IsLifted )
            {
                CastTo = GetFXSymbolProvider()->GetNullableType(
                    Method->GetFirstParam()->GetType(),
                    &m_SymbolCreator
                    );
            }
            else
            {
                CastTo = Method->GetFirstParam()->GetType();
            }

            FirstArgument = m_Semantics->ConvertWithErrorChecking(
                FirstArgument,
                CastTo,
                ExprHasExplicitCastSemantics
                );
        }

        BCSYM * returnType = Method->GetType();

        // If helper doesn't return a Nullable type, but the target type of the conversion is Nullable,
        // pretend that helper returns Nullable type as well. This makes sure we get proper
        // Null propagation.
        if (returnType && TypeHelpers::IsValueType(returnType) && 
            TypeHelpers::IsNullableType( CastType, m_CompilerHost ) &&
            !TypeHelpers::IsNullableType( returnType, m_CompilerHost ))
        {
            returnType = GetFXSymbolProvider()->GetNullableType(
                    returnType,
                    &m_SymbolCreator
                    );    
        }

        TREE_TYPE* convert = ( ConvertUserDefinedCastOperatorToExpressionTree(
                Input,
                m_Semantics->AllocateSymbolReference(
                    Method,
                    TypeHelpers::GetVoidType(),
                    NULL,
                    Loc,
                    NULL
                    ),
                Loc,
                FirstArgument,
                returnType != NULL ? returnType : CastType, // Dev10 #556690 Use method's return type as the cast target type.
                Flags,
                TargetType
                )
            );

        // Dev10 #556690 If return type of the helper method doesn't match the target type of the conversion
        // (for example, conversion from Object to Enum type uses helper that converts to integer type), we need
        // to use extra Conversion.
        if ( returnType != NULL && !BCSYM::AreTypesEqual( CastType, returnType) )
        {  
            convert  = m_ExprTreeGenerator->CreateConvertExpr(
                convert,
                CastType,
                UseCheckedSemantics(Input),
                TargetType,
                Loc
                );
        }

        return convert;
    }
}

// ----------------------------------------------------------------------------
// Boolean needs special handling.
// 



template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertBooleanRuntimeHelperToExpressionTree(
    ILTree::Expression* Input,
    RuntimeMembers Helper,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    ILTree::Expression* SecondArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfNull( FirstArgument );
    ThrowIfNull( SecondArgument );
    ThrowIfFalse( Input->bilop == SX_IS ||
                  Input->bilop == SX_ISNOT ||
                  Input->bilop == SX_EQ ||
                  Input->bilop == SX_NE ||
                  Input->bilop == SX_LE ||
                  Input->bilop == SX_GE ||
                  Input->bilop == SX_LT ||
                  Input->bilop == SX_GT
                  );

    bool bFoundOperator = false;

    Procedure* Method = GetRuntimeMemberProcedure( Helper, Loc );
    if ( !Method )
    {
        return NULL;
    }

    TREE_TYPE *Result = NULL;
    Vtypes vtypeFirstArgument = GetVtypeOfTree( FirstArgument );

    // 
    RuntimeMembers CompareStringHelper = m_CompilerHost->GetSymbolForRuntimeClass(VBEmbeddedOperatorsClass, NULL)? EmbeddedCompareStringMember : CompareStringMember;

    if( Input != NULL &&
        vtypeFirstArgument != t_ref &&
        Helper != CompareStringHelper &&
        !Method->IsInstanceConstructor() )
    {
        bFoundOperator = m_ExprTreeGenerator->DoesBilopMapToOperatorNode(
            Input->bilop,
            UseCheckedSemantics(Input)
            );
    }

    if( !bFoundOperator )
    {
        // Just generate the runtime method. We need to encode a potential
        // argument for the compare parameter.

        ILTree::Expression* OptionCompare = NULL;

        if( Helper == CompareStringHelper ||
            ( vtypeFirstArgument == t_ref && Input->bilop != SX_IS && Input->bilop != SX_ISNOT )
          )
        {
            OptionCompare = m_Semantics->ProduceConstantExpression(
                ( Input->uFlags & SXF_RELOP_TEXT ) ? 1 : 0,
                Input->Loc,
                GetFXSymbolProvider()->GetIntegerType()
                IDE_ARG(0)
                );
        }

        Result = ConvertRuntimeHelperToExpressionTree(
            Helper,
            Input->Loc,
            Input->ResultType,
            FirstArgument,
            SecondArgument,
            OptionCompare,
            Flags,
            TargetType
            );
    }
    else
    {
        // The arguments for boolean are expression, expression, bool, methodinfo.
        // We must set true so that we lift to null by setting LiftToNull = true.

        Parameter* CurrentParameter = Method->GetFirstParam();

        TREE_TYPE *FirstConvertedArgument = ConvertInternalToExpressionTree(
            FirstArgument,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        CurrentParameter = CurrentParameter->GetNext();
        TREE_TYPE *SecondConvertedArgument = ConvertInternalToExpressionTree(
            SecondArgument,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
            Input->bilop,
            FirstConvertedArgument,
            SecondConvertedArgument,
            UseCheckedSemantics(Input),
            true,   // lift to null
            Method, // helper method
            NULL,   // no generic binding context
            TargetType,
            Loc
            );
    }

    ThrowIfNull( Result );

    // Now, we need to generate the equal, but only if we are generaing a call to
    // compare string.

    if( Helper == CompareStringHelper || Helper == CompareDateMember || Helper == CompareDecimalMember )
    {
        ILTree::Expression* ConstantExpression = m_Semantics->ProduceConstantExpression(
            0,
            Input->Loc,
            GetFXSymbolProvider()->GetIntegerType()
            IDE_ARG(0)
            );

        Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
            Input->bilop,
            Result,
            ConvertInternalToExpressionTree(ConstantExpression, Flags, NULL),
            UseCheckedSemantics(Input),
            TargetType,
            Loc
            );
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert one argument runtime helpers to Expression trees.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertRuntimeHelperOperatorToExpressionTree(
    ILTree::Expression* Input,
    RuntimeMembers Helper,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    return( ConvertRuntimeHelperOperatorToExpressionTree(
            Input,
            Helper,
            Loc,
            FirstArgument,
            NULL,
            Flags,
            TargetType
            )
        );
}

// ----------------------------------------------------------------------------
// Convert two-argument operators to expression trees.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertRuntimeHelperOperatorToExpressionTree(
    ILTree::Expression* Input,
    RuntimeMembers Helper,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    ILTree::Expression* SecondArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfNull( FirstArgument );

    TREE_TYPE *Result = NULL;

    bool bFoundOperator = false;

    if( Input != NULL )
    {
        bFoundOperator = m_ExprTreeGenerator->DoesBilopMapToOperatorNode(
            Input->bilop,
            UseCheckedSemantics(Input)
            );
    }

    if( !bFoundOperator )
    {
        // Just generate the runtime method.

        Result = ConvertRuntimeHelperToExpressionTree(
            Helper,
            Loc,
            Input->ResultType,
            FirstArgument,
            SecondArgument,
            Flags,
            TargetType
            );
    }
    else
    {
        Procedure* Method = GetRuntimeMemberProcedure( Helper, Loc );
        if ( !Method )
        {
            return NULL;
        }

        Parameter* CurrentParameter = Method->GetFirstParam();

        TREE_TYPE *FirstConvertedArgument = NULL;
        TREE_TYPE *SecondConvertedArgument = NULL;

        FirstConvertedArgument = ConvertInternalToExpressionTree(
            FirstArgument,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );

        if( SecondArgument == NULL )
        {
            Result = m_ExprTreeGenerator->CreateUnaryOperatorExpr(
                Input->bilop,
                FirstConvertedArgument,
                UseCheckedSemantics(Input),
                Method,
                NULL,   // no generic binding context
                TargetType,
                Loc
                );
        }
        else
        {
            CurrentParameter = CurrentParameter->GetNext();
            SecondConvertedArgument = ConvertInternalToExpressionTree(
                SecondArgument,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                );

            Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
                Input->bilop,
                FirstConvertedArgument,
                SecondConvertedArgument,
                UseCheckedSemantics(Input),
                false, // don't need LiftToNull
                Method,
                NULL,   // no generic binding context
                TargetType,
                Loc
                );
        }
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert a method to the corresponding operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertUserDefinedOperatorToExpressionTree
(
    ILTree::Expression* Input,
    ILTree::Expression* MethodTree,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfNull( MethodTree );
    ThrowIfNull( FirstArgument );

    return( ConvertUserDefinedOperatorToExpressionTree(
            Input,
            MethodTree,
            Loc,
            FirstArgument,
            NULL,
            Flags,
            TargetType
            )
        );
}

// ----------------------------------------------------------------------------
// Convert a method to the corresponding operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertUserDefinedOperatorToExpressionTree
(
    ILTree::Expression* Input,
    ILTree::Expression* MethodTree,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    ILTree::Expression* SecondArgument,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfNull( MethodTree );
    ThrowIfNull( FirstArgument );

    TREE_TYPE *Result = NULL;

    BILOP opcode;
    Procedure* Method = ViewAsProcedure( MethodTree->AsSymbolReferenceExpression().pnamed );

    // If the method is lifted, we must re-write it with the actual method.

    if( Method->IsLiftedOperatorMethod() )
    {
        Method = Method->PLiftedOperatorMethod()->GetActualProc();
        MethodTree->AsSymbolReferenceExpression().pnamed = Method;
        ThrowIfNull( Method );
    }

    // This can be a user defined operator that came through the first phase (and thus has not been lowered),
    // or it could have been lowered to a Call node.

    if( Input->bilop == SX_CALL )
    {
        opcode = Input->AsCallExpression().OperatorOpcode;
    }
    else
    {
        opcode = Input->bilop;
    }

    // We should only call this API if we can map the name.

    ThrowIfFalse(
        m_ExprTreeGenerator->DoesBilopMapToOperatorNode(
            opcode,
            UseCheckedSemantics(Input)
            )
        );

    TREE_TYPE *FirstConvertedArgument = NULL;
    TREE_TYPE *SecondConvertedArgument = NULL;

    // For the two arguments, we must check whether we need to emit
    // new structure types for "nothing".
    // For example, if x is a structure type and we see:
    // x <> nothing
    // We must emit a "new structure" and call the operator on
    // (x, new structure).

    if( SecondArgument != NULL &&
        ShouldConvertNothingToStructureType(
            opcode,
            SecondArgument,
            FirstArgument
            )
      )
    {
        // Ok, instead of converting SX_NOTHING, create a new structure.

        FirstConvertedArgument = CreateNewStructureType(
            SecondArgument->ResultType,
            Input->Loc,
            TargetType
            );
    }
    else
    {
        FirstConvertedArgument = ConvertInternalToExpressionTree(
            FirstArgument,
            Flags,
            GetFXSymbolProvider()->GetExpressionType()
            );
    }


    if (SecondArgument == NULL)
    {
        Result = m_ExprTreeGenerator->CreateUnaryOperatorExpr(
            opcode,
            FirstConvertedArgument,
            UseCheckedSemantics(Input),
            MethodTree->AsSymbolReferenceExpression().Symbol->PProc(),
            MethodTree->AsSymbolReferenceExpression().GenericBindingContext,
            TargetType,
            Loc
            );
    }
    else
    {
        // Do the same for the second argument.

        if( ShouldConvertNothingToStructureType(
                opcode,
                FirstArgument,
                SecondArgument
                )
          )
        {
            // Ok, instead of converting SX_NOTHING, create a new structure.

            SecondConvertedArgument = CreateNewStructureType(
                FirstArgument->ResultType,
                Input->Loc,
                TargetType
                );
        }
        else
        {
            SecondConvertedArgument = ConvertInternalToExpressionTree(
                SecondArgument,
                Flags,
                GetFXSymbolProvider()->GetExpressionType()
                );
        }

        // We always want to lift to bool? if we have a logical operator.
        if( IsBooleanOperator( opcode ) )
        {
            ThrowIfNull( SecondArgument );

            Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
                opcode,
                FirstConvertedArgument,
                SecondConvertedArgument,
                UseCheckedSemantics(Input),
                true,   // lift to null
                MethodTree->AsSymbolReferenceExpression().Symbol->PProc(),
                MethodTree->AsSymbolReferenceExpression().GenericBindingContext,
                TargetType,
                Loc
                );
        }
        else
        {
            Result = m_ExprTreeGenerator->CreateBinaryOperatorExpr(
                opcode,
                FirstConvertedArgument,
                SecondConvertedArgument,
                UseCheckedSemantics(Input),
                false, // don't need LiftToNull
                MethodTree->AsSymbolReferenceExpression().Symbol->PProc(),
                MethodTree->AsSymbolReferenceExpression().GenericBindingContext,
                TargetType,
                Loc
                );
        }
    }

    return Result;
}

// ----------------------------------------------------------------------------
// Convert a method to the corresponding operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertUserDefinedCastOperatorToExpressionTree
(
    ILTree::Expression* Input,
    ILTree::SymbolReferenceExpression* MethodTree,
    const Location &Loc,
    ILTree::Expression* FirstArgument,
    Type* CastType,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfNull( MethodTree );
    ThrowIfFalse( MethodTree->bilop == SX_SYM );
    ThrowIfNull( FirstArgument );
    ThrowIfNull( CastType );

    Procedure* Method = ViewAsProcedure( MethodTree->AsSymbolReferenceExpression().pnamed );

    // If the method is lifted, we must re-write it with the actual method.

    if( Method->IsLiftedOperatorMethod() )
    {
        Method = Method->PLiftedOperatorMethod()->GetActualProc();
        MethodTree->AsSymbolReferenceExpression().pnamed = Method;
        ThrowIfNull( Method );
    }

    BILOP opcode;

    // This can be a user defined operator that came through the first phase (and thus has not been lowered),
    // or it could have been lowered to a Call node.

    if( Input->bilop == SX_CALL )
    {
        opcode = Input->AsCallExpression().OperatorOpcode;
    }
    else
    {
        opcode = Input->bilop;
    }

    ThrowIfFalse( opcode == SX_CTYPE ||
                  opcode == SX_DIRECTCAST ||
                  opcode == SX_TRYCAST );

    TREE_TYPE *ConvertedArgument = ConvertInternalToExpressionTree(
        FirstArgument,
        Flags,
        GetFXSymbolProvider()->GetExpressionType()
        );

    return m_ExprTreeGenerator->CreateConvertExpr(
        ConvertedArgument,
        CastType,
        UseCheckedSemantics(Input),
        MethodTree->Symbol->PProc(),
        MethodTree->GenericBindingContext,
        TargetType,
        Loc
        );
}

// ----------------------------------------------------------------------------
// Create the lambda expression for the coalesce operator.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::CreateLambdaForCoalesce(
    TREE_TYPE* LambdaBody,
    LambdaParamHandle *LambdaParameter,
    Type* LeftHandType,
    Type* RightHandType,
    Type* TargetType,
    const Location& Loc,
    NorlsAllocator &Allocator
    )
{
    ThrowIfNull( LambdaBody );
    ThrowIfNull( LambdaParameter );
    ThrowIfNull( LeftHandType );
    ThrowIfNull( RightHandType );

    // The lambda delegate type here is Func(Of LeftType, RightType).

    Type* LambdaTypeArgs[ 2 ];
    LambdaTypeArgs[ 0 ] = LeftHandType;
    LambdaTypeArgs[ 1 ] = RightHandType;

    Type *DelegateType =  GetFXSymbolProvider()->GetFuncSymbol( 2, LambdaTypeArgs, &m_SymbolCreator );

    // Build the call to the lambda now.

    ArrayList_NorlsAllocBased<LambdaParamHandle *> LambdaParameters(&Allocator);
    LambdaParameters.Add(LambdaParameter);

    TREE_TYPE *Result = m_ExprTreeGenerator->CreateLambdaExpr(
        &LambdaParameters,
        LambdaBody,
        DelegateType,
        GetFXSymbolProvider()->GetType(FX::LambdaExpressionType),
        Loc
        );
        
    // If the flag is set, we need to quote.

    // Bug 114270: not quoting since quoting the lambda for the coalesce operator
    // since that will cahnge the type of Result from LambdaExpression to UnaryExpression.
    // This will fail the call to Coalesc whose signature is (Expresion,Expression,LambdaExpression)
    // since Expressions.UnaryExpresion is not convertable to Expressions.LambdaExpression
    // this call will fail.

    return( Result );
}

// ----------------------------------------------------------------------------
// Given a method symbol and an argument, build a call expression for
// this method.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::BuildCallExpressionFromMethod(
    Procedure* Method,
    ILTree::Expression* Operand,
    LambdaParamHandle *ExprTreeLambdaParameter,
    const Location& Loc,
    NorlsAllocator &Allocator
    )
{
    ThrowIfNull( Method );
    ThrowIfNull( Operand );
    ThrowIfNull( ExprTreeLambdaParameter );

    // It is possible that we need to emit a cast as well, from a nullable type to a non-nullable
    // type. Check that here.

    Parameter* CurrentParameter = Method->GetFirstParam();

    TREE_TYPE *ParameterReference = m_ExprTreeGenerator->CreateLambdaParamReferenceExpr(
        ExprTreeLambdaParameter,
        Loc
        );

    VSASSERT( TypeHelpers::GetElementTypeOfNullable( Operand->ResultType, m_CompilerHost ) ==
        TypeHelpers::GetElementTypeOfNullable( CurrentParameter->GetType(), m_CompilerHost ),
        "How come the IsTrue operator has types that don't match?" );

    if( TypeHelpers::IsNullableType( Operand->ResultType, m_CompilerHost ) &&
        !TypeHelpers::IsNullableType( CurrentParameter->GetType(), m_CompilerHost ) )
    {
        // Here, we need to emit a convert.

        ParameterReference = m_ExprTreeGenerator->CreateCastExpr(
            ParameterReference,
            CurrentParameter->GetType(),
            NULL,   // no specific target type
            Loc);
    }

    // The argument to the call is the parameter reference, which may have been
    // converted above.

    ArrayList_NorlsAllocBased<TREE_TYPE *> ArgumentList(&Allocator);
    ArgumentList.Add(ParameterReference);

    VSASSERT(Method->IsShared(), "operator methods should be static!");

    return m_ExprTreeGenerator->CreateCallExpr(
        Method,
        NULL,   // No generic binding context
        NULL,   // Method is static and hence no instance
        &ArgumentList,
        NULL,   // no specific target type
        Loc
        );
}

// ----------------------------------------------------------------------------
// Return the VType of the tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> Vtypes
ExpressionTreeSemantics<TREE_TYPE>::GetVtypeOfTree(ILTree::Expression* Input)
{
    ThrowIfNull( Input );

    if( TypeHelpers::IsNullableType( Input->ResultType, m_CompilerHost ) )
    {
        return( TypeHelpers::GetElementTypeOfNullable( Input->ResultType, m_CompilerHost )->GetVtype() );
    }
    else
    {
        return( Input->vtype );
    }

}

// ----------------------------------------------------------------------------
// For boolean conversions, we must find the matching signed types of the
// unsigned type in order to do the proper negation semantics.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> Type*
ExpressionTreeSemantics<TREE_TYPE>::GetTypeForBooleanConversion( Vtypes vt )
{
    switch( vt )
    {
    case t_i1:
        // SBytes have no support in expression tree.
        return GetFXSymbolProvider()->GetIntegerType();

    case t_ui1:
        // Bytes have no support in expression tree.
        return GetFXSymbolProvider()->GetIntegerType();

    case t_ui2:
        return GetFXSymbolProvider()->GetShortType();

    case t_ui4:
        return GetFXSymbolProvider()->GetIntegerType();

    case t_ui8:
        return GetFXSymbolProvider()->GetLongType();

    default:
        VSASSERT( false, "Type unsupported for GetTypeForBooleanConversion" );
        return( NULL );
    }
}

// ----------------------------------------------------------------------------
// Convert SX_BAD nodes by creating a bad expression.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertBadExpression(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfFalse(Input->bilop == SX_BAD);

    return m_ExprTreeGenerator->CreateBadExpr( Input->ResultType, TargetType, Input->Loc );
}

// ----------------------------------------------------------------------------
// Convert SX_ADR nodes by just converting the referenced entity (the local
// reference, etc.)
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertAdr(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfFalse(Input->bilop == SX_ADR);

    return ConvertInternalToExpressionTree(
        Input->AsExpressionWithChildren().Left,
        Flags,
        TargetType);
}

// ----------------------------------------------------------------------------
// Conversion of late bound expressions is not supported
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertLateBoundExpression(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfFalse(
        Input->bilop == SX_LATE ||
        Input->bilop == SX_VARINDEX
        );

    ReportErrorForExpressionTree( ERRID_ExprTreeNoLateBind, Input->Loc );
    return NULL;
}

// ----------------------------------------------------------------------------
// Conversion of a unary plus expression.
//
// Note that the unary plus operator itself is not encoded in the tree and treated
// as a nop currently. This is the orcas behavior.
//
// 



template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertPlus(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_PLUS );

    if ( GetVtypeOfTree( Input ) == t_ref )
    {
        return ConvertRuntimeHelperOperatorToExpressionTree(
            Input,
            PlusObjectMember,
            Input->Loc,
            Input->AsExpressionWithChildren().Left,
            Flags,
            TargetType
            );
    }
    else
    {
        return ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left,
            Flags,
            NULL);	// Why NULL here rather than TargetType?
                    // Preserving compat by keeping this the same as the previous code
                    // from which this was re-factored.
    }
}

// ----------------------------------------------------------------------------
// Conversion of SX_SYNCLOCK_




template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertSynclockCheck(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfFalse(Input->bilop == SX_SYNCLOCK_CHECK);

    ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
    return NULL;
}

// ----------------------------------------------------------------------------
// Conversion of SX_IF nodes is not supported.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertIf(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfFalse(Input->bilop == SX_IF);

    ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
    return NULL;
}

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::HandleUnexpectedExprKind(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    VSASSERT( m_CoalesceLambdaParameter == NULL && m_CoalesceArgumentToBeReplaced == NULL,
            "Why do we have the coalescing lambda parameters here?" );

    ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );
    return NULL;
}

// ----------------------------------------------------------------------------
// Convert the VB expressions to an expression tree.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertInternalToExpressionTree(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
    )
{
    ThrowIfNull( Input );

    // WARNING WARNING:
    //
    // This is purely a dispatch method and no ET conversion related semantics
    // should be handled in this method (even if terms of errors). Any ET
    // conversion semantics should be handled in the appropriate virtual Convert*
    // method so that the conversion semantics can then be overridden in any
    // specialization of ExpressionTreeSemantics.

    // Microsoft: we should never be an expression tree that has been converted.
    // If we are an expression tree here, it means that we want to encode an access
    // to it.

    // When converting a lambda for Coalesce, we may need to hijack the tree and replace
    // it with the parameter of the lambda. Do that here.

    if( m_CoalesceLambdaParameter != NULL && m_CoalesceArgumentToBeReplaced != NULL )
    {
        // Now, do it only if the current expression matches.

        if( Input == m_CoalesceArgumentToBeReplaced )
        {
            return m_ExprTreeGenerator->CreateLambdaParamReferenceExpr(
                m_CoalesceLambdaParameter,
                Input->Loc
                );
        }
    }
    else
    {
        VSASSERT( m_CoalesceLambdaParameter == NULL && m_CoalesceArgumentToBeReplaced == NULL,
            "Why do we have the coalescing lambda parameters here?" );
    }

    // WARNING WARNING:
    //
    // This is purely a dispatch method and no ET conversion related semantics
    // should be handled in this method (even if terms of errors). Any ET
    // conversion semantics should be handled in the appropriate virtual Convert*
    // method so that the conversion semantics can then be overridden in any
    // specialization of ExpressionTreeSemantics.

    // Dispatch to the appropriate Convert routines using the visitor impl in
    // InvokeConvertMethod.

    TREE_TYPE *ExpressionResult = InvokeConvertMethod(Input, Flags, TargetType);

    if( ExpressionResult != NULL )
    {
        return( ExpressionResult );
    }
    else
    {
        ReportErrorForExpressionTree( ERRID_ExpressionTreeNotSupported, Input->Loc );

        // Clear the error
        m_LastExprTreeErrID = 0;

        return m_ExprTreeGenerator->CreateBadExpr(
            Input->ResultType,
            GetFXSymbolProvider()->IsTypeAvailable(FX::ExpressionType) ?
                GetFXSymbolProvider()->GetType(FX::ExpressionType) :
                NULL,
            Input->Loc
            );
    }
}

// ----------------------------------------------------------------------------
// Convert lambdas to expression trees.
// ----------------------------------------------------------------------------

template<class TREE_TYPE> TREE_TYPE*
ExpressionTreeSemantics<TREE_TYPE>::ConvertLambdaToExpressionTree(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
    )
{
    ThrowIfNull( Input );

    return( ConvertInternalToExpressionTree(
                Input,
                Flags,
                TargetType
                )
          );
}
