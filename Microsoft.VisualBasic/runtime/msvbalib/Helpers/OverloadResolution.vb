' Copyright (c) Microsoft Corporation.  All rights reserved.

Option Strict On

Imports System
Imports System.Reflection
Imports System.Collections.Generic
Imports System.Diagnostics
Imports System.Text

Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.ConversionResolution
Imports Microsoft.VisualBasic.CompilerServices.Utils

#Const BINDING_LOG = False
#Const GENERICITY_LOG = False

Namespace Microsoft.VisualBasic.CompilerServices

    Friend Class OverloadResolution
        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend Enum ResolutionFailure
            None
            MissingMember
            InvalidArgument
            AmbiguousMatch
            InvalidTarget
        End Enum


        'perhaps this could go into the Symbols utility module
        Private Shared Function IsExactSignatureMatch( _
            ByVal LeftSignature As ParameterInfo(), _
            ByVal LeftTypeParameterCount As Integer, _
            ByVal RightSignature As ParameterInfo(), _
            ByVal RightTypeParameterCount As Integer) As Boolean

            Dim LongerSignature As ParameterInfo()
            Dim ShorterSignature As ParameterInfo()

            If LeftSignature.Length >= RightSignature.Length Then
                LongerSignature = LeftSignature
                ShorterSignature = RightSignature
            Else
                LongerSignature = RightSignature
                ShorterSignature = LeftSignature
            End If

            'If the signatures differ in length, then the extra parameters of the
            'longer signature must all be optional to be an exact match.

            For Index As Integer = ShorterSignature.Length To LongerSignature.Length - 1
                If Not LongerSignature(Index).IsOptional Then
                    Return False
                End If
            Next

            For i As Integer = 0 To ShorterSignature.Length - 1

                Dim Type1 As Type = ShorterSignature(i).ParameterType
                Dim Type2 As Type = LongerSignature(i).ParameterType

                If Type1.IsByRef Then Type1 = Type1.GetElementType
                If Type2.IsByRef Then Type2 = Type2.GetElementType

                If Type1 IsNot Type2 AndAlso _
                   (Not ShorterSignature(i).IsOptional OrElse _
                    Not LongerSignature(i).IsOptional) Then
                    Return False
                End If
            Next

            Return True
        End Function

        Private Enum ComparisonType
            ParameterSpecificty
            GenericSpecificityBasedOnMethodGenericParams
            GenericSpecificityBasedOnTypeGenericParams
        End Enum

        Private Shared Sub CompareNumericTypeSpecificity( _
            ByVal LeftType As Type, _
            ByVal RightType As Type, _
            ByRef LeftWins As Boolean, _
            ByRef RightWins As Boolean)

            'This function implements the notion that signed types are
            'preferred over unsigned types during overload resolution.

            Debug.Assert(IsNumericType(LeftType) AndAlso Not IsEnum(LeftType) AndAlso _
                         IsNumericType(RightType) AndAlso Not IsEnum(RightType), _
                         "expected only numerics here. : #12/10/2003#")

            If LeftType Is RightType Then
                'Do nothing since neither wins.
            Else
                Debug.Assert(GetTypeCode(LeftType) <> GetTypeCode(RightType), _
                             "this should have been caught above")

                If NumericSpecificityRank(GetTypeCode(LeftType)) < _
                        NumericSpecificityRank(GetTypeCode(RightType)) Then
                    LeftWins = True
                Else
                    RightWins = True
                End If
            End If

            Return
        End Sub

        Private Shared Sub CompareParameterSpecificity( _
            ByVal ArgumentType As Type, _
            ByVal LeftParameter As ParameterInfo, _
            ByVal LeftProcedure As MethodBase, _
            ByVal ExpandLeftParamArray As Boolean, _
            ByVal RightParameter As ParameterInfo, _
            ByVal RightProcedure As MethodBase, _
            ByVal ExpandRightParamArray As Boolean, _
            ByRef LeftWins As Boolean, _
            ByRef RightWins As Boolean, _
            ByRef BothLose As Boolean)


            BothLose = False
            Dim LeftType As Type = LeftParameter.ParameterType
            Dim RightType As Type = RightParameter.ParameterType

            If LeftType.IsByRef Then LeftType = GetElementType(LeftType)
            If RightType.IsByRef Then RightType = GetElementType(RightType)

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandLeftParamArray AndAlso IsParamArray(LeftParameter) Then
                LeftType = GetElementType(LeftType)
            End If

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandRightParamArray AndAlso IsParamArray(RightParameter) Then
                RightType = GetElementType(RightType)
            End If

            If IsNumericType(LeftType) AndAlso IsNumericType(RightType) AndAlso _
               Not IsEnum(LeftType) AndAlso Not IsEnum(RightType) Then

                CompareNumericTypeSpecificity(LeftType, RightType, LeftWins, RightWins)
                Return
            End If

            'If both the types are different only by generic method type parameters
            'with the same index position, then treat as identity.

            If LeftProcedure IsNot Nothing AndAlso _
               RightProcedure IsNot Nothing AndAlso _
               IsRawGeneric(LeftProcedure) AndAlso _
               IsRawGeneric(RightProcedure) Then

                If LeftType Is RightType Then Return 'Check this first--shortcut.

                Dim LeftIndex As Integer = IndexIn(LeftType, LeftProcedure)
                Dim RightIndex As Integer = IndexIn(RightType, RightProcedure)

                If LeftIndex = RightIndex AndAlso LeftIndex >= 0 Then Return
            End If

            Dim OperatorMethod As Method = Nothing
            Dim LeftToRight As ConversionClass = ClassifyConversion(RightType, LeftType, OperatorMethod)

            If LeftToRight = ConversionClass.Identity Then Return

            If LeftToRight = ConversionClass.Widening Then

                If OperatorMethod IsNot Nothing AndAlso _
                   ClassifyConversion(LeftType, RightType, OperatorMethod) = ConversionClass.Widening Then

                    ' Although W<-->W conversions don't exist in the set of predefined conversions,
                    ' it can occur with user-defined conversions.  If the two param types widen to each other
                    ' (necessarily by using user-defined conversions), and the argument type is known and
                    ' is identical to one of the parameter types, then that parameter wins.  Otherwise,
                    ' if the arugment type is not specified, we can't make a decision and both lose.

                    If ArgumentType IsNot Nothing AndAlso ArgumentType Is LeftType Then
                        LeftWins = True
                        Return
                    End If

                    If ArgumentType IsNot Nothing AndAlso ArgumentType Is RightType Then
                        RightWins = True
                        Return
                    End If

                    BothLose = True
                    Return

                End If

                LeftWins = True
                Return
            End If

            Dim RightToLeft As ConversionClass = ClassifyConversion(LeftType, RightType, OperatorMethod)

            If RightToLeft = ConversionClass.Widening Then
                RightWins = True
                Return
            End If

            BothLose = True
            Return

        End Sub

        Private Shared Sub CompareGenericityBasedOnMethodGenericParams( _
            ByVal LeftParameter As ParameterInfo, _
            ByVal RawLeftParameter As ParameterInfo, _
            ByVal LeftMember As Method, _
            ByVal ExpandLeftParamArray As Boolean, _
            ByVal RightParameter As ParameterInfo, _
            ByVal RawRightParameter As ParameterInfo, _
            ByVal RightMember As Method, _
            ByVal ExpandRightParamArray As Boolean, _
            ByRef LeftIsLessGeneric As Boolean, _
            ByRef RightIsLessGeneric As Boolean, _
            ByRef SignatureMismatch As Boolean)

            If Not LeftMember.IsMethod OrElse Not RightMember.IsMethod Then
                Return
            End If

            Dim LeftType As Type = LeftParameter.ParameterType
            Dim RightType As Type = RightParameter.ParameterType

            'Since generic methods are instantiated by this point, the parameter
            'types are bound. However, we need to compare against the unbound types.
            Dim RawLeftType As Type = RawLeftParameter.ParameterType
            Dim RawRightType As Type = RawRightParameter.ParameterType

            If LeftType.IsByRef Then
                LeftType = GetElementType(LeftType)
                RawLeftType = GetElementType(RawLeftType)
            End If

            If RightType.IsByRef Then
                RightType = GetElementType(RightType)
                RawRightType = GetElementType(RawRightType)
            End If

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandLeftParamArray AndAlso IsParamArray(LeftParameter) Then
                LeftType = GetElementType(LeftType)
                RawLeftType = GetElementType(RawLeftType)
            End If

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandRightParamArray AndAlso IsParamArray(RightParameter) Then
                RightType = GetElementType(RightType)
                RawRightType = GetElementType(RawRightType)
            End If
#If TELESTO Then
            If LeftType IsNot RightType Then
#Else
            'Need to check type equivalency for the NoPIA case
            If LeftType IsNot RightType AndAlso Not IsEquivalentType(LeftType, RightType) Then
#End If
                'The signatures of the two methods are not identical and so the "least generic" rule
                'does not apply.
                SignatureMismatch = True
                Return
            End If

            Dim LeftProcedure As MethodBase = LeftMember.AsMethod
            Dim RightProcedure As MethodBase = RightMember.AsMethod

            If IsGeneric(LeftProcedure) Then LeftProcedure = DirectCast(LeftProcedure, MethodInfo).GetGenericMethodDefinition
            If IsGeneric(RightProcedure) Then RightProcedure = DirectCast(RightProcedure, MethodInfo).GetGenericMethodDefinition

            ' Only references to generic parameters of the procedures count. For the purpose of this
            ' function, references to generic parameters of a type do not make a procedure more generic.

            If RefersToGenericParameter(RawLeftType, LeftProcedure) Then
                If Not RefersToGenericParameter(RawRightType, RightProcedure) Then
                    RightIsLessGeneric = True
                End If
            ElseIf RefersToGenericParameter(RawRightType, RightProcedure) Then
                If Not RefersToGenericParameter(RawLeftType, LeftProcedure) Then
                    LeftIsLessGeneric = True
                End If
            End If

        End Sub

        Private Shared Sub CompareGenericityBasedOnTypeGenericParams( _
            ByVal LeftParameter As ParameterInfo, _
            ByVal RawLeftParameter As ParameterInfo, _
            ByVal LeftMember As Method, _
            ByVal ExpandLeftParamArray As Boolean, _
            ByVal RightParameter As ParameterInfo, _
            ByVal RawRightParameter As ParameterInfo, _
            ByVal RightMember As Method, _
            ByVal ExpandRightParamArray As Boolean, _
            ByRef LeftIsLessGeneric As Boolean, _
            ByRef RightIsLessGeneric As Boolean, _
            ByRef SignatureMismatch As Boolean)

            Dim LeftType As Type = LeftParameter.ParameterType
            Dim RightType As Type = RightParameter.ParameterType

            'Since generic methods are instantiated by this point, the parameter
            'types are bound. However, we need to compare against the unbound types.
            Dim RawLeftType As Type = RawLeftParameter.ParameterType
            Dim RawRightType As Type = RawRightParameter.ParameterType

            If LeftType.IsByRef Then
                LeftType = GetElementType(LeftType)
                RawLeftType = GetElementType(RawLeftType)
            End If

            If RightType.IsByRef Then
                RightType = GetElementType(RightType)
                RawRightType = GetElementType(RawRightType)
            End If

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandLeftParamArray AndAlso IsParamArray(LeftParameter) Then
                LeftType = GetElementType(LeftType)
                RawLeftType = GetElementType(RawLeftType)
            End If

            'UNDONE: don't use IsParamArray -- for speed, pass that in as a parameter.
            If ExpandRightParamArray AndAlso IsParamArray(RightParameter) Then
                RightType = GetElementType(RightType)
                RawRightType = GetElementType(RawRightType)
            End If

            'Need to check type equivalency for the NoPIA case
#If TELESTO Then
            If LeftType IsNot RightType Then
#Else
            If LeftType IsNot RightType AndAlso Not IsEquivalentType(LeftType, RightType) Then
#End If
                'The signatures of the two methods are not identical and so the "least generic" rule
                'does not apply.
                SignatureMismatch = True
                Return
            End If

            ' Only references to generic parameters of the generic types count. For the purpose of this
            ' function, references to generic parameters of a method do not make a procedure more generic.
            '
            Dim LeftDeclaringType As Type = LeftMember.RawDeclaringType
            Dim RightDeclaringType As Type = RightMember.RawDeclaringType

