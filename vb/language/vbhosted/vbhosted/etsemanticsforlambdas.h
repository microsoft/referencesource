//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ETSemanticsForLambdas:
//      This is a specialization of the ExpressionTreeSemantics class. The ExpressionTreeSemantics
// class supports the VB core compiler requirements for converting lambdas to expression trees,
// and this class extends the ExpressionTreeSemantics class to handle the conversions of lambdas
// to expression trees for hosted compiler scenarios. Note that for any lambda, the expression trees
// produced by this class should be exactly identical in shape to the trees produced by the
// ExpressionTreeSemantics class in the core compiler. The trees need to be identical so that
// existing ET consumers like DLINQ, third party API, etc. see exactly the same shaped trees for
// lambdas compiled through both the normal VB compiler and the hosted VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#include "ExpressionTreeSemantics.h"
#include "VBHostedExpressionTreeSemantics.h"
#include "DLRTreeETGenerator.h"
#include "BoundTreeVisitor.h"


class ETSemanticsForLambdas : public ExpressionTreeSemantics<DLRExpressionTree>
{
    friend class VBHostedExpressionTreeSemantics;

public:

    ETSemanticsForLambdas
    (
        Semantics *SemanticsContext,
        DLRTreeETGenerator *ExprTreeGenerator, 
        VariablesMap *VariablesMap,
        LambdaParamsMap *CurrentLambdaParamsMap
    ) :
        ExpressionTreeSemantics<DLRExpressionTree>(SemanticsContext, ExprTreeGenerator, NULL),
        m_ExprTreeGenerator(ExprTreeGenerator),
        m_VariablesMap(VariablesMap)
    {
        ThrowIfNull(SemanticsContext);
        ThrowIfNull(ExprTreeGenerator);
        ThrowIfNull(VariablesMap);

        m_CurrentLambdaParamsMap = CurrentLambdaParamsMap;
    }

protected:

    // Convert methods that override base class methods.

    DLRExpressionTree* ConvertConstantValue
    (
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    DLRExpressionTree* ConvertSymbol
    (
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    DLRExpressionTree* ConvertLambda(
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType
    );

    // Helpers

    static DLRExpressionTree*
    ConvertConstantValueHelper
    (
        ILTree::Expression* Input,
        ExpressionFlags Flags,
        Type* TargetType,
        DLRTreeETGenerator *ExprTreeGenerator,
        FX::FXSymbolProvider *FXSymbolProvider
    );

private:

    // Data members

    // 






    DLRTreeETGenerator *m_ExprTreeGenerator;

    VariablesMap *m_VariablesMap;
};

