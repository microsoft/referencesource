//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Resumable Method Rewriter
//
//  Transforms the resumable methods, i.e. Iterator and Async methods, into state machines. The
//  state machines are implemented as display classes with MoveNext methods ala C#'s iterator
//  feature.
//
// ===========================================================================================
// REWRITE OF RESUMABLE METHODS - BY EXAMPLE
// ===========================================================================================
//
//   Async Sub f<T>(params..)             Sub f<T>(params..)
//     ...                          ==>     Dim sm As New f_StateMachine<T>
//   End Sub                                sm.params.. = params..
//                                          sm.$MoveNextDelegate = AddressOf sm.MoveNext
//                                          sm.$Me = Me   ' if f were an instance method
//                                          sm.$State = INITIAL_STATE
//                                          sm.$Builder = System.Runtime.CompilerServices.AsyncVoidMethodBuilder.Create()
//                                          sm.$Builder.Start<f_StateMachine<T>>(ref sm)
//                                        End Sub
//
//   Async Function f<T>(params..)        Function f<T>(params..) As Task(Of Integer)
//           As Task(Of Integer)    ==>     Dim sm As New f_StateMachine<T>
//     ...                                  sm.params.. = params..
//   End Function                           sm.$MoveNextDelegate = AddressOf sm.MoveNextDelegate
//                                          sm.$Me = Me   ' if f were an instance method
//                                          sm.$State = INITIAL_STATE
//                                          sm.$Builder = System.Runtime.CompilerServices.AsyncTaskMethodBuilder<T>.Create()
//                                          sm.$Builder.Start<f_StateMachine<T>>(ref sm)
//                                          Return sm.$Builder.Task
//                                        End Function
//
//   Iterator Function f<T>(params..)     Function f<T>(params..) As IEnumerator(Of Integer)
//        As IEnumerator(Of Integer) ==>    Dim sm As New f_StateMachine<T>
//     ...                                  sm.proto$params.. = params..
//   End Function                           sm.$Me = Me   ' if f were an instance method
//                                          sm.$State = INITIAL_STATE
//                                          Return sm
//                                        End Function  
//                                        
//   Iterator Function f<T>(params..)     Function f<T>(params..) As IEnumerable(Of Integer)
//        As IEnumerable(Of Integer) ==>    Dim sm As New f_StateMachine<T>
//     ...                                  sm.proto$params.. = params..
//   End Function                           sm.$Me = Me   ' if f were an instance method
//                                          sm.$State = ENUMERABLE_NOT_YET_STARTED
//                                        End Function
//
// ---------------------------------------------------------------------------------------------------------
//
//     Class f_StateMachine<SM$T>
//       Implements IEnumerable<U'>      ' if "f" returns IEnumerable<U>. I write U' to show it might be rewritten with SM$T if needed
//       Implements IEnumerable<object>  ' if "f" returns the non-generic IEnumerable
//       Implements IEnumerable          ' if "f" returns IEnumerable<U> or the non-generic IEnumerable
//       Implements IEnumerator<U'>      ' if "f" returns IEnumerable<U> or IEnumerator<U>.
//       Implements IEnumerator<object>  ' if "f" returns the non-generic IEnumerable or IEnumerator.
//       Implements IEnumerator          ' if "f" returns IEnumerable<U> / IEnumerable / IEnumerator<U> / IEnumerator
//       Implements IDisposable          ' if "f" is an Iterator method
//       Implements IAsyncStateMachine   ' if "f" is a resumable method
//
//       Dim proto$params.. As ..        ' for Iterables
//       Dim params.. As ..              ' for all resuamble methods
//       Dim $Me As ..                   ' for all resumable methods if they are instance methods
//       Dim $State As Integer
//       Dim $Builder As System.Runtime.CompilerServices.AsyncTaskMethodBuilder<U'>  ' for Async Functions. Or it might be the non-generic AsyncMethodBuilder.
//       Dim $Builder As System.Runtime.CompilerServices.AsyncVoidMethodBuilder      ' for Async Subs.
//       Dim $Disposing As Boolean       ' for Iterables + Iterators
//       Dim $InitialThreadId As Integer ' for Iterables + Iterators
//       Dim $Current As U'              ' for Iterables + Iterators. U' the rewritten form of IEnumerable<U>/IEnumerator<U> that the function returned, or Object if f returned a non-generic.
//       Dim locals..
//       
//       Sub New()
//         Me.$State = INITIAL_STATE
//         Me.$InitialThreadId = System.Threading.Thread.CurrentThread.ManagedThreadId  ' for Iterables
//       End Sub
//
//       Sub Dispose() Implements IDisposable.Dispose  ' for Iterables
//         Me.$Disposing = True
//         Me.MoveNext()
//         Me.$State = m_iStateMachineFinished
//       End Sub
//
//       Readonly Property Current As Object Implements IEnumerator.Current  ' for Iterators+Iterables
//         Get : Return $Current : End Get
//       Readonly Property Current As U' Implements IEnumerator<U'>.Current
//         Get : Return $Current : End Get
//       End Property
//
//       Function GetEnumerator0() As IEnumerator Implements IEnumerable.GetEnumerator ' for Iterables
//         Return Me.GetEnumerator()
//       Function GetEnumerator As IEnumerator<U'> Implements IEnumerable<U'>.GetEnumerator ' for Iterables
//         If System.Threading.Thread.CurrentThread.ManagedThreadId = Me.$InitialThreadId AndAlso Me.$State = ENUMERABLE_NOT_YET_STARTED Then
//           $State = INITIAL_STATE : GetEnumerator = Me
//         Else
//           GetEnumerator = New f_StateMachine<SM$T> : GetEnumerator.$Me = $Me
//         End If
//         params.. = proto$params..
//       End Function
//
//       Sub SetStateMachine(sm As IAsyncStateMachine) Implements IAsyncStateMachine.SetStateMachine ' for Asyncs
//         Me.$Builder.SetStateMachine(sm)
//       End Sub
//
//       Function MoveNext() As Boolean Implements IEnumerator.MoveNext  ' for Iterables + Iterators
//       Sub MoveNext() Implements IAsyncStateMachine.MoveNext  ' for Asyncs (either Subs or Task-returning)
//         ..                           ' <see below for the rewrite>
//       End Sub
//
// ---------------------------------------------------------------------------------------------------------
//
//       Sub MoveNext() Implements IAsyncStateMachine.MoveNext   ' for Sub Asyncs
//         Dim $DoFinallyBodies = True
//         Try
//           SWITCHTABLE
//           ... rewritten body
//           Go To ReturnLabel
//         Catch ex As Exception
//           $State = m_iStateMachineFinished
//           Me.$Builder.SetException(ex)
//         End Try
//         ReturnLabel:
//         $State = m_iStateMachineFinished
//         Me.$Builder.SetResult()
//       End Sub
//
//       Sub MoveNext() Implements IAsyncStateMachine.MoveNext          ' for Task-returning Asyncs
//         Dim $DoFinallyBodies = True
//         [Dim $ReturnValue = Nothing]  ' for Task<T>-returning asyncs
//         Try
//           SWITCHTABLE
//           ... rewritten body
//           Go To ReturnLabel
//         Catch ex As Exception
//           $State = m_iStateMachineFinished
//           Me.$Builder.SetException(ex)
//           Return
//         End Try
//         ReturnLabel:
//           $State = m_iStateMachineFinished
//         Me.$Builder.SetResult([$ReturnTempLocal])
//       End Sub
//
//       Function MoveNext() As Boolean Implements IEnumerator.MoveNext   ' for all Iterables+Iterators
//         Dim $DoFinallyBodies = True
//         Try
//           SWITCHTABLE, including CASE iStateMachineFinished: GOTO AbortLabel ' can't use ReturnLabel since can't switch out of a try
//           If Me.$Disposing Then AbortLabel: Return False 
//           ... rewritten body
//           Go To ReturnLabel
//         Catch ex As Exception
//           $State = m_iStateMachineFinished
//           Throw
//         End Try
//         ReturnLabel:
//         $State = m_iStateMachineFinished
//         Return False
//       End Function
//
// ---------------------------------------------------------------------------------------------------------
//
//    Given TypeOf(e) Is Task(Of T)
//    Await e                            ==>         dim AwaiterLocalTmp = e.GetAwaiter()
//                                                   If Not AwaiterLocalTmp.IsComplete Then
//                                                     <Stack Spill>
//                                                     Me.$State = X
//                                                     $DoFinallyBodies = False
//                                                     Me.$awaitTT = AwaiterLocalTmp
//                                                     Me.$Builder.Await[Unsafe]OnCompleted<TAwaiter,TSM>(ref AwaiterLocalTemp, ref Me)
//                                                     Return
//                                                 LABEL#X:  
//                                                     Me.$State = 0
//                                                     AwaiterLocalTmp = Me.$awaitTT
//                                                     <Restore Stack>
//                                                   End If
//                                                   AwaiterLocalTmp.GetResult()
//                                                   AwaiterLocalTmp = Nothing
//
//
//    Yield e                            ==>         Me.$Current = e
//                                                   Me.$State = X
//                                                   $DoFinallyBodies = False
//                                                   Return True
//                                               LABEL#X:
//                                                   If Me.$Disposing Then Return False
//                                                   Me.$State = INITIAL_STATE
//
//    Return e                           ==>         $ReturnTempLocal = e ' if operand was given (only in Task<U>-returning asyncs)
//    Return                                         Go To ReturnLabel
//    Exit Function                                  
//
//    Try                                ==>     STAGINGPOST#Y:
//      <body>                                       Try
//    Catch ex As Exception                            SWITCH-TABLE
//      <catch>                                        <body>
//    Finally                                        Catch ex As Exception
//      <finally>                                      <catch>
//    End Try                                        Finally
//                                                     If $DoFinallyBodies Then
//                                                       <finally>
//                                                     End If
//                                                   End Try
//
//
// ===========================================================================================
// REWRITETYPE AND GENERIC PARAMETERS, AND REWRITEEXPRESSION / SUBSTITUTEDUMMIES
// ===========================================================================================
// This is a tricky subject. Here's a worked example.
// 
// ORIGINAL:                        RESUMABLE REWRITTEN:
//
// Class C<T>                       Class C<T>
//   Async Fred<U>() As Task<U>       Class FredSM<SM$U>
//     Return e                         Dim builder As AsyncTaskBuilder<SM$U>
//   End Sub                            Sub MoveNext()
// End Class                              builder.SetResult(e {SM$U/U} )
//                                      End Sub  
//                                    End Class
//
//                                    Function Fred<U> As Task<U>
//                                      Dim sm As New FredSM<U>
//                                      Dim builder As AsyncTaskBuilder<U> = ...
//                                      sm.builder = builder
//                                      Return sm.Builder.GetTask()
//                                    End Function
//                                  End Class
//
// * The state-machine class has its OWN generic parameters SM$U,
//   one for each generic parameter U of the resumable method.
//
// * Within the rewritten MoveNext, it thinks that its builder type is AsyncTaskBuilder<SM$U>,
//   whereas the resumable method body thinks the builder type is AsyncTaskBuilder<U>
//
// * The call to awaiter.IsCompleted had initially been bound by semantics within
//   the context of the resumable method as CType(Nothing,AwaiterType<U>).IsCompleted
//   When we come to rewrite it in the MoveNext method, we are providing a DIFFERENT
//   type for the first dummy (the awaiter), and we have to REWRITE THE TYPE of the expression.
//   Similarly for all the other fields, e.g. the Current field of iterators.
//
// * Our convention is that, in this lowering phase, we modify IN-PLACE all the resumable info.
//   IsCompleted/GetResult/genericElementType are all rewritten into the context of the state-machine object.
//
//
// For lambda resumable methods there's an additional hoop:
//
//  ORIGINAL:                      LAMBDA REWRITTEN:                   RESUMABLE REWRITTEN:
//
//  Sub f<T>()                     Sub f<T>()                          Sub f<T>()
//    Dim lambda = Async Sub()       Dim c As New Closure<T>             Dim c As New Closure<T>
//                 End Sub           Dim lambda = new Func(c.Lambda)     Dim lambda = new Func(c.Lambda)
//  End Sub                        End Sub                             End Sub
//
//                                 Class Closure<$CL0>                 Class Closure<$CL0>
//                                   Async Sub Lambda                    Class LambdaSM
//                                   End Sub                             End Class
//                                 End Class                             
//                                                                       Sub Lambda
//                                                                       End Sub
//                                                                     End Class
//
// At least, this is how it looks from the compiler BCSYM perspective.
// But when you look at the IL, you'll see "Class LambdaSM<$CL0>".
// That's because in IL, every class first has all the generic parameters of its PARENT as well as its own.
//
// Incidentally, for this all to work, it's crucial that the "Lambda Rewriting Phase" correctly
// lifts all the additional ResumableInfo fields -- GetTaskBuilder, IsCompleted &c.
// It does that by calling the normal BoundTreeTypeVisitor.
//
//
// ===========================================================================================
// REWRITEEXPRESSION / SUBSTITUTEDUMMIES
// ===========================================================================================
//
// SubstituteDummies is called when semantics bound for us things like
//    {dummy}.IsCompleted As Boolean
//    {dummy}.GetResult()
//
// If any of them were mentioned in lambdas, then lambda-closure-rewrites will have
// rewritten the dummies so at least they're of the correct type.
//
// For the cases where we provide a substitution for some {dummy}, then presumably our substitution
// is already rewritten.
//
// It remains to rewrite the method-references themselves, and also "e", e.g.
//    RewriteExpression( .IsCompleted )
//    RewriteExpression( .GetResult )
//    RewriteExpression( e )
// What does it mean to "rewrite a method-reference"? It means that the original method reference
// might have pointed to e.g. the method TaskAwaiter<T>.GetResult, and we have
// rewrite it to TaskAwaiter<SM$T>.GetResult.
//
// For an SX_CALL, "CallExpression.Left" is the method reference. So we rewrite it often.
//
//    


//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------


class MarkContainsAsyncSpill : public BoundTreeVisitor
{

private:
    Filo<ILTree::Expression*, NorlsAllocWrapper> m_expressionStack;

public:
    MarkContainsAsyncSpill(NorlsAllocator& allocator) : m_expressionStack(NorlsAllocWrapper(&allocator)) { }

protected:

    virtual bool StartExpression(ILTree::Expression** ppExpression)
    {
        if (IsBad(*ppExpression))
        {
            return false;
        }

        ILTree::Expression* pExpression = (*ppExpression);
        
        // We just want to skip over lists and sequences.
        // WARNING: This means that sequence nodes don't have a useful "ContainsAsyncSpill" field.
        // e.g. SX_CALL[ SX_SEQ[ARG1, SX_ASYNCSPILL] ] -- in this case the SX_CALL and SX_ASYNCSPILL
        //      nodes will be marked with ContainsAsyncSpill, but the SX_SEQ node won't.
        // e.g. SL_REDIM[ SX_SEQ[ARG1, SX_ASYNCSPILL] ] -- here nothing will be marked ContainsAsyncSpill
        //      save the spill itself (since we only mark expressions, not statements).
        if (pExpression->bilop == SX_SEQ ||
            pExpression->bilop == SX_SEQ_OP1 ||
            pExpression->bilop == SX_SEQ_OP2 ||
            pExpression->bilop == SX_LIST)
        {
            return true;
        }

        if (pExpression->bilop == SX_ASYNCSPILL)
        {
            MarkAncestors();
        }

        m_expressionStack.PushOrEnqueue(pExpression);

        return true;
    }

    virtual void EndExpression(ILTree::Expression** ppExpression)
    {
        ILTree::Expression* pExpression = (*ppExpression);

        // We just want to skip over lists and sequences.
        if (pExpression->bilop == SX_SEQ ||
            pExpression->bilop == SX_SEQ_OP1 ||
            pExpression->bilop == SX_SEQ_OP2 ||
            pExpression->bilop == SX_LIST)
        {
            return;
        }

        ILTree::Expression* pPoppedExpression = m_expressionStack.PopOrDequeue();
        VSASSERT(pPoppedExpression == pExpression, "Uh oh, our stack and our visitor don't agree on the current expression!");
    }

private:
    MarkContainsAsyncSpill(const MarkContainsAsyncSpill&); // No copying
    MarkContainsAsyncSpill& operator=(const MarkContainsAsyncSpill&); // No copying

    void MarkAncestors()
    {
        for (ArrayListReverseIterator<ILTree::Expression*, NorlsAllocWrapper> it = m_expressionStack.GetIterator();
             it.MoveNext(); )
        {
            it.Current()->ContainsAsyncSpill = true;
        }
    }
};


// =============================================================================
// TemporaryAccounting
// In DEBUG mode, this is an aide to helping us not forget about temporaries.
// It keeps a log of all the temporaries we've explicitly lifted so far.
// At the end, if the original procedure had any non-short-lived temporaries that
// we haven't explicitly accounted for, then it pops up an ASSERT.
// This can help find potential logic or PEVerify errors even in repros that don't
// actually seem to do anything wrong (yet).
// In RELEASE mode, we just stub out the thing so it doesn't do anything.
//
#ifdef DEBUG
class TemporaryAccounting : public BoundTreeVisitor
{

public:
    TemporaryAccounting()
    {
    }
 
    void Account(_In_opt_ BCSYM_Variable *pVariable)
    {
        if (pVariable != NULL && pVariable->IsTemporary())
        {
            m_Account.Add(pVariable);
        }
    }

    void AccountAll(_In_opt_ ILTree::Expression *pExpression)
    {
        if (pExpression != NULL)
        {
            VisitExpression(&pExpression);
        }
    }

    void HoldToAccount(_In_ TemporaryManager *pMustAccountForThese)
    {
        if (pMustAccountForThese == NULL)
        {
            VSFAIL("Cannot pass null pMustAccountForThese");
            return;
        }
        TemporaryIterator temps;
        temps.Init(pMustAccountForThese);
        while (Temporary *temp = temps.GetNext())
        {
            if (temp->Lifetime != LifetimeShortLived && !m_Account.Contains(temp->Symbol))
            {
                VSFAIL("Oops -- it looks like we failed to lift a temporary into the resumable method. There'll probably be logic errors, and maybe a PEVerify failure.");
            }
        }
    }

private:
    TemporaryAccounting(const TemporaryAccounting&); // No copying
    TemporaryAccounting& operator=(const TemporaryAccounting&); // No copying

    HashSet<Symbol*> m_Account;

    virtual bool StartExpression(ILTree::Expression** ppExpression)
    {
        if (IsBad(*ppExpression))
        {
            return false;
        }

        ILTree::Expression* pExpression = (*ppExpression);
        if (pExpression->bilop == SX_SYM)
        {
            BCSYM *pSymbol = pExpression->AsSymbolReferenceExpression().Symbol;
            if (pSymbol != NULL && pSymbol->IsVariable())
            {
                Account(pSymbol->PVariable());
            }
        }
        return true;
    }
};
#else
class TemporaryAccounting
{
public:
    TemporaryAccounting() {}
    void Account(_In_opt_ BCSYM_Variable *) {}
    void AccountAll(_In_opt_ ILTree::Expression *) {}
    void HoldToAccount(_In_ TemporaryManager *) {}
};
#endif


struct LabelTarget : CSingleLink<LabelTarget>
{
    int state;
    LabelDeclaration *target;
};




class ResumableMethodLowerer : BoundTreeRewriter
{

public:
    ResumableMethodLowerer(Semantics *semantics, _In_ ILTree::ProcedureBlock *OriginalBoundBody);
    ~ResumableMethodLowerer() { }

    void CreateStateMachineClass();
    ILTree::ProcedureBlock* MakeMainBody();
    void GenerateCompilerGeneratedStateMachineAttribute(BCSYM_Proc* pResumableProc);
	
private:
    ResumableMethodLowerer(const ResumableMethodLowerer&); // No copying
    ResumableMethodLowerer& operator=(const ResumableMethodLowerer&); // No copying


    // The global ambient state that we get from the "semantics" parameter of the constructor
    Semantics* m_Semantics;
    Compiler* m_Compiler;
    CompilerHost* m_CompilerHost;
    FX::FXSymbolProvider* m_FXSymbolProvider;
    Symbols* m_TransientSymbolCreator;


    // Information about the method that we're lowering, that we get from the "BoundBody" parameter of the constructor
    ILTree::ProcedureBlock* m_OriginalBlock;
    Procedure* m_MainProc;
    Location m_MainProcLoc;
    ClassOrRecordType* m_MainContainerSymbol;
    BCSYM* m_MainContainerType;
    ILTree::ResumableKind m_ResumableKind;
    Type* m_ResumableGenericType; // for IEnumerator<T>,IEnumerable<T>,Task<T>, this is the "T". For all other cases it is NULL.
    bool IsIterator, IsIterable, IsSubAsync, IsFunctionAsync;