#If GENERICITY_LOG Then
            Console.Writeline("----------CompareGenericityBasedOnTypeGenericParams---------")
            Console.Writeline("LeftType: " & LeftType.MetaDataToken & " - " & LeftType.ToString())
            Console.Writeline("LeftRawType: " & RawLeftType.MetaDataToken & " - " & RawLeftType.ToString())
            Console.Writeline("LeftDeclaringType: " & LeftDeclaringType.MetaDataToken & " - " & LeftDeclaringType.ToString())
            Console.Writeline("RightType: " & RightType.MetaDataToken & " - " & RightType.ToString())
            Console.Writeline("RightRawType: " & RawRightType.MetaDataToken & " - " & RawRightType.ToString())
            Console.Writeline("RightDeclaringType: " & RightDeclaringType.MetaDataToken & " - " & RightDeclaringType.ToString())
#End If

            If RefersToGenericParameterCLRSemantics(RawLeftType, LeftDeclaringType) Then
                If Not RefersToGenericParameterCLRSemantics(RawRightType, RightDeclaringType) Then
                    RightIsLessGeneric = True
                End If
            ElseIf RefersToGenericParameterCLRSemantics(RawRightType, RightDeclaringType) Then
                LeftIsLessGeneric = True
            End If
        End Sub

        Private Shared Function LeastGenericProcedure( _
            ByVal Left As Method, _
            ByVal Right As Method, _
            ByVal CompareGenericity As ComparisonType, _
            ByRef SignatureMismatch As Boolean) As Method

            Dim LeftWinsAtLeastOnce As Boolean = False
            Dim RightWinsAtLeastOnce As Boolean = False
            SignatureMismatch = False

            If Not Left.IsMethod OrElse Not Right.IsMethod Then Return Nothing

            Dim ParamIndex As Integer = 0
            Dim LeftParamsLen As Integer = Left.Parameters.Length
            Dim RightParamsLen As Integer = Right.Parameters.Length

            Do While ParamIndex < LeftParamsLen AndAlso ParamIndex < RightParamsLen

                Select Case CompareGenericity
                    Case ComparisonType.GenericSpecificityBasedOnMethodGenericParams

                        CompareGenericityBasedOnMethodGenericParams( _
                            Left.Parameters(ParamIndex), _
                            Left.RawParameters(ParamIndex), _
                            Left, _
                            Left.ParamArrayExpanded, _
                            Right.Parameters(ParamIndex), _
                            Right.RawParameters(ParamIndex), _
                            Right, _
                            False, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            SignatureMismatch)

                    Case ComparisonType.GenericSpecificityBasedOnTypeGenericParams

                        CompareGenericityBasedOnTypeGenericParams( _
                            Left.Parameters(ParamIndex), _
                            Left.RawParameters(ParamIndex), _
                            Left, _
                            Left.ParamArrayExpanded, _
                            Right.Parameters(ParamIndex), _
                            Right.RawParameters(ParamIndex), _
                            Right, _
                            False, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            SignatureMismatch)

                    Case Else
#If TELESTO Then
                        Debug.Assert(False, "Unexpected comparison type!!!") ' Silverlight CLR does not have Debug.Fail.
#Else
                        Debug.Fail("Unexpected comparison type!!!")
#End If
                End Select

                If SignatureMismatch OrElse (LeftWinsAtLeastOnce AndAlso RightWinsAtLeastOnce) Then
                    Return Nothing
                End If

                ParamIndex += 1
            Loop

            Debug.Assert(Not (LeftWinsAtLeastOnce AndAlso RightWinsAtLeastOnce), _
                         "Least generic method logic is confused.")

            If ParamIndex < LeftParamsLen OrElse ParamIndex < RightParamsLen Then
                'The procedures have different numbers of parameters, and so don't have matching signatures.
                Return Nothing
            End If

            If LeftWinsAtLeastOnce Then
                Return Left
            End If

            If RightWinsAtLeastOnce Then
                Return Right
            End If

            Return Nothing
        End Function

        Friend Shared Function LeastGenericProcedure( _
            ByVal Left As Method, _
            ByVal Right As Method) As Method

            If Not (Left.IsGeneric OrElse _
                    Right.IsGeneric OrElse _
                    IsGeneric(Left.DeclaringType) OrElse _
                    IsGeneric(Right.DeclaringType)) Then
                Return Nothing
            End If

            Dim SignatureMismatch As Boolean = False

            Dim LeastGeneric As Method = _
                LeastGenericProcedure( _
                    Left, _
                    Right, _
                    ComparisonType.GenericSpecificityBasedOnMethodGenericParams, _
                    SignatureMismatch)

            If LeastGeneric Is Nothing AndAlso Not SignatureMismatch Then

                LeastGeneric = _
                    LeastGenericProcedure( _
                        Left, _
                        Right, _
                        ComparisonType.GenericSpecificityBasedOnTypeGenericParams, _
                        SignatureMismatch)
            End If

            Return LeastGeneric

        End Function

        Private Shared Sub InsertIfMethodAvailable( _
            ByVal NewCandidate As MemberInfo, _
            ByVal NewCandidateSignature As ParameterInfo(), _
            ByVal NewCandidateParamArrayIndex As Integer, _
            ByVal ExpandNewCandidateParamArray As Boolean, _
            ByVal Arguments As Object(), _
            ByVal ArgumentCount As Integer, _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal CollectOnlyOperators As Boolean, _
            ByVal Candidates As List(Of Method),
            ByVal BaseReference As Container)

            'Note that Arguments, ArgumentNames and TypeNames will be nothing when collecting operators.
            '
            Debug.Assert(Arguments Is Nothing OrElse Arguments.Length = ArgumentCount, "Inconsistency in arguments!!!")

            Dim NewCandidateNode As Method = Nothing

            'If we're collecting only operators, then hiding by name and signature doesn't apply (neither do
            'ParamArrays), so skip all of this logic.
            If Not CollectOnlyOperators Then

                Dim NewCandidateMethod As MethodBase = TryCast(NewCandidate, MethodBase)
                Dim InferenceFailedForNewCandidate As Boolean = False

                ' Note that operators cannot be generic methods
                '
                ' Need to complete type argument inference for generic methods when no type arguments
                ' have been supplied and when type arguments are supplied, the generic method needs to
                ' to be instantiated. Need to complete this so early in the overload process so that
                ' hid-by-sig, paramarray disambiguation etc. are done using the substitued signature.
                '
                If NewCandidate.MemberType = MemberTypes.Method AndAlso IsRawGeneric(NewCandidateMethod) Then

                    NewCandidateNode = _
                        New Method( _
                            NewCandidateMethod, _
                            NewCandidateSignature, _
                            NewCandidateParamArrayIndex, _
                            ExpandNewCandidateParamArray)

                    ' Inferring of type arguments is done when when determining the callability of this
                    ' procedure with these arguments by comparing them against the corresponding parameters.
                    '
                    ' Note that although RejectUncallableProcedure needs to be invoked on the non-generics
                    ' candidates too, it is not done here because some of the candidates might be rejected
                    ' for various reasons like hide-by-sig, paramarray disambiguation, etc. and RejectUncall-
                    ' -ableProcedure would not have to be invoked for them. This is especially important
                    ' because the RejectUncallableProcedure task is expensive.
                    '

                    RejectUncallableProcedure( _
                        NewCandidateNode, _
                        Arguments, _
                        ArgumentNames, _
                        TypeArguments)


                    ' Get the instantiated method for this candidate
                    '
                    NewCandidate = NewCandidateNode.AsMethod
                    NewCandidateSignature = NewCandidateNode.Parameters

                End If

                ' Verify if TypeInference succeeded. This should only happen for Methods
                If NewCandidate IsNot Nothing AndAlso _
                    NewCandidate.MemberType = MemberTypes.Method AndAlso _
                    IsRawGeneric(TryCast(NewCandidate, MethodBase)) Then
                    InferenceFailedForNewCandidate = True
                End If

                For Index As Integer = 0 To Candidates.Count - 1

                    Dim Existing As Method = Candidates.Item(Index)
                    If Existing Is Nothing Then Continue For 'This item was killed earlier, so skip it.

                    Dim ExistingCandidateSignature As ParameterInfo() = Existing.Parameters
                    Dim ExistingCandidate As MethodBase
                    If Existing.IsMethod Then ExistingCandidate = Existing.AsMethod Else ExistingCandidate = Nothing

                    If NewCandidate = Existing Then Continue For 'UNDONE how is this possible?

                    Dim NewCandidateParameterIndex As Integer = 0
                    Dim ExistingCandidateParameterIndex As Integer = 0

                    For CurrentArgument As Integer = 1 To ArgumentCount

                        Dim BothLose As Boolean = False
                        Dim NewCandidateWins As Boolean = False
                        Dim ExistingCandidateWins As Boolean = False

                        CompareParameterSpecificity( _
                            Nothing, _
                            NewCandidateSignature(NewCandidateParameterIndex), _
                            NewCandidateMethod, _
                            ExpandNewCandidateParamArray, _
                            ExistingCandidateSignature(ExistingCandidateParameterIndex), _
                            ExistingCandidate, _
                            Existing.ParamArrayExpanded, _
                            NewCandidateWins, _
                            ExistingCandidateWins, _
                            BothLose)

                        If BothLose Or NewCandidateWins Or ExistingCandidateWins Then
                            GoTo continueloop
                        End If

                        'If a parameter is a param array, there is no next parameter and so advancing
                        'through the parameter list is bad.

                        If NewCandidateParameterIndex <> NewCandidateParamArrayIndex OrElse Not ExpandNewCandidateParamArray Then
                            NewCandidateParameterIndex += 1
                        End If

                        If ExistingCandidateParameterIndex <> Existing.ParamArrayIndex OrElse Not Existing.ParamArrayExpanded Then
                            ExistingCandidateParameterIndex += 1
                        End If

                    Next

                    'UNDONE: the call to GetTypeParameters will create a cloned array instance each time. Fix this perf issue.
                    Dim ExactSignature As Boolean = _
                        IsExactSignatureMatch( _
                            NewCandidateSignature, _
                            GetTypeParameters(NewCandidate).Length, _
                            Existing.Parameters, _
                            Existing.TypeParameters.Length)

                    If Not ExactSignature Then

                        ' If inference failed for any of the candidates, then don't compare them.
                        '
                        ' This simple strategy besides fixing the problems associated with an inference
                        ' failed candidate beating a inference passing candidate also helps with better
                        ' error reporting by showing the inference failed candidates too.
                        '
                        If InferenceFailedForNewCandidate OrElse _
                           (ExistingCandidate IsNot Nothing AndAlso IsRawGeneric(ExistingCandidate)) Then
                            Continue For
                        End If


                        If Not ExpandNewCandidateParamArray AndAlso Existing.ParamArrayExpanded Then
                            'Delete current item from list and continue.
                            Candidates.Item(Index) = Nothing
                            Continue For

                        ElseIf ExpandNewCandidateParamArray AndAlso Not Existing.ParamArrayExpanded Then
                            Return

                        ElseIf Not ExpandNewCandidateParamArray AndAlso Not Existing.ParamArrayExpanded Then
                            'In theory, this shouldn't happen, but another language could
                            'theoretically define two methods with optional arguments that
                            'end up being equivalent. So don't prefer one over the other.
                            Continue For

                        Else
                            'If both are expanded, then see if one uses more on actual
                            'parameters than the other. If so, we prefer the one that uses
                            'more on actual parameters.

                            If (NewCandidateParameterIndex > ExistingCandidateParameterIndex) Then
                                'Delete current item from list and continue.
                                Candidates.Item(Index) = Nothing
                                Continue For

                            ElseIf ExistingCandidateParameterIndex > NewCandidateParameterIndex Then
                                Return

                            End If

                            Continue For

                        End If
                    Else
                        Debug.Assert((BaseReference IsNot Nothing AndAlso BaseReference.IsWindowsRuntimeObject) OrElse
                                     IsOrInheritsFrom(Existing.DeclaringType, NewCandidate.DeclaringType), _
                                     "expected inheritance or WinRT collection types here")

                        If NewCandidate.DeclaringType Is Existing.DeclaringType Then
                            'If the two members are declared in the same container, both should be added to the set
                            'of overloads.  This results in intelligent ambiguity error messages.
                            Exit For

                        End If

                        ' If the base container is a WinRT object implementing collection interfaces they could have
                        ' the same method name with the same signature. We need to add them to the set to throw 
                        ' ambiguity errors.
                        If BaseReference IsNot Nothing AndAlso BaseReference.IsWindowsRuntimeObject() AndAlso
                           Symbols.IsCollectionInterface(NewCandidate.DeclaringType) AndAlso
                           Symbols.IsCollectionInterface(Existing.DeclaringType) Then
                            Exit For
                        End If

                        'If inference did not fail for the base candidate, but failed for the derived candidate, then
                        'the derived candidate cannot hide the base candidate. VSWhidbey Bug 369042.
                        '
                        If Not InferenceFailedForNewCandidate AndAlso _
                           (ExistingCandidate IsNot Nothing AndAlso IsRawGeneric(ExistingCandidate)) Then
                            Continue For
                        End If

                        Return

                    End If

