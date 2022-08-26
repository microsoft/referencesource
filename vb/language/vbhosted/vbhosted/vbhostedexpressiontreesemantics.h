//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// VBHostedExpressionTreeSemantics:
//      This is a specialization of the ExpressionTreeSemantics class. The ExpressionTreeSemantics
// class supports the VB core compiler requirements, whereas the HostedExpressionTreeSemantics
// extends the ExpressionTreeSemantics class to support the VB hosted compiler requirements
// to enable emitting all VB code as DLR ASTs.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#include "sal.h"
#include "DLRTreeETGenerator.h"
#include "ExpressionTreeSemantics.h"

typedef FixedSizeHashTable<64, Variable *, DLRExpressionTree *> VariablesMap;

class VBHostedExpressionTreeSemantics : public ExpressionTreeSemantics<DLRExpressionTree>
{
public:
    VBHostedExpressionTreeSemantics
    (
        Semantics *SemanticsContext,
        DLRTreeETGenerator *ExprTreeGenerator,
        NorlsAllocator *Allocator
    ) :
        ExpressionTreeSemantics<DLRExpressionTree>(SemanticsContext, ExprTreeGenerator, NULL),
        m_ExprTreeGenerator(ExprTreeGenerator),
        m_VariablesMap(NULL),
        m_VariablesList(NULL),
        m_SEQExprContainingDeferredTempInit(NULL),
        m_OriginalInputExpr(NULL)
    {
    }

    virtual DLRExpressionTree* ConvertLambdaToExpressionTree(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

protected:

    // Convert methods that override base class methods.

    virtual DLRExpressionTree* ConvertConstantValue(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

    virtual DLRExpressionTree* ConvertSEQ(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

    virtual DLRExpressionTree* ConvertSymbol(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

    virtual DLRExpressionTree* ConvertAssignment(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
        );

    virtual DLRExpressionTree* ConvertLambda(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    virtual DLRExpressionTree* ConvertExpressionLambda(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    virtual ILTree::Expression* ExtractArgumentForByrefParam(
        ILTree::Expression *Argument
    );

    virtual DLRExpressionTree* ConvertLateBoundExpression(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    virtual DLRExpressionTree* ConvertNewObjectInitializer(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    virtual DLRExpressionTree* ConvertAnonymousType(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    virtual DLRExpressionTree* ConvertWideningConversions(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    virtual IReadOnlyList<DLRExpressionTree*>* CreateArgumentListForExpressionTree
    (
        ExpressionList * ArgumentExpressions,
        ExpressionFlags Flags,
        Symbol * TargetType,
        BCSYM_Proc * TargetMethod,    
        const Location &Loc,
        NorlsAllocator &Allocator
    );

    void CreateArgumentListForCallOrNewWithCopyBack
    (
        ExpressionList* ArgumentExpressions,
        ExpressionFlags Flags,
        Symbol* TargetType,
        BCSYM_Proc* TargetMethod,
        const Location &Loc,
        ArrayList_NorlsAllocBased<DLRExpressionTree *> &Initializers,
        ArrayList_NorlsAllocBased<DLRExpressionTree *> &Arguments
    );

    virtual DLRExpressionTree* ConvertCast(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    // Dev10 bug 822565 - Since there is a difference in semantics for decimal conversions
    // (rounding vs truncating) between lambdas converted to expr trees and other contexts,
    // overriding this method in order to ensure compat with core compiler in non-expr tree
    // lambda contexts.
    //
    RuntimeMembers GetHelperCastFromDecimal(
        Vtypes vtypeTo
    )
    {
        return CodeGenerator::GetHelperFromDecimal(vtypeTo);
    }

private:

    DLRExpressionTree* ConvertArrayAssignment(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    DLRExpressionTree* ConvertVariable(
        Variable* VariableSymbol,
        Type* TargetType,
        const Location &Loc
    );

    static bool IsCallOrNewWithCopyBack(
        ILTree::Expression *Input
    );

    DLRExpressionTree* ConvertCallOrNewWithCopyBack(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    void ConvertCallOrNewWithCopyBack(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        ArrayList_NorlsAllocBased<DLRExpressionTree *> &ExprSequence,
        DLRExpressionTree *&ReturnValueTemp
    );

    DLRExpressionTree* ConvertVarIndexExpression(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    DLRExpressionTree* ConvertLateExpression(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    ILTree::Expression*
    TransformDeferredTemps(
        ILTree::Expression *Input
    );

    static bool IsSeqLateCall(
        ILTree::Expression *Input
    );

    DLRExpressionTree* ConvertSeqLateExpression(
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    DLRExpressionTree* CreateTempVariable(
        _In_z_ WCHAR *VarName,
        Type *VarType,
        const Location &Loc
    );

    // Data members

    DLRTreeETGenerator *m_ExprTreeGenerator;
    
    // Hash to map from BCSYM_Variables to the corresponding created
    // VariableExpressions. Starts out as NULL.
    VariablesMap *m_VariablesMap;

    // List used to track all the allocated temporaries so that they can
    // can be emitted into a scope block. Starts out as NULL.
    ArrayList_NorlsAllocBased<DLRExpressionTree *> *m_VariablesList;

    // This is used to store the input tree after it is transformed when
    // replacing the deferred temps in the tree. It is only set if there
    // is any deferred temp specific init required.
    //
    // Unfortunately there is no marker on the bound tree itself to detect
    // this transformed tree, hence need to store this to identify and
    // handle this scenario specially in ConvertSEQ.
    //
    ILTree::Expression *m_SEQExprContainingDeferredTempInit;

    // This is used to store the original input expression pass in for
    // conversion to expr tree. This needs to be stored here so that
    // ConvertSEQ can distinguish between deferred temp init vs the
    // the actual expression being converted.
    //
    ILTree::Expression *m_OriginalInputExpr;

    // List used to store the initiallaziers for the arguments in a By Ref 
    // call with copy back.
    ArrayList_NorlsAllocBased<DLRExpressionTree *> *m_ByRefArgumentTempInitializers;

    // This is used to store the argument list of a By Ref call with copy 
    // back to indicate it needs special handeling.
    ILTree::Expression *m_ByRefWithCopyBackArguments;
};
