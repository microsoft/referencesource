//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB Lambda Expression semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

ILTree::UnboundLambdaExpression * Semantics::InterpretLambdaExpression(
    _In_ ParseTree::LambdaExpression *LambdaParseTree,
    ExpressionFlags InterpretBodyFlags)
{
    ILTree::UnboundLambdaExpression *UnboundLambda = NULL;

    // Lambda interpretation is necessarily deferred until we try to convert it
    // to something
    UnboundLambda = &m_TreeAllocator.xAllocBilNode(SX_UNBOUND_LAMBDA)->AsUnboundLambdaExpression();
    UnboundLambda->Loc = LambdaParseTree->TextSpan;
    UnboundLambda->FirstParameter = InterpretLambdaParameters(LambdaParseTree->Parameters, LambdaParseTree->MethodFlags);
    UnboundLambda->ResultType = TypeHelpers::GetVoidType();
    UnboundLambda->vtype = t_void;
    UnboundLambda->InterpretBodyFlags = InterpretBodyFlags;
    UnboundLambda->AllowRelaxationSemantics = LambdaParseTree->AllowRelaxationSemantics;
    UnboundLambda->IsFunctionLambda = !!(LambdaParseTree->MethodFlags & DECLF_Function);
    UnboundLambda->IsStatementLambda = LambdaParseTree->IsStatementLambda || LambdaParseTree->IsAsync();
    UnboundLambda->IsSingleLine = LambdaParseTree->IsSingleLine;
    UnboundLambda->IsAsyncKeywordUsed = !!(LambdaParseTree->MethodFlags & DECLF_Async);
    UnboundLambda->IsIteratorKeywordUsed = !!(LambdaParseTree->MethodFlags & DECLF_Iterator);

    if (UnboundLambda->IsStatementLambda)
    {
        VSASSERT(LambdaParseTree->GetStatementLambdaBody() != NULL, "unexpected: statement lambda with null body");

        UnboundLambda->SetLambdaStatement( LambdaParseTree->GetStatementLambdaBody() );
        // We have to set UnboundLambda->WholeSpan, BodySpan. This diagram from boundtrees.h shows how they look
        // for multiline lambdas:
        // <4 Sub() <5                     <4..4> (the normal ILTree) TextSpan
        //    Console.WriteLine("hello")   <5..5> BodySpan
        // 5> End Sub 4>
        //
        // And we set them up from the parse-tree locations, explained in Parser.cpp::ParseStatementLambda:
        // <1 <2 <3 Sub() 3>                           <1..1> LambdaExpression.TextSpan
        //              Console.WriteLine("hello")     <2..2> LambdaExpression.GetStatementLambdaBody.BodyTextSpan
        //       2> End Sub 1>                         <3..3> LambdaExpression.GetStatementLambdaBody.TextSpan
        //
        // <5..5> comes from 3>..2>
        UnboundLambda->BodySpan.m_lBegLine   = LambdaParseTree->GetStatementLambdaBody()->TextSpan.m_lEndLine;    // 3>
        UnboundLambda->BodySpan.m_lBegColumn = LambdaParseTree->GetStatementLambdaBody()->TextSpan.m_lEndColumn;  // 3>
        UnboundLambda->BodySpan.m_lEndLine   = LambdaParseTree->GetStatementLambdaBody()->BodyTextSpan.m_lEndLine;  // 2>
        UnboundLambda->BodySpan.m_lEndColumn = LambdaParseTree->GetStatementLambdaBody()->BodyTextSpan.m_lEndColumn;// 2>
    }
    else
    {
        VSASSERT(LambdaParseTree->GetSingleLineLambdaExpression() != NULL, "unexpected: expression lambda with null expression");

        UnboundLambda->SetLambdaExpression( LambdaParseTree->GetSingleLineLambdaExpression() );
    }

    return UnboundLambda;
}


Location Semantics::ExtractLambdaErrorSpan(_In_ ILTree::UnboundLambdaExpression *UnboundLambda)
{
    // This function figures out a good place to highlight lambdas for errors.
    // In the following examples, the quotation marks show which span it picks:
    //   "Async Function(...) As Task" : ... : End Function
    //   "Iterator Function()" : ... End Function
    //   "Function() 1"
    // It'd be great if, for the last case, we could do just "Function()" 1, but that's not possible --
    // the ILTree::UnboundLambdaExpression no longer contains information about the closing parenthesis.

    // Statement lambdas and expression lambdas store their locations completely differently.
    // Note that things if a single-line lambda had a modifier (Async/Iterator) on it, then it would
    // ALREADY have been turned into a statement lambda.

    if (!UnboundLambda->IsStatementLambda)
    {
        // Expression-lambdas: This is what the spans look like...
        //    <1 Function(x as Integer)    <2 x+1 2> 1>
        // where <1...1> is UnboundLambda->Loc, and <2..2> is UnboundLambda->GetLambdaExpression->Loc
        // We will pick the span <1...1>. Note that the span <1...<2 would be inappropriate, since
        // it would include up to the first characted of the expression, rather than just to the closing parenthesis.
        Location loc1 = UnboundLambda->Loc;
        return loc1;
    }

    // Statement-lambdas: This is what the spans look like...
    //     <1 Async 1> <4 Sub(x As Integer) As Task <5    <1..1> the specifier (if present)
    //         Console.WriteLine("hello")                 <4..4> the normal ILTree Loc
    //     5> End Sub 4>                                  <5..5> BodySpan

    // Specifier:
    ParseTree::SpecifierList *specifiers = UnboundLambda->GetLambdaStatement()->pOwningLambdaExpression->Specifiers;
    VSASSERT(specifiers->HasSpecifier(ParseTree::Specifier::Async) == NULL || specifiers->HasSpecifier(ParseTree::Specifier::Iterator) == NULL,
             "Expected to see no more than one modifier on lambda");
    ParseTree::SpecifierList * specifier = specifiers->HasSpecifier(ParseTree::Specifier::Async);
    if (specifier == NULL)
    {
        specifier = specifiers->HasSpecifier(ParseTree::Specifier::Iterator);
    }

    Location loc1 = (specifier != NULL) ? specifier->Element->TextSpan : Location::GetHiddenLocation();
    Location loc4 = UnboundLambda->Loc;
    Location loc5 = UnboundLambda->BodySpan;

    Location loc;
    loc.SetStart(specifier != NULL ? loc1.GetStartPoint() : loc4.GetStartPoint());
    loc.SetEnd(loc5.GetStartPoint());
    return loc;
}



Parameter *
Semantics::InterpretLambdaParameters
(
    ParseTree::ParameterList *LambdaParameters,
    DECLFLAGS    MethodFlags
)
{
    Parameter *FirstParam = NULL, *LastParam = NULL;

    Declared::MakeParamsFromLambda(
        m_CompilerHost,
        &m_TreeStorage,
        m_SourceFile,
        m_CompilationCaches,
        m_ReportErrors ? m_Errors : NULL,
        LambdaParameters,
        MethodFlags,
        m_Procedure,
        m_Lookup,
        PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags,
        &FirstParam,
        &LastParam);

    Parameter * retval = FirstParam;

    // MakeParamsFromLambda doesn't set the type for parameters, let's set it here
    // Microsoft:2008.09.20 - with respect to the above comment: really???! it does seem to set the type for parameters!
    ParseTree::ParameterList *CurrentParsedParam = LambdaParameters;
    BCSYM_Param* CurrentParam = FirstParam;
    while (CurrentParam != NULL && CurrentParsedParam != NULL)
    {
        while (CurrentParsedParam && CurrentParsedParam->Element->Name->Name.IsBad)
        {
            // We will not create locals for params with bad names, so we should ignore them from
            // the CurrentParsedParam list.
            CurrentParsedParam = CurrentParsedParam->Next;

            if (!CurrentParsedParam)
            {
                continue;
            }
        }
        
        bool TypeIsBad = false;
        Type *CurrentParameterType = CurrentParam->GetRawType();

        if (CurrentParameterType != NULL)
        {
            TypeIsBad = CurrentParameterType->IsBad();
            // Dev10#489103: if CurrentParameterType was a NamedType pointing to a GenericBadRoot,
            // we now consider that IsBad() as well. (in Orcas it wasn't.)

            if (TypeIsBad)
            {
                // Continue interpretation as if the type was object.
                // If the type was wrong an error had been reported already.
                CurrentParam->SetType(GetFXSymbolProvider()->GetObjectType());
                CurrentParam->SetIsBad();
            }
        }
        else
        {
            Type* ParameterType = InterpretTypeName(CurrentParsedParam->Element->Type, TypeIsBad);
            if (TypeIsBad)
            {
                // as above
                CurrentParam->SetType(GetFXSymbolProvider()->GetObjectType());
                CurrentParam->SetIsBad();
            }
            else if (ParameterType != NULL && CurrentParsedParam->Element->Type != NULL)
            {
                if (CurrentParam->IsByRefKeywordUsed())
                {
                    ParameterType = m_SymbolCreator.MakePtrType(ParameterType);
                }
                CurrentParam->SetType(ParameterType);
            }
            else
            {
                CurrentParam->SetType(NULL); // Will be inferred
            }
        }

        CurrentParam = CurrentParam->GetNext();
        CurrentParsedParam = CurrentParsedParam->Next;
    }
    VSASSERT(CurrentParam == NULL &&
        (CurrentParsedParam == NULL || CurrentParsedParam->Element->Name->Name.IsBad),
        "Must exhaust both parameter lists.");

    return retval;
}


