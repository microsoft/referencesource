' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Dynamic

Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class Versioned
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class Versioned
#End If
        ' Prevent creation.
        Private Sub New()
        End Sub

        Public Shared Function CallByName(ByVal Instance As System.Object, ByVal MethodName As String, ByVal UseCallType As CallType, ByVal ParamArray Arguments() As Object) As Object

            Select Case UseCallType

                Case CallType.Method
                    'Need to use LateGet, because we are returning a value
                    Return CompilerServices.NewLateBinding.LateCall(Instance, Nothing, MethodName, Arguments, Nothing, Nothing, Nothing, False)

                Case CallType.Get
                    Return CompilerServices.NewLateBinding.LateGet(Instance, Nothing, MethodName, Arguments, Nothing, Nothing, Nothing)

                Case CallType.Let, _
                     CallType.Set
                    Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
                    If idmop IsNot Nothing Then
                        ' UseCallType is used in the late binder to affect the binding behavior for COM Object, but COM Objects
                        ' don't implement IDynamicMetaObjectProvider.  Therefore it is safe not to pass on UseCallType here.
                        IDOBinder.IDOSet(idmop, MethodName, Nothing, Arguments)
                    Else
                        CompilerServices.NewLateBinding.LateSet(Instance, Nothing, MethodName, Arguments, Nothing, Nothing, False, False, UseCallType)
                    End If
                    Return Nothing

                Case Else
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "CallType"))
            End Select

        End Function

        '*
        '*  IsNumeric -
        '* 
        '* 
        '* NOTE: Code changes here MUST BE PERFORMANCE TESTED
        '* 
        Public Shared Function IsNumeric(ByVal Expression As Object) As Boolean

            Dim ValueInterface As IConvertible = TryCast(Expression, IConvertible)

            If ValueInterface Is Nothing Then
                Return False
            End If

            Select Case ValueInterface.GetTypeCode()

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

                    Return True

                Case TypeCode.Boolean
                    Return True

                Case TypeCode.Char, _
                     TypeCode.String

                    'Convert to double, exception thrown if not a number
                    Dim Value As String = ValueInterface.ToString(Nothing)

                    Try
                        'CONSIDER: Optimize out this exception case
                        Dim i64Value As Int64
                        If IsHexOrOctValue(Value, i64Value) Then
                            Return True
                        End If
                    Catch ex As FormatException
                        Return False
                    End Try

                    Dim dbl As Double
                    Return Conversions.TryParseDouble(Value, dbl)

                Case TypeCode.Empty, _
                     TypeCode.Object, _
                     TypeCode.DBNull, _
                     TypeCode.DateTime

                    'fall through to end

            End Select

            Return False

        End Function

        Public Shared Function TypeName(ByVal Expression As Object) As String

            Dim Result As String
            Dim typ As System.Type

            If Expression Is Nothing Then
                Return "Nothing"
            End If

            typ = Expression.GetType()
#If Not TELESTO Then
            If (typ.IsCOMObject AndAlso (System.String.CompareOrdinal(typ.Name, COMObjectName) = 0)) Then
                Result = TypeNameOfCOMObject(Expression, True)
            Else
                Result = VBFriendlyNameOfType(typ)
            End If
#Else 
            Result = VBFriendlyNameOfType(typ)
#End If
            Return Result  
        End Function

        Public Shared Function SystemTypeName(ByVal VbName As String) As String
#If TELESTO Then
            Select Case Trim(VbName).ToUpper(Globalization.CultureInfo.InvariantCulture) 'Using instead of ToUpperInvariant because ToUpperInvariant isn't available on Telesto and this is equivilant on both platforms
#Else
            Select Case Trim(VbName).ToUpperInvariant()
#End If
                Case "BOOLEAN" : Return "System.Boolean"
                Case "SBYTE" : Return "System.SByte"
                Case "BYTE" : Return "System.Byte"
                Case "SHORT" : Return "System.Int16"
                Case "USHORT" : Return "System.UInt16"
                Case "INTEGER" : Return "System.Int32"
                Case "UINTEGER" : Return "System.UInt32"
                Case "LONG" : Return "System.Int64"
                Case "ULONG" : Return "System.UInt64"
                Case "DECIMAL" : Return "System.Decimal"
                Case "SINGLE" : Return "System.Single"
                Case "DOUBLE" : Return "System.Double"
                Case "DATE" : Return "System.DateTime"
                Case "CHAR" : Return "System.Char"
                Case "STRING" : Return "System.String"
                Case "OBJECT" : Return "System.Object"

                Case Else
                    Return Nothing

            End Select
        End Function

        Public Shared Function VbTypeName(ByVal SystemName As String) As String
#If TELESTO Then
            SystemName = Trim(SystemName).ToUpper(Globalization.CultureInfo.InvariantCulture) 'Using instead of ToUpperInvariant because ToUpperInvariant isn't available on Telesto and this is equivilant on both platforms
#Else
            SystemName = Trim(SystemName).ToUpperInvariant()
#End If
            If Left(SystemName, 7) = "SYSTEM." Then
                SystemName = Mid(SystemName, 8)
            End If

            Select Case SystemName

                Case "BOOLEAN" : Return "Boolean"
                Case "SBYTE" : Return "SByte"
                Case "BYTE" : Return "Byte"
                Case "INT16" : Return "Short"
                Case "UINT16" : Return "UShort"
                Case "INT32" : Return "Integer"
                Case "UINT32" : Return "UInteger"
                Case "INT64" : Return "Long"
                Case "UINT64" : Return "ULong"
                Case "DECIMAL" : Return "Decimal"
                Case "SINGLE" : Return "Single"
                Case "DOUBLE" : Return "Double"
                Case "DATETIME" : Return "Date"
                Case "CHAR" : Return "Char"
                Case "STRING" : Return "String"
                Case "OBJECT" : Return "Object"

                Case Else
                    Return Nothing

            End Select
        End Function

    End Class

End Namespace