#If TELESTO Then
                    Debug.Assert(False,"unexpected code path")
#Else
                    Debug.Fail("unexpected code path")
#End If

continueloop:
                Next
            End If

            If NewCandidateNode IsNot Nothing Then
                Candidates.Add(NewCandidateNode)
            ElseIf NewCandidate.MemberType = MemberTypes.Property Then
                Candidates.Add( _
                    New Method( _
                        DirectCast(NewCandidate, PropertyInfo), _
                        NewCandidateSignature, _
                        NewCandidateParamArrayIndex, _
                        ExpandNewCandidateParamArray))
            Else
                Candidates.Add( _
                    New Method( _
                        DirectCast(NewCandidate, MethodBase), _
                        NewCandidateSignature, _
                        NewCandidateParamArrayIndex, _
                        ExpandNewCandidateParamArray))
            End If

        End Sub

        Friend Shared Function CollectOverloadCandidates( _
            ByVal Members As MemberInfo(), _
            ByVal Arguments As Object(), _
            ByVal ArgumentCount As Integer, _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal CollectOnlyOperators As Boolean, _
            ByVal TerminatingScope As System.Type, _
            ByRef RejectedForArgumentCount As Integer, _
            ByRef RejectedForTypeArgumentCount As Integer,
            ByVal BaseReference As Container) As List(Of Method)


            'Note that Arguments, ArgumentNames and TypeNames will be nothing when collecting operators.
            '
            Debug.Assert(Arguments Is Nothing OrElse Arguments.Length = ArgumentCount, "Inconsistency in arguments!!!")

            Dim TypeArgumentCount As Integer = 0
            If TypeArguments IsNot Nothing Then
                TypeArgumentCount = TypeArguments.Length
            End If

            Dim Candidates As List(Of Method) = New List(Of Method)(Members.Length)

            If Members.Length = 0 Then
                Return Candidates
            End If

            Dim KeepSearching As Boolean = True
            Dim Index As Integer = 0

            Do
                Dim CurrentScope As Type = Members(Index).DeclaringType

                'The terminating scope parameter controls at which point candidate collection
                'will stop. This is useful for overloaded operator resolution where the left
                'and right operands may have a common ancestor and we wish to collect the common
                'candidates only once.
                If TerminatingScope IsNot Nothing AndAlso IsOrInheritsFrom(TerminatingScope, CurrentScope) Then Exit Do
                Do
                    Dim Candidate As MemberInfo = Members(Index)
                    Dim CandidateSignature As ParameterInfo() = Nothing
                    Dim TypeParameterCount As Integer = 0

                    Select Case Candidate.MemberType

                        Case MemberTypes.Constructor, _
                             MemberTypes.Method

                            Dim CurrentMethod As MethodBase = DirectCast(Candidate, MethodBase)

                            If CollectOnlyOperators AndAlso Not IsUserDefinedOperator(CurrentMethod) Then
                                GoTo nextcandidate
                            End If

                            CandidateSignature = CurrentMethod.GetParameters
                            TypeParameterCount = GetTypeParameters(CurrentMethod).Length

                            If IsShadows(CurrentMethod) Then KeepSearching = False

                        Case MemberTypes.Property

                            If CollectOnlyOperators Then GoTo nextcandidate

                            Dim PropertyBlock As PropertyInfo = DirectCast(Candidate, PropertyInfo)
                            Dim GetMethod As MethodInfo = PropertyBlock.GetGetMethod

                            If GetMethod IsNot Nothing Then
                                CandidateSignature = GetMethod.GetParameters

                                Debug.Assert(PropertyBlock.GetSetMethod Is Nothing OrElse _
                                             IsShadows(PropertyBlock.GetSetMethod) = IsShadows(GetMethod), _
                                             "unexpected mismatched shadows on accessors")
                                If IsShadows(GetMethod) Then KeepSearching = False

                            Else
                                Dim SetMethod As MethodInfo = PropertyBlock.GetSetMethod
                                Debug.Assert(SetMethod IsNot Nothing, "must have set here")

                                Dim SetParameters As ParameterInfo() = SetMethod.GetParameters
                                CandidateSignature = New ParameterInfo(SetParameters.Length - 2) {}
                                System.Array.Copy(SetParameters, CandidateSignature, CandidateSignature.Length)

                                If IsShadows(SetMethod) Then KeepSearching = False

                            End If

                        Case MemberTypes.Custom, _
                             MemberTypes.Event, _
                             MemberTypes.Field, _
                             MemberTypes.TypeInfo, _
                             MemberTypes.NestedType

                            'All of these items automatically shadow.
                            If Not CollectOnlyOperators Then
                                KeepSearching = False
                            End If
                            GoTo nextcandidate

                        Case Else
#If TELESTO Then
                            Debug.Assert(False, "what is this?  just ignore it.") ' Silverlight CLR does not have Debug.Fail.
#Else
                            Debug.Fail("what is this?  just ignore it.")
#End If
                            GoTo nextcandidate

                    End Select


                    'We have a possible candidate method if we make it this far.  Insert it into the
                    'list if it qualifies.

                    Debug.Assert(CandidateSignature IsNot Nothing, "must have signature if we have a method")

                    Dim RequiredParameterCount As Integer = 0
                    Dim MaximumParameterCount As Integer = 0
                    Dim ParamArrayIndex As Integer = -1

                    'Weed out procedures that cannot accept the number of supplied arguments.

                    GetAllParameterCounts(CandidateSignature, RequiredParameterCount, MaximumParameterCount, ParamArrayIndex)

                    Dim HasParamArray As Boolean = ParamArrayIndex >= 0
                    If ArgumentCount < RequiredParameterCount OrElse _
                       (Not HasParamArray AndAlso ArgumentCount > MaximumParameterCount) Then
                        RejectedForArgumentCount += 1
                        GoTo nextcandidate
                    End If

                    'If type arguments have been supplied, weed out procedures that don't have an
                    'appropriate number of type parameters.

                    If TypeArgumentCount > 0 AndAlso TypeArgumentCount <> TypeParameterCount Then
                        RejectedForTypeArgumentCount += 1
                        GoTo nextcandidate
                    End If

                    ' A method with a paramarray can be considered in two forms: in an
                    ' expanded form or in an unexpanded form (i.e. as if the paramarray
                    ' decoration was not specified). Weirdly, it can apply in both forms, as 
                    ' in the case of passing Object() to ParamArray x As Object() (because
                    ' Object() converts to both Object() and Object).

                    ' Does the method apply in its unexpanded form? This can only happen if
                    ' either there is no paramarray or if the argument count matches exactly
                    ' (if it's less, then the paramarray is expanded to nothing, if it's more, 
                    ' it's expanded to one or more parameters).

                    If Not HasParamArray OrElse ArgumentCount = MaximumParameterCount Then
                        InsertIfMethodAvailable( _
                            Candidate, _
                            CandidateSignature, _
                            ParamArrayIndex, _
                            False, _
                            Arguments, _
                            ArgumentCount, _
                            ArgumentNames, _
                            TypeArguments, _
                            CollectOnlyOperators, _
                            Candidates,
                            BaseReference)
                    End If

                    'How about it's expanded form? It always applies if there's a paramarray.

                    If HasParamArray Then
                        Debug.Assert(Not CollectOnlyOperators, "didn't expect operator with paramarray")
                        InsertIfMethodAvailable( _
                            Candidate, _
                            CandidateSignature, _
                            ParamArrayIndex, _
                            True, _
                            Arguments, _
                            ArgumentCount, _
                            ArgumentNames, _
                            TypeArguments, _
                            CollectOnlyOperators, _
                            Candidates,
                            BaseReference)
                    End If

nextcandidate:
                    Index += 1

                Loop While Index < Members.Length AndAlso Members(Index).DeclaringType Is CurrentScope

            Loop While KeepSearching AndAlso Index < Members.Length

#If BINDING_LOG Then
            Console.WriteLine("== COLLECTION AND SHADOWING ==")
            For Each item As Method In Candidates
                If item Is Nothing Then
                    Console.WriteLine("dead")
                Else
                    Console.WriteLine(item.DumpContents)
                End If
            Next
#End If

            'Remove the dead entries from the list--simplifies code later on.
            'CONSIDER: this costs time, but dead entires should be relatively rare.
            Index = 0
            While Index < Candidates.Count
                If Candidates(Index) Is Nothing Then
                    Dim Span As Integer = Index + 1
                    While Span < Candidates.Count AndAlso Candidates(Span) Is Nothing
                        Span += 1
                    End While
                    Candidates.RemoveRange(Index, Span - Index)
                End If
                Index += 1
            End While

            Return Candidates
        End Function

        Private Shared Function CanConvert( _
            ByVal TargetType As Type, _
            ByVal SourceType As Type, _
            ByVal RejectNarrowingConversion As Boolean, _
            ByVal Errors As List(Of String), _
            ByVal ParameterName As String, _
            ByVal IsByRefCopyBackContext As Boolean, _
            ByRef RequiresNarrowingConversion As Boolean, _
            ByRef AllNarrowingIsFromObject As Boolean) As Boolean

            Dim ConversionResult As ConversionClass = ClassifyConversion(TargetType, SourceType, Nothing)

            Select Case ConversionResult

                Case ConversionClass.Identity, ConversionClass.Widening
                    Return True

                Case ConversionClass.Narrowing

                    If RejectNarrowingConversion Then
                        If Errors IsNot Nothing Then
                            ReportError( _
                                Errors, _
                                IIf(IsByRefCopyBackContext, ResID.ArgumentNarrowingCopyBack3, ResID.ArgumentNarrowing3), _
                                ParameterName, _
                                SourceType, _
                                TargetType)
                        End If

                        Return False
                    Else
                        RequiresNarrowingConversion = True
                        If SourceType IsNot GetType(Object) Then AllNarrowingIsFromObject = False

                        Return True
                    End If

            End Select

            If Errors IsNot Nothing Then
                ReportError( _
                    Errors, _
                    IIf(ConversionResult = ConversionClass.Ambiguous, _
                        IIf(IsByRefCopyBackContext, _
                            ResID.ArgumentMismatchAmbiguousCopyBack3, _
                            ResID.ArgumentMismatchAmbiguous3), _
                        IIf(IsByRefCopyBackContext, _
                            ResID.ArgumentMismatchCopyBack3, _
                            ResID.ArgumentMismatch3)), _
                    ParameterName, _
                    SourceType, _
                    TargetType)
            End If

            Return False
        End Function

        Private Shared Function InferTypeArgumentsFromArgument( _
            ByVal ArgumentType As Type, _
            ByVal ParameterType As Type, _
            ByVal TypeInferenceArguments As Type(), _
            ByVal TargetProcedure As MethodBase, _
            ByVal DigThroughToBasesAndImplements As Boolean) As Boolean

            Dim Inferred As Boolean = _
                InferTypeArgumentsFromArgumentDirectly( _
                    ArgumentType, _
                    ParameterType, _
                    TypeInferenceArguments, _
                    TargetProcedure, _
                    DigThroughToBasesAndImplements)


            If (Inferred OrElse _
                Not DigThroughToBasesAndImplements OrElse _
                Not IsInstantiatedGeneric(ParameterType) OrElse _
                (Not ParameterType.IsClass AndAlso Not ParameterType.IsInterface)) Then

                'can only inherit from classes or interfaces.
                'can ignore generic parameters here because it
                'were a generic parameter, inference would
                'definitely have succeeded.

                Return Inferred
            End If


            Dim RawGenericParameterType As Type = ParameterType.GetGenericTypeDefinition

            If (IsArrayType(ArgumentType)) Then

                '1. Generic IList is implemented only by one dimensional arrays
                '
                '2. If parameter type is a class, then no other inference from
                '   array is possible.
                '
                If (ArgumentType.GetArrayRank > 1 OrElse _
                    ParameterType.IsClass) Then

                    Return False
                End If
                'For arrays, change the argument type to be IList(Of Array element type)

                ArgumentType = _
                    GetType(System.Collections.Generic.IList(Of )).MakeGenericType(New Type() {ArgumentType.GetElementType})

                If (GetType(System.Collections.Generic.IList(Of )) Is RawGenericParameterType) Then
                    GoTo RetryInference
                End If

            ElseIf (Not ArgumentType.IsClass AndAlso _
                     Not ArgumentType.IsInterface) Then

                Debug.Assert(Not IsGenericParameter(ArgumentType), "Generic parameter unexpected!!!")

                Return False

            ElseIf (IsInstantiatedGeneric(ArgumentType) AndAlso _
                     ArgumentType.GetGenericTypeDefinition Is RawGenericParameterType) Then

                Return False
            End If


            If (ParameterType.IsClass) Then

                If (Not ArgumentType.IsClass) Then
                    Return False
                End If

                Dim Base As Type = ArgumentType.BaseType

                While (Base IsNot Nothing)

                    If (IsInstantiatedGeneric(Base) AndAlso _
                         Base.GetGenericTypeDefinition Is RawGenericParameterType) Then

                        Exit While
                    End If

                    Base = Base.BaseType
                End While

                ArgumentType = Base
            Else

                Dim ImplementedMatch As Type = Nothing
                For Each Implemented As Type In ArgumentType.GetInterfaces

                    If (IsInstantiatedGeneric(Implemented) AndAlso _
                        Implemented.GetGenericTypeDefinition Is RawGenericParameterType) Then

                        If (ImplementedMatch IsNot Nothing) Then
                            'Ambiguous
                            '
                            Return False
                        End If

                        ImplementedMatch = Implemented
                    End If
                Next

                ArgumentType = ImplementedMatch
            End If

            If (ArgumentType Is Nothing) Then
                Return False
            End If