// ------------------------------------------------------------------------------------------------------------------------
// Semantics::InterpretUnboundLambdaBinding -- Spec $11.1 "Expression Reclassification for lambda"
// ------------------------------------------------------------------------------------------------------------------------
// This function calls IntepretUnboundLambda(...) to turn a lambda-parse-tree into a lambda-bound-tree,
// but before and after that it does all the necessary work for figuring out the correct return type
// and generating the relaxed-lambda-stub if one was needed.
// It also sets "RelaxationLevel" which is used (1) by ConversionClassification for judging whether
// Lambda->Type is identity/widening/narrowing/error, and also by (2) OverloadResolution to help pick
// one overload over another based on whether the thing is identity/widening/narrowing/error.
// Note that for Type->Type (e.g. VB$AnonymousDelegate->Action), then ConversionClassification also sets RelaxationLevel independently,
// and that ConvertWithErrorChecking passes on that RelaxationFlag to whoever called it. The caller who ultimately
// uses it is the overload-resolve.
// NOTE: in this function we are entirely responsible for setting RelaxationLevel from a lambda to another type.
// This includes setting it based on the interpretation we had to do (e.g. conversion of return expressions), and
// on the relaxation we had to do (e.g. FunctionStatementLambda->Action is accomplished by a stub), and on the
// target type (e.g. Lambda->Object counts as relaxing to something not a lambda).
//
// The spec $11.1 explains: "A lambda method can be reclassified as a value...."
//  * If the reclassification occurs in the context of a conversion where the target type is known and a delegate type,
//    the lambda method is interpreted as the argument to a delegate-construction expression of the appropriate type.
//  * If the target type is not known [or not a delegate type], then the lambda method is interpreted as the argument
//    to a delegate instantiation expression of an anonymous delegate type with the same signature of the lambda method.
// The spec also says stuff about reclassification to an expression-tree, but the compiler deals with that later.
//
// Note that the spec isn't clear enough here. Notionally, once we've reclassified the lambda to a value in the presence
// of an appropriate target type, then our newly-classified lambda has an IDENTITY conversion to the target type. But
// overload resolution doesn't yet care about that. Overload resolution is more concerned about the delegate relaxation level
// that was needed to reclassify the lambda.
//
ILTree::Expression *
Semantics::InterpretUnboundLambdaBinding
(
    _Inout_ ILTree::UnboundLambdaExpression *UnboundLambda,
    Type *DelegateType,
    bool ConvertToDelegateReturnType,
    _Inout_ DelegateRelaxationLevel &RelaxationLevel,
    bool InferringReturnType,
    _Out_opt_ bool *pRequiresNarrowingConversion,
    _Out_opt_ bool *pNarrowingFromNumericLiteral,
    bool GetLambdaTargetTypeFromDelegate,
    bool inferringLambda,
    AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity,
    bool *pDroppedAsyncReturnTask
)
{
    // NOTE: The techniques used for multi-line and single-line lambdas differ radically.
    //
    // INFERRING: Inferring the return type of a multiline lambda is a sensitive thing, because
    // the process is responsible for emitting errors/warnings (e.g. "no return type can be inferred")
    // and so has to be done exactly once with errors turned on (though can be done additional times
    // with errors turned off).
    // Moreover, if the target delegate type is vague (e.g. "Dim x As Action = multiline_lambda") then
    // we have to first infer the lambda's return type, then interpret the lambda in the light of this
    // return type so generating the appropriate warnings/errors.
    //
    // Inferring the return type of a expression-lambda is easy. It doesn't generate those errors/warnings.
    //
    // HIJACKING: We might have interpreted a lambda, but then decided that its return
    // type wasn't right (e.g. it was a function but we needed an Action). For multi-line lambdas
    // we do this by generating a stub that invokes the regular multiline lambda and does the
    // appropriate thing.
    //
    // For an expression lambda (single-line function), it's instead done by Hijacking: the
    // lambda body itself is altered in place, if possible, e.g. with a Cast operator.


    // NOTE: RelaxationLevel is computed from three places. First, it's computed from the target type.
    // If the target type is Object/System.Delegate/System.MulticastDelegate then the relaxation
    // is at best going to be RelaxationIntoSomethingNotADelegate. Or if the target is a concrete
    // Sub delegate type while the lambda is a function lambda, then the relaxation is going to be
    // at best RelaxationWhileDroppingParamsOrReturn; likewise for parameters.
    // Second, it's computed from how well we managed to interpret the lambda.
    // e.g. For a multiline lambda, if we intended it to return
    // "Integer", then during interpretation as we go through each return statement we keep a log
    // of what manner of conversion there was from the return statement to that intended return type.
    // For a single-line lambda it's done just based on the expression. This is done within the
    // function we call, "InterpretUnboundLambda".
    //   * If the target type of the lambda was "Action" or some other sub delegate, then
    //     the RelaxationLevel isn't set: no relaxation is being done.
    //   * If the target type of the lambda was "Func(Of Int)" or some other function delegate,
    //     then we interpret the lambda with respect to that return type, and set RelaxationLevel accordingly.
    //   * If the target type of the lambda was "Object/Delegate/MulticastDelegate", then first we
    //     infer the return type of the lambda, and then we interpret it with respect to that return type,
    //     and we set RelaxationLevel accordingly.
    //
    // Third, if it's a single-line lambda which we're hijacking (i.e. converting its body into a different
    // expression type), then we set "ConversionRelaxationLevel" from this conversion. However, ConversionRelaxationLevel
    // is ignored.
    //
    // Finally, we call MethodConversion = ClassifyMethodConversion(..). The flags "MethodConversion" are used
    // (1) to update RelaxationLevel, and (2) to determine whether a stub lambda is needed.
    // This call assesses the lambda as a whole (formal parameters and return type); the earlier step only did
    // it based on return type. We pick the worst of the two RelaxationLevels computed.
    //
  
    // Examples: Here are four functions that we want to do OverloadResolution on. We invoke them with either
    // [s] a single-line lambda "Function() "hello"", or with [m] a multi-line lambda "Function() // Return "hello" // End Function".
    // This chart shows the key variables that are set by this function when we interpret the lambda in the context
    // of the target parameter type. Note that generic method type inference (of the "X" in the example below)
    // has already been performed by the time we reach this function.
    //
    //                                    | DelegateType  | DelegateReturnTypeToInfer |RelaxationLevel| Hijack    | 4.ClassifyMethodConversion   |StubRequired|6.RelaxationLevel
    // -----------------------------------+---------------------------+---------------+---------------+-----------+------------------------------+------------+------------------
    // Sub t(xf As Func(Of Object)) - [s] |  given as Func(Of Object) |  Object       | Widening      | to Object |  <skip>                      | no         | Widening
    //                              - [m] |  given as Func(Of Object) |  Object       | Widening      | <no>      |  [Object->Object]None        | no         | Widening
    // Sub t(xo As Object)          - [s] |1.we infer Func(Of String) |  String       | WideNonLambda | to Object |  <skip>                      | no         | WideNonLambda
    //                              - [m] |1.we infer Func(Of String) |  String       | WideNonLambda | <no>      |  [String->String]None        | no         | WideNonLambda
    // Sub t(Of X)(xx As Func(Of X))- [s] |  given as Func(Of String) |  String       | Identity      | to String |  <skip>                      | no         | Identity
    //                              - [m] |  given as Func(Of String) |  String       | Identity      | <no>      |  [String->String]None        | no         | Identity
    // Sub t(xa As Action)          - [s] |  given as Action          |  NULL         | WideDropRet   |3.<no>     |  <skip>                      | yes        | WideDropRet
    //                              - [m] |  given as Action          |2.infer String | WideDropRet   | <no>      |5.[String->NULL]Widening      | no         | WideDropRet
    //
    // 1. This inference is done about ten lines down, "DelegateType = InferLambdaType(UnboundLambda, UnboundLambda->Loc);"
    // 2. This inference is done about fifty lines down but only in particular circumstances,
    //    "DelegateReturnTypeToInterpret = InferMultilineLambdaReturnTypeFromReturnStatements(UnboundLambda);"
    // 3. For a single-line expression lambda, if the target delegate type is a Sub, then no hijacking occurs
    // 4. In this column I've written the conversion it examines from the lambda return type to the intended
    //    delegate return type, and also the "RelaxationLevel" that's produced as a consequence.
    //    Note that these lambdas/delegates have no parameters, so what we see is based solely on the returns.
    // 5. An expression lambda can be converted to "Action" without any stub. But to convert a multiline lambda
    //    you do need a stub for reasons of "MethodConversionReturnValueIsDropped".
    // 6. This column shows the final relaxation level returned by this function.
    //


    if(pRequiresNarrowingConversion)
    {
        *pRequiresNarrowingConversion = false;
    }


    // Dev10#626389, Dev10#693976: If you convert a lambda to Object/Delegate/MulticastDelegate, then
    // the best it can ever be is DelegateRelaxationWideningToNonLambda.
    // And if you drop the return value from a lambda, the best it can be is DelegateRelaxationLevelWideningDropReturnOrArgs...
    DelegateRelaxationLevel maxRelaxationLevel = DelegateRelaxationLevelNone;
    //
    if (TypeHelpers::IsStrictSupertypeOfConcreteDelegate(DelegateType, GetFXSymbolProvider())) // covers Object, System.Delegate, System.MulticastDelegate
    {
        maxRelaxationLevel = DelegateRelaxationLevelWideningToNonLambda;
    }
    else if (DelegateType != NULL && UnboundLambda->IsFunctionLambda)
    {
        Type *DelegateReturnType = TypeHelpers::GetReturnTypeOfDelegateType(DelegateType, m_Compiler);
        if (DelegateReturnType != NULL && DelegateReturnType->IsVoidType())
        {
            maxRelaxationLevel = DelegateRelaxationLevelWideningDropReturnOrArgs;
        }
    }


    // Should we infer the lambda's type? -- yes if the (target) DelegateType was NULL (signifying that we should infer),
    // and also yes if the DelegateType was Object/System.Delegate/System.MulticastDelegate (signifying that we should infer
    // but set DelegateRelaxationLevel as above).
    if (DelegateType == NULL || 
        TypeHelpers::IsStrictSupertypeOfConcreteDelegate(DelegateType, GetFXSymbolProvider())) // covers Object, System.Delegate, System.MulticastDelegate
    {
        Type * TargetType = DelegateType;

        DelegateType = InferLambdaType(UnboundLambda, UnboundLambda->Loc);

        if (!DelegateType)
        {
            if (TargetType &&
                (TargetType==GetFXSymbolProvider()->GetDelegateType() || TargetType==GetFXSymbolProvider()->GetMultiCastDelegateType()))
            {
                ReportSemanticError(ERRID_LambdaNotCreatableDelegate1, UnboundLambda->Loc, TargetType);
            }
            else if (TargetType!=NULL)
            {
                ReportSemanticError(ERRID_LambdaNotDelegate1, UnboundLambda->Loc, TargetType);
            }
            else
            {
                VSFAIL("ICE: InferLambdaType only returns NULL in cases where the lambda is a query, and we shouldn't have tried to convert such a lambda to a NULL target-type.");
                ReportSemanticError(ERRID_InternalCompilerError, UnboundLambda->Loc);
            }
            return AllocateBadExpression(UnboundLambda->Loc);
        }
    }
    
    VSASSERT(DelegateType!=NULL, "Internal logic error: DelegateType should not be NULL here");

    if(pNarrowingFromNumericLiteral)
    {
        *pNarrowingFromNumericLiteral = false;
    }

    Declaration *InvokeMethod = GetInvokeFromDelegate(DelegateType, m_Compiler);
    bool AllowRelaxationSemantics = UnboundLambda->AllowRelaxationSemantics;

    if (InvokeMethod == NULL || IsBad(InvokeMethod) || !InvokeMethod->IsProc())
    {
Mismatch:
        ReportLambdaBindingMismatch(DelegateType, UnboundLambda->Loc, UnboundLambda->IsFunctionLambda);

        return AllocateBadExpression(UnboundLambda->Loc);
    }

    BCSYM_Proc *DelegateProc = InvokeMethod->PProc();
    Parameter *DelegateParameters = DelegateProc->GetFirstParam();

    GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(DelegateType) ? DelegateType->PGenericTypeBinding() : NULL;

    bool badParam = false;
    // Fill in the parameter types.
	Parameter *DelegateParam, *LambdaParam;
    for (DelegateParam = DelegateParameters,
            LambdaParam = UnboundLambda->FirstParameter;
         DelegateParam && LambdaParam;
         DelegateParam = DelegateParam->GetNext(),
            LambdaParam = LambdaParam->GetNext())
    {
        if (TypeHelpers::IsBadType(DelegateParam->GetType()))
        {
            ReportBadType(DelegateParam->GetType(), UnboundLambda->Loc);
            return AllocateBadExpression(UnboundLambda->Loc);
        }

        badParam = badParam || LambdaParam->IsBad();

        // Get the type of the parameter
        Type *DelegateParamType = GetDataType(DelegateParam);
        DelegateParamType = ReplaceGenericParametersWithArguments(DelegateParamType, DelegateBindingContext, m_SymbolCreator);

        Type* tp = LambdaParam->GetRawType();
        if (tp && tp->IsNamedType())
        {
            tp = tp->DigThroughNamedType();
        }

        bool ByRefUsedInAsycnOrIterator = 
            LambdaParam->IsByRefKeywordUsed() && (UnboundLambda->IsAsyncKeywordUsed || UnboundLambda->IsIteratorKeywordUsed);

        if (ByRefUsedInAsycnOrIterator)
        {
            ReportSemanticError(
                UnboundLambda->IsAsyncKeywordUsed ? ERRID_BadAsyncByRefParam : ERRID_BadIteratorByRefParam,
                *LambdaParam->GetLocation());
        }
        
        if (tp && !ByRefUsedInAsycnOrIterator)
        {
            // Verify for restricted types for this argument if we are sticking it in a generic type parameter.
            Type* typeToCheckForArgIterator = LambdaParam->IsByRefKeywordUsed() ? tp->ChaseThroughPointerTypes() : tp;
            if (( TypeHelpers::IsGenericParameter(GetDataType(DelegateParam)) ||
                    DelegateParam->IsByRefKeywordUsed()) &&
                ( IsRestrictedArrayType(typeToCheckForArgIterator, m_CompilerHost) ||
                  IsRestrictedType(typeToCheckForArgIterator, m_CompilerHost)))
            {
                StringBuffer TextBuffer;

                Location* RestrictedTypeLocation = LambdaParam->GetRawType()->GetTypeReferenceLocation();
                if (!RestrictedTypeLocation || RestrictedTypeLocation->IsInvalid())
                {
                    RestrictedTypeLocation = LambdaParam->GetLocation();
                }

                ReportSemanticError(
                    ERRID_RestrictedType1,
                    *RestrictedTypeLocation,
                    m_Errors ? m_Errors->ExtractErrorName(tp, NULL, TextBuffer) : NULL);

                return AllocateBadExpression(UnboundLambda->Loc);
            }

            if (!AllowRelaxationSemantics && !InferringReturnType && !TypeHelpers::EquivalentTypes(LambdaParam->GetType(), DelegateParamType))
            {
                goto Mismatch;
            }
        }
        else if (!tp)
        {
            if (LambdaParam->IsByRefKeywordUsed())
            {
                if (DelegateParamType->IsPointerType())
                {
                    Symbols::SetParamType(LambdaParam, DelegateParamType);
                }
                else
                {
                    Symbols::SetParamType(LambdaParam, m_SymbolCreator.MakePtrType(DelegateParamType));
                }
            }
            else
            {
                Symbols::SetParamType(LambdaParam, DelegateParamType->ChaseThroughPointerTypes());
            }

            if (DelegateType->IsAnonymousDelegate() &&
                !LambdaParam->IsQueryIterationVariable())
            {
                ReportLambdaParameterInferredToBeObject(LambdaParam);
            }
        }
    }

    // We don't allow ParamArray and Optional anymore so argument / param mismatch is always an error.
    // Unless we had no parameters at all on the lambda, that is a valid relaxation.
    if (DelegateParam || LambdaParam)
    {
        if (UnboundLambda->FirstParameter != NULL)
        {
            goto Mismatch;
        }
        if (!AllowRelaxationSemantics && !InferringReturnType)
        {
            goto Mismatch;
        }
    }

    Type *DelegateReturnType = DelegateProc->GetType();
    DelegateReturnType = ReplaceGenericParametersWithArguments(DelegateReturnType, DelegateBindingContext, m_SymbolCreator);

    BackupValue<TriState<bool>> backup_reporttypeinferenceerrors(&m_ReportMultilineLambdaReturnTypeInferenceErrors);

    Type* DelegateReturnTypeToInterpret = NULL;
    if (UnboundLambda->IsStatementLambda && UnboundLambda->IsFunctionLambda && UnboundLambda->GetLambdaStatement()->pReturnType)
    {
        // This lambda is of the form "Function(Parameters) As Type"
        bool typeIsBad;
        DelegateReturnTypeToInterpret = InterpretTypeName(UnboundLambda->GetLambdaStatement()->pReturnType, typeIsBad);

        if (!DelegateReturnTypeToInterpret)
        {
            // There was an error in the return type, this error should be captured elsewhere.
            DelegateReturnTypeToInterpret = TypeHelpers::GetVoidType();
        }
    }
    else if (UnboundLambda->IsStatementLambda && 
             UnboundLambda->IsFunctionLambda &&
             (!GetLambdaTargetTypeFromDelegate ||
              !DelegateReturnType ||
              DelegateReturnType->IsVoidType() ||
              (UnboundLambda->IsAsyncKeywordUsed && GetFXSymbolProvider()->IsTypeAvailable(FX::TaskType) && TypeHelpers::EquivalentTypes(DelegateReturnType, GetFXSymbolProvider()->GetType(FX::TaskType)))
             ))
    {
        // Here we have to infer types differently for Async+Iterator lambdas
        //
        // If we have a multiline function lambda and we do not have a type on the left (GetLambdaTargetTypeFromDelegate will be false):
        // Dim x = Function()
        //             Return 1
        //         End Function
        //
        // Or we have a mutliline function lambda and the type on the left is a sub delegate:
        // Dim x As Action = Function()
        //                       Return 1
        //                   End Function
        //
        // Then we want to infer the return type of the lambda from its return statements.
        //

        // Microsoft:
        // Bug 485730 - in the case where you do
        // Dim x As Action = Function()
        //                       Return 1
        //                   End Function
        // This code path infers a type, which will generate errors. We must do something similar to
        // Semantics::InterpretExpression where we will set m_ReportMultilineLambdaReturnTypeInferenceErrors
        // to false if we run this.

        DelegateReturnTypeToInterpret = InferMultilineLambdaReturnTypeFromReturnStatements(UnboundLambda);
        m_ReportMultilineLambdaReturnTypeInferenceErrors.SetValue(false);

        if (!DelegateReturnTypeToInterpret)
        {
            // There was an error in the return type, this error should be captured elsewhere.
            DelegateReturnTypeToInterpret = TypeHelpers::GetVoidType();
        }
    }
    else if (GetLambdaTargetTypeFromDelegate && UnboundLambda->IsFunctionLambda)
    {
        DelegateReturnTypeToInterpret = DelegateReturnType;
    }
    else if (!UnboundLambda->IsFunctionLambda || DelegateReturnType == NULL)
    {
        DelegateReturnTypeToInterpret = TypeHelpers::GetVoidType();
    }
    else if (!HasFlag(UnboundLambda->InterpretBodyFlags, ExprInferResultTypeExplicit))
    {
        DelegateReturnTypeToInterpret = DelegateReturnType;
    }

    bool fAsyncLacksAwaits = false;
    ILTree::LambdaExpression *Result = InterpretUnboundLambda(UnboundLambda, DelegateReturnTypeToInterpret, &RelaxationLevel, inferringLambda, &fAsyncLacksAwaits);

    // Dev10#524629: If we were interpreting a multiline lambda which declares an explicit "As" clause, then
    // forget the relaxation level that was involved inside the lambda: we should only judge
    // relaxation level based on how well the declared return type matches the target return type
    if (UnboundLambda->IsStatementLambda && UnboundLambda->IsFunctionLambda && UnboundLambda->GetLambdaStatement()->pReturnType)
    {
        MethodConversionClass ReturnTypeConversion = MethodConversionIdentity;
        ClassifyReturnTypeForMethodConversion(DelegateReturnTypeToInterpret, DelegateReturnType, ReturnTypeConversion);
        RelaxationLevel = DetermineDelegateRelaxationLevel(ReturnTypeConversion);
        // Microsoft 2009.06.07 -- this is likely a bug. We should be doing RelaxationLevel = max(RelaxationLevel, DetermineDelegateRelaxationLevel(...))
        // All the way down from MatchArguments through to ConvertWithErrorChecking, people only ever do max(..) on RelaxationLevel;
        // they don't reset it.
    }


    backup_reporttypeinferenceerrors.Restore();

    if (badParam)
    {
        MakeBad(Result);
    }


    // Bug: 139544. When we are assigning a lambda to a void type and we are going to relax the return type
    // we need to check if the body is a lambda and forcefully interpret the lambda at this time
    // otherwise it will be left int he parse tree since we will not try to convert the expression because the void type
    // of the delegate matches the void of the unbound lambda.
    // 

    if (!IsBad(Result) &&
        AllowRelaxationSemantics &&
        DelegateReturnType == NULL &&
        !Result->IsStatementLambda &&
        Result->GetExpressionLambdaBody()->bilop == SX_UNBOUND_LAMBDA &&
        (!Result->IsFunctionLambda || TypeHelpers::IsVoidType(Result->GetExpressionLambdaBody()->ResultType)))
    {
        // Port SP1 CL 2922610 to VS10

        BackupValue<Scope *> Lookup_backup(&m_Lookup);
        BackupValue<TemporaryManager *> TemporaryManager_backup(&m_TemporaryManager);
        BackupValue<LambdaKind> backupInLambda(&m_InLambda);

        m_Lookup = Result->GetLocalsHash();                  
        m_TemporaryManager = Result->TemporaryManager;  

        // 
        m_InLambda = SingleLineLambda;                              


        Type* InferredDelegateReturnType = InferLambdaType(&Result->GetExpressionLambdaBody()->AsUnboundLambdaExpression(), Result->Loc);

        Lookup_backup.Restore();                        
        TemporaryManager_backup.Restore();                   
        backupInLambda.Restore();                        

        Result = InterpretUnboundLambda(UnboundLambda, InferredDelegateReturnType, &RelaxationLevel);

        // NOTE: Result->IsStatementLambda implies UnboundLambda->IsStatementLambda, but not vice versa.
        // e.g. UnboundLambda is "Sub() CallStatement", but Result is "Sub() CallExpression".
        // That's because InterpretUnboundLambda turns simple statement-lambdas into expression lambdas.
    }

    RelaxationLevel = max(RelaxationLevel, maxRelaxationLevel);

    FixLambdaExpressionForClosures(Result);

    BCSYM *pOriginalResultType = NULL;
    ILTree::ILNode *pOriginalLambdaBody = NULL;

    if (Result->IsStatementLambda)
    {
        pOriginalLambdaBody = Result->GetStatementLambdaBody();
        pOriginalResultType = Result->GetStatementLambdaBody()->pReturnType;
    }
    else
    {
        pOriginalLambdaBody = Result->GetExpressionLambdaBody();
        pOriginalResultType = Result->GetExpressionLambdaReturnType();
    }

    if (!IsBad(Result))
    {
        Type* LambdaResultType = 
            Result->IsStatementLambda ? Result->GetStatementLambdaBody()->pReturnType :
            Result->GetExpressionLambdaReturnType();

        // Verify if we are building a restricted type. and report if we are going to put it in a generic type.
        if (!TypeHelpers::IsBadType(LambdaResultType) && DelegateProc->GetType() && TypeHelpers::IsGenericParameter(DelegateProc->GetType()))
        {
            if (IsRestrictedArrayType(LambdaResultType, m_CompilerHost) ||
                IsRestrictedType(LambdaResultType, m_CompilerHost))
            {
                StringBuffer TextBuffer;

                if (m_Errors==NULL)
                {
                    VSFAIL("oops, see bug 729104");
                }
                else
                {
                    ReportSemanticError(
                    ERRID_RestrictedType1,
                    UnboundLambda->IsStatementLambda ? UnboundLambda->GetLambdaStatement()->TextSpan : UnboundLambda->GetLambdaExpression()->TextSpan,
                    m_Errors->ExtractErrorName(LambdaResultType, NULL, TextBuffer));
                }

                return AllocateBadExpression(UnboundLambda->Loc);
            }
        }

        if (!Result->IsFunctionLambda &&
            DelegateReturnType != NULL &&
            !DelegateReturnType->IsVoidType())
        {
            if (pAsyncSubArgumentAmbiguity != NULL)
            {
                *pAsyncSubArgumentAmbiguity |= FoundAsyncOverloadWithFunc;
            }

            // This is a sub lambda being assigned to a non-void returning delegate.
            goto Mismatch;
        }

        if (ConvertToDelegateReturnType && DelegateReturnType && !Result->IsStatementLambda)
        {
            if (TypeHelpers::IsBadType(DelegateReturnType))
            {
                ReportBadType(DelegateReturnType, UnboundLambda->Loc);
                return AllocateBadExpression(UnboundLambda->Loc);
            }
            else if (!Result->IsFunctionLambda && !DelegateReturnType->IsVoidType())
            {
                // This lambda is a Sub lambda assigned to a delegate that expects a return value.
                goto Mismatch;
            }
            else
            {
                if(Result->ExactlyMatchExpressionResultType) // (Bug 73827 - DevDiv Bugs)
                {
                    if ( !TypeHelpers::EquivalentTypes( pOriginalResultType, DelegateReturnType))
                    {
                        goto Mismatch;
                    }
                }
                else
                {
                    // We are modifying the internals of the Lamba here, so we need the Lambda's
                    // Temporary manager and locals hash in scope.

                    BackupValue<Scope *> LocalsHash_backup(&m_Lookup);
                    m_Lookup = Result->GetLocalsHash();

                    BackupValue<TemporaryManager *> TemporaryManager_backup(&m_TemporaryManager);
                    m_TemporaryManager = Result->TemporaryManager;

                    Parameter *CopyBackConversionParam = NULL;
                    bool RequiresNarrowingConversion = false;
                    bool NarrowingFromNumericLiteral = false;
                    DelegateRelaxationLevel ConversionRelaxationLevel = DelegateRelaxationLevelNone;
                    bool RequiresUnwrappingNullable = false;

                    ILTree::Expression *BoundExpression =
                        ConvertWithErrorChecking
                        (
                            Result->GetExpressionLambdaBody(),
                            DelegateReturnType,
                            ExprNoFlags,
                            CopyBackConversionParam,
                            RequiresNarrowingConversion,
                            NarrowingFromNumericLiteral,
                            false /*SuppressMethodNameInErrorMessages*/,
                            ConversionRelaxationLevel,
                            RequiresUnwrappingNullable
                         );

                    // Bug 35827 - DevDiv Bugs
                    if(pRequiresNarrowingConversion)
                    {
                        *pRequiresNarrowingConversion = RequiresNarrowingConversion;
                    }

                    // Bug 68422 - DevDiv Bugs
                    if(pNarrowingFromNumericLiteral)
                    {
                        *pNarrowingFromNumericLiteral = NarrowingFromNumericLiteral;
                    }

                    if(IsBad(BoundExpression))
                    {
                        Result->SetExpressionLambdaBody(BoundExpression);
                        MakeBad(Result);
                    }
                    else
                    {
                        Result->SetExpressionLambdaBody(MakeRValue(BoundExpression));
                    }

                    FixLambdaExpressionForClosures(Result);

                    // backups of m_Lookup and m_Temporary will be restored here when going out of scope
                }
            }
        }

        // For single line function lambdas, report an error if the lambda expression does not return a value.
        // #150829
        if (ConvertToDelegateReturnType &&  
            !IsBad(Result) && 
            !UnboundLambda->IsStatementLambda &&
            (UnboundLambda->IsFunctionLambda && HasFlag(UnboundLambda->InterpretBodyFlags, ExprForceRValue) || 
                UnboundLambda->GetLambdaExpression()->Opcode == ParseTree::Expression::AddressOf) && // Sub lambdas cannot have an address of expression.
            // Leave this checking the result type of the expression in the single line lambda.
            TypeHelpers::IsVoidType(Result->GetExpressionLambdaBody()->ResultType))
        {
            ReportSemanticError(
                ERRID_VoidValue,
                Result->GetExpressionLambdaBody()->Loc);

            MakeBad(Result);
        }

        if (fAsyncLacksAwaits && !IsBad(Result))
        {
            VSASSERT(UnboundLambda->IsStatementLambda, "async lambdas are always set to be statement lambdas");
            ReportSemanticError(WRNID_AsyncLacksAwaits, ExtractLambdaErrorSpan(UnboundLambda));
        }

        // Has a Task-returning async lambda has been relaxed to a void-returning thing (i.e. dropping the Task on the floor)?
        if (pDroppedAsyncReturnTask != NULL)
        {
            bool lambdaIsAsyncFunction = UnboundLambda->IsAsyncKeywordUsed && !IsBad(Result) && Result->IsFunctionLambda;
            bool targetIsVoidReturning = DelegateType != NULL && TypeHelpers::GetReturnTypeOfDelegateType(DelegateType, m_Compiler)->IsVoidType();
            *pDroppedAsyncReturnTask = (lambdaIsAsyncFunction && targetIsVoidReturning);
        }

        if (AllowRelaxationSemantics && !IsBad(Result))
        {
            // Now that we have the returntype and the parameters, check to see if the lambda is compatible with the
            // delegate we are hooking up to.

            Type *pReturnType = NULL;

            if (Result->IsFunctionLambda)
            {
                if (Result->IsStatementLambda)
                {
                    pReturnType = Result->GetStatementLambdaBody()->pReturnType;
                }
                else
                {
                    pReturnType = Result->GetExpressionLambdaReturnType();
                    VSASSERT( pReturnType != TypeHelpers::GetVoidType(), "A function lambda has a void return?" );
                }
            }
            else
            {
                // This is a SUB lambda.
                pReturnType = GetFXSymbolProvider()->GetType(FX::VoidType);
            }

            MethodConversionClass MethodConversion = ClassifyMethodConversion(
                pReturnType, //
                UnboundLambda->FirstParameter,
                false,
                GenericBindingInfo(), // Type of the Target have already been fuly resolved.
                DelegateProc,
                DelegateBindingContext,
                !(Result->IsFunctionLambda && Result->IsStatementLambda), // IgnoreReturnValueErrorsForInference since we already converted it if we could, unless we have a multiline function lambda.
                &m_SymbolCreator);

            RelaxationLevel = max(DetermineDelegateRelaxationLevel(MethodConversion), RelaxationLevel);

            bool WouldHaveMatchedIfOptionStrictWasOff;
            bool * pNarrowingConversion = NULL;

            // Leave pNarrowingConversion == NULL if we already know that we need
            // narrowing conversion, otherwise the flag may be cleared in IsSupportedMethodConversion
            if(pRequiresNarrowingConversion &&
                !(*pRequiresNarrowingConversion))
            {
                pNarrowingConversion = pRequiresNarrowingConversion;
            }

            if (!IsSupportedMethodConversion(
                    m_SourceFile->GetOptionFlags() & OPTION_OptionStrict,
                    MethodConversion,
                    &WouldHaveMatchedIfOptionStrictWasOff,
                    pNarrowingConversion,
                    false) // Not AddressOf
                )
            {
                ReportLambdaBindingMismatch(DelegateType, UnboundLambda->Loc, UnboundLambda->IsFunctionLambda, WouldHaveMatchedIfOptionStrictWasOff);

                return AllocateBadExpression(UnboundLambda->Loc);
            }

            // Can't rely on AddressOf conversion and then have it generate a stub because that will
            // happen when the closures get fixed up and we can't introduce lambda's at that time anymore.
            if (IsStubRequiredForMethodConversion(MethodConversion))
            {
                ParserHelper PH(&m_TreeStorage, UnboundLambda->Loc);

                BCSYM_Param* ParameterList = CopyParameterList(
                    DelegateProc,
                    DelegateBindingContext,
                    true /*Rename the parameters to be unique.*/);

                ParseTree::ArgumentList *InvokeArguments = Semantics::CreateArgumentList(
                    ParameterList,
                    Result->FirstParameter, // first parameter in target signature,
                    false, /*ForceCopyAllArguments*/
                    UnboundLambda->Loc);

                if (Result->IsStatementLambda)
                {
                    Result->SetStatementLambdaBody( &(pOriginalLambdaBody->AsStatementLambdaBlock()) );
                }
                else
                {
                    Result->SetExpressionLambdaBody( &(pOriginalLambdaBody->AsExpression()) );
                }

                FixLambdaExpressionForClosures(Result);

                ParseTree::Expression* CallHandler = PH.CreateMethodCall(
                    PH.CreateBoundExpression(Result),
                    InvokeArguments);

                // Build the lambda expression
                ILTree::UnboundLambdaExpression *StubbedLambda = NULL;
                StubbedLambda = &m_TreeAllocator.xAllocBilNode(SX_UNBOUND_LAMBDA)->AsUnboundLambdaExpression();
                StubbedLambda->Loc = UnboundLambda->Loc;
                StubbedLambda->SetLambdaExpression( CallHandler );
                StubbedLambda->FirstParameter = ParameterList;
                StubbedLambda->ResultType = DelegateType;
                StubbedLambda->vtype = t_ref;
                StubbedLambda->InterpretBodyFlags = ExprSkipOverloadResolution;
                StubbedLambda->AllowRelaxationSemantics = true;
                // Microsoft.2009.03.17 -- the following line is WRONG! The stubbed lambda we generate should be a function
                // if the target delegate is a function, or a sub if the target delegate is a sub. It should not depend
                // on whether the thing we bound to (Result) is a function or a sub. However, we end up not using the IsFunctionLambda
                // field again, so this bug is never exercised.
                StubbedLambda->IsFunctionLambda = Result->IsFunctionLambda;
                StubbedLambda->IsStatementLambda = false;
                StubbedLambda->IsSingleLine = true;

                DelegateRelaxationLevel StubbedRelaxationLevel = DelegateRelaxationLevelNone;
                ILTree::Expression* Lambda =  InterpretUnboundLambdaBinding(
                    StubbedLambda,
                    DelegateType,
                    true, // ConvertToDelegateReturnType
                    StubbedRelaxationLevel,
                    false,
                    NULL,
                    NULL,
                    false,
                    inferringLambda);

                Lambda->AsLambdaExpression().IsRelaxedDelegateLambda = true;

                return Lambda;
            }
        }

        Result->ResultType = DelegateType;
        Result->vtype = t_ref;
    }

    if (Result != NULL && Result->IsAsyncKeywordUsed && !Result->IsFunctionLambda && pAsyncSubArgumentAmbiguity != NULL)
    {
        *pAsyncSubArgumentAmbiguity |= FoundAsyncOverloadWithAction;
    }

    return Result;
}

