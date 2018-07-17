' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Diagnostics
Imports System.Reflection

Imports Microsoft.VisualBasic.CompilerServices.ConversionResolution
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#Region " BACKWARDS COMPATIBILITY "

    'WARNING WARNING WARNING WARNING WARNING
    'This code exists to support Everett compiled applications.  Make sure you understand
    'the backwards compatibility ramifications of any edit you make in this region.
    'WARNING WARNING WARNING WARNING WARNING
#If Not TELESTO Then 'These helpers aren't necessary on Telesto because we don't have an Everett backwards compat issue there.
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class FlowControl
        ' Prevent creation.
        Private Sub New()
        End Sub

        Private NotInheritable Class ObjectFor
            Public Counter As Object
            Public Limit As Object
            Public StepValue As Object
            Public PositiveStep As Boolean
            Public EnumType As Type

            Friend Sub New()
            End Sub
        End Class

        Public Shared Function ForNextCheckR4(ByVal count As Single, ByVal limit As Single, ByVal StepValue As Single) As Boolean
            If StepValue > 0 Then
                Return count <= limit
            Else
                Return count >= limit
            End If
        End Function

        Public Shared Function ForNextCheckR8(ByVal count As Double, ByVal limit As Double, ByVal StepValue As Double) As Boolean
            If StepValue > 0 Then
                Return count <= limit
            Else
                Return count >= limit
            End If
        End Function

        Public Shared Function ForNextCheckDec(ByVal count As Decimal, ByVal limit As Decimal, ByVal StepValue As Decimal) As Boolean
            If System.Decimal.op_LessThan(StepValue, System.Decimal.Zero) Then
                'StepValue <= 0
                'Return true if count >= limit
                Return System.Decimal.op_GreaterThanOrEqual(count, limit)
            Else
                'StepValue > 0
                'Return true if count <= limit
                Return System.Decimal.op_LessThanOrEqual(count, limit)
            End If
        End Function

        Public Shared Function ForLoopInitObj(ByVal Counter As Object, ByVal Start As Object, ByVal Limit As Object, ByVal StepValue As Object, ByRef LoopForResult As Object, ByRef CounterResult As Object) As Boolean
            Dim typ As System.TypeCode
            Dim LoopFor As ObjectFor
            Dim icompare As IComparable
            Dim CompareResult As Integer
            Dim Zero As Object

            If (Start Is Nothing) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Start"))
            ElseIf (Limit Is Nothing) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Limit"))
            ElseIf (StepValue Is Nothing) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Step"))
            End If

            Dim StartType As Type = Start.GetType()
            Dim LimitType As Type = Limit.GetType()
            Dim StepType As Type = StepValue.GetType()

            typ = ObjectType.GetWidestType(Start, Limit)
            typ = ObjectType.GetWidestType(StepValue, typ)

            If typ = TypeCode.String Then
                typ = TypeCode.Double
            End If

            If typ = TypeCode.Object Then
                Throw New ArgumentException(GetResourceString(ResID.ForLoop_CommonType3, VBFriendlyName(StartType), VBFriendlyName(LimitType), VBFriendlyName(StepValue)))
            End If

            LoopFor = New ObjectFor

            Dim StartTypeCode As TypeCode = Type.GetTypeCode(StartType)
            Dim LimitTypeCode As TypeCode = Type.GetTypeCode(LimitType)
            Dim StepTypeCode As TypeCode = Type.GetTypeCode(StepType)

            ' Funky. If one or more of the three values is an enum of the same underlying
            ' type as the loop, and all of the enum types are the same, then make the type
            ' of the loop the enum.
            Dim CurrentEnumType As Type = Nothing

            If (StartTypeCode = typ) AndAlso StartType.IsEnum Then
                CurrentEnumType = StartType
            End If

            If (LimitTypeCode = typ) AndAlso LimitType.IsEnum Then
                If (Not CurrentEnumType Is Nothing) AndAlso _
                   (Not CurrentEnumType Is LimitType) Then
                    CurrentEnumType = Nothing
                    GoTo NotEnumType
                End If

                CurrentEnumType = LimitType
            End If

            If (StepTypeCode = typ) AndAlso StepType.IsEnum Then
                If (Not CurrentEnumType Is Nothing) AndAlso _
                   (Not CurrentEnumType Is StepType) Then
                    CurrentEnumType = Nothing
                    GoTo NotEnumType
                End If

                CurrentEnumType = StepType
            End If