RetryInference:

            Return _
                InferTypeArgumentsFromArgumentDirectly( _
                    ArgumentType, _
                    ParameterType, _
                    TypeInferenceArguments, _
                    TargetProcedure, _
                    DigThroughToBasesAndImplements)

        End Function


        Private Shared Function InferTypeArgumentsFromArgumentDirectly( _
            ByVal ArgumentType As Type, _
            ByVal ParameterType As Type, _
            ByVal TypeInferenceArguments As Type(), _
            ByVal TargetProcedure As MethodBase, _
            ByVal DigThroughToBasesAndImplements As Boolean) As Boolean

            Debug.Assert(Not ParameterType.IsByRef, "didn't expect byref parameter type here")
            Debug.Assert(IsRawGeneric(TargetProcedure), "Type inference for instantiated generic unexpected!!!")

            If Not RefersToGenericParameter(ParameterType, TargetProcedure) Then
                Return True
            End If

            'If a generic method is parameterized by T, an argument of type A matching a parameter of type
            'P can be used to infer a type for T by these patterns:
            '
            '  -- If P is T, then infer A for T
            '  -- If P is G(Of T) and A is G(Of X), then infer X for T
            '  -- If P is or implements G(Of T) and A is G(Of X), then infer X for T
            '  -- If P is Array Of T, and A is Array Of X, then infer X for T

            If IsGenericParameter(ParameterType) Then
                If AreGenericMethodDefsEqual(ParameterType.DeclaringMethod, TargetProcedure) Then
                    Dim ParameterIndex As Integer = ParameterType.GenericParameterPosition
                    If TypeInferenceArguments(ParameterIndex) Is Nothing Then
                        TypeInferenceArguments(ParameterIndex) = ArgumentType

                    ElseIf TypeInferenceArguments(ParameterIndex) IsNot ArgumentType Then
                        Return False

                    End If
                End If

            ElseIf IsInstantiatedGeneric(ParameterType) Then

                Dim BestMatchType As Type = Nothing

                If IsInstantiatedGeneric(ArgumentType) AndAlso _
                    ArgumentType.GetGenericTypeDefinition Is ParameterType.GetGenericTypeDefinition Then
                    BestMatchType = ArgumentType
                End If

                If BestMatchType Is Nothing AndAlso DigThroughToBasesAndImplements Then
                    For Each PossibleGenericType As Type In ArgumentType.GetInterfaces
                        If IsInstantiatedGeneric(PossibleGenericType) AndAlso _
                            PossibleGenericType.GetGenericTypeDefinition Is ParameterType.GetGenericTypeDefinition Then

                            If BestMatchType Is Nothing Then
                                BestMatchType = PossibleGenericType
                            Else
                                ' Multiple generic interfaces match the parameter type
                                Return False
                            End If
                        End If
                    Next
                End If

                If BestMatchType IsNot Nothing Then
                    Dim ParameterTypeParameters As Type() = GetTypeArguments(ParameterType)
                    Dim ArgumentTypeArguments As Type() = GetTypeArguments(BestMatchType)

                    Debug.Assert(ParameterTypeParameters.Length = ArgumentTypeArguments.Length, _
                                 "inconsistent parameter counts")

                    For Index As Integer = 0 To ArgumentTypeArguments.Length - 1
                        If Not InferTypeArgumentsFromArgument( _
                                    ArgumentTypeArguments(Index), _
                                    ParameterTypeParameters(Index), _
                                    TypeInferenceArguments, _
                                    TargetProcedure, _
                                    False) Then     'Don't dig through because generics covariance is not allowed
                            Return False
                        End If
                    Next

                    Return True
                End If

                Return False

            ElseIf IsArrayType(ParameterType) Then

                If IsArrayType(ArgumentType) Then
                    If ParameterType.GetArrayRank = ArgumentType.GetArrayRank Then
                        Return _
                            InferTypeArgumentsFromArgument( _
                                GetElementType(ArgumentType), _
                                GetElementType(ParameterType), _
                                TypeInferenceArguments, _
                                TargetProcedure, _
                                DigThroughToBasesAndImplements)
                    End If
                End If

                Return False
            End If

            Return True

        End Function

        Private Shared Function CanPassToParamArray( _
            ByVal TargetProcedure As Method, _
            ByVal Argument As Object, _
            ByVal Parameter As ParameterInfo) As Boolean

            'This method generates no errors because errors are reported only on the expanded form and
            'the unexpanded form is always accompanied by the expanded form.

            Debug.Assert(IsParamArray(Parameter), "expected ParamArray parameter")

            'A Nothing argument can be passed as an unexpanded ParamArray.
            If Argument Is Nothing Then Return True

            Dim ParameterType As Type = Parameter.ParameterType
#If TELESTO Then
            Dim ArgumentType As Type = GetArgumentType(Argument)
#Else
            Dim ArgumentType As Type = GetArgumentTypeInContextOfParameterType(Argument, ParameterType)
#End If
            Dim ConversionResult As ConversionClass = ClassifyConversion(ParameterType, ArgumentType, Nothing)
            Return ConversionResult = ConversionClass.Widening OrElse ConversionResult = ConversionClass.Identity
        End Function

        Friend Shared Function CanPassToParameter( _
            ByVal TargetProcedure As Method, _
            ByVal Argument As Object, _
            ByVal Parameter As ParameterInfo, _
            ByVal IsExpandedParamArray As Boolean, _
            ByVal RejectNarrowingConversions As Boolean, _
            ByVal Errors As List(Of String), _
            ByRef RequiresNarrowingConversion As Boolean, _
            ByRef AllNarrowingIsFromObject As Boolean) As Boolean

            'A Nothing argument always matches a parameter. Also, it doesn't contribute to type inferencing.
            If Argument Is Nothing Then Return True

            Dim ParameterType As Type = Parameter.ParameterType
            Dim IsByRef As Boolean = ParameterType.IsByRef

            If IsByRef OrElse IsExpandedParamArray Then
                ParameterType = GetElementType(ParameterType)
            End If
#If TELESTO
            Dim ArgumentType As Type = GetArgumentType(Argument)
#Else
            Dim ArgumentType As Type = GetArgumentTypeInContextOfParameterType(Argument, ParameterType)
#End If
            'A Missing argument always matches an optional parameter.
            If Argument Is System.Reflection.Missing.Value Then
                If Parameter.IsOptional Then
                    Return True
                ElseIf Not IsRootObjectType(ParameterType) OrElse Not IsExpandedParamArray Then
                    'Trying to pass a Missing argument to a non-optional parameter.
                    'VSW#489299: CLR throws if that's the case, so we disallow it here.
                    If Errors IsNot Nothing Then
                        If IsExpandedParamArray Then
                            'Trying to pass a Missing argument to an expanded ParamArray.
                            ReportError(Errors, ResID.OmittedParamArrayArgument)
                        Else
                            ReportError(Errors, ResID.OmittedArgument1, Parameter.Name)
                        End If
                    End If
                    Return False
                End If
            End If

            'Check if the conversion from the argument type to the
            'parameter type can succeed.
            Dim CanCopyIn As Boolean = _
                CanConvert( _
                    ParameterType, _
                    ArgumentType, _
                    RejectNarrowingConversions, _
                    Errors, _
                    Parameter.Name, _
                    False, _
                    RequiresNarrowingConversion, _
                    AllNarrowingIsFromObject)

            If Not IsByRef OrElse Not CanCopyIn Then
                Return CanCopyIn
            End If

            'If the parameter is ByRef, check if the conversion from
            'the parameter type to the argument type can succeed.
            Return _
                CanConvert( _
                    ArgumentType, _
                    ParameterType, _
                    RejectNarrowingConversions, _
                    Errors, _
                    Parameter.Name, _
                    True, _
                    RequiresNarrowingConversion, _
                    AllNarrowingIsFromObject)

        End Function

        Friend Shared Function InferTypeArgumentsFromArgument( _
            ByVal TargetProcedure As Method, _
            ByVal Argument As Object, _
            ByVal Parameter As ParameterInfo, _
            ByVal IsExpandedParamArray As Boolean, _
            ByVal Errors As List(Of String)) As Boolean

            'A Nothing argument doesn't contribute to type inferencing.
            If Argument Is Nothing Then Return True

            Dim ParameterType As Type = Parameter.ParameterType
            Dim IsByRef As Boolean = ParameterType.IsByRef

            If IsByRef OrElse IsExpandedParamArray Then
                ParameterType = GetElementType(ParameterType)
            End If
#If TELESTO Then
            Dim ArgumentType As Type = GetArgumentType(Argument)
#Else
            Dim ArgumentType As Type = GetArgumentTypeInContextOfParameterType(Argument, ParameterType)
#End If
            Debug.Assert(TargetProcedure.IsMethod, "we shouldn't be infering type arguments for non-methods")
            If Not InferTypeArgumentsFromArgument( _
                        ArgumentType, _
                        ParameterType, _
                        TargetProcedure.TypeArguments, _
                        TargetProcedure.AsMethod, _
                        True) Then

                If Errors IsNot Nothing Then
                    ReportError(Errors, ResID.TypeInferenceFails1, Parameter.Name)
                End If
                Return False
            End If

            Return True
        End Function

        Friend Shared Function PassToParameter( _
            ByVal Argument As Object, _
            ByVal Parameter As ParameterInfo, _
            ByVal ParameterType As Type) As Object

            'This function takes an object and modifies it so it can be passed
            'as the parameter described by the ParameterInfo.  This involves casting it
            'to the parameter type and/or substituting optional values for Missing
            'arguments.

            Debug.Assert(Parameter IsNot Nothing AndAlso ParameterType IsNot Nothing)

            Dim IsByRef As Boolean = ParameterType.IsByRef
            If IsByRef Then
                ParameterType = ParameterType.GetElementType
            End If

            'An argument represented by a TypedNothing is actually a Nothing
            'reference with a type. This argument passes to the parameter as Nothing.
            If TypeOf Argument Is TypedNothing Then
                Argument = Nothing
            End If

            'A Missing argument loads the parameter's optional value.
            If Argument Is System.Reflection.Missing.Value AndAlso Parameter.IsOptional Then
                Argument = Parameter.DefaultValue
            End If

            'If the argument is a boxed ValueType and we're passing it to a
            'ByRef parameter, then we must forcefully copy it to avoid
            'aliasing since the invocation will modify the boxed ValueType
            'in place. See VS7#304212, changelist #175254.
            If IsByRef Then
#If TELESTO                
                Dim ArgumentType As Type = GetArgumentType(Argument)
#Else
                Dim ArgumentType As Type = GetArgumentTypeInContextOfParameterType(Argument, ParameterType)
