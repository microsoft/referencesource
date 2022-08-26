//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Functions and datastructures for Type Inference.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

Semantics::TypeInference::TypeInference(
    Semantics*          semantics,
    Compiler*           compiler,
    CompilerHost*       compilerHost,
    Symbols*            symbolCreator,
    Procedure*          targetProcedure,
    unsigned            typeArgumentCount,
    _In_count_(typeArgumentCount) Type**              typeArguments,
    _In_count_(typeArgumentCount) bool *              typeArgumentsByAssumption,
    _In_opt_count_(typeArgumentCount) Location *      typeArgumentLocations,
    IReadonlyBitVector* fixedParameterBitVector,
    _In_count_(typeArgumentCount) Parameter**         paramsInferredFrom,
    GenericBindingInfo* genericBindingContext) :
    m_semantics(semantics),
    m_compiler(compiler),
    m_compilerHost(compilerHost),
    m_symbolCreator(symbolCreator),
    m_targetProcedure(targetProcedure),
    m_typeArgumentCount(typeArgumentCount),
    m_typeArguments(typeArguments),
    m_typeArgumentsByAssumption(typeArgumentsByAssumption),
    m_typeArgumentLocations(typeArgumentLocations),
    m_fixedParameterBitVector(fixedParameterBitVector),
    m_paramsInferredFrom(paramsInferredFrom),
    m_genericBindingContext(genericBindingContext),
    m_allocator(NORLSLOC),
    m_allocatorWrapper(NorlsAllocWrapper(&m_allocator)),
    m_inferenceGraph(this),
    m_reportErrors(true),
    m_suppressMethodNameInErrorMessages(false),
    m_candidateIsExtensionMethod(false),
    m_someInferenceFailed(false),
    m_allFailedInferenceIsDueToObject(false),
    m_typeInferenceLevel(TypeInferenceLevelNone),
    m_inferenceErrorReasons(InferenceErrorReasonsOther)
{
}

void Semantics::TypeInference::SetErrorReportingOptions(
    bool    suppressMethodNameInErrorMessages,
    bool    reportErrors,
    bool    candidateIsExtensionMethod)
{
    m_suppressMethodNameInErrorMessages = suppressMethodNameInErrorMessages;
    m_reportErrors = reportErrors;
    m_candidateIsExtensionMethod = candidateIsExtensionMethod;
}

struct Expression{};
bool
Semantics::TypeInference::InferTypeParameters
(
    Parameter*              firstParameter,
    ILTree::Expression**    boundArguments,
    ILTree::Expression*     firstParamArrayArgument,
    Type*                   delegateReturnType, // Can be Null if none.
    const Location &        callLocation,
    bool                    reportInferenceAssumptions, // Report error/warning if parameter type inferred to be object by default
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    m_allFailedInferenceIsDueToObject = true; // remains true until proven otherwise.
    return m_inferenceGraph.InferTypeParameters(firstParameter, 
        boundArguments, 
        firstParamArrayArgument, 
        delegateReturnType,
        callLocation,
        m_semantics,
        reportInferenceAssumptions,
        ppAsyncSubArgumentListAmbiguity
        );
}

bool
Semantics::TypeInference::RegisterInferredType
(
    GenericParameter*   genericParameter,
    Type*               inferredType,
    bool                inferredTypeByAssumption,
    const Location*     argumentLocation,
    Parameter*          parameter
)
{
#if DEBUG
        StringBuffer tempBuffer;

        DBG_SWITCH_PRINTF(fDumpInference, L"    - Inferred [%s] As %s%s\n",
            genericParameter->GetName(),
            GetSemantics()->ExtractErrorName(inferredType, tempBuffer),
            inferredTypeByAssumption ? L" (by assumption)" : L"");
#endif

    return RegisterInferredType(
        genericParameter,
        inferredType,
        inferredTypeByAssumption,
        argumentLocation,
        parameter,

        NULL,
        NULL,

        m_typeArguments,
        m_typeArgumentsByAssumption,
        m_typeArgumentLocations,
        m_paramsInferredFrom,
        m_symbolCreator);
}

bool
Semantics::TypeInference::RegisterInferredType
(
    GenericParameter*                 GenericParam,
    Type*                             ArgumentType,
    bool                              ArgumentTypeByAssumption,
    _In_ const Location*              ArgumentLocation,
    Parameter*                        Param,

    _Out_opt_ Type**                  InferredTypeArgument,
    _Out_opt_ GenericParameter**      InferredTypeParameter,

    _Inout_ Type**                    TypeInferenceArguments,
    _Out_opt_ bool*                   TypeInferenceArgumentsByAssumption,
    _Out_opt_ Location*               InferredTypeArgumentLocations,
    _Out_ Parameter**                 ParamsInferredFrom,
    Symbols *                         SymbolCreator
)
{
    if (ArgumentType)
    {
        ArgumentType = ArgumentType->DigThroughArrayLiteralType(SymbolCreator);
    }
   
    unsigned ParameterIndex = GenericParam->GetPosition();

    if (InferredTypeArgument)
    {
        *InferredTypeArgument = ArgumentType;
    }

    if (InferredTypeParameter)
    {
        *InferredTypeParameter = GenericParam;
    }


    if (TypeInferenceArguments[ParameterIndex] == NULL)
    {
        TypeInferenceArguments[ParameterIndex] = ArgumentType;

        if (TypeInferenceArgumentsByAssumption)
        {
            TypeInferenceArgumentsByAssumption[ParameterIndex] = ArgumentTypeByAssumption;
        }

        if (InferredTypeArgumentLocations)
        {
            InferredTypeArgumentLocations[ParameterIndex] = *ArgumentLocation;
        }

        if (ParamsInferredFrom)
        {
            ParamsInferredFrom[ParameterIndex] = Param;
        }
    }
    else if (!TypeHelpers::EquivalentTypes(TypeInferenceArguments[ParameterIndex], ArgumentType))
    {
        return false;
    }

    return true;
}

bool
Semantics::TypeInference::InferTypeArgumentsFromLambdaArgument
(
    ILTree::Expression*         argument,
    Parameter*          parameter,
    Type*               parameterType,
    _Out_opt_ GenericParameter**  typeParameter,
    _Out_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity
)
{
    return GetSemantics()->InferTypeArgumentsFromLambdaArgument(
            argument,
            parameterType,
            parameter,
            this,
            m_typeArguments,
            m_typeArgumentsByAssumption,
            m_typeArgumentLocations,
            typeParameter,
            m_typeArgumentCount,
            m_targetProcedure,
            ConstructPartialGenericTypeBinding(),
            pAsyncSubArgumentAmbiguity);
}

