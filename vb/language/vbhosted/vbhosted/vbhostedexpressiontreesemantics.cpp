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

#include "StdAfx.h"

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertConstantValue
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    // -----------------------------------Contract start------------------------------------

    ThrowIfNull(Input);

    ThrowIfFalse(Input->bilop == SX_NOTHING ||
                 Input->bilop == SX_CNS_INT ||
                 Input->bilop == SX_CNS_FLT ||
                 Input->bilop == SX_CNS_DEC ||
                 Input->bilop == SX_CNS_STR ||
                 Input->bilop == SX_METATYPE);

    // 




    // -----------------------------------Contract end------------------------------------

    return ETSemanticsForLambdas::ConvertConstantValueHelper(
        Input,
        Flags,
        TargetType,
        m_ExprTreeGenerator,
        GetFXSymbolProvider()
        );
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertSEQ
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    if (Input == m_SEQExprContainingDeferredTempInit)
    {
        // The tree contains deferred temp initialization and need to be converted
        // in a special way.

        ThrowIfFalse(Input->bilop == SX_SEQ);

        NorlsAllocator Scratch(NORLSLOC);
        ArrayList_NorlsAllocBased<DLRExpressionTree *> Expressions(&Scratch);
        
        // Build the expr trees for all the deferred temp initializers

        ILTree::Expression *Current = Input;

        while (Current != NULL)
        {
            ILTree::Expression *Expression;
            DLRExpressionTree *Initializer;

            if( Current->bilop == SX_SEQ )
            {
                Expression = Current->AsExpressionWithChildren().Left;
                Current = Current->AsExpressionWithChildren().Right;
            }
            else
            {
                Expression = Current;
                Current = NULL;
            }

            Initializer = ConvertInternalToExpressionTree(
                Expression,
                Flags,
                TargetType
                );

            Expressions.Add(Initializer);
        }

        // Create a block to hold the sequence of expressions
        return m_ExprTreeGenerator->CreateBlockExpr(
            &Expressions,
            TargetType,
            Input->Loc
            );
    }
    else if( Input->AsExpressionWithChildren().Right &&
             Input->AsExpressionWithChildren().Right->bilop == SX_SYM &&
             Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed &&
             IsVariable( Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed ) &&
             Input->AsExpressionWithChildren().Right->AsSymbolReferenceExpression().pnamed->PVariable()->IsTemporary() &&
             Input->ResultType->IsArrayType() &&
             Input->ResultType->PArrayType()->GetRank() > 1)
    {
        //Multi dimensional array initializer

        ILTree::Expression *ArrayAssignment = Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left;
        ILTree::Expression *Initializers = Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right;
        ILTree::Expression *ArrayVarNode = ArrayAssignment->AsExpressionWithChildren().Left;
        Variable *ArraySymbol = ArrayVarNode->AsSymbolReferenceExpression().pnamed->PVariable();
        
        //Ref to the temp var
        DLRExpressionTree *ArrayVarExpr = ConvertVariable(ArraySymbol, TargetType, ArrayVarNode->Loc);
        //Assignment to the temp var
        DLRExpressionTree *ArrayAssignmentExpr = ConvertInternalToExpressionTree(ArrayAssignment, Flags, TargetType);

        NorlsAllocator Scratch(NORLSLOC);
        ArrayList_NorlsAllocBased<DLRExpressionTree *> Expressions(&Scratch);
        
        //Start the block off with creating the array
        Expressions.Add(ArrayAssignmentExpr);
        
        //Loop through the array assignments
        while( Initializers != NULL )
        {
            ILTree::Expression *Initializer;
            DLRExpressionTree *Element;

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
            
            Element = ConvertArrayAssignment(
                Initializer,
                Flags,
                TargetType
                );

            Expressions.Add(Element);
        }

        //End the block with the temp var so that the block evaluates to the array
        Expressions.Add(ArrayVarExpr);

        // add array temp to list of variables
        // Don't need to add this here again since it is already added in ConvertVariable
        //m_VariablesList->Add(ArrayVarExpr);

        return m_ExprTreeGenerator->CreateBlockExpr(
            &Expressions,
            TargetType,
            Input->Loc
            );
    }
    else if (IsCallOrNewWithCopyBack(Input))
    {
        // This should be the case where copy-out assignments are appended on to a
        // call node (or a constructor). The base implementation ignores the copy-out.
        // But for hosted compiler scenarios, copy-back/copy-out also needs to be
        // supported.

        return ConvertCallOrNewWithCopyBack(
            Input,
            Flags,
            TargetType
            );
    }
    else if (IsSeqLateCall(Input))
    {
        return ConvertSeqLateExpression(Input, Flags, TargetType);
    }
    else
    {
        //Use default implementation in all other cases
        return ExpressionTreeSemantics<DLRExpressionTree>::ConvertSEQ(Input, Flags, TargetType);
    }
}

bool
VBHostedExpressionTreeSemantics::IsCallOrNewWithCopyBack
(
    ILTree::Expression *Input
)
{
    ILTree::Expression *Current = Input;
    while (Current && Current->bilop == SX_SEQ_OP1)
    {
        Current = Current->AsExpressionWithChildren().Left;
    }

    return (Input->bilop == SX_SEQ_OP1 &&
            Current &&
            (Current->bilop == SX_CALL || Current->bilop == SX_NEW));
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertCallOrNewWithCopyBack
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type *TargetType
)
{
    ThrowIfNull(Input);
    ASSERT(IsCallOrNewWithCopyBack(Input), "[VBHostedExpressionTreeSemantics::ConvertCallOrNewWithCopyBack]");

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree *> ExprSequence(&Scratch);

    // The temporary variable that holds the result of the method call so that
    // the value of the call can be loaded at the end of the expr sequence
    // after the copy-back.
    //
    DLRExpressionTree *ReturnValueTemp = NULL;

    // Walk the tree recursively and build the expression starting with the
    // left most leaf first.

    ConvertCallOrNewWithCopyBack(
        Input,
        Flags,
        ExprSequence,
        ReturnValueTemp
        );

    // End the expr sequence/block with the temp var so that the block evaluates
    // to the value of the call or new expr
    //
    ExprSequence.Add(ReturnValueTemp);

    // Now combine all the expressions built as a sequence of expressions in a block
    return m_ExprTreeGenerator->CreateBlockExpr(
        &ExprSequence,
        TargetType,
        Input->Loc
        );
}

