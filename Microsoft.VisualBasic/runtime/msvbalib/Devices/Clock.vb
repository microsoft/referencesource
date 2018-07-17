' Copyright (c) Microsoft Corporation.  All rights reserved.

Option Strict On
Option Explicit On 

Imports System.ComponentModel
Imports System
Imports System.Security.Permissions

Namespace Microsoft.VisualBasic.Devices

    '''**************************************************************************
    ''' ;Clock
    ''' <summary>
    ''' A wrapper object that acts as a discovery mechanism to quickly find out
    ''' the current local time of the machine and the GMT time.
    ''' </summary>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class Clock

        '* PUBLIC *************************************************************

        '''**************************************************************************
        ''' ;LocalTime
        ''' <summary>
        ''' Gets a DateTime that is the current local date and time on this computer.
        ''' </summary>
        ''' <value>A DateTime whose value is the current date and time.</value>
        Public ReadOnly Property LocalTime() As DateTime
            Get
                Return DateTime.Now
            End Get
        End Property

        '''**************************************************************************
        ''' ;GmtTime
        ''' <summary>
        ''' Gets a DateTime that is the current local date and time on this 
        ''' computer expressed as GMT time.
        ''' </summary>
        ''' <value>A DateTime whose value is the current date and time expressed as GMT time.</value>
        ''' CONSIDER: Name of this property UtcTime for consistency with FX?
        Public ReadOnly Property GmtTime() As DateTime
            Get
                Return DateTime.UtcNow
            End Get
        End Property

        '''**************************************************************************
        ''' ;TickCount
        ''' <summary>
        ''' This property wraps the Environment.TickCount property to get the
        ''' number of milliseconds elapsed since the system started.
        ''' </summary>
        ''' <value>An Integer containing the amount of time in milliseconds.</value>
        Public ReadOnly Property TickCount() As Integer
            Get
                Return System.Environment.TickCount
            End Get
        End Property

        '* FRIEND *************************************************************

        '* PRIVATE ************************************************************

    End Class 'Clock
End Namespace