bool
Semantics::TypeInference::InferTypeArgumentsFromAddressOfArgument
(
    ILTree::Expression*         argument,
    Parameter*          parameter,
    Type*               parameterType,
    _Out_opt_ GenericParameter**  typeParameter
)
{
    if (TypeHelpers::IsPointerType(parameterType))
    {
        // This occurs if the parameter is ByRef. Just chase through the pointer type.
        return
            InferTypeArgumentsFromAddressOfArgument(
                argument,
                parameter,
                TypeHelpers::GetReferencedType(parameterType->PPointerType()),
                typeParameter);
    }
    else if (TypeHelpers::IsDelegateType(parameterType))
    {
        GenericBinding * PartialGenericTypeBinding = GetSymbolCreator()->GetGenericBinding(
                false,
                m_targetProcedure,
                m_typeArguments,
                m_typeArgumentCount,
                ConstructPartialGenericTypeBinding(),
                false);  // Don't worry about making copies off the stack -- this is a transient thing


        Type *DelegateType = ReplaceGenericParametersWithArguments(parameterType, PartialGenericTypeBinding, *GetSymbolCreator());
        if ( IsBad(DelegateType))
        {
            return false;
        }

        ILTree::Expression *MethodOperand = argument->AsExpressionWithChildren().Left;
        Declaration *InvokeMethod = GetInvokeFromDelegate(DelegateType, GetCompiler());
        GenericBinding *GenericBindingContextOfTarget = NULL;

        // Port SP1 CL 2929782 to VS10
        if (IsLateReference(MethodOperand))
        {
            // 152384 Calling overload resolution for late bound method will crash.
            // This will be handled by relaxed delegates properly only here we are running before
            // relaxed delegates so we have to deal with this ourselves.
            return false;
        }

        // Calling ResolveMethod modifies the methodoperand, so backup the binding context
        ILTree::Expression *BackupMethodOperand = MethodOperand;
        if (BackupMethodOperand->bilop == SX_OVERLOADED_GENERIC)
        {
            BackupMethodOperand = BackupMethodOperand->AsOverloadedGenericExpression().BaseReference;
        }

        BackupValue<GenericBinding*> OriginalBindingContext(
            BackupMethodOperand->bilop == SX_SYM ? &BackupMethodOperand->AsSymbolReferenceExpression().GenericBindingContext : NULL);

        MethodConversionClass MethodConversion = MethodConversionError;
        bool ResultIsExtensionMethod = false;

        Procedure*  TargetMethod =
            GetSemantics()->ResolveMethodForDelegateInvoke
            (
                MethodOperand,
                argument->uFlags,
                InvokeMethod->PProc(),
                DelegateType,
                parameterType, // Original delegate type for errors
                GenericBindingContextOfTarget,
                true, // SuppressMethodNameInErrorMessages,
                true, // IgnoreReturnValueErrorsForInference,
                MethodConversion,
                OvrldNoFlags,
                ResultIsExtensionMethod,
                NULL // pRequiresNarrowingConversion
            );

        if (TargetMethod != NULL)
        {
            Type* InvokeReturnType = ReplaceGenericParametersWithArguments(
                ViewAsProcedure(InvokeMethod)->GetType(),
                TypeHelpers::IsGenericTypeBinding(DelegateType) ? DelegateType->PGenericTypeBinding() : NULL,
                *GetSymbolCreator());
            Type* TargetReturnType = ReplaceGenericParametersWithArguments(TargetMethod->GetType(),GenericBindingContextOfTarget,*GetSymbolCreator());

            if (TargetReturnType == NULL || InvokeReturnType == NULL)
            {
                ReportNotFailedInferenceDueToObject();
                return true;
            }

            if (TypeHelpers::IsGenericParameter(InvokeReturnType) &&
                InvokeReturnType->PGenericParam()->GetParent() == GetTargetProcedure() &&
                InvokeReturnType == TargetReturnType)
            {
                // Return false if we didn't make any inference progress.
                return false;
            }

            // report the type of the lambda parameter to the delegate parameter.
            return InferTypeArgumentsFromArgument(
                &argument->Loc,
                TargetReturnType,
                false, // not (TargetReturnType) ByAssumption
                parameter,
                InvokeReturnType,
                MatchBaseOfGenericArgumentToParameter,
                typeParameter,
                ConversionRequired::Any);
        }

        return false;
    }
    else if (TypeHelpers::IsGenericParameter(parameterType) &&
                parameterType->PGenericParam()->GetParent() == GetTargetProcedure())
    {
        ReportNotFailedInferenceDueToObject();
        return true;
    }

    // We did not infer anything for this addressOf, AddressOf can never be of type Object, so mark inference
    // as not failed due to object.
    ReportNotFailedInferenceDueToObject();

    return true;
}

bool
Semantics::TypeInference::InferTypeArgumentsFromArgument
(
    _In_ Location*      argumentLocation,
    Type*               argumentType,
    bool                argumentTypeByAssumption,
    Parameter*          parameter,
    Type*               parameterType, // reason is that parameter might be ParamArray so we need the unexpanded one.
    MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
    _Out_opt_ GenericParameter**  inferredTypeParameter, // Needed for error reporting
    ConversionRequiredEnum InferenceRestrictions
)
{
    return ::InferTypeArgumentsFromArgument(
        argumentLocation,
        argumentType,
        argumentTypeByAssumption,
        parameterType,
        parameter,
        this,
        m_typeArguments,
        m_typeArgumentsByAssumption,
        m_typeArgumentLocations,
        m_paramsInferredFrom,
        inferredTypeParameter,
        NULL,
        m_targetProcedure,
        m_compiler,
        m_compilerHost,
        *m_symbolCreator,
        DigThroughToBasesAndImplements,
        InferenceRestrictions);
}

GenericTypeBinding*
Semantics::TypeInference::ConstructPartialGenericTypeBinding
(
)
{
    GenericTypeBinding * parentBinding = NULL;

    if(m_genericBindingContext->IsPartialBinding())
    {
        PartialGenericBinding * partialBinding = m_genericBindingContext->PPartialGenericBinding();

        AssertIfTrue(!partialBinding || partialBinding->m_pGenericBinding->IsGenericTypeBinding());

        if(partialBinding && !partialBinding->m_pGenericBinding->IsGenericTypeBinding())
        {
            parentBinding = partialBinding->m_pGenericBinding->GetParentBinding();
        }
    }
    else if(!m_genericBindingContext->IsNull())
    {
        AssertIfFalse(m_genericBindingContext->IsGenericTypeBinding());

        if(m_genericBindingContext->IsGenericTypeBinding())
        {
            parentBinding = m_genericBindingContext->PGenericTypeBinding();
        }
    }

    return parentBinding;
}

bool
Semantics::TypeInference::SomeInferenceHasFailed
(
)
{
    return m_someInferenceFailed;
}

TypeInferenceLevel
Semantics::TypeInference::GetTypeInferenceLevel
(
)
{
    return m_typeInferenceLevel;
}

bool
Semantics::TypeInference::AllFailedInferenceIsDueToObject
(
)
{
    return m_allFailedInferenceIsDueToObject;
}

inline void
Semantics::TypeInference::MarkInferenceFailure
(
)
{
    m_someInferenceFailed = true;
}

inline void
Semantics::TypeInference::MarkInferenceLevel
(
    TypeInferenceLevel  typeInferenceLevel
)
{
    if (m_typeInferenceLevel < typeInferenceLevel)
    {
        m_typeInferenceLevel = typeInferenceLevel;
    }
}

inline Procedure*
Semantics::TypeInference::GetTargetProcedure()
{
    return m_targetProcedure;
}

inline GenericBindingInfo*
Semantics::TypeInference::GetGenericBindingContext()
{
    return m_genericBindingContext;
}

Semantics*
Semantics::TypeInference::GetSemantics()
{
    ThrowIfNull(m_semantics);
    return m_semantics;
}

Compiler*
Semantics::TypeInference::GetCompiler()
{
    ThrowIfNull(m_compiler);
    return m_compiler;
}

CompilerHost*
Semantics::TypeInference::GetCompilerHost()
{
    ThrowIfNull(m_compilerHost);
    return m_compilerHost;
}

Symbols*
Semantics::TypeInference::GetSymbolCreator()
{
    ThrowIfNull(m_symbolCreator);
    return m_symbolCreator;
}

/*****************************************************************************
*
* InferenceGraph Class
*
******************************************************************************/

Semantics::InferenceGraph::InferenceGraph
(
    _In_ TypeInference*  typeInference
) :
    Graph(typeInference->GetAllocator(), typeInference->GetAllocatorWrapper()),
    m_typeInference(typeInference),
    m_VerifyingAssertions(false)
{
    Procedure* targetProcedure  = typeInference->GetTargetProcedure();

    // Allocate the array of typeParameter nodes
    m_numberOfTypeNodes = targetProcedure->GetGenericParamCount();
    m_typeNodes = reinterpret_cast<InferenceTypeNode**>(typeInference->GetAllocator()->Alloc(sizeof(InferenceTypeNode*) * m_numberOfTypeNodes));

    // Populate the array with the typeParmeters of the target method
    BCSYM_GenericParam *genericParameter = targetProcedure->GetFirstGenericParam();
    for (unsigned i = 0; i < m_numberOfTypeNodes; i++)
    {
        VSASSERT(genericParameter != NULL, "Method said it had x parameters, turns out it doesn't.");
        VSASSERT(genericParameter->GetPosition() == i, "Ordering of generic type parmeters is out of whack.");

        InferenceTypeNode* typeNode = new(*GetTypeInference()->GetAllocator()) InferenceTypeNode(this);
        typeNode->m_DeclaredTypeParam = genericParameter;

        m_typeNodes[i] = typeNode;
        genericParameter = genericParameter->GetNextParam();
    }
    VSASSERT(genericParameter == NULL, "Method said it had x parameters, turns out it doesn't.");
}

