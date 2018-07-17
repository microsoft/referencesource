' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Collections
Imports System.Diagnostics
Imports System.Management
Imports System.Runtime.InteropServices
Imports System.Security
Imports System.Security.Permissions
Imports System.Runtime.Versioning
Imports Microsoft.VisualBasic.CompilerServices

Namespace Microsoft.VisualBasic.Devices

    '''*************************************************************************
    ''' ;ComputerInfo
    ''' <summary>
    ''' Provides configuration information about the current computer and the current process.
    ''' </summary>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    <DebuggerTypeProxy(GetType(ComputerInfo.ComputerInfoDebugView))> _
    Public Class ComputerInfo

        '!!!!!!!!!!!!!! Keep the debugger proxy current as you change this class - see the nested ComputerInfoDebugView below !!!!!!!!!!!!!!!!!

        '= PUBLIC =============================================================

        '''******************************************************************************
        ''' ;New
        ''' <summary>
        ''' Default ctor
        ''' </summary>
        <SecuritySafeCritical()> _	
        Sub New()
        End Sub

        '''******************************************************************************
        ''' ;TotalPhysicalMemory
        ''' <summary>
        ''' Gets the total size of physical memory on the machine.
        ''' </summary>
        ''' <value>A 64-bit unsigned integer containing the size of total physical memory on the machine, in bytes.</value>
        ''' <exception cref="System.ComponentModel.Win32Exception">If we are unable to obtain the memory status.</exception>
        <CLSCompliant(False)> _
        Public ReadOnly Property TotalPhysicalMemory() As UInt64
            <SecuritySafeCritical()> _
            Get
                Return MemoryStatus.TotalPhysicalMemory
            End Get
        End Property

        '''******************************************************************************
        ''' ;AvailablePhysicalMemory
        ''' <summary>
        ''' Gets the total size of free physical memory on the machine.
        ''' </summary>
        ''' <value>A 64-bit unsigned integer containing the size of free physical memory on the machine, in bytes.</value>
        ''' <exception cref="System.ComponentModel.Win32Exception">If we are unable to obtain the memory status.</exception>
        <CLSCompliant(False)> _
        Public ReadOnly Property AvailablePhysicalMemory() As UInt64
            <SecuritySafeCritical()> _
            Get
                Return MemoryStatus.AvailablePhysicalMemory
            End Get
        End Property

        '''******************************************************************************
        ''' ;TotalVirtualMemory
        ''' <summary>
        ''' Gets the total size of user potion of virtual address space for calling process.
        ''' </summary>
        ''' <value>A 64-bit unsigned integer containing the size of user potion of virtual address space for calling process, 
        '''          in bytes.</value>
        ''' <exception cref="System.ComponentModel.Win32Exception">If we are unable to obtain the memory status.</exception>
        <CLSCompliant(False)> _
        Public ReadOnly Property TotalVirtualMemory() As UInt64
            <SecuritySafeCritical()> _
            Get
                Return MemoryStatus.TotalVirtualMemory
            End Get
        End Property

        '''******************************************************************************
        ''' ;AvailableVirtualMemory
        ''' <summary>
        ''' Gets the total size of free user potion of virtual address space for calling process.
        ''' </summary>
        ''' <value>A 64-bit unsigned integer containing the size of free user potion of virtual address space for calling process, 
        '''          in bytes.</value>
        ''' <exception cref="System.ComponentModel.Win32Exception">If we are unable to obtain the memory status.</exception>
        <CLSCompliant(False)> _
        Public ReadOnly Property AvailableVirtualMemory() As UInt64
            <SecuritySafeCritical()> _
            Get
                Return MemoryStatus.AvailableVirtualMemory
            End Get
        End Property

        '''******************************************************************************
        ''' ;InstalledUICulture
        ''' <summary>
        ''' Gets the current UICulture installed on the machine.
        ''' </summary>
        ''' <value>A CultureInfo object represents the UI culture installed on the machine.</value>
        Public ReadOnly Property InstalledUICulture() As Globalization.CultureInfo
            Get
                Return Globalization.CultureInfo.InstalledUICulture
            End Get
        End Property

        '''******************************************************************************
        ''' ;OSFullName
        ''' <summary>
        ''' Gets the full operating system name. This method requires full trust and WMI installed.
        ''' </summary>
        ''' <value>A string contains the operating system name.</value>
        ''' <exception cref="System.Security.SecurityException">If the immediate caller does not have full trust.</exception>
        ''' <exception cref="System.InvalidOperationException">If we cannot obtain the query object from WMI.</exception>
        ''' <remarks>Since this property depends on WMI, we have OSPlatform property that does not require WMI.</remarks>
        Public ReadOnly Property OSFullName() As String
            <SecuritySafeCritical()> _
            Get
                Try
                    ' There is no PInvoke Call for this purpose, have to use WMI.
                    ' The result from WMI is 'MS Windows xxx|C:\WINNT\Device\Harddisk0\Partition1.
                    ' We only show the first part. NOTE: This is fragile.
                    Dim PropertyName As String = "Name"
                    Dim Separator As Char = "|"c

                    Dim Result As String = CStr(OSManagementBaseObject.Properties(PropertyName).Value)
                    If Result.Contains(Separator) Then
                        Return Result.Substring(0, Result.IndexOf(Separator))
                    Else
                        Return Result
                    End If
                Catch ex As System.Runtime.InteropServices.COMException ' VSWhidbey 214588
                    Return OSPlatform
                End Try
            End Get
        End Property

        '''**************************************************************************
        ''' ;OSPlatform
        ''' <summary>
        ''' Gets the platform OS name.
        ''' </summary>
        ''' <value>A string containing a Platform ID like "Win32NT", "Win32S", "Win32Windows". See PlatformID enum.</value>
        ''' <exception cref="System.ExecutionEngineException">If cannot obtain the OS Version information.</exception>
        Public ReadOnly Property OSPlatform() As String
            Get
                Return Environment.OSVersion.Platform.ToString
            End Get
        End Property

        '''******************************************************************************
        ''' ;OSVersion
        ''' <summary>
        ''' Get the current version number of the operating system.
        ''' </summary>
        ''' <value>A string contains the current version number of the operating system.</value>
        ''' <exception cref="System.ExecutionEngineException">If cannot obtain the OS Version information.</exception>
        Public ReadOnly Property OSVersion() As String
            Get
                Return Environment.OSVersion.Version.ToString
            End Get
        End Property

        '= FRIEND =============================================================

        '''******************************************************************************
        ''' ;ComputerInfoDebugView
        ''' <summary>
        ''' Debugger proxy for the ComputerInfo class.  The problem is that OSFullName can time out the debugger
        ''' so we offer a view that doesn't have that field. 
        ''' </summary>
        ''' <remarks></remarks>
        Friend NotInheritable Class ComputerInfoDebugView
            Public Sub New(ByVal RealClass As ComputerInfo)
                m_InstanceBeingWatched = RealClass
            End Sub

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property TotalPhysicalMemory() As UInt64
                Get
                    Return m_InstanceBeingWatched.TotalPhysicalMemory
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property AvailablePhysicalMemory() As UInt64
                Get
                    Return m_InstanceBeingWatched.AvailablePhysicalMemory
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property TotalVirtualMemory() As UInt64
                Get
                    Return m_InstanceBeingWatched.TotalVirtualMemory
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property AvailableVirtualMemory() As UInt64
                Get
                    Return m_InstanceBeingWatched.AvailableVirtualMemory
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property InstalledUICulture() As Globalization.CultureInfo
                Get
                    Return m_InstanceBeingWatched.InstalledUICulture
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property OSPlatform() As String
                Get
                    Return m_InstanceBeingWatched.OSPlatform
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.RootHidden)> _
            Public ReadOnly Property OSVersion() As String
                Get
                    Return m_InstanceBeingWatched.OSVersion
                End Get
            End Property

            <DebuggerBrowsable(DebuggerBrowsableState.Never)> Private m_InstanceBeingWatched As ComputerInfo
        End Class

        '= PRIVATE ============================================================

        '''******************************************************************************
        ''' ;MemoryStatus
        ''' <summary>
        ''' Get the whole memory information details.
        ''' </summary>
        ''' <value>An InternalMemoryStatus class.</value>
        Private ReadOnly Property MemoryStatus() As InternalMemoryStatus
            Get
                If m_InternalMemoryStatus Is Nothing Then
                    m_InternalMemoryStatus = New InternalMemoryStatus
                End If
                Return m_InternalMemoryStatus
            End Get
        End Property

        '''******************************************************************************
        ''' ;OSManagementBaseObject
        ''' <summary>
        ''' Get the management object used in WMI to query for the operating system name.
        ''' </summary>
        ''' <value>A ManagementBaseObject represents the result of "Win32_OperatingSystem" query.</value>
        ''' <exception cref="System.Security.SecurityException">If the immediate caller does not have full trust.</exception>
        ''' <exception cref="System.InvalidOperationException">If we cannot obtain the query object from WMI.</exception>
        Private ReadOnly Property OSManagementBaseObject() As ManagementBaseObject
            <SecurityCritical()> _
            Get
                ' Query string to get the OperatingSystem information.
                Dim QueryString As String = "Win32_OperatingSystem"

                ' Assumption: Each thread will have its own instance of App class so no need to SyncLock this.
                If m_OSManagementObject Is Nothing Then
                    ' Build a query for enumeration of Win32_OperatingSystem instances
                    Dim Query As New SelectQuery(QueryString)

                    ' Instantiate an object searcher with this query
                    Dim Searcher As New ManagementObjectSearcher(Query)

                    Dim ManagementObjCollection As ManagementObjectCollection = Searcher.Get

                    If ManagementObjCollection.Count > 0 Then
                        Debug.Assert(ManagementObjCollection.Count = 1, "Should find 1 instance only!!!")

                        Dim ManagementObjEnumerator As ManagementObjectCollection.ManagementObjectEnumerator = _
                            ManagementObjCollection.GetEnumerator
                        ManagementObjEnumerator.MoveNext()
                        m_OSManagementObject = ManagementObjEnumerator.Current
                    Else
                        Throw ExceptionUtils.GetInvalidOperationException(ResID.MyID.DiagnosticInfo_FullOSName)
                    End If
                End If

                Debug.Assert(m_OSManagementObject IsNot Nothing, "Null management object!!!")
                Return m_OSManagementObject
            End Get
        End Property

        <SecurityCritical()> _	
        Private m_OSManagementObject As ManagementBaseObject = Nothing ' Cache the management object gotten from WMI.
        Private m_InternalMemoryStatus As InternalMemoryStatus = Nothing ' Cache our InternalMemoryStatus

        '''******************************************************************************
        ''' ;InternalMemoryStatus
        ''' <summary>
        ''' This class makes the right call to GlobalMemoryStatus or GlobalMemoryStatusEx depending on Windows OS
        ''' and returns the correct value.
        ''' </summary>
        ''' <remarks>
        ''' VSWhidbey 304259: Need to call GlobalMemoryStatus on OS lower than W2k.
        ''' This table comes from http://support.microsoft.com/default.aspx?scid=kb;en-us;304283.
        '''+--------------------------------------------------------------+
        '''|           |Windows|Windows|Windows|Windows NT|Windows|Windows|
        '''|           |  95   |  98   |  Me   |    4.0   | 2000  |  XP   |
        '''+--------------------------------------------------------------+
        '''|PlatformID | 1     | 1     | 1     | 2        | 2     | 2     |
        '''+--------------------------------------------------------------+
        '''|Major      |       |       |       |          |       |       |
        '''| version   | 4     | 4     | 4     | 4        | 5     | 5     |
        '''+--------------------------------------------------------------+
        '''|Minor      |       |       |       |          |       |       |
        '''| version   | 0     | 10    | 90    | 0        | 0     | 1     |
        '''+--------------------------------------------------------------+
        ''' </remarks>
        Private Class InternalMemoryStatus
            Friend Sub New()
            End Sub

            Friend ReadOnly Property TotalPhysicalMemory() As UInt64
                <SecurityCritical()> _
                Get
                    Refresh()
                    If m_IsOldOS Then
                        Return CType(m_MemoryStatus.dwTotalPhys, UInt64)
                    Else
                        Return m_MemoryStatusEx.ullTotalPhys
                    End If
                End Get
            End Property

            Friend ReadOnly Property AvailablePhysicalMemory() As UInt64
                <SecurityCritical()> _
                Get
                    Refresh()
                    If m_IsOldOS Then
                        Return CType(m_MemoryStatus.dwAvailPhys, UInt64)
                    Else
                        Return m_MemoryStatusEx.ullAvailPhys
                    End If
                End Get
            End Property

            Friend ReadOnly Property TotalVirtualMemory() As UInt64
                <SecurityCritical()> _
                Get
                    Refresh()
                    If m_IsOldOS Then
                        Return CType(m_MemoryStatus.dwTotalVirtual, UInt64)
                    Else
                        Return m_MemoryStatusEx.ullTotalVirtual
                    End If
                End Get
            End Property

            Friend ReadOnly Property AvailableVirtualMemory() As UInt64
                <SecurityCritical()> _
                Get
                    Refresh()
                    If m_IsOldOS Then
                        Return CType(m_MemoryStatus.dwAvailVirtual, UInt64)
                    Else
                        Return m_MemoryStatusEx.ullAvailVirtual
                    End If
                End Get
            End Property

            <SecurityCritical()> _
            <ResourceExposure(ResourceScope.None)> _
            <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
            Private Sub Refresh()
                If (m_IsOldOS) Then
                    m_MemoryStatus = New NativeMethods.MEMORYSTATUS
                    NativeMethods.GlobalMemoryStatus(m_MemoryStatus)
                Else
                    m_MemoryStatusEx = New NativeMethods.MEMORYSTATUSEX
                    m_MemoryStatusEx.Init()
                    If (Not NativeMethods.GlobalMemoryStatusEx(m_MemoryStatusEx)) Then
                        Throw ExceptionUtils.GetWin32Exception(ResID.MyID.DiagnosticInfo_Memory)
                    End If
                End If
            End Sub

            ' Are we on Windows with Major Version < 5 (NT4.0, Me, 98, 95)? 
            Private m_IsOldOS As Boolean = System.Environment.OSVersion.Version.Major < 5
            Private m_MemoryStatus As NativeMethods.MEMORYSTATUS
            Private m_MemoryStatusEx As NativeMethods.MEMORYSTATUSEX
        End Class
    End Class

End Namespace