Semantics::LambdaBodyInterpretExpression::LambdaBodyInterpretExpression
(
    _In_opt_ Semantics * semantics,
    ParseTree::Expression * body,
    bool IsAsyncKeywordUsed,
    ExpressionFlags flags,
    Type * TargetBodyType /*= NULL*/
)
{
    AssertIfNull(semantics);
    AssertIfNull(body);

    m_Semantics = semantics;
    m_Body = body;
    m_IsAsyncKeywordUsed = IsAsyncKeywordUsed;
    m_Flags = flags;
    m_TargetBodyType = TargetBodyType;
    m_OriginalResultType = NULL;
    m_ReturnRelaxationLevel = DelegateRelaxationLevelNone;
    m_SeenAwait = false;
    m_SeenReturnWithOperand = false;
}

ILTree::ILNode*
Semantics::LambdaBodyInterpretExpression::DoInterpretBody()
{
    // References to Me are not allowed in the arguments of a constructor call
    // if that call is the first statement in another constructor.
    BackupValue<bool> backupDisallowMeReferenceInConstructorCall(&m_Semantics->m_DisallowMeReferenceInConstructorCall);
    if (HasFlag(m_Flags, ExprIsInitializationCall))
    {
        m_Semantics->m_DisallowMeReferenceInConstructorCall = true;
    }


    // Set the current "expression-lambda-interpreter" field...
    BackupValue<Semantics::LambdaBodyInterpretExpression*> backupExpressionLambdaInterpreter(&m_Semantics->m_ExpressionLambdaInterpreter);
    m_Semantics->m_ExpressionLambdaInterpreter = this;

    if (m_TargetBodyType != NULL && TypeHelpers::IsVoidType(m_TargetBodyType))
    {
        // If Lambda is a sub, don't necessarily force rValue
        ILTree::Expression *Result = m_Semantics->InterpretExpression(m_Body, m_Flags & (~ExprForceRValue));
        //
        // Dev10#474968: the VB specification says: if it is a Sub, then the expression can be
        // a CALL expression of Void type. But otherwise it must be classified as a value.
        // This "classifying-as-value" can end up producing warnings/errors e.g. for array literals.
        if (!IsBad(Result) && Result->bilop != SX_CALL)
        {
            Result = m_Semantics->ConvertWithErrorChecking(Result, NULL, m_Flags);
        }
        m_OriginalResultType = TypeHelpers::GetVoidType();
        return Result;
    }
    else
    {
        // If lambda is a function, interpret according to target type. (which may be NULL if target type wasn't declared)
        ILTree::Expression *Result = m_Semantics->InterpretExpressionWithTargetType(m_Body, m_Flags, m_TargetBodyType, &m_OriginalResultType);
        // Dev10#487850: the result type can be a BAD array literal...
        VSASSERT(IsBad(Result) || !Result->ResultType->IsArrayLiteralType(), "unexpected: after InterpretExpressionWithTargetType, we should have reclassified arrayliterals to values");
        return Result;
    }


    // following line is never executed, but we put it in to indicate that the backup will be restored when it leaves scope.
    backupDisallowMeReferenceInConstructorCall.Restore();
    backupExpressionLambdaInterpreter.Restore();
}