bool
Semantics::InferenceGraph::InferTypeParameters
(
    Parameter*              firstParameter,
    ILTree::Expression**    boundArguments,
    ILTree::Expression*     firstParamArrayArgument,
    Type*                   delegateReturnType, // Can be Null if none.
    const Location &        callLocation,
    Semantics*              pSemantics, // Default value: NULL
    bool                    reportInferenceAssumptions, // Report error/warning if parameter type is inferred to be object by default
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
#if DEBUG
    if (VSFSWITCH(fDumpInference))
    {
        DebPrintf("============================================================================\n");
        DebPrintf(" Start Inference of (%S)\n", GetTypeInference()->GetTargetProcedure()->GetName());
        DebPrintf("  * Building Inference Graph\n");
    }
#endif  DEBUG

    //Build a graph desribing the flow of type inference data.
    //This creates edges from "regular" arguments to type parameters and from type parameters to lambda arguments. 
    //In the rest of this function that graph is then processed (see below for more details).  Essentally, for each "type parameter" node a list of "type hints" 
    //(possible candidates for type inference) is collected. The dominant type algorithm is then performed over the list
    //of hints associated with each node.
    //
    //The process of populating the graph also seeds type hints for type parameters referenced by explicitly typed lambda parameters. in some cases. 
    //Also, hints sometimes have restrictions placed on them that limit what conversions the dominant type algorithm can consider when it processes them.
    //The restrictions are generally driven by the context in whcih type parameters are used. For example if a type parameter is used
    //as a type parameter of another type (something like IFoo(of T)), then the dominant type algorithm is not allowed to consider any conversions.
    //There are similar restrictions for Array co-varaince.
    PopulateGraph(firstParameter, boundArguments, firstParamArrayArgument, delegateReturnType, callLocation);

AlgorithmStart:

    DBG_SWITCH_PRINTF(fDumpInference, "  * Creating StronglyConnectedComponents\n");
    Graph* StronglyConnectedComponents = BuildStronglyConnectedComponents();

    DBG_SWITCH_PRINTF(fDumpInference, "  * Sorting StronglyConnectedComponents\n");

    GraphNodeList topoSortedGraph(*GetAllocatorWrapper());
    StronglyConnectedComponents->TopoSort(topoSortedGraph);

#if DEBUG
    DumpStronglyConnectedGraph(&topoSortedGraph);
#endif

    DBG_SWITCH_PRINTF(fDumpInference, "  * Inferring Graph\n");

    bool restartAlgorithm = false;
    GraphNodeListIterator iter(&topoSortedGraph);

    //We now iterate over the topologically-sorted strongly connected components of the graph, and generate type hints as appropriate.  When we find 
    //a node for an argument (or a "named node" as it's referred to in the code), we infer types for all type parameters referenced by that argument and 
    //then propigate those types as hints to the referenced type parameters. If there are incoming edges into the named node, they correspond to
    //parameters of lambda arguments that get their value from the delegate type that contains type parameters that would have been inferred during a previous iteration of the loop.
    //Those types are flowed into the lambda argument. 
    //When we encounter a "type parameter" node (or "type nodes" as they are called in the code), we run the dominant type algorithm over all of it's hints and use the resulting type 
    //as the value for the referenced type parameter.
    //
    //If we find a strongly connected component with more than one node, it means we have a cycle and cannot simply run the inference algorithm. 
    //When this happens, we look through the nodes in the cycle for a type parameter node with at least one type hint. If we find one, we remove all incoming edges to that
    //node and then continue. 
    //When we find strongly connected components with more than one node we will try to break the cyle. We do this by looking for type parameters that 
    //have hints in the strongly connected component. If we find one, we remove all incoming edges from the node, infer a type using it's hints, and then restart the whole
    //algorithm from the begining (recompute the strongly connected components, resort them, and then iterate over the graph again).
    //The source nodes of the  incoming edges we removed are added to an "assertion list". After graph traversal is done we then run inference on any "assertion nodes" we may
    //have created.
    while (iter.MoveNext())    
    {
        StronglyConnectedComponent* sccNode = (StronglyConnectedComponent*)iter.Current();
        GraphNodeList* childNodes = sccNode->GetChildNodes();
        unsigned numberOfChildNodes = childNodes->Count();
        ThrowIfFalse(numberOfChildNodes > 0);

        // Small optimization if one node:
        if (numberOfChildNodes == 1)
        {
            InferenceNode* current = (InferenceNode*)(childNodes->GetFirst()->Data());
            if (current->InferTypeAndPropagateHints(reportInferenceAssumptions, pSemantics, ppAsyncSubArgumentListAmbiguity))
            {
                //consider: Microsoft
                //              We should be asserting here, becuase this code is unreachable..
                //              There are two implementations of InferTypeAndPropagateHints,
                //              one for "named nodes" (nodes corresponding to arguments) and another
                //              for "type nodes" (nodes corresponding to types).
                //              The implementation for "named nodes" always returns false, which means 
                //              "don't restart the algorithm". The implementation for "type nodes" only returns true 
                //              if a node has incoming edges that have not been visited previously. In order for that 
                //              to happen the node must be inside a strongly connected component with more than one node 
                //              (i.e. it must be involved in a cycle). If it wasn't we would be visiting it in
                //              topological order, which means all incoming edges should have already been visited.
                //              That means that if we reach this code, there is probably a bug in the traversal process. We
                //              don't want to silently mask the bug. At a minimum we should either assert or generate a compiler error.
                //              
                //              An argument could be made that it is good to have this becuase InferTypeandPropagateHints is virtual,
                //              and should some new node type be added it's implementation may return true, and so this would follow that path.
                //              That argument does make some tiny amount of sense, and so we should keep this code here to make it easier
                //              to make any such modifications in the future. However, we still need an assert to guard against graph traversal bugs, 
                //              and in the event that such changes are made, leave it to the modifier to remove the assert if necessary.
                restartAlgorithm = true;
                break;
            }
        }
        else
        {
            bool madeInferenceProgress = false;

            GraphNodeListIterator inferenceNodeIter(childNodes);
            while (inferenceNodeIter.MoveNext())
            {
                InferenceNode* current = (InferenceNode*)inferenceNodeIter.Current();
                if (current->m_NodeType == TypedNodeType &&
                        ((InferenceTypeNode*)current)->m_inferenceTypeCollection.GetTypeDataList()->Count() > 0)
                {
                    if (current->InferTypeAndPropagateHints(false, NULL, ppAsyncSubArgumentListAmbiguity))
                    {
                        // If edges were broken, restart algorithm to recompute strongly connected components.
                        restartAlgorithm = true;
                    }
                    madeInferenceProgress = true;
                }
            }

            if (!madeInferenceProgress)
            {
                DBG_SWITCH_PRINTF(fDumpInference, "  !! Did not make progress trying to force incoming edges for nodes with TypesHints, just inferring all now, will infer object if no typehints.\n");
                GraphNodeListIterator inferenceNodeIter2(childNodes);
                while (inferenceNodeIter2.MoveNext())
                {
                    InferenceNode* current = (InferenceNode*)inferenceNodeIter2.Current();
                    if (current->m_NodeType == TypedNodeType)
                    {
                        if (current->InferTypeAndPropagateHints(false, NULL, ppAsyncSubArgumentListAmbiguity))
                        {
                            // If edges were broken, restart algorithm to recompute strongly connected components.
                            restartAlgorithm = true;
                        }
                    }
                }
            }

            if (restartAlgorithm)
            {
                break;
            }

        }

    }
    if (restartAlgorithm)
    {
        DBG_SWITCH_PRINTF(fDumpInference, "  !! Restarting Algorithm\n");

        goto AlgorithmStart;
    }


    DBG_SWITCH_PRINTF(fDumpInference, "  * Verifying assertions\n");

    m_VerifyingAssertions = true;
    GraphNodeListIterator assertionIter(&topoSortedGraph);
    while (assertionIter.MoveNext())
    {
        GraphNode* currentNode = assertionIter.Current();
        if (currentNode->m_NodeType == TypedNodeType)
        {
            InferenceTypeNode* currentTypeNode = (InferenceTypeNode*)currentNode;
            currentTypeNode->VerifyTypeAssertions();
        }
    }
    m_VerifyingAssertions = false;

    DBG_SWITCH_PRINTF(fDumpInference, " End Inference\n");
    DBG_SWITCH_PRINTF(fDumpInference, "============================================================================\n");

    return true;
}

Semantics::InferenceTypeNode*
Semantics::InferenceGraph::FindTypeNode
(
    GenericParameter* parameter
)
{
    ThrowIfNull(parameter);
    unsigned position = parameter->GetPosition();

    ThrowIfTrue(position < 0);

    Semantics::InferenceTypeNode* typeNode = NULL;
    if (position < m_numberOfTypeNodes)
    {
        typeNode = m_typeNodes[position];
    }

    if (typeNode && typeNode->m_DeclaredTypeParam == parameter)
    {
        return typeNode;
    }
    else
    {
        return NULL;
    }
}