void
VBHostedExpressionTreeSemantics::ConvertCallOrNewWithCopyBack
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    ArrayList_NorlsAllocBased<DLRExpressionTree *> &ExprSequence,
    DLRExpressionTree *&ReturnValueTemp
)
{
    ThrowIfNull(Input);

    // This should be the case where copy-out assignments are appended on to a
    // call node (or a constructor). The base implementation ignores the copy-out.
    // But for hosted compiler scenarios, copy-back/copy-out also needs to be
    // supported.

    if (Input->bilop == SX_SEQ_OP1)
    {
        ConvertCallOrNewWithCopyBack(
            Input->AsExpressionWithChildren().Left,
            Flags,
            ExprSequence,
            ReturnValueTemp
            );

        ILTree::Expression *CopyOut = Input->AsExpressionWithChildren().Right;
            
        DLRExpressionTree *CopyOutExprTree = ConvertInternalToExpressionTree(
            CopyOut,
            Flags,
            NULL    // No specific target type
            );

        ExprSequence.Add(CopyOutExprTree);
    }
    else
    {
        ThrowIfFalse(Input->bilop == SX_CALL || Input->bilop == SX_NEW);

        // Create a temporary variable to hold the result of the method call so that
        // the value of the call can be loaded at the end after the copy-back
        //
        // 
        ReturnValueTemp = CreateTempVariable(
            L"$VBHosted$_ByrefTemp",
            Input->ResultType,
            Input->Loc
            );

        // Back up any previous list of initiallizers.
        BackupValue<ArrayList_NorlsAllocBased<DLRExpressionTree *> *> BackupInitializersList(&m_ByRefArgumentTempInitializers);
        m_ByRefArgumentTempInitializers = &ExprSequence;

        // Back up any previous arguments list that needed special handeling.
        BackupValue<ILTree::Expression *> BackupArgumentsList(&m_ByRefWithCopyBackArguments);
        m_ByRefWithCopyBackArguments = NULL;

        if(Input->bilop == SX_CALL)
        {
            //Get the arguments from the call expression
            m_ByRefWithCopyBackArguments = Input->AsCallExpression().Right;
        }
        else if( Input->AsNewExpression().ConstructorCall != NULL )
        {
            //Get the arguments from the new expression
            m_ByRefWithCopyBackArguments = Input->AsNewExpression().ConstructorCall->AsCallExpression().Right;
        }

        DLRExpressionTree *CallOrNewExprTree = ConvertInternalToExpressionTree(
            Input,
            Flags,
            NULL    // No specific target type
            );

        DLRExpressionTree *ReturnValueAssignmentExprTree = m_ExprTreeGenerator->CreateAssignExpr(
            ReturnValueTemp,
            CallOrNewExprTree,
            NULL,   // No specific target type
            Input->Loc
            );
        
        // Append the call statement to the tree
        ExprSequence.Add(ReturnValueAssignmentExprTree);
    }
}

IReadOnlyList<DLRExpressionTree*>*
VBHostedExpressionTreeSemantics::CreateArgumentListForExpressionTree
(
    ExpressionList * ArgumentExpressions,
    ExpressionFlags Flags,
    Symbol * TargetType,
    BCSYM_Proc * TargetMethod,
    const Location &Loc,
    NorlsAllocator &Allocator
)
{
    if(ArgumentExpressions != m_ByRefWithCopyBackArguments)
    {
        //Not a special case
        return ExpressionTreeSemantics<DLRExpressionTree>::CreateArgumentListForExpressionTree(
            ArgumentExpressions,
            Flags,
            TargetType,
            TargetMethod,
            Loc,
            Allocator
            );
    }
    else
    {
        //Needs special handeling for by ref temps

        ArrayList_NorlsAllocBased<DLRExpressionTree*> *ArgumentList = NULL;

        Parameter* CurrentParameter = TargetMethod->GetFirstParam();
        while( ArgumentExpressions != NULL )
        {
            if( ArgumentExpressions->bilop != SX_LIST )
            {
                VSASSERT( false, "How did this node get here?" );
                return NULL;
            }

            Type* RealTypeOfParameter = NULL;
            ILTree::Expression* Argument = NULL;

            //Special handling rules:
            //  By ref instance field: copy instance to a temp; pass temp.[field] as arg
            //  By ref static field: no temp needed; pass field as arg
            //  By ref property: copy value to temp; pass temp as arg; use existing copy back logic
            //  All others: copy value to temp; pass temp as arg

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

            DLRExpressionTree *ExprTreeArgument = NULL;

            if(Argument->bilop == SX_ASG_RESADR)
            {
                //This is a by-ref copy back
                ExprTreeArgument = ConvertInternalToExpressionTree(
                    Argument->AsBinaryExpression().Left,
                    Flags,
                    TargetType
                    );

                m_ByRefArgumentTempInitializers->Add(ConvertInternalToExpressionTree(
                    Argument,
                    Flags,
                    TargetType
                    ));
            }
            else if(Argument->bilop == SX_ADR)
            {
                //This is a by-refed field / variable
                ILTree::SymbolReferenceExpression* var = &Argument->AsBinaryExpression().Left->AsSymbolReferenceExpression();

                if(var->BaseReference)
                {
                    //Need to store the expression the field is on to a temp to maintian expression evaluation order
                    DLRExpressionTree *BaseReferenceValueTemp = CreateTempVariable(
                        L"$VBHosted$_ByrefVarTemp",
                        var->BaseReference->ResultType,
                        Argument->Loc
                        );

                    DLRExpressionTree *BaseReferenceValue = ConvertInternalToExpressionTree(
                        var->BaseReference,
                        Flags,
                        TargetType
                        );

                    m_ByRefArgumentTempInitializers->Add(
                        m_ExprTreeGenerator->CreateAssignExpr(
                            BaseReferenceValueTemp,
                            BaseReferenceValue,
                            TargetType,
                            Argument->Loc
                            )
                        );

                    ExprTreeArgument = m_ExprTreeGenerator->CreateFieldExpr(
                        var->Symbol->PVariable(),
                        var->GenericBindingContext->PGenericTypeBinding(),
                        BaseReferenceValueTemp,
                        TargetType,
                        Argument->Loc
                        );
                }
                else
                {
                    ExprTreeArgument = ConvertInternalToExpressionTree(
                        Argument->AsBinaryExpression().Left,
                        Flags,
                        TargetType
                        );
                }
            }
            else
            {
                //This is a normal argument

                // We must check here: it could be that this method takes a structure
                // type, and the argument is SX_NOTHING. We must replace this with a new structure
                // call, if this is the case.
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

                DLRExpressionTree *ValueTemp = CreateTempVariable(
                    L"$VBHosted$_ByrefVarTemp",
                    RealTypeOfParameter,
                    Argument->Loc
                    );

                m_ByRefArgumentTempInitializers->Add(
                    m_ExprTreeGenerator->CreateAssignExpr(
                        ValueTemp,
                        ExprTreeArgument,
                        TargetType,
                        Argument->Loc
                        )
                    );

                ExprTreeArgument = ValueTemp;
            }

            if (ArgumentList == NULL)
            {
                ArgumentList = new (Allocator) ArrayList_NorlsAllocBased<DLRExpressionTree*>(&Allocator);
            }

            ArgumentList->Add(ExprTreeArgument);

            ArgumentExpressions = ArgumentExpressions->AsExpressionWithChildren().Right;
            CurrentParameter = CurrentParameter->GetNext();
        }

        return ArgumentList;
    }
}