Semantics::LambdaBodyInterpretStatement::LambdaBodyInterpretStatement
(
    _In_opt_ Semantics * semantics,
    ParseTree::LambdaBodyStatement * body,
    ExpressionFlags flags,
    bool functionLambda,
    bool isSingleLine,
    bool isAsyncKeywordUsed,
    bool isIteratorKeywordUsed,
    Type * TargetBodyType /*= NULL*/
)
{
    AssertIfNull(semantics);
    AssertIfNull(body);

    m_Semantics = semantics;
    m_Body = body;
    m_Flags = flags;
    m_TargetBodyType = TargetBodyType;
    m_FunctionLambda = functionLambda;
    m_IsSingleLine = isSingleLine;
    m_IsAsyncKeywordUsed = isAsyncKeywordUsed;
    m_IsIteratorKeywordUsed = isIteratorKeywordUsed;
    m_OriginalResultType = NULL;
    m_ReturnRelaxationLevel = DelegateRelaxationLevelNone;
    m_SeenAwait = false;
    m_SeenReturnWithOperand = false;
}


ILTree::ILNode *
Semantics::LambdaBodyInterpretStatement::DoInterpretBody()
{
    // References to Me are not allowed in the arguments of a constructor call
    // if that call is the first statement in another constructor.
    BackupValue<bool> backupDisallowMeReferenceInConstructorCall(&m_Semantics->m_DisallowMeReferenceInConstructorCall);
    if (HasFlag(m_Flags, ExprIsInitializationCall))
    {
        m_Semantics->m_DisallowMeReferenceInConstructorCall = true;
    }

    BackupValue<ILTree::Statement*> backupLastStatementInContext(&(m_Semantics->m_LastStatementInContext));
    BackupValue<ILTree::ExecutableBlock*> backupContext(&(m_Semantics->m_BlockContext));
    BackupValue<ILTree::Statement**> backupStatementTarget(&(m_Semantics->m_StatementTarget));
    BackupValue<Scope*> backupScope(&(m_Semantics->m_Lookup));
    BackupValue<bool> backupCreateImplicitDeclarations(&(m_Semantics->m_CreateImplicitDeclarations));

    // Want to backup m_MultilineLambdaTree because we may want to set it to its pre-existing value once we finish interpreting
    // the block. This will usually be NULL but can also be another ILTree::StatementLambdaBlock if we are in a nested multiline lambda 
    // scenario.
    BackupValue<Semantics::LambdaBodyInterpretStatement*> backupMultilineLambdaInterpreter(&m_Semantics->m_StatementLambdaInterpreter);
    BackupValue<ILTree::StatementLambdaBlock*> backupOuterMultilineLambdaTree(&m_Semantics->m_OuterStatementLambdaTree);

    // Backup the "current expression lambda". See semantics.h for explanation of precedence:
    //   m_ExpressionLambdaInterpreter > m_StatementLambdaInterpreter > m_Procedure
    BackupValue<Semantics::LambdaBodyInterpretExpression*> backupExpressionLambdaInterpreter(&m_Semantics->m_ExpressionLambdaInterpreter);
    m_Semantics->m_ExpressionLambdaInterpreter = NULL;


    // We need a label hash bucket for each lambda, so we have to backup the current one, which is the label hash bucket for the lambdas container.
    BackupValue<LabelDeclarationList**> backupLabelHashBucket(&m_Semantics->m_LabelHashBuckets);
    BackupValue<unsigned> backupLabelHashBucketCount(&m_Semantics->m_LabelHashBucketCount);
        
    // AllocateBlock will establish a context for the block.
    VSASSERT(m_Body!=NULL, "Internal error: the constructors should have guaranteed that m_Body is non-null");
    m_Tree = &(m_Semantics->AllocateBlock(SB_STATEMENT_LAMBDA, m_Body, false)->AsStatementLambdaBlock());

    // The current lambda body we are in.
    m_Semantics->m_StatementLambdaInterpreter = this;

    // INVARIANT: if m_Semantics->m_StatementLambdaInterpreter is non-null,
    // then so is mSemantics->m_StatementLambdaInterpreter->m_Tree

    // The outermost statement lambda block.
    m_Semantics->m_OuterStatementLambdaTree = (m_Semantics->m_OuterStatementLambdaTree == NULL) ? m_Tree : m_Semantics->m_OuterStatementLambdaTree;

    // The new hash bucket for the labels of this lambda.
    m_Semantics->m_LabelHashBucketCount = m_Body ? max(m_Body->definedLabelCount * 2, 7) : 1;
    typedef LabelDeclarationList *LabelDeclarationPointer;
    m_Semantics->m_LabelHashBuckets = new(m_Semantics->m_TreeAllocator) LabelDeclarationPointer[m_Semantics->m_LabelHashBucketCount];

    // Create symbols for all defined labels. (Do this here, rather than when a label
    // statement is encountered, to avoid issues with gotos that preceded their targets.
    m_Semantics->CreateLabelDefinitions(m_Body);

    // Allow implicit declarations within the lambda body if Option Explicit is Off.
    m_Semantics->m_CreateImplicitDeclarations = !(m_Semantics->m_SourceFileOptions & OPTION_OptionExplicit);

    m_Tree->pReturnType = m_FunctionLambda ? 
                          m_TargetBodyType : 
                          m_Semantics->GetFXSymbolProvider()->GetType(FX::VoidType);

    m_Semantics->m_BlockContext->Locals =
                                         m_Semantics->m_SymbolCreator.GetHashTable(
                                                                        NULL,
                                                                        m_Semantics->m_Lookup,
                                                                        true,
                                                                        16,
                                                                        NULL);
    
    m_Semantics->m_Lookup = m_Semantics->m_BlockContext->Locals;

    m_Semantics->m_Lookup->SetIsLambdaHash(true);

    m_Semantics->IndicateLocalResumable();
    // Lambdas [here] indicate their resumable stuff in LambdaBodyInterpretStatement::DoInterpretBody, immediately before doing TryInterpretBlock
    // Procedures indicate their resumable stuff in Semantics::InterpretMethodBody, immediately before doing TryInterpretBlock

    if (!m_Semantics->TryInterpretBlock(m_Body)) // abort?
    {
        m_Semantics->ReportSemanticError(ERRID_EvaluationAborted, m_Body->TextSpan);
        MakeBad(m_Tree);
        return m_Tree;
    }
    
    m_Semantics->PopBlockContext();

    backupDisallowMeReferenceInConstructorCall.Restore();
    
    // SINGLESUB:
    // This is now our opportunity: if it was a single-line sub which only included an expression-statement,
    // then we'll actually return an ILNode::Expression rather than an ILNode::StatementLambdaBlock
    // But we'll only do this for subs that really were single line, i.e. to distinguish
    // "Sub() : Call f() : End Sub" from "Sub() Call f()".
    if (m_Tree->LocalsCount==0 && m_Tree->Locals->CSymbols()==0 &&
        m_Tree->Child!=NULL && m_Tree->Child->Next==NULL &&
        m_Tree->Child->bilop==SL_STMT && m_Tree->Child->AsStatementWithExpression().Operand1!=NULL &&
        m_IsSingleLine && !m_IsAsyncKeywordUsed)
    {
        // Exception: maybe in interpreting the body we had to allocate temporaries.
        // These were allocated by m_Semantics->AllocateLongLivedTemporary. Since we are
        // by definition in a multiline lambda, that routine stores the temporaries in
        // m_Semantics->m_TemporaryManager. (By contrast, expression lambdas would store
        // their temporaries in m_Semantics->m_ProcedureTree->ptempmgr). Anyway, if there
        // were any temporaries in the multiline lambda, we can't turn it into an expression.
        bool blockHasTemporaries = false;
        TemporaryIterator iterTemp;
        iterTemp.Init(m_Semantics->m_TemporaryManager);
        while (Temporary *Current = iterTemp.GetNext())
        {
            if (Current->Lifetime == LifetimeLongLived) 
            {
                blockHasTemporaries = true;
                break;
            }
        }
        if (!blockHasTemporaries)
        {
            // Okay, now we know we can return just the expression.
            // BUT! e.g. "Option Explicit Off / Sub() Call (Function(w) w+1)(3) / w = 1"
            // The compiler has to warn that parameter "w" shadows the implicit variable "w" declared later on.
            // It does this by adding "w" into a Tracking list associated with the Sub. Later on, when we get to
            // the implicit declaration of "w", then we go back into that tracking list.
            // Block lambdas store their tracking list in their m_Tree. But expression lambdas store their tracking
            // list in their parent's context. So, since we've decided to become an expression lambda, we also
            // have to re-house all those tracking variables.
            //
            // AND! e.g. "Sub() Call (Function(w) / Dim w = 1 / End Function)(1)"  [Dev10#694034]
            // The compiler has to warn that the local variable "w" shadows the lambda parameter "w".
            // It does this by storing a "TrackingLambdasForShadowing" list inside the Sub block context.
            // Since we're removing the block context, we have to rehouse this lest as well.

            if (backupContext.OldValue() == NULL)
            {
                // Dev10#670714:
#if IDE  || HOSTED
                // it's expected to get a lambda-without-context when the IDE interprets it out of context
#else
                VSASSERT(m_Semantics->IsVBEESemantics(), "unexpected: a lambda which lacks a context should never arise in normal compilation");
#endif
                // The IDE will call InterpretLambda when it needs to pretty-list, or when it needs to get a list
                // of local symbols for intellisense. In these cases it never bothers to set m_Semantics->m_BlockContext
                // (which in the command-line compiler would refer to block which contains the lambda). In these cases we don't
                // have anywhere to hoist the tracking variables! But it doesn't matter -- the IDE was only
                // using us for prettylisting/intellisense, and the "real" compilation which requires real variable-tracking
                // is done by the background compiler thread and it will have a correct m_BlockContext.
                // As for the hosted compiler, it only ever compiles expressions not Statements,
                // so it never needs to worry about this kind of shadowing. (I hope it doesn't
                // do statement lambdas either... should investigate whether that's an issue.)
            }
            else
            {
                CSingleListIter<TrackingVariablesForImplicitCollision> listIter(&m_Tree->TrackingVarsForImplicit);
                while (TrackingVariablesForImplicitCollision *current = listIter.Next())
                {
                    TrackingVariablesForImplicitCollision * newTrackedVar = new(m_Semantics->m_TreeAllocator) TrackingVariablesForImplicitCollision(
                        current->Name,
                        current->Location,
                        current->ErrorId);
                    backupContext.OldValue()->TrackingVarsForImplicit.InsertFirst(newTrackedVar);
                    // Question: why did we clone the trackedVar instead of just doing InsertFirst(current)?
                    // Answer: because "current" is a node in a linked list, and "current->m_next" from the old list
                    // makes it unsuitable to add into the new list without awkward patching up.
                }

                // Dev10#694034: and similarly copy the tracked lambdas list
                while (m_Tree->TrackingLambdasForShadowing && !(m_Tree->TrackingLambdasForShadowing->Count() == 0) )
                {
                    if (backupContext.OldValue()->TrackingLambdasForShadowing == NULL)
                    {
                        backupContext.OldValue()->TrackingLambdasForShadowing = new (m_Semantics->m_TreeStorage) Queue<ILTree::LambdaExpression*, NorlsAllocWrapper>(&m_Semantics->m_TreeStorage);
                    }

                    ILTree::LambdaExpression *pLambdaExpression = m_Tree->TrackingLambdasForShadowing->PopOrDequeue();
                    backupContext.OldValue()->TrackingLambdasForShadowing->PushOrEnqueue(pLambdaExpression);
                }
            }

            return m_Tree->Child->AsStatementWithExpression().Operand1;
        }
    }

    // Dev10 #830534: Make sure the tree is marked as bad if it contains bad nodes.
    if (!IsBad(m_Tree))
    {
        BoundTreeBadnessVisitor visitor;
        visitor.Visit(m_Tree);
        if (visitor.TreeIsBad())
        {
            MakeBad(m_Tree);
        }
    }

    return m_Tree;
}

