' Copyright (c) Microsoft Corporation.  All rights reserved.

Option Strict On

Imports System
Imports System.Diagnostics
Imports System.Globalization
Imports System.Collections.Generic
Imports System.Reflection

Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)>
    Public NotInheritable Class Operators
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class Operators
#End If

        Friend Shared ReadOnly Boxed_ZeroDouble As Object = 0.0R
        Friend Shared ReadOnly Boxed_ZeroSinge As Object = 0.0F
        Friend Shared ReadOnly Boxed_ZeroDecimal As Object = CDec(0)
        Friend Shared ReadOnly Boxed_ZeroLong As Object = 0L
        Friend Shared ReadOnly Boxed_ZeroInteger As Object = 0I
        Friend Shared ReadOnly Boxed_ZeroShort As Object = 0S
        Friend Shared ReadOnly Boxed_ZeroULong As Object = 0UL
        Friend Shared ReadOnly Boxed_ZeroUInteger As Object = 0UI
        Friend Shared ReadOnly Boxed_ZeroUShort As Object = 0US
        Friend Shared ReadOnly Boxed_ZeroSByte As Object = CSByte(0)
        Friend Shared ReadOnly Boxed_ZeroByte As Object = CByte(0)

        Private Sub New()
        End Sub

        Private Const TCMAX As Integer = TypeCode.String + 1

        Private Shared Function ToVBBool(ByVal conv As IConvertible) As SByte
            Return CSByte(conv.ToBoolean(Nothing))
        End Function

        Private Shared Function ToVBBoolConv(ByVal conv As IConvertible) As IConvertible
            Return CSByte(conv.ToBoolean(Nothing))
        End Function

        'This function determines the enum result type of And, Or, Xor operations.
        'If the type of Left and Right are the same enum type, then return that type, otherwise if
        'one is an enum and the other is Nothing, return that type, otherwise return Nothing.
        Private Shared Function GetEnumResult(ByVal Left As Object, ByVal Right As Object) As Type

            Debug.Assert(Left Is Nothing OrElse Right Is Nothing OrElse CType(Left, IConvertible).GetTypeCode = CType(Right, IConvertible).GetTypeCode, _
                         "Expected identical type codes for checking enum result")


            If Left IsNot Nothing Then

                If TypeOf Left Is System.Enum Then

                    If Right Is Nothing Then
                        Return Left.GetType

                    ElseIf TypeOf Right Is System.Enum Then
                        Dim LeftType As Type = Left.GetType
                        If LeftType Is Right.GetType Then
                            Return LeftType
                        End If

                    End If

                End If

            ElseIf TypeOf Right Is System.Enum Then
                Return Right.GetType

            End If

            Return Nothing

        End Function

        Private Shared Function GetNoValidOperatorException(ByVal Op As UserDefinedOperator, ByVal Operand As Object) As Exception
            Return New InvalidCastException(GetResourceString(ResID.UnaryOperand2, OperatorNames(Op), VBFriendlyName(Operand)))
        End Function

        Private Shared Function GetNoValidOperatorException(ByVal Op As UserDefinedOperator, ByVal Left As Object, ByVal Right As Object) As Exception
            Const MAX_INSERTION_SIZE As Integer = 32

            Dim Substitution1 As String
            Dim Substitution2 As String

            If Left Is Nothing Then
                Substitution1 = "'Nothing'"
            Else
                Dim LeftString As String = TryCast(Left, String)

                If LeftString IsNot Nothing Then
                    Substitution1 = _
                        GetResourceString(ResID.NoValidOperator_StringType1, Strings.Left(LeftString, MAX_INSERTION_SIZE))
                Else
                    Substitution1 = GetResourceString(ResID.NoValidOperator_NonStringType1, VBFriendlyName(Left))
                End If
            End If

            If Right Is Nothing Then
                Substitution2 = "'Nothing'"
            Else
                Dim RightString As String = TryCast(Right, String)

                If RightString IsNot Nothing Then
                    Substitution2 = _
                        GetResourceString(ResID.NoValidOperator_StringType1, Strings.Left(RightString, MAX_INSERTION_SIZE))
                Else
                    Substitution2 = GetResourceString(ResID.NoValidOperator_NonStringType1, VBFriendlyName(Right))
                End If
            End If

            Return New InvalidCastException(GetResourceString(ResID.BinaryOperands3, OperatorNames(Op), Substitution1, Substitution2))
        End Function

