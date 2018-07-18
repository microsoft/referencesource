' Copyright (c) Microsoft Corporation.  All rights reserved.

Option Explicit On
Option Strict On

Imports System
Imports System.Diagnostics
Imports System.Windows.Forms
Imports System.ComponentModel
Imports System.Security
Imports System.Security.Permissions
Imports System.Runtime.Versioning
Imports Microsoft.VisualBasic.CompilerServices

Namespace Microsoft.VisualBasic.Devices

    '*****************************************************************************
    ';Keyboard
    '
    'Remarks: A class representing a computer keyboard. Enables discovery of key
    '         state information for the most common scenarios and enables SendKeys 
    '*****************************************************************************
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class Keyboard

        '''*****************************************************************************
        ''' ;SendKeys
        ''' <summary>
        ''' Sends keys to the active window as if typed as keyboard with wait = false.
        ''' </summary>
        ''' <param name="keys">A string containing the keys to be sent (typed).</param>
        ''' <remarks>VSWhidbey 404397.</remarks>
        Public Sub SendKeys(ByVal keys As String)
            SendKeys(keys, False)
        End Sub

        '*****************************************************************************
        ';SendKeys
        '
        'Summary: Sends keys to the active window as if typed at keyboard. This overloaded
        '         version uses the same conventions as the VB6 SendKeys.
        '  Param: Keys - A string containing the keys to be sent (typed).
        '  Param: Wait - Wait for messages to be processed before returning.
        '*****************************************************************************
        Public Sub SendKeys(ByVal keys As String, ByVal wait As Boolean)
            If wait Then
                System.Windows.Forms.SendKeys.SendWait(keys)
            Else
                System.Windows.Forms.SendKeys.Send(keys)
            End If
        End Sub

        '*****************************************************************************
        ';ShiftDown
        '
        'Summary: Gets the state (up or down) of the Shift key.
        'Returns: True if the key is down otherwise false.
        '*****************************************************************************
        Public ReadOnly Property ShiftKeyDown() As Boolean
            Get
                Dim Keys As Keys = Control.ModifierKeys
                Return CType(Keys And Keys.Shift, Boolean)
            End Get
        End Property

        '*****************************************************************************
        ';AltDown
        '
        'Summary: Gets the state (up or down) of the Alt key.
        'Returns: True if the key is down otherwise false.
        '*****************************************************************************
        Public ReadOnly Property AltKeyDown() As Boolean
            Get
                Dim Keys As Keys = Control.ModifierKeys
                Return CType(Keys And Keys.Alt, Boolean)
            End Get
        End Property

        '*****************************************************************************
        ';CtrlDown
        '
        'Summary: Gets the state (up or down) of the Ctrl key.
        'Returns: True if the key is down otherwise false.
        '*****************************************************************************
        Public ReadOnly Property CtrlKeyDown() As Boolean
            Get
                Dim Keys As Keys = Control.ModifierKeys
                Return CType(Keys And Keys.Control, Boolean)
            End Get
        End Property

        '*****************************************************************************
        ';CapsLock
        '
        'Summary: Gets the toggle state of the Caps Lock key.
        'Returns: True if the key is on otherwise false.
        '*****************************************************************************
        Public ReadOnly Property CapsLock() As Boolean
            <SecuritySafeCritical()> _
            <ResourceExposure(ResourceScope.None)> _
            <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
            Get
                'Security Note: Only the state of the Caps Lock is returned

                'The low order byte of the return value from GetKeyState is 1 if the key is
                'toggled on.
                Return CType((UnsafeNativeMethods.GetKeyState(Keys.CapsLock) And 1), Boolean)
            End Get
        End Property

        '*****************************************************************************
        ';NumLock
        '
        'Summary: Gets the toggle state of the Num Lock key.
        'Returns: True if the key is on otherwise false.
        '*****************************************************************************
        Public ReadOnly Property NumLock() As Boolean
            <SecuritySafeCritical()> _
            <ResourceExposure(ResourceScope.None)> _
            <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
            Get
                'Security Note: Only the state of the Num Lock is returned

                'The low order byte of the return value from GetKeyState is 1 if the key is
                'toggled on.
                Return CType((UnsafeNativeMethods.GetKeyState(Keys.NumLock) And 1), Boolean)
            End Get
        End Property

        '*****************************************************************************
        ';ScrollLock
        '
        'Summary: Gets the toggle state of the Scroll Lock key.
        'Returns: True if the key is on otherwise false.
        '*****************************************************************************
        Public ReadOnly Property ScrollLock() As Boolean
            <SecuritySafeCritical()> _
            <ResourceExposure(ResourceScope.None)> _
            <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
            Get
                'Security Note: Only the state of the Scroll Lock is returned

                'The low order byte of the return value from GetKeyState is 1 if the key is
                'toggled on.
                Return CType((UnsafeNativeMethods.GetKeyState(Keys.Scroll) And 1), Boolean)
            End Get
        End Property

        '* FRIEND **************************************************************************

        '* PRIVATE **************************************************************************

    End Class

End Namespace
