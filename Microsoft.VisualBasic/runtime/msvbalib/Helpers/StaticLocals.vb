' Copyright (c) Microsoft Corporation.  All rights reserved.

Namespace Microsoft.VisualBasic.CompilerServices

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
    Public NotInheritable Class StaticLocalInitFlag
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <System.Serializable()> _
    Public NotInheritable Class StaticLocalInitFlag
#End If
        Public State As Short
    End Class

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)>
    Public NotInheritable Class IncompleteInitialization
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <System.Serializable()> _
    Public NotInheritable Class IncompleteInitialization
#End If

        Inherits System.Exception

#If Not TELESTO Then
        ' FxCop: deserialization constructor must be defined as private.
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Private Sub New(ByVal info As System.Runtime.Serialization.SerializationInfo, ByVal context As System.Runtime.Serialization.StreamingContext)
            MyBase.New(info, context)
        End Sub
#End If

#If TELESTO Then
        Public Sub New(ByVal message As String)
#Else
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Public Sub New(ByVal message As String)
#End If
            MyBase.New(message)
        End Sub

#If TELESTO Then
        Public Sub New(ByVal message As String, ByVal innerException As System.Exception)
#Else
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Public Sub New(ByVal message As String, ByVal innerException As System.Exception)
#End If
            MyBase.New(message, innerException)
        End Sub

        ' default constructor
        Public Sub New()
            MyBase.New()
        End Sub

    End Class

End Namespace