    // Helpers for CreateStateMachineClass
    BCSYM_Implements* GetImplementsList();
    BCSYM_GenericParam* CopyGenericParamsFromMethod();
    BCSYM_GenericConstraint* RewriteConstraint(BCSYM_GenericConstraint *oldConstraint);
    BCSYM* Bind(BCSYM *type, _In_opt_ Type* typeGenericArgument, _In_opt_ STRING *memberName, _In_opt_ Type* memberGenericArg0 = NULL, _In_opt_ Type* memberGenericArg1 = NULL); // binds to a member, or to a type if memberName is NULL
    void RobustAdd(StatementListBuilder *pBldr, ILTree::Statement *pStatement); // Will report an error if the statement was NULL
    bool m_RobustHasReportedErrors;
    //
    BCSYM_Property* AddCurrentPropertyToStateMachineClass(bool doGeneric);
    BCSYM_Proc* AddGetEnumeratorMethodToStateMachineClass(bool doGeneric);
    void AddConstructorToStateMachineClass();
    void AddDisposeMethodToStateMachineClass();
    void AddResetMethodToStateMachineClass();
    void AddSetStateMachineMethodToStateMachineClass();
    void AddMoveNextMethodToStateMachineClass();
    //
    BCSYM_Variable* AddFieldToStateMachineClass(_In_z_ STRING* fieldName, BCSYM* fieldType);
    BCSYM_NamedRoot* AddLocalToProc(_In_z_ STRING* name, BCSYM_Proc* proc, Type* type, Location &loc);
    void AddMethodToStateMachineClass(BCSYM_Proc *alreadyAllocatedProc, _In_z_ STRING* methodName, Type* returnType, DECLFLAGS uFlags, _In_opt_z_ STRING* parameterName=NULL, Type* parameterType=NULL);

    Location GetMainProcDebugBeginLocation();
    Location GetMainProcDebugEndLocation();


    // Information about the state machine, generated during the call to CreateStateMachineClass:
    ClassOrRecordType* m_StateMachineSymbol;  // This is the state machine itself
    BCSYM *m_StateMachineType;                // The type of the state machine, from the perspective of methods inside it (i.e. an open generic binding if necessary)
    STRING *m_stateMachineNamePrefix;         // e.g. "VB$StateMachine_3". Used for the name of the class, and also to generate unique labels
    BCSYM_SyntheticMethod* m_moveNextMethod;  // [bool/void] MoveNext()
    BCSYM_Variable* m_StateField;             // field "$State"
    BCSYM_Variable* m_MeField;                // field "$Me", present if the original resumable method was an instance method
    BCSYM_Variable* m_BuilderField;           // field "$Builder", present in Async methods
    DynamicHashTable<Type*, BCSYM_Variable*, NorlsAllocWrapper> m_AwaiterFields; // field "$AwaitTT" for whatever awaiter types are needed
    BCSYM_Variable* m_DisposingField;         // field "$Disposing" of type bool, present in Iterator+Iterable resumables (i.e. Iterator methods)
    BCSYM *m_CurrentType;                     // for iterator methods, this is the generic element type (if there was one) rewritten in the generic context of the state machine
    BCSYM_Variable* m_CurrentField;           // field "$Current", present in Iterator+Iterable resumables (i.e. iterator methods)
    BCSYM_Variable* m_InitialThreadField;     // field "$InitialThreadId", present in Iterator+Iterable resumables (i.e. iterator methods)
    BCSYM_Variable* m_StackStorageField;      // field "$Stack" of type Object, present in Builder+Sub resumables (i.e. Async methods)
    DynamicArray<BCSYM_Variable*> m_ParameterFields;      // List of fields "x, y, ..." corresponding to the parameters of the resumable method "f(x, y, ...)"
    DynamicArray<BCSYM_Variable*> m_ParameterProtoFields; // List of fields "proto$x, proto$y, ..." corresponding to the parameters of the resmable method
    //
    static const int ENUMERABLE_NOT_YET_STARTED = -2; // For efficiency, the same instance of StateMachine can be both the Enumerable and the first Enumerator that's got from it
    static const int INITIAL_STATE = -1;              // MaxInt. (so that switch tables don't need to check it explicitly; they just fall off the end)
    int m_iStateMachineFinished;                      // Number is computed by AddMoveNextMethod, and subsequently used in AddDisposeMethod.
    static const int FINISHED_STATE_SENTINEL = -2;    // The constructor initializes iStateMachineFinished to this sentinel


    // Helpers for rewriting the body that goes into MoveNext
    virtual ILTree::Expression* RewriteExpression(_In_ ILTree::Expression *expr);    // These first three virtual methods
    virtual ILTree::Statement* RewriteStatement(_In_ ILTree::Statement *statement);  // override the methods from BoundTreeVisitor
    virtual BCSYM* RewriteType(_In_ BCSYM *type);                                    // The remaining rewrites are just helper methods.
    ILTree::Expression* RewriteAwaitExpression(_In_ ILTree::AwaitExpression *expr);
    BCSYM_GenericBinding* RewriteGenericBinding(BCSYM_GenericBinding *pBinding);
    ILTree::Expression* RewriteSymbolExpression(_In_ ILTree::SymbolReferenceExpression *expr);
    ILTree::Expression* RewriteCast(_In_ ILTree::BinaryExpression *expr);
    Variable* RewriteVariable(_In_ Variable* originalVariable);
    ILTree::Statement* RewriteYieldStatement(_In_ ILTree::YieldStatement *statement);
    ILTree::Statement* RewriteExitStatement(_In_ ILTree::ReturnStatement *statement); 
    ILTree::Statement* RewriteReturnStatement(_In_ ILTree::ReturnStatement *statement);
    ILTree::Statement* RewriteTryBlock(_In_ ILTree::TryBlock *block);
    ILTree::Statement* RewriteCatchBlock(_In_ ILTree::CatchBlock *block);
    ILTree::Statement* RewriteFinallyBlock(_In_ ILTree::FinallyBlock *block);
    ILTree::Statement* RewriteForBlock(_In_ ILTree::ForBlock *block);
    ILTree::Statement* RewriteSelectBlock(_In_ ILTree::SelectBlock *block);
    ParseTree::Expression* RewriteDummy(ILTree::Expression* dummyCall, ParseTree::Expression* me, ParseTree::Expression* arg, Location &loc);
    ParseTree::Expression* SubstituteDummies(ILTree::CallExpression* dummyCall, ParseTree::Expression* me, ParseTree::Expression* arg, Location &loc);
    ParseTree::Expression* SubstituteLateBoundDummies(ILTree::LateBoundExpression* dummyCall, ParseTree::Expression* me, ParseTree::Expression* arg, Location &loc);
    BCSYM_Variable* GetAwaiterField(Type *pAwaiterFieldType);
    BCSYM_Variable* GetTemporary(Type *pType);



	
    //
    Symbol* m_DoFinallyBodiesLocal;    // local to the MoveNext method "$doFinallyBodies", related to user's Try/Finally handlers
    Symbol* m_ReturnTempLocal;         // Local to store the result of a Return statement operand
    LabelDeclaration* m_ReturnLabel;   // We "Go To" here rather than doing "SetResult():Return" so that finally blocks execute before SetResult()
    DynamicHashTable<BCSYM_Variable*, BCSYM_Variable*, NorlsAllocWrapper> m_LocalFieldMap;       // maps from locals variables in the original, to fields in the state machine
    DynamicHashTable<GenericParameter*, GenericParameter*, NorlsAllocWrapper> m_GenericParamMap; // maps from generic parameters of original resumable method, to generic parameters of state machine
    //
    int m_HighestStateUsed;                          // Each yield/await point has a state integer associated with it.
    int m_HighestAwaiterFieldUsed;                   // Each awaiter field has a unique name
    int m_HighestStagepostUsed;                      // Each try block has a label with a unique name; this counter provides that unique name
    CSingleList<LabelTarget> *m_CurrentScopeTargets; // Each Try/Finally block has its own local state->label map.
    ILTree::Statement* MakeSwitchFromCurrentScope();
    ILTree::Statement* MakeLabel(_In_z_ wchar_t *prefix, int i, _In_opt_ CSingleList<LabelTarget> *insert, _In_opt_ LabelDeclaration **decl); // Creates a label statement "prefix%i". If "insert" is non-NULL then adds "i:decl" into it. If decl is non-NULL then returns the labdecl in there.
    ILTree::Statement* MakeIf(Location loc, _In_ ILTree::Expression *condition, _In_ ILTree::Statement* thenBranch, _In_opt_ ILTree::Statement* elseBranch); // Creates an IF block-statement
    //
    DynamicHashTable<BCSYM_Hash*, ILTree::ExecutableBlock*, NorlsAllocWrapper> m_TryHandlers;   // contains all the try-handler-hashes that we've encountered so far, and the Catch/Finally they came from
    bool m_StatementContainsAwait; // When evaluating a Return statement, indicates if the operand contains an Await expression.
    //
    TemporaryAccounting m_TemporaryAccounting;  // If there are any temporaries we've failed to account for, it's probably an error
    HashSet<BCSYM_Variable*> m_TemporaryVars; // TemporaryManage can't reuse LongLived temporaries, so we manage reusing them here.
    bool m_RewritingOpImplicit;
};


//-------------------------------------------------------------------------------------------------
//


ResumableMethodLowerer::ResumableMethodLowerer(Semantics *semantics, _In_ ILTree::ProcedureBlock* originalBlock) : BoundTreeRewriter(&semantics->m_TransientSymbolCreator),
    m_AwaiterFields(semantics->m_TransientSymbolCreator.GetNorlsAllocator()),
    m_LocalFieldMap(semantics->m_TransientSymbolCreator.GetNorlsAllocator()),
    m_GenericParamMap(semantics->m_TransientSymbolCreator.GetNorlsAllocator()),
    m_TryHandlers(semantics->m_TransientSymbolCreator.GetNorlsAllocator())
{
    ThrowIfNull(semantics);
    ThrowIfNull(originalBlock);

    m_Semantics = semantics;
    m_Compiler = semantics->m_Compiler;
    m_CompilerHost = semantics->m_CompilerHost;
    m_FXSymbolProvider = semantics->m_FXSymbolProvider;
    m_TransientSymbolCreator = &semantics->m_TransientSymbolCreator;

    m_OriginalBlock = originalBlock;
    m_MainProc = originalBlock->pproc;
    m_MainProcLoc = Location::GetInvalidLocation(); // determined afterwards
    m_MainContainerSymbol = m_MainProc->GetContainingClass();
    m_MainContainerType = m_MainContainerSymbol; if (IsGenericOrHasGenericParent(m_MainContainerSymbol)) m_MainContainerType = SynthesizeOpenGenericBinding(m_MainContainerSymbol, semantics->m_TransientSymbolCreator);
    m_ResumableKind = originalBlock->m_ResumableKind;
    IsIterator = m_ResumableKind == ILTree::IteratorResumable;
    IsIterable = m_ResumableKind == ILTree::IterableResumable;
    IsSubAsync = m_ResumableKind == ILTree::SubResumable;
    IsFunctionAsync = m_ResumableKind == ILTree::TaskResumable;
	m_ResumableGenericType = originalBlock->m_ResumableGenericType;
				
    m_RobustHasReportedErrors = false;

    m_StateMachineSymbol = NULL;
    m_StateMachineType = NULL;
    m_stateMachineNamePrefix = NULL;
    m_moveNextMethod = NULL;
    m_MeField = NULL;
    m_StateField = NULL;
    m_BuilderField = NULL;
    m_DisposingField = NULL;
    m_CurrentField = NULL;
    m_InitialThreadField = NULL;

    m_DoFinallyBodiesLocal = NULL;
    m_ReturnTempLocal = NULL;
    m_ReturnLabel = NULL;
    m_HighestStateUsed = -1;
    m_HighestStagepostUsed = -1;
    m_HighestAwaiterFieldUsed = -1;
    m_iStateMachineFinished = FINISHED_STATE_SENTINEL;
    m_CurrentScopeTargets = NULL;
    m_RewritingOpImplicit = false;

    // Location...
    if (m_MainProc->HasLocation())
    {
        m_MainProcLoc  = *m_MainProc->GetLocation(); // regular methods
    }
    else if (m_MainProc->IsSyntheticMethod())
    {
        if (m_MainProc->PSyntheticMethod()->GetBodySpan().IsValid()) m_MainProcLoc = m_MainProc->PSyntheticMethod()->GetBodySpan(); // statement lambdas
        else m_MainProcLoc = m_MainProc->PSyntheticMethod()->GetWholeSpan(); // expression lambdas
    }
    VSASSERT(m_MainProcLoc.IsValid(), "Error: got an invalid location for the Mainproc. This will throw asserts in codegen.");


    AssertIfFalse(IsIterator || IsIterable || IsSubAsync || IsFunctionAsync);
}

BCSYM_Variable* ResumableMethodLowerer::GetAwaiterField(Type *awaiterFieldType)
{
    ASSERT(m_Semantics != NULL, "Must have m_Semantics already defined.");

    BCSYM_Variable *awaiterField = m_AwaiterFields.GetValueOrDefault(awaiterFieldType, NULL);
    if (awaiterField != NULL)
    {
        return awaiterField;
    }

    // VB uses type identity by the "BCSYM::AreTypesEqual" function (or TypeHelpers::EquivalentTypes, which does the same)
    // So if we didn't find an awaiter field by pointer identity, we should also check by true identity.
    auto it = m_AwaiterFields.GetConstIterator();
    while (it.MoveNext())
    {
        if (BCSYM::AreTypesEqual(it.Current().Key(), awaiterFieldType))
        {
            awaiterField = it.Current().Value();
            m_AwaiterFields.SetValue(awaiterFieldType, awaiterField); // save into the dictionary for quicker lookup in future
            return awaiterField;
        }
    }

    // Otherwise, the awaiter field didn't already exist for this awaiter-field-type, and
    // we must create a new one
    m_HighestAwaiterFieldUsed ++;
    StringBuffer namebuffer;
    namebuffer.AppendPrintf(L"$awaiter_%i", m_HighestAwaiterFieldUsed);
    STRING* awaiterFieldName = m_Compiler->AddString(&namebuffer);
    awaiterField = AddFieldToStateMachineClass(awaiterFieldName, awaiterFieldType);

    m_AwaiterFields.SetValue(awaiterFieldType, awaiterField);

    return awaiterField;
}

BCSYM_Variable* ResumableMethodLowerer::GetTemporary(Type *pType)
{

    BCSYM_Variable* pTemporary = NULL; 

    HashSetIterator<BCSYM_Variable*> iterator(&m_TemporaryVars);

    while (iterator.MoveNext())
    {
        pTemporary = iterator.Current();
        
        if (BCSYM::CompareReferencedTypes(
             pTemporary->GetType()->DigThroughNamedType(),
            (GenericBinding *)NULL,
            pType->DigThroughNamedType(),
            (GenericBinding *)NULL,
            m_Semantics->GetSymbols()) == EQ_Match)
        {
            return pTemporary;
        }
    }

    bool existingVariableReused = false;

    pTemporary = m_Semantics->GetTemporaryManager()->AllocateLongLivedTemporary(
        pType, 
        NULL, // Location
        m_moveNextMethod->GetBoundTree(),
        &existingVariableReused);

    m_TemporaryVars.Add(pTemporary);
    
    return pTemporary;
}


void ResumableMethodLowerer::CreateStateMachineClass()
{

    AssertIfFalse(m_StateMachineSymbol == NULL);

    // Allocate the clas...
    m_StateMachineSymbol = m_TransientSymbolCreator->AllocClass(false);

    // Name...
    m_stateMachineNamePrefix = m_Semantics->m_SymbolsCreatedDuringInterpretation->GetStateMachineNameGenerator()->GetNextName();
    StringBuffer namebuffer;
    namebuffer.AppendSTRING(m_stateMachineNamePrefix);
    namebuffer.AppendString(L"_");
    namebuffer.AppendSTRING(m_MainProc->GetName());
    if (m_MainProc->GetGenericParamCount()>0) namebuffer.AppendPrintf(L"`%i", m_MainProc->GetGenericParamCount());
    STRING* className = m_Compiler->AddString(&namebuffer);

    // Generic parameters...
    BCSYM_GenericParam* genericParams = (m_MainProc->IsGeneric()) ? CopyGenericParamsFromMethod() : NULL;

    // Implements...
    BCSYM_Implements* implementsList = GetImplementsList();

    // Fill out class information...
    m_TransientSymbolCreator->GetClass(
        NULL,
        className,
        className,
        m_MainProc->GetContainingNamespace()->GetName(),
        m_MainProc->GetSourceFile(),
        IsFunctionAsync || IsSubAsync ? 
            m_FXSymbolProvider->GetType(FX::ValueTypeType) :
            m_FXSymbolProvider->GetObjectType(),
        implementsList,
        IsFunctionAsync || IsSubAsync ? 
            (DECLF_Private | DECLF_NotInheritable | DECLF_Structure) : 
            (DECLF_Private | DECLF_NotInheritable) ,
        t_bad,  // no underlying type because we're not an enum
        m_TransientSymbolCreator->AllocVariable(false, false),
        NULL, // no children yet
        NULL, // no children yet
        genericParams,
        NULL, // ListOfSymbols - We're not tracking the list of symbols created by m_TransientSymbolCreator
        m_StateMachineSymbol);

    // Other class setup...
    m_StateMachineSymbol->SetBindingDone(true);
    m_StateMachineSymbol->SetIsStateMachineClass(true);
    m_StateMachineSymbol->SetOriginalResumableProc(m_MainProc);
    Scope* hash = m_TransientSymbolCreator->GetHashTable(className, m_StateMachineSymbol, true, 16, NULL);
    Scope* unbindableHash = m_TransientSymbolCreator->GetHashTable(className, m_StateMachineSymbol, true, 16, NULL);
    m_StateMachineSymbol->SetHashes(hash, unbindableHash);
    Symbols::SetParent(m_StateMachineSymbol, m_MainContainerSymbol->GetUnBindableChildrenHash());
    m_Semantics->RegisterTransientSymbol(m_StateMachineSymbol);

    if (IsFunctionAsync || IsSubAsync)
    {
        m_StateMachineSymbol->SetIsStruct(true);
    }

    // The type of the state machine (i.e. the type of "Me" from the perspective of instance methods within the state machine)
    m_StateMachineType = IsGenericOrHasGenericParent(m_StateMachineSymbol)
                         ? (BCSYM*)SynthesizeOpenGenericBinding(m_StateMachineSymbol, m_Semantics->m_TransientSymbolCreator)
                         : (BCSYM*)m_StateMachineSymbol;


    // ADD FIELDS AND METHODS

    m_StateField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldState), m_FXSymbolProvider->GetIntegerType());

    if (IsFunctionAsync || IsSubAsync)
    {
        Type *builderType;
        if (IsSubAsync)
        {
            builderType = Bind(m_FXSymbolProvider->GetType(FX::AsyncVoidMethodBuilderType), NULL, NULL);
        }
        else if (m_ResumableGenericType == NULL)
        {
            builderType = Bind(m_FXSymbolProvider->GetType(FX::AsyncTaskMethodBuilderType), NULL, NULL);
        }
        else
        {
            builderType = Bind(m_FXSymbolProvider->GetType(FX::GenericAsyncTaskMethodBuilderType), RewriteType(m_ResumableGenericType), NULL);
        }

        if (builderType != NULL && !builderType->IsBad())
        {
            m_BuilderField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldBuilder), builderType);
        }
    }

    if (IsFunctionAsync || IsSubAsync)
    {
        m_StackStorageField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldStack), m_FXSymbolProvider->GetObjectType());
    }

    if (IsIterator || IsIterable)
    {
        m_DisposingField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldDisposing), m_FXSymbolProvider->GetBooleanType());
        BCSYM *currentFieldType = (m_ResumableGenericType!=NULL) ? RewriteType(m_ResumableGenericType) : m_FXSymbolProvider->GetObjectType();
        if (currentFieldType != NULL && !currentFieldType->IsBad())
        {
            m_CurrentField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldCurrent), currentFieldType);
            // Note: if currentFieldType were ArgIterator or one of the other restricted types that couldn't be made into a field,
            // then this would produce unverifiable code. But elsewhere the compiler has already prevented ArgIterator from
            // being a generic type argument to IEnumerable(Of T)/IEnumerator(T), so an error would already have been reported.
            VSASSERT((!IsRestrictedType(currentFieldType, m_CompilerHost) && !IsRestrictedArrayType(currentFieldType, m_CompilerHost)) ||
                     m_Semantics->m_Errors->HasErrors(),
                     "About to put illegal iterator current type in the state machine. This will be un-PEVerifyable. Why hasn't an error already been reported?");
        }
    }

    if (IsIterable)
    {
        m_InitialThreadField = AddFieldToStateMachineClass(STRING_CONST(m_Compiler, StateMachineClassFieldInitialThreadId), m_FXSymbolProvider->GetIntegerType());
    }

    if (!m_MainProc->IsShared())
    {
        if (m_MainContainerType != NULL && !m_MainContainerType->IsBad())
        {
            STRING* name;
            if (m_MainContainerType->IsNamedRoot() && ClosureNameMangler::IsClosureClass(m_MainContainerType->PNamedRoot()))
            {
                name = ClosureNameMangler::EncodeNonLocalVariableName(m_Compiler, m_MainContainerType->PNamedRoot()->GetName());
            }
            else
            {
                name = ClosureNameMangler::EncodeMeName(m_Compiler);
            }
            m_MeField = AddFieldToStateMachineClass(name, RewriteType(m_MainContainerType));
            // Note: it's impossible for m_MainContainerType to be one of the restricted types
        }
    }

    BCITER_Parameters pit(m_MainProc);
    while (BCSYM_Param *param = pit.PparamNext())
    {
        Declaration *declParam = m_OriginalBlock->Locals->SimpleBind(param->GetName());
        ThrowIfFalse(declParam->IsVariable());
        Variable *originalVariable = declParam->PVariable();
        if (originalVariable==NULL || originalVariable->GetType()==NULL || originalVariable->GetType()->IsBad()) continue;

        STRING *newName = ClosureNameMangler::EncodeLocalVariableName(m_Compiler,originalVariable->GetName());
        Variable *newVariable = AddFieldToStateMachineClass(newName, RewriteType(originalVariable->GetType()));
        m_LocalFieldMap.SetValue(originalVariable, newVariable);
        m_ParameterFields.AddElement(newVariable);
        // Note: errors have already been reported elsewhere if any of the parameters were restricted types.

        if (IsIterable)
        {
            StringBuffer buf; buf.AppendString(L"proto$"); buf.AppendSTRING(param->GetName());
            newVariable = AddFieldToStateMachineClass(m_Compiler->AddString(&buf), RewriteType(originalVariable->GetType()));
            m_ParameterProtoFields.AddElement(newVariable);
        }
    }


    AddMoveNextMethodToStateMachineClass();


    if (IsIterator || IsIterable)
    {
        AddConstructorToStateMachineClass();
        AddDisposeMethodToStateMachineClass(); // NB. this requires that AddMoveNextMethod already have been called
        AddResetMethodToStateMachineClass();
        AddCurrentPropertyToStateMachineClass(false); // property Current As Object implements IEnumerable.Current
        AddCurrentPropertyToStateMachineClass(true);  // property Current as T implements IEnumerable<T>.Current
    }

    if (IsIterable)
    {
        AddGetEnumeratorMethodToStateMachineClass(false); // function GetEnumerator() As IEnumerator implements IEnumerable.GetEnumerator
        AddGetEnumeratorMethodToStateMachineClass(true);  // function GetEnumerator() As IEnumerator<T> implements IEnumerable<T>.GetEnumerator
    }

    if (IsFunctionAsync || IsSubAsync)
    {
        AddSetStateMachineMethodToStateMachineClass();
    }

}