//This is overriden for handeling the special case of Multidimensional array temp variables and Script Scope Variables
DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertSymbol
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_SYM );

    Declaration* Symbol = Input->AsSymbolReferenceExpression().pnamed;

    if(IsVariable( Symbol ))
    {
        Variable* VariableSymbol = Symbol->PVariable();
        if(VariableSymbol->IsTemporary() || VariableSymbol->IsFromScriptScope())
        {
            //Temp Var or Script Scope Var
            return ConvertVariable(VariableSymbol, TargetType, Input->Loc);
        }
    }
    //Use default implementation in all other cases
    return ExpressionTreeSemantics<DLRExpressionTree>::ConvertSymbol(Input, Flags, TargetType);
}

//This is overriden for handeling the special case of Multidimensional array assignment temp varialbe assignment
DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertAssignment
(
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

    if(Input->AsExpressionWithChildren().Left->bilop == SX_INDEX)
    {
        return ConvertArrayAssignment(
            Input, 
            Flags,
            NULL
            );
    }
    else
    {
        DLRExpressionTree *LeftExpr = ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left,
            Flags,
            NULL    // no specific target type
            );

        DLRExpressionTree *RightExpr = ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Right,
            Flags,
            NULL    // no specific target type
            );

        DLRExpressionTree *Result = m_ExprTreeGenerator->CreateAssignExpr(
            LeftExpr,
            RightExpr,
            TargetType,
            Input->Loc
            );

        return Result;
    }
}

//A helper for assigning to multidimensional arrays. 
//Although this will work for non multidimensional arrays it is not the most efficent for them. 
DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertArrayAssignment
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ASG );

    ILTree::Expression *IndexNode = Input->AsExpressionWithChildren().Left;
    ILTree::Expression *Array = IndexNode->AsExpressionWithChildren().Left;
    ILTree::Expression *Value = Input->AsExpressionWithChildren().Right;

    //Get the array
    DLRExpressionTree *ArrayExpr = ConvertInternalToExpressionTree(Array, Flags, TargetType);
    //Get the value
    DLRExpressionTree *ValueExpr = ConvertInternalToExpressionTree(Value, Flags, TargetType);

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree *> Arguments(&Scratch);
    
    //Get the Indexes
    ILTree::Expression *List = IndexNode->AsExpressionWithChildren().Right;
    while( List != NULL )
    {
        ILTree::Expression *Index;
        DLRExpressionTree *Element;

        if( List->bilop == SX_LIST )
        {
            Index = List->AsExpressionWithChildren().Left;
            List = List->AsExpressionWithChildren().Right;
        }
        else
        {
            Index = List;
            List = NULL;
        }
        
        Element = ConvertInternalToExpressionTree(Index, Flags, TargetType);

        Arguments.Add(Element);
    }
    Arguments.Add(ValueExpr);

    return m_ExprTreeGenerator->CreateCallExpr(L"Set", ArrayExpr, &Arguments, TargetType, Input->Loc);
}

//Helper for handling the singleton nature of variable expressions
DLRExpressionTree* 
VBHostedExpressionTreeSemantics::ConvertVariable
(
    Variable* VariableSymbol,
    Type* TargetType,
    const Location &Loc
)
{
    DLRExpressionTree **TreeNode = NULL;
    if((TreeNode = m_VariablesMap->HashFind(VariableSymbol)) && *TreeNode)
    {
        return *TreeNode;
    }
    else
    {
        DLRExpressionTree *NewTreeNode = m_ExprTreeGenerator->CreateVariableExpr(
            VariableSymbol->GetEmittedName(),
            VariableSymbol->GetType(),
            TargetType,
            Loc
            );

        // Add newly created varibale node to the variable list so that
        // it can be added to a block, but only if it is not a script scope
        // variable since script scope variable are not locals, but are
        // hosted provided variables.
        //
        if (!VariableSymbol->IsFromScriptScope())
        {
            m_VariablesList->Add(NewTreeNode);
        }

        // All variables need to be added to the map to ensure uniqueness.
        m_VariablesMap->HashAdd(VariableSymbol, NewTreeNode);

        return NewTreeNode;
    }
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertLambda
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull(Input);
    ThrowIfFalse(Input->bilop == SX_LAMBDA);

    // Don't allow Sub lambdas to be converted to expression trees.
    if (Input->ResultType->IsDelegate())
    {
        BCSYM_NamedRoot* proc = GetInvokeFromDelegate(Input->ResultType, m_Compiler);
        if(proc && proc->PProc() && (!proc->PProc()->GetType() || TypeHelpers::IsVoidType(proc->PProc()->GetType())))
        {
            ReportErrorForExpressionTree( ERRID_StatementLambdaInExpressionTree, Input->Loc );
            return NULL;
        }
    }

    return ExpressionTreeSemantics<DLRExpressionTree>::ConvertLambda(Input, Flags, TargetType);
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertExpressionLambda
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull(Input);
    ThrowIfFalse(Input->bilop == SX_LAMBDA);

    // Don't allow Sub lambdas to be converted to expression trees.
    if (Input->ResultType->IsDelegate())
    {
        BCSYM_NamedRoot* proc = GetInvokeFromDelegate(Input->ResultType, m_Compiler);
        if(proc && proc->PProc() && (!proc->PProc()->GetType() || TypeHelpers::IsVoidType(proc->PProc()->GetType())))
        {
            ReportErrorForExpressionTree( ERRID_StatementLambdaInExpressionTree, Input->Loc );
            return NULL;
        }
    }

    // For converting lambdas to Expression Trees, need to use a separate ET semantics implementation that ensures the following:
    //  - same restrictions as the core compiler during converting the expressions in lambdas
    //  - for expressions in lambda, generate ETs based on ET 1.0 nodes and have exactly the same shape as those
    //      produced by the core compiler
    //
    ETSemanticsForLambdas LambdaETSemantics(m_Semantics, m_ExprTreeGenerator, m_VariablesMap, m_CurrentLambdaParamsMap);

    DLRExpressionTree *Result =
        LambdaETSemantics.ConvertLambdaToExpressionTree(
            Input,
            Flags,
            NULL       // no specific target type
            );

    return Result;
}

