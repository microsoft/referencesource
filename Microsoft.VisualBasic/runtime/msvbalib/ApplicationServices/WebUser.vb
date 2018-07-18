' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict Off

Imports Microsoft.VisualBasic.CompilerServices
Imports System
Imports System.Security.Permissions
Imports System.Security.Principal

Namespace Microsoft.VisualBasic.ApplicationServices

    '''************************************************************************
    ''';WebUser
    ''' <summary>
    ''' Class abstracting a web application user
    ''' </summary>
    ''' <remarks></remarks>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class WebUser
        Inherits User

        '==PUBLIC**************************************************************

        '''********************************************************************
        ''';New
        ''' <summary>
        ''' Creates an instance of a WebUser
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
        End Sub

        '==PROTECTED************************************************************

        '''*********************************************************************
        ''';InternalPrincipal
        ''' <summary>
        ''' Gets the current user from the HTTPContext
        ''' </summary>
        ''' <value>An IPrincipal representing the current user</value>
        ''' <remarks></remarks>
        Protected Overrides Property InternalPrincipal() As IPrincipal
            Get
		Dim httpContext As Object = Microsoft.VisualBasic.MyServices.Internal.SkuSafeHttpContext.Current()
                If httpContext Is Nothing Then
		    Throw ExceptionUtils.GetInvalidOperationException(CompilerServices.ResID.WebNotSupportedOnThisSKU)
		Else
                    Return httpContext.User
		End If
            End Get
            Set(ByVal value As IPrincipal)
		Dim httpContext As Object = Microsoft.VisualBasic.MyServices.Internal.SkuSafeHttpContext.Current()
                If httpContext Is Nothing Then
		    Throw ExceptionUtils.GetInvalidOperationException(CompilerServices.ResID.WebNotSupportedOnThisSKU)
		Else
		    httpContext.User = value
                End If
            End Set
        End Property

    End Class
End Namespace