#End If
                If ArgumentType IsNot Nothing AndAlso IsValueType(ArgumentType) Then
                    Argument = Conversions.ForceValueCopy(Argument, ArgumentType)
                End If
            End If

            'Peform the conversion to the parameter type and return the result.
            Return Conversions.ChangeType(Argument, ParameterType)
        End Function

        Private Shared Function FindParameterByName(ByVal Parameters As ParameterInfo(), ByVal Name As String, ByRef Index As Integer) As Boolean
            'Find the Index of the parameter in Parameters which matches Name. Return True if such a parameter is found.

            Dim ParamIndex As Integer = 0
            Do While ParamIndex < Parameters.Length
                If Operators.CompareString(Name, Parameters(ParamIndex).Name, True) = 0 Then
                    Index = ParamIndex
                    Return True
                End If
                ParamIndex += 1
            Loop
            Return False
        End Function

        Private Shared Function CreateMatchTable(ByVal Size As Integer, ByVal LastPositionalMatchIndex As Integer) As Boolean()
            'Create a table for keeping track of which parameters have been matched with
            'an argument. Used for detecting multiple matches during named argument matching,
            'and also for loading the optional values of unmatched parameters.

            Dim Result As Boolean() = New Boolean(Size - 1) {}
            For Index As Integer = 0 To LastPositionalMatchIndex
                Result(Index) = True
            Next
            Return Result
        End Function

        Friend Shared Function CanMatchArguments( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal RejectNarrowingConversions As Boolean, _
            ByVal Errors As List(Of String)) As Boolean

            Dim ReportErrors As Boolean = Errors IsNot Nothing

            TargetProcedure.ArgumentsValidated = True

            'First instantiate the generic method.  If type arguments aren't supplied, 
            'we need to infer them first.
            'In error cases, the method might already be instantiated, so need to use the
            'passed in type params only if the method has not yet been instantiated.
            'In the non-error case, the method is always uninstantiated at this time.
            '
#If DEBUG AND NOt TELESTO
            Debug.Assert(Not (Errors Is Nothing AndAlso TargetProcedure.IsMethod AndAlso IsInstantiatedGeneric(TargetProcedure.AsMethod)), _
                                "Instantiated generic method unexpected!!!")
#Else 
            Debug.Assert(Not (Errors Is Nothing AndAlso _
                              TargetProcedure.IsMethod AndAlso _
                              TargetProcedure.AsMethod.IsGenericMethod AndAlso (Not TargetProcedure.AsMethod.IsGenericMethodDefinition)), _
                                "Instantiated generic method unexpected!!!")
#End If

            If TargetProcedure.IsMethod AndAlso IsRawGeneric(TargetProcedure.AsMethod) Then
                If TypeArguments.Length = 0 Then
                    TypeArguments = New Type(TargetProcedure.TypeParameters.Length - 1) {}
                    TargetProcedure.TypeArguments = TypeArguments

                    If Not InferTypeArguments(TargetProcedure, Arguments, ArgumentNames, TypeArguments, Errors) Then
                        Return False
                    End If
                Else
                    TargetProcedure.TypeArguments = TypeArguments
                End If

                If Not InstantiateGenericMethod(TargetProcedure, TypeArguments, Errors) Then
                    Return False
                End If
            End If

            Dim Parameters As ParameterInfo() = TargetProcedure.Parameters
            Debug.Assert(Arguments.Length <= Parameters.Length OrElse _
                            (TargetProcedure.ParamArrayExpanded AndAlso TargetProcedure.ParamArrayIndex >= 0), _
                         "argument count mismatch -- this method should have been rejected already")

            Dim ArgIndex As Integer = ArgumentNames.Length
            Dim ParamIndex As Integer = 0

            'STEP 1
            'Match all positional arguments until we encounter a ParamArray or run out of positional arguments.
            Do While ArgIndex < Arguments.Length

                'The loop is finished if we encounter a ParamArray.
                If ParamIndex = TargetProcedure.ParamArrayIndex Then Exit Do

                If Not CanPassToParameter( _
                            TargetProcedure, _
                            Arguments(ArgIndex), _
                            Parameters(ParamIndex), _
                            False, _
                            RejectNarrowingConversions, _
                            Errors, _
                            TargetProcedure.RequiresNarrowingConversion, _
                            TargetProcedure.AllNarrowingIsFromObject) Then

                    'If errors are needed, keep going to catch them all.
                    If Not ReportErrors Then Return False

                End If

                ArgIndex += 1
                ParamIndex += 1
            Loop

            'STEP 2
            'Match all remaining positional arguments to the ParamArray.
            If TargetProcedure.HasParamArray Then

                Debug.Assert(ParamIndex = TargetProcedure.ParamArrayIndex, _
                             "current parameter must be param array by this point")

                If TargetProcedure.ParamArrayExpanded Then
                    'Treat the ParamArray in its expanded form. Match remaining arguments to the
                    'ParamArray's element type.

                    'Nothing passed to a ParamArray will widen to both the type of the ParamArray and the
                    'array type of the ParamArray. In that case, we explicitly disallow matching an
                    'expanded ParamArray. If one argument remains and it is Nothing, reject the match.

                    If ArgIndex = Arguments.Length - 1 AndAlso Arguments(ArgIndex) Is Nothing Then
                        'No need to generate an error for this case since Nothing will always match
                        'the associated unexpanded form.
                        Return False
                    End If

                    Do While ArgIndex < Arguments.Length

                        If Not CanPassToParameter( _
                                    TargetProcedure, _
                                    Arguments(ArgIndex), _
                                    Parameters(ParamIndex), _
                                    True, _
                                    RejectNarrowingConversions, _
                                    Errors, _
                                    TargetProcedure.RequiresNarrowingConversion, _
                                    TargetProcedure.AllNarrowingIsFromObject) Then

                            'If errors are needed, keep going to catch them all.
                            If Not ReportErrors Then Return False

                        End If

                        ArgIndex += 1
                    Loop

                Else
                    'Treat the ParamArray in its unexpanded form. Determine if the argument can
                    'be passed directly as a ParamArray.

                    Debug.Assert(Arguments.Length - ArgIndex <= 1, _
                                 "must have zero or one arg left to match the unexpanded paramarray")  'Candidate collection guarantees this.

                    'Need one argument left over for the unexpanded form to be applicable.
                    If Arguments.Length - ArgIndex <> 1 Then
                        'No need to generate an error for this case because the error
                        'reporting will be done on the expanded form. All we need to do is
                        'disqualify the unexpanded form.
                        Return False
                    End If

                    If Not CanPassToParamArray( _
                                TargetProcedure, _
                                Arguments(ArgIndex), _
                                Parameters(ParamIndex)) Then

                        ' VSW 259007: We do need to report errors when only the
                        ' unexpanded form is being considered.
                        If ReportErrors Then
#If TELESTO Then
                            ReportError( _
                                Errors, _
                                ResID.ArgumentMismatch3, _
                                Parameters(ParamIndex).Name, _
                                GetArgumentType(Arguments(ArgIndex)), _
                                Parameters(ParamIndex).ParameterType)
#Else
                            ReportError( _
                                Errors, _
                                ResID.ArgumentMismatch3, _
                                Parameters(ParamIndex).Name, _
                                GetArgumentTypeInContextOfParameterType(Arguments(ArgIndex),
                                                                        Parameters(ParamIndex).ParameterType), _
                                Parameters(ParamIndex).ParameterType)
#End If
                        End If

                        Return False
                    End If

                End If

                'Matching the ParamArray consumes this parameter. Increment the parameter index.
                ParamIndex += 1
            End If

            'If needed, create the table which keeps track of matched Parameters.
            'Initialize it using the positional matches we've found thus far.
            'This table is needed if we potentially have unmatched Optional parameters.
            'This can happen when named arguments exist or when the number of positional
            'arguments is less than the number of parameters.
            Dim MatchedParameters As Boolean() = Nothing

            If ArgumentNames.Length > 0 OrElse ParamIndex < Parameters.Length Then
                MatchedParameters = CreateMatchTable(Parameters.Length, ParamIndex - 1)
            End If

            'STEP 3
            'Match all named arguments.
            If ArgumentNames.Length > 0 Then

                Debug.Assert(Parameters.Length > 0, "expected some parameters here")  'Candidate collection guarantees this.

                'The named argument mapping table contains indicies into the
                'parameters array to describe the association between arguments
                'and parameters.
                '
                'Given an array of arguments and an array of argument names, the
                'index n into each of these arrays represents the nth named argument
                'and its assocated name. If argument n matches the name of the
                'parameter at index m in the array of parameters, then the named
                'argument mapping table will contain the value m at index n.

                Dim NamedArgumentMapping As Integer() = New Integer(ArgumentNames.Length - 1) {}

                ArgIndex = 0
                Do While ArgIndex < ArgumentNames.Length

                    If Not FindParameterByName(Parameters, ArgumentNames(ArgIndex), ParamIndex) Then
                        'This named argument does not match the name of any parameter.
                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False
                        ReportError(Errors, ResID.NamedParamNotFound2, ArgumentNames(ArgIndex), TargetProcedure)
                        GoTo skipargument
                    End If

                    If ParamIndex = TargetProcedure.ParamArrayIndex Then
                        'This named argument matches a ParamArray parameter.
                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False
                        ReportError(Errors, ResID.NamedParamArrayArgument1, ArgumentNames(ArgIndex))
                        GoTo skipargument
                    End If

                    If MatchedParameters(ParamIndex) Then
                        'This named argument matches a parameter which has already been specified.
                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False
                        ReportError(Errors, ResID.NamedArgUsedTwice2, ArgumentNames(ArgIndex), TargetProcedure)
                        GoTo skipargument
                    End If

                    If Not CanPassToParameter( _
                                TargetProcedure, _
                                Arguments(ArgIndex), _
                                Parameters(ParamIndex), _
                                False, _
                                RejectNarrowingConversions, _
                                Errors, _
                                TargetProcedure.RequiresNarrowingConversion, _
                                TargetProcedure.AllNarrowingIsFromObject) Then

                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False

                    End If

                    MatchedParameters(ParamIndex) = True
                    NamedArgumentMapping(ArgIndex) = ParamIndex