ILTree::LambdaExpression *
Semantics::InterpretUnboundLambda
(
    _In_ ILTree::UnboundLambdaExpression *UnboundLambda,
    Type * ReturnTypeOfLambda, /*= NULL*/
    _Inout_opt_ DelegateRelaxationLevel *RelaxationLevel, /*= NULL*/
    bool inferringLambda,
    _Out_opt_ bool * pAsyncLacksAwaits
)
{
    ILTree::LambdaExpression * boundLambda = NULL;
    LambdaBodyInterpretNode * pBodyInterpreter = NULL;

            
    if (UnboundLambda->IsStatementLambda)
    {
    
        // We come here for single-line sub lambdas (which aren't expression-like)
        // and for single-line async function lambdas (which have a "return" implicitly inserted)
        // and for multi-line function and sub lamdbdas

        pBodyInterpreter = new (zeromemory) LambdaBodyInterpretStatement ( 
            this,
            UnboundLambda->GetLambdaStatement(),
            UnboundLambda->InterpretBodyFlags,
            UnboundLambda->IsFunctionLambda,
            UnboundLambda->IsSingleLine,
            UnboundLambda->IsAsyncKeywordUsed,
            UnboundLambda->IsIteratorKeywordUsed,
            UnboundLambda->ExactlyMatchExpressionResultType ? TypeHelpers::GetVoidType() : ReturnTypeOfLambda); // Don't allow conversions in the body if we have match exactly.

        boundLambda = CreateLambdaExpression(UnboundLambda->FirstParameter, 
                                             UnboundLambda->Loc,
                                             UnboundLambda->BodySpan,
                                             UnboundLambda->ResultType, 
                                             pBodyInterpreter, 
                                             UnboundLambda->IsFunctionLambda,
                                             UnboundLambda->IsAsyncKeywordUsed,
                                             UnboundLambda->IsIteratorKeywordUsed,
                                             UnboundLambda->IsFunctionLambda ? MultiLineFunctionLambda : MultiLineSubLambda,
                                             UnboundLambda->ExactlyMatchExpressionResultType ? TypeHelpers::GetVoidType() : ReturnTypeOfLambda,
                                             inferringLambda);

        // SINGLESUB: There's no guarantee that, just because we put an UnboundLambda->IsStatementLambda into the function,
        // that we'll get a boundLambda->IsStatementLambda out. That's because the body interpreter is at liberty
        // to return an expression representation of UnboundLambda->GetLambdaStatement(), if it can be expressed as
        // an expression, e.g. "Sub() Console.WriteLine("hello")". It does this because only expression lambdas
        // can be turned into expression trees.
        //
        // Note: in the above call to CreateLambdaExpression, we passed either MultiLineFunctionLambda or MultiLineSubLambda
        // as our initial guess. The only effect of that is that CreateLambdaExpression will allocate long-lived temporaries
        // in the temporary manager for the lambda statement body. Well, if it did, then the above trick (returning an expression)
        // is suppressed.

        // Output the relaxation level we are going to apply to the result type
        // so overload resolution can pick the best match.
        if (RelaxationLevel && ReturnTypeOfLambda && !ReturnTypeOfLambda->IsBad())
        {
            // Dev10#524269: for multiline lambdas, this relaxation level is initiazed to RelaxationLevelNone
            // at the start of LambdaBodyInterpretStatement::DoInterpretBody, called by CreateLambdaExpression.
            // As it interprets the body, and interprets return statements, it is updated.
            *RelaxationLevel = pBodyInterpreter->m_ReturnRelaxationLevel;
            // Microsoft 2009.06.07 -- this is likely a bug. We should be doing RelaxationLevel = max(RelaxationLevel, m_ReturnRelaxationLevel)
            // All the way down from MatchArguments through to ConvertWithErrorChecking, people only ever do max(..) on RelaxationLevel;
            // they don't reset it.

            // ASYNC: This mostly correctly deals with return relaxation level for async lambdas.
            // That's because m_ReturnRelaxationLevel is set by StatementSemantics::InterpretReturnExpression
            // And it itself already digs into the "T" of a "Task(Of T)" return type in order to judge,
            // and likewise into the "T" of a "IEnumerable(Of T)" yield type in order to judge.
            // It's also correct for single-line async function expression lambdas, since the parser
            // turns them into statement lambdas.

            // Dev11#94373: we also need to track whether there were any returns with operands, since
            // turning "Async Function() : End Function" into a Func(Of Task(Of Integer)) is a delegate
            // relaxation.
            if (!pBodyInterpreter->m_SeenReturnWithOperand &&
                UnboundLambda->IsAsyncKeywordUsed &&
                GetFXSymbolProvider()->IsTypeAvailable(FX::TaskType) &&
                !TypeHelpers::EquivalentTypes(ReturnTypeOfLambda, GetFXSymbolProvider()->GetType(FX::TaskType)))
            {
                *RelaxationLevel = max(*RelaxationLevel, DelegateRelaxationLevelWideningDropReturnOrArgs);
            }

            
        }
    }
    else 
    {
        // A single-line expression lambda, either non-async-function or a compiler-synthesized expression-like sub:

        pBodyInterpreter = new (zeromemory) LambdaBodyInterpretExpression (
            this,
            UnboundLambda->GetLambdaExpression(),
            UnboundLambda->IsAsyncKeywordUsed,
            UnboundLambda->InterpretBodyFlags,
            UnboundLambda->ExactlyMatchExpressionResultType ? TypeHelpers::GetVoidType() : ReturnTypeOfLambda); // Don't allow conversions in the body if we have match exactly.

        boundLambda = CreateLambdaExpression(
                                             UnboundLambda->FirstParameter, 
                                             UnboundLambda->Loc, 
                                             Location::GetInvalidLocation(), // BodySpan
                                             UnboundLambda->ResultType, 
                                             pBodyInterpreter, 
                                             UnboundLambda->IsFunctionLambda,
                                             UnboundLambda->IsAsyncKeywordUsed,
                                             UnboundLambda->IsIteratorKeywordUsed,
                                             SingleLineLambda,
                                             NULL, // This is a single line lambda
                                             inferringLambda);

        if (UnboundLambda->IsIteratorKeywordUsed)
        {
            ReportSemanticError(ERRID_BadIteratorExpressionLambda, UnboundLambda->Loc);
            MakeBad(boundLambda);
        }
        if (UnboundLambda->IsAsyncKeywordUsed)
        {
            VSFAIL("Error: single-line async function lambdas should be represented as statement lambdas");
            ReportSemanticError(ERRID_InternalCompilerError, UnboundLambda->Loc);
            MakeBad(boundLambda);
        }


        // Output the relaxation level we are going to apply to the result type
        // so overload resolution can pick the best match.

        Type *pOriginalResultType = pBodyInterpreter->GetOriginalResultType();
        ReturnTypeOfLambda = boundLambda->GetExpressionLambdaReturnType();
        // e.g. expression                                     pOriginalResultType  ReturnTypeOfLambda
        //   CType(Function() 1, Func(Of Double))                Integer              Double
        //   CType(Function() AddressOf Main, Func(Of Action))   Action               Action
        //
        // Q. Why do we reassign "TargetTypeForReturnExpressions" rather than using the one that was given to us?
        // A. I don't know.

        if (RelaxationLevel && ReturnTypeOfLambda && !ReturnTypeOfLambda->IsBad())
        {
            Type *pExpectedReturnType = ReturnTypeOfLambda;
            MethodConversionClass ReturnTypeConversion = MethodConversionIdentity;
            ClassifyReturnTypeForMethodConversion(pOriginalResultType,  ReturnTypeOfLambda, ReturnTypeConversion);
            *RelaxationLevel = max(DetermineDelegateRelaxationLevel(ReturnTypeConversion), *RelaxationLevel);
        }

    }

    boundLambda->ExactlyMatchExpressionResultType = UnboundLambda->ExactlyMatchExpressionResultType;

    if (pAsyncLacksAwaits != NULL) *pAsyncLacksAwaits = (UnboundLambda->IsAsyncKeywordUsed && !pBodyInterpreter->m_SeenAwait);

    delete pBodyInterpreter;

    return boundLambda;
}


ILTree::LambdaExpression *
Semantics::CreateLambdaExpression
(
    _In_opt_ Parameter * FirstParameter,
    Location & LambdaTextSpan,
    Location BodySpan,
    Type * LambdaResultType,
    LambdaBodyInterpreter * bodyInterpreter,
    bool IsFunctionLambda,
    bool IsAsyncKeywordUsed,
    bool IsIteratorKeywordUsed,
    LambdaKind lambdaKind,
    _In_opt_ Type *pTargetTypeOfMultilineLambdaBody,
    bool inferringLambda
)
{
    BackupValue<Scope *> Lookup_backup(&m_Lookup);
    SymbolList ListOfBuiltLocals;

    NorlsAllocator QueryMemberLookup(NORLSLOC);
    int addedToScope=0;
    bool IsQueryLambda=m_InQuery;
    bool IsCurrentQueryLambda = false;
    bool ShadowsLocalSymbol = false;
    BackupValue<bool> UseQueryNameLookup_backup(&m_UseQueryNameLookup);

    for (Parameter *LambdaParam = FirstParameter;
         LambdaParam;
         LambdaParam = LambdaParam->GetNext())
    {
        BCSYM_Variable *ParamLocal = m_SymbolCreator.AllocVariable( false, false );
        STRING* ParameterName = LambdaParam->GetName();
        Location& ParameterLocation = *LambdaParam->GetLocation();

        m_SymbolCreator.GetVariable(NULL,
                                    ParameterName,
                                    ParameterName,
                                    DECLF_Dim | DECLF_Public,
                                    VAR_Param,
                                    LambdaParam->GetType(),
                                    NULL, // don't send initializer info
                                    &ListOfBuiltLocals,
                                    ParamLocal );

        ParamLocal->SetIsLambdaMember(true);

        // BEGIN add this var into scope to lookup members within a Query
        if(LambdaParam->IsQueryIterationVariable())
        {
            IsQueryLambda = true;
            IsCurrentQueryLambda = true;

            AssertIfTrue(ParamLocal->IsQueryRecord());
            ParamLocal->SetIsQueryRecord(LambdaParam->IsQueryRecord());

            // only "records" are going to the list
            if(ParamLocal->IsQueryRecord())
            {
                QueryMemberLookupVariable * lookupVar = new (QueryMemberLookup) QueryMemberLookupVariable;

                lookupVar->IterationVariable = ParamLocal;

                // insert first since it is easier to remove from the beginning
                m_QueryMemberLookupList.InsertFirst(lookupVar);
                addedToScope++;
            }

            m_UseQueryNameLookup = true;
        }
        // END add this var into scope to lookup members within a Query
        else if (!LambdaParam->IsRelaxedDelegateVariable())
        {
            // Check to see if variable is shadowing an existing variable in scope.
            Symbol *NameBinding=NULL;
            if (!CheckNameForShadowingOfLocals(
                ParameterName,
                ParameterLocation,
                ERRID_LambdaParamShadowLocal1,
                true /*DeferForImplicitDeclarations*/))
            {
                ShadowsLocalSymbol = true;
            }
        }
    }
    
    m_Lookup =
        m_SymbolCreator.GetHashTable(
            NULL,
            m_Lookup,
            true,
            2,
            &ListOfBuiltLocals);

    m_Lookup->SetIsLambdaHash(true);

    // If we ever don't create a temporary manager for a lambda (unlikely), you'll need
    // to modify the anonymous types to not key off the temporary manager as an indication
    // of whether we are in the same lambda or not.

    BackupValue<TemporaryManager *> TemporaryManager_backup(&m_TemporaryManager);

    m_TemporaryManager =
        new(m_TreeAllocator)
            TemporaryManager(
                m_Compiler,
                m_CompilerHost,
                m_Procedure,
                &m_TreeStorage
#if IDE
                , NULL
                , true      // TemporaryManager for a lambda
#endif IDE
                );

    BackupValue<LambdaKind> backupInLambda(&m_InLambda);
    m_InLambda = lambdaKind;

    BackupValue<bool> backupInQuery(&m_InQuery);
    m_InQuery = IsQueryLambda;

    ILTree::ILNode *BoundExpression = bodyInterpreter->InterpretBody(&m_SymbolCreator);       

    backupInQuery.Restore();
    backupInLambda.Restore();

    AssertIfFalse(addedToScope>=0);
    AssertIfFalse((unsigned)addedToScope <= m_QueryMemberLookupList.NumberOfEntries());
    while(addedToScope>0)
    {
        m_QueryMemberLookupList.Remove(m_QueryMemberLookupList.GetFirst());
        addedToScope--;
    }

    UseQueryNameLookup_backup.Restore();

    ILTree::LambdaExpression *Result = NULL;

    // Method has a lambda expression.  Set the flag so closures code will run
    m_methodHasLambda = true;

    if (TypeHelpers::IsVoidType(LambdaResultType) &&
        !IsCurrentQueryLambda)
    {
        Type *pResultOfExpression = NULL;
        // e.g. in VB$Func<T>, this pResultOfExpression is the "T"

        if (BoundExpression->bilop == SB_STATEMENT_LAMBDA)
        {
            pResultOfExpression = pTargetTypeOfMultilineLambdaBody;
        }
        else if (IsFunctionLambda)
        {
            pResultOfExpression = BoundExpression->AsExpression().ResultType;
        }
        else
        {
            pResultOfExpression = TypeHelpers::GetVoidType();
        }

        LambdaResultType = GetInstantiatedAnonymousDelegate(FirstParameter, pResultOfExpression, IsFunctionLambda, LambdaTextSpan);
    }

    Result = &m_TreeAllocator.xAllocBilNode(SX_LAMBDA)->AsLambdaExpression();

    if (BoundExpression->bilop == SB_STATEMENT_LAMBDA)
    {
        Result->SetStatementLambdaBody( &(BoundExpression->AsStatementLambdaBlock()) );
        BoundExpression->AsStatementLambdaBlock().pOwningLambdaExpression = Result;
        Result->BodySpan = BodySpan;
    }
    else
    {
        Result->SetExpressionLambdaBody( &(BoundExpression->AsExpression()) );
        Result->BodySpan = Location::GetInvalidLocation();
    }

    Result->IsFunctionLambda = IsFunctionLambda;
    Result->IsAsyncKeywordUsed = IsAsyncKeywordUsed;
    Result->IsIteratorKeywordUsed = IsIteratorKeywordUsed;
    Result->Loc = LambdaTextSpan;
    Result->FirstParameter = FirstParameter;
    Result->SetLocalsHash(m_Lookup);
    Result->TemporaryManager = m_TemporaryManager;
    Result->ResultType = LambdaResultType; // the exact delegate type may be not known yet
    Result->vtype = t_ref;
    Result->IsPartOfQuery = IsCurrentQueryLambda;
    Result->IsRelaxedDelegateLambda = false;
    Result->m_ResumableKind = bodyInterpreter->GetResumableKind();
    Result->m_ResumableGenericType = bodyInterpreter->GetResumableGenericType();
    Result->IsConvertedFromAsyncSingleLineFunction = 
        IsAsyncKeywordUsed && 
        IsFunctionLambda &&
        ((LambdaBodyInterpretStatement*)bodyInterpreter)->IsSingleLine();

    // We only want to track the lambdas that are interpretted properly, not the lambdas that
    // are being used to infer the lambdas return type.
    //
    // Dev10#709831: actually the "inferringLambda" flag isn't authoritative. e.g.
    // "Dim f = Function() Function() / return 1 / End Function" then even though we're interpreting
    // the outermost function lambda, it doesn't percolate the flag "inferringLambda" all the way through
    // the callstack. Instead, a more reliable flag is "this->m_ReportErrors": it works because
    // it gets turned off during inference; and it's appropriate because the "TrackingLambdasForShadowing"
    // datastructure exists solely for the purposes of reporting errors (even though they're actually
    // reported later on in CheckChildrenForBlockLevelShadowing).
    if (m_BlockContext && !inferringLambda && this->m_ReportErrors)
    {
        if (!m_BlockContext->TrackingLambdasForShadowing)
        {
            m_BlockContext->TrackingLambdasForShadowing = new (m_TreeStorage) Queue<ILTree::LambdaExpression*, NorlsAllocWrapper>(&m_TreeStorage);
        }
    
        m_BlockContext->TrackingLambdasForShadowing->PushOrEnqueue(Result);
    }
    
    Lookup_backup.Restore();
    TemporaryManager_backup.Restore();

    if (IsBad(BoundExpression))
    {
        MakeBad(Result);
    }

    return Result;
}