void
Semantics::InferenceGraph::PopulateGraph
(
    Parameter*                  parameters,
    ILTree::Expression**        boundArguments,
    ILTree::Expression*         firstParamArrayArgument,
    Type*                       delegateReturnType, // Can be Null if none.
    const Location &            callLocation
)
{
    ArgumentParameterIterator iter(parameters, boundArguments, firstParamArrayArgument, false /*ignoreFirstParameter*/);
    while (iter.MoveNext())
    {
        ILTree::Expression* CurrentArgument = iter.CurrentArgument();
        InferenceNamedNode* nameNode = new(*GetTypeInference()->GetAllocator()) InferenceNamedNode(this);
        nameNode->m_Parameter = iter.CurrentParameter();
        nameNode->m_ParameterType = iter.ParameterType();
        nameNode->SetExpression(CurrentArgument);
        nameNode->SetArgumentHolder(iter.CurrentArgumentHolder());

        Type* parameterType = ReplaceGenericParametersWithArguments(
            iter.ParameterType(),
            *GetTypeInference()->GetGenericBindingContext(),
            *GetTypeInference()->GetSymbolCreator());

        switch (CurrentArgument->bilop)
        {
        case SX_UNBOUND_LAMBDA:
        case SX_LAMBDA:
            AddLambdaToGraph(parameterType, iter.CurrentParameter(), CurrentArgument, nameNode);
            break;

        case SX_ADDRESSOF:
            AddAddressOfToGraph(parameterType, iter.CurrentParameter(), CurrentArgument, nameNode);
            break;

        default:
            AddTypeToGraph(parameterType, iter.CurrentParameter(), nameNode, true /*outgoing (name->type) edge*/);
            break;
        }
    }

    if (delegateReturnType)
    {
        AddDelegateReturnTypeToGraph(delegateReturnType, callLocation);
    }
}

void
Semantics::InferenceGraph::AddDelegateReturnTypeToGraph
(
    Type*               delegateReturnType,
    const Location &    callLocation
)
{
    Procedure*  targetProc = GetTypeInference()->GetTargetProcedure();
    Type*       targetMethodType = targetProc->GetType();

    InferenceNamedNode* nameNode = new(*GetTypeInference()->GetAllocator()) InferenceNamedNode(this);
    nameNode->m_Parameter = NULL;
    nameNode->m_ParameterType = targetMethodType;
    nameNode->SetExpression(
        GetTypeInference()->GetSemantics()->AllocateExpression(
            SX_BOGUS,
            delegateReturnType,
            callLocation));


    // Add the edges from all the current generic parameters to this named node.
    GraphNodeArrayListIterator iter(GetVertices());
    while (iter.MoveNext())
    {
        InferenceNode* currentNode = (InferenceNode*)iter.Current();
        if (currentNode->m_NodeType == TypedNodeType)
        {
            AddEdge(currentNode, nameNode);

            DBG_SWITCH_PRINTF(fDumpInference, "    - Adding: [%S] -> >? \n",
                ((InferenceTypeNode*)currentNode)->m_DeclaredTypeParam->PNamedRoot()->GetName());
        }
    }


    // Add the edges from the resultType outgoing to the generic parameters.
    AddTypeToGraph(targetMethodType, NULL, nameNode, true /*outgoing (name->type) edge*/);
}

void
Semantics::InferenceGraph::AddLambdaToGraph
(
    Type*               parameterType,
    Parameter*          parameter,
    ILTree::Expression*         lambdaExpression,
    _Out_ InferenceNamedNode* nameNode
)
{
    VSASSERT(
        lambdaExpression->bilop == SX_UNBOUND_LAMBDA ||
        lambdaExpression->bilop == SX_LAMBDA,
        "");

    if (TypeHelpers::IsGenericParameter(parameterType))
    {
        // Lambda is bound to a generic typeParam, just infer anonymous delegate
        AddTypeToGraph(parameterType, parameter, nameNode, true /*outgoing (name->type) edge*/);
    }
    else if (TypeHelpers::IsGenericTypeBinding(parameterType) && parameterType->IsDelegate())
    {
        Symbols * symbols = GetTypeInference()->GetSymbolCreator();
        Procedure *invokeMethod = GetInvokeFromDelegate(parameterType, GetTypeInference()->GetCompiler())->PProc();
        GenericTypeBinding* delegateBindingContext = parameterType->PGenericTypeBinding();

        Parameter* firstLambdaParameter;
        if (lambdaExpression->bilop == SX_LAMBDA)
        {
            firstLambdaParameter = lambdaExpression->AsLambdaExpression().FirstParameter;
        }
        else if (lambdaExpression->bilop == SX_UNBOUND_LAMBDA)
        {
            firstLambdaParameter = lambdaExpression->AsUnboundLambdaExpression().FirstParameter;
        }

        Parameter* delegateParameter;
        Parameter* lambdaParameter;
        for (delegateParameter = invokeMethod->GetFirstParam(),
             lambdaParameter = firstLambdaParameter;

             delegateParameter && lambdaParameter;

             delegateParameter = delegateParameter->GetNext(),
             lambdaParameter = lambdaParameter->GetNext())
        {
            Type * delegateParameterType = ReplaceGenericParametersWithArguments(
                GetDataType(delegateParameter),
                delegateBindingContext,
                *symbols);

            Type * lambdaParameterType = GetDataType(lambdaParameter);
            if (lambdaParameterType != NULL)
            {
                GenericParameter* typeParameter = NULL;
                // Prepopulate the hint from the lambda's parameter.
                bool inferenceOk = GetTypeInference()->InferTypeArgumentsFromArgument(
                    lambdaParameter->GetLocation(),
                    lambdaParameterType,
                    false, // not (lambdaParameterType) By----ymption
                    delegateParameter,
                    delegateParameterType,
                    MatchBaseOfGenericArgumentToParameter,
                    &typeParameter,
                    ConversionRequired::Any);
                // no need to report inference errors here because we want a type mismatch error.
            }

            if (delegateParameterType)
            {
                AddTypeToGraph(delegateParameterType, parameter, nameNode, false /*incoming (type->name) edge*/);
            }
            else
            {
               //consider: Microsoft
                //              This code is not necessary.
                //              When delegateParameterType is null this method should have no effect,
                //              because RefersToGenericParameter(NULL) will return false and AddTypeToGraph only
                //              adds edges if the parameter type references at least one generic parameter.
                //              As a result we should consider removing it.                
                AddTypeToGraph(delegateParameterType, parameter, nameNode, true /*incoming (type->name) edge*/);
            }
        }

        Type * delegateReturnType = ReplaceGenericParametersWithArguments(
            invokeMethod->GetType(),
            delegateBindingContext,
            *symbols);
        AddTypeToGraph(delegateReturnType, parameter, nameNode, true /*outgoing (name->type) edge*/);
    }
    else if (TypeHelpers::IsGenericTypeBinding(parameterType) &&
            GetTypeInference()->GetSemantics()->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericExpressionType) &&
            TypeHelpers::EquivalentTypes(
                parameterType->PGenericTypeBinding()->GetGenericType(),
                GetTypeInference()->GetSemantics()->GetFXSymbolProvider()->GetGenericExpressionType()))
    {
        // If we've got an Expression(Of T), skip through to T
        AddLambdaToGraph(
            parameterType->PGenericTypeBinding()->GetArgument(0),
            parameter,
            lambdaExpression,
            nameNode);
    }
    else if (TypeHelpers::IsPointerType(parameterType))
    {
        AddLambdaToGraph(
            TypeHelpers::GetReferencedType(parameterType->PPointerType())    ,
            parameter,
            lambdaExpression,
            nameNode);
    }
    else
    {
        // no type inference needed for this lambda because its arguments aren't generic.
    }
}