NotEnumType:
            LoopFor.EnumType = CurrentEnumType

            Try
                LoopFor.Counter = ObjectType.CTypeHelper(Start, typ)
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Start", VBFriendlyName(StartType), VBFriendlyName(ObjectType.TypeFromTypeCode(typ))))
            End Try

            Try
                LoopFor.Limit = ObjectType.CTypeHelper(Limit, typ)
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Limit", VBFriendlyName(LimitType), VBFriendlyName(ObjectType.TypeFromTypeCode(typ))))
            End Try

            Try
                LoopFor.StepValue = ObjectType.CTypeHelper(StepValue, typ)
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Step", VBFriendlyName(StepType), VBFriendlyName(ObjectType.TypeFromTypeCode(typ))))
            End Try

            'Check and save whether this is a positive or negative step
            Zero = ObjectType.CTypeHelper(0, typ)
            icompare = CType(LoopFor.StepValue, IComparable)
            CompareResult = icompare.CompareTo(Zero)

            If CompareResult >= 0 Then
                LoopFor.PositiveStep = True
            Else
                LoopFor.PositiveStep = False
            End If

            LoopForResult = LoopFor
            If Not LoopFor.EnumType Is Nothing Then
                CounterResult = System.Enum.ToObject(LoopFor.EnumType, LoopFor.Counter)
            Else
                CounterResult = LoopFor.Counter
            End If
            Return CheckContinueLoop(LoopFor)
        End Function

        Public Shared Function ForNextCheckObj(ByVal Counter As Object, ByVal LoopObj As Object, ByRef CounterResult As Object) As Boolean

            Dim LoopFor As ObjectFor

            If LoopObj Is Nothing Then
                Throw VbMakeException(vbErrors.IllegalFor)
            End If

            If Counter Is Nothing Then
                Throw New NullReferenceException(GetResourceString(ResID.Argument_InvalidNullValue1, "Counter"))
            End If

            LoopFor = CType(LoopObj, ObjectFor)

            Dim type1, type2 As TypeCode
            Dim WidestType, ResultType As TypeCode

            ' At this point, we know it's IConvertible
            type1 = CType(Counter, IConvertible).GetTypeCode()
            type2 = CType(LoopFor.StepValue, IConvertible).GetTypeCode()

            If (type1 = type2) AndAlso (Not type1 = TypeCode.String) Then
                'Nothing to do now
                WidestType = type1
            Else
                WidestType = ObjectType.GetWidestType(type1, type2)
                If WidestType = TypeCode.String Then
                    WidestType = TypeCode.Double
                End If

                If ResultType = TypeCode.Object Then
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_CommonType2, VBFriendlyName(ObjectType.TypeFromTypeCode(type1)), VBFriendlyName(ObjectType.TypeFromTypeCode(type2))))
                End If

                Try
                    Counter = ObjectType.CTypeHelper(Counter, WidestType)
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Start", VBFriendlyName(Counter.GetType()), VBFriendlyName(ObjectType.TypeFromTypeCode(WidestType))))
                End Try

                Try
                    LoopFor.Limit = ObjectType.CTypeHelper(LoopFor.Limit, WidestType)
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Limit", VBFriendlyName(LoopFor.Limit.GetType()), VBFriendlyName(ObjectType.TypeFromTypeCode(WidestType))))
                End Try

                Try
                    LoopFor.StepValue = ObjectType.CTypeHelper(LoopFor.StepValue, WidestType)
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, "Step", VBFriendlyName(LoopFor.StepValue.GetType()), VBFriendlyName(ObjectType.TypeFromTypeCode(WidestType))))
                End Try
            End If

            LoopFor.Counter = ObjectType.AddObj(Counter, LoopFor.StepValue)
            ResultType = CType(LoopFor.Counter, IConvertible).GetTypeCode()

            If Not LoopFor.EnumType Is Nothing Then
                CounterResult = System.Enum.ToObject(LoopFor.EnumType, LoopFor.Counter)
            Else
                CounterResult = LoopFor.Counter
            End If

            If Not (ResultType = WidestType) Then
                'Overflow to bigger type occurred
                LoopFor.Limit = ObjectType.CTypeHelper(LoopFor.Limit, ResultType)
                LoopFor.StepValue = ObjectType.CTypeHelper(LoopFor.StepValue, ResultType)
                'If we overflow, then we should always be at the end of the loop
                Return False
            End If

            ForNextCheckObj = CheckContinueLoop(LoopFor)
        End Function

        Public Shared Function ForEachInArr(ByVal ary As System.Array) As Collections.IEnumerator
            Dim Result As Collections.IEnumerator = CType(ary, Collections.ICollection).GetEnumerator()
            If Result Is Nothing Then
                Throw VbMakeException(vbErrors.IllegalFor)
            End If
            Return Result
        End Function

        Public Shared Function ForEachInObj(ByVal obj As Object) As Collections.IEnumerator
            Dim Result As Collections.IEnumerator
            Dim ienum As System.Collections.IEnumerable

            If obj Is Nothing Then
                Throw VbMakeException(vbErrors.ObjNotSet)
            End If

            'Initialize the enumerator 
            Try
                ienum = CType(obj, Collections.IEnumerable)
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw MakeException1(vbErrors.DoesntImplementICollection, obj.GetType.ToString)
            End Try

            Result = ienum.GetEnumerator() 'Positioned before first element

            If Result Is Nothing Then
                Throw MakeException1(vbErrors.DoesntImplementICollection, obj.GetType.ToString)
            End If

            Return Result
        End Function

        Public Shared Function ForEachNextObj(ByRef obj As Object, ByVal enumerator As Collections.IEnumerator) As Boolean
            If enumerator.MoveNext() Then
                obj = enumerator.Current
                Return True
            Else
                obj = Nothing
                Return False
            End If
        End Function

        Private Shared Function CheckContinueLoop(ByVal LoopFor As ObjectFor) As Boolean
            Dim icompare As IComparable
            Dim CompareResult As Integer

            Try
                icompare = CType(LoopFor.Counter, IComparable)
                CompareResult = icompare.CompareTo(LoopFor.Limit)

                If LoopFor.PositiveStep Then
                    If CompareResult <= 0 Then
                        Return True
                    Else
                        Return False
                    End If
                Else
                    If CompareResult >= 0 Then
                        Return True
                    Else
                        Return False
                    End If
                End If

            Catch ex As InvalidCastException
                Throw New ArgumentException(GetResourceString(ResID.Argument_IComparable2, "loop control variable", VBFriendlyName(LoopFor.Counter)))
            End Try
        End Function

        Public Shared Sub CheckForSyncLockOnValueType(ByVal obj As Object)
            If Not obj Is Nothing AndAlso obj.GetType.IsValueType() Then
                Throw New ArgumentException(GetResourceString(ResID.SyncLockRequiresReferenceType1, VBFriendlyName(obj.GetType)))
            End If
        End Sub

    End Class