void ResumableMethodLowerer::RobustAdd(StatementListBuilder *pBldr, ILTree::Statement *pStatement)
{
    ThrowIfNull(pBldr);
    if (pStatement == NULL)
    {
        if (!m_RobustHasReportedErrors) m_Semantics->ReportSemanticError(ERRID_InternalCompilerError, m_MainProcLoc);
        m_RobustHasReportedErrors = true;
    }
    else
    {
        pBldr->Add(pStatement);
    }
}


BCSYM* ResumableMethodLowerer::Bind(BCSYM *type, _In_opt_ Type* typeGenericArgument, _In_opt_ STRING *memberName, _In_opt_ Type* memberGenericArg0, _In_opt_ Type* memberGenericArg1)
{
    // This is a helper method for binding. It binds to one of these things (depending on which optional parameters are given):
    //     type
    //     type<typeGenericArgument>
    //     type.methodName
    //     type<typeGenericArgument>.methodName
    //     type.methodName<memberGenericArg0>
    //     type<typeGenericArgument>.methodName<memberGenericArg0>
    //     type.methodName<memberGenericArg0,memberGenericArg1>
    //     type<typeGenericArgument>.methodName<memberGenericArg0,memberGenericArg1>
    // If the type given is already generic and you pass NULL for GenericArgument, then it will use the generic type you gave.
    // If the type given is already generic and you pass non-NULL for GenericArgument, it is an error.

    if (type == NULL)
    {
        return NULL;
    }

    if (typeGenericArgument == NULL && memberName==NULL) return type;

    if (typeGenericArgument!=NULL && type->IsGenericBinding())
    {
        ThrowIfFalse(false); // Shouldn't specify generic argument for something that's already generic
        return NULL;
    }

    // 1. Get the symbol itself...
    BCSYM_NamedRoot *symbol;
    if (type->IsNamedRoot())
    {
        symbol = type->PNamedRoot();
    }
    else if (type->IsGenericTypeBinding())
    {
        symbol = type->PGenericTypeBinding()->GetGeneric();
    }
    else
    {
        ThrowIfFalse(false); // Should give either a symbol or a generic bound symbol
        return NULL;
    }

    // 2. If GenericArgument was supplied, resolve typeName<GenericArgument>
    BCSYM_GenericTypeBinding *genericBinding = NULL;
    if (type->IsGenericTypeBinding())
    {
        genericBinding = type->PGenericTypeBinding();
    }
    else if (typeGenericArgument != NULL)
    {
        BCSYM** argArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
        argArray[0] = typeGenericArgument;
        genericBinding = m_TransientSymbolCreator->GetGenericBinding(false, symbol, argArray, 1, NULL)->PGenericTypeBinding();
    }

    // 3. If that's it, we can return immediately
    if (memberName == NULL) return (genericBinding!=NULL) ? genericBinding : symbol;

    // 4. Get the member symbol
    BCSYM_NamedRoot *memberSymbol = symbol->SimpleBind(NULL, memberName);
    if (memberSymbol == NULL) return NULL;
    if (genericBinding == NULL) return memberSymbol;
    
    // 5. Prepare member generic arguments if needed
    BCSYM **memberGenericArguments = NULL;
    int memberGenericArgumentCount = (memberGenericArg0 == NULL ? 0 : 1) + (memberGenericArg1 == NULL ? 0 : 1);
    if (memberGenericArgumentCount > 0)
    {
        // We'll allocate size 2, even if only the first one was needed, just for ease
        memberGenericArguments = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)*2));
        memberGenericArguments[0] = memberGenericArg0;
        memberGenericArguments[1] = memberGenericArg1;
    }

    // 6. Get its generic binding
    BCSYM *genericMethodBinding = m_TransientSymbolCreator->GetGenericBinding(false, memberSymbol, memberGenericArguments, memberGenericArgumentCount, genericBinding /*parent*/);
    return genericMethodBinding;
}



BCSYM_GenericParam* ResumableMethodLowerer::CopyGenericParamsFromMethod()
{
    AssertIfFalse(m_MainProc->IsGeneric());

    // Create the generic parameters, with the same names.
    GenericParameter *firstGp = NULL, *lastGp = NULL;
    for (GenericParameter* oldGp = m_MainProc->GetFirstGenericParam(); oldGp != NULL; oldGp = oldGp->GetNextParam())
    {
        // Here we don't copy variance, since In|Out can not be specified on any generic parameter of method. 
        // If GetVariance() is not Variance_None, an error has been generated during binding.

        StringBuffer newNameBuf;
        newNameBuf.AppendString(L"SM$");
        newNameBuf.AppendSTRING(oldGp->GetName());
        STRING *newName = m_Compiler->AddString(&newNameBuf);

        GenericParameter* newGp = m_TransientSymbolCreator->GetGenericParam(NULL, newName, oldGp->GetPosition(), false, oldGp->GetVariance());
        m_GenericParamMap.SetValue(oldGp, newGp);

        if (firstGp == NULL) {firstGp = newGp; lastGp = newGp;}
        else {lastGp->SetNextParam(newGp); lastGp = newGp;}
    }

    for (GenericParameter *oldGp = m_MainProc->GetFirstGenericParam(), *newGp = firstGp;
         oldGp != NULL;
         oldGp = oldGp->GetNextParam(), newGp = newGp->GetNextParam())
    {
        GenericConstraint *oldConstraint = oldGp->GetConstraints();
        GenericConstraint *newConstraint = RewriteConstraint(oldConstraint);
        newGp->SetConstraints(newConstraint);
    }

    return firstGp;
}


BCSYM_GenericConstraint* ResumableMethodLowerer::RewriteConstraint(BCSYM_GenericConstraint *oldConstraint)
{
    if (oldConstraint == NULL) return NULL;

    BCSYM_GenericConstraint *oldNext = oldConstraint->Next();
    BCSYM_GenericConstraint *newNext = RewriteConstraint(oldNext);

    BCSYM *oldRoot = oldConstraint->IsGenericTypeConstraint() ? oldConstraint->PGenericTypeConstraint()->GetType() : NULL;
    BCSYM *newRoot = oldRoot==NULL ? oldRoot : RewriteType(oldRoot);

    if (oldRoot==newRoot && oldNext==newNext) return oldConstraint;

    BCSYM_GenericConstraint *newConstraint;
    if (oldConstraint->IsGenericTypeConstraint()) newConstraint = m_TransientSymbolCreator->GetGenericTypeConstraint(oldConstraint->GetLocation(), newRoot);
    else if (oldConstraint->IsGenericNonTypeConstraint()) newConstraint = m_TransientSymbolCreator->GetGenericNonTypeConstraint(oldConstraint->GetLocation(), oldConstraint->PGenericNonTypeConstraint()->GetConstraintKind());
    else {VSFAIL("Error: don't know what manner of constraint we had"); return oldConstraint;}

    newConstraint->SetIsBadConstraint(oldConstraint->IsBadConstraint());
    *newConstraint->GetNextConstraintTarget() = newNext;
    return newConstraint;
}


BCSYM_GenericBinding* ResumableMethodLowerer::RewriteGenericBinding(BCSYM_GenericBinding *pBinding)
{
    if (m_RewritingOpImplicit && pBinding != nullptr && pBinding == m_ResumableGenericType)
    {
        pBinding = m_TransientSymbolCreator->GetGenericBinding(
            IsBad(pBinding),
            pBinding->GetGeneric(),
            pBinding->GetArguments(),
            pBinding->GetArgumentCount(),
            pBinding->GetParentBinding(),
            true /* AllocAndCopyArgumentsToNewList */);             
    }

    return pBinding;
}

BCSYM* ResumableMethodLowerer::RewriteType(_In_ BCSYM *type)
{
    if (type==NULL || !type->IsGenericParam()) return BoundTreeRewriter::RewriteType(type);

    GenericParameter *oldGp = type->PGenericParam(), *newGp;
    if (m_GenericParamMap.GetValue(oldGp, &newGp)) return newGp;
    else return oldGp;
}


BCSYM_Implements* ResumableMethodLowerer::GetImplementsList()
{
    ThrowIfNull(m_StateMachineSymbol); // this must have been already alloc'd (even if not yet filled out) before we can declare implements clauses

    BCSYM_Implements* implementsList = NULL;
    BCSYM_NamedType* namedType;

    if (IsIterator || IsIterable)
    {
        BCSYM *genericElementType = (m_ResumableGenericType!=NULL) ? RewriteType(m_ResumableGenericType) : m_FXSymbolProvider->GetObjectType();


        namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 2,
            m_Compiler->AddString(L"System"), NULL, NULL,
            m_Compiler->AddString(L"IDisposable"), m_FXSymbolProvider->GetType(FX::IDisposableType), NULL);
        m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);

        namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 3,
            m_Compiler->AddString(L"System"), NULL, NULL,
            m_Compiler->AddString(L"Collections"), NULL, NULL,
            m_Compiler->AddString(L"IEnumerator"), m_FXSymbolProvider->GetType(FX::IEnumeratorType), NULL);
        m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);

        namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 4,
            m_Compiler->AddString(L"System"), NULL, NULL,
            m_Compiler->AddString(L"Collections"), NULL, NULL,
            m_Compiler->AddString(L"Generic"), NULL, NULL,
            m_Compiler->AddString(L"IEnumerator`1"),
                    m_FXSymbolProvider->GetType(FX::GenericIEnumeratorType),
                    m_Semantics->CreateNameTypeArguments(m_Semantics->m_TransientSymbolCreator, true, 1, genericElementType));
        m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);

        if (IsIterable)
        {
            namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 3,
                m_Compiler->AddString(L"System"), NULL, NULL,
                m_Compiler->AddString(L"Collections"), NULL, NULL,
                m_Compiler->AddString(L"IEnumerable"), m_FXSymbolProvider->GetType(FX::IEnumerableType), NULL);
            m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);

            namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 4,
                m_Compiler->AddString(L"System"), NULL, NULL,
                m_Compiler->AddString(L"Collections"), NULL, NULL,
                m_Compiler->AddString(L"Generic"), NULL, NULL,
                m_Compiler->AddString(L"IEnumerable`1"),
                        m_FXSymbolProvider->GetType(FX::GenericIEnumerableType),
                        m_Semantics->CreateNameTypeArguments(m_Semantics->m_TransientSymbolCreator, true, 1, genericElementType));
            m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);
        }
    }

    if (IsFunctionAsync || IsSubAsync)
    {
        namedType = m_Semantics->CreateNamedType(*m_TransientSymbolCreator, m_StateMachineSymbol, 4,
                m_Compiler->AddString(L"System"), NULL, NULL,
                m_Compiler->AddString(L"Runtime"), NULL, NULL,
                m_Compiler->AddString(L"CompilerServices"), NULL, NULL,
                m_Compiler->AddString(L"IAsyncStateMachine"), m_FXSymbolProvider->GetType(FX::IAsyncStateMachineType), NULL);
        m_TransientSymbolCreator->GetImplements(NULL, namedType->GetHashingName(), namedType, &implementsList);
    }

    return implementsList;
}



BCSYM_Variable* ResumableMethodLowerer::AddFieldToStateMachineClass(_In_z_ STRING* fieldName, BCSYM* fieldType)
{
    Variable* field = m_TransientSymbolCreator->AllocVariable(false, false);

    m_TransientSymbolCreator->GetVariable(
        NULL, // no location
        fieldName,
        fieldName,
        DECLF_Friend,
        VAR_Member,
        fieldType,
        NULL, // no const expression
        NULL, // not part of a symbol list
        field);

    Symbols::AddSymbolToHash(m_StateMachineSymbol->GetHash(), field, true, false, false);
    AssertIfNull(field);
    AssertIfFalse(field->GetContainingClass() == m_StateMachineSymbol);
    return field;
}


void ResumableMethodLowerer::AddMethodToStateMachineClass(BCSYM_Proc *alreadyAllocatedProc, _In_z_ STRING* methodName, Type* returnType, DECLFLAGS uFlags, _In_z_ STRING* parameterName, Type* parameterType)
{
    ThrowIfNull(alreadyAllocatedProc);
    ThrowIfNull(methodName);
    ThrowIfFalse( (parameterName!=NULL) == (parameterType!=NULL));

    BCSYM_Param *parameter = NULL;
    if (parameterName != NULL)
    {
        parameter = m_TransientSymbolCreator->GetParam(
                        NULL,
                        parameterName,
                        parameterType,
                        PARAMF_ByVal,
                        NULL,
                        NULL,
                        NULL,
                        false);
    }


    m_TransientSymbolCreator->GetProc(
        NULL, // no location
        methodName,
        methodName,
        NULL, // no code block location
        NULL, // no proc block location
        uFlags,
        returnType,
        parameter,
        NULL, // no return parameter
        NULL, // no lib name
        NULL, // no alias name
        SYNTH_TransientSymbol,
        NULL, // generic parameters
        NULL, // not part of a symbol list
        alreadyAllocatedProc);

    Symbols::AddSymbolToHash(m_StateMachineSymbol->GetHash(), alreadyAllocatedProc, true, false, false);
}


BCSYM_NamedRoot* ResumableMethodLowerer::AddLocalToProc(_In_z_ STRING* name, BCSYM_Proc* proc, Type* type, Location &loc)
{
    // WARNING: This function requires that the proc already be set up in "m_Semantics" as the current
    // proc we're working on (e.g. m_Semantics->m_Lookup has to point to the correct locals hash).
    ThrowIfFalse(name!=NULL && proc!=NULL && type!=NULL);

    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    ParseTree::IdentifierDescriptor id = ph.CreateIdentifierDescriptor(name);
    ParseTree::AlreadyBoundType *ptype = ph.CreateBoundType(type);

    ParseTree::VariableDeclarationStatement *DeclStatement = ph.CreateVariableDeclarationStatement(name, ptype, NULL);
    ParseTree::VariableDeclaration *Decl = ph.CreateVariableDeclaration(id);
    Decl->Type = ptype;
    //
    Declared::MakeLocalsFromDeclarationTrees(
        m_CompilerHost,
        &m_Semantics->m_TreeStorage,
        m_Semantics->m_CompilationCaches,
        m_Semantics->m_Errors,
        m_Semantics->m_SourceFile,
        proc,
        m_Semantics->m_Lookup,
        DeclStatement,
        Decl,
        false,  // not being called from ParseTreeService
        TypeResolveNoFlags,
        DECLF_NoFlags
        );

    // We want to get back a reference to the local we just created.
    // But MakeLocalsFromDeclarationTrees doesn't give us one directly.
    // So we workaround with the following grotty solution.
    ParseTree::Expression *DeclRef = ph.CreateNameExpression(id);
    ILTree::Expression *smfield = m_Semantics->InterpretExpression(DeclRef, ExprNoFlags);
    return smfield->AsSymbolReferenceExpression().Symbol;
}


void ResumableMethodLowerer::AddConstructorToStateMachineClass()
{
    BCSYM_Param* param = NULL;
    BCSYM_Proc* procCtor = Declared::BuildSyntheticConstructor(
        m_TransientSymbolCreator,
        m_Compiler,
        param,
        NULL, // don't need to be on an owning list
        Declared::SyntheticConstructorType::Instance);

    Symbols::AddSymbolToHash(m_StateMachineSymbol->GetHash(), procCtor, true, false, false);
    AssertIfNull(procCtor);
    AssertIfFalse(procCtor->GetContainingClass() == m_StateMachineSymbol);

    // Add the method body (as text to be parsed and interpreted later)
    StringBuffer code;
    code.AppendPrintf(WIDE("Me._State = %i\r\n"), INITIAL_STATE); // WARNING: This variable name also appears in StringConstants.h

    if (IsIterable)
    {
        // In .NET Core the Thread class doesn't exist. To get the managed thread id there is an equivalent property in System.Environment
        // for .NET 4.5 and above. If that property is not available (pre-4.5) then fallback to the old way.
        if (m_CompilerHost->GetSymbolForRuntimeMember(GetCurrentManagedThreadId, m_Semantics->m_CompilationCaches))
        {
            code.AppendString(WIDE("Me._InitialThreadId = Global.System.Environment.CurrentManagedThreadId\r\n")); // WARNING: This string also appears in StringConstants.h
        }
        else
        {
            code.AppendString(WIDE("Me._InitialThreadId = Global.System.Threading.Thread.CurrentThread.ManagedThreadId\r\n")); // WARNING: This string also appears in StringConstants.h
        }
    }

    Symbols::SetCode(
        procCtor->PSyntheticMethod(),
        code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
        code.GetStringLength());
}



void ResumableMethodLowerer::AddSetStateMachineMethodToStateMachineClass()
{
    ThrowIfFalse(IsFunctionAsync || IsSubAsync);

    BCSYM_SyntheticMethod* proc = m_TransientSymbolCreator->AllocSyntheticMethod();
    AddMethodToStateMachineClass(
        proc,
        m_Compiler->AddString(L"System.Runtime.CompilerServices.IAsyncStateMachine.SetStateMachine"),
        NULL, // Null here denotes a void returning method
        DECLF_Private | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed,
        m_Compiler->AddString(L"sm"),
        m_FXSymbolProvider->GetType(FX::IAsyncStateMachineType)
        );

    // implements IAsyncStateMachine.SetStateMachine
    BCSYM_Interface* IAsyncStateMachineInterface = m_FXSymbolProvider->GetType(FX::IAsyncStateMachineType)->PInterface();
    BCSYM_NamedRoot* IAsyncStateMachineSetStateMachine = IAsyncStateMachineInterface->SimpleBind(NULL, m_Compiler->AddString(L"SetStateMachine"));
    BCSYM_ImplementsList *ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"SetStateMachine"));
    ImplementsList->SetImplementedMember(IAsyncStateMachineSetStateMachine, NULL);
    proc->SetImplementsList(ImplementsList);

    StringBuffer code;
    code.AppendString(WIDE("Me._Builder.SetStateMachine(sm)\r\n")); // WARNING: this variable name is also declared in StringConstants.h:StateMachineClassFieldBuilder

    Symbols::SetCode(
        proc,
        code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
        code.GetStringLength());
}


void ResumableMethodLowerer::AddDisposeMethodToStateMachineClass()
{
    ThrowIfFalse(IsIterator || IsIterable);

    BCSYM_SyntheticMethod* proc = m_TransientSymbolCreator->AllocSyntheticMethod();
    AddMethodToStateMachineClass(
        proc,
        m_Compiler->AddString(L"System.IDisposable.Dispose"),
        NULL, // Null here denotes a void returning method
        DECLF_Private | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed
        );

    // implements IDisposable.Dispose
    BCSYM_Interface* IDisposableInterface = m_FXSymbolProvider->GetType(FX::IDisposableType)->PInterface();
    BCSYM_NamedRoot* IDisposableDispose = IDisposableInterface->SimpleBind(NULL, m_Compiler->AddString(L"Dispose"));    
    BCSYM_ImplementsList *ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"Dispose"));
    ImplementsList->SetImplementedMember(IDisposableDispose, NULL);
    proc->SetImplementsList(ImplementsList);

    StringBuffer code;
    VSASSERT(m_iStateMachineFinished != FINISHED_STATE_SENTINEL, "Expected iStateMachineFinished to have been already calculated by now");
    code.AppendString(WIDE("Me._Disposing = True\r\n")); // WARNING: this variable name is also declared in StringConstants.h:StateMachineClassFieldDisposing
    code.AppendString(WIDE("Me.MoveNext()\r\n"));
    code.AppendPrintf(WIDE("Me._State=%i\r\n"),m_iStateMachineFinished);

    Symbols::SetCode(
        proc,
        code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
        code.GetStringLength());
}


