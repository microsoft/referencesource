' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict On

Imports System
Imports System.Collections
Imports System.Collections.Generic
Imports System.Collections.ObjectModel
Imports System.Diagnostics
Imports System.IO.Ports
Imports System.Security.Permissions
Imports System.Text
Imports Microsoft.VisualBasic.CompilerServices

Namespace Microsoft.VisualBasic.Devices

    '''*************************************************************************
    ''';Ports
    ''' <summary>
    '''  Gives access to Ports on the local machine
    ''' </summary>
    ''' <remarks>Only serial ports are supported at present, but this class may expand in the future</remarks>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class Ports

        '==PUBLIC**************************************************************

        '''*********************************************************************
        ''';New
        ''' <summary>
        '''  Constructor
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
        End Sub

        '''*********************************************************************
        ''';OpenSerialPort
        ''' <summary>
        '''  Creates an opens a SerialPort object
        ''' </summary>
        ''' <param name="portName">The name of the port to open</param>
        ''' <returns>An opened SerialPort</returns>
        ''' <remarks>
        '''  We delegate all validation to the fx SerialPort class. We open the port so exceptions will
        '''  be thrown as quickly as possible
        '''</remarks>
        Public Function OpenSerialPort(ByVal portName As String) As SerialPort
            Dim Port As New SerialPort(portName)
            Port.Open()
            Return Port
        End Function

        '''*********************************************************************
        ''';OpenSerialPort
        ''' <summary>
        '''  Creates an opens a SerialPort object
        ''' </summary>
        ''' <param name="portName">The name of the port to open</param>
        ''' <param name="baudRate">The baud rate of the port</param>
        ''' <returns>An opened SerialPort</returns>
        ''' <remarks>
        '''  We delegate all validation to the fx SerialPort class. We open the port so exceptions will
        '''  be thrown as quickly as possible
        '''</remarks>
        Public Function OpenSerialPort(ByVal portName As String, ByVal baudRate As Integer) As SerialPort
            Dim Port As New SerialPort(portName, baudRate)
            Port.Open()
            Return Port
        End Function

        '''*********************************************************************
        ''';OpenSerialPort
        ''' <summary>
        '''  Creates an opens a SerialPort object
        ''' </summary>
        ''' <param name="portName">The name of the port to open</param>
        ''' <param name="baudRate">The baud rate of the port</param>
        ''' <param name="parity">The parity of the port</param>
        ''' <returns>An opened SerialPort</returns>
        ''' <remarks>
        '''  We delegate all validation to the fx SerialPort class. We open the port so exceptions will
        '''  be thrown as quickly as possible
        '''</remarks>
        Public Function OpenSerialPort(ByVal portName As String, ByVal baudRate As Integer, ByVal parity As Parity) As SerialPort
            Dim Port As New SerialPort(portName, baudRate, parity)
            Port.Open()
            Return Port
        End Function

        '''*********************************************************************
        ''';OpenSerialPort
        ''' <summary>
        '''  Creates an opens a SerialPort object
        ''' </summary>
        ''' <param name="portName">The name of the port to open</param>
        ''' <param name="baudRate">The baud rate of the port</param>
        ''' <param name="parity">The parity of the port</param>
        ''' <param name="dataBits">The data bits of the port</param>
        ''' <returns>An opened SerialPort</returns>
        ''' <remarks>
        '''  We delegate all validation to the fx SerialPort class. We open the port so exceptions will
        '''  be thrown as quickly as possible
        '''</remarks>
        Public Function OpenSerialPort(ByVal portName As String, ByVal baudRate As Integer, ByVal parity As Parity, ByVal dataBits As Integer) As SerialPort
            Dim Port As New SerialPort(portName, baudRate, parity, dataBits)
            Port.Open()
            Return Port
        End Function

        '''*********************************************************************
        ''';OpenSerialPort
        ''' <summary>
        '''  Creates an opens a SerialPort object
        ''' </summary>
        ''' <param name="portName">The name of the port to open</param>
        ''' <param name="baudRate">The baud rate of the port</param>
        ''' <param name="parity">The parity of the port</param>
        ''' <param name="dataBits">The data bits of the port</param>
        ''' <param name="stopBits">The stop bit setting of the port</param>
        ''' <returns>An opened SerialPort</returns>
        ''' <remarks>
        '''  We delegate all validation to the fx SerialPort class. We open the port so exceptions will
        '''  be thrown as quickly as possible
        '''</remarks>
        Public Function OpenSerialPort(ByVal portName As String, ByVal baudRate As Integer, ByVal parity As Parity, ByVal dataBits As Integer, ByVal stopBits As StopBits) As SerialPort
            Dim Port As New SerialPort(portName, baudRate, parity, dataBits, stopBits)
            Port.Open()
            Return Port
        End Function

        '''*********************************************************************
        ''';SerialPortNames
        ''' <summary>
        '''  Returns the names of the serial ports on the local machine
        ''' </summary>
        ''' <value>A collection of the names of the serial ports</value>
        ''' <remarks></remarks>
        Public ReadOnly Property SerialPortNames() As ReadOnlyCollection(Of String)
            Get
                Dim names() As String = SerialPort.GetPortNames()
                Dim namesList As New List(Of String)

                For Each portName As String in names
                    namesList.Add(portName)
                Next
                
                Return New ReadOnlyCollection(Of String)(namesList)
            End Get
        End Property

        '==PRIVATE*************************************************************

    End Class
End Namespace
