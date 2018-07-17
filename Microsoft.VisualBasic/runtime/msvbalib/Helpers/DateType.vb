' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Globalization
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#Region " BACKWARDS COMPATIBILITY "

    'WARNING WARNING WARNING WARNING WARNING
    'This code exists to support Everett compiled applications.  Make sure you understand
    'the backwards compatibility ramifications of any edit you make in this region.
    'WARNING WARNING WARNING WARNING WARNING

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class DateType
        ' Prevent creation.
        Private Sub New()
        End Sub

        Public Shared Function FromString(ByVal Value As String) As Date
            Return DateType.FromString(Value, GetCultureInfo())
        End Function

        Public Shared Function FromString(ByVal Value As String, ByVal culture As Globalization.CultureInfo) As Date
            Dim ParsedDate As System.DateTime

            If TryParse(Value, ParsedDate) Then
                Return ParsedDate
            Else
                'Truncate the string to 32 characters for the message
                Throw New InvalidCastException(GetResourceString(ResID.InvalidCast_FromStringTo, Left(Value, 32), "Date"))
            End If
        End Function

        Public Shared Function FromObject(ByVal Value As Object) As Date

            If Value Is Nothing Then
                Exit Function
            End If

            Dim ValueInterface As IConvertible
            Dim ValueTypeCode As TypeCode

            ValueInterface = TryCast(Value, IConvertible)

            If Not ValueInterface Is Nothing Then

                ValueTypeCode = ValueInterface.GetTypeCode()

                Select Case ValueTypeCode
                    Case TypeCode.DateTime
                        Return ValueInterface.ToDateTime(Nothing)

                    Case TypeCode.String
                        Return DateType.FromString(ValueInterface.ToString(Nothing), GetCultureInfo())

                    Case TypeCode.Boolean, _
                         TypeCode.Byte, _
                         TypeCode.Int16, _
                         TypeCode.Int32, _
                         TypeCode.Int64, _
                         TypeCode.Single, _
                         TypeCode.Double, _
                         TypeCode.Decimal, _
                         TypeCode.Char
                        ' Fall through to error

                    Case Else
                        ' Fall through to error
                End Select

            End If

            Throw New InvalidCastException(GetResourceString(ResID.InvalidCast_FromTo, VBFriendlyName(Value), "Date"))
        End Function

        Friend Shared Function TryParse(ByVal Value As String, ByRef Result As System.DateTime) As Boolean
            Const ParseStyle As DateTimeStyles = _
                        DateTimeStyles.AllowWhiteSpaces Or _
                        DateTimeStyles.NoCurrentDateDefault
            Dim Culture As CultureInfo = GetCultureInfo()
            Return System.DateTime.TryParse(ToHalfwidthNumbers(Value, Culture), Culture, ParseStyle, Result)
        End Function

    End Class

#End Region

End Namespace