#End If 'not TELESTO
#End Region

 

    'REVIEW VSW#395750:  need to rewrite these FlowControl helpers to handle unsigned types.
#If TELESTO And Not NETCORE Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
    Friend NotInheritable Class ObjectFlowControl
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class ObjectFlowControl
#End If

        Private Sub New()
        End Sub

#If TELESTO Then

#If NETCORE
        Public Shared Sub CheckForSyncLockOnValueType(ByVal Expression As Object)
#Else
        Friend Shared Sub CheckForSyncLockOnValueType(ByVal Expression As Object)
#End If

            If Expression IsNot Nothing AndAlso Expression.GetType.IsValueType() Then
                Throw New ArgumentException( _
                    GetResourceString(ResID.SyncLockRequiresReferenceType1, VBFriendlyName(Expression.GetType)))
            End If
        End Sub
#End If

#If TELESTO And Not NETCORE Then
    End Class

    Public NotInheritable Class LateBinderObjectFlowControl
        Public NotInheritable Class ForLoopControl 'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
#Else
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
        Public NotInheritable Class ForLoopControl
#End If
            Private Counter As Object
            Private Limit As Object
            Private StepValue As Object
            Private PositiveStep As Boolean
            Private EnumType As Type
            Private WidestType As Type
            Private WidestTypeCode As TypeCode
            Private UseUserDefinedOperators As Boolean
            Private OperatorPlus As Method
            Private OperatorGreaterEqual As Method
            Private OperatorLessEqual As Method


            Private Sub New()
            End Sub

            ' CONSIDER: Is there a better way of doing this?
            Private Shared Function GetWidestType(ByVal Type1 As System.Type, ByVal Type2 As System.Type) As Type
                If Type1 Is Nothing OrElse Type2 Is Nothing Then Return Nothing

                If Not Type1.IsEnum AndAlso Not Type2.IsEnum Then
                    Dim tc1 As TypeCode = GetTypeCode(Type1)
                    Dim tc2 As TypeCode = GetTypeCode(Type2)

                    If IsNumericType(tc1) AndAlso IsNumericType(tc2) Then
                        Return MapTypeCodeToType(ForLoopWidestTypeCode(tc1)(tc2))
                    End If
                End If

                Dim LeftToRight As ConversionClass = ClassifyConversion(Type2, Type1, Nothing)
                If LeftToRight = ConversionClass.Identity OrElse LeftToRight = ConversionClass.Widening Then
                    Return Type2
                End If

                Dim RightToLeft As ConversionClass = ClassifyConversion(Type1, Type2, Nothing)
                If RightToLeft = ConversionClass.Widening Then
                    Return Type1
                End If

                Return Nothing
            End Function

            Private Shared Function GetWidestType(ByVal Type1 As System.Type, ByVal Type2 As System.Type, ByVal Type3 As System.Type) As Type
                Return GetWidestType(Type1, GetWidestType(Type2, Type3))
            End Function

            Private Shared Function ConvertLoopElement(ByVal ElementName As String, ByVal Value As Object, ByVal SourceType As Type, ByVal TargetType As Type) As Object
                Try
                    Return Conversions.ChangeType(Value, TargetType)
                Catch ex As AccessViolationException
                    Throw ex
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_ConvertToType3, ElementName, VBFriendlyName(SourceType), VBFriendlyName(TargetType)))
                End Try
            End Function

            Private Shared Function VerifyForLoopOperator( _
                ByVal Op As UserDefinedOperator, _
                ByVal ForLoopArgument As Object, _
                ByVal ForLoopArgumentType As Type) As Method

                Dim OperatorMethod As Method = Operators.GetCallableUserDefinedOperator(Op, ForLoopArgument, ForLoopArgument)

                If OperatorMethod Is Nothing Then
                    Throw New ArgumentException(GetResourceString( _
                        ResID.ForLoop_OperatorRequired2, _
                        VBFriendlyNameOfType(ForLoopArgumentType, FullName:=True), _
                        Symbols.OperatorNames(Op)))
                End If

                Dim OperatorInfo As MethodInfo = TryCast(OperatorMethod.AsMethod, MethodInfo)
                Dim Parameters As ParameterInfo() = OperatorInfo.GetParameters

                ' Validate the types
                Select Case Op
                    Case UserDefinedOperator.Plus, UserDefinedOperator.Minus
                        If Parameters.Length <> 2 OrElse _
                            Parameters(0).ParameterType IsNot ForLoopArgumentType OrElse _
                            Parameters(1).ParameterType IsNot ForLoopArgumentType OrElse _
                            OperatorInfo.ReturnType IsNot ForLoopArgumentType Then
                            Throw New ArgumentException(GetResourceString( _
                                ResID.ForLoop_UnacceptableOperator2, _
                                OperatorMethod.ToString, _
                                VBFriendlyNameOfType(ForLoopArgumentType, FullName:=True)))
                        End If

                    Case UserDefinedOperator.LessEqual, UserDefinedOperator.GreaterEqual
                        If Parameters.Length <> 2 OrElse _
                            Parameters(0).ParameterType IsNot ForLoopArgumentType OrElse _
                            Parameters(1).ParameterType IsNot ForLoopArgumentType Then
                            Throw New ArgumentException(GetResourceString( _
                                ResID.ForLoop_UnacceptableRelOperator2, _
                                OperatorMethod.ToString, _
                                VBFriendlyNameOfType(ForLoopArgumentType, FullName:=True)))
                        End If
                End Select

                Return OperatorMethod
            End Function

            Public Shared Function ForLoopInitObj(ByVal Counter As Object, ByVal Start As Object, ByVal Limit As Object, ByVal StepValue As Object, ByRef LoopForResult As Object, ByRef CounterResult As Object) As Boolean
                'CONSIDER: Find a better way of doing the compare
                Dim LoopFor As ForLoopControl

                If (Start Is Nothing) Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Start"))
                ElseIf (Limit Is Nothing) Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Limit"))
                ElseIf (StepValue Is Nothing) Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNullValue1, "Step"))
                End If

                Dim StartType As Type = Start.GetType()
                Dim LimitType As Type = Limit.GetType()
                Dim StepType As Type = StepValue.GetType()

                Dim WidestType As Type = GetWidestType(StepType, StartType, LimitType)

                If WidestType Is Nothing Then
                    Throw New ArgumentException(GetResourceString(ResID.ForLoop_CommonType3, VBFriendlyName(StartType), VBFriendlyName(LimitType), VBFriendlyName(StepValue)))
                End If

                LoopFor = New ForLoopControl

                Dim WidestTypeCode As TypeCode = GetTypeCode(WidestType)

                ' If the widest typecode is Object, try to use user defined conversions.
                If WidestTypeCode = TypeCode.Object Then
                    LoopFor.UseUserDefinedOperators = True
                End If

                If WidestTypeCode = TypeCode.String Then
                    WidestTypeCode = TypeCode.Double
                End If

                Dim StartTypeCode As TypeCode = Type.GetTypeCode(StartType)
                Dim LimitTypeCode As TypeCode = Type.GetTypeCode(LimitType)
                Dim StepTypeCode As TypeCode = Type.GetTypeCode(StepType)

                ' Funky. If one or more of the three values is an enum of the same underlying
                ' type as the loop, and all of the enum types are the same, then make the type
                ' of the loop the enum.
                Dim CurrentEnumType As Type = Nothing

                If (StartTypeCode = WidestTypeCode) AndAlso StartType.IsEnum Then
                    CurrentEnumType = StartType
                End If

                If (LimitTypeCode = WidestTypeCode) AndAlso LimitType.IsEnum Then
                    If (Not CurrentEnumType Is Nothing) AndAlso _
                       (Not CurrentEnumType Is LimitType) Then
                        CurrentEnumType = Nothing
                        GoTo NotEnumType
                    End If

                    CurrentEnumType = LimitType
                End If

                If (StepTypeCode = WidestTypeCode) AndAlso StepType.IsEnum Then
                    If (Not CurrentEnumType Is Nothing) AndAlso _
                       (Not CurrentEnumType Is StepType) Then
                        CurrentEnumType = Nothing
                        GoTo NotEnumType
                    End If

                    CurrentEnumType = StepType
                End If
