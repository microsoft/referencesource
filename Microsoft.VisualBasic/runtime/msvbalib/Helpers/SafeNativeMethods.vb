' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Security
Imports System.Security.Permissions
Imports System.Text
Imports System.Runtime.InteropServices
Imports System.Runtime.Versioning

Namespace Microsoft.VisualBasic.CompilerServices

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <SecurityCritical()> _
    <ComVisible(False)> _
    <SuppressUnmanagedCodeSecurityAttribute()> _
    Friend NotInheritable Class _
        SafeNativeMethods

        <ResourceExposure(ResourceScope.None)> _
        <PreserveSig()> Friend Declare Function _
            IsWindowEnabled _
                Lib "user32" (ByVal hwnd As IntPtr) As <MarshalAs(UnmanagedType.Bool)> Boolean

        <ResourceExposure(ResourceScope.None)> _
        <PreserveSig()> Friend Declare Function _
            IsWindowVisible _
                Lib "user32" (ByVal hwnd As IntPtr) As <MarshalAs(UnmanagedType.Bool)> Boolean

        <ResourceExposure(ResourceScope.None)> _
        <PreserveSig()> Friend Declare Function _
            GetWindowThreadProcessId _
                Lib "user32" (ByVal hwnd As IntPtr, ByRef lpdwProcessId As Integer) As Integer

        <ResourceExposure(ResourceScope.None)> _
        <PreserveSig()> Friend Declare Sub _
            GetLocalTime _
                Lib "kernel32" (ByVal systime As NativeTypes.SystemTime)

        '''*************************************************************************
        ''' ;New
        ''' <summary>
        ''' FxCop violation: Avoid uninstantiated internal class. 
        ''' Adding a private constructor to prevent the compiler from generating a default constructor.
        ''' </summary>
        Private Sub New()
        End Sub
    End Class

End Namespace

