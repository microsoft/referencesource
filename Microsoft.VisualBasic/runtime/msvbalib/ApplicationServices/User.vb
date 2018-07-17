' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict On

Imports Microsoft.VisualBasic.CompilerServices
Imports System
Imports System.Collections
Imports System.ComponentModel
Imports System.Diagnostics
Imports System.Security.Permissions
Imports System.Security.Principal

Namespace Microsoft.VisualBasic.ApplicationServices

    '''************************************************************************
    ''';User
    ''' <summary>
    ''' Class abstracting the computer user
    ''' </summary>
    ''' <remarks></remarks>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class User

        '==PUBLIC**************************************************************

        '''********************************************************************
        ''';New
        ''' <summary>
        ''' Creates an instance of User
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
        End Sub

        '''********************************************************************
        ''';Name
        ''' <summary>
        ''' The name of the current user
        ''' </summary>
        ''' <remarks></remarks>
        Public ReadOnly Property Name() As String
            Get
                Return InternalPrincipal.Identity.Name
            End Get
        End Property

        '''********************************************************************
        ''';CurrentPrincipal
        ''' <summary>
        ''' The current IPrincipal which represents the current user
        ''' </summary>
        ''' <value>An IPrincipal representing the current user</value>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> Public Property CurrentPrincipal() As IPrincipal
            Get
                Return InternalPrincipal
            End Get
            Set(ByVal value As IPrincipal)
                InternalPrincipal = value
            End Set
        End Property

        '''********************************************************************
        ''';InitializeWithWindowsUser
        ''' <summary>
        ''' Sets My.User to point at the logged on windows user.
        ''' </summary>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> Public Sub InitializeWithWindowsUser()
            System.Threading.Thread.CurrentPrincipal = New System.Security.Principal.WindowsPrincipal(System.Security.Principal.WindowsIdentity.GetCurrent)
        End Sub

        '''********************************************************************
        ''';IsAuthenticated
        ''' <summary>
        ''' Indicates whether or not the current user has been authenticated.
        ''' </summary>
        ''' <remarks></remarks>
        Public ReadOnly Property IsAuthenticated() As Boolean
            Get
                Return InternalPrincipal.Identity.IsAuthenticated
            End Get
        End Property

        '''********************************************************************
        ''';IsInRole
        ''' <summary>
        ''' Indicates whether or not the current user is a member of the passed in role
        ''' </summary>
        ''' <param name="role">The name of the role</param>
        ''' <returns>True if the user is a member of the role otherwise False</returns>
        ''' <remarks></remarks>
        Public Function IsInRole(ByVal role As String) As Boolean
            Return InternalPrincipal.IsInRole(role)
        End Function

        '''********************************************************************
        ''';IsInRole
        ''' <summary>
        ''' Indicates whether or not the current user is a member of the passed in built in role
        ''' </summary>
        ''' <param name="role">An enum representing a built in role</param>
        ''' <returns>True if the user is a member of the role otherwise False</returns>
        ''' <remarks>
        ''' For windows users, the built in roles map to WindowsBuiltInRoles. For non windows, the
        ''' built in roles map to the name of the role (ie BuiltInRole.Administrator maps to "Administrator")
        ''' </remarks>
        Public Function IsInRole(ByVal role As BuiltInRole) As Boolean
            ValidateBuiltInRoleEnumValue(role, "role")

            Dim converter As TypeConverter = TypeDescriptor.GetConverter(GetType(BuiltInRole))
            If IsWindowsPrincipal() Then
                Dim windowsRole As WindowsBuiltInRole = DirectCast(converter.ConvertTo(role, GetType(WindowsBuiltInRole)), WindowsBuiltInRole)
                Return DirectCast(InternalPrincipal, WindowsPrincipal).IsInRole(windowsRole)
            Else
                Return InternalPrincipal.IsInRole(converter.ConvertToString(role))
            End If
        End Function

        '==PROTECTED***********************************************************

        ''';InternalPrincipal
        ''' <summary>
        ''' The principal representing the current user.
        ''' </summary>
        ''' <value>An IPrincipal representing the current user</value>
        ''' <remarks> 
        ''' This should be overriden by derived classes that don't get the current
        ''' user from the current thread
        ''' </remarks>
        Protected Overridable Property InternalPrincipal() As IPrincipal
            Get
                Return System.Threading.Thread.CurrentPrincipal
            End Get
            Set(ByVal value As IPrincipal)
                System.Threading.Thread.CurrentPrincipal = value
            End Set
        End Property

        '==PRIVATE************************************************************

        '''*******************************************************************
        ''';IsWindowsPrincipal
        ''' <summary>
        ''' Indicates whether or not the current principal is a WindowsPrincipal
        ''' </summary>
        ''' <returns>True if the current principal is a WindowsPrincipal, otherwise False</returns>
        ''' <remarks></remarks>
        Private Function IsWindowsPrincipal() As Boolean
            Return TypeOf InternalPrincipal Is WindowsPrincipal
        End Function

        '''****************************************************************
        ''';ValidateBuiltInRoleEnumValue
        ''' <summary>
        ''' Determine if a value passed as a BuiltInRole enum is a legal BuiltInRole 
        ''' enum value
        ''' </summary>
        ''' <param name="testMe"></param>
        ''' <remarks></remarks>
        Friend Shared Sub ValidateBuiltInRoleEnumValue(ByVal testMe As BuiltInRole, ByVal parameterName As String)
            'Can't do a range check because the enum represents non-sequential values from all over
            If testMe = BuiltInRole.AccountOperator OrElse _
                testMe = BuiltInRole.Administrator OrElse _
                testMe = BuiltInRole.BackupOperator OrElse _
                testMe = BuiltInRole.Guest OrElse _
                testMe = BuiltInRole.PowerUser OrElse _
                testMe = BuiltInRole.PrintOperator OrElse _
                testMe = BuiltInRole.Replicator OrElse _
                testMe = BuiltInRole.SystemOperator OrElse _
                testMe = BuiltInRole.User Then
                Return 'it's good
            End If
            Throw New System.ComponentModel.InvalidEnumArgumentException(parameterName, CType(testMe, Integer), GetType(BuiltInRole))
        End Sub

    End Class 'User

    '''**********************************************************************
    ''';BuiltInRole
    ''' <summary>
    ''' An enum of built in roles
    ''' </summary>
    ''' <remarks>These map to the WindowsBuiltInRoles</remarks>
    <TypeConverter(GetType(BuiltInRoleConverter))> _
    Public Enum BuiltInRole As Integer
        '!!!!!!!!! Any changes to this enum must have an accompanying change made to User::ValidateBuiltInRoleEnumValue()
        AccountOperator = WindowsBuiltInRole.AccountOperator
        Administrator = WindowsBuiltInRole.Administrator
        BackupOperator = WindowsBuiltInRole.BackupOperator
        Guest = WindowsBuiltInRole.Guest
        PowerUser = WindowsBuiltInRole.PowerUser
        PrintOperator = WindowsBuiltInRole.PrintOperator
        Replicator = WindowsBuiltInRole.Replicator
        SystemOperator = WindowsBuiltInRole.SystemOperator
        User = WindowsBuiltInRole.User
    End Enum

    '''**********************************************************************
    ''';BuiltInRoleConverter
    ''' <summary>
    ''' Enables converting BuiltInRoles to WindowsBuiltInRoles
    ''' </summary>
    ''' <remarks></remarks>
    <HostProtection(SharedState:=True)> _
    Public Class BuiltInRoleConverter
        Inherits TypeConverter

        '==PUBLIC************************************************************

        '''******************************************************************
        ''';New
        ''' <summary>
        ''' Creates converter
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
            MyBase.New()
        End Sub

        '''******************************************************************
        ''';CanConvertTo
        ''' <summary>
        ''' Extends the default TypeConverter to indicate we can convert to WindowsBuiltInRoles
        ''' </summary>
        ''' <param name="context"></param>
        ''' <param name="destinationType"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        Public Overrides Function CanConvertTo(ByVal context As System.ComponentModel.ITypeDescriptorContext, ByVal destinationType As System.Type) As Boolean
            If destinationType IsNot Nothing AndAlso destinationType.Equals(GetType(WindowsBuiltInRole)) Then
                Return True
            End If

            Return MyBase.CanConvertTo(context, destinationType)
        End Function

        '''******************************************************************
        ''';ConvertTo
        ''' <summary>
        ''' Extends the default TypeConvert to enable converting to WindowsBuiltInRoles
        ''' </summary>
        ''' <param name="context"></param>
        ''' <param name="culture"></param>
        ''' <param name="value"></param>
        ''' <param name="destinationType"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        Public Overrides Function ConvertTo(ByVal context As System.ComponentModel.ITypeDescriptorContext, ByVal culture As System.Globalization.CultureInfo, ByVal value As Object, ByVal destinationType As System.Type) As Object
            If destinationType IsNot Nothing AndAlso destinationType.Equals(GetType(WindowsBuiltInRole)) Then
                User.ValidateBuiltInRoleEnumValue(DirectCast(value, BuiltInRole), "value")
                Return GetWindowsBuiltInRole(value)
            End If
            Return MyBase.ConvertTo(context, culture, value, destinationType)
        End Function

        '==PRIVATE*********************************************************

        '''****************************************************************
        ''';GetWindowsBuiltInRole
        ''' <summary>
        ''' Returns the WindowsBuiltInRole that corresponds to the passed in BuiltInRole
        ''' </summary>
        ''' <param name="role"></param>
        ''' <returns>The WindowsBuiltInrole</returns>
        ''' <remarks></remarks>
        Private Function GetWindowsBuiltInRole(ByVal role As Object) As WindowsBuiltInRole
            Dim roleName As String = [Enum].GetName(GetType(BuiltInRole), role)
            Dim windowsRole As Object = [Enum].Parse(GetType(WindowsBuiltInRole), roleName)
            If windowsRole IsNot Nothing Then
                Return DirectCast(windowsRole, WindowsBuiltInRole)
            End If
        End Function
    End Class 'BuiltInRoleConverter

End Namespace

