' Copyright (c) Microsoft Corporation.  All rights reserved.

Option Strict On
Option Explicit On 

Imports System.ComponentModel
Imports System.Security.Permissions
Imports System.Windows.Forms
Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils

Namespace Microsoft.VisualBasic.Devices

    '''**************************************************************************
    ''' ;Mouse
    ''' <summary>
    ''' A wrapper object that acts as a discovery mechanism for finding 
    ''' information about the mouse on your computer such as whether the mouse
    ''' exists, the number of buttons, wheelscrolls details.
    ''' 
    ''' This class is a Singleton Class. See Common.Computer for details.
    ''' </summary>
    ''' <remarks></remarks>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class Mouse

        '= PUBLIC =============================================================

        '''**************************************************************************
        ''' ;ButtonsSwapped
        ''' <summary>
        ''' Gets a value indicating whether the functions of the left and right
        ''' mouses buttons have been swapped.
        ''' </summary>
        ''' <value>
        ''' true if the functions of the left and right mouse buttons are swapped. false otherwise. 
        ''' </value>
        ''' <exception cref="System.InvalidOperationException">If no mouse is installed.</exception>
        Public ReadOnly Property ButtonsSwapped() As Boolean
            Get
                If System.Windows.Forms.SystemInformation.MousePresent Then
                    Return SystemInformation.MouseButtonsSwapped
                Else
                    Throw GetInvalidOperationException(ResID.MyID.Mouse_NoMouseIsPresent)
                End If
            End Get
        End Property

        '''**************************************************************************
        ''' ;WheelExists
        ''' <summary>
        ''' Gets a value indicating whether a mouse with a mouse wheel is installed
        ''' </summary>
        ''' <value>true if a mouse with a mouse wheel is installed, false otherwise.</value>
        ''' <exception cref="System.InvalidOperationException">If no mouse is installed.</exception>
        Public ReadOnly Property WheelExists() As Boolean
            Get
                If System.Windows.Forms.SystemInformation.MousePresent Then
                    Return SystemInformation.MouseWheelPresent
                Else
                    Throw GetInvalidOperationException(ResID.MyID.Mouse_NoMouseIsPresent)
                End If
            End Get
        End Property

        '''**************************************************************************
        ''' ;WheelScrollLines
        ''' <summary>
        ''' Gets the number of lines to scroll when the mouse wheel is rotated.
        ''' </summary>
        ''' <value>The number of lines to scroll.</value>
        ''' <exception cref="System.InvalidOperationException">if no mouse is installed or no wheels exists.</exception>
        Public ReadOnly Property WheelScrollLines() As Integer
            Get
                If WheelExists Then
                    Return SystemInformation.MouseWheelScrollLines
                Else
                    Throw GetInvalidOperationException(ResID.MyID.Mouse_NoWheelIsPresent)
                End If
            End Get
        End Property

        '= FRIEND =============================================================

        '= PRIVATE ============================================================

    End Class 'Mouse
End Namespace