void
Semantics::InferenceGraph::AddAddressOfToGraph
(
    Type*               parameterType,
    Parameter*          parameter,
    ILTree::Expression*         addressOfExpression,
    _Out_ InferenceNamedNode* nameNode
)
{
    VSASSERT(addressOfExpression->bilop == SX_ADDRESSOF, "");

    if (TypeHelpers::IsGenericParameter(parameterType))
    {
        // Lambda is bound to a generic typeParam, just infer anonymous delegate
        AddTypeToGraph(parameterType, parameter, nameNode, true /*outgoing (name->type) edge*/);
    }
    else if (TypeHelpers::IsGenericTypeBinding(parameterType) && parameterType->IsDelegate())
    {
        Symbols * symbols = GetTypeInference()->GetSymbolCreator();
        Procedure *invokeMethod = GetInvokeFromDelegate(parameterType, GetTypeInference()->GetCompiler())->PProc();
        GenericTypeBinding* delegateBindingContext = parameterType->PGenericTypeBinding();

        Type * delegateReturnType = ReplaceGenericParametersWithArguments(
            invokeMethod->GetType(),
            delegateBindingContext,
            *symbols);
        AddTypeToGraph(delegateReturnType, parameter, nameNode, true /*outgoing (name->type) edge*/);


        Parameter* delegateParameter;
        for (delegateParameter = invokeMethod->GetFirstParam();
             delegateParameter;
             delegateParameter = delegateParameter->GetNext())
        {
            Type * delegateParameterType = ReplaceGenericParametersWithArguments(
                GetDataType(delegateParameter),
                delegateBindingContext,
                *symbols);

            if (delegateParameterType)
            {
                AddTypeToGraph(delegateParameterType, parameter, nameNode, false /*incoming (type->name) edge*/);
            }
            else
            {
                //consider: Microsoft
                //              This code is not necessary.
                //              When delegateParameterType is null this method should have no effect,
                //              because RefersToGenericParameter(NULL) will return false and AddTypeToGraph only
                //              adds edges if the parameter type references at least one generic parameter.
                //              As a result we should consider removing it.
                AddTypeToGraph(delegateParameterType, parameter, nameNode, true /*incoming (type->name) edge*/);
            }
        }

     }
    else if (TypeHelpers::IsGenericTypeBinding(parameterType) &&
            GetTypeInference()->GetSemantics()->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericExpressionType) &&
            TypeHelpers::EquivalentTypes(
                parameterType->PGenericTypeBinding()->GetGenericType(),
                GetTypeInference()->GetSemantics()->GetFXSymbolProvider()->GetGenericExpressionType()))
    {
        // If we've got an Expression(Of T), skip through to T
        AddAddressOfToGraph(
            parameterType->PGenericTypeBinding()->GetArgument(0),
            parameter,
            addressOfExpression,
            nameNode);
    }
    else if (TypeHelpers::IsPointerType(parameterType))
    {
        AddAddressOfToGraph(
            TypeHelpers::GetReferencedType(parameterType->PPointerType())    ,
            parameter,
            addressOfExpression,
            nameNode);
    }
    else
    {
        // no type inference needed for this lambda because its arguments aren't generic.
    }
}

void
Semantics::InferenceGraph::AddTypeToGraph
(
    Type*               parameterType,
    Parameter*          parameter,
    _Out_ InferenceNamedNode* nameNode,
    bool                isOutgoingEdge
)
{
    unsigned NumberOfGenericParameters = 0;
    BCSYM_GenericParam** GenericParameters = NULL;

    if (RefersToGenericParameter(
            parameterType,
            GetTypeInference()->GetAllocator(),
            GenericParameters,
            &NumberOfGenericParameters))
    {
        for (unsigned i = 0; i < NumberOfGenericParameters; i++)
        {
            BCSYM_GenericParam* genericParameter = GenericParameters[i];
            InferenceTypeNode* typeNode = FindTypeNode(genericParameter);
            if (typeNode != NULL)
            {
                if (typeNode->m_Parameter == NULL)
                {
                    typeNode->m_Parameter = parameter;
                }

                if (isOutgoingEdge)
                {
                    AddEdge(nameNode, typeNode);
                }
                else
                {
                    AddEdge(typeNode, nameNode);
                }


                DBG_SWITCH_PRINTF(fDumpInference, "    - Adding: [%S] %s %S \n",
                    nameNode->m_Parameter ? nameNode->m_Parameter->GetName() : L"?",
                    isOutgoingEdge ? "->" : "<-",
                    genericParameter->GetName());
            }
        }
    }
}

inline Semantics::TypeInference*
Semantics::InferenceGraph::GetTypeInference()
{
    return m_typeInference;
}

inline bool
Semantics::InferenceGraph::IsVerifyingAssertions()
{
    return m_VerifyingAssertions;
}

void
Semantics::InferenceGraph::RegisterTypeParameterHint
(
    GenericParameter*                   genericParameter,
    Type*                               inferredType,
    bool                                inferredTypeByAssumption,
    const Location*                     argumentLocation,
    Parameter*                          parameter,
    bool                                inferredFromObject,
    ConversionRequiredEnum              inferenceRestrictions)
{
#if DEBUG
    StringBuffer infBuf, infBuf2, genBuf, genBuf2;
    Semantics *s = GetTypeInference()->GetSemantics();
    // nb. it causes memory corruption if we save the result of "const STRING *t = s->ExtractErrorName(...)",
    // which is why we inline them all.

    switch (inferenceRestrictions)
    {
        case ConversionRequired::Identity:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s =}, i.e. %s should be identical to %s according to CLR\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(inferredType, infBuf2), s->ExtractErrorName(genericParameter, genBuf2));
            break;
        case ConversionRequired::None:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s none}, i.e. no requirements on %s (but try %s as a suggested candidate)\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(genericParameter, genBuf2), s->ExtractErrorName(inferredType, infBuf2));
            break;
        case ConversionRequired::Any:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s +any}, i.e. %s should convert to %s through any VB conversion\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(inferredType, infBuf2), s->ExtractErrorName(genericParameter, genBuf2));
            break;
        case ConversionRequired::AnyReverse:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s -any}, i.e. %s should convert to %s through any VB conversion\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(genericParameter, genBuf2), s->ExtractErrorName(inferredType, infBuf2));
            break;
        case ConversionRequired::AnyAndReverse:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s +-any}, i.e. %s and %s should mutually convert for ByRef\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(inferredType, infBuf2), s->ExtractErrorName(genericParameter, genBuf2));
            break;
        case ConversionRequired::ArrayElement:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s +arr}, i.e. %s should convert to %s through array element conversions\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(inferredType, infBuf2), s->ExtractErrorName(genericParameter, genBuf2));
            break;
        case ConversionRequired::Reference:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s +r}, i.e. %s should convert to %s through reference conversions\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(inferredType, infBuf2), s->ExtractErrorName(genericParameter, genBuf2));
            break;
        case ConversionRequired::ReverseReference:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - Adding hint %s:{%s -r}, i.e. %s should convert to %s through reference conversions\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf),  s->ExtractErrorName(genericParameter, genBuf2), s->ExtractErrorName(inferredType, infBuf2));
            break;
        default:
            DBG_SWITCH_PRINTF(fDumpInference, L"    - UNKNOWN HINT TYPE %s:{%s ???}\n",
                              s->ExtractErrorName(genericParameter, genBuf), s->ExtractErrorName(inferredType, infBuf));
    }
    if (inferredTypeByAssumption)
    {
        DBG_SWITCH_PRINTF(fDumpInference, L"      ... that hint was By Assumption\n");
    }
#endif


    InferenceTypeNode* typeNode = m_typeNodes[genericParameter->GetPosition()];
    typeNode->AddTypeHint(inferredType, inferredTypeByAssumption, argumentLocation, parameter, inferredFromObject, inferenceRestrictions);
}

/*****************************************************************************
*
* InferenceNode Class
*
******************************************************************************/

Semantics::InferenceNode::InferenceNode(_In_ Graph *graph) :
    GraphNode(graph),
    m_InferenceComplete(false),
    m_Parameter(NULL)
{
}

inline Semantics::InferenceGraph*
Semantics::InferenceNode::GetInferenceGraph()
{
    return (InferenceGraph*)m_Graph;
}

inline Semantics::TypeInference*
Semantics::InferenceNode::GetTypeInference()
{
    return GetInferenceGraph()->GetTypeInference();
}

/*****************************************************************************
*
* InferenceTypeNode Class
*
******************************************************************************/
Semantics::InferenceTypeNode::InferenceTypeNode(_In_ InferenceGraph *graph) :
    InferenceNode(graph),
    m_TypeAssertions(*graph->GetAllocatorWrapper()),
    m_InferredType(NULL),
    m_inferenceTypeCollection(graph->GetTypeInference()->GetSemantics(), graph->GetAllocator(), graph->GetAllocatorWrapper())
{
    m_NodeType = TypedNodeType;
}