NotEnumType:
                LoopFor.EnumType = CurrentEnumType

                If Not LoopFor.UseUserDefinedOperators Then
                    LoopFor.WidestType = MapTypeCodeToType(WidestTypeCode)
                Else
                    LoopFor.WidestType = WidestType
                End If

                LoopFor.WidestTypeCode = WidestTypeCode

                LoopFor.Counter = ConvertLoopElement("Start", Start, StartType, LoopFor.WidestType)
                LoopFor.Limit = ConvertLoopElement("Limit", Limit, LimitType, LoopFor.WidestType)
                LoopFor.StepValue = ConvertLoopElement("Step", StepValue, StepType, LoopFor.WidestType)

                ' Verify that the required operators are present.
                If LoopFor.UseUserDefinedOperators Then
                    LoopFor.OperatorPlus = VerifyForLoopOperator(UserDefinedOperator.Plus, LoopFor.Counter, LoopFor.WidestType)
                    VerifyForLoopOperator(UserDefinedOperator.Minus, LoopFor.Counter, LoopFor.WidestType)
                    LoopFor.OperatorLessEqual = VerifyForLoopOperator(UserDefinedOperator.LessEqual, LoopFor.Counter, LoopFor.WidestType)
                    LoopFor.OperatorGreaterEqual = VerifyForLoopOperator(UserDefinedOperator.GreaterEqual, LoopFor.Counter, LoopFor.WidestType)
                End If

                'Important: a Zero step is considered Positive. This is consistent with the early-bound behavior.
                LoopFor.PositiveStep = Operators.ConditionalCompareObjectGreaterEqual( _
                        LoopFor.StepValue, _
                        Operators.SubtractObject(LoopFor.StepValue, LoopFor.StepValue), _
                        False)

                LoopForResult = LoopFor

                If Not LoopFor.EnumType Is Nothing Then
                    CounterResult = System.Enum.ToObject(LoopFor.EnumType, LoopFor.Counter)
                Else
                    CounterResult = LoopFor.Counter
                End If

                Return CheckContinueLoop(LoopFor)
            End Function

            Public Shared Function ForNextCheckObj(ByVal Counter As Object, ByVal LoopObj As Object, ByRef CounterResult As Object) As Boolean

                Dim LoopFor As ForLoopControl

                If LoopObj Is Nothing Then
                    Throw VbMakeException(vbErrors.IllegalFor)
                End If

                If Counter Is Nothing Then
                    Throw New NullReferenceException(GetResourceString(ResID.Argument_InvalidNullValue1, "Counter"))
                End If

                LoopFor = CType(LoopObj, ForLoopControl)

                Dim NeedToChangeType As Boolean = False

                If Not LoopFor.UseUserDefinedOperators Then
                    ' At this point, we know it's IConvertible
                    Dim CounterTypeCode As TypeCode = DirectCast(Counter, IConvertible).GetTypeCode()

                    If CounterTypeCode <> LoopFor.WidestTypeCode OrElse CounterTypeCode = TypeCode.String Then
                        If CounterTypeCode = TypeCode.Object Then
                            Throw New ArgumentException(GetResourceString(ResID.ForLoop_CommonType2, VBFriendlyName(MapTypeCodeToType(CounterTypeCode)), VBFriendlyName(LoopFor.WidestType)))
                        Else
                            Dim WidestType As Type = GetWidestType(MapTypeCodeToType(CounterTypeCode), LoopFor.WidestType)
                            Dim WidestTypeCode As TypeCode = GetTypeCode(WidestType)

                            If WidestTypeCode = TypeCode.String Then
                                WidestTypeCode = TypeCode.Double
                            End If

                            LoopFor.WidestTypeCode = WidestTypeCode
                            LoopFor.WidestType = MapTypeCodeToType(WidestTypeCode)
                            NeedToChangeType = True
                        End If
                    End If
                End If

                If NeedToChangeType OrElse LoopFor.UseUserDefinedOperators Then
                    Counter = ConvertLoopElement("Start", Counter, Counter.GetType(), LoopFor.WidestType)

                    If Not LoopFor.UseUserDefinedOperators Then
                        LoopFor.Limit = ConvertLoopElement("Limit", LoopFor.Limit, LoopFor.Limit.GetType(), LoopFor.WidestType)
                        LoopFor.StepValue = ConvertLoopElement("Step", LoopFor.StepValue, LoopFor.StepValue.GetType(), LoopFor.WidestType)
                    End If
                End If

                If Not LoopFor.UseUserDefinedOperators Then
                    LoopFor.Counter = Operators.AddObject(Counter, LoopFor.StepValue)

                    Dim ResultTypeCode As TypeCode = CType(LoopFor.Counter, IConvertible).GetTypeCode()

                    If Not LoopFor.EnumType Is Nothing Then
                        CounterResult = System.Enum.ToObject(LoopFor.EnumType, LoopFor.Counter)
                    Else
                        CounterResult = LoopFor.Counter
                    End If

                    If ResultTypeCode <> LoopFor.WidestTypeCode Then
                        'Overflow to bigger type occurred
                        LoopFor.Limit = Conversions.ChangeType(LoopFor.Limit, MapTypeCodeToType(ResultTypeCode))
                        LoopFor.StepValue = Conversions.ChangeType(LoopFor.StepValue, MapTypeCodeToType(ResultTypeCode))
                        'If we overflow, then we should always be at the end of the loop
                        Return False
                    End If
                Else
                    ' Execute addition.
                    LoopFor.Counter = Operators.InvokeUserDefinedOperator( _
                        LoopFor.OperatorPlus, _
                        True, _
                        Counter, _
                        LoopFor.StepValue)

                    If LoopFor.Counter.GetType() IsNot LoopFor.WidestType Then
                        LoopFor.Counter = ConvertLoopElement("Start", LoopFor.Counter, LoopFor.Counter.GetType(), LoopFor.WidestType)
                    End If

                    CounterResult = LoopFor.Counter
                End If

                Return CheckContinueLoop(LoopFor)
            End Function

            Public Shared Function ForNextCheckR4(ByVal count As Single, ByVal limit As Single, ByVal StepValue As Single) As Boolean
                'Important: a Zero step is considered Positive. This is consistent with integral For loops.
                If StepValue >= 0 Then
                    Return count <= limit
                Else
                    Return count >= limit
                End If
            End Function

            Public Shared Function ForNextCheckR8(ByVal count As Double, ByVal limit As Double, ByVal StepValue As Double) As Boolean
                'Important: a Zero step is considered Positive. This is consistent with integral For loops.
                If StepValue >= 0 Then
                    Return count <= limit
                Else
                    Return count >= limit
                End If
            End Function

            'CONSIDER:  If the compiler used operator overloading for Decimals in For loops, this function would no longer be needed.
            Public Shared Function ForNextCheckDec(ByVal count As Decimal, ByVal limit As Decimal, ByVal StepValue As Decimal) As Boolean
                'Important: a Zero step is considered Positive. This is consistent with integral For loops.
                If StepValue >= 0 Then
                    Return count <= limit
                Else
                    Return count >= limit
                End If
            End Function

            Private Shared Function CheckContinueLoop(ByVal LoopFor As ForLoopControl) As Boolean

                If Not LoopFor.UseUserDefinedOperators Then
                    Dim icompare As IComparable
                    Dim CompareResult As Integer

                    Try
                        icompare = CType(LoopFor.Counter, IComparable)
                        CompareResult = icompare.CompareTo(LoopFor.Limit)

                        If LoopFor.PositiveStep Then
                            Return CompareResult <= 0
                        Else
                            Return CompareResult >= 0
                        End If

                    Catch ex As InvalidCastException
                        Throw New ArgumentException(GetResourceString(ResID.Argument_IComparable2, "loop control variable", VBFriendlyName(LoopFor.Counter)))
                    End Try
                Else
                    If LoopFor.PositiveStep Then
                        Return CBool(Operators.InvokeUserDefinedOperator( _
                            LoopFor.OperatorLessEqual, _
                            True, _
                            LoopFor.Counter, _
                            LoopFor.Limit))
                    Else
                        Return CBool(Operators.InvokeUserDefinedOperator( _
                            LoopFor.OperatorGreaterEqual, _
                            True, _
                            LoopFor.Counter, _
                            LoopFor.Limit))
                    End If
                End If
            End Function

        End Class
    End Class

End Namespace