ILTree::Expression *
Semantics::ConvertToDelegateType
(
    _In_ ILTree::LambdaExpression *Lambda,
    Type *DelegateType,
    bool ConvertToDelegateReturnType,
    DelegateRelaxationLevel &RelaxationLevel,
    _Out_opt_ bool *pRequiresNarrowingConversionForReturnValue,
    _Out_opt_ bool *pNarrowingFromNumericLiteral
)
{
    if(pRequiresNarrowingConversionForReturnValue)
    {
        *pRequiresNarrowingConversionForReturnValue = false;
    }

    if(pNarrowingFromNumericLiteral)
    {
        *pNarrowingFromNumericLiteral = false;
    }

    if(!TypeHelpers::IsVoidType(Lambda->ResultType) && !Lambda->ResultType->IsAnonymousDelegate())
     {
        // we already have delegate type for this Lambda
        if(TypeHelpers::EquivalentTypes(Lambda->ResultType, DelegateType))
        {
            FixLambdaExpressionForClosures(Lambda);
            return Lambda;
        }
        else
        {
            goto Mismatch;
        }
    }

    Declaration *InvokeMethod = GetInvokeFromDelegate(DelegateType, m_Compiler);

    if (InvokeMethod == NULL || IsBad(InvokeMethod) || !InvokeMethod->IsProc())
    {
        if (!TypeHelpers::IsDelegateType(DelegateType) ||
            TypeHelpers::IsStrictSupertypeOfConcreteDelegate(DelegateType, GetFXSymbolProvider())) // covers Object, System.Delegate, System.MulticastDelegate
        {
             ReportSemanticError(
                DelegateType == GetFXSymbolProvider()->GetDelegateType() ||
                    DelegateType == GetFXSymbolProvider()->GetMultiCastDelegateType() ?
                        ERRID_LambdaNotCreatableDelegate1 :
                        ERRID_LambdaNotDelegate1,
                Lambda->Loc,
                DelegateType);

            return AllocateBadExpression(Lambda->Loc);
        }

Mismatch:
        Semantics::ReportLambdaBindingMismatch(DelegateType, Lambda->Loc, Lambda->IsFunctionLambda);

        return AllocateBadExpression(Lambda->Loc);
    }

    GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(DelegateType) ? DelegateType->PGenericTypeBinding() : NULL;

	Parameter *DelegateParam, *LambdaParam;
    // Check the parameter types.
    for (DelegateParam = InvokeMethod->PProc()->GetFirstParam(),
            LambdaParam = Lambda->FirstParameter;
         DelegateParam && LambdaParam;
         DelegateParam = DelegateParam->GetNext(),
            LambdaParam = LambdaParam->GetNext())
    {
        if (TypeHelpers::IsBadType(DelegateParam->GetType()))
        {
            ReportBadType(DelegateParam->GetType(), Lambda->Loc);
            return AllocateBadExpression(Lambda->Loc);
        }

        // Get the type of the parameter
        Type *DelegateParamType = GetDataType(DelegateParam);

        DelegateParamType = ReplaceGenericParametersWithArguments(DelegateParamType, DelegateBindingContext, m_SymbolCreator);

        Type *LambdaParamType = LambdaParam->GetType();


        if (!LambdaParamType)
        {
            goto Mismatch;
        }

        if(TypeHelpers::IsBadType(LambdaParamType))
        {
            ReportBadType(LambdaParamType, Lambda->Loc);
            return AllocateBadExpression(Lambda->Loc);
        }

        if (!TypeHelpers::EquivalentTypes(LambdaParamType, DelegateParamType))
        {
            goto Mismatch;
        }
    }

    // If we didn't have the same number of parameters, then we can't match.
    if (DelegateParam || LambdaParam)
    {
        goto Mismatch;
    }

    // create a copy since we are going to modify this node
    Lambda = m_TreeAllocator.xShallowCopyLambdaTree(Lambda);

    if(ConvertToDelegateReturnType)
    {
        Type *DelegateReturnType = InvokeMethod->PProc()->GetType();

        if(!DelegateReturnType)
        {
            goto Mismatch;
        }

        if (TypeHelpers::IsBadType(DelegateReturnType))
        {
            ReportBadType(DelegateReturnType, Lambda->Loc);
            return AllocateBadExpression(Lambda->Loc);
        }

        DelegateReturnType = ReplaceGenericParametersWithArguments(DelegateReturnType, DelegateBindingContext, m_SymbolCreator);

        if(!TypeHelpers::EquivalentTypes(Lambda->GetExpressionLambdaBody()->ResultType, DelegateReturnType))
        {
            if(Lambda->ExactlyMatchExpressionResultType)
            {
                goto Mismatch;
            }

            ExpressionFlags ConvertResultTypeFlags = Lambda->ConvertResultTypeFlags;

            // #28762
            if(HasFlag(ConvertResultTypeFlags, ExprIsOperandOfConditionalBranch) &&
                DelegateReturnType != GetFXSymbolProvider()->GetBooleanType())
            {
                ClearFlag(ConvertResultTypeFlags, ExprIsOperandOfConditionalBranch);
            }

            Parameter *CopyBackConversionParam = NULL;
            bool RequiresNarrowingConversion = false;
            bool NarrowingFromNumericLiteral = false;
            DelegateRelaxationLevel ConversionRelaxationLevel = DelegateRelaxationLevelNone;
            bool RequiresUnwrappingNullable = false;

            BackupValue<Scope *> Lookup_backup(&m_Lookup);
            BackupValue<TemporaryManager *> TemporaryManager_backup(&m_TemporaryManager);

            m_Lookup = Lambda->GetLocalsHash();
            m_TemporaryManager = Lambda->TemporaryManager;

            //undone : Microsoft - extmet DCR
            //DO NOT 


            ILTree::Expression *BoundExpression =
                ConvertWithErrorChecking
                (
                    Lambda->GetExpressionLambdaBody(),
                    DelegateReturnType,
                    ConvertResultTypeFlags,
                    CopyBackConversionParam,
                    RequiresNarrowingConversion,
                    NarrowingFromNumericLiteral,
                    false, /*SuppressMethodNameInErrorMessages*/
                    ConversionRelaxationLevel,
                    RequiresUnwrappingNullable
                );

            // Bug 35827 - DevDiv Bugs
            if(pRequiresNarrowingConversionForReturnValue)
            {
                *pRequiresNarrowingConversionForReturnValue = RequiresNarrowingConversion;
            }

            // Bug 68422 - DevDiv Bugs
            if(pNarrowingFromNumericLiteral)
            {
                *pNarrowingFromNumericLiteral = NarrowingFromNumericLiteral;
            }

            if(IsBad(BoundExpression))
            {
                Lambda->SetExpressionLambdaBody(BoundExpression);
                MakeBad(Lambda);
            }
            else
            {
                Lambda->SetExpressionLambdaBody(MakeRValue(BoundExpression));
            }
        }
    }

    Lambda->ResultType = DelegateType;
    Lambda->vtype = DelegateType->GetVtype();

    FixLambdaExpressionForClosures(Lambda);

    return Lambda;

}


void
Semantics::ReportLambdaBindingMismatch
(
    Type *DelegateType,
    const Location &Loc,
    bool isFunctionLambda,
    bool WouldHaveMatchedIfOptionStrictWasOff // = false
)
{
    if (m_ReportErrors)
    {
        StringBuffer DelegateRepresentation;
        DelegateType->GetBasicRep(m_Compiler, m_Procedure == NULL ? NULL : m_Procedure->GetContainingClass(), &DelegateRepresentation, NULL);

        unsigned lambdaBindingMismatchError = isFunctionLambda ? ERRID_LambdaBindingMismatch1 : ERRID_LambdaBindingMismatch2;

        ReportSemanticError(
            WouldHaveMatchedIfOptionStrictWasOff ? ERRID_LambdaBindingMismatchStrictOff1 : lambdaBindingMismatchError,
            Loc,
            DelegateRepresentation);
    }
}

void 
Semantics::ReportLambdaParameterInferredToBeObject
(
    Parameter* pLambdaParam
)
{

    VSASSERT( pLambdaParam != NULL,
                      "Input to ReportLambdaParameterInferredToBeObject is invalid");
    
    if ( m_SourceFile->GetOptionFlags() & OPTION_OptionStrict )
    {
        ReportSemanticError(ERRID_StrictDisallowImplicitObjectLambda, *(pLambdaParam->GetLocation()));
    }
    else
    {
        StringBuffer buf;
        ReportSemanticError(WRNID_ObjectAssumedVar1, *(pLambdaParam->GetLocation()), ResLoadString(WRNID_MissingAsClauseinVarDecl, &buf));
    }

}

ILTree::Expression *
Semantics::CreateRelaxedDelegateLambda
(
    ILTree::Expression* MethodOperand,
    ILTree::Expression* ObjectArgument,
    Procedure *TargetMethod,   // e.g. "Dim x as InvokeMethod_Delegate = AddressOf TargetMethod"
    Procedure *InvokeMethod,
    Type *DelegateType,
    BCSYM_GenericBinding *GenericBindingContext,
    const Location &Location,
    bool CallIsExtensionMethod
)
{
    ParserHelper PH(&m_TreeStorage, Location);
    bool CallIsLateBound = IsLateReference(MethodOperand);
    //bool CallIsExtensionMethod = MethodOperand->bilop == SX_EXTENSION_CALL;

    GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(DelegateType) ? DelegateType->PGenericTypeBinding() : NULL;
    BCSYM_Param* ParameterList = CopyParameterList(
        InvokeMethod,
        DelegateBindingContext,
        true /*Rename the parameters to be unique.*/);

    ParseTree::ArgumentList *InvokeArguments = NULL;
    if (CallIsLateBound)
    {
        VSASSERT(TargetMethod == NULL, "Not expecting a target method for late bound scenario");
        InvokeArguments = Semantics::CreateArgumentList(
            ParameterList,
            NULL,
            true, /*ForceCopyAllArguments*/
            Location);
    }
    else if (CallIsExtensionMethod)
    {
        VSASSERT(TargetMethod != NULL, "Must have a targetmethod.");
        InvokeArguments = Semantics::CreateArgumentList(
            ParameterList,
            TargetMethod->GetFirstParam()->GetNext(), // first parameter in target signature,
            false, /*ForceCopyAllArguments*/
            Location);
    }
    else
    {
        VSASSERT(TargetMethod != NULL, "Must have a targetmethod.");
        InvokeArguments = Semantics::CreateArgumentList(
            ParameterList,
            TargetMethod->GetFirstParam(), // first parameter in target signature,
            false, /*ForceCopyAllArguments*/
            Location);
    }

    unsigned SeqOp2Flags = 0;
    ILTree::Expression *TemporaryAssignment = NULL;
    ILTree::Expression *MethodReference = NULL;
    if (CallIsLateBound)
    {
        // Must have late reference passed in
        ThrowIfFalse(MethodOperand->bilop == SX_LATE_REFERENCE);

        LateReferenceExpression &LateReference = MethodOperand->AsPropertyReferenceExpression();
        ILTree::LateBoundExpression &LateCall = LateReference.Left->AsLateBoundExpression();

        MethodReference = MethodOperand;
    }
    else
    {
        ILTree::Expression* ExpressionToInvokeTargetMethodOn = NULL;
        if (CallIsExtensionMethod)
        {
            ExpressionToInvokeTargetMethodOn = NULL;

            // Port SP1 CL 2954860 to VS10
            // Microsoft:
            // fix for 163410: ObjectArgument is passed in, so we shouldn't dig it out.
            // ObjectArgument = MethodOperand->sxExtensionCall().ImplicitArgumentList->sxOp().Left->sxArg().Left;

            Variable *TempObjectArgument = NULL;

            // Capture the extension method object in a temporary value to bind it in case it is a byref variable.
            // since we will generate a closure, this prevents our object from being lifted.
            TemporaryAssignment = CaptureInShortLivedTemporary(ObjectArgument, TempObjectArgument);

            ILTree::Expression* FirstArgumentExpressino = AllocateSymbolReference(
                    TempObjectArgument,
                    ObjectArgument->ResultType,
                    NULL,
                    Location);

            ParseTree::ArgumentList * ArgListElement = new(m_TreeStorage) ParseTree::ArgumentList;
            ArgListElement->TextSpan = Location;
            ArgListElement->Element = new(m_TreeStorage) ParseTree::Argument;
            ArgListElement->Element->TextSpan = Location;
            ArgListElement->Element->Value = PH.CreateBoundExpression(FirstArgumentExpressino);

            ArgListElement->Next = InvokeArguments;
            InvokeArguments = ArgListElement;
            // Setting this flag indicates that ConvertSeqOp in ExpressionTreeSemantics assumes the structure is
            // what this generates. If you modify the bound tree created here, please update that logic respectively
            SeqOp2Flags = SXF_SEQ_OP2_RELAXED_LAMBDA_EXT;
        }
        else if (TargetMethod->IsShared())
        {
            ExpressionToInvokeTargetMethodOn = NULL;
        }
        // Port SP1 CL 2955581 to VS10
        else if (IsMeReference(ObjectArgument) && 
                    !TypeHelpers::IsValueType(ObjectArgument->ResultType)) // Bug #168248 - DevDiv Bugs: Should capture by value to mimic boxing behavior for non-relaxed delegates.
        {
            // Optimization, no need to create temp for Me reference.
            // So no closure code needed. We do need to for MyBase and MyClass
            ExpressionToInvokeTargetMethodOn = ObjectArgument;
        }
        else
        {
            Variable *TempObjectArgument = NULL;

            TemporaryAssignment = CaptureInShortLivedTemporary(ObjectArgument, TempObjectArgument);

            ExpressionToInvokeTargetMethodOn = AllocateSymbolReference(
                    TempObjectArgument,
                    ObjectArgument->ResultType,
                    NULL,
                    Location);
            // Setting this flag indicates that ConvertSeqOp in ExpressionTreeSemantics assumes the structure is
            // what this generates. If you modify the bound tree created here, please update that logic respectively
            SeqOp2Flags = SXF_SEQ_OP2_RELAXED_LAMBDA_INST;
        }

        MethodReference = ReferToSymbol(
                    Location,
                    TargetMethod,
                    chType_NONE,
                    ExpressionToInvokeTargetMethodOn,
                    GenericBindingContext,
                    ExprIsExplicitCallTarget | ExprSuppressMeSynthesis);
    }

    ParseTree::Expression* CallHandler = PH.CreateMethodCall(
            PH.CreateBoundExpression(MethodReference),
            InvokeArguments);

    // Build the lambda expression
    ILTree::UnboundLambdaExpression *UnboundLambda = NULL;
    UnboundLambda = &m_TreeAllocator.xAllocBilNode(SX_UNBOUND_LAMBDA)->AsUnboundLambdaExpression();
    UnboundLambda->Loc = Location;
    UnboundLambda->FirstParameter = ParameterList;
    UnboundLambda->ResultType = DelegateType;
    UnboundLambda->IsFunctionLambda = IsFunction(InvokeMethod);
    UnboundLambda->IsStatementLambda = false;
    UnboundLambda->IsSingleLine = true;
    UnboundLambda->SetLambdaExpression( CallHandler );
    UnboundLambda->vtype = t_ref;
    UnboundLambda->InterpretBodyFlags = ExprSkipOverloadResolution | ExprDontInferResultType;
    UnboundLambda->AllowRelaxationSemantics = true;

    DelegateRelaxationLevel StubbedRelaxationLevel = DelegateRelaxationLevelNone;

    VSASSERT(!m_IsCreatingRelaxedDelegateLambda, "Why is it nestly creating relaxed delegate lambda");
    m_IsCreatingRelaxedDelegateLambda = true;

    ILTree::Expression* Lambda =  InterpretUnboundLambdaBinding(
        UnboundLambda,
        DelegateType,
        true, // ConvertToDelegateReturnType
        StubbedRelaxationLevel);

    m_IsCreatingRelaxedDelegateLambda = false;	
    Lambda->AsLambdaExpression().IsRelaxedDelegateLambda = true;

    if (TemporaryAssignment != NULL)
    {
        ILTree::Expression* SeqOp2 = AllocateExpression(
            SX_SEQ_OP2,
            DelegateType,
            TemporaryAssignment,
            Lambda,
            Location);
        SetFlag32(SeqOp2, SeqOp2Flags);

        return SeqOp2;
    }
    else
    {
        return Lambda;
    }
}


//========================================================================================================================================
// ConvertAnonymousDelegateToOtherDelegate
//
// WARNING!!! This function's name is unrelated to its behavior. It should be refactored and renamed.
// It can be called in cases where the input is a named rather than an anonymous delegate (see Dev10#691957)
// and also its behavior (1) should be put into a separate function.
//
// (1) If it's been given an <Input> which is a named function-line lambda that's already of the correct
//     TargetType, then it just returns that directly (first turning its body into a SEQ_OP2 where the second
//     operation is NULL if it happened to be a single-line expression lambda: I don't know why).
// (2) Otherwise, regardless of whether <Input> is named or anonymous, it constructs the expression
//     "AddressOf <Input>.Invoke" and calls ConvertWithErrorChecking on that.
//========================================================================================================================================
ILTree::Expression*
Semantics::ConvertAnonymousDelegateToOtherDelegate
(
    _In_ ILTree::Expression*     Input,
    Type*           TargetType
)
{
    VSASSERT(Input != NULL, "Must have input");
    VSASSERT(Input->ResultType != NULL, "Must have result type for the input");
    VSASSERT(TargetType != NULL, "Must have target type to convert to.");
    VSASSERT(
        TypeHelpers::IsDelegateType(TargetType) && TargetType != GetFXSymbolProvider()->GetMultiCastDelegateType(),
        "Should have proper delegate type as argument.");

    Declaration* InvokeMethod = GetInvokeFromDelegate(Input->ResultType, m_Compiler);
    VSASSERT(InvokeMethod != NULL && InvokeMethod->IsProc(), "Not a proper delegate type.");

    // Microsoft:
    // We have to do something similar to ConvertToDelegateType here, where we check that
    // the delegate type against the lambda type. This is cause ConvertWithErrorChecking
    // may call us.

    if(
        Input->bilop == SX_LAMBDA &&
        !TypeHelpers::IsVoidType(Input->ResultType) &&
        !Input->ResultType->IsAnonymousDelegate())
    {
        // we already have delegate type for this Lambda
        if(TypeHelpers::EquivalentTypes(Input->ResultType, TargetType))
        {
            FixLambdaExpressionForClosures(&Input->AsLambdaExpression());
            return Input;
        }
    }

    ParserHelper PH(&m_TreeStorage, Input->Loc);

    GenericTypeBinding *DelegateBindingContext = TypeHelpers::IsGenericTypeBinding(Input->ResultType) ? Input->ResultType->PGenericTypeBinding() : NULL;
    ILTree::Expression* MethodReference = ReferToSymbol(
                Input->Loc,
                InvokeMethod,
                chType_NONE,
                Input,
                DelegateBindingContext,
                ExprIsExplicitCallTarget | ExprSuppressMeSynthesis);

    ParseTree::Expression * AddressOfParseTree = PH.CreateAddressOf(PH.CreateBoundExpression(MethodReference), false);
    // Dev10#691957 - the "false" flag here means "If the conversion fails, then look at the current SourceFile
    // to see if it's Option Strict On or Off, and base error messages on that". If we passed "true" then it would
    // mean "If the conversion fails, then look at the source file which contained InvokeMethod to see if that
    // source file had Option Strict On or Off, and base error messages on that".
    // Here for lambdas we always want to base it on the current source file. Anyway, the InvokeMethod might
    // not even be in a source file! e.g. if InvokeMethod = Func(Of String).Invoke, then it'll be in a metadata assembly.
    // (Note: in the case where a class field initializer is in one file, but it gets slipstreamed into a constructor
    // from another file, well, Semantics::InitializeFields does magic to make "SourceFile" point to the right place.
    // So we needn't worry about that.)

    ILTree::Expression* AddressOfExpression = InterpretExpression(AddressOfParseTree, ExprNoFlags);

    return ConvertWithErrorChecking(AddressOfExpression, TargetType, ExprNoFlags);

}