bool
Semantics::InferenceTypeNode::InferTypeAndPropagateHints
(
    bool reportInferenceAssumptions, // Default value: false
    Semantics *pSemantics, // Default value: NULL
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    int             numberOfIncomingEdges = GetIncomingEdges().Count();
    bool            restartAlgorithm = false;
    Location        argumentLocation = {0};
    int             numberOfIncomingWithNothing = 0;

#if DEBUG
    StringBuffer tempBuffer;

    DBG_SWITCH_PRINTF(fDumpInference, "   . Processing: %S\n",
        GetTypeInference()->GetSemantics()->ExtractErrorName(m_DeclaredTypeParam, tempBuffer))
#endif

    if (numberOfIncomingEdges > 0)
    {
        argumentLocation = ((InferenceNamedNode*)GetIncomingEdges()[0])->GetExpression()->Loc;

    }
    else
    {
        argumentLocation.SetLocationToHidden();
    }

    int numberOfAssertions = 0;
    bool IncomingFromObject = false;
    GraphNodeArrayListIterator iter(&GetIncomingEdges());
    while (iter.MoveNext())
    {
        GraphNode* currentGraphNode = iter.Current();
        VSASSERT(currentGraphNode->m_NodeType == NamedNodeType, "Should only have named nodes as incoming edges.");
        InferenceNamedNode* currentNamedNode = (InferenceNamedNode*)currentGraphNode;

        if ( currentNamedNode->GetExpression()->ResultType != NULL && TypeHelpers::IsRootObjectType(currentNamedNode->GetExpression()->ResultType))
        {
            IncomingFromObject = true;
        }

        if (!currentNamedNode->m_InferenceComplete)
        {
            // Remember the incomplete incoming edges as unfinished by
            // adding them to assertions list.
            // Then remove them from the incoming edges.
            m_TypeAssertions.Add(currentNamedNode);
            GetInferenceGraph()->RemoveEdge(currentNamedNode, this);
            restartAlgorithm = true;
            numberOfAssertions++;

#if DEBUG
            StringBuffer tempBuffer1;

            DBG_SWITCH_PRINTF(fDumpInference, "  !! Found Assertion, removing edge: [%S] -> %S\n",
                    currentNamedNode->m_Parameter ? currentNamedNode->m_Parameter->GetName() : L"?",
                    GetTypeInference()->GetSemantics()->ExtractErrorName(m_DeclaredTypeParam, tempBuffer1));
#endif
        }
        else
        {
            ILTree::Expression *inputExpression = currentNamedNode->GetExpression();
            // We should not infer from a Nothing literal because according
            // in reality Nothing is not object but is "Any" type.
            if (IsNothingLiteral(inputExpression) && !inputExpression->IsExplicitlyCast)
            {
                numberOfIncomingWithNothing++;
            }
        }
    }

    if (numberOfIncomingEdges > 0 && numberOfIncomingEdges == numberOfIncomingWithNothing)
    {
        DBG_SWITCH_PRINTF(fDumpInference, "  !! Inference has failed: All incoming type hints, were based on 'Nothing' \n");
        GetTypeInference()->MarkInferenceFailure();
        GetTypeInference()->ReportNotFailedInferenceDueToObject();
    }

    unsigned numberOfTypeHints = m_inferenceTypeCollection.GetTypeDataList()->Count();

    if (numberOfTypeHints == 0)
    {
        if (numberOfAssertions == numberOfIncomingEdges)
        {
            GetTypeInference()->MarkInferenceLevel(TypeInferenceLevelOrcas);

#if DEBUG
            StringBuffer tempBuffer2;

            DBG_SWITCH_PRINTF(fDumpInference, "  !! All incoming edges translated to assertions, skipping Inference for this parameter %S\n",
                GetTypeInference()->GetSemantics()->ExtractErrorName(m_DeclaredTypeParam, tempBuffer2));
#endif
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpInference, "  !! Inference has failed. No Type hints, and some, not all were assertions, otherwise we would have picked object for strict.\n");
            m_InferredType = NULL;
            GetTypeInference()->MarkInferenceFailure();
            if (!IncomingFromObject)
            {
                GetTypeInference()->ReportNotFailedInferenceDueToObject();
            }
        }
    }
    else if (numberOfTypeHints == 1)
    {
        DominantTypeDataTypeInference* typeData = (DominantTypeDataTypeInference*)(*m_inferenceTypeCollection.GetTypeDataList())[0];
        m_InferredType = typeData->ResultType;
        m_InferredTypeByAssumption = typeData->ByAssumption;
        if ((!argumentLocation.IsValid() || argumentLocation.IsHidden()) &&
            typeData->ArgumentLocation != NULL &&
            typeData->ArgumentLocation->IsValid())
        {
            argumentLocation = *typeData->ArgumentLocation;
        }
    }
    else
    {
        // Run the whidbey algorith to see if we are smarter now.
        Type* firstInferredType = NULL;
        DominantTypeDataList* allTypeData = m_inferenceTypeCollection.GetTypeDataList();
        DominantTypeDataListIterator dominantIter(allTypeData);
        while (dominantIter.MoveNext())
        {
            DominantTypeData* currentTypeInfo = dominantIter.Current();
            if (firstInferredType == NULL)
            {
                firstInferredType = currentTypeInfo->ResultType;
            }
            else if (!TypeHelpers::EquivalentTypes(firstInferredType, currentTypeInfo->ResultType))
            {
                // Whidbey failed hard here, in orca's we added dominant type information.
                GetTypeInference()->MarkInferenceLevel(TypeInferenceLevelOrcas);
            }
        }

        DominantTypeDataList dominantTypeDataList(*GetTypeInference()->GetAllocatorWrapper());
        DominantTypeDataList newDominantTypeDataList(*GetTypeInference()->GetAllocatorWrapper());
        InferenceErrorReasons errorReasons = InferenceErrorReasonsOther;

        m_inferenceTypeCollection.FindDominantType(dominantTypeDataList, &errorReasons);

        if (dominantTypeDataList.Count() == 1)
        {
            //consider: Microsoft
            //              This seems dangerous to me, that we 
            //              remove error reasons here.
            //              Instead of clearing these, what we should be doing is 
            //              asserting that they are not set.
            //              If for some reason they get set, but 
            //              we enter this path, then we have a bug.
            //              This code is just masking any such bugs.
            errorReasons &= ~InferenceErrorReasonsAmbiguous;
            errorReasons &= ~InferenceErrorReasonsNoBest;

            m_InferredType = dominantTypeDataList[0]->ResultType;
            m_InferredTypeByAssumption = ((DominantTypeDataTypeInference*)dominantTypeDataList[0])->ByAssumption;
            argumentLocation = *((DominantTypeDataTypeInference*)dominantTypeDataList[0])->ArgumentLocation;
            // Also update the location of the argument for constraint error reporting later on.
        }
        else
        {
            if (errorReasons & InferenceErrorReasonsAmbiguous)
            {
                DBG_SWITCH_PRINTF(fDumpInference, "  !! Inference has failed. Dominant type algorithm found ambiguous types.\n");
                GetTypeInference()->ReportAmbiguousInferenceError(dominantTypeDataList);
            }
            else
            {
                //consider: Microsoft
                //              This code appears to be operating under the assumption that if the error reason is not due to an 
                //              ambiguity then it must be because there was no best match.
                //              We should be asserting here to verify that assertion.
                DBG_SWITCH_PRINTF(fDumpInference, "  !! Inference has failed. Dominant type algorithm could not find a dominant type.\n");
                GetTypeInference()->ReportIncompatibleInferenceError(allTypeData, &m_inferenceTypeCollection);
            }
            m_InferredType = (*allTypeData)[0]->ResultType;
            GetTypeInference()->MarkInferenceFailure();
        }

        GetTypeInference()->RegisterErrorReasons(errorReasons);
    }

    if (m_InferredType!=NULL)
    {
        //consider: Microsoft
        //              Why do we set m_InferenceComplete here, when it just 
        //              gets overriden after the "if" statement?
        m_InferenceComplete = GetTypeInference()->RegisterInferredType(
            m_DeclaredTypeParam,
            m_InferredType,
            m_InferredTypeByAssumption,
            &argumentLocation,
            m_Parameter);
    }

    m_InferenceComplete = true;

    return restartAlgorithm;
}

inline void
Semantics::TypeInference::ReportNotFailedInferenceDueToObject
(
)
{
    m_allFailedInferenceIsDueToObject = false;
}

void
Semantics::TypeInference::ReportIncompatibleInferenceError
(
    DominantTypeDataList *typeInfos,
    TypeInferenceCollection* typeCollection
)
{
    if (typeInfos->Count() < 1)
    {
        return;
    }

    DominantTypeDataTypeInference* originalTypeInfo = (DominantTypeDataTypeInference*)(*typeInfos)[0];

    // Since they get added fifo, we need to walk the list backward.
    for (unsigned i = 1; i < typeInfos->Count(); i++)
    {
        DominantTypeDataTypeInference* currentTypeInfo = (DominantTypeDataTypeInference*)(*typeInfos)[i];

        if (!currentTypeInfo->InferredFromObject)
        {
            ReportNotFailedInferenceDueToObject();
        }
    }
}

void
Semantics::TypeInference::RegisterErrorReasons
(
    InferenceErrorReasons inferenceErrorReasons
)
{
    m_inferenceErrorReasons = m_inferenceErrorReasons | inferenceErrorReasons;
}