ILTree::Expression*
VBHostedExpressionTreeSemantics::ExtractArgumentForByrefParam
(
    ILTree::Expression *Argument
)
{
    // For hosted compiler scenarios expressions are fully supported,
    // including copy back of byref parameters. Hence this method needs
    // to be override the base implementation which does not offer
    // support for copy back.

    return Argument;
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertLambdaToExpressionTree
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    // Init the m_VariablesMap hash and the m_VariablesList list for use when processing the
    // expression specified by Input.

    NorlsAllocator Scratch(NORLSLOC);

    BackupValue<VariablesMap *> BackupVariablesMap(&m_VariablesMap);
    VariablesMap VariablesMap(&Scratch);
    m_VariablesMap = &VariablesMap;

    BackupValue<ArrayList_NorlsAllocBased<DLRExpressionTree *> *> BackupVariablesList(&m_VariablesList);
    ArrayList_NorlsAllocBased<DLRExpressionTree *>  VariablesList(&Scratch);
    m_VariablesList = &VariablesList;

    // Back up the original input expr and set the new value
    BackupValue<ILTree::Expression *> BackupOrigInputExpr(&m_OriginalInputExpr);
    m_OriginalInputExpr = Input;

    // Transform the tree to remove any deferred temps introduced when
    // xml expressions are interpreted.
    ILTree::Expression *TransformedInput = TransformDeferredTemps(Input);

    // Back up the SEQ expr that identifies the deferred init expr.
    BackupValue<ILTree::Expression *> BackupSEQExpr(&m_SEQExprContainingDeferredTempInit);
    m_SEQExprContainingDeferredTempInit = NULL;

    if (TransformedInput != Input)
    {
        // Ensure that the transformed tree starts with a SEQ
        ThrowIfFalse(TransformedInput->bilop == SX_SEQ);

        // Store the transformed input so that ConvertSEQ can recognize
        // and handle this specially. Unfortunately there is no marked
        // on the bound tree itself to detect this situation.
        //
        m_SEQExprContainingDeferredTempInit = TransformedInput;
    }

    // Invoke the base implementation for the actual functionality

    DLRExpressionTree *Result = ExpressionTreeSemantics<DLRExpressionTree>::ConvertLambdaToExpressionTree(
        TransformedInput,
        Flags,
        TargetType
        );

    if (m_VariablesList->Count() > 0)
    {
        // If any variables/temporaries were created, then a scope block needs to be created
        // so that the variables can be emitted into this block.

        ArrayList_NorlsAllocBased<DLRExpressionTree *> Expressions(&Scratch);
        Expressions.Add(Result);

        Result = m_ExprTreeGenerator->CreateBlockExpr(
            m_VariablesList,
            &Expressions,
            TargetType,
            Input->Loc
            );
    }

    return Result;
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertLateBoundExpression
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_LATE || Input->bilop == SX_VARINDEX );

    DLRExpressionTree* Result = NULL;

    switch (Input->bilop)
    {
    case SX_LATE:
        Result = ConvertLateExpression(Input, Flags, TargetType);
        break;

    case SX_VARINDEX:
        Result = ConvertVarIndexExpression(Input, Flags, TargetType);
        break;
    }

    return Result;
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertVarIndexExpression
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree*> ArgumentsList(&Scratch);

    RuntimeMembers Helper = UndefinedRuntimeMember;

    //Instance
    if(Input->AsVariantIndexExpression().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsVariantIndexExpression().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNothingConstantExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetObjectType(),
                TargetType,
                Input->Loc
                )
            );
    }
    // In an odd twist, the fourth and fifth parameters are actually a paramarray that captures both the Arguments and ArgumentNames.
    // See, ExpressionSemantics.cpp:15576 for an explanation of how this expression tree is constructed.
    //Arguments
    if(Input->AsVariantIndexExpression().Right->AsExpressionWithChildren().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsVariantIndexExpression().Right->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArrayList_NorlsAllocBased<DLRExpressionTree*> ElementsList(&Scratch);
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNewArrayInitExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetObjectType(), 
                &ElementsList,
                TargetType,
                Input->Loc
                )
            );
    }
    //Argument Names
    if(Input->AsVariantIndexExpression().Right->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsVariantIndexExpression().Right->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArrayList_NorlsAllocBased<DLRExpressionTree*> ElementsList(&Scratch);
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNewArrayInitExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetStringType(), 
                &ElementsList,
                TargetType,
                Input->Loc
                )
            );
    }

    switch (SXF_VARINDEX_ENUM(Input->uFlags))
    {
    case SXE_VARINDEX_GET:
        //Public Shared Function LateIndexGet(
        //    ByVal o As Object,
        //    ByVal args() As Object,
        //    ByVal paramnames() As String) As Object
        Helper = LateIndexGetMember;
        break;

    case SXE_VARINDEX_SET:
        // This Set may be the "copy-back" of a latebound expression being passed as a byref param,
        // in which case we call a special helper which determines if the Set will
        // work (since the latebound expression may result in a ReadOnly property or a method).
        //
        if (Input->uFlags & SXF_LATE_OPTIMISTIC || Input->uFlags & SXF_LATE_RVALUE_BASE)
        {
            //Public Shared Sub LateIndexSetComplex(
            //    ByVal o As Object,
            //    ByVal args() As Object,
            //    ByVal paramnames() As String,
            //    ByVal OptimisticSet As Boolean,
            //    ByVal RValueBase As Boolean)
            Helper = LateIndexSetComplexMember;
            //Optimistic Set
            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateBooleanConstantExpr(
                    (bool) (Input->uFlags & SXF_LATE_OPTIMISTIC ? COMPLUS_TRUE : COMPLUS_FALSE),
                    TargetType,
                    Input->Loc
                    )
                );
            //RValueBase
            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateBooleanConstantExpr(
                    (bool) (Input->uFlags & SXF_LATE_RVALUE_BASE ? COMPLUS_TRUE : COMPLUS_FALSE),
                    TargetType,
                    Input->Loc
                    )
                );
        }
        else
        {
            //Public Shared Sub LateIndexSet(
            //    ByVal o As Object,
            //    ByVal args() As Object,
            //    ByVal paramnames() As String)
            Helper = LateIndexSetMember;
        }
        break;
    }

    Procedure *HelperProcedure = GetRuntimeMemberProcedure( Helper, Input->Loc );
    if ( !HelperProcedure )
    {
        return NULL;
    }

    return m_ExprTreeGenerator->CreateCallExpr(
        HelperProcedure,
        NULL,   // no generic binding context
        NULL,   // no instance
        &ArgumentsList,
        TargetType,
        Input->Loc
        );
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertLateExpression
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree*> ArgumentsList(&Scratch);

    RuntimeMembers Helper = UndefinedRuntimeMember;

    //Instance
    if(Input->AsLateBoundExpression().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsLateBoundExpression().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNothingConstantExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetObjectType(), 
                TargetType, 
                Input->Loc
                )
            );
    }
    //Type
    if(Input->AsLateBoundExpression().LateClass)
    {
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateTypeRefExpr(
                Input->AsLateBoundExpression().LateClass,
                TargetType,
                Input->Loc
                )
            );
    }
    else
    {
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNothingConstantExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetTypeType(), 
                TargetType,
                Input->Loc
                )
            );
    }
    //MemberName
    if(Input->AsLateBoundExpression().LateIdentifier)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsLateBoundExpression().LateIdentifier,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNothingConstantExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetStringType(), 
                TargetType,
                Input->Loc
                )
            );
    }
    // In an odd twist, the fourth and fifth parameters are actually a paramarray that captures both the Arguments and ArgumentNames.
    // See, ExpressionSemantics.cpp:15576 for an explanation of how this expression tree is constructed.
    //Arguments
    if(Input->AsLateBoundExpression().Right->AsExpressionWithChildren().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsLateBoundExpression().Right->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArrayList_NorlsAllocBased<DLRExpressionTree*> ElementsList(&Scratch);
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNewArrayInitExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetObjectType(), 
                &ElementsList,
                TargetType,
                Input->Loc
                )
            );
    }
    //Argument Names
    if(Input->AsLateBoundExpression().Right->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsLateBoundExpression().Right->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArrayList_NorlsAllocBased<DLRExpressionTree*> ElementsList(&Scratch);
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNewArrayInitExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetStringType(), 
                &ElementsList,
                TargetType,
                Input->Loc
                )
            );
    }
    //Type Arguments
    if(Input->AsLateBoundExpression().TypeArguments)
    {
        ArgumentsList.Add(
            ConvertInternalToExpressionTree(
                Input->AsLateBoundExpression().TypeArguments,
                Flags,
                TargetType
                )
            );
    }
    else
    {
        ArrayList_NorlsAllocBased<DLRExpressionTree*> ElementsList(&Scratch);
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateNewArrayInitExpr(
                m_CompilerHost->GetFXSymbolProvider()->GetTypeType(),
                &ElementsList,
                TargetType,
                Input->Loc
                )
            );
    }

    switch (SXF_LATE_ENUM(Input->uFlags))
    {
    case SXE_LATE_GET:
        //Public Shared Function LateGet(
        //    ByVal Instance As Object,
        //    ByVal Type As System.Type,
        //    ByVal MemberName As String,
        //    ByVal Arguments As Object(),
        //    ByVal ArgumentNames As String(),
        //    ByVal TypeArguments As Type(),
        //    ByVal CopyBack As Boolean()) As Object
        Helper = LateGetMember;
        //Copy Back
        if(Input->AsLateBoundExpression().AssignmentInfoArrayParam)
        {
            ASSERT(Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left &&
                Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left &&
                Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left,
                "[VBHostedExpressionTreeSemantics::ConvertLateExpression] Late Bound Expression has unexpected shape");

            ILTree::Expression* temp = Input->AsLateBoundExpression().AssignmentInfoArrayParam-> //SX_SEQ_OP2
                AsExpressionWithChildren().Left-> //SX_SEQ
                AsExpressionWithChildren().Left-> //SX_ASG
                AsExpressionWithChildren().Left;  //SX_SYM

            DLRExpressionTree *Var = 
                ConvertInternalToExpressionTree(
                    temp,
                    Flags,
                    TargetType
                    );

            DLRExpressionTree *Array = 
                ConvertInternalToExpressionTree(
                    (ILTree::Expression*) Input->AsLateBoundExpression().AssignmentInfoArrayParam,
                    Flags,
                    TargetType
                    );

            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateAssignExpr(
                    Var,
                    Array,
                    TargetType,
                    Input->AsLateBoundExpression().AssignmentInfoArrayParam->Loc
                    )
                );
        }
        else
        {
            ArgumentsList.Add
                (m_ExprTreeGenerator->CreateNothingConstantExpr(
                    m_SymbolCreator.GetArrayType(1, m_CompilerHost->GetFXSymbolProvider()->GetBooleanType()),
                    TargetType,
                    Input->Loc
                    )
                );
        }
        break;

    case SXE_LATE_CALL:
        //Public Shared Function LateCall(
        //    ByVal Instance As Object,
        //    ByVal Type As System.Type,
        //    ByVal MemberName As String,
        //    ByVal Arguments As Object(),
        //    ByVal ArgumentNames As String(),
        //    ByVal TypeArguments As System.Type(),
        //    ByVal CopyBack As Boolean(),
        //    ByVal IgnoreReturn As Boolean) As Object
        Helper = LateCallMember;
        //Copy Back
        if(Input->AsLateBoundExpression().AssignmentInfoArrayParam)
        {
            ASSERT(Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left &&
                Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left &&
                Input->AsLateBoundExpression().AssignmentInfoArrayParam->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left,
                "[VBHostedExpressionTreeSemantics::ConvertLateExpression] Late Bound Expression has unexpected shape");

            ILTree::Expression* temp = Input->AsLateBoundExpression().AssignmentInfoArrayParam-> //SX_SEQ_OP2
                AsExpressionWithChildren().Left-> //SX_SEQ
                AsExpressionWithChildren().Left-> //SX_ASG
                AsExpressionWithChildren().Left;  //SX_SYM

            DLRExpressionTree *Var = 
                ConvertInternalToExpressionTree(
                    temp,
                    Flags,
                    TargetType
                    );

            DLRExpressionTree *Array = 
                ConvertInternalToExpressionTree(
                    (ILTree::Expression*) Input->AsLateBoundExpression().AssignmentInfoArrayParam,
                    Flags,
                    TargetType
                    );

            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateAssignExpr(
                    Var,
                    Array,
                    TargetType,
                    Input->AsLateBoundExpression().AssignmentInfoArrayParam->Loc
                    )
                );
        }
        else
        {
            ArgumentsList.Add
                (m_ExprTreeGenerator->CreateNothingConstantExpr(
                    m_SymbolCreator.GetArrayType(1, m_CompilerHost->GetFXSymbolProvider()->GetBooleanType()),
                    TargetType,
                    Input->Loc
                    )
                );
        }
        //Ignore Return
        ArgumentsList.Add(
            m_ExprTreeGenerator->CreateBooleanConstantExpr(
                (bool) COMPLUS_TRUE,
                TargetType,
                Input->Loc
                )
            );
        break;

        //Not actually possible in an expression
    case SXE_LATE_SET:
        if (Input->uFlags & SXF_LATE_OPTIMISTIC || Input->uFlags & SXF_LATE_RVALUE_BASE)
        {
            //Public Shared Sub LateSetComplex(
            //    ByVal Instance As Object,
            //    ByVal Type As Type,
            //    ByVal MemberName As String,
            //    ByVal Arguments() As Object,
            //    ByVal ArgumentNames() As String,
            //    ByVal TypeArguments() As Type,
            //    ByVal OptimisticSet As Boolean,
            //    ByVal RValueBase As Boolean)
            Helper = LateSetComplexMember;
            //Optimistic Set
            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateBooleanConstantExpr(
                    (bool) (Input->uFlags & SXF_LATE_OPTIMISTIC ? COMPLUS_TRUE : COMPLUS_FALSE),
                    TargetType,
                    Input->Loc
                    )
                );
            //RValueBase
            ArgumentsList.Add(
                m_ExprTreeGenerator->CreateBooleanConstantExpr(
                    (bool) (Input->uFlags & SXF_LATE_RVALUE_BASE ? COMPLUS_TRUE : COMPLUS_FALSE),
                    TargetType,
                    Input->Loc
                    )
                );
        }
        else
        {
            //Public Shared Sub LateSet(
            //    ByVal Instance As Object,
            //    ByVal Type As Type,
            //    ByVal MemberName As String,
            //    ByVal Arguments() As Object,
            //    ByVal ArgumentNames() As String,
            //    ByVal TypeArguments As Type())
            Helper = LateSetMember;
        }
        break;

    default:
        break;
    }

    Procedure *HelperProcedure = GetRuntimeMemberProcedure( Helper, Input->Loc );
    if ( !HelperProcedure )
    {
        return NULL;
    }

    return m_ExprTreeGenerator->CreateCallExpr(
        HelperProcedure,
        NULL,   // no generic binding context
        NULL,   // no instance
        &ArgumentsList,
        TargetType,
        Input->Loc
        );
}