#Region " Comparison Operators = <> < <= > >= "

        Private Enum CompareClass
            Less = -1
            Equal = 0
            Greater = 1
            Unordered
            UserDefined
            Undefined
        End Enum

        Public Shared Function CompareObjectEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Equal, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Equal, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison = 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.Equal, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Equal, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison = 0
            End Select
        End Function

        Public Shared Function CompareObjectNotEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return True
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.NotEqual, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.NotEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison <> 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectNotEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return True
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.NotEqual, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.NotEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison <> 0
            End Select
        End Function

        Public Shared Function CompareObjectLess(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Less, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Less, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison < 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectLess(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.Less, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Less, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison < 0
            End Select
        End Function

        Public Shared Function CompareObjectLessEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.LessEqual, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.LessEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison <= 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectLessEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.LessEqual, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.LessEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison <= 0
            End Select
        End Function

        Public Shared Function CompareObjectGreaterEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.GreaterEqual, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.GreaterEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison >= 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectGreaterEqual(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.GreaterEqual, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.GreaterEqual, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison >= 0
            End Select
        End Function

        Public Shared Function CompareObjectGreater(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Object
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Greater, Left, Right)
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Greater, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison > 0
            End Select
        End Function

        Public Shared Function ConditionalCompareObjectGreater(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Boolean
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return False
                Case CompareClass.UserDefined
                    Return CBool(InvokeUserDefinedOperator(UserDefinedOperator.Greater, Left, Right))
                Case CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.Greater, Left, Right)
                Case Else
                    Debug.Assert(Comparison = CompareClass.Less OrElse _
                                 Comparison = CompareClass.Equal OrElse _
                                 Comparison = CompareClass.Greater)
                    Return Comparison > 0
            End Select
        End Function

        'UNDONE UNDONE UNDONE:  remove this function after the next toolset update that incorporates build 40213.
        'and rename CompareObject2 back to CompareObject, but keep it private.
        Public Shared Function CompareObject(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As Integer
            Dim Comparison As CompareClass = CompareObject2(Left, Right, TextCompare)

            Select Case Comparison
                Case CompareClass.Unordered
                    Return 0
                Case CompareClass.UserDefined, _
                     CompareClass.Undefined
                    Throw GetNoValidOperatorException(UserDefinedOperator.IsTrue, Left, Right)
                Case Else
                    Return Comparison
            End Select
        End Function

        Private Shared Function CompareObject2(ByVal Left As Object, ByVal Right As Object, ByVal TextCompare As Boolean) As CompareClass
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)
            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If

            conv2 = TryCast(Right, IConvertible)
            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'Special cases for Char()
            If tc1 = TypeCode.Object Then
                Dim LeftCharArray As Char() = TryCast(left, Char())

                If LeftCharArray IsNot Nothing Then
                    If tc2 = TypeCode.String OrElse tc2 = TypeCode.Empty OrElse ((tc2 = TypeCode.Object) AndAlso (TypeOf Right Is Char())) Then
                        'Treat Char() as String for these cases
                        Left = CStr(LeftCharArray)
                        conv1 = CType(Left, IConvertible)
                        tc1 = TypeCode.String
                    End If
                End If
            End If

            If (tc2 = TypeCode.Object) Then
                Dim RightCharArray As Char() = TryCast(right, Char())

                If RightCharArray IsNot Nothing Then
                    If tc1 = TypeCode.String OrElse tc1 = TypeCode.Empty Then
                        Right = CStr(RightCharArray)
                        conv2 = DirectCast(Right, IConvertible)
                        tc2 = TypeCode.String
                    End If
                End If
            End If

            Select Case tc1 * TCMAX + tc2  'CONSIDER: overflow checking is not necessary for this calculation - perf improvement.

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return 0

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return CompareBoolean(Nothing, conv2.ToBoolean(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return CompareInt32(Nothing, conv2.ToSByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return CompareInt32(Nothing, conv2.ToByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return CompareInt32(Nothing, conv2.ToInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return CompareInt32(Nothing, conv2.ToUInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return CompareInt32(Nothing, conv2.ToInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return CompareUInt32(Nothing, conv2.ToUInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return CompareInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return CompareUInt64(Nothing, conv2.ToUInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal
                    Return CompareDecimal(0D, conv2)

                Case TypeCode.Empty * TCMAX + TypeCode.Single
                    Return CompareSingle(Nothing, conv2.ToSingle(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Double
                    Return CompareDouble(Nothing, conv2.ToDouble(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.DateTime
                    Return CompareDate(Nothing, conv2.ToDateTime(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Char
                    Return CompareChar(Nothing, conv2.ToChar(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return CType(CompareString(Nothing, conv2.ToString(Nothing), TextCompare), CompareClass)


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return CompareBoolean(conv1.ToBoolean(Nothing), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return CompareBoolean(conv1.ToBoolean(Nothing), conv2.ToBoolean(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return CompareInt32(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return CompareInt32(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return CompareInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64
                    Return CompareInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return CompareDecimal(ToVBBoolConv(conv1), conv2)

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return CompareSingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return CompareDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return CompareBoolean(conv1.ToBoolean(Nothing), CBool(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return CompareInt32(conv1.ToSByte(Nothing), Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return CompareInt32(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return CompareInt32(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return CompareInt32(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return CompareInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64

                    Return CompareInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal

                    Return CompareDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single

                    Return CompareSingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return CompareDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return CompareDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))

                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return CompareInt32(conv1.ToByte(Nothing), Nothing)

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean
                    Return CompareInt32(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return CompareInt32(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16

                    Return CompareInt32(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32

                    Return CompareUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return CompareUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return CompareInt32(conv1.ToInt16(Nothing), Nothing)

                Case TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return CompareInt32(conv1.ToInt16(Nothing), ToVBBool(conv2))


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return CompareInt32(conv1.ToUInt16(Nothing), Nothing)

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean
                    Return CompareInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return CompareInt32(conv1.ToInt32(Nothing), Nothing)

                Case TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return CompareInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return CompareUInt32(conv1.ToUInt32(Nothing), Nothing)

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean
                    Return CompareInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return CompareInt64(conv1.ToInt64(Nothing), Nothing)

                Case TypeCode.Int64 * TCMAX + TypeCode.Boolean
                    Return CompareInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return CompareUInt64(conv1.ToUInt64(Nothing), Nothing)

                Case TypeCode.UInt64 * TCMAX + TypeCode.Boolean
                    Return CompareDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty
                    Return CompareDecimal(conv1, 0D)

                Case TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return CompareDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Single * TCMAX + TypeCode.Empty
                    Return CompareSingle(conv1.ToSingle(Nothing), Nothing)

                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return CompareSingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.Double * TCMAX + TypeCode.Empty
                    Return CompareDouble(conv1.ToDouble(Nothing), Nothing)

                Case TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return CompareDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))


                Case TypeCode.DateTime * TCMAX + TypeCode.Empty
                    Return CompareDate(conv1.ToDateTime(Nothing), Nothing)

                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime
                    Return CompareDate(conv1.ToDateTime(Nothing), conv2.ToDateTime(Nothing))

                Case TypeCode.DateTime * TCMAX + TypeCode.String
                    Return CompareDate(conv1.ToDateTime(Nothing), CDate(conv2.ToString(Nothing)))


                Case TypeCode.Char * TCMAX + TypeCode.Empty
                    Return CompareChar(conv1.ToChar(Nothing), Nothing)

                Case TypeCode.Char * TCMAX + TypeCode.Char
                    Return CompareChar(conv1.ToChar(Nothing), conv2.ToChar(Nothing))

                Case TypeCode.Char * TCMAX + TypeCode.String, _
                     TypeCode.String * TCMAX + TypeCode.Char, _
                     TypeCode.String * TCMAX + TypeCode.String
                    Return CType(CompareString(conv1.ToString(Nothing), conv2.ToString(Nothing), TextCompare), CompareClass)


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return CType(CompareString(conv1.ToString(Nothing), Nothing, TextCompare), CompareClass)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return CompareBoolean(CBool(conv1.ToString(Nothing)), conv2.ToBoolean(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return CompareDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.DateTime
                    Return CompareDate(CDate(conv1.ToString(Nothing)), conv2.ToDateTime(Nothing))

                Case Else
#If 0 Then
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime  'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char      'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime  'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char      'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime  'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char      'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char     'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char     'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
#End If

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return CompareClass.UserDefined
            End If

            Return CompareClass.Undefined

        End Function

        Private Shared Function CompareBoolean(ByVal Left As Boolean, ByVal Right As Boolean) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        Private Shared Function CompareInt32(ByVal Left As Int32, ByVal Right As Int32) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        Private Shared Function CompareUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        Private Shared Function CompareInt64(ByVal Left As Int64, ByVal Right As Int64) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        Private Shared Function CompareUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        'This function takes IConvertible because the JIT does not behave properly with Decimal temps
        'REVIEW VSW#395742: does the JIT now behave properly?
        Private Shared Function CompareDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As CompareClass
            Dim Result As Integer = System.Decimal.Compare(Left.ToDecimal(Nothing), Right.ToDecimal(Nothing))

            'Normalize the result.
            If Result = 0 Then
                Return CompareClass.Equal
            ElseIf Result > 0 Then
                Return CompareClass.Greater
            Else
                Return CompareClass.Less
            End If
        End Function

        Private Shared Function CompareSingle(ByVal Left As Single, ByVal Right As Single) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left < Right Then Return CompareClass.Less
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Unordered
        End Function

        Private Shared Function CompareDouble(ByVal Left As Double, ByVal Right As Double) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left < Right Then Return CompareClass.Less
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Unordered
        End Function

        Private Shared Function CompareDate(ByVal Left As Date, ByVal Right As Date) As CompareClass
            Dim Result As Integer = System.DateTime.Compare(Left, Right)

            'Normalize the result.
            If Result = 0 Then
                Return CompareClass.Equal
            ElseIf Result > 0 Then
                Return CompareClass.Greater
            Else
                Return CompareClass.Less
            End If
        End Function

        Private Shared Function CompareChar(ByVal Left As Char, ByVal Right As Char) As CompareClass
            If Left = Right Then Return CompareClass.Equal
            If Left > Right Then Return CompareClass.Greater
            Return CompareClass.Less
        End Function

        'String comparisons occur often enough that maybe the TextCompare should be broken out into two members that the compiler statically selects
        Public Shared Function CompareString(ByVal Left As String, ByVal Right As String, ByVal TextCompare As Boolean) As Integer
            If Left Is Right Then
                Return CompareClass.Equal
            End If

            If Left Is Nothing Then
                If Right.Length() = 0 Then
                    Return CompareClass.Equal
                End If

                Return CompareClass.Less
            End If

            If Right Is Nothing Then
                If Left.Length() = 0 Then
                    Return CompareClass.Equal
                End If

                Return CompareClass.Greater
            End If

            Dim Result As Integer

            If TextCompare Then
                Result = GetCultureInfo().CompareInfo.Compare(Left, Right, OptionCompareTextFlags)
            Else
                Result = System.String.CompareOrdinal(Left, Right)
            End If

            'Normalize the result.
            If Result = 0 Then
                Return CompareClass.Equal
            ElseIf Result > 0 Then
                Return CompareClass.Greater
            Else
                Return CompareClass.Less
            End If
        End Function

#End Region

#Region " Operator Unary Plus + "

        Public Shared Function PlusObject(ByVal Operand As Object) As Object

            If Operand Is Nothing Then
                Return Boxed_ZeroInteger
            End If

            Dim conv As IConvertible
            Dim typ As TypeCode

            conv = TryCast(Operand, IConvertible)

            If conv Is Nothing Then
                If Operand Is Nothing Then
                    typ = TypeCode.Empty
                Else
                    typ = TypeCode.Object
                End If
            Else
                typ = conv.GetTypeCode()
            End If


            Select Case typ

                Case TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Boolean
                    Return CShort(conv.ToBoolean(Nothing))

                Case TypeCode.SByte
                    Return conv.ToSByte(Nothing)

                Case TypeCode.Byte
                    Return conv.ToByte(Nothing)

                Case TypeCode.Int16
                    Return conv.ToInt16(Nothing)

                Case TypeCode.UInt16
                    Return conv.ToUInt16(Nothing)

                Case TypeCode.Int32
                    Return conv.ToInt32(Nothing)

                Case TypeCode.UInt32
                    Return conv.ToUInt32(Nothing)

                Case TypeCode.Int64
                    Return conv.ToInt64(Nothing)

                Case TypeCode.UInt64
                    Return conv.ToUInt64(Nothing)

                Case TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double
                    Return Operand

                Case TypeCode.DateTime, _
                     TypeCode.Char
                    ' Fall through to error

                Case TypeCode.String
                    Return CDbl(conv.ToString(Nothing))

                Case TypeCode.Object
                    Return InvokeUserDefinedOperator(UserDefinedOperator.UnaryPlus, Operand)

                Case Else
                    ' Fall through to error
            End Select

            Throw GetNoValidOperatorException(UserDefinedOperator.UnaryPlus, Operand)
        End Function

#End Region

#Region " Operator Negate - "

        Public Shared Function NegateObject(ByVal Operand As Object) As Object

            Dim conv As IConvertible
            Dim tc As TypeCode

            conv = TryCast(Operand, IConvertible)

            If conv Is Nothing Then
                If Operand Is Nothing Then
                    tc = TypeCode.Empty
                Else
                    tc = TypeCode.Object
                End If
            Else
                tc = conv.GetTypeCode()
            End If


            Select Case tc

                Case TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Boolean
                    If TypeOf Operand Is Boolean Then
                        Return NegateBoolean(DirectCast(Operand, Boolean))
                    Else
                        Return NegateBoolean(conv.ToBoolean(Nothing))
                    End If

                Case TypeCode.SByte
                    If TypeOf Operand Is SByte Then
                        Return NegateSByte(DirectCast(Operand, SByte))
                    Else
                        Return NegateSByte(conv.ToSByte(Nothing))
                    End If

                Case TypeCode.Byte
                    If TypeOf Operand Is Byte Then
                        Return NegateByte(DirectCast(Operand, Byte))
                    Else
                        Return NegateByte(conv.ToByte(Nothing))
                    End If

                Case TypeCode.Int16
                    If TypeOf Operand Is Int16 Then
                        Return NegateInt16(DirectCast(Operand, Int16))
                    Else
                        Return NegateInt16(conv.ToInt16(Nothing))
                    End If

                Case TypeCode.UInt16
                    If TypeOf Operand Is UInt16 Then
                        Return NegateUInt16(DirectCast(Operand, UInt16))
                    Else
                        Return NegateUInt16(conv.ToUInt16(Nothing))
                    End If

                Case TypeCode.Int32
                    If TypeOf Operand Is Int32 Then
                        Return NegateInt32(DirectCast(Operand, Int32))
                    Else
                        Return NegateInt32(conv.ToInt32(Nothing))
                    End If

                Case TypeCode.UInt32
                    If TypeOf Operand Is UInt32 Then
                        Return NegateUInt32(DirectCast(Operand, UInt32))
                    Else
                        Return NegateUInt32(conv.ToUInt32(Nothing))
                    End If

                Case TypeCode.Int64
                    If TypeOf Operand Is Int64 Then
                        Return NegateInt64(DirectCast(Operand, Int64))
                    Else
                        Return NegateInt64(conv.ToInt64(Nothing))
                    End If

                Case TypeCode.UInt64
                    If TypeOf Operand Is UInt64 Then
                        Return NegateUInt64(DirectCast(Operand, UInt64))
                    Else
                        Return NegateUInt64(conv.ToUInt64(Nothing))
                    End If

                Case TypeCode.Decimal
                    If TypeOf Operand Is Decimal Then
                        Return NegateDecimal(DirectCast(Operand, Decimal))
                    Else
                        Return NegateDecimal(conv.ToDecimal(Nothing))
                    End If

                Case TypeCode.Single
                    If TypeOf Operand Is Single Then
                        Return NegateSingle(DirectCast(Operand, Single))
                    Else
                        Return NegateSingle(conv.ToSingle(Nothing))
                    End If

                Case TypeCode.Double
                    If TypeOf Operand Is Double Then
                        Return NegateDouble(DirectCast(Operand, Double))
                    Else
                        Return NegateDouble(conv.ToDouble(Nothing))
                    End If

                Case TypeCode.DateTime, _
                     TypeCode.Char
                    'Fall through to error.

                Case TypeCode.String
                    Dim StringOperand As String = TryCast(Operand, String)

                    If StringOperand IsNot Nothing Then
                        Return NegateString(StringOperand)
                    Else
                        Return NegateString(conv.ToString(Nothing))
                    End If

                Case TypeCode.Object
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Negate, Operand)

                Case Else
                    'Fall through to error.

            End Select

            Throw GetNoValidOperatorException(UserDefinedOperator.Negate, Operand)

        End Function

        Private Shared Function NegateBoolean(ByVal Operand As Boolean) As Object
            Return -CShort(Operand)
        End Function

        Private Shared Function NegateSByte(ByVal Operand As SByte) As Object
            If Operand = SByte.MinValue Then
                Return -CShort(SByte.MinValue)
            End If
            Return -Operand
        End Function

        Private Shared Function NegateByte(ByVal Operand As Byte) As Object
            Return -CShort(Operand)
        End Function

        Private Shared Function NegateInt16(ByVal Operand As Int16) As Object
            If Operand = Int16.MinValue Then
                Return -CInt(Int16.MinValue)
            End If
            Return -Operand
        End Function

        Private Shared Function NegateUInt16(ByVal Operand As UInt16) As Object
            Return -CInt(Operand)
        End Function

        Private Shared Function NegateInt32(ByVal Operand As Int32) As Object
            If Operand = Int32.MinValue Then
                Return -CLng(Int32.MinValue)
            End If
            Return -Operand
        End Function

        Private Shared Function NegateUInt32(ByVal Operand As UInt32) As Object
            Return -CLng(Operand)
        End Function

        Private Shared Function NegateInt64(ByVal Operand As Int64) As Object
            If Operand = Int64.MinValue Then
                Return -CDec(Int64.MinValue)
            End If
            Return -Operand
        End Function

        Private Shared Function NegateUInt64(ByVal Operand As UInt64) As Object
            Return -CDec(Operand)
        End Function

        Private Shared Function NegateDecimal(ByVal Operand As Decimal) As Object
            'Using try/catch instead of check with MinValue since the overflow case should be very rare
            'and a compare would be a big cost for the normal case.
            Try
                Return -Operand
            Catch ex As OverflowException
                Return -CDbl(Operand)
            End Try
        End Function

        Private Shared Function NegateSingle(ByVal Operand As Single) As Object
            Return -Operand
        End Function

        Private Shared Function NegateDouble(ByVal Operand As Double) As Object
            Return -Operand
        End Function

        Private Shared Function NegateString(ByVal Operand As String) As Object
            Return -CDbl(Operand)
        End Function

#End Region

#Region " Operator Not "

        'UNDONE UNSIGNED - is it faster to do it the old way, with local vars of the specific types that get returned?
        Public Shared Function NotObject(ByVal Operand As Object) As Object

            Dim conv As IConvertible
            Dim tc As TypeCode

            conv = TryCast(Operand, IConvertible)

            If conv Is Nothing Then
                If Operand Is Nothing Then
                    tc = TypeCode.Empty
                Else
                    tc = TypeCode.Object
                End If
            Else
                tc = conv.GetTypeCode()
            End If


            Select Case tc

                Case TypeCode.Empty
                    Return Not 0I

                Case TypeCode.Boolean
                    Return NotBoolean(conv.ToBoolean(Nothing))

                Case TypeCode.SByte
                    Return NotSByte(conv.ToSByte(Nothing), Operand.GetType())

                Case TypeCode.Byte
                    Return NotByte(conv.ToByte(Nothing), Operand.GetType())

                Case TypeCode.Int16
                    Return NotInt16(conv.ToInt16(Nothing), Operand.GetType())

                Case TypeCode.UInt16
                    Return NotUInt16(conv.ToUInt16(Nothing), Operand.GetType())

                Case TypeCode.Int32
                    Return NotInt32(conv.ToInt32(Nothing), Operand.GetType())

                Case TypeCode.UInt32
                    Return NotUInt32(conv.ToUInt32(Nothing), Operand.GetType())

                Case TypeCode.Int64
                    Return NotInt64(conv.ToInt64(Nothing), Operand.GetType())

                Case TypeCode.UInt64
                    Return NotUInt64(conv.ToUInt64(Nothing), Operand.GetType())

                Case TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double
                    Return NotInt64(conv.ToInt64(Nothing))

                Case TypeCode.DateTime, _
                     TypeCode.Char
                    'Fall through to error.

                Case TypeCode.String
                    Return NotInt64(CLng(conv.ToString(Nothing)))

                Case TypeCode.Object
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Not, Operand)

                Case Else
                    'Fall through to error.

            End Select

            Throw GetNoValidOperatorException(UserDefinedOperator.Not, Operand)

        End Function

        Private Shared Function NotBoolean(ByVal Operand As Boolean) As Object
            Return Not Operand
        End Function

        Private Shared Function NotSByte(ByVal Operand As SByte, ByVal OperandType As Type) As Object
            Dim Result As SByte = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotByte(ByVal Operand As Byte, ByVal OperandType As Type) As Object
            Dim Result As Byte = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotInt16(ByVal Operand As Int16, ByVal OperandType As Type) As Object
            Dim Result As Int16 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotUInt16(ByVal Operand As UInt16, ByVal OperandType As Type) As Object
            Dim Result As UInt16 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotInt32(ByVal Operand As Int32, ByVal OperandType As Type) As Object
            Dim Result As Int32 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotUInt32(ByVal Operand As UInt32, ByVal OperandType As Type) As Object
            Dim Result As UInt32 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotInt64(ByVal Operand As Int64) As Object
            Return Not Operand
        End Function

        Private Shared Function NotInt64(ByVal Operand As Int64, ByVal OperandType As Type) As Object
            Dim Result As Int64 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

        Private Shared Function NotUInt64(ByVal Operand As UInt64, ByVal OperandType As Type) As Object
            Dim Result As UInt64 = Not Operand

            If OperandType.IsEnum Then
                Return System.Enum.ToObject(OperandType, Result)
            End If
            Return Result
        End Function

#End Region

#Region " Operator And "

        Public Shared Function AndObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2  'CONSIDER: overflow checking is not necessary for this calculation - perf improvement.

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean, _
                     TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return False

                Case TypeCode.Empty * TCMAX + TypeCode.SByte, _
                     TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return AndSByte(CSByte(0), CSByte(0), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte, _
                     TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return AndByte(CByte(0), CByte(0), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return AndInt16(0S, 0S, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return AndUInt16(0US, 0US, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return AndInt32(0I, 0I, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return AndUInt32(0UI, 0UI, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return AndInt64(0L, 0L, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return AndUInt64(0UL, 0UL, GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Empty * TCMAX + TypeCode.Double
                    Return AndInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return AndInt64(Nothing, CLng(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return AndBoolean(conv1.ToBoolean(Nothing), conv2.ToBoolean(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return AndSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return AndInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return AndInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal, _
                     TypeCode.Boolean * TCMAX + TypeCode.Single, _
                     TypeCode.Boolean * TCMAX + TypeCode.Double

                    Return AndInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))  'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return AndBoolean(conv1.ToBoolean(Nothing), CBool(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return AndSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return AndSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte

                    Return AndInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16

                    Return AndInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return AndInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return AndInt64(conv1.ToInt64(Nothing), CLng(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return AndInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return AndByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte
                    Return AndUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16

                    Return AndUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32

                    Return AndUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Int16
                    Return AndInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return AndInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return AndUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int32 * TCMAX + TypeCode.Int32
                    Return AndInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean, _
                     TypeCode.Single * TCMAX + TypeCode.Boolean, _
                     TypeCode.Double * TCMAX + TypeCode.Boolean

                    Return AndInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return AndUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int64 * TCMAX + TypeCode.Int64
                    Return AndInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing), GetEnumResult(Left, Right))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.


                Case TypeCode.UInt64 * TCMAX + TypeCode.UInt64
                    Return AndUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return AndInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return AndInt64(CLng(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return AndBoolean(CBool(conv1.ToString(Nothing)), conv2.ToBoolean(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return AndInt64(CLng(conv1.ToString(Nothing)), conv2.ToInt64(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return AndInt64(CLng(conv1.ToString(Nothing)), CLng(conv2.ToString(Nothing)))

#If 0 Then
                'ERROR CASES
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If
            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.And, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.And, Left, Right)

        End Function

        Private Shared Function AndBoolean(ByVal Left As Boolean, ByVal Right As Boolean) As Object
            Return Left And Right
        End Function

        Private Shared Function AndSByte(ByVal Left As SByte, ByVal Right As SByte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As SByte = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndByte(ByVal Left As Byte, ByVal Right As Byte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Byte = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndInt16(ByVal Left As Int16, ByVal Right As Int16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int16 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndUInt16(ByVal Left As UInt16, ByVal Right As UInt16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt16 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndInt32(ByVal Left As Int32, ByVal Right As Int32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int32 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndUInt32(ByVal Left As UInt32, ByVal Right As UInt32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt32 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndInt64(ByVal Left As Int64, ByVal Right As Int64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int64 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function AndUInt64(ByVal Left As UInt64, ByVal Right As UInt64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt64 = Left And Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

#End Region

#Region " Operator Or "

        Public Shared Function OrObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2  'CONSIDER: overflow checking is not necessary for this calculation - perf improvement.

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return OrBoolean(Nothing, conv2.ToBoolean(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte, _
                     TypeCode.Empty * TCMAX + TypeCode.Byte, _
                     TypeCode.Empty * TCMAX + TypeCode.Int16, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt16, _
                     TypeCode.Empty * TCMAX + TypeCode.Int32, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt32, _
                     TypeCode.Empty * TCMAX + TypeCode.Int64, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt64

                    Return Right

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Empty * TCMAX + TypeCode.Double
                    Return OrInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return OrInt64(Nothing, CLng(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return OrBoolean(conv1.ToBoolean(Nothing), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return OrBoolean(conv1.ToBoolean(Nothing), conv2.ToBoolean(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return OrSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return OrInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return OrInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal, _
                     TypeCode.Boolean * TCMAX + TypeCode.Single, _
                     TypeCode.Boolean * TCMAX + TypeCode.Double

                    Return OrInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))  'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return OrBoolean(conv1.ToBoolean(Nothing), CBool(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty, _
                     TypeCode.Byte * TCMAX + TypeCode.Empty, _
                     TypeCode.Int16 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Empty, _
                     TypeCode.Int32 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Empty, _
                     TypeCode.Int64 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Empty

                    Return Left

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return OrSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return OrSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte

                    Return OrInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16

                    Return OrInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return OrInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return OrInt64(conv1.ToInt64(Nothing), CLng(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return OrInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return OrByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte
                    Return OrUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16

                    Return OrUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32

                    Return OrUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Int16
                    Return OrInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return OrInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return OrUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int32 * TCMAX + TypeCode.Int32
                    Return OrInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean, _
                     TypeCode.Single * TCMAX + TypeCode.Boolean, _
                     TypeCode.Double * TCMAX + TypeCode.Boolean

                    Return OrInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return OrUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int64 * TCMAX + TypeCode.Int64
                    Return OrInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing), GetEnumResult(Left, Right))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.


                Case TypeCode.UInt64 * TCMAX + TypeCode.UInt64
                    Return OrUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return OrInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return OrInt64(CLng(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return OrBoolean(CBool(conv1.ToString(Nothing)), conv2.ToBoolean(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return OrInt64(CLng(conv1.ToString(Nothing)), conv2.ToInt64(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return OrInt64(CLng(conv1.ToString(Nothing)), CLng(conv2.ToString(Nothing)))

#If 0 Then
                'ERROR CASES
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If
            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Or, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Or, Left, Right)

        End Function

        Private Shared Function OrBoolean(ByVal Left As Boolean, ByVal Right As Boolean) As Object
            Return Left Or Right
        End Function

        Private Shared Function OrSByte(ByVal Left As SByte, ByVal Right As SByte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As SByte = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrByte(ByVal Left As Byte, ByVal Right As Byte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Byte = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrInt16(ByVal Left As Int16, ByVal Right As Int16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int16 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrUInt16(ByVal Left As UInt16, ByVal Right As UInt16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt16 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrInt32(ByVal Left As Int32, ByVal Right As Int32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int32 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrUInt32(ByVal Left As UInt32, ByVal Right As UInt32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt32 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrInt64(ByVal Left As Int64, ByVal Right As Int64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int64 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function OrUInt64(ByVal Left As UInt64, ByVal Right As UInt64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt64 = Left Or Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

#End Region

#Region " Operator Xor "

        Public Shared Function XorObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2  'CONSIDER: overflow checking is not necessary for this calculation - perf improvement.

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return XorBoolean(Nothing, conv2.ToBoolean(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return XorSByte(Nothing, conv2.ToSByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return XorByte(Nothing, conv2.ToByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return XorInt16(Nothing, conv2.ToInt16(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return XorUInt16(Nothing, conv2.ToUInt16(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return XorInt32(Nothing, conv2.ToInt32(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return XorUInt32(Nothing, conv2.ToUInt32(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return XorInt64(Nothing, conv2.ToInt64(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return XorUInt64(Nothing, conv2.ToUInt64(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Empty * TCMAX + TypeCode.Double
                    Return XorInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return XorInt64(Nothing, CLng(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return XorBoolean(conv1.ToBoolean(Nothing), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return XorBoolean(conv1.ToBoolean(Nothing), conv2.ToBoolean(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return XorSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return XorInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return XorInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal, _
                     TypeCode.Boolean * TCMAX + TypeCode.Single, _
                     TypeCode.Boolean * TCMAX + TypeCode.Double

                    Return XorInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))  'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return XorBoolean(conv1.ToBoolean(Nothing), CBool(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return XorSByte(conv1.ToSByte(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return XorSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return XorSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte

                    Return XorInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16

                    Return XorInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return XorInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return XorInt64(conv1.ToInt64(Nothing), CLng(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return XorByte(conv1.ToByte(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return XorInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return XorByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing), GetEnumResult(Left, Right))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte
                    Return XorUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16

                    Return XorUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32

                    Return XorUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return XorInt16(conv1.ToInt16(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.Int16 * TCMAX + TypeCode.Int16
                    Return XorInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return XorUInt16(conv1.ToUInt16(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return XorInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return XorUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return XorInt32(conv1.ToInt32(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.Int32 * TCMAX + TypeCode.Int32
                    Return XorInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return XorUInt32(conv1.ToUInt32(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean, _
                     TypeCode.Single * TCMAX + TypeCode.Boolean, _
                     TypeCode.Double * TCMAX + TypeCode.Boolean

                    Return XorInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))

                Case TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return XorUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return XorInt64(conv1.ToInt64(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.Int64 * TCMAX + TypeCode.Int64
                    Return XorInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing), GetEnumResult(Left, Right))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return XorUInt64(conv1.ToUInt64(Nothing), Nothing, GetEnumResult(Left, Right))

                Case TypeCode.UInt64 * TCMAX + TypeCode.UInt64
                    Return XorUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing), GetEnumResult(Left, Right))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return XorInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return XorInt64(CLng(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return XorBoolean(CBool(conv1.ToString(Nothing)), conv2.ToBoolean(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return XorInt64(CLng(conv1.ToString(Nothing)), conv2.ToInt64(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return XorInt64(CLng(conv1.ToString(Nothing)), CLng(conv2.ToString(Nothing)))

#If 0 Then
                'ERROR CASES
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If
            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Xor, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Xor, Left, Right)

        End Function

        Private Shared Function XorBoolean(ByVal Left As Boolean, ByVal Right As Boolean) As Object
            Return Left Xor Right
        End Function

        Private Shared Function XorSByte(ByVal Left As SByte, ByVal Right As SByte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As SByte = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorByte(ByVal Left As Byte, ByVal Right As Byte, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Byte = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorInt16(ByVal Left As Int16, ByVal Right As Int16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int16 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorUInt16(ByVal Left As UInt16, ByVal Right As UInt16, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt16 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorInt32(ByVal Left As Int32, ByVal Right As Int32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int32 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorUInt32(ByVal Left As UInt32, ByVal Right As UInt32, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt32 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorInt64(ByVal Left As Int64, ByVal Right As Int64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As Int64 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

        Private Shared Function XorUInt64(ByVal Left As UInt64, ByVal Right As UInt64, Optional ByVal EnumType As Type = Nothing) As Object
            Dim Result As UInt64 = Left Xor Right

            If EnumType IsNot Nothing Then Return System.Enum.ToObject(EnumType, Result)
            Return Result
        End Function

#End Region

#Region " Operator Plus + "

        Public Shared Function AddObject(ByVal Left As Object, ByVal Right As Object) As Object
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'Special cases for Char()
            If tc1 = TypeCode.Object Then
                Dim LeftCharArray As Char() = TryCast(Left, Char())

                If LeftCharArray IsNot Nothing Then
                    If tc2 = TypeCode.String OrElse tc2 = TypeCode.Empty OrElse ((tc2 = TypeCode.Object) AndAlso (TypeOf Right Is Char())) Then
                        'Treat Char() as String for these cases
                        Left = CStr(LeftCharArray)
                        conv1 = CType(Left, IConvertible)
                        tc1 = TypeCode.String
                    End If
                End If
            End If

            If (tc2 = TypeCode.Object) Then
                Dim RightCharArray As Char() = TryCast(Right, Char())

                If RightCharArray IsNot Nothing Then
                    If tc1 = TypeCode.String OrElse tc1 = TypeCode.Empty Then
                        Right = CStr(RightCharArray)
                        conv2 = DirectCast(Right, IConvertible)
                        tc2 = TypeCode.String
                    End If
                End If
            End If


            'UNDONE : CONVERSION FROM SINGLE AND DOUBLE MUST DO ROUNDING!!
            Select Case tc1 * TCMAX + tc2

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return AddInt16(Nothing, ToVBBool(conv2))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return conv2.ToSByte(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return conv2.ToByte(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return conv2.ToInt16(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return conv2.ToUInt16(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return conv2.ToInt32(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return conv2.ToUInt32(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return conv2.ToInt64(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return conv2.ToUInt64(Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Empty * TCMAX + TypeCode.Double, _
                     TypeCode.Empty * TCMAX + TypeCode.String, _
                     TypeCode.DBNull * TCMAX + TypeCode.String

                    Return Right

                Case TypeCode.Empty * TCMAX + TypeCode.DateTime
                    Return AddString(CStr(CDate(Nothing)), CStr(conv2.ToDateTime(Nothing)))

                Case TypeCode.Empty * TCMAX + TypeCode.Char
                    Return AddString(ControlChars.NullChar, conv2.ToString(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return AddInt16(ToVBBool(conv1), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return AddInt16(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return AddSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return AddInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return AddInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64
                    Return AddInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return AddDecimal(ToVBBoolConv(conv1), conv2.ToDecimal(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return AddSingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return AddDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return AddDouble(ToVBBool(conv1), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return conv1.ToSByte(Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return AddSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return AddSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return AddInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return AddInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64

                    Return AddInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal

                    Return AddDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single

                    Return AddSingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return AddDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return AddDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return conv1.ToByte(Nothing)

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return AddInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return AddByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return AddUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return AddUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return AddUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt16(Nothing)


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt16(Nothing)

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return AddInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt32(Nothing)


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt32(Nothing)

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean
                    Return AddInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt64(Nothing)


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt64(Nothing)

                Case TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return AddDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty, _
                     TypeCode.String * TCMAX + TypeCode.Empty, _
                     TypeCode.String * TCMAX + TypeCode.DBNull

                    Return Left


                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return AddSingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return AddDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))


                Case TypeCode.DateTime * TCMAX + TypeCode.Empty
                    Return AddString(CStr(conv1.ToDateTime(Nothing)), CStr(CDate(Nothing)))

                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime
                    Return AddString(CStr(conv1.ToDateTime(Nothing)), CStr(conv2.ToDateTime(Nothing)))

                Case TypeCode.DateTime * TCMAX + TypeCode.String
                    Return AddString(CStr(conv1.ToDateTime(Nothing)), conv2.ToString(Nothing))


                Case TypeCode.Char * TCMAX + TypeCode.Empty
                    Return AddString(conv1.ToString(Nothing), ControlChars.NullChar)

                Case TypeCode.Char * TCMAX + TypeCode.Char, _
                     TypeCode.Char * TCMAX + TypeCode.String, _
                     TypeCode.String * TCMAX + TypeCode.Char
                    Return AddString(conv1.ToString(Nothing), conv2.ToString(Nothing))


                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return AddDouble(CDbl(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return AddDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.DateTime
                    Return AddString(conv1.ToString(Nothing), CStr(conv2.ToDateTime(Nothing)))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return AddString(conv1.ToString(Nothing), conv2.ToString(Nothing))


#If 0 Then
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
#End If

                Case Else

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Plus, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Plus, Left, Right)

        End Function

        Private Shared Function AddByte(ByVal Left As Byte, ByVal Right As Byte) As Object
            'Range of possible values:  [0, 510]
            Dim Result As Int16 = CShort(Left) + CShort(Right)

            If Result > Byte.MaxValue Then
                Return Result
            Else
                Return CByte(Result) 'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
            End If
        End Function

        Private Shared Function AddSByte(ByVal Left As SByte, ByVal Right As SByte) As Object
            'Range of possible values:  [-256, 254]
            Dim Result As Int16 = CShort(Left) + CShort(Right)

            If Result > SByte.MaxValue OrElse Result < SByte.MinValue Then
                Return Result
            Else
                Return CSByte(Result) 'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
            End If
        End Function

        Private Shared Function AddInt16(ByVal Left As Int16, ByVal Right As Int16) As Object
            'Range of possible values:  [-65536, 65534]
            Dim Result As Int32 = CInt(Left) + CInt(Right)

            If Result > Int16.MaxValue OrElse Result < Int16.MinValue Then
                Return Result
            Else
                Return CShort(Result) 'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
            End If
        End Function

        Private Shared Function AddUInt16(ByVal Left As UInt16, ByVal Right As UInt16) As Object
            'Range of possible values:  [0, 131070]
            Dim Result As Int32 = CInt(Left) + CInt(Right)

            If Result > UInt16.MaxValue Then
                Return Result
            Else
                Return CUShort(Result) 'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
            End If
        End Function

        Private Shared Function AddInt32(ByVal Left As Int32, ByVal Right As Int32) As Object
            'Range of possible values:  [-4294967296, 4294967294]
            Dim Result As Int64 = CLng(Left) + CLng(Right)

            If Result > Int32.MaxValue OrElse Result < Int32.MinValue Then
                Return Result
            Else
                Return CInt(Result)
            End If
        End Function

        Private Shared Function AddUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As Object
            'Range of possible values:  [0, 8589934590]
            Dim Result As Int64 = CLng(Left) + CLng(Right)

            If Result > UInt32.MaxValue Then
                Return Result
            Else
                Return CUInt(Result)
            End If
        End Function

        Private Shared Function AddInt64(ByVal Left As Int64, ByVal Right As Int64) As Object
            'Range of possible values:  [-18446744073709551616, 18446744073709551614]
            Try
                Return Left + Right
            Catch e As OverflowException
                Return CDec(Left) + CDec(Right)
            End Try

#If 0 Then  'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
            'this function could be rewritten to check the roundtrip and not have to do unecessary decimal addition.
            Dim Result As Decimal = CDec(Left) + CDec(Right)

            If Result > Int64.MaxValue OrElse Result < Int64.MinValue Then
                Return Result
            Else
                Return CLng(Result)
            End If
#End If 'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
        End Function

        Private Shared Function AddUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As Object
            'Range of possible values:  [0, 36893488147419103230]
            Try
                Return Left + Right
            Catch e As OverflowException
                Return CDec(Left) + CDec(Right)
            End Try

#If 0 Then  'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
            'this function could be rewritten to check the roundtrip and not have to do unecessary decimal addition.
            Dim Result As Decimal = CDec(Left) + CDec(Right)

            If Result > UInt64.MaxValue Then
                Return Result
            Else
                Return CULng(Result)
            End If
#End If 'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
        End Function

        Private Shared Function AddDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As Object
            'REVIWE VSW#395758: there must be a better way to do this.  If not, ask for one.
            Dim LeftValue As Decimal = Left.ToDecimal(Nothing)
            Dim RightValue As Decimal = Right.ToDecimal(Nothing)

            Try
                Return LeftValue + RightValue
            Catch ex As OverflowException
                Return CDbl(LeftValue) + CDbl(RightValue)
            End Try
        End Function

        Private Shared Function AddSingle(ByVal Left As Single, ByVal Right As Single) As Object
            Dim Result As Double = CDbl(Left) + CDbl(Right)

            If ((Result <= Single.MaxValue AndAlso Result >= Single.MinValue)) Then
                Return CSng(Result)
            ElseIf Double.IsInfinity(Result) AndAlso (Single.IsInfinity(Left) OrElse Single.IsInfinity(Right)) Then
                Return CSng(Result)
            Else
                Return Result
            End If
        End Function

        Private Shared Function AddDouble(ByVal Left As Double, ByVal Right As Double) As Object
            Return Left + Right
        End Function

        Private Shared Function AddString(ByVal Left As String, ByVal Right As String) As Object
            Return Left & Right
        End Function

#End Region

#Region " Operator Minus - "

        Public Shared Function SubtractObject(ByVal Left As Object, ByVal Right As Object) As Object
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return SubtractInt16(Nothing, ToVBBool(conv2))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return SubtractSByte(Nothing, conv2.ToSByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return SubtractByte(Nothing, conv2.ToByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return SubtractInt16(Nothing, conv2.ToInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return SubtractUInt16(Nothing, conv2.ToUInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return SubtractInt32(Nothing, conv2.ToInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return SubtractUInt32(Nothing, conv2.ToUInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return SubtractInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return SubtractUInt64(Nothing, conv2.ToUInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal
                    Return SubtractDecimal(0D, conv2)

                Case TypeCode.Empty * TCMAX + TypeCode.Single
                    Return SubtractSingle(Nothing, conv2.ToSingle(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Double
                    Return SubtractDouble(Nothing, conv2.ToDouble(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return SubtractDouble(Nothing, CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return SubtractInt16(ToVBBool(conv1), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return SubtractInt16(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return SubtractSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return SubtractInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return SubtractInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64
                    Return SubtractInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return SubtractDecimal(ToVBBoolConv(conv1), conv2.ToDecimal(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return SubtractSingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return SubtractDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return SubtractDouble(ToVBBool(conv1), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return conv1.ToSByte(Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return SubtractSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return SubtractSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return SubtractInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return SubtractInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64

                    Return SubtractInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal

                    Return SubtractDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single

                    Return SubtractSingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return SubtractDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return SubtractDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return conv1.ToByte(Nothing)

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return SubtractInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return SubtractByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return SubtractUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return SubtractUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return SubtractUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt16(Nothing)


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt16(Nothing)

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return SubtractInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt32(Nothing)


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt32(Nothing)

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean
                    Return SubtractInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return conv1.ToInt64(Nothing)


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return conv1.ToUInt64(Nothing)

                Case TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return SubtractDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return Left


                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return SubtractSingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return SubtractDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return CDbl(conv1.ToString(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return SubtractDouble(CDbl(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return SubtractDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return SubtractDouble(CDbl(conv1.ToString(Nothing)), CDbl(conv2.ToString(Nothing)))


#If 0 Then
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If

                Case Else

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object OrElse _
               (tc1 = TypeCode.DateTime AndAlso tc2 = TypeCode.DateTime) OrElse _
               (tc1 = TypeCode.DateTime AndAlso tc2 = TypeCode.Empty) OrElse _
               (tc1 = TypeCode.Empty AndAlso tc2 = TypeCode.DateTime) Then

                Return InvokeUserDefinedOperator(UserDefinedOperator.Minus, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Minus, Left, Right)

        End Function

        Private Shared Function SubtractByte(ByVal Left As Byte, ByVal Right As Byte) As Object
            'Range of possible values:  [-255, 255]
            Dim Result As Int16 = CShort(Left) - CShort(Right)

            If Result < Byte.MinValue Then
                Return Result
            Else
                Return CByte(Result)  'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
            End If
        End Function

        Private Shared Function SubtractSByte(ByVal Left As SByte, ByVal Right As SByte) As Object
            'Range of possible values:  [-255, 255]
            Dim Result As Int16 = CShort(Left) - CShort(Right)

            If Result < SByte.MinValue OrElse Result > SByte.MaxValue Then
                Return Result
            Else
                Return CSByte(Result)
            End If
        End Function

        Private Shared Function SubtractInt16(ByVal Left As Int16, ByVal Right As Int16) As Object
            'Range of possible values:  [-65535, 65535]
            Dim Result As Int32 = CInt(Left) - CInt(Right)

            If Result < Int16.MinValue OrElse Result > Int16.MaxValue Then
                Return Result
            Else
                Return CShort(Result)
            End If
        End Function

        Private Shared Function SubtractUInt16(ByVal Left As UInt16, ByVal Right As UInt16) As Object
            'Range of possible values:  [-65535, 65535]
            Dim Result As Int32 = CInt(Left) - CInt(Right)

            If Result < UInt16.MinValue Then
                Return Result
            Else
                Return CUShort(Result)
            End If
        End Function

        Private Shared Function SubtractInt32(ByVal Left As Int32, ByVal Right As Int32) As Object
            'Range of possible values:  [-4294967295, 4294967295]
            Dim Result As Int64 = CLng(Left) - CLng(Right)

            If Result < Int32.MinValue OrElse Result > Int32.MaxValue Then
                Return Result
            Else
                Return CInt(Result)
            End If
        End Function

        Private Shared Function SubtractUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As Object
            'Range of possible values:  [-4294967295, 4294967295]
            Dim Result As Int64 = CLng(Left) - CLng(Right)

            If Result < UInt32.MinValue Then
                Return Result
            Else
                Return CUInt(Result)
            End If
        End Function

        Private Shared Function SubtractInt64(ByVal Left As Int64, ByVal Right As Int64) As Object
            'Range of possible values:  [-18446744073709551615, 18446744073709551615]
            Try
                Return Left - Right
            Catch ex As OverflowException
                Return CDec(Left) - CDec(Right)
            End Try

#If 0 Then  'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
            'this function could be rewritten to check the roundtrip and not have to do unecessary decimal subtraction.
            Dim Result As Decimal = CDec(Left) - CDec(Right)

            If Result < Int64.MinValue OrElse Result > Int64.MaxValue Then
                Return Result
            Else
                Return CLng(Result)
            End If
#End If 'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
        End Function

        Private Shared Function SubtractUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As Object
            'Range of possible values:  [-18446744073709551615, 18446744073709551615]
            Try
                Return Left - Right
            Catch ex As OverflowException
                Return CDec(Left) - CDec(Right)
            End Try

#If 0 Then  'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
            'this function could be rewritten to check the roundtrip and not have to do unecessary decimal subtraction.
            Dim Result As Decimal = CDec(Left) - CDec(Right)

            If Result < UInt64.MinValue Then
                Return Result
            Else
                Return CULng(Result)
            End If
#End If 'REVIEW VSW#395757: which implementation is better?  If only we could turn off overflow checking on a per-block basis, then
        End Function

        Private Shared Function SubtractDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As Object
            'REVIEW VSW#395758: there must be a better way to do this.  If not, ask for one.
            Dim LeftValue As Decimal = Left.ToDecimal(Nothing)
            Dim RightValue As Decimal = Right.ToDecimal(Nothing)

            Try
                Return LeftValue - RightValue
            Catch ex As OverflowException
                Return CDbl(LeftValue) - CDbl(RightValue)
            End Try
        End Function

        Private Shared Function SubtractSingle(ByVal Left As Single, ByVal Right As Single) As Object
            Dim Result As Double = CDbl(Left) - CDbl(Right)

            If ((Result <= Single.MaxValue AndAlso Result >= Single.MinValue)) Then
                Return CSng(Result)
            ElseIf Double.IsInfinity(Result) AndAlso (Single.IsInfinity(Left) OrElse Single.IsInfinity(Right)) Then
                Return CSng(Result)
            Else
                Return Result
            End If
        End Function

        Private Shared Function SubtractDouble(ByVal Left As Double, ByVal Right As Double) As Object
            Return Left - Right
        End Function

#End Region

#Region " Operator Multiply * "

        Public Shared Function MultiplyObject(ByVal Left As Object, ByVal Right As Object) As Object
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode


            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If



            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'REVIEW VSW#395755: make shared members that represent pre-boxed zero values for each type.

            Select Case tc1 * TCMAX + tc2

                Case TypeCode.Empty * TCMAX + TypeCode.Empty, _
                     TypeCode.Empty * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean, _
                     TypeCode.Boolean * TCMAX + TypeCode.Empty, _
                     TypeCode.Empty * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroShort

                Case TypeCode.Empty * TCMAX + TypeCode.SByte, _
                     TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroSByte

                Case TypeCode.Empty * TCMAX + TypeCode.Byte, _
                     TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroByte

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroUShort

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroUInteger

                Case TypeCode.Empty * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroLong

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroULong

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroDecimal

                Case TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroSinge

                Case TypeCode.Empty * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return Boxed_ZeroDouble

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return MultiplyDouble(Nothing, CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return MultiplyInt16(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return MultiplySByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return MultiplyInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return MultiplyInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64
                    Return MultiplyInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return MultiplyDecimal(ToVBBoolConv(conv1), conv2.ToDecimal(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return MultiplySingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return MultiplyDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return MultiplyDouble(ToVBBool(conv1), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return MultiplySByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return MultiplySByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return MultiplyInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return MultiplyInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64

                    Return MultiplyInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal

                    Return MultiplyDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single

                    Return MultiplySingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return MultiplyDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return MultiplyDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return MultiplyInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return MultiplyByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return MultiplyUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return MultiplyUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return MultiplyUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return MultiplyInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean
                    Return MultiplyInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return MultiplyDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return MultiplySingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return MultiplyDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return MultiplyDouble(CDbl(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return MultiplyDouble(CDbl(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return MultiplyDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return MultiplyDouble(CDbl(conv1.ToString(Nothing)), CDbl(conv2.ToString(Nothing)))


#If 0 Then
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                case TypeCode.String * TCMAX + TypeCode.Char 'XX

#End If

                Case Else

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Multiply, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Multiply, Left, Right)

        End Function

        Private Shared Function MultiplyByte(ByVal Left As Byte, ByVal Right As Byte) As Object
            'Range of possible values:  [0, 65025]
            Dim Result As Int32 = CInt(Left) * CInt(Right)

            If Result > Byte.MaxValue Then
                If Result > Int16.MaxValue Then
                    Return Result
                Else
                    Return CShort(Result) 'REVIEW VSW#395757: overflow checking can be turned off here, and other similar places, since we've already checked for overflow.
                End If
            Else
                Return CByte(Result)
            End If
        End Function

        Private Shared Function MultiplySByte(ByVal Left As SByte, ByVal Right As SByte) As Object
            'Range of possible values:  [-16256 ,16384]
            Dim Result As Int16 = CShort(Left) * CShort(Right)

            If Result > SByte.MaxValue OrElse Result < SByte.MinValue Then
                Return Result
            Else
                Return CSByte(Result)
            End If
        End Function

        Private Shared Function MultiplyInt16(ByVal Left As Int16, ByVal Right As Int16) As Object
            'Range of possible values:  [-1073709056, 1073741824]
            Dim Result As Int32 = CInt(Left) * CInt(Right)

            If Result > Int16.MaxValue OrElse Result < Int16.MinValue Then
                Return Result
            Else
                Return CShort(Result)
            End If
        End Function

        Private Shared Function MultiplyUInt16(ByVal Left As UInt16, ByVal Right As UInt16) As Object
            'Range of possible values:  [0, 4294836225]
            Dim Result As Int64 = CLng(Left) * CLng(Right)

            If Result > UInt16.MaxValue Then
                If Result > Int32.MaxValue Then
                    Return Result
                Else
                    Return CInt(Result)
                End If
            Else
                Return CUShort(Result)
            End If
        End Function

        Private Shared Function MultiplyInt32(ByVal Left As Int32, ByVal Right As Int32) As Object
            'Range of possible values:  [-4611686016279904256, 4611686018427387904]
            Dim Result As Int64 = CLng(Left) * CLng(Right)

            If Result > Int32.MaxValue OrElse Result < Int32.MinValue Then
                Return Result
            Else
                Return CInt(Result)
            End If
        End Function

        Private Shared Function MultiplyUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As Object
            'Range of possible values:  [0, 18446744065119617025]
            Dim Result As UInt64 = CULng(Left) * CULng(Right)

            If Result > UInt32.MaxValue Then
                If Result > Int64.MaxValue Then
                    Return CDec(Result)
                Else
                    Return CLng(Result)
                End If
            Else
                Return CUInt(Result)
            End If
        End Function

        Private Shared Function MultiplyInt64(ByVal Left As Int64, ByVal Right As Int64) As Object
            'CONSIDER VSW#395757: isn't there a better way to do this?
            Try
                Return Left * Right
            Catch ex As OverflowException
            End Try

            Try
                Return CDec(Left) * CDec(Right)
            Catch ex As OverflowException
                Return CDbl(Left) * CDbl(Right)
            End Try
        End Function

        Private Shared Function MultiplyUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As Object
            ''CONSIDER VSW#395757: isn't there a better way to do this?
            Try
                Return Left * Right
            Catch ex As OverflowException
            End Try

            Try
                Return CDec(Left) * CDec(Right)
            Catch ex As OverflowException
                Return CDbl(Left) * CDbl(Right)
            End Try
        End Function

        Private Shared Function MultiplyDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As Object
            'REVIEW VSW#395758: there must be a better way to do this.  If not, ask for one.
            Dim LeftValue As Decimal = Left.ToDecimal(Nothing)
            Dim RightValue As Decimal = Right.ToDecimal(Nothing)

            Try
                Return LeftValue * RightValue
            Catch ex As OverflowException
                'Converting to Double is inconsistent with Division, where we convert to Single.
                Return CDbl(LeftValue) * CDbl(RightValue)
            End Try
        End Function

        Private Shared Function MultiplySingle(ByVal Left As Single, ByVal Right As Single) As Object
            Dim Result As Double = CDbl(Left) * CDbl(Right)

            If ((Result <= Single.MaxValue AndAlso Result >= Single.MinValue)) Then
                Return CSng(Result)
            ElseIf Double.IsInfinity(Result) AndAlso (Single.IsInfinity(Left) OrElse Single.IsInfinity(Right)) Then
                Return CSng(Result)
            Else
                Return Result
            End If
        End Function

        Private Shared Function MultiplyDouble(ByVal Left As Double, ByVal Right As Double) As Object
            Return Left * Right
        End Function

#End Region

#Region " Operator Divide / "

        Public Shared Function DivideObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode


            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return DivideDouble(Nothing, Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return DivideDouble(Nothing, ToVBBool(conv2))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte, _
                     TypeCode.Empty * TCMAX + TypeCode.Byte, _
                     TypeCode.Empty * TCMAX + TypeCode.Int16, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt16, _
                     TypeCode.Empty * TCMAX + TypeCode.Int32, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt32, _
                     TypeCode.Empty * TCMAX + TypeCode.Int64, _
                     TypeCode.Empty * TCMAX + TypeCode.UInt64, _
                     TypeCode.Empty * TCMAX + TypeCode.Double
                    Return DivideDouble(Nothing, conv2.ToDouble(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal
                    Return DivideDecimal(0D, conv2)

                Case TypeCode.Empty * TCMAX + TypeCode.Single
                    Return DivideSingle(Nothing, conv2.ToSingle(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return DivideDouble(Nothing, CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return DivideDouble(ToVBBool(conv1), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return DivideDouble(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return DivideDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return DivideDecimal(ToVBBoolConv(conv1), conv2)

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return DivideSingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return DivideDouble(ToVBBool(conv1), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty, _
                     TypeCode.Byte * TCMAX + TypeCode.Empty, _
                     TypeCode.Int16 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Empty, _
                     TypeCode.Int32 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Empty, _
                     TypeCode.Int64 * TCMAX + TypeCode.Empty, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return DivideDouble(conv1.ToDouble(Nothing), Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return DivideDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte, _
                     TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Byte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double
                    Return DivideDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal
                    Return DivideDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single
                    Return DivideSingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String
                    Return DivideDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty
                    Return DivideDecimal(conv1, 0D)

                Case TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return DivideDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Single * TCMAX + TypeCode.Empty
                    Return DivideSingle(conv1.ToSingle(Nothing), Nothing)

                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return DivideSingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return DivideDouble(CDbl(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return DivideDouble(CDbl(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double
                    Return DivideDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return DivideDouble(CDbl(conv1.ToString(Nothing)), CDbl(conv2.ToString(Nothing)))
#If 0 Then
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime
                Case TypeCode.Boolean * TCMAX + TypeCode.Char
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime
                Case TypeCode.SByte * TCMAX + TypeCode.Char
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime
                Case TypeCode.Byte * TCMAX + TypeCode.Char
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime
                Case TypeCode.Int16 * TCMAX + TypeCode.Char
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime
                Case TypeCode.Int32 * TCMAX + TypeCode.Char
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime
                Case TypeCode.Int64 * TCMAX + TypeCode.Char
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime
                Case TypeCode.Decimal * TCMAX + TypeCode.Char
                Case TypeCode.Single * TCMAX + TypeCode.DateTime
                Case TypeCode.Single * TCMAX + TypeCode.Char
                Case TypeCode.Double * TCMAX + TypeCode.DateTime
                Case TypeCode.Double * TCMAX + TypeCode.Char
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal
                Case TypeCode.DateTime * TCMAX + TypeCode.Single
                Case TypeCode.DateTime * TCMAX + TypeCode.Double
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime
                Case TypeCode.DateTime * TCMAX + TypeCode.Char
                Case TypeCode.DateTime * TCMAX + TypeCode.String
                Case TypeCode.Char * TCMAX + TypeCode.Empty
                Case TypeCode.Char * TCMAX + TypeCode.Boolean
                Case TypeCode.Char * TCMAX + TypeCode.SByte
                Case TypeCode.Char * TCMAX + TypeCode.Byte
                Case TypeCode.Char * TCMAX + TypeCode.Int16
                Case TypeCode.Char * TCMAX + TypeCode.UInt16
                Case TypeCode.Char * TCMAX + TypeCode.Int32
                Case TypeCode.Char * TCMAX + TypeCode.UInt32
                Case TypeCode.Char * TCMAX + TypeCode.Int64
                Case TypeCode.Char * TCMAX + TypeCode.UInt64
                Case TypeCode.Char * TCMAX + TypeCode.Decimal
                Case TypeCode.Char * TCMAX + TypeCode.Single
                Case TypeCode.Char * TCMAX + TypeCode.Double
                Case TypeCode.Char * TCMAX + TypeCode.DateTime
                Case TypeCode.Char * TCMAX + TypeCode.Char
                Case TypeCode.Char * TCMAX + TypeCode.String
                Case TypeCode.String * TCMAX + TypeCode.DateTime
                Case TypeCode.String * TCMAX + TypeCode.Char
#End If

                Case Else

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Divide, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Divide, Left, Right)

        End Function

        Private Shared Function DivideDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As Object
            Dim LeftValue As Decimal = Left.ToDecimal(Nothing)
            Dim RightValue As Decimal = Right.ToDecimal(Nothing)

            Try
                Return LeftValue / RightValue
            Catch ex As OverflowException
                'Converting to Single is inconsistent with Multiplication, where we convert to Double.
                Return CSng(LeftValue) / CSng(RightValue)
            End Try
        End Function

        Private Shared Function DivideSingle(ByVal Left As Single, ByVal Right As Single) As Object
            'REVIEW VSW#395759: think about this function.  Is it correct?
            Dim Result As Single = Left / Right

            If Single.IsInfinity(Result) Then
                If Single.IsInfinity(Left) OrElse Single.IsInfinity(Right) Then
                    Return Result
                End If
                Return CDbl(Left) / CDbl(Right)
            Else
                Return Result
            End If

#If 0 Then
            'REVIEW VSW# 395759: wouldn't this be a better implementation, performance wise?  it does change the semantics, though.       
            If ((Result <= Single.MaxValue AndAlso Result >= Single.MinValue)) Then
                Return CSng(Result)
            ElseIf Double.IsInfinity(Result) AndAlso (Single.IsInfinity(Left) OrElse Single.IsInfinity(Right)) Then
                Return CSng(Result)
            Else
                Return Result
            End If
#End If
        End Function

        Private Shared Function DivideDouble(ByVal Left As Double, ByVal Right As Double) As Object
            Return Left / Right
        End Function

#End Region

#Region " Operator Power ^ "

        Public Shared Function ExponentObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            Dim LeftValue As Double
            Dim RightValue As Double


            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1
                Case TypeCode.Empty
                    LeftValue = 0.0R

                Case TypeCode.Boolean
                    LeftValue = ToVBBool(conv1)

                Case TypeCode.SByte, _
                     TypeCode.Byte, _
                     TypeCode.Int16, _
                     TypeCode.UInt16, _
                     TypeCode.Int32, _
                     TypeCode.UInt32, _
                     TypeCode.Int64, _
                     TypeCode.UInt64, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double
                    LeftValue = conv1.ToDouble(Nothing)

                Case TypeCode.String
                    LeftValue = CDbl(conv1.ToString(Nothing))

                Case TypeCode.Object
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Power, Left, Right)

                Case Else
                    'DateTime
                    'Char
                    Throw GetNoValidOperatorException(UserDefinedOperator.Power, Left, Right)
            End Select

            Select Case tc2
                Case TypeCode.Empty
                    RightValue = 0.0R

                Case TypeCode.Boolean
                    RightValue = ToVBBool(conv2)

                Case TypeCode.SByte, _
                     TypeCode.Byte, _
                     TypeCode.Int16, _
                     TypeCode.UInt16, _
                     TypeCode.Int32, _
                     TypeCode.UInt32, _
                     TypeCode.Int64, _
                     TypeCode.UInt64, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double
                    RightValue = conv2.ToDouble(Nothing)

                Case TypeCode.String
                    RightValue = CDbl(conv2.ToString(Nothing))

                Case TypeCode.Object
                    Return InvokeUserDefinedOperator(UserDefinedOperator.Power, Left, Right)

                Case Else
                    'DateTime
                    'Char
                    Throw GetNoValidOperatorException(UserDefinedOperator.Power, Left, Right)
            End Select

            Return LeftValue ^ RightValue

        End Function

#End Region

#Region " Operator Mod "

        Public Shared Function ModObject(ByVal Left As Object, ByVal Right As Object) As Object
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode


            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'REVIEW VSW#395755: make shared members that represent pre-boxed zero values for each type.

            Select Case tc1 * TCMAX + tc2

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return ModInt32(Nothing, Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return ModInt16(Nothing, ToVBBool(conv2))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return ModSByte(Nothing, conv2.ToSByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return ModByte(Nothing, conv2.ToByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return ModInt16(Nothing, conv2.ToInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return ModUInt16(Nothing, conv2.ToUInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return ModInt32(Nothing, conv2.ToInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return ModUInt32(Nothing, conv2.ToUInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return ModInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return ModUInt64(Nothing, conv2.ToUInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal
                    Return ModDecimal(0D, conv2.ToDecimal(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Single
                    Return ModSingle(Nothing, conv2.ToSingle(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Double
                    Return ModDouble(Nothing, conv2.ToDouble(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return ModDouble(Nothing, CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return ModInt16(ToVBBool(conv1), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return ModInt16(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return ModSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return ModInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return ModInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64
                    Return ModInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal
                    Return ModDecimal(ToVBBoolConv(conv1), conv2.ToDecimal(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Single
                    Return ModSingle(ToVBBool(conv1), conv2.ToSingle(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Double
                    Return ModDouble(ToVBBool(conv1), conv2.ToDouble(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return ModDouble(ToVBBool(conv1), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return ModSByte(conv1.ToSByte(Nothing), Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return ModSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return ModSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return ModInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return ModInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64

                    Return ModInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal

                    Return ModDecimal(conv1, conv2)

                Case TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single

                    Return ModSingle(conv1.ToSingle(Nothing), conv2.ToSingle(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return ModDouble(conv1.ToDouble(Nothing), conv2.ToDouble(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return ModDouble(conv1.ToDouble(Nothing), CDbl(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return ModByte(conv1.ToByte(Nothing), Nothing)

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return ModInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return ModByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return ModUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32
                    Return ModUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return ModUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return ModInt16(conv1.ToInt16(Nothing), Nothing)


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return ModUInt16(conv1.ToUInt16(Nothing), Nothing)

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return ModInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return ModInt32(conv1.ToInt32(Nothing), Nothing)


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return ModUInt32(conv1.ToUInt32(Nothing), Nothing)

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean
                    Return ModInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return ModInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return ModUInt64(conv1.ToUInt64(Nothing), Nothing)

                Case TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean
                    Return ModDecimal(conv1, ToVBBoolConv(conv2))


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty
                    Return ModDecimal(conv1, 0D)


                Case TypeCode.Single * TCMAX + TypeCode.Empty
                    Return ModSingle(conv1.ToSingle(Nothing), Nothing)

                Case TypeCode.Single * TCMAX + TypeCode.Boolean
                    Return ModSingle(conv1.ToSingle(Nothing), ToVBBool(conv2))


                Case TypeCode.Double * TCMAX + TypeCode.Empty
                    Return ModDouble(conv1.ToDouble(Nothing), Nothing)

                Case TypeCode.Double * TCMAX + TypeCode.Boolean
                    Return ModDouble(conv1.ToDouble(Nothing), ToVBBool(conv2))


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return ModDouble(CDbl(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return ModDouble(CDbl(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return ModDouble(CDbl(conv1.ToString(Nothing)), conv2.ToDouble(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return ModDouble(CDbl(conv1.ToString(Nothing)), CDbl(conv2.ToString(Nothing)))


#If 0 Then
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If
                Case Else

            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Modulus, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.Modulus, Left, Right)

        End Function

        Private Shared Function ModSByte(ByVal Left As SByte, ByVal Right As SByte) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModByte(ByVal Left As Byte, ByVal Right As Byte) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModInt16(ByVal Left As Int16, ByVal Right As Int16) As Object
            'REVIEW VSW#395763: is it really necessary to consider promotion to Integer for Short Mod Short?
            Dim Result As Integer = CInt(Left) Mod CInt(Right)

            If Result < Int16.MinValue OrElse Result > Int16.MaxValue Then
                Return Result
            Else
                Return CShort(Result)  'REVIEW VSW#395757: overflow checking not needed here.
            End If
        End Function

        Private Shared Function ModUInt16(ByVal Left As UInt16, ByVal Right As UInt16) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModInt32(ByVal Left As Integer, ByVal Right As Integer) As Object
            'Do operation with Int64 to avoid OverflowException with Int32.MinValue and -1
            Dim result As Long = CLng(Left) Mod CLng(Right)

            If result < Int32.MinValue OrElse result > Int32.MaxValue Then
                Return result
            Else
                Return CInt(result)   'REVIEW VSW#395757: overflow checking not needed here.
            End If
        End Function

        Private Shared Function ModUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModInt64(ByVal Left As Int64, ByVal Right As Int64) As Object

            If Left = Int64.MinValue AndAlso Right = -1 Then
                Return 0L
            Else
                Return Left Mod Right
            End If

#If 0 Then
            'OLD IMPLEMENTATION
            'If i1 = Int64.MinValue and i2 = -1, then we get an overflow
            Try
                Return i1 Mod i2
            Catch ex As OverflowException
                Dim DecimalResult As Decimal
                DecimalResult = CDec(i1) Mod CDec(i2)
                'Overflow is not caused by remainder, so we will most likely still return Int64
                If DecimalResult < Int64.MinValue OrElse DecimalResult > Int64.MaxValue Then
                    Return DecimalResult
                Else
                    Return CLng(DecimalResult)
                End If
            End Try
#End If
        End Function

        Private Shared Function ModUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModDecimal(ByVal Left As IConvertible, ByVal Right As IConvertible) As Object
            Dim LeftValue As Decimal = Left.ToDecimal(Nothing)
            Dim RightValue As Decimal = Right.ToDecimal(Nothing)

            Return LeftValue Mod RightValue
        End Function

        Private Shared Function ModSingle(ByVal Left As Single, ByVal Right As Single) As Object
            Return Left Mod Right
        End Function

        Private Shared Function ModDouble(ByVal Left As Double, ByVal Right As Double) As Object
            Return Left Mod Right
        End Function

#End Region

#Region " Operator Integral Divide \ "

        Public Shared Function IntDivideObject(ByVal Left As Object, ByVal Right As Object) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode


            conv1 = TryCast(Left, IConvertible)

            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If


            conv2 = TryCast(Right, IConvertible)

            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If


            Select Case tc1 * TCMAX + tc2  'CONSIDER: overflow checking is not necessary for this calculation - perf improvement.

                Case TypeCode.Empty * TCMAX + TypeCode.Empty
                    Return IntDivideInt32(Nothing, Nothing)

                Case TypeCode.Empty * TCMAX + TypeCode.Boolean
                    Return IntDivideInt16(Nothing, ToVBBool(conv2))

                Case TypeCode.Empty * TCMAX + TypeCode.SByte
                    Return IntDivideSByte(Nothing, conv2.ToSByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Byte
                    Return IntDivideByte(Nothing, conv2.ToByte(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int16
                    Return IntDivideInt16(Nothing, conv2.ToInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt16
                    Return IntDivideUInt16(Nothing, conv2.ToUInt16(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int32
                    Return IntDivideInt32(Nothing, conv2.ToInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt32
                    Return IntDivideUInt32(Nothing, conv2.ToUInt32(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Int64
                    Return IntDivideInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.UInt64
                    Return IntDivideUInt64(Nothing, conv2.ToUInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.Decimal, _
                     TypeCode.Empty * TCMAX + TypeCode.Single, _
                     TypeCode.Empty * TCMAX + TypeCode.Double
                    Return IntDivideInt64(Nothing, conv2.ToInt64(Nothing))

                Case TypeCode.Empty * TCMAX + TypeCode.String
                    Return IntDivideInt64(Nothing, CLng(conv2.ToString(Nothing)))


                Case TypeCode.Boolean * TCMAX + TypeCode.Empty
                    Return IntDivideInt16(ToVBBool(conv1), Nothing)

                Case TypeCode.Boolean * TCMAX + TypeCode.Boolean
                    Return IntDivideInt16(ToVBBool(conv1), ToVBBool(conv2))

                Case TypeCode.Boolean * TCMAX + TypeCode.SByte
                    Return IntDivideSByte(ToVBBool(conv1), conv2.ToSByte(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.Byte, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int16
                    Return IntDivideInt16(ToVBBool(conv1), conv2.ToInt16(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt16, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int32
                    Return IntDivideInt32(ToVBBool(conv1), conv2.ToInt32(Nothing))

                Case TypeCode.Boolean * TCMAX + TypeCode.UInt32, _
                     TypeCode.Boolean * TCMAX + TypeCode.Int64, _
                     TypeCode.Boolean * TCMAX + TypeCode.UInt64, _
                     TypeCode.Boolean * TCMAX + TypeCode.Decimal, _
                     TypeCode.Boolean * TCMAX + TypeCode.Single, _
                     TypeCode.Boolean * TCMAX + TypeCode.Double

                    Return IntDivideInt64(ToVBBool(conv1), conv2.ToInt64(Nothing))  'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.Boolean * TCMAX + TypeCode.String
                    Return IntDivideInt64(ToVBBool(conv1), CLng(conv2.ToString(Nothing)))


                Case TypeCode.SByte * TCMAX + TypeCode.Empty
                    Return IntDivideSByte(conv1.ToSByte(Nothing), Nothing)

                Case TypeCode.SByte * TCMAX + TypeCode.Boolean
                    Return IntDivideSByte(conv1.ToSByte(Nothing), ToVBBool(conv2))

                Case TypeCode.SByte * TCMAX + TypeCode.SByte
                    Return IntDivideSByte(conv1.ToSByte(Nothing), conv2.ToSByte(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.Byte, _
                     TypeCode.SByte * TCMAX + TypeCode.Int16, _
                     TypeCode.Byte * TCMAX + TypeCode.SByte, _
                     TypeCode.Byte * TCMAX + TypeCode.Int16, _
                     TypeCode.Int16 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int16

                    Return IntDivideInt16(conv1.ToInt16(Nothing), conv2.ToInt16(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt16, _
                     TypeCode.SByte * TCMAX + TypeCode.Int32, _
                     TypeCode.Byte * TCMAX + TypeCode.Int32, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int32 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int32

                    Return IntDivideInt32(conv1.ToInt32(Nothing), conv2.ToInt32(Nothing))

                Case TypeCode.SByte * TCMAX + TypeCode.UInt32, _
                     TypeCode.SByte * TCMAX + TypeCode.Int64, _
                     TypeCode.SByte * TCMAX + TypeCode.UInt64, _
                     TypeCode.SByte * TCMAX + TypeCode.Decimal, _
                     TypeCode.SByte * TCMAX + TypeCode.Single, _
                     TypeCode.SByte * TCMAX + TypeCode.Double, _
                     TypeCode.Byte * TCMAX + TypeCode.Int64, _
                     TypeCode.Byte * TCMAX + TypeCode.Decimal, _
                     TypeCode.Byte * TCMAX + TypeCode.Single, _
                     TypeCode.Byte * TCMAX + TypeCode.Double, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int16 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int16 * TCMAX + TypeCode.Single, _
                     TypeCode.Int16 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Double, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int32 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int32 * TCMAX + TypeCode.Single, _
                     TypeCode.Int32 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt32 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Double, _
                     TypeCode.Int64 * TCMAX + TypeCode.SByte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Byte, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int16, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int32, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.Int64 * TCMAX + TypeCode.Int64, _
                     TypeCode.Int64 * TCMAX + TypeCode.UInt64, _
                     TypeCode.Int64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.Int64 * TCMAX + TypeCode.Single, _
                     TypeCode.Int64 * TCMAX + TypeCode.Double, _
                     TypeCode.UInt64 * TCMAX + TypeCode.SByte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Int64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Decimal, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Single, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Double, _
                     TypeCode.Decimal * TCMAX + TypeCode.SByte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Byte, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int16, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt16, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int32, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt32, _
                     TypeCode.Decimal * TCMAX + TypeCode.Int64, _
                     TypeCode.Decimal * TCMAX + TypeCode.UInt64, _
                     TypeCode.Decimal * TCMAX + TypeCode.Decimal, _
                     TypeCode.Decimal * TCMAX + TypeCode.Single, _
                     TypeCode.Decimal * TCMAX + TypeCode.Double, _
                     TypeCode.Single * TCMAX + TypeCode.SByte, _
                     TypeCode.Single * TCMAX + TypeCode.Byte, _
                     TypeCode.Single * TCMAX + TypeCode.Int16, _
                     TypeCode.Single * TCMAX + TypeCode.UInt16, _
                     TypeCode.Single * TCMAX + TypeCode.Int32, _
                     TypeCode.Single * TCMAX + TypeCode.UInt32, _
                     TypeCode.Single * TCMAX + TypeCode.Int64, _
                     TypeCode.Single * TCMAX + TypeCode.UInt64, _
                     TypeCode.Single * TCMAX + TypeCode.Decimal, _
                     TypeCode.Single * TCMAX + TypeCode.Single, _
                     TypeCode.Single * TCMAX + TypeCode.Double, _
                     TypeCode.Double * TCMAX + TypeCode.SByte, _
                     TypeCode.Double * TCMAX + TypeCode.Byte, _
                     TypeCode.Double * TCMAX + TypeCode.Int16, _
                     TypeCode.Double * TCMAX + TypeCode.UInt16, _
                     TypeCode.Double * TCMAX + TypeCode.Int32, _
                     TypeCode.Double * TCMAX + TypeCode.UInt32, _
                     TypeCode.Double * TCMAX + TypeCode.Int64, _
                     TypeCode.Double * TCMAX + TypeCode.UInt64, _
                     TypeCode.Double * TCMAX + TypeCode.Decimal, _
                     TypeCode.Double * TCMAX + TypeCode.Single, _
                     TypeCode.Double * TCMAX + TypeCode.Double

                    Return IntDivideInt64(conv1.ToInt64(Nothing), conv2.ToInt64(Nothing))   'UNDONE: what about error messages on the overflow?  not very useful coming from iconvertible code.

                Case TypeCode.SByte * TCMAX + TypeCode.String, _
                     TypeCode.Byte * TCMAX + TypeCode.String, _
                     TypeCode.Int16 * TCMAX + TypeCode.String, _
                     TypeCode.UInt16 * TCMAX + TypeCode.String, _
                     TypeCode.Int32 * TCMAX + TypeCode.String, _
                     TypeCode.UInt32 * TCMAX + TypeCode.String, _
                     TypeCode.Int64 * TCMAX + TypeCode.String, _
                     TypeCode.UInt64 * TCMAX + TypeCode.String, _
                     TypeCode.Decimal * TCMAX + TypeCode.String, _
                     TypeCode.Single * TCMAX + TypeCode.String, _
                     TypeCode.Double * TCMAX + TypeCode.String

                    Return IntDivideInt64(conv1.ToInt64(Nothing), CLng(conv2.ToString(Nothing)))


                Case TypeCode.Byte * TCMAX + TypeCode.Empty
                    Return IntDivideByte(conv1.ToByte(Nothing), Nothing)

                Case TypeCode.Byte * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int16 * TCMAX + TypeCode.Boolean
                    Return IntDivideInt16(conv1.ToInt16(Nothing), ToVBBool(conv2))

                Case TypeCode.Byte * TCMAX + TypeCode.Byte
                    Return IntDivideByte(conv1.ToByte(Nothing), conv2.ToByte(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt16 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt16
                    Return IntDivideUInt16(conv1.ToUInt16(Nothing), conv2.ToUInt16(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt32 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt32

                    Return IntDivideUInt32(conv1.ToUInt32(Nothing), conv2.ToUInt32(Nothing))

                Case TypeCode.Byte * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt16 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt32 * TCMAX + TypeCode.UInt64, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Byte, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt16, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt32, _
                     TypeCode.UInt64 * TCMAX + TypeCode.UInt64

                    Return IntDivideUInt64(conv1.ToUInt64(Nothing), conv2.ToUInt64(Nothing))


                Case TypeCode.Int16 * TCMAX + TypeCode.Empty
                    Return IntDivideInt16(conv1.ToInt16(Nothing), Nothing)


                Case TypeCode.UInt16 * TCMAX + TypeCode.Empty
                    Return IntDivideUInt16(conv1.ToUInt16(Nothing), Nothing)

                Case TypeCode.UInt16 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int32 * TCMAX + TypeCode.Boolean
                    Return IntDivideInt32(conv1.ToInt32(Nothing), ToVBBool(conv2))


                Case TypeCode.Int32 * TCMAX + TypeCode.Empty
                    Return IntDivideInt32(conv1.ToInt32(Nothing), Nothing)


                Case TypeCode.UInt32 * TCMAX + TypeCode.Empty
                    Return IntDivideUInt32(conv1.ToUInt32(Nothing), Nothing)

                Case TypeCode.UInt32 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Int64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.UInt64 * TCMAX + TypeCode.Boolean, _
                     TypeCode.Decimal * TCMAX + TypeCode.Boolean, _
                     TypeCode.Single * TCMAX + TypeCode.Boolean, _
                     TypeCode.Double * TCMAX + TypeCode.Boolean

                    Return IntDivideInt64(conv1.ToInt64(Nothing), ToVBBool(conv2))


                Case TypeCode.Int64 * TCMAX + TypeCode.Empty
                    Return IntDivideInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.UInt64 * TCMAX + TypeCode.Empty
                    Return IntDivideUInt64(conv1.ToUInt64(Nothing), Nothing)


                Case TypeCode.Decimal * TCMAX + TypeCode.Empty, _
                     TypeCode.Single * TCMAX + TypeCode.Empty, _
                     TypeCode.Double * TCMAX + TypeCode.Empty
                    Return IntDivideInt64(conv1.ToInt64(Nothing), Nothing)


                Case TypeCode.String * TCMAX + TypeCode.Empty
                    Return IntDivideInt64(CLng(conv1.ToString(Nothing)), Nothing)

                Case TypeCode.String * TCMAX + TypeCode.Boolean
                    Return IntDivideInt64(CLng(conv1.ToString(Nothing)), ToVBBool(conv2))

                Case TypeCode.String * TCMAX + TypeCode.SByte, _
                     TypeCode.String * TCMAX + TypeCode.Byte, _
                     TypeCode.String * TCMAX + TypeCode.Int16, _
                     TypeCode.String * TCMAX + TypeCode.UInt16, _
                     TypeCode.String * TCMAX + TypeCode.Int32, _
                     TypeCode.String * TCMAX + TypeCode.UInt32, _
                     TypeCode.String * TCMAX + TypeCode.Int64, _
                     TypeCode.String * TCMAX + TypeCode.UInt64, _
                     TypeCode.String * TCMAX + TypeCode.Decimal, _
                     TypeCode.String * TCMAX + TypeCode.Single, _
                     TypeCode.String * TCMAX + TypeCode.Double

                    Return IntDivideInt64(CLng(conv1.ToString(Nothing)), conv2.ToInt64(Nothing))

                Case TypeCode.String * TCMAX + TypeCode.String
                    Return IntDivideInt64(CLng(conv1.ToString(Nothing)), CLng(conv2.ToString(Nothing)))

#If 0 Then
                'ERROR CASES
                Case TypeCode.Empty * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Empty * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Boolean * TCMAX + TypeCode.Char 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.SByte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Byte * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt16 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt32 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Int64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.UInt64 * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Decimal * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Single * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Single * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Double * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Double * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Single 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Double 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.Char 'XX
                Case TypeCode.DateTime * TCMAX + TypeCode.String 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Empty 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Boolean 'XX
                Case TypeCode.Char * TCMAX + TypeCode.SByte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Byte 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt16 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt32 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Int64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.UInt64 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Decimal 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Single 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Double 'XX
                Case TypeCode.Char * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.Char * TCMAX + TypeCode.Char 'XX
                Case TypeCode.Char * TCMAX + TypeCode.String 'XX
                Case TypeCode.String * TCMAX + TypeCode.DateTime 'XX
                Case TypeCode.String * TCMAX + TypeCode.Char 'XX
#End If
            End Select

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.IntegralDivide, Left, Right)
            End If

            Throw GetNoValidOperatorException(UserDefinedOperator.IntegralDivide, Left, Right)

        End Function

        Private Shared Function IntDivideSByte(ByVal Left As SByte, ByVal Right As SByte) As Object
            If Left = Sbyte.MinValue AndAlso Right = -1 Then
                Return -CShort(Sbyte.MinValue)
            End If

            Return Left \ Right
        End Function

        Private Shared Function IntDivideByte(ByVal Left As Byte, ByVal Right As Byte) As Object
            Return Left \ Right
        End Function

        Private Shared Function IntDivideInt16(ByVal Left As Int16, ByVal Right As Int16) As Object
            If Left = Short.MinValue AndAlso Right = -1 Then
                Return -CInt(Short.MinValue)
            End If

            Return Left \ Right
        End Function

        Private Shared Function IntDivideUInt16(ByVal Left As UInt16, ByVal Right As UInt16) As Object
            Return Left \ Right
        End Function

        Private Shared Function IntDivideInt32(ByVal Left As Int32, ByVal Right As Int32) As Object
            If Left = Integer.MinValue AndAlso Right = -1 Then
                Return -CLng(Integer.MinValue)
            End If

            Return Left \ Right
        End Function

        Private Shared Function IntDivideUInt32(ByVal Left As UInt32, ByVal Right As UInt32) As Object
            Return Left \ Right
        End Function

        Private Shared Function IntDivideInt64(ByVal Left As Int64, ByVal Right As Int64) As Object
            Return Left \ Right
        End Function

        Private Shared Function IntDivideUInt64(ByVal Left As UInt64, ByVal Right As UInt64) As Object
            Return Left \ Right
        End Function

#End Region

#Region " Operator Shift Left << "

        Public Shared Function LeftShiftObject(ByVal Operand As Object, ByVal Amount As Object) As Object

            'VSW#395761: There's no benefit from making this look like negate, so don't change it.
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Operand, IConvertible)

            If conv1 Is Nothing Then
                If Operand Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If

            conv2 = TryCast(Amount, IConvertible)

            If conv2 Is Nothing Then
                If Amount Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.ShiftLeft, Operand, Amount)
            End If

            Select Case tc1
                Case TypeCode.Empty
                    Return Nothing << CInt(Amount)
                Case TypeCode.Boolean
                    Return CShort(conv1.ToBoolean(Nothing)) << CInt(Amount)
                Case TypeCode.SByte
                    Return conv1.ToSByte(Nothing) << CInt(Amount)
                Case TypeCode.Byte
                    Return conv1.ToByte(Nothing) << CInt(Amount)
                Case TypeCode.Int16
                    Return conv1.ToInt16(Nothing) << CInt(Amount)
                Case TypeCode.UInt16
                    Return conv1.ToUInt16(Nothing) << CInt(Amount)
                Case TypeCode.Int32
                    Return conv1.ToInt32(Nothing) << CInt(Amount)
                Case TypeCode.UInt32
                    Return conv1.ToUInt32(Nothing) << CInt(Amount)
                Case TypeCode.Int64, _
                     TypeCode.Single, _
                     TypeCode.Double, _
                     TypeCode.Decimal
                    Return conv1.ToInt64(Nothing) << CInt(Amount)
                Case TypeCode.UInt64
                    Return conv1.ToUInt64(Nothing) << CInt(Amount)
                Case TypeCode.String
                    Return CLng(conv1.ToString(Nothing)) << CInt(Amount)
            End Select

            Throw GetNoValidOperatorException(UserDefinedOperator.ShiftLeft, Operand)
        End Function

#End Region

#Region " Operator Shift Right >> "

        Public Shared Function RightShiftObject(ByVal Operand As Object, ByVal Amount As Object) As Object

            'VSW#395761: There's no benefit from making this look like negate, so don't change it.
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Operand, IConvertible)

            If conv1 Is Nothing Then
                If Operand Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If

            conv2 = TryCast(Amount, IConvertible)

            If conv2 Is Nothing Then
                If Amount Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.ShiftRight, Operand, Amount)
            End If

            Select Case tc1
                Case TypeCode.Empty
                    Return Nothing >> CInt(Amount)
                Case TypeCode.Boolean
                    Return CShort(conv1.ToBoolean(Nothing)) >> CInt(Amount)
                Case TypeCode.SByte
                    Return conv1.ToSByte(Nothing) >> CInt(Amount)
                Case TypeCode.Byte
                    Return conv1.ToByte(Nothing) >> CInt(Amount)
                Case TypeCode.Int16
                    Return conv1.ToInt16(Nothing) >> CInt(Amount)
                Case TypeCode.UInt16
                    Return conv1.ToUInt16(Nothing) >> CInt(Amount)
                Case TypeCode.Int32
                    Return conv1.ToInt32(Nothing) >> CInt(Amount)
                Case TypeCode.UInt32
                    Return conv1.ToUInt32(Nothing) >> CInt(Amount)
                Case TypeCode.Int64, _
                     TypeCode.Single, _
                     TypeCode.Double, _
                     TypeCode.Decimal
                    Return conv1.ToInt64(Nothing) >> CInt(Amount)
                Case TypeCode.UInt64
                    Return conv1.ToUInt64(Nothing) >> CInt(Amount)
                Case TypeCode.String
                    Return CLng(conv1.ToString(Nothing)) >> CInt(Amount)
            End Select

            Throw GetNoValidOperatorException(UserDefinedOperator.ShiftRight, Operand)
        End Function

#End Region

#Region " Operator Like "

#If Not TELESTO Then

        ' - Some odd refactoring happened here that we must live with.  We no longer emit runtime helper calls to this function from the
        'compiler--we use the functions defined in LikeOperator.vb  But we have to hang on to this because an Everett app running on a Whidbey+
        'runtime could try to call this.  There was a TODO to remove this during Whidbey but it's just as well they didn't because it would have toasted
        'Everett apps.
        Public Shared Function LikeObject(ByVal Source As Object, ByVal Pattern As Object, ByVal CompareOption As CompareMethod) As Object

            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Source, IConvertible)
            If conv1 Is Nothing Then
                If Source Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If

            conv2 = TryCast(Pattern, IConvertible)
            If conv2 Is Nothing Then
                If Pattern Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'Special cases for Char()
            If (tc1 = TypeCode.Object) AndAlso (TypeOf Source Is Char()) Then
                tc1 = TypeCode.String
            End If

            If (tc2 = TypeCode.Object) AndAlso (TypeOf Pattern Is Char()) Then
                tc2 = TypeCode.String
            End If

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Like, Source, Pattern)
            End If

            Return LikeString(CStr(Source), CStr(Pattern), CompareOption)
        End Function

        'UNDONE: can't the code generator just call the right compare version?  The can remove this function.
        Public Shared Function LikeString(ByVal Source As String, ByVal Pattern As String, ByVal CompareOption As CompareMethod) As Boolean
            If CompareOption = CompareMethod.Binary Then
                Return LikeStringBinary(Source, Pattern)
            Else
                Return LikeStringText(Source, Pattern)
            End If
        End Function

        Private Shared Function LikeStringBinary(ByVal Source As String, ByVal Pattern As String) As Boolean
            'Match Source to Pattern using "?*#[!a-g]" pattern matching characters
            Dim SourceIndex As Integer
            Dim PatternIndex As Integer
            Dim SourceEndIndex As Integer
            Dim PatternEndIndex As Integer
            Dim p As Char
            Dim s As Char
            Dim InsideBracket As Boolean
            Dim SeenHyphen As Boolean
            Dim StartRangeChar As Char
            Dim EndRangeChar As Char
            Dim Match As Boolean
            Dim SeenLiteral As Boolean
            Dim SeenNot As Boolean
            Dim Skip As Integer
            Const NullChar As Char = ChrW(0)
            Dim LiteralIsRangeEnd As Boolean = False

            '        Options = CompareOptions.Ordinal

            If Pattern Is Nothing Then
                PatternEndIndex = 0
            Else
                PatternEndIndex = Pattern.Length
            End If

            If Source Is Nothing Then
                SourceEndIndex = 0
            Else
                SourceEndIndex = Source.Length
            End If

            If SourceIndex < SourceEndIndex Then
                s = Source.Chars(SourceIndex)
            End If

            Do While PatternIndex < PatternEndIndex
                p = Pattern.Chars(PatternIndex)

                If p = "*"c AndAlso (Not InsideBracket) Then        'If Then Else has faster performance the Select Case
                    'Determine how many source chars to skip
                    Skip = AsteriskSkip(Pattern.Substring(PatternIndex + 1), Source.Substring(SourceIndex), SourceEndIndex - SourceIndex, CompareMethod.Binary, m_InvariantCompareInfo)

                    If Skip < 0 Then
                        Return False
                    ElseIf Skip > 0 Then
                        SourceIndex += Skip
                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        End If
                    End If

                ElseIf p = "?"c AndAlso (Not InsideBracket) Then
                    'Match any character
                    SourceIndex = SourceIndex + 1
                    If SourceIndex < SourceEndIndex Then
                        s = Source.Chars(SourceIndex)
                    End If

                ElseIf p = "#"c AndAlso (Not InsideBracket) Then
                    If Not System.Char.IsDigit(s) Then
                        Exit Do
                    End If
                    SourceIndex = SourceIndex + 1
                    If SourceIndex < SourceEndIndex Then
                        s = Source.Chars(SourceIndex)
                    End If

                ElseIf p = "-"c AndAlso _
                        (InsideBracket AndAlso SeenLiteral AndAlso (Not LiteralIsRangeEnd) AndAlso (Not SeenHyphen)) AndAlso _
                        (((PatternIndex + 1) >= PatternEndIndex) OrElse (Pattern.Chars(PatternIndex + 1) <> "]"c)) Then

                    SeenHyphen = True

                ElseIf p = "!"c AndAlso _
                        (InsideBracket AndAlso (Not SeenNot)) Then

                    SeenNot = True
                    Match = True

                ElseIf p = "["c AndAlso (Not InsideBracket) Then
                    InsideBracket = True
                    StartRangeChar = NullChar
                    EndRangeChar = NullChar
                    SeenLiteral = False

                ElseIf p = "]"c AndAlso InsideBracket Then
                    InsideBracket = False

                    If SeenLiteral Then
                        If Match Then
                            SourceIndex += 1
                            If SourceIndex < SourceEndIndex Then
                                s = Source.Chars(SourceIndex)
                            End If
                        Else
                            Exit Do
                        End If
                    ElseIf SeenHyphen Then
                        If Not Match Then
                            Exit Do
                        End If
                    ElseIf SeenNot Then
                        '[!] should be matched to literal ! same as if outside brackets
                        If "!"c <> s Then
                            Exit Do
                        End If
                        SourceIndex += 1
                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        End If
                    End If

                    Match = False
                    SeenLiteral = False
                    SeenNot = False
                    SeenHyphen = False

                Else
                    'Literal character
                    SeenLiteral = True
                    LiteralIsRangeEnd = False

                    If InsideBracket Then
                        If SeenHyphen Then
                            SeenHyphen = False
                            LiteralIsRangeEnd = True
                            EndRangeChar = p

                            If StartRangeChar > EndRangeChar Then
                                Throw VbMakeException(vbErrors.BadPatStr)
                            ElseIf (SeenNot AndAlso Match) OrElse (Not SeenNot AndAlso Not Match) Then
                                'Calls to ci.Compare are expensive, avoid them for good performance
                                Match = (s > StartRangeChar) AndAlso (s <= EndRangeChar)

                                If SeenNot Then
                                    Match = Not Match
                                End If
                            End If
                        Else
                            StartRangeChar = p

                            'This compare handles non range chars such as the "abc" and "uvw" 
                            'and the first char of a range such as "d" in "[abcd-tuvw]".
                            Match = LikeStringCompareBinary(SeenNot, Match, p, s)
                        End If
                    Else
                        If p <> s AndAlso Not SeenNot Then
                            Exit Do
                        End If

                        SeenNot = False
                        SourceIndex += 1

                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        ElseIf SourceIndex > SourceEndIndex Then
                            Return False
                        End If
                    End If
                End If

                PatternIndex += 1
            Loop

            If InsideBracket Then
                If SourceEndIndex = 0 Then
                    Return False
                Else
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Pattern"))
                End If
            Else
                Return (PatternIndex = PatternEndIndex) AndAlso (SourceIndex = SourceEndIndex)
            End If
        End Function

        Private Shared Function LikeStringText(ByVal Source As String, ByVal Pattern As String) As Boolean
            'Match Source to Pattern using "?*#[!a-g]" pattern matching characters
            Dim SourceIndex As Integer
            Dim PatternIndex As Integer
            Dim SourceEndIndex As Integer
            Dim PatternEndIndex As Integer
            Dim p As Char
            Dim s As Char
            Dim InsideBracket As Boolean
            Dim SeenHyphen As Boolean
            Dim StartRangeChar As Char
            Dim EndRangeChar As Char
            Dim Match As Boolean
            Dim SeenLiteral As Boolean
            Dim SeenNot As Boolean
            Dim Skip As Integer
            Dim Options As CompareOptions
            Dim ci As CompareInfo
            Const NullChar As Char = ChrW(0)
            Dim LiteralIsRangeEnd As Boolean = False

            If Pattern Is Nothing Then
                PatternEndIndex = 0
            Else
                PatternEndIndex = Pattern.Length
            End If

            If Source Is Nothing Then
                SourceEndIndex = 0
            Else
                SourceEndIndex = Source.Length
            End If

            If SourceIndex < SourceEndIndex Then
                s = Source.Chars(SourceIndex)
            End If

            ci = GetCultureInfo().CompareInfo
            Options = CompareOptions.IgnoreCase Or _
                      CompareOptions.IgnoreWidth Or _
                      CompareOptions.IgnoreNonSpace Or _
                      CompareOptions.IgnoreKanaType

            Do While PatternIndex < PatternEndIndex
                p = Pattern.Chars(PatternIndex)

                If p = "*"c AndAlso (Not InsideBracket) Then        'If Then Else has faster performance the Select Case
                    'Determine how many source chars to skip
                    Skip = AsteriskSkip(Pattern.Substring(PatternIndex + 1), Source.Substring(SourceIndex), SourceEndIndex - SourceIndex, CompareMethod.Text, ci)

                    If Skip < 0 Then
                        Return False
                    ElseIf Skip > 0 Then
                        SourceIndex += Skip
                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        End If
                    End If

                ElseIf p = "?"c AndAlso (Not InsideBracket) Then
                    'Match any character
                    SourceIndex = SourceIndex + 1
                    If SourceIndex < SourceEndIndex Then
                        s = Source.Chars(SourceIndex)
                    End If

                ElseIf p = "#"c AndAlso (Not InsideBracket) Then
                    If Not System.Char.IsDigit(s) Then
                        Exit Do
                    End If
                    SourceIndex = SourceIndex + 1
                    If SourceIndex < SourceEndIndex Then
                        s = Source.Chars(SourceIndex)
                    End If

                ElseIf p = "-"c AndAlso _
                        (InsideBracket AndAlso SeenLiteral AndAlso (Not LiteralIsRangeEnd) AndAlso (Not SeenHyphen)) AndAlso _
                        (((PatternIndex + 1) >= PatternEndIndex) OrElse (Pattern.Chars(PatternIndex + 1) <> "]"c)) Then

                    SeenHyphen = True

                ElseIf p = "!"c AndAlso _
                        (InsideBracket AndAlso Not SeenNot) Then
                    SeenNot = True
                    Match = True

                ElseIf p = "["c AndAlso (Not InsideBracket) Then
                    InsideBracket = True
                    StartRangeChar = NullChar
                    EndRangeChar = NullChar
                    SeenLiteral = False

                ElseIf p = "]"c AndAlso InsideBracket Then
                    InsideBracket = False

                    If SeenLiteral Then
                        If Match Then
                            SourceIndex += 1
                            If SourceIndex < SourceEndIndex Then
                                s = Source.Chars(SourceIndex)
                            End If
                        Else
                            Exit Do
                        End If
                    ElseIf SeenHyphen Then
                        If Not Match Then
                            Exit Do
                        End If
                    ElseIf SeenNot Then
                        '[!] should be matched to literal ! same as if outside brackets
                        If (ci.Compare("!", s) <> 0) Then
                            Exit Do
                        End If
                        SourceIndex += 1
                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        End If
                    End If

                    Match = False
                    SeenLiteral = False
                    SeenNot = False
                    SeenHyphen = False

                Else
                    'Literal character
                    SeenLiteral = True
                    LiteralIsRangeEnd = False

                    If InsideBracket Then
                        If SeenHyphen Then
                            SeenHyphen = False
                            LiteralIsRangeEnd = True
                            EndRangeChar = p

                            If StartRangeChar > EndRangeChar Then
                                Throw VbMakeException(vbErrors.BadPatStr)
                            ElseIf (SeenNot AndAlso Match) OrElse (Not SeenNot AndAlso Not Match) Then
                                'Calls to ci.Compare are expensive, avoid them for good performance
                                If Options = CompareOptions.Ordinal Then
                                    Match = (s > StartRangeChar) AndAlso (s <= EndRangeChar)
                                Else
                                    Match = (ci.Compare(StartRangeChar, s, Options) < 0) AndAlso (ci.Compare(EndRangeChar, s, Options) >= 0)
                                End If

                                If SeenNot Then
                                    Match = Not Match
                                End If
                            End If
                        Else
                            StartRangeChar = p

                            'This compare handles non range chars such as the "abc" and "uvw" 
                            'and the first char of a range such as "d" in "[abcd-tuvw]".
                            Match = LikeStringCompare(ci, SeenNot, Match, p, s, Options)
                        End If
                    Else
                        If Options = CompareOptions.Ordinal Then
                            If p <> s AndAlso Not SeenNot Then
                                Exit Do
                            End If
                        Else
                            ' Slurp up the diacritical marks, if any (both non-spacing marks and modifier symbols)
                            ' Note that typically, we'll only have at most one diacritical mark.  Therefore, I'm not
                            ' using StringBuilder here, since the minimal overhead of appending a character doesn't
                            ' justify invoking a couple of instances of StringBuilder..
                            Dim pstr As String = p
                            Dim sstr As String = s
                            Do While PatternIndex + 1 < PatternEndIndex AndAlso _
                                    (UnicodeCategory.ModifierSymbol = Char.GetUnicodeCategory(Pattern.Chars(PatternIndex + 1)) OrElse _
                                    UnicodeCategory.NonSpacingMark = Char.GetUnicodeCategory(Pattern.Chars(PatternIndex + 1)))
                                pstr = pstr & Pattern.Chars(PatternIndex + 1)
                                PatternIndex = PatternIndex + 1
                            Loop
                            Do While SourceIndex + 1 < SourceEndIndex AndAlso _
                                    (UnicodeCategory.ModifierSymbol = Char.GetUnicodeCategory(Source.Chars(SourceIndex + 1)) OrElse _
                                    UnicodeCategory.NonSpacingMark = Char.GetUnicodeCategory(Source.Chars(SourceIndex + 1)))
                                sstr = sstr & Source.Chars(SourceIndex + 1)
                                SourceIndex = SourceIndex + 1
                            Loop

                            If (ci.Compare(pstr, sstr, OptionCompareTextFlags) <> 0) AndAlso Not SeenNot Then
                                Exit Do
                            End If
                        End If

                        SeenNot = False
                        SourceIndex += 1

                        If SourceIndex < SourceEndIndex Then
                            s = Source.Chars(SourceIndex)
                        ElseIf SourceIndex > SourceEndIndex Then
                            Return False
                        End If
                    End If
                End If

                PatternIndex += 1
            Loop

            If InsideBracket Then
                If SourceEndIndex = 0 Then
                    Return False
                Else
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Pattern"))
                End If
            Else
                Return (PatternIndex = PatternEndIndex) AndAlso (SourceIndex = SourceEndIndex)
            End If
        End Function

        Private Shared Function LikeStringCompareBinary(ByVal SeenNot As Boolean, ByVal Match As Boolean, ByVal p As Char, ByVal s As Char) As Boolean
            If SeenNot AndAlso Match Then
                Return p <> s
            ElseIf Not SeenNot AndAlso Not Match Then
                Return p = s
            Else
                Return Match
            End If
        End Function

        Private Shared Function LikeStringCompare(ByVal ci As CompareInfo, ByVal SeenNot As Boolean, ByVal Match As Boolean, ByVal p As Char, ByVal s As Char, ByVal Options As CompareOptions) As Boolean
            If SeenNot AndAlso Match Then
                If Options = CompareOptions.Ordinal Then
                    Return p <> s
                Else
                    Return Not (ci.Compare(p, s, Options) = 0)
                End If
            ElseIf Not SeenNot AndAlso Not Match Then
                If Options = CompareOptions.Ordinal Then
                    Return p = s
                Else
                    Return (ci.Compare(p, s, Options) = 0)
                End If
            Else
                Return Match
            End If
        End Function

        Private Shared Function AsteriskSkip(ByVal Pattern As String, ByVal Source As String, ByVal SourceEndIndex As Integer, _
            ByVal CompareOption As CompareMethod, ByVal ci As CompareInfo) As Integer

            'Returns the number of source characters to skip over to handle an asterisk in the pattern. 
            'When there's only a single asterisk in the pattern, it computes how many pattern equivalent chars  
            'follow the *: [a-z], [abcde], ?, # each count as one char.
            'Pattern contains the substring following the *
            'Source contains the substring not yet matched.

            Dim p As Char
            Dim SeenLiteral As Boolean
            Dim SeenSpecial As Boolean   'Remembers if we've seen #, ?, [abd-eg], or ! when they have their special meanings
            Dim InsideBracket As Boolean
            Dim Count As Integer
            Dim PatternEndIndex As Integer
            Dim PatternIndex As Integer
            Dim TruncatedPattern As String
            Dim Options As CompareOptions

            PatternEndIndex = Len(Pattern)

            'Determine how many pattern equivalent chars follow the *, and if there are multiple *s
            '[a-z], [abcde] each count as one char.
            Do While PatternIndex < PatternEndIndex
                p = Pattern.Chars(PatternIndex)

                Select Case p
                    Case "*"c
                        If Count > 0 Then
                            'We found multiple asterisks with an intervening pattern
                            If SeenSpecial Then
                                'Pattern uses special characters which means we can't compute easily how far to skip. 
                                Count = MultipleAsteriskSkip(Pattern, Source, Count, CompareOption)
                                Return SourceEndIndex - Count
                            Else
                                'Pattern uses only literals, so we can directly search for the pattern in the source
                                'TODO: Handle cases where pattern could be replicated in the source.
                                TruncatedPattern = Pattern.Substring(0, PatternIndex)    'Remove the second * and everything trailing  

                                If CompareOption = CompareMethod.Binary Then
                                    Options = CompareOptions.Ordinal
                                Else
                                    Options = CompareOptions.IgnoreCase Or CompareOptions.IgnoreWidth Or CompareOptions.IgnoreNonSpace Or CompareOptions.IgnoreKanaType
                                End If

                                'Count = Source.LastIndexOf(TruncatedPattern)
                                Count = ci.LastIndexOf(Source, TruncatedPattern, Options)
                                Return Count
                            End If

                        Else
                            'Do nothing, which colalesces multiple asterisks together
                        End If

                    Case "-"c
                        If Pattern.Chars(PatternIndex + 1) = "]"c Then
                            SeenLiteral = True
                        End If

                    Case "!"c
                        If Pattern.Chars(PatternIndex + 1) = "]"c Then
                            SeenLiteral = True
                        Else
                            SeenSpecial = True
                        End If

                    Case "["c
                        If InsideBracket Then
                            SeenLiteral = True
                        Else
                            InsideBracket = True
                        End If

                    Case "]"c
                        If SeenLiteral OrElse Not InsideBracket Then
                            Count += 1
                            SeenSpecial = True
                        End If
                        SeenLiteral = False
                        InsideBracket = False

                    Case "?"c, "#"c
                        If InsideBracket Then
                            SeenLiteral = True
                        Else
                            Count += 1
                            SeenSpecial = True
                        End If

                    Case Else
                        If InsideBracket Then
                            SeenLiteral = True
                        Else
                            Count += 1
                        End If
                End Select

                PatternIndex += 1
            Loop

            Return SourceEndIndex - Count
        End Function

        Private Shared Function MultipleAsteriskSkip(ByVal Pattern As String, ByVal Source As String, ByVal Count As Integer, ByVal CompareOption As CompareMethod) As Integer
            'Multiple asterisks with intervening chars were found in the pattern, such as "*<chars>*".
            'Use a recursive approach to determine how many source chars to skip.
            'Start near the end of Source and move backwards one char at a time until a match is found or we reach start of Source.

            Dim SourceEndIndex As Integer
            Dim NewSource As String
            Dim Result As Boolean

            SourceEndIndex = Len(Source)

            Do While Count < SourceEndIndex
                NewSource = Source.Substring(SourceEndIndex - Count)

                Try
                    Result = LikeString(NewSource, Pattern, CompareOption)
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Result = False
                End Try

                If Result Then
                    Exit Do
                End If

                Count += 1
            Loop

            Return Count
        End Function

#End If

#End Region

#Region " Operator Concatenate & "

        Public Shared Function ConcatenateObject(ByVal Left As Object, ByVal Right As Object) As Object
            Dim conv1, conv2 As IConvertible
            Dim tc1, tc2 As TypeCode

            conv1 = TryCast(Left, IConvertible)
            If conv1 Is Nothing Then
                If Left Is Nothing Then
                    tc1 = TypeCode.Empty
                Else
                    tc1 = TypeCode.Object
                End If
            Else
                tc1 = conv1.GetTypeCode()
            End If

            conv2 = TryCast(Right, IConvertible)
            If conv2 Is Nothing Then
                If Right Is Nothing Then
                    tc2 = TypeCode.Empty
                Else
                    tc2 = TypeCode.Object
                End If
            Else
                tc2 = conv2.GetTypeCode()
            End If

            'Special cases for Char()
            If (tc1 = TypeCode.Object) AndAlso (TypeOf Left Is Char()) Then
                tc1 = TypeCode.String
            End If

            If (tc2 = TypeCode.Object) AndAlso (TypeOf Right Is Char()) Then
                tc2 = TypeCode.String
            End If

            If tc1 = TypeCode.Object OrElse tc2 = TypeCode.Object Then
                Return InvokeUserDefinedOperator(UserDefinedOperator.Concatenate, Left, Right)
            End If

            Dim LeftIsNull As Boolean = (tc1 = TypeCode.DBNull)
            Dim RightIsNull As Boolean = (tc2 = TypeCode.DBNull)

            If LeftIsNull And RightIsNull Then
                Return Left
            ElseIf LeftIsNull And Not RightIsNull Then
                Left = ""
            ElseIf RightIsNull And Not LeftIsNull Then
                Right = ""
            End If

            Return CStr(Left) & CStr(Right)
        End Function

#End Region

    End Class

End Namespace