InferenceErrorReasons
Semantics::TypeInference::GetErrorReasons
(
)
{
    return m_inferenceErrorReasons;
}

void
Semantics::TypeInference::ReportAmbiguousInferenceError
(
    DominantTypeDataList &typeInfos
)
{
    VSASSERT(typeInfos.Count() >= 2, "Must have at least 2 elements in the list");
    ThrowIfFalse(typeInfos.Count() >= 2);

    DominantTypeDataTypeInference* originalTypeInfo = (DominantTypeDataTypeInference*)typeInfos[0];

    // Since they get added fifo, we need to walk the list backward.
    for (unsigned i = 1; i < typeInfos.Count(); i++)
    {
        DominantTypeDataTypeInference* currentTypeInfo = (DominantTypeDataTypeInference*)typeInfos[i];

        if (!currentTypeInfo->InferredFromObject)
        {
            ReportNotFailedInferenceDueToObject();
        }
    }
}

void
Semantics::InferenceTypeNode::VerifyTypeAssertions()
{
#if DEBUG
    VerifyIncomingInferenceComplete(NamedNodeType);
#endif

    Semantics* semantics = GetTypeInference()->GetSemantics();

    GraphNodeArrayListIterator assertionIter(&m_TypeAssertions);
    while (assertionIter.MoveNext())
    {
        Procedure*      operatorMethod = NULL;
        GenericBinding* operatorMethodGenericContext = NULL;
        GraphNode*      assertedNode = assertionIter.Current();
        if (assertedNode->m_NodeType == NamedNodeType)
        {
            //consider: Microsoft
            //              We should look into removing this assert.
            //              I searched through the entire compiler code base, and we do not 
            //              check this flag anywhere, so I'm not sure that the point of asserting here is.
            VSASSERT(GetInferenceGraph()->IsVerifyingAssertions(), "InferTypeAndPropHints will check this flag and go in error reporting mode");

            InferenceNamedNode* nameNode = (InferenceNamedNode*)assertedNode;

            DBG_SWITCH_PRINTF(fDumpInference, "    - Verifying %S\n",
                nameNode->m_Parameter ? nameNode->m_Parameter->GetName() : L"?");

            nameNode->InferTypeAndPropagateHints();
        }
    }
}

void
Semantics::InferenceTypeNode::AddTypeHint
(
    Type*                               type,
    bool                                typeByAssumption,
    const Location*                     argumentLocation,
    Parameter*                          parameter,
    bool                                inferredFromObject,
    ConversionRequiredEnum              inferenceRestrictions
)
{
    VSASSERT(!typeByAssumption || type->IsObject() || type->IsArrayLiteralType(), "unexpected: a type which was 'by assumption', but isn't object or array literal");

    DominantTypeDataTypeInference* typeData = new(*(GetTypeInference()->GetAllocator())) DominantTypeDataTypeInference(GetTypeInference()->GetAllocatorWrapper());

    typeData->ResultType = type->ChaseThroughPointerTypes();
    typeData->ByAssumption = typeByAssumption;
    typeData->InferenceRestrictions = inferenceRestrictions;

    typeData->ArgumentLocation = argumentLocation;
    typeData->Parameter = parameter;
    typeData->InferredFromObject = inferredFromObject;
    typeData->TypeParameter = m_DeclaredTypeParam;

    bool foundInList = false;
    DominantTypeDataListIterator iterForDuplicates(m_inferenceTypeCollection.GetTypeDataList());
    while (iterForDuplicates.MoveNext())
    {
        DominantTypeDataTypeInference* competitor = (DominantTypeDataTypeInference*)iterForDuplicates.Current();
        if (BCSYM::AreTypesEqual(typeData->ResultType, competitor->ResultType))
        {
            competitor->InferenceRestrictions = CombineConversionRequirements(
                                                    competitor->InferenceRestrictions,
                                                    typeData->InferenceRestrictions);
            competitor->ByAssumption &= typeData->ByAssumption;

            VSASSERT(!foundInList, "List is supposed to be unique: how can we already find two of the same type in this list.");
            foundInList = true;
        }
    }
    if (!foundInList)
    {
        m_inferenceTypeCollection.GetTypeDataList()->Add(typeData);
    }
}

/*****************************************************************************
*
* InferenceNameNode Class
*
******************************************************************************/

Semantics::InferenceNamedNode::InferenceNamedNode(_In_ InferenceGraph *graph) :
    InferenceNode(graph),
    m_expression(NULL),
    m_argumentHolder(NULL)
{
    m_NodeType = NamedNodeType;
}

#if DEBUG
void
Semantics::InferenceNode::VerifyIncomingInferenceComplete
(
    unsigned nodeType
)
{
    if (!GetTypeInference()->SomeInferenceHasFailed())
    {
        GraphNodeArrayListIterator incomingIter(&GetIncomingEdges());
        while (incomingIter.MoveNext())
        {
            GraphNode* current = incomingIter.Current();
            VSASSERT(current->m_NodeType == nodeType, "Should only have expected incoming edges.");
            VSASSERT( ((InferenceNode*)current)->m_InferenceComplete, "Should have inferred type already");
        }
    }
}
#endif

