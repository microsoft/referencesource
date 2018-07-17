' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports Microsoft.Win32
Imports System.ComponentModel
Imports System.Security.Permissions

Namespace Microsoft.VisualBasic.MyServices

    '''*************************************************************************
    ''' ;RegistryProxy
    ''' <summary>
    ''' An extremely thin wrapper around Microsoft.Win32.Registry to expose the type through My.
    ''' More details: http://ddwww/spectool/Documents/Whidbey/VB/RAD%20Framework/My%20Proxy.doc
    ''' </summary>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    <System.ComponentModel.EditorBrowsable(EditorBrowsableState.Never)> _
    Public Class RegistryProxy


        '= PUBLIC =============================================================

        Public ReadOnly Property CurrentUser() As RegistryKey
            Get
                Return Registry.CurrentUser
            End Get
        End Property

        Public ReadOnly Property LocalMachine() As RegistryKey
            Get
                Return Registry.LocalMachine
            End Get
        End Property

        Public ReadOnly Property ClassesRoot() As RegistryKey
            Get
                Return Registry.ClassesRoot
            End Get
        End Property

        Public ReadOnly Property Users() As RegistryKey
            Get
                Return Registry.Users
            End Get
        End Property

        Public ReadOnly Property PerformanceData() As RegistryKey
            Get
                Return Registry.PerformanceData
            End Get
        End Property

        Public ReadOnly Property CurrentConfig() As RegistryKey
            Get
                Return Registry.CurrentConfig
            End Get
        End Property

        <System.Obsolete("The DynData registry key works only on Win9x, which is not supported by this version of the .NET Framework.  Use the PerformanceData registry key instead.  This property will be removed from a future version of the framework.")> _
        Public ReadOnly Property DynData() As RegistryKey
            Get
                Return Nothing
            End Get
        End Property

        Public Function GetValue(ByVal keyName As String, ByVal valueName As String, _
            ByVal defaultValue As Object) As Object

            Return Registry.GetValue(keyName, valueName, defaultValue)
        End Function

        Public Sub SetValue(ByVal keyName As String, ByVal valueName As String, ByVal value As Object)
            Registry.SetValue(keyName, valueName, value)
        End Sub

        Public Sub SetValue(ByVal keyName As String, ByVal valueName As String, ByVal value As Object, _
            ByVal valueKind As Microsoft.Win32.RegistryValueKind)

            Registry.SetValue(keyName, valueName, value, valueKind)
        End Sub

        '= FRIEND =============================================================

        '''*************************************************************************
        ''' ;New
        ''' <summary>
        ''' Proxy class can only created by internal classes.
        ''' </summary>
        Friend Sub New()
        End Sub


    End Class
End Namespace

