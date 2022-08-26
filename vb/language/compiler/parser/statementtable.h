#ifdef DEF_STATEMENT

DEF_STATEMENT(SyntaxError                                    , Statement                                 , AsStatement)
DEF_STATEMENT(Empty                                          , Statement                                 , AsStatement)
DEF_STATEMENT(CCConst                                        , CCConstStatement                          , AsCCConst)
DEF_STATEMENT(CCIf                                           , CCIfStatement                             , AsCCIf)
DEF_STATEMENT(CCElseIf                                       , CCIfStatement                             , AsCCIf)
DEF_STATEMENT(CCElse                                         , CCElseStatement                           , AsCCElse)
DEF_STATEMENT(CCEndIf                                        , CCEndStatement                            , AsCCEnd)
DEF_STATEMENT(Region                                         , RegionStatement                           , AsRegion)
DEF_STATEMENT(Structure                                      , TypeStatement                             , AsType)
DEF_STATEMENT(Enum                                           , EnumTypeStatement                         , AsEnumType)
DEF_STATEMENT(Interface                                      , TypeStatement                             , AsType)
DEF_STATEMENT(Class                                          , TypeStatement                             , AsType)
DEF_STATEMENT(Module                                         , TypeStatement                             , AsType)
DEF_STATEMENT(Namespace                                      , NamespaceStatement                        , AsNamespace)
DEF_STATEMENT(ProcedureDeclaration                           , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(FunctionDeclaration                            , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(ConstructorDeclaration                         , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(OperatorDeclaration                            , OperatorDefinitionStatement               , AsOperatorDefinition)
DEF_STATEMENT(DelegateProcedureDeclaration                   , DelegateDeclarationStatement              , AsDelegateDeclaration)
DEF_STATEMENT(DelegateFunctionDeclaration                    , DelegateDeclarationStatement              , AsDelegateDeclaration)
DEF_STATEMENT(EventDeclaration                               , EventDeclarationStatement                 , AsEventDeclaration)
DEF_STATEMENT(BlockEventDeclaration                          , BlockEventDeclarationStatement            , AsBlockEventDeclaration)
DEF_STATEMENT(AddHandlerDeclaration                          , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(RemoveHandlerDeclaration                       , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(RaiseEventDeclaration                          , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(ForeignProcedureDeclaration                    , ForeignMethodDeclarationStatement         , AsForeignMethodDeclaration)
DEF_STATEMENT(ForeignFunctionDeclaration                     , ForeignMethodDeclarationStatement         , AsForeignMethodDeclaration)
DEF_STATEMENT(ForeignFunctionNone                            , ForeignMethodDeclarationStatement         , AsForeignMethodDeclaration)
DEF_STATEMENT(Property                                       , PropertyStatement                         , AsProperty)
DEF_STATEMENT(PropertyGet                                    , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(PropertySet                                    , MethodDefinitionStatement                 , AsMethodDefinition)
DEF_STATEMENT(AutoProperty                                   , AutoPropertyStatement                     , AsAutoProperty)
DEF_STATEMENT(Enumerator                                     , EnumeratorStatement                       , AsEnumerator)
DEF_STATEMENT(EnumeratorWithValue                            , EnumeratorWithValueStatement              , AsEnumeratorWithValue)
DEF_STATEMENT(VariableDeclaration                            , VariableDeclarationStatement              , AsVariableDeclaration)
DEF_STATEMENT(Implements                                     , TypeListStatement                         , AsTypeList)
DEF_STATEMENT(Inherits                                       , TypeListStatement                         , AsTypeList)
DEF_STATEMENT(Imports                                        , ImportsStatement                          , AsImports)
DEF_STATEMENT(OptionUnknown                                  , OptionStatement                           , AsOption)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(FirstOption,OptionUnknown)

#endif

DEF_STATEMENT(OptionInvalid                                  , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionCompareNone                              , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionCompareText                              , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionCompareBinary                            , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionExplicitOn                               , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionExplicitOff                              , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionStrictOn                                 , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionStrictOff                                , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionInferOn                                  , OptionStatement                           , AsOption)
DEF_STATEMENT(OptionInferOff                                 , OptionStatement                           , AsOption)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(LastOption,OptionInferOff)

#endif

DEF_STATEMENT(Attribute                                      , AttributeStatement                        , AsAttribute)
DEF_STATEMENT(File                                           , FileBlockStatement                        , AsFileBlock)
DEF_STATEMENT(ProcedureBody                                  , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(PropertyGetBody                                , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(PropertySetBody                                , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(FunctionBody                                   , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(OperatorBody                                   , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(AddHandlerBody                                 , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(RemoveHandlerBody                              , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(RaiseEventBody                                 , MethodBodyStatement                       , AsMethodBody)
DEF_STATEMENT(LambdaBody                                     , LambdaBodyStatement                       , AsLambdaBody)

DEF_STATEMENT(CommentBlock                                   , CommentBlockStatement                     , AsCommentBlock)
DEF_STATEMENT(BlockIf                                        , IfStatement                               , AsIf)
DEF_STATEMENT(LineIf                                         , IfStatement                               , AsIf)
DEF_STATEMENT(ElseIf                                         , ElseIfStatement                           , AsElseIf) // (sibling of If)
DEF_STATEMENT(BlockElse                                      , ElseStatement                             , AsElse) // (sibling of If)
DEF_STATEMENT(LineElse                                       , ElseStatement                             , AsElse) // (sibling of If)
DEF_STATEMENT(Select                                         , SelectStatement                           , AsSelect) // (Case statements as children)
DEF_STATEMENT(Case                                           , CaseStatement                             , AsCase)
DEF_STATEMENT(CaseElse                                       , ExecutableBlockStatement                  , AsExecutableBlock)
DEF_STATEMENT(Try                                            , ExecutableBlockStatement                  , AsExecutableBlock)
DEF_STATEMENT(Catch                                          , CatchStatement                            , AsCatch)
DEF_STATEMENT(Finally                                        , FinallyStatement                          , AsFinally)
DEF_STATEMENT(ForFromTo                                      , ForFromToStatement                        , AsForFromTo)
DEF_STATEMENT(ForEachIn                                      , ForEachInStatement                        , AsForEachIn)
DEF_STATEMENT(While                                          , ExpressionBlockStatement                  , AsExpressionBlock)
DEF_STATEMENT(DoWhileTopTest                                 , TopTestDoStatement                        , AsTopTestDo)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(FirstDoLoop, DoWhileTopTest)

#endif

DEF_STATEMENT(DoUntilTopTest                                 , TopTestDoStatement                        , AsTopTestDo)
DEF_STATEMENT(DoWhileBottomTest                              , ExpressionBlockStatement                  , AsExpressionBlock)
DEF_STATEMENT(DoUntilBottomTest                              , ExpressionBlockStatement                  , AsExpressionBlock)
DEF_STATEMENT(DoForever                                      , ExpressionBlockStatement                  , AsExpressionBlock) // (null expression)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(LastDoLoop, DoForever)

#endif

DEF_STATEMENT(Using                                          , UsingStatement                            , AsUsing)
DEF_STATEMENT(With                                           , ExpressionBlockStatement                  , AsExpressionBlock)
// End constructs for block statements. These are always siblings of the block they end.
DEF_STATEMENT(EndIf                                          , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndUsing                                       , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndWith                                        , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndSelect                                      , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndStructure                                   , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndEnum                                        , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndInterface                                   , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndClass                                       , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndModule                                      , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndNamespace                                   , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndSub                                         , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndFunction                                    , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndGet                                         , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndSet                                         , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndProperty                                    , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndOperator                                    , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndEvent                                       , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndAddHandler                                  , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndRemoveHandler                               , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndRaiseEvent                                  , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndNext                                        , EndNextStatement                          , AsEndNext)
DEF_STATEMENT(EndWhile                                       , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndLoopWhile                                   , BottomTestLoopStatement                   , AsBottomTestLoop) // (If the loop has a bottom test, it appears in the Do and the EndLoop.)
DEF_STATEMENT(EndLoopUntil                                   , BottomTestLoopStatement                   , AsBottomTestLoop) // (If the loop has a bottom test, it appears in the Do and the EndLoop.)
DEF_STATEMENT(EndLoop                                        , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndTry                                         , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndSyncLock                                    , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndRegion                                      , CCEndStatement                            , AsCCEnd)
DEF_STATEMENT(EndCommentBlock                                , Statement                                 , AsStatement)
DEF_STATEMENT(EndUnknown                                     , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(EndInvalid                                     , EndBlockStatement                         , AsStatement)
DEF_STATEMENT(Label                                          , LabelReferenceStatement                   , AsLabelReference)
DEF_STATEMENT(Goto                                           , LabelReferenceStatement                   , AsLabelReference)
DEF_STATEMENT(Return                                         , ExpressionStatement                       , AsExpression)
DEF_STATEMENT(OnError                                        , OnErrorStatement                          , AsOnError)
DEF_STATEMENT(Resume                                         , ResumeStatement                           , AsResume)
DEF_STATEMENT(Call                                           , CallStatement                             , AsCall)
DEF_STATEMENT(RaiseEvent                                     , RaiseEventStatement                       , AsRaiseEvent)
DEF_STATEMENT(Assign                                         , AssignmentStatement                       , AsAssignment)
// The order of the assign operator opcodes is the same as the
// corresponding expression operator opcodes.
DEF_STATEMENT(AssignPlus                                     , AssignmentStatement                       , AsAssignment)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(FirstOperatorAssign, AssignPlus)

#endif

DEF_STATEMENT(AssignMinus                                    , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignMultiply                                 , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignDivide                                   , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignPower                                    , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignIntegralDivide                           , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignConcatenate                              , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignShiftLeft                                , AssignmentStatement                       , AsAssignment)
DEF_STATEMENT(AssignShiftRight                               , AssignmentStatement                       , AsAssignment)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(LastOperatorAssign, AssignShiftRight)

#endif

DEF_STATEMENT(Stop                                           , Statement                                 , AsStatement)
DEF_STATEMENT(End                                            , Statement                                 , AsStatement)
DEF_STATEMENT(ContinueDo                                     , Statement                                 , AsStatement)
DEF_STATEMENT(ContinueFor                                    , Statement                                 , AsStatement)
DEF_STATEMENT(ContinueWhile                                  , Statement                                 , AsStatement)
DEF_STATEMENT(ContinueUnknown                                , Statement                                 , AsStatement)
DEF_STATEMENT(ContinueInvalid                                , Statement                                 , AsStatement)
DEF_STATEMENT(ExitDo                                         , Statement                                 , AsStatement)
DEF_STATEMENT(ExitFor                                        , Statement                                 , AsStatement)
DEF_STATEMENT(ExitSub                                        , Statement                                 , AsStatement)
DEF_STATEMENT(ExitFunction                                   , Statement                                 , AsStatement)
DEF_STATEMENT(ExitOperator                                   , Statement                                 , AsStatement)
DEF_STATEMENT(ExitProperty                                   , Statement                                 , AsStatement)
DEF_STATEMENT(ExitTry                                        , Statement                                 , AsStatement)
DEF_STATEMENT(ExitSelect                                     , Statement                                 , AsStatement)
DEF_STATEMENT(ExitWhile                                      , Statement                                 , AsStatement)
DEF_STATEMENT(ExitUnknown                                    , Statement                                 , AsStatement)
DEF_STATEMENT(ExitInvalid                                    , Statement                                 , AsStatement)
DEF_STATEMENT(AssignMid                                      , AssignMidStatement                        , AsAssignMid)
DEF_STATEMENT(Erase                                          , EraseStatement                            , AsErase)
DEF_STATEMENT(Error                                          , ExpressionStatement                       , AsExpression)
DEF_STATEMENT(Throw                                          , ExpressionStatement                       , AsExpression)
DEF_STATEMENT(Redim                                          , RedimStatement                            , AsRedim)
DEF_STATEMENT(AddHandler                                     , HandlerStatement                          , AsHandler)
DEF_STATEMENT(RemoveHandler                                  , HandlerStatement                          , AsHandler)
DEF_STATEMENT(HiddenBlock                                    , HiddenBlockStatement                      , AsHiddenBlock)
DEF_STATEMENT(SyncLock                                       , ExpressionBlockStatement                  , AsExpressionBlock)
DEF_STATEMENT(Await                                          , ExpressionStatement                       , AsExpression)
DEF_STATEMENT(Yield                                          , ExpressionStatement                       , AsExpression)

#ifdef DEF_STATEMENT_ENUM_VALUE

DEF_STATEMENT_ENUM_VALUE(MaxOpcodes, Yield)

#endif

#endif