ILTree::UnboundLambdaExpression *
Semantics::ShallowCloneUnboundLambdaExpression
(
    _In_ ILTree::UnboundLambdaExpression * Input
)
{
    ILTree::UnboundLambdaExpression *Result = &m_TreeAllocator.xAllocBilNode(SX_UNBOUND_LAMBDA)->AsUnboundLambdaExpression();
    memcpy(Result, Input, sizeof(ILTree::UnboundLambdaExpression));
    return Result;

    // Note: previously we just copied the fields one by one instead of using memcpy. But this was
    // fragile -- easy for someone to add a field but forget to update UnboundLambdaExpression.
    // That's why we switched to memcpy. However the memcpy version copies more than used to be copied before.
    //
    // The current memcpy implementation has different behavior from before because it also copies:
    //  * IsScratch. In most suites we used to see Input->IsScratch && !Result->IsScratch.
    //  * uFlags. In vbc\ArrayLiterals\Misc\LambdaParamShadowsLocal we used to see Input->uFlags==1 && Result->uFlags==0
    //
    // The current memcpy implementation also copies some extra fields that never used to be copied but which
    // (as far as I can tell) were always "0" in Input, so memcpy and failure-to-copy amount to the same thing.
    //  * ExactlyMatchExpressionResultType
    //  * LateBoundCallArgumentIndex
    //  * IsExplicitlyCast
    //  * NameCanBeType
    //  * ForcedLiftCatenationIIFCoalesce
}

// Will retreive from the cache or regreate the generic anonymous delegate type.
// It will not generate an instance with a generic typebinding. This will be
// AnonymousDelegate(Of TArg0, TArg1, TResult)
ClassOrRecordType *
Semantics::RetreiveAnonymousDelegate
(
    BCSYM_Class *ContainingClass,
    BCSYM_Param * LambdaParameters,
    Type * LambdaResultType,
    bool IsFunctionLambda,
    Location CreatingLocation
)
{
    DWORD AnonymousDelegateHash = GetAnonymousDelegateHashCode(LambdaParameters, IsFunctionLambda);

    TransientSymbolStore *Transients = m_SymbolsCreatedDuringInterpretation;
    if (m_SymbolsCreatedDuringInterpretation)
    {
        ClassOrRecordType** PossibleMatch = Transients->GetAnonymousDelegateHash()->HashFind(AnonymousDelegateHash);

        while (PossibleMatch && *PossibleMatch)
        {
            Declaration *PossibleInvoke = GetInvokeFromDelegate(*PossibleMatch, m_Compiler);

            if (MatchAnonymousDelegate(
                    PossibleInvoke->PProc()->GetFirstParam(),
                    PossibleInvoke->PProc()->GetRawType(),
                    (*PossibleMatch)->PClass()->GetSourceFile(),
                    PossibleInvoke->GetLocation(), 

                    LambdaParameters,
                    IsFunctionLambda,
                    m_SourceFile,
                    &CreatingLocation,

                    !m_MergeAnonymousTypeTemplates) &&
                (m_MergeAnonymousTypeTemplates || Transients->GetAnonymousDelegateHash()->IsUncommited(AnonymousDelegateHash, PossibleMatch)))
            {
                return *PossibleMatch;  // Found in hash table return.
            }

            PossibleMatch = Transients->GetAnonymousDelegateHash()->FindNextWithKey(AnonymousDelegateHash);
        }
    }

    ClassOrRecordType *AnonymousDelegate =
        ConstructAnonymousDelegateClass(
            LambdaParameters,
            LambdaResultType,
            IsFunctionLambda,
            CreatingLocation);

#if IDE
    if (AnonymousDelegate && !m_MergeAnonymousTypeTemplates)
    {
        AnonymousDelegate->SetIsNonMergedAnonymousType(true);
    }
#endif

    if (AnonymousDelegate && Transients)
    {
        Transients->GetAnonymousDelegateHash()->TransactHashAdd(AnonymousDelegateHash, AnonymousDelegate);
    }

    return AnonymousDelegate;
}


// May Return NULL !!!
Type*
Semantics::GetInstantiatedAnonymousDelegate
(
    _In_ ILTree::LambdaExpression *Lambda
)
{
    // Do not Infer Anonymous Delegate Type for Lamdas built by Queries
    if(Lambda->IsPartOfQuery)
    {
        return NULL;
    }

    return GetInstantiatedAnonymousDelegate(
                Lambda->FirstParameter,
                Lambda->GetExpressionLambdaBody()->ResultType,
                Lambda->IsFunctionLambda,
                Lambda->Loc);
}

// Will use RetreiveAnonymousDelegate to retreive the anonymous delegate from the cache or
// regreate the generic anonymous delegate type.
// It will then instantiate the generic binding. It receives AnonymousDelegate(Of TArg0, TArg1, TResult)
// and will make it AnonymousDelegate(Of String, Integer, Double) based on the lambda's parameters and returntype.
Type*
Semantics::GetInstantiatedAnonymousDelegate
(
    BCSYM_Param * LambdaParameters,
    Type * LambdaResultType,
    bool IsFunctionLambda,
    Location CreatingLocation
)
{
    ClassOrRecordType *AnonymousDelegate =
        RetreiveAnonymousDelegate(
            NULL,
            LambdaParameters,
            LambdaResultType,
            IsFunctionLambda,
            CreatingLocation);

    // Dev10#526518: If LambdaResultType==NULL, that might be because it's a function whose return type
    // hasn't yet been inferred. (actually, it would be an error to arrive where we are now if the
    // return type hasn't yet been inferred). If LambdaResultType->IsVoidType() then it might be a Sub,
    // or it might be a Function whose return type couldn't be inferred, e.g. "Function() AddressOf Main".
    // The correct way to judge whether it's a Sub is with "IsFunctionLambda".

    if ( LambdaParameters==NULL && !IsFunctionLambda )
    {
        // This Lambda Expression does not have any parameters or a return type.
        // Thus it must be a Sub() that doesn't take any arguments.
        // We should return the anonymous delegate as the type in this case,
        // instead of wrapping it in a GenericBinding.

        //We must register this anonymous delegate here because normally it is registered together with the
        // generic binding. Since we don't have a generic binding in this case, we register the delegate here.

        RegisterAnonymousDelegateTransientSymbolIfNotAlreadyRegistered(AnonymousDelegate);
        return AnonymousDelegate;
    }


    GenericBinding *OpenBinding =
        SynthesizeOpenGenericBinding(
            AnonymousDelegate,
            m_SymbolCreator);

    BCSYM_Param* CurrentParam = LambdaParameters;
    unsigned int currentArgument = 0;
    while (CurrentParam != NULL)
    {
        // Dig through ByRef.
        Type* CurrentType = GetDataType(CurrentParam);
        if (CurrentType == NULL)
        {
            CurrentType = GetFXSymbolProvider()->GetObjectType();
        }
        if (CurrentParam->IsByRefKeywordUsed() && CurrentType->IsPointerType())
        {
            CurrentType = CurrentType->PPointerType()->GetRoot();
        }

        OpenBinding->GetArguments()[currentArgument] = CurrentType;

        currentArgument++;
        CurrentParam = CurrentParam->GetNext();
    }

    if (IsFunctionLambda)
    {
        if (LambdaResultType && !LambdaResultType->IsVoidType()) // #150394 Don't replace with the Void type
        {
            OpenBinding->GetArguments()[currentArgument] = LambdaResultType;
        }
        else
        {
            // Dev10#526518: but we have to replace it with SOMETHING, otherwise we'll report ----
            // generic-param-names to the user
            OpenBinding->GetArguments()[currentArgument] = GetCompilerHost()->GetFXSymbolProvider()->GetObjectType();
        }
    }

    return m_SymbolCreator.GetGenericBindingWithLocation(
            false,
            AnonymousDelegate,
            OpenBinding->GetArguments(),
            OpenBinding->GetArgumentCount(),
            NULL,
            &CreatingLocation);
}


DWORD
Semantics::GetAnonymousDelegateHashCode
(
    BCSYM_Param * Parameters,
    bool IsFunctionLambda
)
{
    DWORD hash = 0;
    // first bit is if delegate is sub or function.
    if (IsFunctionLambda)
    {
        hash |= 0x1;
    }

    // next 7 bits encode will encode the number of arguments. (Populated at the end).

    unsigned int numberOfParameters = 0;
    int currentBit = 8;
    BCSYM_Param* CurrentParam = Parameters;
    // the next 24 bits will indicate byval/byref.
    while (CurrentParam != NULL)
    {
        if (CurrentParam->IsByRefKeywordUsed())
        {
            hash |= 0x1 << currentBit;
        }

        numberOfParameters++;
        currentBit++;
        if (currentBit >= 32)
        {
            currentBit = 8;
        }
        CurrentParam = CurrentParam->GetNext();
    }
    //up to 128 parameters will have a unique bucket, above that
    // 0xFE = 000000000000000000000000011111100
    hash |= (numberOfParameters << 1) & 0xFE;

    return hash;
}

// This method will only match on arity, byval/byref, param name, and is function or sub
// This will NOT check any of the types on the params
bool
MatchAnonymousDelegate
(
    BCSYM_Param*    InvokeParameters,
    Type*           InvokeResultType,
    SourceFile*     InvokeSourceFile,
    Location*       InvokeLocation,
    BCSYM_Param*    LambdaParameters,
    bool            LambdaIsFunction,
    SourceFile*     LambdaSourceFile,
    Location*       LambdaLocation,
    bool            CheckLocation
)
{
    BCSYM_Param* InvokeParam = InvokeParameters;
    BCSYM_Param* LambdaParam = LambdaParameters;

    while (InvokeParam && LambdaParam)
    {
        if (InvokeParam->IsByRefKeywordUsed() != LambdaParam->IsByRefKeywordUsed())
        {
            return false;
        }
        if (!StringPool::IsEqual(InvokeParam->GetName(), LambdaParam->GetName()))
        {
            return false;
        }

        InvokeParam = InvokeParam->GetNext();
        LambdaParam = LambdaParam->GetNext();
    }

    if (InvokeParam != NULL || LambdaParam != NULL)
    {
        return false;
    }

    if (CheckLocation)
    {
        if (! (InvokeSourceFile == LambdaSourceFile &&
              Location::Compare(LambdaLocation, InvokeLocation) == 0))
        {
            return false;
        }
    }

    // warning: this test for "Subness" only works because the Invoke lambda has already been bound.
    // Prior to binding, while we're still inferring the result type, a function lambda will be provisionally
    // marked with Void return type until the real one has been calculated. See also Dev10#526518
    bool InvokeIsSub = InvokeResultType == NULL || InvokeResultType->IsVoidType();
    bool LambdaIsSub = !LambdaIsFunction;

    return InvokeIsSub == LambdaIsSub;
}

#if IDE 
bool
MatchAnonymousDelegate
(
    Container *AnonymousDelegate1,
    Container *AnonymousDelegate2
)
{
    if (AnonymousDelegate1 && AnonymousDelegate2)
    {
        if (AnonymousDelegate1 == AnonymousDelegate2)
        {
            return true;
        }

        Declaration *Invoke1 = GetInvokeFromDelegate(AnonymousDelegate1, NULL);
        Declaration *Invoke2 = GetInvokeFromDelegate(AnonymousDelegate2, NULL);

        if (!Invoke1 || !Invoke1->IsProc() ||
            !Invoke2 || !Invoke2->IsProc())
        {
            VSASSERT(false, "How could this happen, we have a delegate in our hash that doesn't have a proper invoke method.");
            return false;
        }

        return MatchAnonymousDelegate(
                    Invoke1->PProc()->GetFirstParam(),
                    Invoke1->PProc()->GetRawType(),
                    AnonymousDelegate1->PClass()->GetSourceFile(),
                    Invoke1->GetLocation(),

                    Invoke2->PProc()->GetFirstParam(),
                    Invoke2->PProc()->GetRawType(),
                    AnonymousDelegate2->PClass()->GetSourceFile(),
                    Invoke2->GetLocation(),

                    false); // Don't check location .
    }

    // Even though the caller will not let this go through, match NULL to NULL
    // to match the logic on anonymous types.
    return AnonymousDelegate1 == AnonymousDelegate2;
}
#endif

