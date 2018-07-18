' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Strict On
Option Infer On
Option Explicit On
Option Compare Binary

<Assembly: System.Reflection.AssemblyVersion("10.0.0.0")>

Namespace Global.Microsoft.VisualBasic
    Namespace CompilerServices
        <Global.System.Diagnostics.DebuggerNonUserCode()>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Public Class ProjectData
            Private Sub New()
            End Sub
            Public Overloads Shared Sub SetProjectError(ex As Global.System.Exception)
            End Sub
            Public Overloads Shared Sub SetProjectError(ex As Global.System.Exception, lErl As Integer)
            End Sub
            Public Shared Sub ClearProjectError()
            End Sub
        End Class

        <Global.System.Diagnostics.DebuggerNonUserCode()>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Partial Public Class Utils
            Public Shared Function CopyArray(arySrc As Global.System.Array, aryDest As Global.System.Array) As Global.System.Array
                If arySrc Is Nothing Then
                    Return aryDest
                End If
                Dim lLength As Integer
                lLength = arySrc.Length
                If lLength = 0 Then
                    Return aryDest
                End If
                If aryDest.Rank() <> arySrc.Rank() Then
                    Throw New Global.System.InvalidCastException()
                End If
                Dim iDim As Integer
                For iDim = 0 To aryDest.Rank() - 2
                    If aryDest.GetUpperBound(iDim) <> arySrc.GetUpperBound(iDim) Then
                        Throw New Global.System.ArrayTypeMismatchException()
                    End If
                Next iDim
                If lLength > aryDest.Length Then
                    lLength = aryDest.Length
                End If
                If arySrc.Rank > 1 Then
                    Dim LastRank As Integer = arySrc.Rank
                    Dim lenSrcLastRank As Integer = arySrc.GetLength(LastRank - 1)
                    Dim lenDestLastRank As Integer = aryDest.GetLength(LastRank - 1)
                    If lenDestLastRank = 0 Then
                        Return aryDest
                    End If
                    Dim lenCopy As Integer = If(lenSrcLastRank > lenDestLastRank, lenDestLastRank, lenSrcLastRank)
                    Dim i As Integer
                    For i = 0 To (arySrc.Length \ lenSrcLastRank) - 1
                        Global.System.Array.Copy(arySrc, i * lenSrcLastRank, aryDest, i * lenDestLastRank, lenCopy)
                    Next i
                Else
                    Global.System.Array.Copy(arySrc, aryDest, lLength)
                End If
                Return aryDest
            End Function
        End Class

        <Global.System.Diagnostics.DebuggerNonUserCode()>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Public Class StaticLocalInitFlag
            Public State As Short
        End Class

        <Global.System.Diagnostics.DebuggerNonUserCode()>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Public Class IncompleteInitialization
            Inherits Global.System.Exception
            Public Sub New()
                MyBase.New()
            End Sub
        End Class

        <Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Public Class DesignerGeneratedAttribute
            Inherits Global.System.Attribute
        End Class

        <Global.System.AttributeUsage(Global.System.AttributeTargets.Parameter, Inherited:=False)>
        <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
        Public Class OptionCompareAttribute
            Inherits Global.System.Attribute
        End Class

    End Namespace

    <Global.System.AttributeUsage(Global.System.AttributeTargets.Class, Inherited:=False)>
    <Global.System.ComponentModel.EditorBrowsable(Global.System.ComponentModel.EditorBrowsableState.Never)>
    Public Class HideModuleNameAttribute
        Inherits Global.System.Attribute
    End Class

    <Global.System.Diagnostics.DebuggerNonUserCode()>
    Public Module Constants
        Public Const vbCrLf As String = ChrW(13) & ChrW(10)
        Public Const vbNewLine As String = ChrW(13) & ChrW(10)
        Public Const vbCr As String = ChrW(13)
        Public Const vbLf As String = ChrW(10)
        Public Const vbBack As String = ChrW(8)
        Public Const vbFormFeed As String = ChrW(12)
        Public Const vbTab As String = ChrW(9)
        Public Const vbVerticalTab As String = ChrW(11)
        Public Const vbNullChar As String = ChrW(0)
        Public Const vbNullString As String = Nothing
    End Module
End Namespace
