' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System

Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#Region " BACKWARDS COMPATIBILITY "

    'WARNING WARNING WARNING WARNING WARNING
    'This code exists to support Everett compiled applications.  Make sure you understand
    'the backwards compatibility ramifications of any edit you make in this region.
    'WARNING WARNING WARNING WARNING WARNING

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class CharType
        ' Prevent creation.
        Private Sub New()
        End Sub

        Public Shared Function FromString(ByVal Value As String) As Char
            If (Value Is Nothing) OrElse (Value.Length = 0) Then
                Return ControlChars.NullChar
            End If

            Return Value.Chars(0)
        End Function

        Public Shared Function FromObject(ByVal Value As Object) As Char

            If Value Is Nothing Then
                Return ChrW(0)
            End If

            Dim ValueInterface As IConvertible
            Dim ValueTypeCode As TypeCode

            ValueInterface = TryCast(Value, IConvertible)

            If Not ValueInterface Is Nothing Then

                ValueTypeCode = ValueInterface.GetTypeCode()

                Select Case ValueTypeCode
                    Case TypeCode.Char
                        Return ValueInterface.ToChar(Nothing)

                    Case TypeCode.String
                        Return CharType.FromString(ValueInterface.ToString(Nothing))

                    Case TypeCode.Boolean, _
                         TypeCode.Byte, _
                         TypeCode.Int16, _
                         TypeCode.Int32, _
                         TypeCode.Int64, _
                         TypeCode.Single, _
                         TypeCode.Double, _
                         TypeCode.Decimal, _
                         TypeCode.DateTime
                        ' Fall through to error

                    Case Else
                        ' Fall through to error
                End Select
            End If

            Throw New InvalidCastException(GetResourceString(ResID.InvalidCast_FromTo, VBFriendlyName(Value), "Char"))
        End Function

    End Class

#End Region

End Namespace