skipargument:
                    ArgIndex += 1
                Loop

                'Store this away for use when/if we invoke this method.
                TargetProcedure.NamedArgumentMapping = NamedArgumentMapping
            End If

            'All remaining unmatched parameters must be Optional.
            If MatchedParameters IsNot Nothing Then
                For Index As Integer = 0 To MatchedParameters.Length - 1
                    If MatchedParameters(Index) = False AndAlso Not Parameters(Index).IsOptional Then
                        'This parameter is not optional.
                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False
                        ReportError(Errors, ResID.OmittedArgument1, Parameters(Index).Name)
                    End If
                Next
            End If

            'If errors were generated, the arguments failed to match the target procedure.
            If Errors IsNot Nothing AndAlso Errors.Count > 0 Then
                Return False
            End If

            Return True

        End Function

        Private Shared Function InstantiateGenericMethod( _
            ByVal TargetProcedure As Method, _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

            'Verify that all type arguments have been supplied.
            Debug.Assert(TypeArguments.Length = TargetProcedure.TypeParameters.Length, "expected length match")

            Dim ReportErrors As Boolean = Errors IsNot Nothing

            For TypeArgumentIndex As Integer = 0 To TypeArguments.Length - 1

                If TypeArguments(TypeArgumentIndex) Is Nothing Then
                    If Not ReportErrors Then Return False
                    ReportError( _
                            Errors, _
                            ResID.UnboundTypeParam1, _
                            TargetProcedure.TypeParameters(TypeArgumentIndex).Name)
                End If

            Next

            If Errors Is Nothing OrElse Errors.Count = 0 Then
                'Create the instantiated form of the generic method using the type arguments
                'inferred during argument matching.
                If Not TargetProcedure.BindGenericArguments Then
                    If Not ReportErrors Then Return False
                    ReportError(Errors, ResID.FailedTypeArgumentBinding)
                End If
            End If

            'If errors were generated, the instantiation failed.
            If Errors IsNot Nothing AndAlso Errors.Count > 0 Then
                Return False
            End If

            Return True
        End Function

        'may not want Method as TargetProcedure - may instead want to pass the required information in separately.
        'this means that for the simple case of only one method we do not need to allocate a Method object.
        Friend Shared Sub MatchArguments( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal MatchedArguments As Object())

            Dim Parameters As ParameterInfo() = TargetProcedure.Parameters

            Debug.Assert(TargetProcedure.ArgumentsValidated, _
                         "expected validation of arguments to be made before matching")
            Debug.Assert(MatchedArguments.Length = Parameters.Length OrElse _
                         MatchedArguments.Length = Parameters.Length + 1, _
                         "size of matched arguments array must equal number of parameters")
            Debug.Assert(Arguments.Length <= Parameters.Length OrElse _
                            (TargetProcedure.ParamArrayExpanded AndAlso TargetProcedure.ParamArrayIndex >= 0), _
                         "argument count mismatch -- this method should have been rejected already")

            Dim NamedArgumentMapping As Integer() = TargetProcedure.NamedArgumentMapping

            Dim ArgIndex As Integer = 0
            If NamedArgumentMapping IsNot Nothing Then ArgIndex = NamedArgumentMapping.Length
            Dim ParamIndex As Integer = 0

            'STEP 1
            'Match all positional arguments until we encounter a ParamArray or run out of positional arguments.
            Do While ArgIndex < Arguments.Length

                'The loop is finished if we encounter a ParamArray.
                If ParamIndex = TargetProcedure.ParamArrayIndex Then Exit Do

                MatchedArguments(ParamIndex) = _
                    PassToParameter(Arguments(ArgIndex), Parameters(ParamIndex), Parameters(ParamIndex).ParameterType)

                ArgIndex += 1
                ParamIndex += 1
            Loop

            'STEP 2
            'Match all remaining positional arguments to the ParamArray.
            If TargetProcedure.HasParamArray Then
                Debug.Assert(ParamIndex = TargetProcedure.ParamArrayIndex, _
                             "current parameter must be param array by this point")

                If TargetProcedure.ParamArrayExpanded Then
                    'Treat the ParamArray in its expanded form. Pass the remaining arguments into
                    'the ParamArray.

                    Dim RemainingArgumentCount As Integer = Arguments.Length - ArgIndex
                    Dim ParamArrayParameter As ParameterInfo = Parameters(ParamIndex)
                    Dim ParamArrayElementType As System.Type = ParamArrayParameter.ParameterType.GetElementType

                    Dim ParamArrayArgument As System.Array = _
                        System.Array.CreateInstance(ParamArrayElementType, RemainingArgumentCount)

                    Dim ParamArrayIndex As Integer = 0
                    Do While ArgIndex < Arguments.Length

                        ParamArrayArgument.SetValue( _
                            PassToParameter(Arguments(ArgIndex), ParamArrayParameter, ParamArrayElementType), _
                            ParamArrayIndex)

                        ArgIndex += 1
                        ParamArrayIndex += 1
                    Loop

                    MatchedArguments(ParamIndex) = ParamArrayArgument

                Else
                    Debug.Assert(Arguments.Length - ArgIndex = 1, _
                                 "must have one arg left to match the unexpanded paramarray")

                    'Treat the ParamArray in its unexpanded form. Pass the one remaining argument
                    'directly as a ParamArray.
                    MatchedArguments(ParamIndex) = _
                        PassToParameter(Arguments(ArgIndex), Parameters(ParamIndex), Parameters(ParamIndex).ParameterType)
                End If

                'Matching the ParamArray consumes this parameter. Increment the parameter index.
                ParamIndex += 1
            End If

            'If needed, create the table which keeps track of matched Parameters.
            'Initialize it using the positional matches we've found thus far.
            'This table is needed if we potentially have unmatched Optional parameters.
            'This can happen when named arguments exist or when the number of positional
            'arguments is less than the number of parameters.
            Dim MatchedParameters As Boolean() = Nothing

            If NamedArgumentMapping IsNot Nothing OrElse ParamIndex < Parameters.Length Then
                MatchedParameters = CreateMatchTable(Parameters.Length, ParamIndex - 1)
            End If

            'STEP 3
            'Match all named arguments.
            If NamedArgumentMapping IsNot Nothing Then

                Debug.Assert(Parameters.Length > 0, "expected some parameters here")  'Candidate collection guarantees this.

                'The named argument mapping table contains indicies into the
                'parameters array to describe the association between arguments
                'and parameters.
                '
                'Given an array of arguments and an array of argument names, the
                'index n into each of these arrays represents the nth named argument
                'and its assocated name. If argument n matches the name of the
                'parameter at index m in the array of parameters, then the named
                'argument mapping table will contain the value m at index n.

                ArgIndex = 0
                Do While ArgIndex < NamedArgumentMapping.Length
                    ParamIndex = NamedArgumentMapping(ArgIndex)

                    MatchedArguments(ParamIndex) = _
                        PassToParameter(Arguments(ArgIndex), Parameters(ParamIndex), Parameters(ParamIndex).ParameterType)

                    Debug.Assert(Not MatchedParameters(ParamIndex), "named argument match collision")
                    MatchedParameters(ParamIndex) = True
                    ArgIndex += 1
                Loop

            End If

            'If all has gone well, by this point any unmatched parameters are Optional.
            'Fill in unmatched parameters with their optional values.
            If MatchedParameters IsNot Nothing Then
                For Index As Integer = 0 To MatchedParameters.Length - 1
                    If MatchedParameters(Index) = False Then
                        Debug.Assert(Parameters(Index).IsOptional, _
                                     "unmatched, non-optional parameter. How did we get this far?")
                        MatchedArguments(Index) = _
                            PassToParameter(System.Reflection.Missing.Value, Parameters(Index), Parameters(Index).ParameterType)
                    End If
                Next
            End If

            Return
        End Sub

        Private Shared Function InferTypeArguments( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

            Dim ReportErrors As Boolean = Errors IsNot Nothing

            Dim Parameters As ParameterInfo() = TargetProcedure.RawParameters

            Debug.Assert(Arguments.Length <= Parameters.Length OrElse _
                            (TargetProcedure.ParamArrayExpanded AndAlso TargetProcedure.ParamArrayIndex >= 0), _
                         "argument count mismatch -- this method should have been rejected already")

            Dim ArgIndex As Integer = ArgumentNames.Length
            Dim ParamIndex As Integer = 0

            'STEP 1
            'Infer from all positional arguments until we encounter a ParamArray or run out of positional arguments.
            Do While ArgIndex < Arguments.Length

                'The loop is finished if we encounter a ParamArray.
                If ParamIndex = TargetProcedure.ParamArrayIndex Then Exit Do

                If Not InferTypeArgumentsFromArgument( _
                            TargetProcedure, _
                            Arguments(ArgIndex), _
                            Parameters(ParamIndex), _
                            False, _
                            Errors) Then

                    'If errors are needed, keep going to catch them all.
                    If Not ReportErrors Then Return False
                End If

                ArgIndex += 1
                ParamIndex += 1
            Loop

            'STEP 2
            'Infer from all remaining positional arguments matching a ParamArray.
            If TargetProcedure.HasParamArray Then

                Debug.Assert(ParamIndex = TargetProcedure.ParamArrayIndex, _
                             "current parameter must be param array by this point")

                If TargetProcedure.ParamArrayExpanded Then
                    'Treat the ParamArray in its expanded form. Infer the element type from the remaining arguments.
                    Do While ArgIndex < Arguments.Length

                        If Not InferTypeArgumentsFromArgument( _
                                    TargetProcedure, _
                                    Arguments(ArgIndex), _
                                    Parameters(ParamIndex), _
                                    True, _
                                    Errors) Then

                            'If errors are needed, keep going to catch them all.
                            If Not ReportErrors Then Return False

                        End If

                        ArgIndex += 1
                    Loop

                Else
                    'Treat the ParamArray in its unexpanded form. Infer the ParamArray type from the argument.

                    Debug.Assert(Arguments.Length - ArgIndex <= 1, _
                                 "must have zero or one arg left to match the unexpanded paramarray")  'Candidate collection guarantees this.

                    If Arguments.Length - ArgIndex <> 1 Then
                        'Type inferencing not possible here.
                        Return True
                    End If

                    If Not InferTypeArgumentsFromArgument( _
                                TargetProcedure, _
                                Arguments(ArgIndex), _
                                Parameters(ParamIndex), _
                                False, _
                                Errors) Then
                        Return False
                    End If

                End If

                'Matching the ParamArray consumes this parameter. Increment the parameter index.
                ParamIndex += 1
            End If

            'STEP 3
            'Infer from named arguments.
            If ArgumentNames.Length > 0 Then

                Debug.Assert(Parameters.Length > 0, "expected some parameters here")  'Candidate collection guarantees this.

                ArgIndex = 0
                Do While ArgIndex < ArgumentNames.Length

                    If Not FindParameterByName(Parameters, ArgumentNames(ArgIndex), ParamIndex) Then
                        GoTo skipargument
                    End If

                    If ParamIndex = TargetProcedure.ParamArrayIndex Then
                        GoTo skipargument
                    End If

                    If Not InferTypeArgumentsFromArgument( _
                                TargetProcedure, _
                                Arguments(ArgIndex), _
                                Parameters(ParamIndex), _
                                False, _
                                Errors) Then

                        'If errors are needed, keep going to catch them all.
                        If Not ReportErrors Then Return False

                    End If
skipargument:
                    ArgIndex += 1
                Loop

            End If

            'If errors were generated, inference of type arguments failed.
            If Errors IsNot Nothing AndAlso Errors.Count > 0 Then
                Return False
            End If

            Return True
        End Function

        Friend Shared Sub ReorderArgumentArray( _
            ByVal TargetProcedure As Method, _
            ByVal ParameterResults As Object(), _
            ByVal Arguments As Object(), _
            ByVal CopyBack As Boolean(), _
            ByVal LookupFlags As BindingFlags)

            'No need to copy back if there are no valid targets .
            'The copy back array will be be Nothing if the compiler determined that all
            'arguments are Rvalues.
            If CopyBack Is Nothing Then
                Return
            End If

            'Initialize the copy back array to all ByVal.
            For Index As Integer = 0 To CopyBack.Length - 1
                CopyBack(Index) = False
            Next

            'No need to copy back if there are no byref parameters. Properties can't have
            'ByRef arguments, so skip these as well.
            'CONSIDER: how to know when TargetProcedure is a Get property accessor?
            If HasFlag(LookupFlags, BindingFlags.SetProperty) OrElse _
               Not TargetProcedure.HasByRefParameter Then
                Return
            End If

            Debug.Assert(CopyBack.Length = Arguments.Length, "array sizes must match")
            Debug.Assert(ParameterResults.Length = TargetProcedure.Parameters.Length, "parameter arrays must match")

            Dim Parameters As ParameterInfo() = TargetProcedure.Parameters
            Dim NamedArgumentMapping As Integer() = TargetProcedure.NamedArgumentMapping

            Dim ArgIndex As Integer = 0
            If NamedArgumentMapping IsNot Nothing Then ArgIndex = NamedArgumentMapping.Length
            Dim ParamIndex As Integer = 0

            'STEP 1
            'Copy back all positional parameters until we encounter a ParamArray or run out of positional arguments.
            Do While ArgIndex < Arguments.Length

                'The loop is finished if we encounter a ParamArray.
                If ParamIndex = TargetProcedure.ParamArrayIndex Then Exit Do

                If Parameters(ParamIndex).ParameterType.IsByRef Then
                    Arguments(ArgIndex) = ParameterResults(ParamIndex)
                    CopyBack(ArgIndex) = True
                End If

                ArgIndex += 1
                ParamIndex += 1
            Loop

            'STEP 2
            'No need to copy back from the ParamArray because they can't be ByRef. Skip it.

            'STEP 3
            'Copy back all named arguments.
            If NamedArgumentMapping IsNot Nothing Then
                ArgIndex = 0
                Do While ArgIndex < NamedArgumentMapping.Length
                    ParamIndex = NamedArgumentMapping(ArgIndex)

                    If Parameters(ParamIndex).ParameterType.IsByRef Then
                        Arguments(ArgIndex) = ParameterResults(ParamIndex)
                        CopyBack(ArgIndex) = True
                    End If

                    ArgIndex += 1
                Loop
            End If

            Return
        End Sub

        Private Shared Function RejectUncallableProcedures( _
            ByVal Candidates As List(Of Method), _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByRef CandidateCount As Integer, _
            ByRef SomeCandidatesAreGeneric As Boolean) As Method

            Dim BestCandidate As Method = Nothing

            For Index As Integer = 0 To Candidates.Count - 1

                Dim CandidateProcedure As Method = Candidates(Index)

                If Not CandidateProcedure.ArgumentMatchingDone Then

                    RejectUncallableProcedure( _
                        CandidateProcedure, _
                        Arguments, _
                        ArgumentNames, _
                        TypeArguments)
                End If

                If CandidateProcedure.NotCallable Then
                    CandidateCount -= 1
                Else
                    BestCandidate = CandidateProcedure

                    If CandidateProcedure.IsGeneric OrElse IsGeneric(CandidateProcedure.DeclaringType) Then

                        SomeCandidatesAreGeneric = True

                        'ElseIf Not RequiresSomeConversion Then
                        '    'This candidate is an exact match which means the audition is over,
                        '    'but only if the candidate is not generic. (A less-generic method 
                        '    'might have the same signature.)
                        '
                        '    'CONSIDER: This shortcut is possible only if overridden methods are
                        '    'eliminated from the set of callable methods, or if the call is known
                        '    'to be virtual.
                        '    CandidateCount = 1
                        '    Exit For

                    End If

                End If

            Next


#If BINDING_LOG Then
            Console.WriteLine("== REJECT UNCALLABLE ==")
            For Each item As Method In Candidates
                If item Is Nothing Then
                    Console.WriteLine("dead ** didn't expect this here.")
                Else
                    Console.WriteLine(item.DumpContents)
                End If
            Next
#End If
            Return BestCandidate

        End Function

        Private Shared Sub RejectUncallableProcedure( _
            ByVal Candidate As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type())

            Debug.Assert(Candidate.ArgumentMatchingDone = False, "Argument matching being done multiple times!!!")

            If Not CanMatchArguments( _
                        Candidate, _
                        Arguments, _
                        ArgumentNames, _
                        TypeArguments, _
                        False, _
                        Nothing) Then

                Candidate.NotCallable = True
            End If

            Candidate.ArgumentMatchingDone = True

        End Sub

' Type.IsEquivalentTo is not supported in .Net 4.0
#If NOT TELESTO Then
        'For NoPIA, if Argument is of type __ComObject, then treat it as its corresponding PIA type
        '
        Private Shared Function GetArgumentTypeInContextOfParameterType(
            ByVal Argument As Object,
            ByVal ParameterType As Type) As Type

            Dim ArgumentType As Type = GetArgumentType(Argument)

            If ArgumentType Is Nothing OrElse 
               ParameterType Is Nothing Then Return ArgumentType

            'Check if Argument's runtime type is equivalent to the PIA type.  If it is then we
            'want to use the PIA type instead of __ComObject
            If (ParameterType.IsImport AndAlso
                ParameterType.IsInterface AndAlso
                ParameterType.IsInstanceOfType(Argument)
               ) OrElse
                     IsEquivalentType(ArgumentType, ParameterType) Then

                ArgumentType = ParameterType

            End If

            Return ArgumentType
        End Function
#End If
        Private Shared Function GetArgumentType(ByVal Argument As Object) As Type
            'A Nothing object has no type.
            If Argument Is Nothing Then Return Nothing
            'A typed Nothing object stores the type that Nothing should be considered as.
            Dim TypedNothingArgument As TypedNothing = TryCast(Argument, TypedNothing)

            If TypedNothingArgument IsNot Nothing Then Return TypedNothingArgument.Type
            'Otherwise, just return the type of the object.
            Return Argument.GetType
        End Function

        Private Shared Function MoreSpecificProcedure( _
            ByVal Left As Method, _
            ByVal Right As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal CompareGenericity As ComparisonType, _
            Optional ByRef BothLose As Boolean = False, _
            Optional ByVal ContinueWhenBothLose As Boolean = False) As Method

            BothLose = False
            Dim LeftWinsAtLeastOnce As Boolean = False
            Dim RightWinsAtLeastOnce As Boolean = False

            'Compare the parameters that match the supplied positional arguments.

            Dim LeftMethod As MethodBase
            Dim RightMethod As MethodBase
            If Left.IsMethod Then LeftMethod = Left.AsMethod Else LeftMethod = Nothing
            If Right.IsMethod Then RightMethod = Right.AsMethod Else RightMethod = Nothing

            Dim LeftParamIndex As Integer = 0
            Dim RightParamIndex As Integer = 0

            Dim ArgIndex As Integer = ArgumentNames.Length
            Do While ArgIndex < Arguments.Length

                'Compare parameters only for supplied arguments.
                'UNDONE
                'if ArgumentSupplied then
                Dim ArgumentType As Type = GetArgumentType(Arguments(ArgIndex))

                Select Case CompareGenericity
                    Case ComparisonType.GenericSpecificityBasedOnMethodGenericParams
                        ' Compare GenericSpecificity
                        CompareGenericityBasedOnMethodGenericParams( _
                            Left.Parameters(LeftParamIndex), _
                            Left.RawParameters(LeftParamIndex), _
                            Left, _
                            Left.ParamArrayExpanded, _
                            Right.Parameters(RightParamIndex), _
                            Right.RawParameters(RightParamIndex), _
                            Right, _
                            Right.ParamArrayExpanded, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            BothLose)

                    Case ComparisonType.GenericSpecificityBasedOnTypeGenericParams
                        ' Compare GenericSpecificity
                        CompareGenericityBasedOnTypeGenericParams( _
                            Left.Parameters(LeftParamIndex), _
                            Left.RawParametersFromType(LeftParamIndex), _
                            Left, _
                            Left.ParamArrayExpanded, _
                            Right.Parameters(RightParamIndex), _
                            Right.RawParametersFromType(RightParamIndex), _
                            Right, _
                            Right.ParamArrayExpanded, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            BothLose)

                    Case ComparisonType.ParameterSpecificty
                        ' Compare ParameterSpecificity
                        CompareParameterSpecificity( _
                                ArgumentType, _
                                Left.Parameters(LeftParamIndex), _
                                LeftMethod, _
                                Left.ParamArrayExpanded, _
                                Right.Parameters(RightParamIndex), _
                                RightMethod, _
                                Right.ParamArrayExpanded, _
                                LeftWinsAtLeastOnce, _
                                RightWinsAtLeastOnce, _
                                BothLose)

                    Case Else
#If TELESTO Then
                        Debug.Assert(False, "Unexpected comparison type!!!") ' Silverlight CLR does not have Debug.Fail.
#Else
                        Debug.Fail("Unexpected comparison type!!!")
#End If
                End Select

                If (BothLose AndAlso (Not ContinueWhenBothLose)) OrElse _
                   (LeftWinsAtLeastOnce AndAlso RightWinsAtLeastOnce) Then
                    Return Nothing
                End If

                'UNDONE
                'end if

                If LeftParamIndex <> Left.ParamArrayIndex Then LeftParamIndex += 1
                If RightParamIndex <> Right.ParamArrayIndex Then RightParamIndex += 1
                ArgIndex += 1
            Loop

            ArgIndex = 0
            Do While ArgIndex < ArgumentNames.Length

                Dim LeftParameterFound As Boolean = FindParameterByName(Left.Parameters, ArgumentNames(ArgIndex), LeftParamIndex)
                Dim RightParameterFound As Boolean = FindParameterByName(Right.Parameters, ArgumentNames(ArgIndex), RightParamIndex)

                If Not LeftParameterFound OrElse Not RightParameterFound Then
                    Throw New InternalErrorException()
                End If

                Dim ArgumentType As Type = GetArgumentType(Arguments(ArgIndex))

                Select Case CompareGenericity
                    Case ComparisonType.GenericSpecificityBasedOnMethodGenericParams
                        ' Compare GenericSpecificity
                        CompareGenericityBasedOnMethodGenericParams( _
                            Left.Parameters(LeftParamIndex), _
                            Left.RawParameters(LeftParamIndex), _
                            Left, _
                            True, _
                            Right.Parameters(RightParamIndex), _
                            Right.RawParameters(RightParamIndex), _
                            Right, _
                            True, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            BothLose)

                    Case ComparisonType.GenericSpecificityBasedOnTypeGenericParams
                        ' Compare GenericSpecificity
                        CompareGenericityBasedOnTypeGenericParams( _
                            Left.Parameters(LeftParamIndex), _
                            Left.RawParameters(LeftParamIndex), _
                            Left, _
                            True, _
                            Right.Parameters(RightParamIndex), _
                            Right.RawParameters(RightParamIndex), _
                            Right, _
                            True, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            BothLose)

                    Case ComparisonType.ParameterSpecificty
                        ' Compare ParameterSpecificity
                        CompareParameterSpecificity( _
                            ArgumentType, _
                            Left.Parameters(LeftParamIndex), _
                            LeftMethod, _
                            True, _
                            Right.Parameters(RightParamIndex), _
                            RightMethod, _
                            True, _
                            LeftWinsAtLeastOnce, _
                            RightWinsAtLeastOnce, _
                            BothLose)
                End Select

                If (BothLose AndAlso (Not ContinueWhenBothLose)) OrElse _
                   (LeftWinsAtLeastOnce AndAlso RightWinsAtLeastOnce) Then
                    Return Nothing
                End If

                ArgIndex += 1
            Loop

            Debug.Assert(Not (LeftWinsAtLeastOnce AndAlso RightWinsAtLeastOnce), _
                         "Most specific method logic is confused.")

            If LeftWinsAtLeastOnce Then Return Left
            If RightWinsAtLeastOnce Then Return Right

            Return Nothing
        End Function

        Private Shared Function MostSpecificProcedure( _
            ByVal Candidates As List(Of Method), _
            ByRef CandidateCount As Integer, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String()) As Method


            For Each CurrentCandidate As Method In Candidates

                If CurrentCandidate.NotCallable OrElse CurrentCandidate.RequiresNarrowingConversion Then
                    Continue For
                End If

                Dim CurrentCandidateIsBest As Boolean = True

                For Each Contender As Method In Candidates

                    If Contender.NotCallable OrElse _
                       Contender.RequiresNarrowingConversion OrElse _
                       (Contender = CurrentCandidate AndAlso _
                            Contender.ParamArrayExpanded = CurrentCandidate.ParamArrayExpanded) Then

                        Continue For
                    End If

                    Dim BestOfTheTwo As Method = _
                        MoreSpecificProcedure( _
                            CurrentCandidate, _
                            Contender, _
                            Arguments, _
                            ArgumentNames, _
                            ComparisonType.ParameterSpecificty, _
                            ContinueWhenBothLose:=True)        'Bug VSWhidbey 501632

                    If BestOfTheTwo Is CurrentCandidate Then
                        If Not Contender.LessSpecific Then
                            Contender.LessSpecific = True
                            CandidateCount -= 1
                        End If
                    Else
                        'The current candidate can't be the most specific.
                        CurrentCandidateIsBest = False

                        If BestOfTheTwo Is Contender AndAlso Not CurrentCandidate.LessSpecific Then
                            CurrentCandidate.LessSpecific = True
                            CandidateCount -= 1
                        End If

                        ' Can't exit early because of VSW 211832
                    End If

                Next

                If CurrentCandidateIsBest Then
                    Debug.Assert(CandidateCount = 1, "Surprising overload candidate remains.")
                    Return CurrentCandidate
                End If

            Next

            Return Nothing
        End Function

        Private Shared Function RemoveRedundantGenericProcedures( _
            ByVal Candidates As List(Of Method), _
            ByRef CandidateCount As Integer, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String()) As Method


            For LeftIndex As Integer = 0 To Candidates.Count - 1
                Dim Left As Method = Candidates(LeftIndex)

                If Not Left.NotCallable Then

                    For RightIndex As Integer = LeftIndex + 1 To Candidates.Count - 1
                        Dim Right As Method = Candidates(RightIndex)

                        If Not Right.NotCallable AndAlso _
                           Left.RequiresNarrowingConversion = Right.RequiresNarrowingConversion Then

                            Dim LeastGeneric As Method = Nothing
                            Dim SignatureMismatch As Boolean = False

                            ' Least generic based on generic method's type parameters

                            If Left.IsGeneric() OrElse Right.IsGeneric() Then

                                LeastGeneric = _
                                    MoreSpecificProcedure( _
                                        Left, _
                                        Right, _
                                        Arguments, _
                                        ArgumentNames, _
                                        ComparisonType.GenericSpecificityBasedOnMethodGenericParams, _
                                        SignatureMismatch)

                                If LeastGeneric IsNot Nothing Then
                                    CandidateCount -= 1
                                    If CandidateCount = 1 Then
                                        Return LeastGeneric
                                    End If
                                    If LeastGeneric Is Left Then
                                        Right.NotCallable = True
                                    Else
                                        Left.NotCallable = True
                                        Exit For
                                    End If
                                End If
                            End If


                            ' Least generic based on method's generic parent's type parameters

                            If Not SignatureMismatch AndAlso _
                               LeastGeneric Is Nothing AndAlso _
                               (IsGeneric(Left.DeclaringType) OrElse IsGeneric(Right.DeclaringType)) Then

                                LeastGeneric = _
                                    MoreSpecificProcedure( _
                                        Left, _
                                        Right, _
                                        Arguments, _
                                        ArgumentNames, _
                                        ComparisonType.GenericSpecificityBasedOnTypeGenericParams, _
                                        SignatureMismatch)

                                If LeastGeneric IsNot Nothing Then
                                    CandidateCount -= 1
                                    If CandidateCount = 1 Then
                                        Return LeastGeneric
                                    End If
                                    If LeastGeneric Is Left Then
                                        Right.NotCallable = True
                                    Else
                                        Left.NotCallable = True
                                        Exit For
                                    End If
                                End If
                            End If
                        End If

                    Next

                End If
            Next

            Return Nothing
        End Function


        Private Shared Sub ReportError( _
            ByVal Errors As List(Of String), _
            ByVal ResourceID As String, _
            ByVal Substitution1 As String, _
            ByVal Substitution2 As Type, _
            ByVal Substitution3 As Type)

            Debug.Assert(Errors IsNot Nothing, "expected error table")
            Errors.Add( _
                GetResourceString( _
                    ResourceID, _
                    Substitution1, _
                    VBFriendlyName(Substitution2), _
                    VBFriendlyName(Substitution3)))
        End Sub

        Private Shared Sub ReportError( _
            ByVal Errors As List(Of String), _
            ByVal ResourceID As String, _
            ByVal Substitution1 As String, _
            ByVal Substitution2 As Method)

            Debug.Assert(Errors IsNot Nothing, "expected error table")
            Errors.Add( _
                GetResourceString( _
                    ResourceID, _
                    Substitution1, _
                    Substitution2.ToString))
        End Sub

        Private Shared Sub ReportError( _
            ByVal Errors As List(Of String), _
            ByVal ResourceID As String, _
            ByVal Substitution1 As String)

            Debug.Assert(Errors IsNot Nothing, "expected error table")
            Errors.Add( _
                GetResourceString( _
                    ResourceID, _
                    Substitution1))
        End Sub

        Private Shared Sub ReportError(ByVal Errors As List(Of String), ByVal ResourceID As String)

            Debug.Assert(Errors IsNot Nothing, "expected error table")
            Errors.Add( _
                GetResourceString(ResourceID))
        End Sub

        Private Delegate Function ArgumentDetector( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

        Private Delegate Function CandidateProperty(ByVal Candidate As Method) As Boolean

        Private Shared Function ReportOverloadResolutionFailure( _
            ByVal OverloadedProcedureName As String, _
            ByVal Candidates As List(Of Method), _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal ErrorID As String, _
            ByVal Failure As ResolutionFailure, _
            ByVal Detector As ArgumentDetector, _
            ByVal CandidateFilter As CandidateProperty) As Exception

            Dim ErrorMessage As StringBuilder = New StringBuilder
            Dim Errors As New List(Of String)
            Dim CandidateReportCount As Integer = 0

            For Index As Integer = 0 To Candidates.Count - 1

                Dim CandidateProcedure As Method = Candidates(Index)

                If CandidateFilter(CandidateProcedure) Then

                    If CandidateProcedure.HasParamArray Then
                        'We may have two versions of paramarray methods in the list. So skip the first
                        'one (the unexpanded one). However, we don't want to skip the unexpanded form
                        'if the expanded form will fail the filter.
                        Dim IndexAhead As Integer = Index + 1
                        While IndexAhead < Candidates.Count
                            If CandidateFilter(Candidates(IndexAhead)) AndAlso _
                               Candidates(IndexAhead) = CandidateProcedure Then
                                Continue For
                            End If
                            IndexAhead += 1
                        End While
                    End If

                    CandidateReportCount += 1

                    Errors.Clear()
                    Dim Result As Boolean = _
                        Detector(CandidateProcedure, Arguments, ArgumentNames, TypeArguments, Errors)
                    Debug.Assert(Result = False AndAlso Errors.Count > 0, "expected this candidate to fail")

                    ErrorMessage.Append(vbCrLf & "    '")
                    ErrorMessage.Append(CandidateProcedure.ToString)
                    ErrorMessage.Append("':")
                    For Each ErrorString As String In Errors
                        ErrorMessage.Append(vbCrLf & "        ")
                        ErrorMessage.Append(ErrorString)
                    Next
                End If

            Next

            Debug.Assert(CandidateReportCount > 0, "expected at least one candidate")

            Dim Message As String = GetResourceString(ErrorID, OverloadedProcedureName, ErrorMessage.ToString)
            If CandidateReportCount = 1 Then
                'ParamArrays may cause only one candidate to get reported. In this case, reporting an
                'ambiguity is misleading.
                'CONSIDER  3/1/2004: Using the same error message for the single-candidate case
                'is also misleading, but the benefit is not high enough for constructing a better message.
                'CONSIDER  2/26/2004: InvalidCastException is thrown only for back compat.  It would
                'be nice if the latebinder had its own set of exceptions to throw.
                Return New InvalidCastException(Message)
            Else
                Return New AmbiguousMatchException(Message)
            End If
        End Function

        Private Shared Function DetectArgumentErrors( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

            Return _
                CanMatchArguments( _
                    TargetProcedure, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    False, _
                    Errors)
        End Function

        Private Shared Function CandidateIsNotCallable(ByVal Candidate As Method) As Boolean
            Return Candidate.NotCallable
        End Function

        Private Shared Function ReportUncallableProcedures( _
            ByVal OverloadedProcedureName As String, _
            ByVal Candidates As List(Of Method), _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Failure As ResolutionFailure) As Exception

            Return _
                ReportOverloadResolutionFailure( _
                    OverloadedProcedureName, _
                    Candidates, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    ResID.NoCallableOverloadCandidates2, _
                    Failure, _
                    AddressOf DetectArgumentErrors, _
                    AddressOf CandidateIsNotCallable)
        End Function

        Private Shared Function DetectArgumentNarrowing( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

            Return _
                CanMatchArguments( _
                    TargetProcedure, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    True, _
                    Errors)
        End Function

        Private Shared Function CandidateIsNarrowing(ByVal Candidate As Method) As Boolean
            Return Not Candidate.NotCallable AndAlso Candidate.RequiresNarrowingConversion
        End Function

        Private Shared Function ReportNarrowingProcedures( _
            ByVal OverloadedProcedureName As String, _
            ByVal Candidates As List(Of Method), _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Failure As ResolutionFailure) As Exception

            Return _
                ReportOverloadResolutionFailure( _
                    OverloadedProcedureName, _
                    Candidates, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    ResID.NoNonNarrowingOverloadCandidates2, _
                    Failure, _
                    AddressOf DetectArgumentNarrowing, _
                    AddressOf CandidateIsNarrowing)
        End Function

        Private Shared Function DetectUnspecificity( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal ArgumentNames As String(), _
            ByVal TypeArguments As Type(), _
            ByVal Errors As List(Of String)) As Boolean

            ReportError(Errors, ResID.NotMostSpecificOverload)
            Return False
        End Function

        Private Shared Function CandidateIsUnspecific(ByVal Candidate As Method) As Boolean
            Return Not Candidate.NotCallable AndAlso Not Candidate.RequiresNarrowingConversion AndAlso Not Candidate.LessSpecific
        End Function

        Private Shared Function ReportUnspecificProcedures( _
            ByVal OverloadedProcedureName As String, _
            ByVal Candidates As List(Of Method), _
            ByVal Failure As ResolutionFailure) As Exception

            Return _
                ReportOverloadResolutionFailure( _
                    OverloadedProcedureName, _
                    Candidates, _
                    Nothing, _
                    Nothing, _
                    Nothing, _
                    ResID.NoMostSpecificOverload2, _
                    Failure, _
                    AddressOf DetectUnspecificity, _
                    AddressOf CandidateIsUnspecific)
        End Function

        Friend Shared Function ResolveOverloadedCall( _
                ByVal MethodName As String, _
                ByVal Candidates As List(Of Method), _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal LookupFlags As BindingFlags, _
                ByVal ReportErrors As Boolean, _
                ByRef Failure As ResolutionFailure) As Method

            'Optimistically hope to succeed.
            Failure = ResolutionFailure.None

            'From here on, CandidateCount will be used to keep track of the
            'number of remaining viable Candidates in the list.
            Dim CandidateCount As Integer = Candidates.Count
            Dim SomeCandidatesAreGeneric As Boolean = False

            Dim Best As Method = _
                RejectUncallableProcedures( _
                    Candidates, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    CandidateCount, _
                    SomeCandidatesAreGeneric)

            If CandidateCount = 1 Then
                Return Best
            End If

            If CandidateCount = 0 Then
                Failure = ResolutionFailure.InvalidArgument
                If ReportErrors Then
                    Throw ReportUncallableProcedures(MethodName, Candidates, Arguments, ArgumentNames, TypeArguments, Failure)
                End If
                Return Nothing
            End If

            If SomeCandidatesAreGeneric Then
                Best = RemoveRedundantGenericProcedures(Candidates, CandidateCount, Arguments, ArgumentNames)
                If CandidateCount = 1 Then
                    Return Best
                End If
            End If

            'See if only one does not require narrowing.  If all candidates require narrowing,
            'but one does so only from Object, pick that candidate.

            Dim NarrowOnlyFromObjectCount As Integer = 0
            Dim BestNarrowingCandidate As Method = Nothing

            For Each Candidate As Method In Candidates

                If Not Candidate.NotCallable Then
                    If Candidate.RequiresNarrowingConversion Then

                        CandidateCount -= 1

                        If Candidate.AllNarrowingIsFromObject Then
                            NarrowOnlyFromObjectCount += 1
                            BestNarrowingCandidate = Candidate
                        End If
                    Else
                        Best = Candidate
                    End If
                End If

            Next

            If CandidateCount = 1 Then
                Return Best
            End If

            If CandidateCount = 0 Then
                If NarrowOnlyFromObjectCount = 1 Then
                    Return BestNarrowingCandidate
                End If

                Failure = ResolutionFailure.AmbiguousMatch
                If ReportErrors Then
                    Throw ReportNarrowingProcedures(MethodName, Candidates, Arguments, ArgumentNames, TypeArguments, Failure)
                End If
                Return Nothing
            End If

            Best = MostSpecificProcedure(Candidates, CandidateCount, Arguments, ArgumentNames)

            If Best IsNot Nothing Then
                Return Best
            End If

            Failure = ResolutionFailure.AmbiguousMatch
            If ReportErrors Then
                Throw ReportUnspecificProcedures(MethodName, Candidates, Failure)
            End If
            Return Nothing
        End Function

        Friend Shared Function ResolveOverloadedCall( _
                ByVal MethodName As String, _
                ByVal Members As MemberInfo(), _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal LookupFlags As BindingFlags, _
                ByVal ReportErrors As Boolean, _
                ByRef Failure As ResolutionFailure,
                ByVal BaseReference As Container) As Method

#If BINDING_LOG Then
            Console.WriteLine("== MEMBERS ==")
            For Each m As MemberInfo In Members
                Console.WriteLine(MemberToString(m))
            Next
#End If

            'Build the list of candidate Methods, one of which overload resolution will
            'select.
            Dim RejectedForArgumentCount As Integer = 0
            Dim RejectedForTypeArgumentCount As Integer = 0

            Dim Candidates As List(Of Method) = _
                CollectOverloadCandidates( _
                    Members, _
                    Arguments, _
                    Arguments.Length, _
                    ArgumentNames, _
                    TypeArguments, _
                    False, _
                    Nothing, _
                    RejectedForArgumentCount, _
                    RejectedForTypeArgumentCount,
                    BaseReference)

            ' If there is only one candidate and it is NotCallable, let ResolveOverloadedCall
            ' figure out the error message and exception.
            If Candidates.Count = 1 AndAlso Not Candidates.Item(0).NotCallable Then
                Return Candidates.Item(0)
            End If

            If Candidates.Count = 0 Then
                Failure = ResolutionFailure.MissingMember

                If ReportErrors Then
                    Dim ErrorID As String = ResID.NoViableOverloadCandidates1

                    If RejectedForArgumentCount > 0 Then
                        ErrorID = ResID.NoArgumentCountOverloadCandidates1
                    ElseIf RejectedForTypeArgumentCount > 0 Then
                        ErrorID = ResID.NoTypeArgumentCountOverloadCandidates1
                    End If
                    Throw New MissingMemberException(GetResourceString(ErrorID, MethodName))
                End If
                Return Nothing
            End If

            Return _
                ResolveOverloadedCall( _
                    MethodName, _
                    Candidates, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    LookupFlags, _
                    ReportErrors, _
                    Failure)

        End Function

    End Class

End Namespace
