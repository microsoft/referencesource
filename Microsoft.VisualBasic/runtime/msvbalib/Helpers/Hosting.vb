' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Security.Permissions

Namespace Microsoft.VisualBasic.CompilerServices

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public Interface IVbHost
        Function GetParentWindow() As System.Windows.Forms.IWin32Window
        Function GetWindowTitle() As String
    End Interface

    <HostProtection(Resources:=HostProtectionResource.SharedState)> _
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class HostServices

        Private Shared m_host As IVbHost

        Public Shared Property VBHost() As IVbHost
            Get
                Return m_host
            End Get

            Set(ByVal Value As IVbHost)
                m_host = Value
            End Set
        End Property

    End Class

End Namespace