bool
Semantics::InferenceNamedNode::InferTypeAndPropagateHints
(
    bool reportInferenceAssumptions, // Report error if Param is inferred to be Object by default
    Semantics *pSemantics, // Default value: NULL
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    TypeInference* typeInference = GetTypeInference();

#if DEBUG
    VerifyIncomingInferenceComplete(TypedNodeType);

    StringBuffer tempBuffer;

    DBG_SWITCH_PRINTF(fDumpInference, "   . Processing: [%S]\n",
        m_Parameter != NULL ? m_Parameter->GetName() : L"?");
#endif


    // Check if all incoming are ok, otherwise skip inference.
    GraphNodeArrayListIterator iter(&GetIncomingEdges());
    while (iter.MoveNext())
    {
        GraphNode* currentGraphNode = iter.Current();
        VSASSERT(currentGraphNode->m_NodeType == TypedNodeType, "Should only have typed nodes as incoming edges.");
        InferenceTypeNode* currentTypedNode = (InferenceTypeNode*)currentGraphNode;
        if (!currentTypedNode->m_InferredType)
        {
            bool SkipThisNode = false;
            Type* ParameterTypeForLambdaObjectInference = m_ParameterType->ChaseThroughPointerTypes();
            if (m_expression->bilop == SX_UNBOUND_LAMBDA  && TypeHelpers::IsDelegateType(ParameterTypeForLambdaObjectInference))
            {
                // Check here if we need to infer Object for some of the parameters of the Lambda if we weren't able.
                // to infer these otherwise. This is only the case for arguments of the lambda that have a GenericParam
                // of the method we are inferring that is not yet inferred.

                // Now find the invoke method of the delegate
                Declaration *InvokeMethod = GetInvokeFromDelegate(ParameterTypeForLambdaObjectInference, GetTypeInference()->GetCompiler());

                if (InvokeMethod != NULL && !IsBad(InvokeMethod) && InvokeMethod->IsProc())
                {
                    Parameter* firstLambdaParameter = m_expression->AsUnboundLambdaExpression().FirstParameter;
                    Parameter* lambdaParameter;
                    Parameter* delegateParam;
                    for (delegateParam = ViewAsProcedure(InvokeMethod)->GetFirstParam(),
                         lambdaParameter = firstLambdaParameter;

                         delegateParam && lambdaParameter;

                         delegateParam = delegateParam->GetNext(),
                         lambdaParameter = lambdaParameter->GetNext())
                    {
                        Type *ParamType =
                            ReplaceGenericParametersWithArguments(
                                delegateParam->GetType(),
                                ParameterTypeForLambdaObjectInference->PGenericBinding(),
                                *GetTypeInference()->GetSymbolCreator());

                        if (lambdaParameter->GetType() == NULL &&
                            BCSYM::AreTypesEqual(ParamType, currentTypedNode->m_DeclaredTypeParam))
                        {
                            // If this was an argument to the unbound Lambda, infer Object.
                            currentTypedNode->m_InferredType = GetTypeInference()->GetSemantics()->GetFXSymbolProvider()->GetObjectType();
                            GetTypeInference()->RegisterInferredType(
                                currentTypedNode->m_DeclaredTypeParam,
                                currentTypedNode->m_InferredType,
                                currentTypedNode->m_InferredTypeByAssumption,
                                lambdaParameter->GetLocation(),
                                currentTypedNode->m_Parameter);

                            // 
                            // Port SP1 CL 2941063 to VS10
                            // Bug 153317
                            // If reportInferenceAssumptions is true then report an error if Option Strict On
                            // or a warning if Option Strict Off because we have no hints about the lambda parameter
                            // and we are assuming that it is an object. This flag was threaded down from 
                            // Semantics::MatchArguments. The flag is necessary because m_ReportErrors is false
                            // when we want to report this error, during type inference.
                            // e.g. "Sub f(Of T, U)(ByVal x As Func(Of T, U))" invoked with "f(function(z)z)"
                            // needs to put the squiggly on the first "z".
                            if (reportInferenceAssumptions)
                            {
                                BackupValue<bool> backup_report_errors(&(pSemantics->m_ReportErrors));
                                pSemantics->m_ReportErrors = true;
                                pSemantics->ReportLambdaParameterInferredToBeObject(lambdaParameter);
                                backup_report_errors.Restore();
                            }

                            SkipThisNode = true;
                            break;
                        }
                    }
                }

            }

            if (!SkipThisNode)
            {
                m_InferenceComplete = true;
                return false; // DOn't restart the algorithm.
            }
        }
    }

    Type *argumentType = NULL;
    GenericParameter* typeParameter = NULL;
    bool inferenceOk = false;

    switch (m_expression->bilop)
    {
        case SX_ADDRESSOF:
            inferenceOk = typeInference->InferTypeArgumentsFromAddressOfArgument(
                m_expression,
                m_Parameter,
                m_ParameterType,
                &typeParameter);
            break;
        case SX_UNBOUND_LAMBDA:
        case SX_LAMBDA:
            // $


            if (m_expression->ResultType->IsVoidType())
            {
                AsyncSubAmbiguityFlags AsyncSubArgumentAmbiguity = FoundNoAsyncOverload;

                GetTypeInference()->MarkInferenceLevel(TypeInferenceLevelOrcas);
                inferenceOk = typeInference->InferTypeArgumentsFromLambdaArgument(
                    m_expression,
                    m_Parameter,
                    m_ParameterType,
                    &typeParameter,
                    &AsyncSubArgumentAmbiguity);

                AddAsyncSubArgumentAmbiguity(ppAsyncSubArgumentListAmbiguity, this->m_argumentHolder, AsyncSubArgumentAmbiguity);
                break;
            }
            __fallthrough;
        default:

            // We should not infer from a Nothing literal because according
            // in reality Nothing is not object but is "Any" type.
            if (IsNothingLiteral(m_expression) && !m_expression->IsExplicitlyCast)
            {
                m_InferenceComplete = true;

                // continue without restarting, if all hints are Nothhing the InferenceTypeNode will mark
                // the inference as failed.
                return false;
            }

            ConversionRequiredEnum InferenceRestrictions = ConversionRequired::Any;
            if ( m_Parameter &&
                 m_Parameter->IsByRefKeywordUsed() &&
                 (HasFlag32(m_expression, SXF_LVALUE) ||
                    (IsPropertyReference(m_expression) &&
                     GetTypeInference()->GetSemantics()->AssignmentPossible(m_expression->AsPropertyReferenceExpression()))
                 )
               )
            {
                // a ByRef parameter needs (if the argument was an lvalue) to be copy-backable into
                // that argument.
                VSASSERT(InferenceRestrictions == ConversionRequired::Any, "there should have been no prior restrictions by the time we encountered ByRef");

                InferenceRestrictions = CombineConversionRequirements(
                                            InferenceRestrictions,
                                            InvertConversionRequirement(InferenceRestrictions));

                VSASSERT(InferenceRestrictions == ConversionRequired::AnyAndReverse, "expected ByRef to require AnyAndReverseConversion");

            }

            inferenceOk = typeInference->InferTypeArgumentsFromArgument(
                &m_expression->Loc,
                m_expression->ResultType,
                m_expression->bilop==SX_ARRAYLITERAL && m_expression->ResultType->IsArrayLiteralType() && m_expression->AsArrayLiteralExpression().NumDominantCandidates!=1,
                m_Parameter,
                m_ParameterType,
                MatchBaseOfGenericArgumentToParameter,
                &typeParameter,
                InferenceRestrictions);
            break;
    }

    if (!inferenceOk)
    {
        DBG_SWITCH_PRINTF(fDumpInference, "  !! Inference has failed. Mismatch of Argument and Parameter signature, so could not find type hints.\n");
        typeInference->MarkInferenceFailure();
        if ( !(m_expression->ResultType != NULL && TypeHelpers::IsRootObjectType(m_expression->ResultType)))
        {
            typeInference->ReportNotFailedInferenceDueToObject();
        }
    }

    m_InferenceComplete = true;

    return false; // Don't restart the algorithm;
}

void
Semantics::InferenceNamedNode::SetExpression
(
    _In_opt_ ILTree::Expression* expression
)
{
    ThrowIfNull(expression);

    m_expression = expression;
}

ILTree::Expression*
Semantics::InferenceNamedNode::GetExpression
(
)
{
    return m_expression;
}

void Semantics::InferenceNamedNode::SetArgumentHolder(_In_opt_ ILTree::Expression* argumentHolder)
{
    m_argumentHolder = argumentHolder;
}

/*****************************************************************************
*
* ArgumentParameterIterator Class
*
******************************************************************************/

Semantics::ArgumentParameterIterator::ArgumentParameterIterator
(
    Parameter*      parameters,
    ILTree::Expression**    boundArguments,
    ILTree::Expression*     firstParamArrayArgument,
    bool            ignoreFirstParameter
) :
    m_parameter(parameters),
    m_nextParameter(parameters),
    m_parameterType(NULL),
    m_parameterIsExpandedParamArray(false),

    m_boundArguments(boundArguments),
    m_firstParamArrayArgument(firstParamArrayArgument),
    m_currentParamArrayArgument(firstParamArrayArgument),
    m_argumentIndex(0),
    m_argument(NULL),
    m_argumentType(NULL),
    m_argumentHolder(NULL),

    m_ignoreFirstParameter(ignoreFirstParameter)
{
}

bool
Semantics::ArgumentParameterIterator::MoveNext
(
)
{
    if (m_ignoreFirstParameter && m_parameter)
    {
        m_parameter = m_parameter->GetNext();
        m_nextParameter = m_parameter;
        m_ignoreFirstParameter = false;
    }

    if (m_nextParameter == NULL)
    {
        return false;
    }

    // Set current parameter
    m_parameter = m_nextParameter;
    m_parameterType = GetDataType(m_parameter);

    ILTree::Expression *Argument = m_boundArguments[m_argumentIndex];
    if (Argument)
    {
        m_argumentHolder = Argument;
        m_argument = Argument->AsArgumentExpression().Left;
        m_argumentType = m_argument->ResultType;

        m_argumentIndex++;
    }
    else if (m_parameter->IsParamArray() && m_currentParamArrayArgument)
    {
        m_argumentHolder = m_currentParamArrayArgument;
        m_argument = m_currentParamArrayArgument->AsExpressionWithChildren().Left;
        m_argumentType = m_argument->ResultType;

        if (m_currentParamArrayArgument == m_firstParamArrayArgument &&
            m_currentParamArrayArgument->AsExpressionWithChildren().Right == NULL &&
            Semantics::ArgumentTypePossiblyMatchesParamarrayArrayShape(m_argumentType, m_parameterType))
        {
            // Consider in unexpanded form
        }
        else if (TypeHelpers::IsArrayType(m_parameterType))
        {
            // Consider in expanded form
            //
            m_parameterType = TypeHelpers::GetElementType(m_parameterType->PArrayType());
            m_parameterIsExpandedParamArray = true;
        }

        m_currentParamArrayArgument = m_currentParamArrayArgument->AsExpressionWithChildren().Right;
    }
    else
    {
        return false; // Mismatch in arguments.
    }

    // Fetch next parameter for next call
    if (m_parameter != NULL)
    {
        if (m_parameterIsExpandedParamArray)
        {
            m_nextParameter = m_parameter;
        }
        else
        {
            m_nextParameter = m_parameter->GetNext();
        }
    }


    return true;
}

Parameter*
Semantics::ArgumentParameterIterator::CurrentParameter()
{
    return m_parameter;
}

Type*
Semantics::ArgumentParameterIterator::ParameterType()
{
    return m_parameterType;
}

ILTree::Expression*
Semantics::ArgumentParameterIterator::CurrentArgument()
{
    return m_argument;
}

Type*
Semantics::ArgumentParameterIterator::ArgumentType()
{
    return m_argumentType;
}

ILTree::Expression*
Semantics::ArgumentParameterIterator::CurrentArgumentHolder()
{
    return m_argumentHolder;
}

bool
Semantics::ArgumentParameterIterator::IsExpandedParamArray()
{
    return m_parameterIsExpandedParamArray;
}
