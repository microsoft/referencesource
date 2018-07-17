' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Collections.Generic
Imports System.Diagnostics
Imports System.Dynamic
Imports System.Linq.Expressions
Imports System.Reflection

Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.OverloadResolution
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

    Partial Public NotInheritable Class Operators

        Friend Shared Function CollectOperators( _
            ByVal Op As UserDefinedOperator, _
            ByVal Type1 As System.Type, _
            ByVal Type2 As System.Type, _
            ByRef FoundType1Operators As Boolean, _
            ByRef FoundType2Operators As Boolean) As List(Of Method)

            'Given an operator kind and two types to scan, construct a list of operators by
            'collecting operators from both types.
            '
            'The second type can be NULL, in which case operators are collected from only the first
            'type.

            Dim SearchBothTypes As Boolean = Type2 IsNot Nothing
            Dim Result As List(Of Method)

            If Not IsRootObjectType(Type1) AndAlso IsClassOrValueType(Type1) Then

                Dim Container As Container = New Container(Type1)
                Dim Members As MemberInfo() = Container.LookupNamedMembers(OperatorCLSNames(Op))

                Result = _
                    CollectOverloadCandidates( _
                        Members, _
                        Nothing, _
                        IIf(IsUnaryOperator(Op), 1, 2), _
                        Nothing, _
                        Nothing, _
                        True, _
                        Nothing, _
                        Nothing, _
                        Nothing,
                        Nothing)

                If Result.Count > 0 Then FoundType1Operators = True

            Else
                Result = New List(Of Method)
            End If

            If SearchBothTypes AndAlso Not IsRootObjectType(Type2) AndAlso IsClassOrValueType(Type2) Then

                Dim CommonAncestor As Type = Type1
                While CommonAncestor IsNot Nothing
                    If IsOrInheritsFrom(Type2, CommonAncestor) Then
                        Exit While
                    End If
                    CommonAncestor = CommonAncestor.BaseType
                End While

                Dim Container As Container = New Container(Type2)
                Dim Members As MemberInfo() = Container.LookupNamedMembers(OperatorCLSNames(Op))
                Dim SecondResult As List(Of Method)

                'Collect operators up until the common ancestor because we don't want
                'duplicate operators in the result list.
                SecondResult = _
                    CollectOverloadCandidates( _
                        Members, _
                        Nothing, _
                        IIf(IsUnaryOperator(Op), 1, 2), _
                        Nothing, _
                        Nothing, _
                        True, _
                        CommonAncestor, _
                        Nothing, _
                        Nothing,
                        Nothing)

                If SecondResult.Count > 0 Then FoundType2Operators = True

                'Merge the second result into the main result.
                Result.AddRange(SecondResult)
            End If

            Return Result

        End Function

        Friend Shared Function ResolveUserDefinedOperator( _
            ByVal Op As UserDefinedOperator, _
            ByVal Arguments As Object(), _
            ByVal ReportErrors As Boolean) As Method

            'Given an operation to perform with operands, select the appropriate
            'user-defined operator.  If one exists, it will be supplied as an out parameter.  This
            'function will generate compile errors if the resolution is ambiguous.
            '
            'Unary operators will have only one operand.
            '
            'To select the appropriate operator, first collect all applicable operators.  If only one
            'exists, resolution is complete. If more than one exists, perform standard method overload
            'resolution to select the correct operator.  If none exist, report an error.
            '
            'See the language specification for an in-depth discussion of the algorithm.

            Debug.Assert((IsBinaryOperator(Op) AndAlso Arguments.Length = 2) OrElse _
                         (IsUnaryOperator(Op) AndAlso Arguments.Length = 1), _
                         "second operand supplied for a unary operator?")

            'The value Nothing is treated as the default value of the type of the other operand in a binary operator expression.
            'If one of the operands is Nothing, find the other operand's type now.  In a unary operator expression, or if both
            'operands are Nothing in a binary operator expression, the type of operation is Integer.  However, these cases
            '(necessarily involving intrinsic types) should not reach this far.

            'During normal overload resolution, Nothing matches any type.  In the context of operator overload resolution,
            'Nothing must match only the type of the other operand.  To do this, we introduce the notion of a typed Nothing.
            'We represent a typed Nothing with an instance of a special object which overload resolution uses to understand that
            'Nothing should match only one type.

            'Make a copy of the arguments so that typed Nothings don't escape from this function.
            Arguments = DirectCast(Arguments.Clone, Object())

            Dim LeftType As Type
            Dim RightType As Type = Nothing

            If Arguments(0) Is Nothing Then
                Debug.Assert(Arguments.Length > 1, "unary op on Nothing unexpected here")
                Debug.Assert(Arguments(1) IsNot Nothing, "binary op on Nothing operands unexpected here")

                RightType = Arguments(1).GetType
                LeftType = RightType
                Arguments(0) = New TypedNothing(LeftType)
            Else
                LeftType = Arguments(0).GetType

                If Arguments.Length > 1 Then
                    If Arguments(1) IsNot Nothing Then
                        RightType = Arguments(1).GetType
                    Else
                        RightType = LeftType
                        Arguments(1) = New TypedNothing(RightType)
                    End If
                End If
            End If

            'First construct the list of operators we will consider.
            Dim FoundLeftOperators As Boolean
            Dim FoundRightOperators As Boolean
            Dim Candidates As List(Of Method) = _
                CollectOperators( _
                    Op, _
                    LeftType, _
                    RightType, _
                    FoundLeftOperators, _
                    FoundRightOperators)

            If Candidates.Count > 0 Then
                'There are operators available, so use standard method overload resolution
                'to choose the correct one.

                Dim Failure As ResolutionFailure

                Return _
                    ResolveOverloadedCall( _
                        OperatorNames(Op), _
                        Candidates, _
                        Arguments, _
                        NoArgumentNames, _
                        NoTypeArguments, _
                        BindingFlags.InvokeMethod, _
                        ReportErrors, _
                        Failure)
            End If

            Return Nothing

        End Function

        Friend Shared Function InvokeUserDefinedOperator( _
            ByVal OperatorMethod As Method, _
            ByVal ForceArgumentValidation As Boolean, _
            ByVal ParamArray Arguments As Object()) As Object

            Debug.Assert(OperatorMethod IsNot Nothing, "Operator can't be nothing at this point")

            'Overload resolution will potentially select one method before validating arguments.
            'Validate those arguments now.
            'CONSIDER: move the overload list construction up and out of overload resolution and into this function.
            If Not OperatorMethod.ArgumentsValidated OrElse ForceArgumentValidation Then

                If Not CanMatchArguments(OperatorMethod, Arguments, NoArgumentNames, NoTypeArguments, False, Nothing) Then

                    Const ReportErrors As Boolean = True
                    If ReportErrors Then
                        Dim ErrorMessage As String = ""
                        Dim Errors As New List(Of String)

                        Dim Result As Boolean = _
                            CanMatchArguments(OperatorMethod, Arguments, NoArgumentNames, NoTypeArguments, False, Errors)

                        Debug.Assert(Result = False AndAlso Errors.Count > 0, "expected this candidate to fail")

                        For Each ErrorString As String In Errors
                            ErrorMessage &= vbCrLf & "    " & ErrorString
                        Next

                        ErrorMessage = GetResourceString(ResID.MatchArgumentFailure2, OperatorMethod.ToString, ErrorMessage)
                        'We are missing a member which can match the arguments, so throw a missing member exception.
                        Throw New InvalidCastException(ErrorMessage)
                    End If

                    Return Nothing
                End If

            End If

            Dim BaseReference As Container = New Container(OperatorMethod.DeclaringType)
            Return _
                BaseReference.InvokeMethod( _
                    OperatorMethod, _
                    Arguments, _
                    Nothing, _
                    BindingFlags.InvokeMethod)
        End Function 'InvokeUserDefinedOperator

        Friend Shared Function InvokeUserDefinedOperator( _
            ByVal Op As UserDefinedOperator, _
            ByVal ParamArray Arguments As Object()) As Object

            If IDOUtils.TryCastToIDMOP(Arguments(0)) IsNot Nothing Then
                Return IDOBinder.InvokeUserDefinedOperator(Op, Arguments)
            Else
                Return InvokeObjectUserDefinedOperator(Op, Arguments)
            End If
        End Function 'InvokeUserDefinedOperator

        <ObsoleteAttribute("do not use this method", True)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function FallbackInvokeUserDefinedOperator( _
                ByVal vbOp As Object, _
                ByVal Arguments As Object()) As Object

            Return InvokeObjectUserDefinedOperator(CType(vbOp, UserDefinedOperator), Arguments)
        End Function 'FallbackInvokeUserDefinedOperator

        Friend Shared Function InvokeObjectUserDefinedOperator( _
            ByVal Op As UserDefinedOperator, _
            ByVal Arguments As Object()) As Object

            Dim OperatorMethod As Method = ResolveUserDefinedOperator(Op, Arguments, True)

            If OperatorMethod IsNot Nothing Then
                Return InvokeUserDefinedOperator(OperatorMethod, False, Arguments)
            End If

            'There are no results, so the operation is not defined for the operands.
            If Arguments.Length > 1 Then
                Throw GetNoValidOperatorException(Op, Arguments(0), Arguments(1))
            Else
                Throw GetNoValidOperatorException(Op, Arguments(0))
            End If
        End Function 'InvokeObjectUserDefinedOperator

        Friend Shared Function GetCallableUserDefinedOperator( _
            ByVal Op As UserDefinedOperator, _
            ByVal ParamArray Arguments As Object()) As Method

            Dim OperatorMethod As Method = ResolveUserDefinedOperator(Op, Arguments, False)

            If OperatorMethod IsNot Nothing Then
                'Overload resolution will potentially select one method before validating arguments.
                'Validate those arguments now.
                If Not OperatorMethod.ArgumentsValidated Then
                    If Not CanMatchArguments(OperatorMethod, Arguments, NoArgumentNames, NoTypeArguments, False, Nothing) Then
                        Return Nothing
                    End If
                End If
            End If

            Return OperatorMethod
        End Function

    End Class

End Namespace