void ResumableMethodLowerer::AddResetMethodToStateMachineClass()
{
    ThrowIfFalse(IsIterator || IsIterable);

    BCSYM_SyntheticMethod* proc = m_TransientSymbolCreator->AllocSyntheticMethod();
    AddMethodToStateMachineClass(
        proc,
        m_Compiler->AddString(L"System.Collections.IEnumerator.Reset"),
        NULL, // Null here denotes a void returning method
        DECLF_Private | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed
        );

    // implements IEnumerator.Reset
    BCSYM_Interface* IEnumeratorInterface = m_FXSymbolProvider->GetType(FX::IEnumeratorType)->PInterface();
    BCSYM_NamedRoot* IEnumeratorReset = IEnumeratorInterface->SimpleBind(NULL, m_Compiler->AddString(L"Reset"));    
    BCSYM_ImplementsList *ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"Reset"));
    ImplementsList->SetImplementedMember(IEnumeratorReset, NULL);
    proc->SetImplementsList(ImplementsList);

    StringBuffer code;
    code.AppendString(WIDE("Throw New Global.System.NotSupportedException()\r\n"));

    Symbols::SetCode(
        proc,
        code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
        code.GetStringLength());
}


BCSYM_Property* ResumableMethodLowerer::AddCurrentPropertyToStateMachineClass(bool doGeneric)
{
    ThrowIfFalse(IsIterator || IsIterable);

    // What is the type that this property getter returns?
    BCSYM *propertyType = doGeneric ? m_CurrentField->GetType() : m_FXSymbolProvider->GetObjectType();

    // What is the thing we're implementing?
    BCSYM_Interface* IEnumeratorInterface;
    BCSYM_NamedRoot* IEnumeratorCurrent;
    BCSYM_ImplementsList* ImplementsList;
    GenericBinding* ImplementsBinding;
    if (doGeneric)
    {
        // implements IEnumerator`1.Current[propertyType]
        IEnumeratorInterface = m_FXSymbolProvider->GetType(FX::GenericIEnumeratorType)->PInterface();
        IEnumeratorCurrent = IEnumeratorInterface->SimpleBind(NULL, m_Compiler->AddString(L"Current"));
        ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"Current"));
        BCSYM** argArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
        argArray[0] = propertyType;
        ImplementsBinding = m_TransientSymbolCreator->GetGenericBinding(false, IEnumeratorInterface, argArray, 1, NULL);
    }
    else
    {
        // implements IEnumerator.Current
        IEnumeratorInterface = m_FXSymbolProvider->GetType(FX::IEnumeratorType)->PInterface();
        IEnumeratorCurrent = IEnumeratorInterface->SimpleBind(NULL, m_Compiler->AddString(L"Current"));
        ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"Current"));
        ImplementsBinding = NULL;
    }
    ImplementsList->SetImplementedMember(IEnumeratorCurrent, ImplementsBinding);

    // What is the name of the Current property, and its getter?
    StringBuffer propNameBuf, getNameBuf;
    IEnumeratorInterface->GetQualifiedName(&propNameBuf, false, 0, true, ImplementsBinding, 0, 0, true);
    propNameBuf.AppendString(L".Current");
    STRING *propName = m_Compiler->AddString(&propNameBuf);
    IEnumeratorInterface->GetQualifiedName(&getNameBuf, false, 0, true, ImplementsBinding, 0, 0, true);
    getNameBuf.AppendString(L".get_Current");
    STRING *getName = m_Compiler->AddString(&getNameBuf);


    SymbolList BindableSymbols;
    SymbolList UnBindableSymbols;

    // First, the "get_Current" method...
    BCSYM_SyntheticMethod *GetMethod = m_TransientSymbolCreator->AllocSyntheticMethod(true);
    m_TransientSymbolCreator->GetProc(
        NULL, // no location
        getName,
        getName,
        NULL, // no code block location
        NULL, // no proc block location
        DECLF_PropGet | DECLF_Private | DECLF_Hidden | DECLF_SpecialName | DECLF_OverridesKeywordUsed,
        propertyType,
        NULL, // parameters
        NULL, // no return parameter
        NULL, // no lib name
        NULL, // no alias name
        SYNTH_TransientSymbol,
        NULL, // generic parameters
        &UnBindableSymbols,
        GetMethod);

    StringBuffer code;
    code.AppendString(WIDE("Return Me._Current\r\n")); // WARNING: this variable name is also declared in StringConstants.h:StateMachineClassFieldCurrent
    Symbols::SetCode(
        GetMethod,
        code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
        code.GetStringLength());

    // Next, fill out the *SIGNATURE* of the "Current" property...
    Property *PropertySymbol = m_TransientSymbolCreator->AllocProperty(true);
    m_TransientSymbolCreator->GetProc(
        NULL,
        propName,
        propName,
        NULL, // no code block location
        NULL, // no proc block location
        DECLF_Private | DECLF_Function | DECLF_HasRetval,
        propertyType,
        NULL, // parameters
        NULL, // no return parameter
        NULL, // no lib name
        NULL, // no alias name
        SYNTH_TransientSymbol,
        NULL,
        NULL, // not added to either Bindable nor UnBindableSymbolList
        PropertySymbol->GetPropertySignature()); 

    // Next, fill out the remainder of the "CURRENT" property...
    m_TransientSymbolCreator->GetProperty(
        NULL,
        m_Compiler->AddString(L"Current"),
        DECLF_Private | DECLF_ShadowsKeywordUsed | DECLF_ReadOnly | DECLF_OverridesKeywordUsed,
        GetMethod,
        NULL,   // no SetMethod
        PropertySymbol,
        &BindableSymbols);

    PropertySymbol->SetIsFromAnonymousType(false);


    // implements...
    PropertySymbol->SetImplementsList(ImplementsList);

    Symbols::AddSymbolListToHash(m_StateMachineSymbol->GetHash(), &BindableSymbols, true);
    Symbols::AddSymbolListToHash(m_StateMachineSymbol->GetUnBindableChildrenHash(), &UnBindableSymbols, true);

    return PropertySymbol;
}


BCSYM_Proc* ResumableMethodLowerer::AddGetEnumeratorMethodToStateMachineClass(bool doGeneric)
{
    ThrowIfFalse(IsIterable);

    BCSYM *returnType; // the return type of this GetEnumerator function
    if (doGeneric)
    {
        // IEnumerator<T>.
        BCSYM_Interface* GenericIEnumeratorInterface = m_FXSymbolProvider->GetType(FX::GenericIEnumeratorType)->PInterface();
        BCSYM** argArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
        argArray[0] = m_CurrentField->GetType();
        returnType = m_TransientSymbolCreator->GetGenericBinding(false, GenericIEnumeratorInterface, argArray, 1, NULL);
    }
    else
    {
        // IEnumerator.
        returnType = m_FXSymbolProvider->GetType(FX::IEnumeratorType);
    }


    // which methods do we implement?
    BCSYM_Interface* IEnumerableInterface;
    BCSYM_ImplementsList* ImplementsList;
    BCSYM_NamedRoot* IEnumerableGetEnumerator;
    GenericBinding* ImplementsBinding;
    if (doGeneric)
    {
        // implements IEnumerable`1[T].GetEnumerator
        IEnumerableInterface = m_FXSymbolProvider->GetType(FX::GenericIEnumerableType)->PInterface();
        IEnumerableGetEnumerator = IEnumerableInterface->SimpleBind(NULL, m_Compiler->AddString(L"GetEnumerator"));
        ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"GetEnumerator"));
        BCSYM** argArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*)));
        argArray[0] = m_CurrentField->GetType();
        ImplementsBinding = m_TransientSymbolCreator->GetGenericBinding(false, IEnumerableInterface, argArray, 1, NULL);
    }
    else
    {
        // implements IEnumerable.GetEnumerator
        IEnumerableInterface = m_FXSymbolProvider->GetType(FX::IEnumerableType)->PInterface();
        IEnumerableGetEnumerator = IEnumerableInterface->SimpleBind(NULL, m_Compiler->AddString(L"GetEnumerator"));
        ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"GetEnumerator"));
        ImplementsBinding = NULL;
    }
    ImplementsList->SetImplementedMember(IEnumerableGetEnumerator, ImplementsBinding);


    // Make the method...
    BCSYM_SyntheticMethod* proc = m_TransientSymbolCreator->AllocSyntheticMethod();
    AddMethodToStateMachineClass(
        proc,
        m_Compiler->AddString(doGeneric ? L"GetEnumerator" : L"GetEnumerator0"),
        returnType,
        DECLF_Function | DECLF_Private | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed
        );

    // implements clause
    proc->SetImplementsList(ImplementsList);


    if (!doGeneric)
    {
        StringBuffer code;
        code.AppendString(WIDE("Return Me.GetEnumerator()\r\n"));
        // We called the generic one "GetEnumerator" and the non-generic one "GetEnumerator0", so this will definitely find the right one

        Symbols::SetCode(
            proc,
            code.AllocateBufferAndCopy(m_TransientSymbolCreator->GetNorlsAllocator()),
            code.GetStringLength());

        return proc;
    }

    // =============

    // For the generic case...
    // It's too fragile to generate the following code as "StringBuffer code".
    // That's because it refers to lots of variables by name, and to the classname itself,
    // and we want to be sure we bind to the right things. e.g. if we do the substitution _ -> $,
    // what if the user had _ in their names? what if we had a variable named $$Me? Can't do it.

    Location loc = Location::GetHiddenLocation();

    ILTree::ProcedureBlock *block = &m_Semantics->AllocateStatement(SB_PROC, loc, 0, false)->AsProcedureBlock();
    block->pproc = proc;
    block->pproc->SetBoundTree(block);
    m_Semantics->m_Procedure = block->pproc;
    m_Semantics->m_ProcedureTree = block;

    Scope *LocalsAndParameters = NULL;
    Variable *FirstParamVar = NULL;
    Declared::MakeLocalsFromParams(m_Compiler, &m_Semantics->m_TreeStorage, proc, m_Semantics->m_Errors, false, &FirstParamVar, &LocalsAndParameters);
    block->Locals = LocalsAndParameters;
    m_Semantics->m_Lookup = LocalsAndParameters;
    if (block->pproc->GetType()!=NULL && !block->pproc->GetType()->IsVoidType())
    {
        Variable *ReturnVariable = Declared::MakeReturnLocal(m_Compiler, &m_Semantics->m_TreeStorage, block->pproc, NULL);
        Symbols::AddSymbolToHash(block->Locals, ReturnVariable, true, false, false);
        block->ReturnVariable = ReturnVariable;
    }
    else
    {
        VSFAIL("The GetEnumerator method should have had a return variable");
    }


    block->ptempmgr = new(m_Semantics->m_TreeStorage) TemporaryManager(m_Compiler, m_CompilerHost, block->pproc, &m_Semantics->m_TreeStorage);
    m_Semantics->m_TemporaryManager = block->ptempmgr;

    m_Semantics->m_ContainingClass = m_StateMachineSymbol;
    m_Semantics->m_ContainingContainer = m_StateMachineSymbol;

    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    StatementListBuilder bldr(&block->Child);
    ParseTree::Expression *pLHS, *pRHS;
    ILTree::Statement *stmt;

    Symbol *StateMachineVariable = AddLocalToProc(STRING_CONST(m_Compiler, StateMachineVariableName), m_MainProc, m_StateMachineType, loc);

    // If Global.System.Threading.Thread.CurrentThread.ManagedThreadId = Me._InitialThreadId AndAlso Me._State = ENUMERABLE_NOT_YET_STARTED Then
    //     Me.$State=INITIAL_STATE : StateMachineVariable=Me
    // Else
    //     StateMachineVariable=new StateMachineClass() : [StateMachineVariable.$Me = Me.$Me]
    // EndIf
    //
    if (m_InitialThreadField != NULL && m_StateField != NULL && StateMachineVariable != NULL)
    {
        // [1] Condition:
        
        // In .NET Core the Thread class doesn't exist. To get the managed thread id there is an equivalent property in System.Environment
        // for .NET 4.5 and above. If that property is not available (pre-4.5) then fallback to the old way.
        if (m_CompilerHost->GetSymbolForRuntimeMember(GetCurrentManagedThreadId, m_Semantics->m_CompilationCaches))
        {
            pLHS = ph.CreateQualifiedNameExpression(ph.CreateGlobalNameSpaceExpression(), 
                                                    3, 
                                                    m_Compiler->AddString(L"System"), 
                                                    m_Compiler->AddString(L"Environment"), 
                                                    m_Compiler->AddString(L"CurrentManagedThreadId"));
        }
        else
        {
            pLHS = ph.CreateQualifiedNameExpression(ph.CreateGlobalNameSpaceExpression(), 
                                                    5, 
                                                    m_Compiler->AddString(L"System"), 
                                                    m_Compiler->AddString(L"Threading"), 
                                                    m_Compiler->AddString(L"Thread"), 
                                                    m_Compiler->AddString(L"CurrentThread"), 
                                                    m_Compiler->AddString(L"ManagedThreadId"));
        }

        pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_InitialThreadField);
        ParseTree::Expression* pCondition1 = ph.CreateBinaryExpression(ParseTree::Expression::Equal, pLHS, pRHS);
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(ENUMERABLE_NOT_YET_STARTED);
        ParseTree::Expression* pCondition2 = ph.CreateBinaryExpression(ParseTree::Expression::Equal, pLHS, pRHS);
        ILTree::Expression* condition = m_Semantics->InterpretExpression(ph.CreateBinaryExpression(ParseTree::Expression::AndAlso, pCondition1, pCondition2), ExprNoFlags);
        //
        // [2] Then:
        ILTree::Statement* thenBranch = NULL;
        StatementListBuilder thenBldr(&thenBranch);
        // Me.State = INITIAL_STATE
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(INITIAL_STATE);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&thenBldr, stmt);
        // StateMachineVariable = Me
        pLHS = ph.CreateBoundSymbol(StateMachineVariable);
        pRHS = ph.CreateMeReference();
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&thenBldr, stmt);
        //
        // [3] Else:
        ILTree::Statement* elseBranch = NULL;
        StatementListBuilder elseBldr(&elseBranch);
        // StateMachineVariable = New StateMachineClass
        pLHS = ph.CreateBoundSymbol(StateMachineVariable);
        pRHS = ph.CreateNewObject(ph.CreateBoundType(m_StateMachineType));
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&elseBldr, stmt);
        // [StateMachineVariablle.$Me = Me.$Me]
        if (m_MeField!=NULL)
        {
            VSASSERT(!m_MainProc->IsShared(), "How can we have a 'Me' field for a shared resumable method?");
            pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_MeField);
            pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_MeField);
            stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
            RobustAdd(&elseBldr, stmt);
        }
        //
        // [4] Make the IF
        stmt = MakeIf(loc, condition, thenBranch, elseBranch);
        RobustAdd(&bldr, stmt);
    }

    ThrowIfFalse(m_ParameterFields.Count() == m_ParameterProtoFields.Count());
    for (unsigned int i=0; i<m_ParameterFields.Count(); i++)
    {
        // StateMachineVariable.copyParam = StateMachineVariable.protoParam
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_ParameterFields.Element(i));
        pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_ParameterProtoFields.Element(i));
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // Return StateMachineVariable
    pRHS = ph.CreateBoundSymbol(StateMachineVariable);
    ILTree::ReturnStatement *Return = &m_Semantics->AllocateStatement(SL_RETURN, loc, 0, false)->AsReturnStatement();
    Return->ReturnExpression = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
    RobustAdd(&bldr, Return);

    
    return proc;
}

Location ResumableMethodLowerer::GetMainProcDebugBeginLocation()
{
    Location loc = CodeGenerator::GetDebugBeginLocation(m_OriginalBlock);

    if (!loc.IsValid())
    {
        loc = Location::GetHiddenLocation();
    }

    return loc;
}

Location ResumableMethodLowerer::GetMainProcDebugEndLocation()
{
    Location loc = CodeGenerator::GetDebugEndLocation(m_OriginalBlock);

    if (!loc.IsValid())
    {
        loc = Location::GetHiddenLocation();
    }

    return loc;
}