bool
VBHostedExpressionTreeSemantics::IsSeqLateCall
(
    ILTree::Expression *Input
)
{
    ILTree::Expression *Current = Input;
    while (Current && Current->bilop == SX_SEQ_OP1)
    {
        Current = Current->AsExpressionWithChildren().Left;
    }

    return (Input->bilop == SX_SEQ_OP1 &&
            Current &&
            Current->bilop == SX_LATE);
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertSeqLateExpression
(
    ILTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType
)
{
    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree *> Expressions(&Scratch);
    ArrayList_NorlsAllocBased<DLRExpressionTree *> ExpressionsBackwords(&Scratch);

    ILTree::Expression *Current = Input;
    ILTree::Expression *Expression = NULL;
    DLRExpressionTree *ReturnValueTemp = NULL;
    
    //Loop through the SEQs
    //


    while (Current)
    {
        if(Current->bilop == SX_SEQ_OP1)
        {
            Expression = Current->AsExpressionWithChildren().Right;
            Current = Current->AsExpressionWithChildren().Left;
        }
        else
        {
            Expression = Current;
            Current = NULL;
        }
        if(Expression->bilop == SX_IF)
        {
            ArrayList_NorlsAllocBased<DLRExpressionTree *> TrueExpressions(&Scratch);

            ILTree::ExpressionWithChildren IfExpression = Expression->AsExpressionWithChildren();
            DLRExpressionTree *Condition = 
                ConvertInternalToExpressionTree(
                    IfExpression.Left,
                    Flags,
                    TargetType
                    );

            DLRExpressionTree *TrueValue = 
                ConvertInternalToExpressionTree(
                    IfExpression.Right,
                    Flags,
                    TargetType
                    );
            DLRExpressionTree *NullValue = 
                m_ExprTreeGenerator->CreateNothingConstantExpr(
                    m_CompilerHost->GetFXSymbolProvider()->GetObjectType(), 
                    TargetType, 
                    IfExpression.Loc
                    );

            DLRExpressionTree *FalseValue = 
                m_ExprTreeGenerator->CreateNothingConstantExpr(
                    m_CompilerHost->GetFXSymbolProvider()->GetObjectType(), 
                    TargetType, 
                    IfExpression.Loc
                    );

            //Set up the true side of the condition to be a block that does the copy back and "returns" the same null as the false side so the types will always match
            TrueExpressions.Add(TrueValue);
            TrueExpressions.Add(NullValue);

            TrueValue = 
                m_ExprTreeGenerator->CreateBlockExpr(
                    &TrueExpressions,
                    TargetType, 
                    IfExpression.Right->Loc
                    );

            ExpressionsBackwords.Add(
                m_ExprTreeGenerator->CreateTernaryIfExpr(
                    Condition, 
                    TrueValue, 
                    FalseValue, 
                    TargetType, 
                    IfExpression.Loc
                    )
                );
        }
        else if(Expression->bilop == SX_LATE )
        {

            ASSERT(!ReturnValueTemp, "[VBHostedExpressionTreeSemantics::ConvertSeqLateExpression] there should be only one late node.");

            // Create a temporary variable to hold the result of the late node so that
            // the value of the call can be loaded at the end after the copy-back(s)
            //
            // 
            ReturnValueTemp = CreateTempVariable(
                L"$VBHosted$_LateTemp",
                Expression->ResultType,
                Expression->Loc
                );

            DLRExpressionTree *LateNode = 
                ConvertInternalToExpressionTree(
                    Expression,
                    Flags,
                    TargetType
                    );

            ExpressionsBackwords.Add(
                m_ExprTreeGenerator->CreateAssignExpr(
                    ReturnValueTemp,
                    LateNode,
                    TargetType,
                    Expression->Loc
                    )
                );

        }
        else
        {
            ASSERT(false, "[VBHostedExpressionTreeSemantics::ConvertSeqLateExpression] unexpected node type");
        }
    }

    ASSERT(ReturnValueTemp, "[VBHostedExpressionTreeSemantics::ConvertSeqLateExpression] there should have been a late node.");

    //Need to reverse the list of expressions
    for(unsigned long ind = ExpressionsBackwords.Count() - 1; Expressions.Count() < ExpressionsBackwords.Count(); ind--)
    {
        Expressions.Add(ExpressionsBackwords[ind]);
    }
    Expressions.Add(ReturnValueTemp);

    return m_ExprTreeGenerator->CreateBlockExpr(
        &Expressions,
        TargetType,
        Input->Loc
        );
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::CreateTempVariable
(
    _In_z_ WCHAR *VarName,
    Type *VarType,
    const Location &Loc
)
{
    DLRExpressionTree* ReturnValueTemp = 
        m_ExprTreeGenerator->CreateVariableExpr(
            VarName,
            VarType,
            NULL,   // No specific target type
            Loc
            );

    // Add temp to list of variables
    m_VariablesList->Add(ReturnValueTemp);
    return ReturnValueTemp;
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertNewObjectInitializer
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( IsObjectInitializer( Input ) );

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<DLRExpressionTree*> Expressions(&Scratch);

    ExpressionList* Initializers = ( ExpressionList* )Input->AsExpressionWithChildren().Right;
    
    //Get the temp that stores the new object
    DLRExpressionTree* NewObjTemp = 
        ConvertInternalToExpressionTree(
            Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left, 
            Flags, 
            NULL
            );

    //Start with the constructor
    Expressions.Add(
        ConvertAssignment(
            Input->AsExpressionWithChildren().Left,
            Flags,
            TargetType
            )
        );

    //Loop through the initializers
    while( Initializers != NULL )
    {
        if( Initializers->bilop != SX_LIST )
        {
            return( NULL );
        }

        ILTree::Expression* Initializer = ( ILTree::Expression* )Initializers->AsExpressionWithChildren().Left;
        
        Expressions.Add(
            ConvertInternalToExpressionTree(
                Initializer, 
                Flags, 
                TargetType)
            );

        Initializers = Initializers->AsExpressionWithChildren().Right;
    }

    //End with the temp
    Expressions.Add(NewObjTemp);

    return m_ExprTreeGenerator->CreateBlockExpr(
        &Expressions,
        TargetType,
        Input->Loc
        );
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertAnonymousType
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ANONYMOUSTYPE );

    ILTree::AnonymousTypeExpression* AnonymousType = &Input->AsAnonymousTypeExpression();

    NorlsAllocator Scratch(NORLSLOC);
    ArrayList_NorlsAllocBased<PropertyAssignmentInfo<DLRExpressionTree>> Initializations(&Scratch);
    
    for( ULONG i = 0; i < AnonymousType->BoundMembersCount; i += 1 )
    {
        PropertyAssignmentInfo<DLRExpressionTree> InitInfo;

        InitInfo.PropertyGetter = AnonymousType->BoundMembers[ i ].Property->GetProperty();
        InitInfo.BindingContext = AnonymousType->AnonymousTypeBinding;

        if(AnonymousType->BoundMembers[ i ].Temp)
        {
            //This init is copied to a temp
            
            //Get the temp
            DLRExpressionTree* VarExpr = ConvertInternalToExpressionTree(
                AnonymousType->BoundMembers[ i ].Temp,
                Flags,
                TargetType
                );

            //Get the value
            DLRExpressionTree* Expr = ConvertInternalToExpressionTree(
                AnonymousType->BoundMembers[ i ].BoundExpression,
                Flags,
                TargetType
                );

            //assign the value to the temp, and set the ----igment as the initial value of the property
            InitInfo.Value = m_ExprTreeGenerator->CreateAssignExpr(
                VarExpr,
                Expr,
                TargetType,
                Input->Loc
                );
        }
        else
        {
            InitInfo.Value = ConvertInternalToExpressionTree(
                AnonymousType->BoundMembers[ i ].BoundExpression,
                Flags,
                TargetType
                );
        }

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

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertWideningConversions
(
    ILTree::Expression* Input,
    ExpressionFlags Flags,
    Type* TargetType

)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_WIDE_COERCE );

    DLRExpressionTree *Result = NULL;

    if( Input->AsExpressionWithChildren().Left->bilop == SX_LAMBDA && 
        m_Semantics->IsOrInheritsFrom(
                Input->ResultType,
                GetFXSymbolProvider()->GetGenericExpressionType()
                )
        )
    {
        //This is a lambda that needs quoting

        // Only lambdas can be quoted, and they are always surrounded by SX_WIDE_COERCE.
        // The quoting happens here, and here only, for both quoting of lambda passed to
        // a method accepting Expression(Of T) as an argument, as well as direct cast
        // CType(Function () ..., Expression(Of T))

        Result = ConvertExpressionLambda(
            Input->AsExpressionWithChildren().Left,
            Flags,
            NULL   // no specific target type
            );

        //Only attempt to quote if a lambda was created.
        if(Result && m_ExprTreeGenerator->GetManagedExpressionTree(Result)->NodeType == System::Linq::Expressions::ExpressionType::Lambda)
        {
            Result = m_ExprTreeGenerator->CreateQuoteExpr(
                Result,
                NULL,       // no specific target type
                Input->Loc
                );
        }
    }
    else
    {
        //Use default implementation in all other cases
        Result = ExpressionTreeSemantics<DLRExpressionTree>::ConvertWideningConversions(Input, Flags, TargetType);
    }

    return Result;
}

ILTree::Expression*
VBHostedExpressionTreeSemantics::TransformDeferredTemps
(
    ILTree::Expression *Input
)
{
    ThrowIfNull(Input);

    if (m_Semantics->m_methodDeferredTempCount <= 0)
    {
        return Input;
    }

    m_Semantics->m_methodDeferredTempCount = 0;
    DeferredTempIterator DefTempIter(m_Semantics);
    ILTree::Expression *TransformedInput = Input;

    DefTempIter.Iterate(&TransformedInput);

    // Ensure that the transformed tree starts with a SEQ
    ThrowIfFalse(
        TransformedInput == Input ||
        TransformedInput->bilop == SX_SEQ
        );

    return TransformedInput;
}

DLRExpressionTree*
VBHostedExpressionTreeSemantics::ConvertCast
(
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

    if(Input->bilop == SX_CTYPE &&
        Input->AsBinaryExpression().Left &&
        Input->AsBinaryExpression().Left->vtype == t_ref &&
        Input->vtype == t_struct
        )
    {
        //Need special handling for converting from potentially nothing to a struct/nullable
        ILTree::Expression* fromExpr = Input->AsBinaryExpression().Left;

        //Get the value to use if the expression is null
        DLRExpressionTree* defaultExpr = NULL;
        if(TypeHelpers::IsNullableType(Input->ResultType, m_CompilerHost))
        {
            //Converting to a nullable
            if(Input->AsBinaryExpression().Left->ResultType->IsInterface())
            {
                //Maintain consistency with core compiler
                //Use a default instance even when converting to nullable when converting from interfaces.
                DLRExpressionTree* structureDefault = CreateNewStructureType(
                                                        TypeHelpers::GetElementTypeOfNullable(Input->ResultType),
                                                        Location(),
                                                        TargetType
                                                        );
                defaultExpr = m_ExprTreeGenerator->CreateCastExpr(
                                                        structureDefault,
                                                        Input->ResultType,
                                                        TargetType,
                                                        Input->Loc
                                                        );
            }
            else
            {
                //Converting to a nullable so use a nothing constant
                defaultExpr = m_ExprTreeGenerator->CreateNothingConstantExpr(
                                                    Input->ResultType,
                                                    TargetType,
                                                    Input->Loc
                                                    );
            }
        }
        else
        {
            //Converting to a value type so use a default instance
            defaultExpr = CreateNewStructureType(
                            Input->ResultType,
                            Location(),
                            TargetType
                            );
        }

        if(fromExpr->bilop == SX_NOTHING)
        {
            return defaultExpr;
        }
        else
        {
            BCSYM_Variable* tempVar = NULL;
            ILTree::SymbolReferenceExpression * varExpr = NULL;
            ILTree::ILNode* newConvertNode = NULL;
            NorlsAllocator Scratch(NORLSLOC);
            ArrayList_NorlsAllocBased<DLRExpressionTree *> Expressions(&Scratch);
            BILALLOC expressionAllocator(m_Compiler, &Scratch);

            //Create a temp
            tempVar = m_Semantics->AllocateShortLivedTemporary(fromExpr->ResultType);

            //Get the DLR expression for the temp
            DLRExpressionTree* conversionTemp = ConvertVariable(tempVar, TargetType, Location());

            //Create an expression to reference the temp
            varExpr = m_Semantics->AllocateSymbolReference(
                                    tempVar,
                                    fromExpr->ResultType,
                                    NULL, //No base reference expression
                                    Location()
                                    );

            //Copy conversion expression
            newConvertNode = expressionAllocator.xCopyBilNode(Input);

            //Make the value being converted the temp
            newConvertNode->AsBinaryExpression().Left = varExpr;

            //Create an expression for the null check
            DLRExpressionTree* conditionExpr = m_ExprTreeGenerator->CreateReferenceEqualExpr(
                conversionTemp,
                m_ExprTreeGenerator->CreateNothingConstantExpr(fromExpr->ResultType, TargetType, Input->Loc),
                TargetType,
                Input->Loc
                );

            //Defer to core compiler for actually doing the cast on the temp
            DLRExpressionTree* conversionExpr = ExpressionTreeSemantics<DLRExpressionTree>::ConvertCast(&newConvertNode->AsExpression(), Flags, TargetType);

            //Start the block by assigning the value to the temp
            Expressions.Add(
                m_ExprTreeGenerator->CreateAssignExpr(
                    conversionTemp,
                    ConvertInternalToExpressionTree(
                        fromExpr,
                        Flags,
                        TargetType
                        ),
                    TargetType,
                    fromExpr->Loc
                    )
                );

            //End the block with the if
            Expressions.Add(
                m_ExprTreeGenerator->CreateTernaryIfExpr(
                    conditionExpr,
                    defaultExpr,
                    conversionExpr,
                    TargetType,
                    Input->Loc
                    )
                );

            return m_ExprTreeGenerator->CreateBlockExpr(
                &Expressions,
                TargetType,
                Input->Loc
                );
        }
    }
    else
    {
        //Use default implementation for all other cases
        return ExpressionTreeSemantics<DLRExpressionTree>::ConvertCast(Input, Flags, TargetType);
    }
}