ClassOrRecordType*
Semantics::ConstructAnonymousDelegateClass
(
    BCSYM_Param * LambdaParameters,
    Type * LambdaResultType,
    bool IsFunctionLambda,
    Location CreatingLocation
)
{
    bool IsDummyAnonymousDelegate = false;

    // Build a stub with the name VB$AnonymousDelegate_<number>
    STRING *ClassName = NULL;
    if (m_SymbolsCreatedDuringInterpretation)
    {
        ClassName = m_SymbolsCreatedDuringInterpretation->GetAnonymousDelegateNameGenerator()->GetNextName();
    }
    else
    {
        ClassName = m_Compiler->AddString(VB_ANONYMOUSTYPE_DELEGATE_PREFIX);
        IsDummyAnonymousDelegate = true;
    }

    ClassOrRecordType *DelegateClass = m_TransientSymbolCreator.AllocClass(true);
    SymbolList DelegateChildren;


    // Computer the number of lambda parameters.
    unsigned int NumberOfLambdaParameters = 0;
    BCSYM_Param *CurrentLambdaParameter = LambdaParameters;
    while (CurrentLambdaParameter != NULL)
    {
        NumberOfLambdaParameters++;
        CurrentLambdaParameter = CurrentLambdaParameter->GetNext();
    }

    unsigned int NumberOfGenericParameters = IsFunctionLambda ? NumberOfLambdaParameters+1 : NumberOfLambdaParameters;
    // Dev10#526518: if it's a function lambda, we need an additional parameter.
    // Note that it would be a mistake to use "LambdaResultType==NULL || LambdaResultType->IsVoidType()" as
    // our check. That's because Void is used as the temporary placeholder return type while inferring the
    // real return type.

    // Anonymous Type generic parameters goes from T0 ... Tm where m = number of initializer fields
    BCSYM_GenericParam *GenericParams = NULL;
    BCSYM_GenericParam **GenericParam = &GenericParams;

    // Tricky loop here, GenericParams will be the head of the list and GenericParam is the pointer
    // to the next Element of the Last element.
	unsigned i;
    for (i = 0; i < NumberOfLambdaParameters; ++i)
    {
        WCHAR GenericParamName[MaxStringLengthForParamSize + 1];
        IfFailThrow(StringCchPrintfW(GenericParamName, DIM(GenericParamName), WIDE("TArg%d"), i));

        *GenericParam =
            m_TransientSymbolCreator.GetGenericParam(
                NULL,
                m_Compiler->AddString(GenericParamName),
                i,
                false,
                Variance_None); // Variance-spec $1.4: we see no need for variance in anonymous delegates

        GenericParam = (*GenericParam)->GetNextParamTarget();
    }

    BCSYM_GenericParam* ReturnParam = NULL;

    // Allocate generic parameter for the Result.
    if (IsFunctionLambda)
    {
        WCHAR GenericParamName[MaxStringLengthForParamSize + 1];
        IfFailThrow(StringCchPrintfW(GenericParamName, DIM(GenericParamName), WIDE("TResult")));

        ReturnParam = m_TransientSymbolCreator.GetGenericParam(
                NULL,
                m_Compiler->AddString(GenericParamName),
                i,
                false,
                Variance_None); // Variance-spec $1.4: we see no use for variance in anonymous delegates
        *GenericParam =ReturnParam;

        GenericParam = (*GenericParam)->GetNextParamTarget();
    }

    ////
    //// Create the class symbol for the delegate
    ////
    BCSYM_Namespace *ContainingNamespace = m_Project ?
                            m_Compiler->GetUnnamedNamespace(m_Project):
                            m_Compiler->GetUnnamedNamespace();  // Generate anonymous types in global namespace.
    BCSYM_Container *Container = ContainingNamespace->PContainer();

    m_TransientSymbolCreator.GetClass(
        &CreatingLocation,
        ClassName,
        GetEmittedTypeNameFromActualTypeName(ClassName, GenericParams, m_Compiler), // emitted name for class,
        Container->GetContainingNamespace()->GetName(),
        m_SourceFile,
        GetFXSymbolProvider()->GetMultiCastDelegateType(),
        NULL,           // Implements list
        DECLF_NotInheritable | DECLF_Friend | DECLF_Delegate | DECLF_Anonymous,
        t_bad,
        m_TransientSymbolCreator.AllocVariable(false, false),
        &DelegateChildren,
        NULL,
        GenericParams,
        NULL,
        DelegateClass);


    PopulateDelegateClassForLambda(
        DelegateClass,
        &DelegateChildren,
        CreatingLocation,
        LambdaParameters,
        GenericParams,
        ReturnParam);

    DelegateClass->SetTypeAccessExplictlySpecified(false);
    DelegateClass->SetBaseExplicitlySpecified(false);

    Scope *Hash = m_TransientSymbolCreator.GetHashTable(ClassName, DelegateClass, true, 16, &DelegateChildren);
    Scope *UnbindableHash = m_TransientSymbolCreator.GetHashTable(ClassName, DelegateClass, true, 16, NULL);

    DelegateClass->SetHashes(Hash, UnbindableHash);
    DelegateClass->SetBindingDone(true);

//    VSASSERT(Container->GetUnBindableChildrenHash(), "How can a container not have an unbindable members hash ?");

    Symbols::SetParent(DelegateClass, Container->GetHash());

    // Don't register anonymous delegates when interpreting method bodies. They
    // will be reigstered later only if required for the eventual bound tree generated.
    // Bug Devdiv 78381.
    //
    if (!m_InterpretingMethodBody)
    {
        RegisterTransientSymbol(DelegateClass);
    }

    // Generate the attributes for the delegate to make it display better in the debugger
    GenerateAnonymousDelegateAttributes(DelegateClass);

    return DelegateClass;
}

//==============================================================================
// To avoid showing the nasty generated anonymous delegate name in the debugger,
// we instead will show <generated method>.  This is accomplished by
// adding some DebuggerDisplay attribute values
//
// Similar to the name we display for anonymous types, this should not be
// localized
//==============================================================================
void
Semantics::GenerateAnonymousDelegateAttributes
(
    ClassOrRecordType *DelegateClass
)
{
    STRING *TypeString = m_Compiler->AddString(
            L"Global.System.Diagnostics.DebuggerDisplay(\"<generated method>\",Type := \"<generated method>\")");

    GenerateDebuggerDisplayAttribute(
        DelegateClass,
        TypeString);
}


BCSYM_Proc*
Semantics::PopulateDelegateClassForLambda
(
    ClassOrRecordType *DelegateClass,
    _Inout_opt_ SymbolList *DelegateChildren,
    Location &TextSpan,
    BCSYM_Param *Parameters,
    BCSYM_GenericParam *GenericParams,
    BCSYM *ReturnType
)
{
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
    BCSYM_Param *LastParam = NULL;
    BCSYM_Param *FirstParam = NULL;

    LastParam = NULL;
    FirstParam = BuildGenericParametersForLambda(
        Parameters,
        DelegateClass->GetFirstGenericParam(),
        &LastParam,
        &m_TransientSymbolCreator);

    BCSYM_SyntheticMethod *InvokeMethod = m_TransientSymbolCreator.AllocSyntheticMethod(!m_MergeAnonymousTypeTemplates);

    // Dev10 #800576 IDE matches AnonymousDelegates by signature and location of the Invoke method. Need to set location in this case.
    m_TransientSymbolCreator.GetProc(
                        !m_MergeAnonymousTypeTemplates ? &TextSpan : NULL, // The container type binding will have location
                        STRING_CONST(m_Compiler, DelegateInvoke),
                        STRING_CONST(m_Compiler, DelegateInvoke),
                        (CodeBlockLocation*) NULL,
                        (CodeBlockLocation*) NULL,
                        DECLF_Function | DECLF_Public | DECLF_New | DECLF_Overridable | DECLF_HasRetval,
                        ReturnType,
                        FirstParam,
                        NULL,
                        NULL,
                        NULL,
                        SYNTH_TransientSymbol,
                        NULL,
                        DelegateChildren,
                        InvokeMethod);

    InvokeMethod->SetIsMethodCodeTypeRuntime(true);

    if (!(m_SourceFile->GetCompilerProject() && 
          m_SourceFile->GetCompilerProject()->GetOutputType() == OUTPUT_WinMDObj &&
          DelegateClass->GetAccess() == ACCESS_Public) &&
          GetFXSymbolProvider()->IsTypeAvailable(FX::AsyncCallbackType) &&
          GetFXSymbolProvider()->IsTypeAvailable(FX::IAsyncResultType))
    {
        // Asynchronous begin Invoke
        // IAsyncResult BeginInvoke(<params>, AsyncCallback, Object)
        BCSYM_SyntheticMethod *BeginInvokeMethod = m_TransientSymbolCreator.AllocSyntheticMethod( !m_MergeAnonymousTypeTemplates );

        LastParam = NULL;
        FirstParam = Declared::CloneParameters(FirstParam, false, &LastParam, &m_TransientSymbolCreator);

        // Now, tack on DelegateCallback & DelegateAsyncState parameters
        m_TransientSymbolCreator.GetParam(
            NULL,
            STRING_CONST(m_Compiler, DelegateCallback), // "DelegateCallback"
            GetFXSymbolProvider()->GetType(FX::AsyncCallbackType),
            0,
            NULL,
            &FirstParam,
            &LastParam,
            false);  // Not a return type param

        m_TransientSymbolCreator.GetParam(
            NULL,
            STRING_CONST(m_Compiler, DelegateAsyncState), // "DelegateAsyncState
            GetFXSymbolProvider()->GetType(FX::ObjectType),
            0,
            NULL,
            &FirstParam,
            &LastParam,
            false);  // Not a return type param

        m_TransientSymbolCreator.GetProc(
                           NULL, // The generic binding will have the location
                           STRING_CONST(m_Compiler, DelegateBeginInvoke),
                           STRING_CONST(m_Compiler, DelegateBeginInvoke),
                           ( CodeBlockLocation* ) NULL,
                           ( CodeBlockLocation* ) NULL,
                           DECLF_Function | DECLF_Public | DECLF_Overridable | DECLF_New,
                           GetFXSymbolProvider()->GetType(FX::IAsyncResultType),
                           FirstParam,
                           NULL,
                           NULL,
                           NULL,
                           SYNTH_TransientSymbol,
                           NULL, // no generic params
                           DelegateChildren,
                           BeginInvokeMethod );
        BeginInvokeMethod->SetIsMethodCodeTypeRuntime(true);

        // Asynchronous end Invoke
        // <rettype> EndInvoke(<non-byval params>, IAsyncResult)
        // EndInvoke takes only byref params because they need to be copied-out when the async call finishes

        FirstParam = LastParam = NULL;
        BCSYM_SyntheticMethod *EndInvokeMethod = m_TransientSymbolCreator.AllocSyntheticMethod( !m_MergeAnonymousTypeTemplates );

        // First, make the regular parameters
        FirstParam = Declared::CloneParameters(
                FirstParam,
                true, // All ByVal parameters are ignored for EndInvoke
                &LastParam,
                &m_TransientSymbolCreator);

        // Now, tack on the IAsyncResult parameter and make the proc
        m_TransientSymbolCreator.GetParam(
                            NULL,
                            STRING_CONST(m_Compiler, DelegateAsyncResult),
                            GetFXSymbolProvider()->GetType(FX::IAsyncResultType),
                            0,
                            NULL,
                            &FirstParam,
                            &LastParam,
                            false);  // Not a return type param

        m_TransientSymbolCreator.GetProc(
                           NULL, // The generic binding will have the location
                           STRING_CONST( m_Compiler, DelegateEndInvoke ),
                           STRING_CONST( m_Compiler, DelegateEndInvoke ),
                           (CodeBlockLocation*) NULL,
                           (CodeBlockLocation*) NULL,
                           DECLF_Function | DECLF_HasRetval | DECLF_Public | DECLF_Overridable | DECLF_New,
                           ReturnType,
                           FirstParam,
                           NULL,
                           NULL,
                           NULL,
                           SYNTH_TransientSymbol,
                           NULL, // no generic params
                           DelegateChildren,
                           EndInvokeMethod );
        EndInvokeMethod->SetIsMethodCodeTypeRuntime(true);
    }

    // Main constructor, <object, IntPointer>
    FirstParam = LastParam = NULL;
    BCSYM_SyntheticMethod *ConstructorMethod = m_TransientSymbolCreator.AllocSyntheticMethod( !m_MergeAnonymousTypeTemplates );

    m_TransientSymbolCreator.GetParam(
                        NULL,
                        STRING_CONST(m_Compiler, DelegateTargetObject), // "TargetObject"
                        GetFXSymbolProvider()->GetType(FX::ObjectType), // param type will be "object"
                        0,
                        NULL,
                        &FirstParam,
                        &LastParam,
                        false);  // Not a return type param

    m_TransientSymbolCreator.GetParam(
                        NULL,
                        STRING_CONST(m_Compiler, DelegateTargetMethod), // "TargetMethod"
                        GetFXSymbolProvider()->GetType(FX::IntPtrType),  // param is type Integer Pointer
                        0,
                        NULL,
                        &FirstParam,
                        &LastParam,
                        false);  // Not a return type param

    m_TransientSymbolCreator.GetProc(
                       NULL, // The generic bindint will have the location
                       STRING_CONST(m_Compiler, Constructor), // .ctor
                       STRING_CONST(m_Compiler, Constructor), // .ctor
                       ( CodeBlockLocation* ) NULL,
                       ( CodeBlockLocation* ) NULL,
                       DECLF_Function | DECLF_Public | DECLF_Constructor | DECLF_Hidden | DECLF_SpecialName,
                       NULL,
                       FirstParam,
                       NULL,
                       NULL,
                       NULL,
                       SYNTH_TransientSymbol,
                       NULL, // no generic params
                       DelegateChildren,
                       ConstructorMethod );
    ConstructorMethod->SetIsMethodCodeTypeRuntime(true);

    return InvokeMethod;
}


bool
Semantics::IsOrCouldBeConvertedIntoLambda
(
    ILTree::Expression * Expr
)
{
    return Expr->bilop == SX_LAMBDA || Expr->bilop == SX_UNBOUND_LAMBDA || Expr->bilop == SX_ADDRESSOF;
}

void
Semantics::FixLambdaExpressionForClosures
(
    ILTree::LambdaExpression * Expr
)
{

    // 
    bool isOrCouldBeConvertedIntoALambda = false;

    if (Expr->IsStatementLambda)
    {
        return;
    }
    else
    {
        ILTree::Expression *BoundExpression = Expr->GetExpressionLambdaBody();

        if (IsOrCouldBeConvertedIntoLambda(Expr->GetExpressionLambdaBody()))
        {
            Expr->SetExpressionLambdaBody(AllocateExpression(
                SX_SEQ_OP2,
                BoundExpression->ResultType,
                BoundExpression,
                NULL,
                BoundExpression->Loc
                )
            );
        }
    }
}


BCSYM_Param*
Semantics::BuildGenericParametersForLambda(
    BCSYM_Param*        Parameter,
    BCSYM_GenericParam* GenericParameter,
    BCSYM_Param**       LastParameterSymbol,
    Symbols*            Symbols
)
{
    BCSYM_Param *ParamFirst  = NULL;
    BCSYM_Param *LastParam   = NULL;

    BCSYM_Param*        CurrentParameter = Parameter;
    BCSYM_GenericParam* CurrentGenericParameter = GenericParameter;

    while (CurrentParameter != NULL && CurrentGenericParameter != NULL)
    {
        bool isByRef = CurrentParameter->IsByRefKeywordUsed();

        Type* parameterType = CurrentGenericParameter;
        if (isByRef)
        {
            parameterType = m_TransientSymbolCreator.MakePtrType(parameterType);
        }

        m_TransientSymbolCreator.GetParam(
           NULL,
           CurrentParameter->GetName(),
           parameterType,
           isByRef ? PARAMF_ByRef : PARAMF_ByVal,
           NULL,
           &ParamFirst,
           &LastParam,
           false);

        CurrentGenericParameter = CurrentGenericParameter->GetNextParam();
        CurrentParameter = CurrentParameter->GetNext();
    }

    return ParamFirst;
}

BCSYM_Hash *ILTree::LambdaExpression::GetLocalsHash() 
{ 
    return pLocalsHash;
}

void ILTree::LambdaExpression::SetLocalsHash(_In_ BCSYM_Hash *pValue)
{
    ThrowIfNull(pValue);

    pLocalsHash = pValue;
}

ILTree::ILNode *ILTree::LambdaExpression::GetLambdaBody()
{
    if (IsStatementLambda)
    {
        return pLambdaStatementBlock;
    }
    else
    {
        return pLambdaExpression;
    }
}

ILTree::Expression** ILTree::LambdaExpression::GetAddressOfExpressionLambdaBody()
{
    VSASSERT( !IsStatementLambda, "This was not an expression lambda");
    return !IsStatementLambda ? &(pLambdaExpression) : NULL;
}

void ILTree::LambdaExpression::SetExpressionLambdaBody(_In_ ILTree::Expression *pValue)
{
    ThrowIfNull(pValue);
    IsStatementLambda = false;
    pLambdaExpression = pValue;
}

Type* ILTree::LambdaExpression::GetExpressionLambdaReturnType()
{
    VSASSERT( !IsStatementLambda, "This was not an expression lambda");
    if (!IsStatementLambda)
    {
        if (IsFunctionLambda)
        {
            return GetExpressionLambdaBody()->ResultType;
        }
        else
        {
            return TypeHelpers::GetVoidType();
        }
    }
    else
    {
        return NULL;
    }
}

ILTree::StatementLambdaBlock *ILTree::LambdaExpression::GetStatementLambdaBody()
{
    VSASSERT( IsStatementLambda, "This was not a statement lambda");
    return IsStatementLambda ? pLambdaStatementBlock : NULL;
}


void ILTree::LambdaExpression::SetStatementLambdaBody(_In_ ILTree::StatementLambdaBlock *pValue)
{
    ThrowIfNull(pValue);
    IsStatementLambda = true;
    pLambdaStatementBlock = pValue;
}
        