void ResumableMethodLowerer::AddMoveNextMethodToStateMachineClass()
{
    Location loc = Location::GetHiddenLocation();

    m_moveNextMethod = m_TransientSymbolCreator->AllocSyntheticMethod();

    AddMethodToStateMachineClass(m_moveNextMethod,
                                 m_Compiler->AddString(L"MoveNext"),
                                 (IsIterator || IsIterable) ? m_FXSymbolProvider->GetBooleanType() : NULL,
                                 DECLF_Friend | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed);
    
    m_moveNextMethod->SetIsResumable();

    // implements [IEnumerator|IAsyncStateMachine].MoveNext
    BCSYM_Interface* IEnumeratorInterface = m_FXSymbolProvider->GetType((IsIterator||IsIterable) ? FX::IEnumeratorType : FX::IAsyncStateMachineType)->PInterface();
    BCSYM_NamedRoot* IEnumeratorMoveNext = IEnumeratorInterface->SimpleBind(NULL, m_Compiler->AddString(L"MoveNext"));    
    BCSYM_ImplementsList *ImplementsList = m_TransientSymbolCreator->GetImplementsList(NULL, m_StateMachineSymbol, m_Compiler->AddString(L"MoveNext"));
    ImplementsList->SetImplementedMember(IEnumeratorMoveNext, NULL);
    m_moveNextMethod->SetImplementsList(ImplementsList);

    // Set the location so that we can update it during EnC.
    m_moveNextMethod->SetWholeSpan(m_MainProcLoc);

    ILTree::ProcedureBlock *block = &m_Semantics->AllocateStatement(SB_PROC, loc, 0, false)->AsProcedureBlock();
    block->pproc = m_moveNextMethod;
    m_moveNextMethod->SetBoundTree(block);
    m_moveNextMethod->SetJITOptimizationsMustBeDisabled(m_MainProc->JITOptimizationsMustBeDisabled());
    m_Semantics->m_Procedure = block->pproc;
    m_Semantics->m_ProcedureTree = block;

    block->ptempmgr = new(m_Semantics->m_TreeStorage) TemporaryManager(m_Compiler, m_CompilerHost, block->pproc, &m_Semantics->m_TreeStorage);
    m_Semantics->m_TemporaryManager = block->ptempmgr;

    Scope *LocalsAndParameters = NULL;
    Variable *FirstParamVar = NULL;
    Declared::MakeLocalsFromParams(
        m_Compiler,
        &m_Semantics->m_TreeStorage,
        block->pproc,
        m_Semantics->m_Errors,
        false,
        &FirstParamVar,
        &LocalsAndParameters);
    block->Locals = LocalsAndParameters;
    m_Semantics->m_Lookup = LocalsAndParameters;

    if (block->pproc->GetType()!=NULL && !block->pproc->GetType()->IsVoidType())
    {
        VSASSERT(IsIterator || IsIterable, "Expected a return-type only on MoveNext of iterator/iterable methods");
        VSASSERT(TypeHelpers::EquivalentTypes(m_moveNextMethod->GetType(), m_FXSymbolProvider->GetBooleanType()), "Expected return-type of MoveNext to be boolean");
        Variable *ReturnVariable = Declared::MakeReturnLocal(m_Compiler, &m_Semantics->m_TreeStorage, block->pproc, NULL);
        Symbols::AddSymbolToHash(block->Locals, ReturnVariable, true, false, false);
        block->ReturnVariable = ReturnVariable;
    }
    else
    {
        VSASSERT(!IsIterator && !IsIterable, "Expected iterator/iterable MoveNext to have a return type");
    }

    m_DoFinallyBodiesLocal = AddLocalToProc(STRING_CONST(m_Compiler, StateMachineDoFinallyBodies), block->pproc, m_FXSymbolProvider->GetBooleanType(), loc);

    ILTree::Statement* pReturnLabelStatement = MakeLabel(L"return", 0, NULL, &m_ReturnLabel);

    if (IsFunctionAsync && m_ResumableGenericType != NULL)
    {
        m_ReturnTempLocal = AddLocalToProc(STRING_CONST(m_Compiler, StateMachineReturnTemp),
                                           block->pproc,
                                           RewriteType(m_ResumableGenericType),
                                           loc);
    }

    // Also must copy all "LifetimeDefaultValue" local temporaries into MoveNext. See CodeGenerator::GenerateConvertClass
    //
    // Here we only copy LifetimeDefaultValue temporaries to Movenext. Other temporaries are lifted to the display class,
    // then we don't need to keep them. But in the future if we decide to do some optimization about variable lifting, then 
    // we need to copy the temporary which is not lifted. The reason we need to keep not lifted temporaries is that AssignLocalSlots
    // needs to assign a slot for each not lifted temporaries.
    TemporaryIterator temps;
    temps.Init(m_OriginalBlock->ptempmgr);
    while (Temporary *temp = temps.GetNext())
    {
        if (temp->Lifetime != LifetimeDefaultValue) continue;
        block->ptempmgr->AllocateDefaultValueTemporary(RewriteType(temp->Symbol->GetType()), temp->Symbol->GetLocation());
        m_TemporaryAccounting.Account(temp->Symbol);
    }

    m_Semantics->m_ContainingClass = m_StateMachineSymbol;
    m_Semantics->m_ContainingContainer = m_StateMachineSymbol;

    CSingleList<LabelTarget> targets;
    m_CurrentScopeTargets = &targets;




    // ====================================================================
    // REWRITE THE ORIGINAL FUNCTION BODY INTO A MOVENEXT BODY
    //
    ILTree::Statement *rewrittenBody = RewriteStatementList(m_OriginalBlock->Child);
    // ====================================================================



    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    StatementListBuilder bldr(&block->Child);
    ParseTree::Expression *pLHS, *pRHS;
    ILTree::Statement *stmt;


    // We'll use the next available state number for "past the end", so iterators can jump to this
    // if someone called MoveNext when they were past the end. (async methods don't need bother with this).
    VSASSERT(m_iStateMachineFinished == FINISHED_STATE_SENTINEL, "Expected that iStateMachineFinished wouldn't have been computed yet");
    m_iStateMachineFinished = ++m_HighestStateUsed;
    LabelDeclaration *pAbortLabel = NULL;
    ILTree::Statement* pAbortLabelStatement = MakeLabel(L"abort", m_iStateMachineFinished, NULL, &pAbortLabel);
    //
    if (IsIterator || IsIterable)
    {
        LabelTarget *labtarg = m_Semantics->m_TreeStorage.Alloc<LabelTarget>();
        labtarg->state = m_iStateMachineFinished;
        labtarg->target = pAbortLabel;
        m_CurrentScopeTargets->InsertLast(labtarg);
    }


    // doFinallyBodies = True
    if (m_DoFinallyBodiesLocal != NULL)
    {
        pLHS = ph.CreateBoundSymbol(m_DoFinallyBodiesLocal);
        pRHS = ph.CreateBoolConst(true);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // Try...
    ILTree::TryBlock *OuterTry = &m_Semantics->AllocateStatement(SB_TRY, loc, NoResume, false)->AsTryBlock();
    m_Semantics->EstablishBlockContext(OuterTry);
    OuterTry->EndConstructLocation = loc;
    RobustAdd(&bldr, OuterTry);
    bldr.m_ppLastStatement = &OuterTry->Child;

    // switch/jump-table based on state
    bldr.AddList(MakeSwitchFromCurrentScope());

    if ((IsIterator || IsIterable) && m_DisposingField != NULL)
    {
        // If Me.Disposing Then AbortLabel: Return False
        ILTree::Expression *condition = m_Semantics->InterpretExpression(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_DisposingField), ExprNoFlags);
        //
        ILTree::Statement* thenBranch = NULL;
        StatementListBuilder thenBldr(&thenBranch);
        RobustAdd(&thenBldr, pAbortLabelStatement);
        ILTree::ReturnStatement *returnStatement = &m_Semantics->AllocateStatement(SL_RETURN, loc, NoResume, false)->AsReturnStatement();
        returnStatement->uFlags |= SLF_EXITS_TRY;
        returnStatement->ReturnExpression = m_Semantics->InterpretExpression(ph.CreateBoolConst(false), ExprNoFlags);
        RobustAdd(&thenBldr, returnStatement);
        //
        stmt = MakeIf(loc, condition, thenBranch, NULL);
        RobustAdd(&bldr, stmt);
    }


    //==============================================================================================
    // body
    
    // First add nop and sequence point for the beginning of the async method.  Putting the first
    // sequence point here instead of at IL offset 0x0 ensures that a breakpoint set on the
    // beginning of the method will only be hit when the async method is called for the first time,
    // and not on subsequent resumptions of the method.  Further, this is necessary for Step Over
    // Async to funtion properly, because it sets a breakpoint at IL offset 0 and relies on the
    // switch table jump code to be hidden so that it doesn't stop until the instruction pointer
    // is actually past the resume IL offset.
    stmt = m_Semantics->AllocateStatement(SL_DEBUGLOCNOP, GetMainProcDebugBeginLocation(), NoResume, false);
    RobustAdd(&bldr, stmt);
    
    // Then the user's code
    bldr.AddList(rewrittenBody);

    // GetMainProcDebugEndLocation() doesn't get added immediately after the user body.  Instead
    // we add it after the ReturnLabel so that stepping over any return statement will hit the
    // sequence point corresponding to the end of the method.

    //==============================================================================================


    // Goto ReturnLabel
    ILTree::GotoStatement *gotoStmt = &m_Semantics->AllocateStatement(SL_GOTO, loc, NoResume, false)->AsGotoStatement();
    gotoStmt->Label = m_ReturnLabel;
    gotoStmt->uFlags = SLF_EXITS_TRY;
    RobustAdd(&bldr, gotoStmt);


    m_Semantics->PopBlockContext();

    // Track the original methods locals so we can show them in the correct scopes while debugging.
    // Do this after Popping the previous context since doing so attempts to reset m_Semantics->m_Lookup
    // which is not valid as m_OriginalBlock is a top-level method block..
    //
    // Add them to block->Locals (instead of to OuterTry->Locals) so that the variables
    // will show up in the debugger locals/watch windows when the instruction pointer is
    // on the end construct of the method.  The sequence point for the end construct is
    // past the OuterTry block, hence this requirement.
    BCITER_HASH_CHILD outerScopeLocalsIterator(m_OriginalBlock->Locals);
    while ( BCSYM_NamedRoot *pOriginalLocal = outerScopeLocalsIterator.GetNext())
    {
        if ( pOriginalLocal->IsVariable() && pOriginalLocal->PVariable()->GetRewrittenName() )
        {
            Variable* pLocal = m_TransientSymbolCreator->AllocVariable(false, false);

            m_TransientSymbolCreator->GetVariable(
                NULL, // no location
                pOriginalLocal->GetName(),
                pOriginalLocal->GetName(),
                DECLF_Friend,
                pOriginalLocal->PVariable()->GetVarkind(),
                RewriteType(pOriginalLocal->PVariable()->GetType()),
                NULL, // no const expression
                NULL, // not part of a symbol list
                pLocal);

            pLocal->PVariable()->SetRewrittenName(pOriginalLocal->PVariable()->GetRewrittenName());
            Symbols::AddSymbolToHash(block->Locals, pLocal, true, false, false);
        }
    }

    bldr.m_ppLastStatement = &OuterTry->Next;

    // Catch ex As Exception
    ILTree::CatchBlock *OuterCatch = &m_Semantics->AllocateStatement(SB_CATCH, loc, NoResume, false)->AsCatchBlock();
    if (IsSubAsync)
    {
        // Record this catch handler's IL start offset in the PDB for
        // special debugger handling of this catch handler.
        OuterCatch->uFlags = SBF_CATCH_ASYNCSUB;
    }
    RobustAdd(&bldr, OuterCatch);
    bldr.m_ppLastStatement = &OuterCatch->Child;
    m_Semantics->EstablishBlockContext(OuterCatch);
    OuterCatch->EndConstructLocation = Location::GetHiddenLocation();
    block->fSeenCatch = true;
    //
    // Locals for the catch block. (The only local is "ex" itself)
    OuterCatch->Locals = m_TransientSymbolCreator->GetHashTable(NULL, m_Semantics->m_Lookup, true, 1, NULL);
    m_Semantics->m_Lookup = OuterCatch->Locals;
    //
    STRING *exName = STRING_CONST(m_Compiler, StateMachineExceptionName);
    ParseTree::IdentifierDescriptor id = ph.CreateIdentifierDescriptor(exName);
    ParseTree::AlreadyBoundType *pType = ph.CreateBoundType(m_FXSymbolProvider->GetType(FX::ExceptionType));
    ParseTree::VariableDeclarationStatement *pDecl = ph.CreateVariableDeclarationStatement(exName, pType, NULL);
    BCSYM_NamedRoot *catchVariable = NULL;
    Declared::MakeLocalsFromTrees(m_CompilerHost, &m_Semantics->m_TreeStorage, m_Semantics->m_CompilationCaches, m_Semantics->m_Errors,
        m_MainContainerSymbol, m_Semantics->m_SourceFile, block->pproc,
        m_Semantics->m_Lookup, pDecl, false, TypeResolveNoFlags, &catchVariable);
    OuterCatch->CatchVariableTree = m_Semantics->ReferToSymbol(loc, catchVariable, chType_NONE, NULL,
                        IsGenericOrHasGenericParent(catchVariable->GetParent()) ? SynthesizeOpenGenericBinding(catchVariable->GetParent(), *m_TransientSymbolCreator) : NULL,
                        ExprNoFlags);

    // <Me.stateField> = <m_iStateMachineFinished>
    if (m_StateField != NULL)
    {
        VSASSERT(m_iStateMachineFinished != FINISHED_STATE_SENTINEL, "Expected iStateMachineFinished to have been already calculated by now");
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(m_iStateMachineFinished);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    if ((IsFunctionAsync || IsSubAsync) && m_BuilderField != NULL)
    {
        // <Me.builderField>.SetException(ex)
        BCSYM *method = Bind(m_BuilderField->GetType(), NULL, m_Compiler->AddString(L"SetException"));
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_BuilderField), method), ph.CreateArgList(ph.CreateBoundSymbol(catchVariable)), loc);
        ILTree::StatementWithExpression* CallStatement = &m_Semantics->AllocateStatement(SL_STMT, loc, 0, false)->AsStatementWithExpression();
        CallStatement->Operand1 = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, CallStatement);

        // Return
        ILTree::ReturnStatement *retStmt = &m_Semantics->AllocateStatement(SL_RETURN, loc, NoResume, false)->AsReturnStatement();
        retStmt->uFlags = SLF_EXITS_CATCH;
        RobustAdd(&bldr, retStmt);
    }

    if (IsIterator || IsIterable)
    {
        // Throw
        stmt = m_Semantics->AllocateStatement(SL_ERROR, loc, 0, false);
        RobustAdd(&bldr, stmt);
    }

    m_Semantics->PopBlockContext();
    bldr.m_ppLastStatement = &OuterCatch->Next;


    // ReturnLabel:
    RobustAdd(&bldr, pReturnLabelStatement);
    
    // <Me.stateField> = <m_iStateMachineFinished>
    if (m_StateField != NULL)
    {
        VSASSERT(m_iStateMachineFinished != FINISHED_STATE_SENTINEL, "Expected iStateMachineFinished to have been already calculated by now");
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(m_iStateMachineFinished);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        stmt->Loc = GetMainProcDebugEndLocation();
        RobustAdd(&bldr, stmt);
    }

    if ((IsFunctionAsync || IsSubAsync) && m_BuilderField != NULL)
    {
        // <Me.builderField>.SetResult( [ReturnTempLocal] )
        BCSYM *method = Bind(m_BuilderField->GetType(), NULL, m_Compiler->AddString(L"SetResult"));
        ParseTree::ArgumentList *pArgList = (m_ReturnTempLocal == NULL) ? NULL : ph.CreateArgList(ph.CreateBoundSymbol(m_ReturnTempLocal));
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_BuilderField), method), pArgList, loc);
        ILTree::StatementWithExpression* CallStatement = &m_Semantics->AllocateStatement(SL_STMT, loc, 0, false)->AsStatementWithExpression();
        CallStatement->Operand1 = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, CallStatement);
    }

    if (IsIterator || IsIterable)
    {
        // Return False
        pRHS = ph.CreateBoolConst(false);
        stmt = m_Semantics->AllocateStatement(SL_RETURN, loc, NoResume, false);
        stmt->uFlags |= SLF_EXITS_TRY;
        stmt->AsReturnStatement().ReturnExpression = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, stmt);
    }



    MarkContainsAsyncSpill awaitVisitor(*m_Semantics->GetTransientSymbols()->GetNorlsAllocator());
    awaitVisitor.Visit(m_moveNextMethod->GetBoundTree());
}


int _cdecl compare_labeltargets(const void *arg1, const void *arg2)
{
    LabelTarget *target1 = (LabelTarget*)arg1;
    LabelTarget *target2 = (LabelTarget*)arg2;
    return target1->state - target2->state;
}

ILTree::Statement* ResumableMethodLowerer::MakeSwitchFromCurrentScope()
{
    ThrowIfNull(m_CurrentScopeTargets);

    Location loc = Location::GetHiddenLocation();
    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);

    if (m_CurrentScopeTargets->IsEmpty()) return NULL;
    if (m_StateField == NULL) return NULL;

    unsigned long cEntries = m_CurrentScopeTargets->NumberOfEntries();
    int min = m_CurrentScopeTargets->GetFirst()->state;
    LABEL_ENTRY_BIL ** labels = reinterpret_cast<LABEL_ENTRY_BIL**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(VBMath::Multiply(cEntries, sizeof(LABEL_ENTRY_BIL*))));

    CSingleListIter<LabelTarget>  titer(m_CurrentScopeTargets);
    LabelTarget *labtarg = NULL;
    for (unsigned long i=0; NULL != (labtarg = titer.Next()); i++)
    {
        VSASSERT(labtarg->state == min+i, "Expected labels to be in contiguous order");
        labels[i] = labtarg->target;
    }

    ILTree::AsyncSwitchStatement *switchStmt = &m_Semantics->AllocateStatement(SL_ASYNCSWITCH, loc, NoResume, false)->AsAsyncSwitchStatement();
    ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
    if (min != 0)
    {
        pRHS = ph.CreateBinaryExpression(ParseTree::Expression::Minus, pRHS, ph.CreateIntConst(min));
    }
    switchStmt->NormalizedSelector = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
    switchStmt->cEntries = cEntries;
    switchStmt->Labels = labels;
    return switchStmt;

}


ILTree::Statement* ResumableMethodLowerer::MakeLabel(_In_z_ wchar_t *prefix, int i, _In_opt_ CSingleList<LabelTarget> *insert, _In_opt_ LabelDeclaration **decl)
{
    ThrowIfNull(prefix);

    // MAKELABEL:
    // This function creates a label with name "prefix%i_stateMachineNamePrefix"
    // If "decl" parameter is non-null, then it returns the LabelDeclaration here.
    // If "insert" paremeter is non-null, then it appends "i:labdecl" to the insert list.
    // It returns the LabelStatement it made.

    // Pick a name...
    StringBuffer namebuffer;
    namebuffer.AppendPrintf(L"%s%i_",prefix,i);
    namebuffer.AppendSTRING(m_stateMachineNamePrefix);
    STRING* labelName = m_Compiler->AddString(&namebuffer);

    // Create the LabelDeclaration...
    ParserHelper ph(&m_Semantics->m_TreeStorage, Location::GetHiddenLocation());
    ParseTree::LabelReferenceStatement *labref = new (m_Semantics->m_TreeStorage) ParseTree::LabelReferenceStatement;
    labref->Label = ph.CreateIdentifierDescriptor(labelName);
    LabelDeclaration *labdec = m_Semantics->GetLabel(labref, true, NULL, NULL, NULL);
    if (decl != NULL)
    {
        *decl = labdec;
    }
    
    // Append "i:labdec" to the list...
    if (insert != NULL)
    {
        LabelTarget *labtarg = m_Semantics->m_TreeStorage.Alloc<LabelTarget>();
        labtarg->state = i;
        labtarg->target = labdec;
        insert->InsertLast(labtarg);
    }

    // Return the LabelStatement for the label we've just made...
    ILTree::LabelStatement *labStmt = &m_Semantics->AllocateStatement(SL_LABEL, Location::GetHiddenLocation(), NoResume, false)->AsLabelStatement();
    labStmt->Label = labdec;
    labStmt->Label->IsDefined = true;
    labStmt->Label->pLabelParent = labStmt;
    return labStmt;
}


ILTree::Statement* ResumableMethodLowerer::MakeIf(Location loc, _In_ ILTree::Expression *condition, _In_ ILTree::Statement* thenBranch, _In_opt_ ILTree::Statement* elseBranch)
{
    // Build the block for the if statement
    ILTree::IfGroup *ifBlk = &m_Semantics->AllocateStatement(SB_IF_BLOCK, loc, NoResume, false)->AsIfGroup();
    ifBlk->EndConstructLocation = Location::GetHiddenLocation();
    
    // Then branch
    ILTree::IfBlock *ifStmt = &m_Semantics->AllocateStatement(SB_IF, loc, NoResume, false)->AsIfBlock();
    ifStmt->Conditional = condition;
    ifBlk->Child = ifStmt;
    ifStmt->Parent = ifBlk;
    if (thenBranch != NULL) thenBranch->Parent = ifStmt;
    ifStmt->Child = thenBranch;

    // Else branch
    if (elseBranch != NULL)
    {
        ILTree::IfBlock *elseStmt = &m_Semantics->AllocateStatement(SB_ELSE_IF, loc, NoResume, false)->AsIfBlock();
        ifStmt->Next = elseStmt;
        elseStmt->Parent = ifBlk;
        elseStmt->Prev = ifStmt;
        elseBranch->Parent = elseStmt;
        elseStmt->Child = elseBranch;
    }
    
    return ifBlk;
}



ILTree::Expression* ResumableMethodLowerer::RewriteExpression(_In_ ILTree::Expression *pExpr)
{
    if (pExpr == NULL)
    {
        VSFAIL("Shouldn't attempt to rewrite a NULL expression.");
        return NULL;
    }
    if (IsBad(pExpr))
    {
        return pExpr;
    }

    if (pExpr->RewrittenByStateMachine != NULL) return pExpr->RewrittenByStateMachine;
    // The "RewrittenByStateMachine" is because some IL-trees are actually
    // DAGs not trees -- e.g. the binary coalesce operator IF(a,b), or XML-literals

    ILTree::Expression *Result;
    if (pExpr->bilop == SX_AWAIT) Result = RewriteAwaitExpression(&pExpr->AsAwaitExpression());
    else if (pExpr->bilop == SX_SYM) Result = RewriteSymbolExpression(&pExpr->AsSymbolReferenceExpression());
    else if (pExpr->bilop == SX_CTYPE || pExpr->bilop == SX_TRYCAST || pExpr->bilop == SX_DIRECTCAST) Result = RewriteCast(&pExpr->AsBinaryExpression());
    else 
    {   

        ILTree::CallExpression* pCallExp = pExpr->bilop == SX_CALL ?  &(pExpr->AsCallExpression()) : nullptr;
        if (pCallExp && 
            pCallExp->Left != nullptr &&
            pCallExp->Left->bilop == SX_SYM && 
            pCallExp->Left->AsSymbolReferenceExpression().Symbol &&
            pCallExp->Left->AsSymbolReferenceExpression().Symbol->IsMethodImpl() &&         
            (! wcscmp(pCallExp->Left->AsSymbolReferenceExpression().Symbol->PMethodImpl()->GetName(), L"op_Implicit") ||
            ! wcscmp(pCallExp->Left->AsSymbolReferenceExpression().Symbol->PMethodImpl()->GetName(), L"op_Explicit")))
        {
            m_RewritingOpImplicit = true;
        }

        Result = BoundTreeRewriter::RewriteExpression(pExpr);

        if (m_RewritingOpImplicit)
        {
            m_RewritingOpImplicit = false;
        }

    }


#ifdef DEBUG
    //VSASSERT(TypeHelpers::EquivalentTypes(Result->ResultType, RewriteType(pExpr->ResultType)), "Unexpected async expr rewrite: it didn't also rewrite the type");
    //
    // This assertion is correct in principle, but I've commented it out because of shortcomings elsewhere in the compiler...
    //
    // EXPLANATION:
    // 1. What is the correct Result->ResultType?
    //    Answer: it should be what you get from RewriteType(pExpr->ResultType).
    //    And it should also be the natural type of that rewritten expression according to the VB language spec.
    //
    // 2. Should we force it to be that, i.e. "Result->ResultType = RewriteType(pExpr->ResultType)"?
    //    Answer: no, there's no need, since each of the three methods we call (RewriteAwaitExpression, RewriteSymbolExpression, RewriteExpression)
    //    all do their work correctly. In most cases it happens automatically; there's just one exception, dealt with explicitly
    //    in RewriteSymbolExpression to do with ReDim
    //
    // 3. So why don't we have an ASSERT to verify that those three methods are doing what they're supposed to?
    //    Answer: I'd love to. But there's one case upstream of us, in lambda closure rewriting, where
    //    it produces something with an incorrect pExpr->ResultType (it lacks generic type binding).
    //    This isn't a problem in practice because codegen in that case isn't affected by the lack of a correct type.
    //    But because our three functions produce the correct natural type according to the VB language,
    //    they end up disagreeing with the incorrect pExpr->ResultType, and so the assert ended up firing falsely.
    //
    // It'd be neat if we fixed up lambda closure rewriting. But I'm not going to mess with that here.
    // The repro (which causes the above assert to fire) is this one:
	//     Public Sub f(Of U)(x As U)
    //        Dim lambda = Async Sub() Console.WriteLine(x)
    //     End Sub
    //
    // I've left the comment here because it deserves explanation.
#endif


    pExpr->RewrittenByStateMachine = Result;

    return Result;
}



ILTree::Expression* ResumableMethodLowerer::RewriteCast(_In_ ILTree::BinaryExpression *expr)
{
    ILTree::Expression *Result = BoundTreeRewriter::RewriteExpression(expr);
    Result->ResultType = RewriteType(expr->ResultType);;
    return Result;
}


ILTree::Expression* ResumableMethodLowerer::RewriteSymbolExpression(_In_ ILTree::SymbolReferenceExpression *symref)
{
    Variable *originalVariable = symref->Symbol->IsVariable() ? symref->Symbol->PVariable() : NULL;

    if (originalVariable != NULL && originalVariable->GetVarkind() == VAR_Param && 
        originalVariable->GetType() != NULL &&  TypeHelpers::IsPointerType(originalVariable->GetType()))
    {

        // For ByRef parameter, ScanMethodForObsoleteUsage has already reported an error, then RewriteVariable can
        // be skipped. But if the type of symref invovles type parameter then it must be substituted, see bug 282233.
        symref->ResultType = RewriteType(symref->ResultType);
        return symref;
    }

    if (originalVariable!=NULL && originalVariable->IsMe())
    {
        if (originalVariable == m_StateMachineSymbol->GetMe())
        {
            VSFAIL("This expression has already been rewritten. Why are you rewriting it again?");
            return symref;
        }
        // Note: it's fine to refer to MyBase.field in a state-machine MoveNext method.
        // It will be rewritten to Me.$Me.field. What's not allowed is a virtual call
        // to a protected base method. Thankfully, those were already removed by FixupMyBaseMyClassCalls
    }

    Variable *newVariable = RewriteVariable(originalVariable);
    if (newVariable == originalVariable) return BoundTreeRewriter::RewriteExpression(symref);

    if (symref->BaseReference != NULL) symref->BaseReference = RewriteExpression(symref->BaseReference);

    ParserHelper ph(&m_Semantics->m_TreeStorage, symref->Loc);
    ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
    ILTree::Expression *result = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);

    // After this, the type of "result" will be the same as the type of newVariable.
    // That's mostly correct. But there's one ugly exception we have to deal with:
    // If you have "Option Explicit Off" and then "Redim x(i)", then the variable x
    // continues to have type "Object" (like all implicit variables), but the type of
    // the variable-reference-expression ends up with type "Object[]".
    // This problem was tracked by Dev11#171405 (for async) and Dev10#683075 (for lambdas).

    // Anyway, we might have to patch up the type of "result" by calling RewriteType.
    // Optimization: however, let's only do so if it was actually needed, i.e. if the
    // symbol-reference-expression we were given in the first place had a type different from its underlying symbol.
    if (symref->ResultType != originalVariable->GetType())
    {
        result->ResultType = RewriteType(symref->ResultType);
    }
    else
    {
        VSASSERT(TypeHelpers::EquivalentTypes(result->ResultType, newVariable->GetType()), "Unexpected async symbol rewrite: the rewritten symbol expression's type didn't agree with the rewritten symbol");
    }

    return result;
}


Variable* ResumableMethodLowerer::RewriteVariable(_In_ Variable* originalVariable)
{
    // INVARIANT: If given a non-null originalVariable, we will also return a non-null.
    // Moreover, if the class+method we're lifting isn't bad, then the thing we return
    // will be a correctly lifted (if necessary) version of the original variable

    m_TemporaryAccounting.Account(originalVariable);

    // Here we'll only lift local variables.
    // e.g. in "fred.Jones", the Jones symbol reference isn't a local and isn't Me, so it doesn't get rewritten.
    if (originalVariable==NULL || (!originalVariable->IsLocal() && !originalVariable->IsMe())) return originalVariable;

    // Catch and Finally locals don't get lifted. This includes the exception variable itself if it is being declared as
    // part of the catch block. If the exception variable is an existing variable then that variable is lifted if
    // it belongs to an outer scope that requires lifting. Also if a lambda inside the catch block referred to the
    // catch variable, the closure rewriter had already lifted it and this case is handled differently (see the
    // RewriteCatchBlock method).
    if (originalVariable->GetImmediateParent() != NULL && originalVariable->GetImmediateParent()->IsHash())
    {
        BCSYM_Hash *parentHash = originalVariable->GetImmediateParent()->PHash();
        if (m_TryHandlers.Contains(parentHash))
        {
            return originalVariable;
        }
    }

    Variable* newVariable = NULL;

    if (originalVariable->IsMe())
    {
        VSASSERT(originalVariable == m_MainContainerSymbol->GetMe(), "Error: how can there be a 'Me' reference that's not to the resumable method's class?");
        if (m_MeField == NULL)
        {
            return originalVariable; // e.g. if the class was bad, then we couldn't generate a "MeField", so let's just return the best we can
        }
        newVariable = m_MeField;
        VSASSERT(newVariable != NULL, "Error: lifted a non-null Me variable into a null");
        return newVariable;
    }

    bool alreadyMapped = m_LocalFieldMap.GetValue(originalVariable, &newVariable);
    if (alreadyMapped)
    {
        VSASSERT(newVariable != NULL, "Error: lifted a non-null Me variable into an already-mapped null");
        return newVariable;
    
    }
    
    STRING *encodedName = ClosureNameMangler::EncodeResumableVariableName(m_Compiler,originalVariable->GetName());
    STRING *newName = NULL;
    for (int suffix=1; ; suffix++)
    {
        StringBuffer buf; buf.AppendSTRING(encodedName); buf.AppendPrintf(L"$%i",suffix);
        newName = m_Compiler->AddString(&buf);
        if (m_StateMachineSymbol->SimpleBind(NULL, newName)==NULL)
        {
            originalVariable->SetRewrittenName(newName);  
            break;       
        }
    }
    Type *newType = RewriteType(originalVariable->GetType());
    newVariable = AddFieldToStateMachineClass(newName, newType);
    m_LocalFieldMap.SetValue(originalVariable, newVariable);

    VSASSERT(newVariable != NULL, "Error: lifted a non-null Me variable into a newly-created null");

    // Note: if this variable has a restricted type (ArgIterator &c.) then we'd end up producing
    // unverifiable code, since restricted types aren't allowed as fields. But we're safe...
    // The only things we lift are local variables and certain long-lived temporaries.
    // For local variables of restricted type, they've alreayd reported errors at declaration time.
    // The only long-lived temporaries we lift are For-loop limit, step, enumerator, exception, Selector.
    // All cases will have already reported an error if they had a restricted type in that position.
    VSASSERT((!IsRestrictedType(newType, m_CompilerHost) && !IsRestrictedArrayType(newType, m_CompilerHost)) ||
                m_Semantics->m_Errors->HasErrors(),
                "About to put illegal lifted variable in the state machine. This will be un-PEVerifyable. Why hasn't an error already been reported?");

    return newVariable;
}



ILTree::Expression* ResumableMethodLowerer::RewriteAwaitExpression(_In_ ILTree::AwaitExpression *await)
{
#if DEBUG
    BoundTreeBadnessVisitor badnessVisitor;
    ILTree::Expression *badExprTest = await;
    badnessVisitor.VisitExpression(&badExprTest);
    bool bWasBad = badnessVisitor.TreeIsBad();
#endif

    Type *resultType = RewriteType(await->ResultType); // nb. resultType might be VOID in the await-statement case
    Location loc = await->Loc;
    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    ParseTree::Expression *pLHS, *pRHS;
    ILTree::Statement *stmt;

    m_StatementContainsAwait = true;

    m_TemporaryAccounting.AccountAll(await->GetAwaiterDummy);
    m_TemporaryAccounting.AccountAll(await->IsCompletedDummy);
    m_TemporaryAccounting.AccountAll(await->GetResultDummy);
    
    if (await->GetAwaiterDummy==NULL || await->IsCompletedDummy==NULL || await->GetResultDummy==NULL)
    {
        // an error has already been reported for these cases
        return m_Semantics->AllocateBadExpression(loc);
    }

    await->Left = RewriteExpression(await->Left);
    VSASSERT(await->Right == NULL, "Unexpected: await has two operands"); // but we'll rewrite it anyway


    int state = ++m_HighestStateUsed;

    ILTree::Expression *pAwaiterAssignmentExpression = NULL;
    Type* pAwaiterType = RewriteType(await->GetAwaiterDummy->ResultType);
    Type* pAwaiterLocalTmpType = pAwaiterType;
    BCSYM_Variable* pAwaiterLocalTmpSym = GetTemporary(pAwaiterLocalTmpType);
    ParseTree::Expression* pAwaiterLocalTmp = ph.CreateBoundSymbol(pAwaiterLocalTmpSym);
    
    // If something's known by CLR to be a reference type, then we just store it in the "object" awaiter field.
    // If not, then we create a field specially for it...
    bool useObjectAwaiterField = TypeHelpers::IsReferenceType(pAwaiterType) && !TypeHelpers::IsGenericParameter(pAwaiterType);
    Type *pAwaiterFieldType = useObjectAwaiterField ? m_FXSymbolProvider->GetObjectType() : pAwaiterType;
    // Note: consider generics. Even if they have a class constraint, the CLR still doesn't know that they're reference
    // types and would need to box/unbox when storing/retreiving from object field. We do these as fields to avoid that.
    // (it's a vanishingly rare case anyway... would only arise with "await MyAwaitable<TAwaiterType>".)
    BCSYM_Variable* awaiterField = GetAwaiterField(pAwaiterFieldType);
    // Note: it's impossible for the result of GetAwaiter() to be one of the restricted types.
    // That's because to return a restricted type from a function is bad (unverifiable) metadata


    ILTree::AsyncSpillExpression* pAsyncSpillExpr = &m_Semantics->AllocateExpression(SX_ASYNCSPILL,
                                                                                     TypeHelpers::GetVoidType(),
                                                                                     loc)->AsAsyncSpillExpression();

    ////////////////////////////////////////// 
    // Create an expression representing a reference to the stack storage field.
    ParseTree::Expression* pStackStorage = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StackStorageField);
    pAsyncSpillExpr->StackStorageReference = &m_Semantics->InterpretExpression(pStackStorage, ExprNoFlags)->AsSymbolReferenceExpression();


    ////////////////////////////////////////// 
    // Dim AwaiterLocalTmp = e.GetAwaiter()
    ////////////////////////////////////////// 
    if (await->GetAwaiterDummy != NULL)
    {
        pRHS = RewriteDummy(await->GetAwaiterDummy, ph.CreateBoundExpression(await->Left), NULL /* arg */, loc);

        //
        // Generate an assignment statement and then pull out the operand. This
        // is a bit cheesy, but it's the simplest way to generate an assignment
        // expression.
        {
            // If the call to GetAwaiter is late-bound, we will have already
            // reported any warnings during the initial binding of the dummy
            // method. In that case, temporarily disable errors so that we
            // don't report duplicates.
            BackupValue<bool> backup_ReportErrors(&m_Semantics->m_ReportErrors);
            m_Semantics->m_ReportErrors = await->GetAwaiterDummy->bilop != SX_LATE;

            stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pAwaiterLocalTmp, pRHS), false /* link */);

            backup_ReportErrors.Restore();
        }

        VSASSERT(stmt!=NULL, "Error substituting GetAwaiter");
        pAwaiterAssignmentExpression = stmt->AsStatementWithExpression().Operand1;
        VSASSERT(bWasBad || !IsBad(pAwaiterAssignmentExpression), "Bad GetAwaiter substitution");
    }





    /////////////////////////////////////////
    // AwaiterLocalTmp.IsComplete
    /////////////////////////////////////////
    if (pAwaiterLocalTmp != NULL)
    {
        // [1] Condition...
        ParseTree::Expression *condition; 
        if (await->IsCompletedDummy != NULL)
        {
            condition = RewriteDummy(await->IsCompletedDummy, pAwaiterLocalTmp, NULL, loc);

            if (await->IsCompletedDummy->bilop == SX_LATE)
            {
                // A late-bound IsCompleted will return an Object, which needs to be
                // converted to a Boolean to be used as a condition.
                condition = ph.CreateConversion(condition, ph.CreateBoundType(m_FXSymbolProvider->GetBooleanType()));
            }
        }
        else
        {
            // We might get here if Semantics had already reported an error to bind
            condition = ph.CreateBoolConst(true);
        }

        {
            // If the get access to IsCompleted is late-bound, we will have already
            // reported any warnings during the initial binding of the dummy
            // method. In that case, temporarily disable errors so that we
            // don't report duplicates.
            BackupValue<bool> backup_ReportErrors(&m_Semantics->m_ReportErrors);
            m_Semantics->m_ReportErrors = await->IsCompletedDummy == NULL ||
                                          await->IsCompletedDummy->bilop != SX_LATE;

            pAsyncSpillExpr->IsCompletedCondition = m_Semantics->InterpretExpression(condition, ExprNoFlags);

            backup_ReportErrors.Restore();
        }
        VSASSERT(pAsyncSpillExpr->IsCompletedCondition!=NULL, "Error in IsCompletedCondition");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->IsCompletedCondition), "Bad IsCompletedCondition");
    }



    //////////////////////////
    // Me.stateField = <state>
    //////////////////////////
    if (m_StateField != NULL)
    {
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(state);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false /* link */);
        VSASSERT(stmt!=NULL, "Error in Me.stateField = <state>");
        pAsyncSpillExpr->StateAssignment = stmt->AsStatementWithExpression().Operand1;
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->StateAssignment), "Bad StateAssignment");
    }



    ////////////////////////////////////////////////////////////////////////
    // Me.AwaiterField = AwaiterLocalTmp
    ////////////////////////////////////////////////////////////////////////
    if (awaiterField != NULL && pAwaiterLocalTmp != NULL && pAwaiterLocalTmpType != NULL)
    {
        pAsyncSpillExpr->CreateAwaiterArray = NULL;

        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), awaiterField); 
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pAwaiterLocalTmp), false );
        pAsyncSpillExpr->AwaitXAssignment= stmt->AsStatementWithExpression().Operand1;

        VSASSERT(pAsyncSpillExpr->AwaitXAssignment!=NULL, "Error in AwaitXAssignment");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->AwaitXAssignment), "Bad AwaitXAssignment");
    }
       


    ///////////////////////////////////
    // OnCompleted...
    ///////////////////////////////////
    bool isLateBound = pAwaiterLocalTmpType->IsObject();
    //
    if (!isLateBound)
    {
        ///////////////////////////////////
        // Me.$Builder.Await[Unsafe]OnCompleted<TAwaiter,TSM>(ref AwaiterLocalTemp, ref Me)
        ///////////////////////////////////
        if (m_BuilderField != NULL && pAwaiterLocalTmp != NULL)
        {
            Type *ICriticalNotifyCompletionType = m_FXSymbolProvider->GetType(FX::ICriticalNotifyCompletionType);
            bool isCritical = TypeHelpers::IsOrInheritsFromOrImplements(pAwaiterLocalTmpType, ICriticalNotifyCompletionType, m_CompilerHost);

            BCSYM *method = Bind(m_BuilderField->GetType(),
                                 NULL,
                                 m_Compiler->AddString(isCritical ? L"AwaitUnsafeOnCompleted" : L"AwaitOnCompleted"),
                                 pAwaiterLocalTmpType,
                                 m_StateMachineType);
            ParseTree::Expression *pArg0 = pAwaiterLocalTmp;
            ILTree::Expression *expMe = m_Semantics->InterpretExpression(ph.CreateMeReference(), ExprNoFlags);
            SetFlag32(expMe, SXF_LVALUE); // In VB, "Me" is always an RValue. But we need it as an LValue for this call...
            ParseTree::Expression *pArg1 = ph.CreateBoundExpression(expMe);
            ParseTree::ArgumentList *pArgList = ph.CreateArgList(loc, 2, pArg0, pArg1);
            pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_BuilderField), method), pArgList, loc);
            pAsyncSpillExpr->OnCompletedCall = m_Semantics->InterpretExpression(pRHS, ExprResultNotNeeded);

            VSASSERT(pAsyncSpillExpr->OnCompletedCall!=NULL, "Error in OnCompletedCall");
            VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedCall), "Bad OnCompletedCall");
        }
    }
    else
    {
        ///////////////////////////////////
        // Dim dcast1 As ICriticalNotifyCompletion = TryCast(AwaiterLocalTemp, ICriticalNotifyCompletion)
        // If dcast1 IsNot Nothing Then
        //   builder.AwaitUnsafeOnCompleted<ICriticalNotifyCompletion,TSM>(ref dcast1, ref Me)
        // Else
        //   Dim dcast2 As INotifyCompletion = DirectCast(AwaiterLocalTemp, INotifyCompletion)
        //   builder.AwaitOnCompleted<INotifyCompletion,TSM>(ref dcast2, ref Me)
        // End If
        ///////////////////////////////////

        Type *ICriticalNotifyCompletionType = m_FXSymbolProvider->GetType(FX::ICriticalNotifyCompletionType);
        Type *INotifyCompletionType = m_FXSymbolProvider->GetType(FX::INotifyCompletionType);
        BCSYM_Variable* dcast1Sym = GetTemporary(ICriticalNotifyCompletionType);
        BCSYM_Variable* dcast2Sym = GetTemporary(INotifyCompletionType);
        //
        pLHS = ph.CreateBoundSymbol(dcast1Sym);
        pRHS = ph.CreateConversion(pAwaiterLocalTmp, ph.CreateBoundType(ICriticalNotifyCompletionType), ParseTree::Expression::TryCast);
        ILTree::Statement* stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false );
        pAsyncSpillExpr->OnCompletedCall = stmt->AsStatementWithExpression().Operand1; // cheap trick to get an AssignmentExpression
        VSASSERT(pAsyncSpillExpr->OnCompletedCall!=NULL, "Error in late OnCompletedCall");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedCall), "Bad late OnCompletedCall");
        //
        pRHS = ph.CreateBinaryExpression(ParseTree::Expression::IsNot, ph.CreateBoundSymbol(dcast1Sym), ph.CreateNothingConst());
        pAsyncSpillExpr->OnCompletedExtraCondition = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        VSASSERT(pAsyncSpillExpr->OnCompletedExtraCondition!=NULL, "Error in late OnCompletedExtraCondition");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedExtraCondition), "Bad late OnCompletedExtraCondition");
        //
        BCSYM *method = Bind(m_BuilderField->GetType(), NULL, m_Compiler->AddString(L"AwaitUnsafeOnCompleted"), ICriticalNotifyCompletionType, m_StateMachineType);
        ILTree::Expression *expMe = m_Semantics->InterpretExpression(ph.CreateMeReference(), ExprNoFlags);
        SetFlag32(expMe, SXF_LVALUE); // In VB, "Me" is always an RValue. But we need it as an LValue for this call...
        ParseTree::ArgumentList *pArgList = ph.CreateArgList(loc, 2, ph.CreateBoundSymbol(dcast1Sym), ph.CreateBoundExpression(expMe));
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_BuilderField), method), pArgList, loc);
        pAsyncSpillExpr->OnCompletedExtraIfTrue = m_Semantics->InterpretExpression(pRHS, ExprResultNotNeeded);
        VSASSERT(pAsyncSpillExpr->OnCompletedExtraIfTrue!=NULL, "Error in late OnCompletedExtraIfTrue");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedExtraIfTrue), "Bad late OnCompletedExtraIfTrue");
        //
        pLHS = ph.CreateBoundSymbol(dcast2Sym);
        pRHS = ph.CreateConversion(pAwaiterLocalTmp, ph.CreateBoundType(INotifyCompletionType), ParseTree::Expression::DirectCast);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false );
        pAsyncSpillExpr->OnCompletedExtraIfFalse1 = stmt->AsStatementWithExpression().Operand1; // cheap trick to get an AssignmentExpression
        VSASSERT(pAsyncSpillExpr->OnCompletedExtraIfFalse1!=NULL, "Error in late OnCompletedExtraIfFalse1");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedExtraIfFalse1), "Bad late OnCompletedExtraIfFalse1");
        //
        method = Bind(m_BuilderField->GetType(), NULL, m_Compiler->AddString(L"AwaitOnCompleted"), INotifyCompletionType, m_StateMachineType);
        expMe = m_Semantics->InterpretExpression(ph.CreateMeReference(), ExprNoFlags);
        SetFlag32(expMe, SXF_LVALUE); // In VB, "Me" is always an RValue. But we need it as an LValue for this call...
        pArgList = ph.CreateArgList(loc, 2, ph.CreateBoundSymbol(dcast2Sym), ph.CreateBoundExpression(expMe));
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_BuilderField), method), pArgList, loc);
        pAsyncSpillExpr->OnCompletedExtraIfFalse2 = m_Semantics->InterpretExpression(pRHS, ExprResultNotNeeded);
        VSASSERT(pAsyncSpillExpr->OnCompletedExtraIfFalse2!=NULL, "Error in late OnCompletedExtraIfFalse2");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->OnCompletedExtraIfFalse2), "Bad late OnCompletedExtraIfFalse2");
    }
    

    ///////////////////////////
    // $DoFinallyBodies = False
    ///////////////////////////
    if (m_DoFinallyBodiesLocal != NULL)
    {
        pLHS = ph.CreateBoundSymbol(m_DoFinallyBodiesLocal);
        pRHS = ph.CreateBoolConst(false);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false /* link */);
        VSASSERT(stmt!=NULL, "Error in DoFinallyBodies = false");
        pAsyncSpillExpr->DoFinallyBodiesAssignment = stmt->AsStatementWithExpression().Operand1;

        // We need to know if we're exiting a try block so that we can generate
        // the proper kind of return.
        pAsyncSpillExpr->uFlags |= SLF_EXITS_TRY;

        VSASSERT(pAsyncSpillExpr->DoFinallyBodiesAssignment!=NULL, "Error in DoFinallyBodiesAssignment");
    }

    ////////////
    // LABEL#X:  
    ////////////
    {
        stmt = MakeLabel(L"resume", state, m_CurrentScopeTargets, NULL);
        LabelDeclaration* pResumeLabelDeclaration = stmt->AsLabelStatement().Label;
        pAsyncSpillExpr->ResumeLabel = pResumeLabelDeclaration;
        
        VSASSERT(pAsyncSpillExpr->ResumeLabel!=NULL, "Error in ResumeLabel");
    }


        
    ////////////////
    // Me.$State = INITIAL_STATE
    ////////////////
    if (m_StateField != NULL)
    {
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(INITIAL_STATE);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        pAsyncSpillExpr->StateResetAssignment = stmt->AsStatementWithExpression().Operand1;

        VSASSERT(pAsyncSpillExpr->StateResetAssignment!=NULL, "Error in StateResetAssignment");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->StateResetAssignment), "Bad StateResetAssignment");
    }

    ///////////////////////////////////////////////////////////////////
    // AwaiterLocalTmp = Me.$awaitX
    //    or
    // AwaiterLocalTmp = DirectCast(Me.$awaitX, AwaiterType)
    ///////////////////////////////////////////////////////////////////
    if (awaiterField != NULL && pAwaiterLocalTmp != NULL && pAwaiterLocalTmpType != NULL)
    {
        pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(),awaiterField);
        if (pAwaiterFieldType != pAwaiterLocalTmpType)
        {
            // Notionally we should use BCSYM::AreTypesEqual rather than pointer equality.
            // But awaiterFieldType was created as either pointer-equal to pAwaiterLocalTmpType,
            // or as Object, so that's not needed.
            pRHS = ph.CreateConversion(pRHS, ph.CreateBoundType(pAwaiterLocalTmpType));
        }
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pAwaiterLocalTmp, pRHS), false);
        pAsyncSpillExpr->RestoreAwaiterStatements = stmt->AsStatementWithExpression().Operand1;  

        VSASSERT(pAsyncSpillExpr->RestoreAwaiterStatements!=NULL, "Error in RestoreAwaiterStatements");
        VSASSERT(bWasBad || !IsBad(pAsyncSpillExpr->RestoreAwaiterStatements), "Bad RestoreAwaiterStatements");
    }



    //////////////////////////////
    // AwaiterLocalTmp.GetResult()
    //////////////////////////////
    ILTree::Expression* pGetResultExpression = NULL;
    if (await->GetResultDummy != NULL && awaiterField != NULL)
    {
        ParseTree::Expression* substLHS = pAwaiterLocalTmp; 

        pRHS = RewriteDummy(await->GetResultDummy, substLHS, NULL /* arg */, loc);

        {
            // If the call to GetResult is late-bound, we will have already
            // reported any warnings during the initial binding of the dummy
            // method. In that case, temporarily disable errors so that we
            // don't report duplicates.
            BackupValue<bool> backup_ReportErrors(&m_Semantics->m_ReportErrors);
            m_Semantics->m_ReportErrors = await->GetResultDummy->bilop != SX_LATE;

            pGetResultExpression = m_Semantics->InterpretExpression(pRHS, await->GetResultDummy->ResultType->IsVoidType() ? ExprResultNotNeeded : ExprNoFlags);
 
            if (await->GetResultDummy->bilop == SX_LATE)
            {
                m_Semantics->SetLateCallInvocationProperties(pGetResultExpression->AsBinaryExpression(), resultType->IsVoidType() ? ExprResultNotNeeded : 0);
            }

            backup_ReportErrors.Restore();
        }

        VSASSERT(pGetResultExpression!=NULL, "Error in GetResult()");
        VSASSERT(bWasBad || !IsBad(pGetResultExpression), "Bad GetResult()");
    }
    else
    {
        // We might get here if semantics had already reported an error to bind
        pGetResultExpression =  m_Semantics->AllocateBadExpression(loc);
    }

    /////////////////////////////
    // AwaiterLocalTmp = Nothing
    /////////////////////////////
    pRHS = ph.CreateNothingConst();
    pLHS = pAwaiterLocalTmp;
    stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false /* link */);
    ILTree::Expression *pNullOutAwaiterExpression = stmt->AsStatementWithExpression().Operand1;

    ILTree::Expression* Result =
        m_Semantics->AllocateExpression(
                SX_SEQ,
                resultType,
                pAwaiterAssignmentExpression,
                m_Semantics->AllocateExpression(
                    SX_SEQ,
                    resultType,
                    pAsyncSpillExpr,
                    m_Semantics->AllocateExpression(
                        SX_SEQ,
                        resultType,
                        pGetResultExpression,
                        pNullOutAwaiterExpression,
                        loc),
                    loc),
                loc);

#if DEBUG
    badnessVisitor.VisitExpression(&Result);
    bool bIsBad = badnessVisitor.TreeIsBad();
    VSASSERT(bWasBad || !bIsBad, "In rewriting Await we have turned it bad");
#endif

    // Since BoundTreeVisitor skips SX_SEQ. 
    // So if FOR loop, if limite is await, then it 
    // will be skipped for marking as ConatinsAsyncSpill.
    Result->ContainsAsyncSpill = true;

    return Result;
}


ILTree::Statement* ResumableMethodLowerer::RewriteStatement(_In_ ILTree::Statement *pStatement)
{
    if (pStatement == NULL)
    {
        VSFAIL("Shouldn't attempt to rewrite a NULL statement");
        return NULL;
    }
    if (IsBad(pStatement))
    {
        // an error has already been reported
        return pStatement;
    }

    ILTree::Statement *stmt;
    switch (pStatement->bilop)
    {
        case SL_YIELD: stmt = RewriteYieldStatement(&pStatement->AsYieldStatement()); break;
        case SL_RETURN: stmt = RewriteReturnStatement(&pStatement->AsReturnStatement()); break;
        case SB_TRY: stmt = RewriteTryBlock(&pStatement->AsTryBlock()); break;
        case SB_CATCH: stmt = RewriteCatchBlock(&pStatement->AsCatchBlock()); break;
        // The following are just workarounds for stack-spilling, lifting &c.
        // rather than fundamental parts of the async rewrite:
        case SB_FINALLY: stmt = RewriteFinallyBlock(&pStatement->AsFinallyBlock()); break;
        case SB_FOR: stmt = RewriteForBlock(&pStatement->AsForBlock()); break;
        case SB_SELECT: stmt = RewriteSelectBlock(&pStatement->AsSelectBlock()); break;
        default: stmt = BoundTreeRewriter::RewriteStatement(pStatement);
    }
    
    return stmt;
}


ILTree::Statement* ResumableMethodLowerer::RewriteYieldStatement(_In_ ILTree::YieldStatement *pStatement)
{
    ThrowIfNull(pStatement->YieldExpression);
    pStatement->YieldExpression = RewriteExpression(pStatement->YieldExpression);

    Location loc = pStatement->Loc;
    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    ILTree::Statement *Result = NULL;
    StatementListBuilder bldr(&Result);
    ParseTree::Expression *pLHS, *pRHS;
    ILTree::Statement *stmt;

    // We'll pick out a new state number to be associated with this yield statement
    m_HighestStateUsed ++;
    int state = m_HighestStateUsed;

    // Me.Current = value
    if (m_CurrentField != NULL)
    {
        ParseTree::Expression* pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_CurrentField);
        ParseTree::Expression* pRHS = ph.CreateBoundExpression(pStatement->YieldExpression);
        ILTree::Statement* stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // Me.stateField = <state>
    if (m_StateField != NULL)
    {
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(state);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // DoFinallyBodies = False
    if (m_DoFinallyBodiesLocal != NULL)
    {
        pLHS = ph.CreateBoundSymbol(m_DoFinallyBodiesLocal);
        pRHS = ph.CreateBoolConst(false);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // Return True
    pRHS = ph.CreateBoolConst(true);
    stmt = m_Semantics->AllocateStatement(SL_RETURN, loc, NoResume, false);
    stmt->uFlags = pStatement->uFlags & (SLF_EXITS_TRY | SLF_EXITS_CATCH);
    stmt->uFlags |= SLF_EXITS_TRY;
    stmt->AsReturnStatement().ReturnExpression = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
    RobustAdd(&bldr, stmt);
    
    // label:
    stmt = MakeLabel(L"resume", state, m_CurrentScopeTargets, NULL);
    RobustAdd(&bldr, stmt);

    // If Me.Disposing Then Return False
    if (m_DisposingField != NULL)
    {
        ILTree::Expression *condition = m_Semantics->InterpretExpression(ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_DisposingField), ExprNoFlags);
        ILTree::ReturnStatement *thenBranch = &m_Semantics->AllocateStatement(SL_RETURN, loc, NoResume, false)->AsReturnStatement();
        thenBranch->uFlags = pStatement->uFlags & (SLF_EXITS_TRY | SLF_EXITS_CATCH);
        thenBranch->uFlags |= SLF_EXITS_TRY;
        thenBranch->ReturnExpression = m_Semantics->InterpretExpression(ph.CreateBoolConst(false), ExprNoFlags);
        stmt = MakeIf(loc, condition, thenBranch, NULL);
        RobustAdd(&bldr,stmt);
    }

    // Me.stateField = INITIAL_STATE
    if (m_StateField != NULL)
    {
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), m_StateField);
        pRHS = ph.CreateIntConst(INITIAL_STATE);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr,stmt);
    }


    return Result;
}



ILTree::Statement* ResumableMethodLowerer::RewriteReturnStatement(_In_ ILTree::ReturnStatement *pStatement)
{
    ILTree::Expression *expr = pStatement->ReturnExpression == NULL ? NULL : &pStatement->ReturnExpression->AsExpression();
    m_StatementContainsAwait = false;
    expr = expr==NULL ? NULL : RewriteExpression(expr);
    pStatement->ReturnExpression = expr;

    Location loc = pStatement->Loc;
    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    ILTree::Statement *Result = NULL;
    StatementListBuilder bldr(&Result);
    ParseTree::Expression *pLHS, *pRHS;
    ILTree::Statement *stmt;

    if (pStatement->ReturnExpression != NULL && m_ReturnTempLocal != NULL)
    {
        // $ReturnTempLocal = expression
        pLHS = ph.CreateBoundSymbol(m_ReturnTempLocal);
        pRHS = ph.CreateBoundExpression(&pStatement->ReturnExpression->AsExpression());
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false /* link */);
        RobustAdd(&bldr, stmt);
    }

    // Goto ReturnLabel
    ILTree::GotoStatement *gotoStmt = &m_Semantics->AllocateStatement(SL_GOTO, loc, NoResume, false)->AsGotoStatement();
    gotoStmt->Label = m_ReturnLabel;
    gotoStmt->uFlags = pStatement->uFlags & (SLF_EXITS_TRY | SLF_EXITS_CATCH);
    gotoStmt->uFlags |= SLF_EXITS_TRY;
    RobustAdd(&bldr, gotoStmt);

    return Result;
}

ILTree::Statement* ResumableMethodLowerer::RewriteTryBlock(_In_ ILTree::TryBlock *block)
{
    // ORIGINAL                           REWRITTEN
    // 
    //   |                                  |
    //  Try ---Child---> Stmt1             StagingPost:
    //   |                 |                |
    //   |               Stmt2             Try --Child---> SwitchTable
    //   |                 :                |                |
    //   |                                  |              Stmt1
    //  Catch --Child--> Stmt3              |                |
    //   |                 |                |              Stmt2
    //   |                 :                |                :
    //   |                                  |
    //  Finally -Child-> Stmt4             Catch --Child--> Stmt3
    //   |                 |                |                |
    //   |                 :                |                :
    //   |                                  |        
    //                                     Finally -Child-> IfBlk --Child--> If(cond) --Child--> Stmt4
    //                                      |                                                      |
    //                                      |                                                      :


    // Now, prepare the actual result of this function...
    ILTree::Statement *Result = NULL;
    StatementListBuilder bldrOuter(&Result);

    // StagingPost:
    m_HighestStagepostUsed ++;
    int stagepost = m_HighestStagepostUsed;
    LabelDeclaration *labdecl;
    ILTree::Statement* stmt = MakeLabel(L"stagepost", stagepost, NULL, &labdecl);
    RobustAdd(&bldrOuter, stmt);

    // try { ... }
    RobustAdd(&bldrOuter, block);

    // Save the source location of the TRY line and then set the try block's location
    // to hidden.  We do not want to map the opening of the try block to the try line
    // because it will then include the inner switch table as part of the try line.
    // This would mean setting a breakpoint on the try would then get hit upon
    // resuming the async method at an await inside the try block.  What we really
    // want is for the target label that points to the beginning of the user's code
    // for the try block to map to the try line, which is done using a NOP below.
    Location locTry = block->Loc;
    block->Loc.SetLocationToHidden();

    // Now, prepare the child of the Try block...
    CSingleList<LabelTarget> *outerTargets = m_CurrentScopeTargets;
    CSingleList<LabelTarget> innerTargets;
    m_CurrentScopeTargets = &innerTargets;
    ILTree::Statement *rewrittenTryBody = RewriteStatementList(block->Child);
    block->Child = NULL;
    StatementListBuilder bldrInner(&block->Child);
    // switch...
    bldrInner.AddList(MakeSwitchFromCurrentScope());
    // Mapping to the TRY line
    stmt = m_Semantics->AllocateStatement(SL_DEBUGLOCNOP, locTry, NoResume, false);
    bldrInner.Add(stmt);
    // body...
    bldrInner.AddList(rewrittenTryBody);
    m_CurrentScopeTargets = outerTargets;


    // Oh... for every state->target that was generated in the inner switch-list,
    // we have to add state->stagepost to the outer switch-list.
    CSingleListIter<LabelTarget> iter(&innerTargets);
    while (LabelTarget *innerLabtarg = iter.Next())
    {
        LabelTarget *outerLabtarg = m_Semantics->m_TreeStorage.Alloc<LabelTarget>();
        outerLabtarg->state = innerLabtarg->state;
        outerLabtarg->target = labdecl;
        m_CurrentScopeTargets->InsertLast(outerLabtarg);
    }

    return Result;
}

ILTree::Statement* ResumableMethodLowerer::RewriteCatchBlock(_In_ ILTree::CatchBlock *block)
{
    m_moveNextMethod->GetBoundTree()->fSeenCatch = true;
    m_TryHandlers.SetValue(block->Locals, block); // RewriteSymbolExpression uses this information to avoid lifting locals from this block

    // Grab the original catch variable before rewriting so we can check the
    // m_LocalFieldMap to see if it became lifted.  If it was lifted, then
    // we'll have to use a short lived temporary to hold the exception and
    // then store it into the lifted variable (below).
    Variable* originalCatchVariable = NULL;
    if (block->CatchVariableTree != NULL &&
		block->CatchVariableTree->bilop == SX_SYM &&
        block->CatchVariableTree->AsSymbolReferenceExpression().pnamed->IsVariable())
    {
        originalCatchVariable = block->CatchVariableTree->AsSymbolReferenceExpression().pnamed->PVariable();
    }

    ILTree::CatchBlock *result = &BoundTreeRewriter::RewriteStatement(block)->AsCatchBlock();

    if (result->ExceptionTemporary == NULL &&
        originalCatchVariable != NULL)
    {
        VSASSERT(result->ExceptionStore == NULL, "When is ExceptionStore not NULL and ExceptionTemporary is NULL?");

        Variable* mappedCatchVariable;
        bool fMapped = m_LocalFieldMap.GetValue(originalCatchVariable, &mappedCatchVariable);

        if (fMapped && mappedCatchVariable == result->CatchVariableTree->AsSymbolReferenceExpression().pnamed)
        {
            // The catch variable was actually an existing variable from outside the catch scope
            // and it has been lifted into a field on the state machine class.  We need to use a
            // temporary in order to store the exception into this field.  This is what
            // ExceptionTemporary is used for.
            //Type* type = result->CatchVariableTree->AsSymbolReferenceExpression().pnamed->PMember()->GetType();
            Type* type = mappedCatchVariable->GetType();
            result->ExceptionTemporary = m_Semantics->AllocateShortLivedTemporary(type);

            // Create the assignment of the temporary back to the lifted variable
            Location hiddenLoc = Location::GetHiddenLocation();
            ILTree::Expression* assign = m_Semantics->AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                &result->CatchVariableTree->AsSymbolReferenceExpression(),
                m_Semantics->AllocateSymbolReference(result->ExceptionTemporary, type, NULL, hiddenLoc),
                hiddenLoc);
            ILTree::Statement* stmt = m_Semantics->AllocateStatement(SL_STMT, hiddenLoc, NoResume, false);
            stmt->AsStatementWithExpression().Operand1 = assign;
            stmt->Parent = result;
            result->ExceptionStore = stmt;

            return result;
        }
    }

    if (result->ExceptionTemporary == NULL) return result;

    result->ExceptionStore = RewriteStatementList(result->ExceptionStore); // because BoundTreeVisitor fails to rewrite this statement-list

    // Trouble is that the BoundTreeVisitor doesn't visit the temporary fields.
    // Also, the CatchBlock structure assumes that they're VARIABLES, but we might have to rewrite them into SYMBOL REFERENCES.
    ParserHelper ph(&m_Semantics->m_TreeStorage, block->Loc);

    Variable *newVariable = RewriteVariable(block->ExceptionTemporary);
    if (newVariable != block->ExceptionTemporary)
    {
        ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
        result->LiftedExceptionTemporary = &m_Semantics->InterpretExpression(pRHS, ExprNoFlags)->AsSymbolReferenceExpression();
    }
    m_TemporaryAccounting.Account(block->ExceptionTemporary);
    // 






    return result;
}

ILTree::Statement* ResumableMethodLowerer::RewriteFinallyBlock(_In_ ILTree::FinallyBlock *block)
{
    Location loc = Location::GetHiddenLocation();

    m_TryHandlers.SetValue(block->Locals, block); // RewriteSymbolExpression uses this information to avoid lifting locals from this block
    ILTree::Statement *rewrittenFinallyBody = BoundTreeRewriter::RewriteStatementList(block->Child);
    block->Child = NULL;
    ParserHelper ph(&m_Semantics->m_TreeStorage, block->Loc);

    // if (DoFinallyBodies) Then <rewrittenFinallyBody>
    if (m_DoFinallyBodiesLocal != NULL)
    {
        // Save the source location of the FINALLY line and then set the finally block's
        // location to hidden.  We do not want to map the opening of the finally block to
        // the finally line because it will then include the "if (DoFinallyBodies)" check
        // as part of the finally line. This would mean setting a breakpoint on the
        // finally would then get hit upon stepping over an await inside the corresponding
        // try block, ruining the step over await, and being incorrect since the finally
        // body will not actually execute at that point.  What we really want is for the
        // consequence of the if to map to the finally line, which is done using a NOP below.
        Location locFinally = block->Loc;
        block->Loc.SetLocationToHidden();

        // Mapping to the FINALLY line
        ILTree::Statement *stmt = m_Semantics->AllocateStatement(SL_DEBUGLOCNOP, locFinally, NoResume, false);
        stmt->Next = rewrittenFinallyBody;
        
        ILTree::Expression *condition = m_Semantics->InterpretExpression(ph.CreateBoundSymbol(m_DoFinallyBodiesLocal), ExprNoFlags);
        stmt = MakeIf(loc, condition, stmt, NULL);
        block->Child = stmt;
    }

    return block;
}

ILTree::Statement* ResumableMethodLowerer::RewriteForBlock(_In_ ILTree::ForBlock *block)
{
    ILTree::ForBlock *result = &BoundTreeRewriter::RewriteStatement(block)->AsForBlock();
    if (result->LimitTemporary == NULL && result->StepTemporary == NULL && result->EnumeratorTemporary == NULL) return result;

    // Trouble is that the BoundTreeVisitor doesn't visit the temporary fields.
    // Also, the ForBlock structure assumes that they're VARIABLES, but we might have to rewrite them into SYMBOL REFERENCES.
    ParserHelper ph(&m_Semantics->m_TreeStorage, block->Loc);

    if (result->LimitTemporary != NULL)
    {
        Variable *newVariable = RewriteVariable(result->LimitTemporary);
        if (newVariable != result->LimitTemporary)
        {
            ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
            result->LiftedLimitTemporary = &m_Semantics->InterpretExpression(pRHS, ExprNoFlags)->AsSymbolReferenceExpression();
        }
        m_TemporaryAccounting.Account(result->LimitTemporary);
    }

    if (result->StepTemporary != NULL)
    {
        Variable *newVariable = RewriteVariable(result->StepTemporary);
        if (newVariable != result->StepTemporary)
        {
            ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
            result->LiftedStepTemporary = &m_Semantics->InterpretExpression(pRHS, ExprNoFlags)->AsSymbolReferenceExpression();
        }
        m_TemporaryAccounting.Account(result->StepTemporary);
    }

    if (result->EnumeratorTemporary != NULL)
    {
        Variable *newVariable = RewriteVariable(result->EnumeratorTemporary);
        if (newVariable != result->EnumeratorTemporary)
        {
            ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
            result->LiftedEnumeratorTemporary = &m_Semantics->InterpretExpression(pRHS, ExprNoFlags)->AsSymbolReferenceExpression();
        }
        m_TemporaryAccounting.Account(result->EnumeratorTemporary);
    }

    return result;
}


ILTree::Statement* ResumableMethodLowerer::RewriteSelectBlock(_In_ ILTree::SelectBlock *block)
{
    ILTree::SelectBlock *result = &BoundTreeRewriter::RewriteStatement(block)->AsSelectBlock();
    if (result->SelectorTemporary == NULL) return result;

    // As explained in the For block case, we just have to lift temporary fields
    ParserHelper ph(&m_Semantics->m_TreeStorage, block->Loc);

    Variable *newVariable = RewriteVariable(result->SelectorTemporary);
    if (newVariable != result->SelectorTemporary)
    {
        ParseTree::Expression *pRHS = ph.CreateBoundMemberSymbol(ph.CreateMeReference(), newVariable);
        result->LiftedSelectorTemporary = &m_Semantics->InterpretExpression(pRHS, ExprNoFlags)->AsSymbolReferenceExpression();
    }
    m_TemporaryAccounting.Account(result->SelectorTemporary);

    return result;
}


ParseTree::Expression* ResumableMethodLowerer::RewriteDummy(ILTree::Expression* pDummy, ParseTree::Expression* pMe, ParseTree::Expression* pArg, Location &loc)
{
    BILOP exprKind = pDummy->bilop;
    VSASSERT(exprKind == SX_CALL || exprKind == SX_LATE, "Why isn't the dummy method call actually a call or late-bound expression?");

    if (exprKind == SX_CALL)
    {
        ILTree::CallExpression* pDummyAsCall = &pDummy->AsCallExpression();
        pDummyAsCall->Left = RewriteExpression(pDummyAsCall->Left); // to rewrite the method symbol itself
        return SubstituteDummies(pDummyAsCall, pMe, pArg, loc);
    }
    else if (exprKind == SX_LATE)
    {
        ILTree::LateBoundExpression* pDummyAsLateBound = &pDummy->AsLateBoundExpression();
        pDummyAsLateBound->LateIdentifier = RewriteExpression(pDummyAsLateBound->LateIdentifier); // to rewrite the method symbol itself
        return SubstituteLateBoundDummies(pDummyAsLateBound, pMe, pArg, loc);
    }
    else
    {
        return NULL;
    }
}

ParseTree::Expression* ResumableMethodLowerer::SubstituteDummies(ILTree::CallExpression* callDummy, ParseTree::Expression* me, ParseTree::Expression* arg, Location &loc)
{   
    VSASSERT(callDummy != NULL && callDummy->Left != NULL && callDummy->Left->bilop == SX_SYM && me!=NULL, "Improper dummy passed to SubstituteDummies");

    ILTree::SymbolReferenceExpression *callee = &callDummy->Left->AsSymbolReferenceExpression();
    BCSYM *member = NULL;
    if (callee->GenericBindingContext != NULL && callee->GenericBindingContext->IsGenericTypeBinding())
    {
        member = m_TransientSymbolCreator->GetGenericBinding(false, callee->Symbol, NULL, 0, callee->GenericBindingContext->PGenericTypeBinding());
    }
    else if (callee->GenericBindingContext != NULL)
    {
        member = callee->GenericBindingContext; 
    }
    else
    {
        member = callee->Symbol;
    }

    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    if (callDummy->MeArgument != NULL)
    {
        // instance invocation
        ParseTree::Expression* target = ph.CreateBoundSymbolOnNewInstance(me, member);
        ParseTree::ArgumentList* args = (arg==NULL ? NULL : ph.CreateArgList(arg));
        return ph.CreateMethodCallOnAlreadyResolvedTarget(target, args, loc);
    }
    else
    {
        // extension-method invocation
        ParseTree::Expression* target = ph.CreateBoundSymbol(member);
        ParseTree::ArgumentList* args = (arg==NULL ? ph.CreateArgList(me) : ph.CreateArgList(2, me, arg));
        return ph.CreateMethodCallOnAlreadyResolvedTarget(target, args, loc);
    }
}

ParseTree::Expression* ResumableMethodLowerer::SubstituteLateBoundDummies(ILTree::LateBoundExpression* callDummy, ParseTree::Expression* me, ParseTree::Expression* arg, Location &loc)
{
    VSASSERT(callDummy != NULL &&
             callDummy->LateIdentifier != NULL &&
             callDummy->LateIdentifier->bilop == SX_CNS_STR &&
             me != NULL,
             "Improper dummy passed to SubstituteDummies");

    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);

    ILTree::StringConstantExpression* pMethodName = &callDummy->LateIdentifier->AsStringConstant();
    ParseTree::Expression *target = ph.CreateQualifiedExpression(me, ph.CreateNameExpression(m_Compiler->AddString(pMethodName->Spelling)));
    ParseTree::ArgumentList* args = (arg == NULL ? NULL : ph.CreateArgList(arg));
    return ph.CreateMethodCall(target, args, loc);
}


//-------------------------------------------------------------------------------------------------
//
// ResumableMethodLowerer::MakeMainBody
//

ILTree::ProcedureBlock* ResumableMethodLowerer::MakeMainBody()
{
    Location loc = Location::GetHiddenLocation();

    ILTree::ProcedureBlock *block = &m_Semantics->AllocateStatement(SB_PROC, loc, 0, false)->AsProcedureBlock();
    block->pproc = m_MainProc;
    m_MainProc->SetJITOptimizationsMustBeDisabled(false); // note: the MoveNextMethod has already copied this flag from us
    m_Semantics->m_Procedure = block->pproc;
    m_Semantics->m_ProcedureTree = block;

    block->m_ResumableKind = ILTree::NotResumable; // because (after the rewriting) it will become just a plain non-resumable method
    block->fSeenCatch = false;
    block->fSeenLineNumber = false;
    block->fSeenOnErr = false;
    block->fSeenResume = false;

    Scope *LocalsAndParameters = NULL;
    Variable *FirstParamVar = NULL;
    Declared::MakeLocalsFromParams(
        m_Compiler,
        &m_Semantics->m_TreeStorage,
        m_MainProc,
        m_Semantics->m_Errors,
        false,
        &FirstParamVar,
        &LocalsAndParameters);
    block->Locals = LocalsAndParameters;
    m_Semantics->m_Lookup = LocalsAndParameters;

    if (block->pproc->GetType()!=NULL && !block->pproc->GetType()->IsVoidType())
    {
        Variable *ReturnVariable = Declared::MakeReturnLocal(m_Compiler, &m_Semantics->m_TreeStorage, block->pproc, NULL);
        Symbols::AddSymbolToHash(block->Locals, ReturnVariable, true, false, false);
        block->ReturnVariable = ReturnVariable;
    }

    block->ptempmgr = new(m_Semantics->m_TreeStorage) TemporaryManager(m_Compiler, m_CompilerHost, block->pproc, &m_Semantics->m_TreeStorage);
    m_Semantics->m_TemporaryManager = block->ptempmgr;

    m_Semantics->m_ContainingClass = m_MainContainerSymbol;
    m_Semantics->m_ContainingContainer = m_MainContainerSymbol;

    ParserHelper ph(&m_Semantics->m_TreeStorage, loc);
    ParseTree::Expression *pLHS, *pRHS;
    StatementListBuilder bldr(&block->Child);
    ILTree::Statement *stmt;


    // Get the type of the state machine variable.
    // e.g. if the state machine had open type "class StateMachine<T>",
    // and we are now inside the body of "void FredAsync<U>() {var sm = new StateMachine<U>();}"
    // then the type of our "sm" variable is the open type, but providing our formal parameter "U" as the generic argument.
    BCSYM *stateMachineType;
    if (m_MainProc->GetFirstGenericParam() != NULL)
    {
        BCSYM **gArgArray = reinterpret_cast<BCSYM**>(m_TransientSymbolCreator->GetNorlsAllocator()->Alloc(sizeof(BCSYM*) * m_MainProc->GetGenericParamCount()));
        unsigned gArgCount = 0;
        for (BCSYM_GenericParam *gParam = m_MainProc->GetFirstGenericParam(); gParam!=NULL; gParam=gParam->GetNextParam())
        {
            gArgArray[gArgCount] = gParam;
            gArgCount++;
        }
        GenericTypeBinding *parentBinding = (IsGenericOrHasGenericParent(m_MainContainerSymbol)) ? SynthesizeOpenGenericBinding(m_MainContainerSymbol, *m_TransientSymbolCreator)->PGenericTypeBinding() : NULL;
        stateMachineType = m_TransientSymbolCreator->GetGenericBinding(false, m_StateMachineSymbol, gArgArray, gArgCount, parentBinding);
    }
    else if (IsGenericOrHasGenericParent(m_StateMachineSymbol))
    {
        stateMachineType = SynthesizeOpenGenericBinding(m_StateMachineSymbol, m_Semantics->m_TransientSymbolCreator);
    }
    else
    {
        stateMachineType = m_StateMachineSymbol; 
    }

    Symbol *StateMachineVariable = AddLocalToProc(STRING_CONST(m_Compiler, StateMachineVariableName), m_MainProc, stateMachineType, loc);

    // Dim sm = new StateMachine<...>
    if (StateMachineVariable != NULL)
    {
        pLHS = ph.CreateBoundSymbol(StateMachineVariable);
        pRHS = ph.CreateNewObject(ph.CreateBoundType(stateMachineType));
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    if (!m_MainProc->IsShared() && m_MeField != NULL)
    {
        // sm.Me = Me
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_MeField);
        pRHS = ph.CreateMeReference();
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    // sm.param... = param...
    for (unsigned int i=0; i<m_ParameterFields.Count(); i++)
    {
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), IsIterable ? m_ParameterProtoFields.Element(i) : m_ParameterFields.Element(i));
        STRING* paramName = ClosureNameMangler::DecodeVariableName(m_Compiler, m_ParameterFields.Element(i)->GetName());
        pRHS = ph.CreateBoundSymbol(block->Locals->SimpleBind(paramName)->PVariable());
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }

    if (m_StateField != NULL)
    {
        // sm.state = ENUMERABLE_NOT_YET_STARTED | INITIAL_STATE
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_StateField);
        pRHS = ph.CreateIntConst(IsIterable ? ENUMERABLE_NOT_YET_STARTED : INITIAL_STATE);
        ILTree::Statement *stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);
    }
    
    if ((IsFunctionAsync || IsSubAsync) && m_BuilderField != NULL)
    {
        // sm.builder = {AsyncMethodBuilderType}.Create()
        pLHS = ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_BuilderField);
        BCSYM *method = Bind(m_FXSymbolProvider->GetType(IsSubAsync ? FX::AsyncVoidMethodBuilderType : (m_ResumableGenericType ? FX::GenericAsyncTaskMethodBuilderType : FX::AsyncTaskMethodBuilderType)),
                             m_ResumableGenericType,
                             m_Compiler->AddString(L"Create"));
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(ph.CreateBoundSymbol(method), NULL, loc);
        stmt = m_Semantics->InterpretAssignment(ph.CreateAssignment(pLHS, pRHS), false);
        RobustAdd(&bldr, stmt);

        // sm.builder.Start<f_StateMachine<T>>(ref sm)
        BCSYM *builderType = stmt->AsStatementWithExpression().Operand1->AsBinaryExpression().Right->ResultType;
        // We're re-using the builderType that was computed from interpreting the above statement
        BCSYM *start = Bind(builderType, NULL, m_Compiler->AddString(L"Start"), stateMachineType);
        ParseTree::Expression *pCall = ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_BuilderField), start);
        pRHS = ph.CreateMethodCallOnAlreadyResolvedTarget(pCall, ph.CreateArgList(ph.CreateBoundSymbol(StateMachineVariable)), loc);
        ILTree::StatementWithExpression* CallStatement = &m_Semantics->AllocateStatement(SL_STMT, loc, 0, false)->AsStatementWithExpression();
        CallStatement->Operand1 = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, CallStatement);
    }

    if (IsFunctionAsync && m_BuilderField != NULL)
    {
        // return sm.builder.Task
        BCSYM *prop = Bind(m_FXSymbolProvider->GetType(m_ResumableGenericType ? FX::GenericAsyncTaskMethodBuilderType : FX::AsyncTaskMethodBuilderType),
                           m_ResumableGenericType,
                           m_Compiler->AddString(L"Task"));
        pRHS = ph.CreateBoundMemberSymbol(ph.CreateBoundMemberSymbol(ph.CreateBoundSymbol(StateMachineVariable), m_BuilderField), prop);
        ILTree::ReturnStatement *Return = &m_Semantics->AllocateStatement(SL_RETURN, loc, 0, false)->AsReturnStatement();
        Return->ReturnExpression = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, Return);
    }

    if ((IsIterator || IsIterable) && StateMachineVariable != NULL)
    {
        // return sm
        pRHS = ph.CreateBoundSymbol(StateMachineVariable);
        ILTree::ReturnStatement *Return = &m_Semantics->AllocateStatement(SL_RETURN, loc, 0, false)->AsReturnStatement();
        Return->ReturnExpression = m_Semantics->InterpretExpression(pRHS, ExprNoFlags);
        RobustAdd(&bldr, Return);
    }

    m_TemporaryAccounting.HoldToAccount(m_OriginalBlock->ptempmgr);

    return block;
}



class FindMyBaseMyClass : public BoundTreeVisitor
{
public:
    FindMyBaseMyClass(List<ILTree::Expression*, NorlsAllocWrapper> *list) : m_mbcCallList(list) {}
    List<ILTree::Expression*, NorlsAllocWrapper> *m_mbcCallList;

    virtual bool StartExpression(ILTree::Expression **ppExpr)
    {
        if (IsBad(*ppExpr))
        {
            return false;
        }

        if (ppExpr == NULL || *ppExpr == NULL || IsBad(*ppExpr)) return false;
        if ( (*ppExpr)->bilop == SX_CALL)
        {
            ILTree::CallExpression *call = &(*ppExpr)->AsCallExpression();
            if (call->MeArgument != NULL && IsMyBaseMyClassSymbol(call->MeArgument)) m_mbcCallList->AddLast(call);
        }
        else if ( (*ppExpr)->bilop == SX_DELEGATE_CTOR_CALL)
        {
            ILTree::DelegateConstructorCallExpression *delCall = &(*ppExpr)->AsDelegateConstructorCallExpression();
            if (delCall->ObjectArgument != NULL && delCall->ObjectArgument->IsExprNode() && IsMyBaseMyClassSymbol(&delCall->ObjectArgument->AsExpression())) m_mbcCallList->AddLast(delCall);
        }
        return true;
    }

};




ILTree::ProcedureBlock*
Semantics::LowerResumableMethod(_In_opt_ ILTree::ProcedureBlock *BoundBody)
{
    if (BoundBody==NULL || IsBad(BoundBody)) return BoundBody;

    switch (BoundBody->m_ResumableKind)
    {
        case ILTree::UnknownResumable:
            VSFAIL("There was a non-bad method with UnknownResumable. How? It should have been caught earlier in IndicateLocalResumable");
            return BoundBody;

        case ILTree::ErrorResumable:
        case ILTree::NotResumable:
            return BoundBody;

        case ILTree::TaskResumable:
        case ILTree::SubResumable:            
        case ILTree::IteratorResumable:
        case ILTree::IterableResumable:
        {
            // Generally we're allowed to come here with even if there were errors already reported (e.g. missing return type)

            // Why is the place where this is normally done not reaching the required
            // anonymous delegates nested inside an async lambda? See Dev11#68556
            RegisterRequiredAnonymousDelegates(BoundBody);

            // If the initial code contained any calls to MyBase.f(), or MyClass.f(),
            // then they have to be redirected through a stub. (Incidentally lambda closures
            // have to do the same thing). Anyway, we fix this first:
            ClosureRoot croot(BoundBody->pproc, BoundBody, this, m_TransientSymbolCreator);
            List<ILTree::Expression*, NorlsAllocWrapper> mybase_fixups(m_TransientSymbolCreator.GetNorlsAllocator());
            FindMyBaseMyClass finder(&mybase_fixups);
            finder.Visit(BoundBody);
            croot.FixupMyBaseMyClassCalls(&mybase_fixups);

            // Now we can create the state-machine class, including a rewritten "MoveNextBody"
            ResumableMethodLowerer lowerer(this, BoundBody);
            lowerer.CreateStateMachineClass();

            lowerer.GenerateCompilerGeneratedStateMachineAttribute(BoundBody->pproc);

            // And we'll return a replacement body for the resumable method
            return lowerer.MakeMainBody();
        }
        default:
            VSFAIL("Internal error: I don't know this kind of resumable!");
            return BoundBody;
    }
}


void
ResumableMethodLowerer::GenerateCompilerGeneratedStateMachineAttribute
(
    BCSYM_Proc *pResumableProc
)
{
    ThrowIfNull(pResumableProc);
    ThrowIfNull(m_Semantics);

    FX::TypeName StateMachineTypeName;
    STRING* StateMachineTypeNameString = NULL;
    
    if (pResumableProc->IsAsyncKeywordUsed())
    {
        StateMachineTypeName = FX::AsyncStateMachineAttributeType;
		StateMachineTypeNameString = STRING_CONST(m_Semantics->GetCompiler(), ComAsyncStateMachineAttribute);
    }
    else
    {
        VSASSERT(pResumableProc->IsIteratorKeywordUsed(), "It must be Iterator!");
        StateMachineTypeName = FX::IteratorStateMachineAttributeType;
		StateMachineTypeNameString = STRING_CONST(m_Semantics->GetCompiler(), ComIteratorStateMachineAttribute);
    }
    
    if (m_Semantics->GetFXSymbolProvider()->IsTypeAvailable(StateMachineTypeName) && m_StateMachineType)
    {
        BCSYM_NamedRoot* StateMachineAttributeType = m_Semantics->GetFXSymbolProvider()->GetType(StateMachineTypeName);

        // Passing voidType to see if this sybmol is globally aviliable. Don't pass in null, or it will assume it is in the current class
        // and not perform certain checks if the symbol is marked protected. Otherwise the voidType is used to get a generic bindings of which
        // are none, so that is good too.
        // Empty generic binding context is good because statemachineattribute doesn't have
        if (StateMachineAttributeType != NULL && 
            m_Semantics->IsAccessible(StateMachineAttributeType, NULL, TypeHelpers::GetVoidType()))
        {

            BCSYM_NamedType *AttributeName =
                m_Semantics->CreateNamedType(
                    *m_TransientSymbolCreator,
                    pResumableProc, // Context
                    4,
                    m_Compiler->AddString(L"System"),   // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    m_Compiler->AddString(L"Runtime"),   // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    m_Compiler->AddString(L"CompilerServices"),  // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    StateMachineTypeNameString,   // Name
                    StateMachineAttributeType,   // ptyp
                    NULL);  // Generic type arguments
            AttributeName->SetIsAttributeName(true);

            BCSYM_ApplAttr *AttributeSymbol =
                m_TransientSymbolCreator->GetApplAttr(
                    AttributeName,
                    NULL,
                    false,
                    false);

            Location loc = {0};
            loc.Invalidate();

            // A place holder imposes a dependency between the state machine type and the place holder type, which may cause problem. 
            // IAsyncStateMachineType is chosen to be the place holder, since the statemachine type is derived from it, therefore we use an existing dependency. 
            ASSERT(m_FXSymbolProvider->IsTypeAvailable(FX::IAsyncStateMachineType), "IAsyncStateMachine is not defined, but m_StateMachineType is defined.");

            StringBuffer attributeArg;
            attributeArg.AppendString(StateMachineAttributeType->GetGlobalQualifiedName());
            // Since state machine name is not valid VB syntax, which contains "$". If state machine is used here, parser will report error.
            // The work around is to use IAsyncStateMachine as a place holder, and it will be replaced by the real state machine type 
            // afeter semantics analysis. 
            attributeArg.AppendString(L"(GetType(Global.System.Runtime.CompilerServices.IAsyncStateMachine))");

            AttributeSymbol->SetExpression(
                m_TransientSymbolCreator->GetConstantExpression( 
                    &loc,
                    pResumableProc,
                    NULL, // no const expr list
                    NULL, // no const expr list
                    NULL, // no fake string
                    attributeArg.GetString(),
                    -1 ) // figure the size out based on what we pass in for the expression
                );

            ILTree::Expression *ptreeExprBound = 
                Attribute::GetBoundTreeFromApplAttr(
                    AttributeSymbol,
                    pResumableProc,
                    m_Semantics->m_Errors,
                    m_TransientSymbolCreator->GetNorlsAllocator(),
                    m_CompilerHost);

            if (ptreeExprBound)
            {

                // Replace IAsyncStateMachine by state machine type.
                VSASSERT(
                    ptreeExprBound->bilop == SX_APPL_ATTR &&
                    ptreeExprBound->AsAttributeApplicationExpression().Left &&
                    ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right &&
                    ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right->AsExpressionWithChildren().Left && 
                    ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right->AsExpressionWithChildren().Left->bilop == SX_METATYPE && 
                    ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right->AsExpressionWithChildren().Left->AsBinaryExpression().Left &&
                    ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right->AsExpressionWithChildren().Left->AsBinaryExpression().Left->bilop == SX_NOTHING, 
                    "Bound Tree shape is not right!");
                
                ptreeExprBound->AsAttributeApplicationExpression().Left->AsCallExpression().Right->AsExpressionWithChildren().Left->AsBinaryExpression().Left->AsExpression().ResultType = 
                    m_StateMachineType->IsGenericTypeBinding() 
                    ? m_StateMachineType->PGenericTypeBinding()->GetGenericType() 
                    : m_StateMachineType;
            
                AttributeSymbol->SetBoundTree(&ptreeExprBound->AsAttributeApplicationExpression());
				
                // This attribute can be attached directly to the symbol if decomplation is gone, Roslyn should consider it. 
                m_Semantics->m_SymbolsCreatedDuringInterpretation->GetResumableMethodToStateMachineAttrMap()->TransactHashAdd(pResumableProc, AttributeSymbol);
            }
        }
    }
}



