' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Security
Imports System.Security.Permissions
Imports System.Text
Imports System.Runtime.ConstrainedExecution
Imports System.Runtime.InteropServices
Imports System.Runtime.Versioning

Namespace Microsoft.VisualBasic.CompilerServices

    <ComVisible(False)> _
    Friend NotInheritable Class NativeMethods

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <PreserveSig()> _
        Friend Declare Auto Function _
            WaitForInputIdle _
                Lib "user32" (ByVal Process As NativeTypes.LateInitSafeHandleZeroOrMinusOneIsInvalid, ByVal Milliseconds As Integer) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            GetWindow _
                Lib "user32" (ByVal hwnd As IntPtr, ByVal wFlag As Integer) As IntPtr

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            GetDesktopWindow _
                Lib "user32" () As IntPtr

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("user32", CharSet:=CharSet.Auto, PreserveSig:=True, SetLastError:=True)> _
        Friend Shared Function GetWindowText(ByVal hWnd As IntPtr, <Out(), MarshalAs(UnmanagedType.LPTStr)> ByVal lpString As StringBuilder, ByVal nMaxCount As Integer) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            AttachThreadInput _
                Lib "user32" (ByVal idAttach As Integer, ByVal idAttachTo As Integer, ByVal fAttach As Integer) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            SetForegroundWindow _
                Lib "user32" (ByVal hwnd As IntPtr) As <MarshalAs(UnmanagedType.Bool)> Boolean

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            SetFocus _
                Lib "user32" (ByVal hwnd As IntPtr) As IntPtr

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Auto Function _
            FindWindow _
                Lib "user32" (ByVal lpClassName As String, ByVal lpWindowName As String) As IntPtr

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        <PreserveSig()> _
        Friend Declare Function _
            CloseHandle _
                Lib "kernel32" (ByVal hObject As IntPtr) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Function _
            WaitForSingleObject _
                Lib "kernel32" (ByVal hHandle As NativeTypes.LateInitSafeHandleZeroOrMinusOneIsInvalid, ByVal dwMilliseconds As Integer) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport( _
             "kernel32", _
             CharSet:=CharSet.Auto, _
             PreserveSig:=True, _
             BestfitMapping:=False, _
             ThrowOnUnmappableChar:=True)> _
        Friend Shared Sub GetStartupInfo(<InAttribute(), OutAttribute()> ByVal lpStartupInfo As NativeTypes.STARTUPINFO)
        End Sub

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport( _
             "kernel32", _
             CharSet:=CharSet.Auto, _
             PreserveSig:=True, _
             BestfitMapping:=False, _
             ThrowOnUnmappableChar:=True)> _
        Friend Shared Function CreateProcess( _
            ByVal lpApplicationName As String, _
            ByVal lpCommandLine As String, _
            ByVal lpProcessAttributes As NativeTypes.SECURITY_ATTRIBUTES, _
            ByVal lpThreadAttributes As NativeTypes.SECURITY_ATTRIBUTES, _
            <MarshalAs(UnmanagedType.Bool)> ByVal bInheritHandles As Boolean, _
            ByVal dwCreationFlags As Integer, _
            ByVal lpEnvironment As IntPtr, _
            ByVal lpCurrentDirectory As String, _
            ByVal lpStartupInfo As NativeTypes.STARTUPINFO, _
            ByVal lpProcessInformation As NativeTypes.PROCESS_INFORMATION) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport( _
             "kernel32", _
             CharSet:=CharSet.Auto, _
             PreserveSig:=True, _
             BestfitMapping:=False, _
             ThrowOnUnmappableChar:=True)> _
         Friend Shared Function GetVolumeInformation( _
             <MarshalAs(UnmanagedType.LPTStr)> ByVal lpRootPathName As String, _
             ByVal lpVolumeNameBuffer As StringBuilder, _
             ByVal nVolumeNameSize As Integer, _
             ByRef lpVolumeSerialNumber As Integer, _
             ByRef lpMaximumComponentLength As Integer, _
             ByRef lpFileSystemFlags As Integer, _
             ByVal lpFileSystemNameBuffer As IntPtr, _
             ByVal nFileSystemNameSize As Integer) As Integer
        End Function

        '''**************************************************************************
        ''' ;SHFileOperation
        ''' <summary>
        ''' Given a 32-bit SHFILEOPSTRUCT, call the appropriate SHFileOperation function
        ''' to perform shell file operation.
        ''' </summary>
        ''' <param name="lpFileOp">32-bit SHFILEOPSTRUCT</param>
        ''' <returns>0 if successful, non-zero otherwise.</returns>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Friend Shared Function SHFileOperation(ByRef lpFileOp As SHFILEOPSTRUCT) As Int32
            If (IntPtr.Size = 4) Then ' 32-bit platforms
                Return SHFileOperation32(lpFileOp)
            Else ' 64-bit plaforms

                ' Create a new SHFILEOPSTRUCT64. The only difference is the packing, so copy all fields.
                Dim lpFileOp64 As New SHFILEOPSTRUCT64
                lpFileOp64.hwnd = lpFileOp.hwnd
                lpFileOp64.wFunc = lpFileOp.wFunc
                lpFileOp64.pFrom = lpFileOp.pFrom
                lpFileOp64.pTo = lpFileOp.pTo
                lpFileOp64.fFlags = lpFileOp.fFlags
                lpFileOp64.fAnyOperationsAborted = lpFileOp.fAnyOperationsAborted
                lpFileOp64.hNameMappings = lpFileOp.hNameMappings
                lpFileOp64.lpszProgressTitle = lpFileOp.lpszProgressTitle

                ' P/Invoke SHFileOperation with the 64 bit structure.
                Dim result As Int32 = SHFileOperation64(lpFileOp64)

                ' Only need to check if any operations were aborted.
                lpFileOp.fAnyOperationsAborted = lpFileOp64.fAnyOperationsAborted

                Return result
            End If
        End Function

        '''**************************************************************************
        ''' ;SHFileOperation32
        ''' <summary>
        ''' Copies, moves, renames or deletes a file system object on 32-bit platforms.
        ''' </summary>
        ''' <param name="lpFileOp">Pointer to an SHFILEOPSTRUCT structure that contains information this function needs
        '''       to carry out the specified operation. This parameter must contain a valid value that is not NULL.
        '''       You are responsible for validating the value. If you do not, you will experience unexpected result.</param>
        ''' <returns>Returns zero if successful, non zero otherwise.</returns>
        ''' <remarks>
        ''' You should use fully-qualified path names with this function. Using it with relative path names is not thread safe.
        ''' You cannot use SHFileOperation to move special folders My Documents and My Pictures from a local drive to a remote computer.
        ''' File deletion is recursive unless you set the FOF_NORECURSION flag.
        ''' </remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("shell32.dll", CharSet:=CharSet.Auto, EntryPoint:="SHFileOperation", SetLastError:=True, ThrowOnUnmappableChar:=True)> _
        Private Shared Function SHFileOperation32(ByRef lpFileOp As SHFILEOPSTRUCT) As Int32
        End Function


        '''**************************************************************************
        ''' ;SHFILEOPSTRUCT
        ''' <summary>
        ''' Contains information that the SHFileOperation function uses to perform file operations
        ''' on 32-bit platforms.
        ''' </summary>
        ''' <remarks>
        ''' * For detail documentation: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/structures/shfileopstruct.asp.
        ''' Members:
        '''   hwnd: Window handle to the dialog box to display information about the status of the operation.
        '''   wFunc: Value indicates which operation (copy, move, rename, delete) to perform.
        '''   pFrom: Buffer for 1 or more source file names. Each name ends with a NULL separator + additional NULL at the end.
        '''   pTo: Buffer for destination name(s). Same rule as pFrom.
        '''   fFlags: Flags that control details of the operation.
        '''   fAnyOperationsAborted: Out param. TRUE if user aborted any file operations. Otherwise, FALSE.
        '''   hNameMappings: Handle to name mapping object containing old and new names of renamed files (not used).
        '''   lpszProgressTitle: Address of a string to use as title of progress dialog box. (not used).
        ''' typedef struct _SHFILEOPSTRUCT {
        '''    HWND hwnd;
        '''    UINT wFunc;
        '''    LPCTSTR pFrom;
        '''    LPCTSTR pTo;
        '''    FILEOP_FLAGS fFlags;                           (WORD)
        '''    BOOL fAnyOperationsAborted;
        '''    LPVOID hNameMappings;
        '''    LPCTSTR lpszProgressTitle;
        ''' } SHFILEOPSTRUCT, *LPSHFILEOPSTRUCT;
        ''' * VSWhidbey 321345, 321346: From KB151799 - The SHFILEOPSTRUCT is not double-word aligned.
        '''   If no steps are taken, the last 3 variables will not be passed correctly. Hence the Pack:=1.
        ''' </remarks>             
        <StructLayout(LayoutKind.Sequential, Pack:=1, CharSet:=CharSet.Auto)> _
        Friend Structure SHFILEOPSTRUCT
            Friend hwnd As IntPtr
            Friend wFunc As UInt32
            <MarshalAs(UnmanagedType.LPTStr)> Friend pFrom As String
            <MarshalAs(UnmanagedType.LPTStr)> Friend pTo As String
            Friend fFlags As UInt16
            Friend fAnyOperationsAborted As Boolean
            Friend hNameMappings As IntPtr
            <MarshalAs(UnmanagedType.LPTStr)> Friend lpszProgressTitle As String
        End Structure

        '''**************************************************************************
        ''' ;SHFileOperation64
        ''' <summary>
        ''' Copies, moves, renames or deletes a file system object on 64-bit platforms.
        ''' </summary>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("shell32.dll", CharSet:=CharSet.Auto, EntryPoint:="SHFileOperation", SetLastError:=True, ThrowOnUnmappableChar:=True)> _
        Private Shared Function SHFileOperation64(ByRef lpFileOp As SHFILEOPSTRUCT64) As Int32
        End Function

        '''**************************************************************************
        ''' ;SHFILEOPSTRUCT64
        ''' <summary>
        ''' Contains information that the SHFileOperation function uses to perform file operations
        ''' on 64-bit platforms, where the structure is unpacked. VSWhidbey 421265, 
        ''' </summary>
        <StructLayout(LayoutKind.Sequential, CharSet:=CharSet.Auto)> _
        Private Structure SHFILEOPSTRUCT64
            Friend hwnd As IntPtr
            Friend wFunc As UInt32
            <MarshalAs(UnmanagedType.LPTStr)> Friend pFrom As String
            <MarshalAs(UnmanagedType.LPTStr)> Friend pTo As String
            Friend fFlags As UInt16
            Friend fAnyOperationsAborted As Boolean
            Friend hNameMappings As IntPtr
            <MarshalAs(UnmanagedType.LPTStr)> Friend lpszProgressTitle As String
        End Structure

        '''**************************************************************************
        ''' ;SHFileOperationType 
        ''' <summary>
        ''' Values that indicate which file operation to perform. Used in SHFILEOPSTRUCT
        ''' </summary>
        Friend Enum SHFileOperationType As UInt32
            FO_MOVE = &H1
            FO_COPY = &H2
            FO_DELETE = &H3
            FO_RENAME = &H4
        End Enum


        '''**************************************************************************
        ''' ;ShFileOperationFlags 
        ''' <summary>
        ''' Flags that control the file operation. Used in SHFILEOPSTRUCT.
        ''' </summary>
        <Flags()> _
        Friend Enum ShFileOperationFlags As UInt16
            ' The pTo member specifies multiple destination files (one for each source file) 
            ' rather than one directory where all source files are to be deposited.
            FOF_MULTIDESTFILES = &H1
            ' Not currently used. 
            FOF_CONFIRMMOUSE = &H2
            ' Do not display a progress dialog box. 
            FOF_SILENT = &H4
            ' Give the file being operated on a new name in a move, copy, or rename operation 
            ' if a file with the target name already exists. 
            FOF_RENAMEONCOLLISION = &H8
            ' Respond with "Yes to All" for any dialog box that is displayed. 
            FOF_NOCONFIRMATION = &H10
            ' If FOF_RENAMEONCOLLISION is specified and any files were renamed,
            ' assign a name mapping object containing their old and new names to the hNameMappings member.
            FOF_WANTMAPPINGHANDLE = &H20
            ' Preserve Undo information, if possible. Undone can only be done from the same process.
            ' If pFrom does not contain fully qualified path and file names, this flag is ignored. 
            ' NOTE: Not setting this flag will let the file be deleted permanently, unlike the doc says.
            FOF_ALLOWUNDO = &H40
            ' Perform the operation on files only if a wildcard file name (*.*) is specified.
            FOF_FILESONLY = &H80
            ' Display a progress dialog box but do not show the file names. 
            FOF_SIMPLEPROGRESS = &H100
            ' Do not confirm the creation of a new directory if the operation requires one to be created. 
            FOF_NOCONFIRMMKDIR = &H200
            ' Do not display a user interface if an error occurs. 
            FOF_NOERRORUI = &H400
            ' Do not copy the security attributes of the file.
            FOF_NOCOPYSECURITYATTRIBS = &H800
            ' Only operate in the local directory. Don't operate recursively into subdirectories.
            FOF_NORECURSION = &H1000
            ' Do not move connected files as a group. Only move the specified files. 
            FOF_NO_CONNECTED_ELEMENTS = &H2000
            ' Send a warning if a file is being destroyed during a delete operation rather than recycled. 
            ' This flag partially overrides FOF_NOCONFIRMATION.
            FOF_WANTNUKEWARNING = &H4000
            ' Treat reparse points as objects, not containers.
            FOF_NORECURSEREPARSE = &H8000
        End Enum 'FileOperationFlags


        '''**************************************************************************
        ''' ;SHChangeNotify
        ''' <summary>
        ''' Notifies the system of an event that an application has performed. 
        ''' An appliation should use this function if it performs an action that may affect the shell.
        ''' </summary>
        ''' <param name="wEventId">Describes the event that has occured. Typically, only one event is specified at at a time.
        '''       If more than one event is specified, the values contained in dwItem1 and dwItem2 must be the same,
        '''       respectively, for all specified events. See ShellChangeNotificationEvents.</param>
        ''' <param name="uFlags">Flags that indicate the meaning of the dwItem1 and dwItem2 parameter. See ShellChangeNotificationFlags.</param>
        ''' <param name="dwItem1">First event-dependent value.</param>
        ''' <param name="dwItem2">Second event-dependent value.</param>
        ''' <remarks>
        ''' Win 95/98/Me: SHChangeNotify is supported by Microsoft Layer for Unicode. 
        ''' To use this http://msdn.microsoft.com/library/default.asp?url=/library/en-us/mslu/winprog/microsoft_layer_for_unicode_on_windows_95_98_me_systems.asp
        ''' </remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("shell32.dll", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Sub SHChangeNotify(ByVal wEventId As UInt32, ByVal uFlags As UInt32, _
                ByVal dwItem1 As IntPtr, ByVal dwItem2 As IntPtr)
        End Sub


        '''**************************************************************************
        ''' ;SHChangeEventTypes 
        ''' <summary>
        ''' Describes the event that has occured. Used in SHChangeNotify.
        ''' There are more values in shellapi.h. Only include the relevant ones.
        ''' </summary>
        Friend Enum SHChangeEventTypes As UInt32
            ' Specifies a combination of all of the disk event identifiers. 
            SHCNE_DISKEVENTS = &H2381F
            ' All events have occurred. 
            SHCNE_ALLEVENTS = &H7FFFFFFF
        End Enum


        '''**************************************************************************
        ''' ;SHChangeEventParameterFlags 
        ''' <summary>
        ''' Indicates the meaning of dwItem1 and dwItem2 parameters in SHChangeNotify method.
        ''' There are more values in shellapi.h. Only include the relevant one.
        ''' </summary>
        Friend Enum SHChangeEventParameterFlags As UInt32
            ' The dwItem1 and dwItem2 parameters are DWORD values. 
            SHCNF_DWORD = &H3
        End Enum

        '''**************************************************************************
        ''' ;MEMORYSTATUS
        ''' <summary>
        ''' Contains information about the current state of both physical and virtual memory.
        ''' </summary>
        <StructLayout(LayoutKind.Sequential)> _
        Friend Structure MEMORYSTATUS
            'typedef struct _MEMORYSTATUS {  
            '    DWORD dwLength;            Size of the MEMORYSTATUS data structure, in bytes. You do not need to set this member before calling the GlobalMemoryStatus function; the function sets it. 
            '    DWORD dwMemoryLoad;        Number between 0 and 100 that specifies the approximate percentage of physical memory that is in use 
            '    SIZE_T dwTotalPhys;        Total size of physical memory, in bytes. 
            '    SIZE_T dwAvailPhys;        Size of physical memory available, in bytes. 
            '    SIZE_T dwTotalPageFile;    Size of the committed memory limit, in bytes.
            '    SIZE_T dwAvailPageFile;    Size of available memory to commit, in bytes. 
            '    SIZE_T dwTotalVirtual;     Total size of the user mode portion of the virtual address space of the calling process, in bytes. 
            '    SIZE_T dwAvailVirtual;     Size of unreserved and uncommitted memory in the user mode portion of the virtual address space of the calling process, in bytes. 
            '} MEMORYSTATUS, *LPMEMORYSTATUS;

            Friend dwLength As UInt32
            Friend dwMemoryLoad As UInt32
            Friend dwTotalPhys As UInt32
            Friend dwAvailPhys As UInt32
            Friend dwTotalPageFile As UInt32
            Friend dwAvailPageFile As UInt32
            Friend dwTotalVirtual As UInt32
            Friend dwAvailVirtual As UInt32
        End Structure

        '''**************************************************************************
        ''' ;GlobalMemoryStatus
        ''' <summary>
        ''' Obtains information about the system's current usage of both physical and virtual memory.
        ''' </summary>
        ''' <param name="lpBuffer">Pointer to a MEMORYSTATUS structure.</param>
        ''' <remarks>
        ''' Requirement:
        ''' Client: Requires Windows XP, Windows 2000 Professional, Windows NT Workstation, Windows Me, Windows 98, or Windows 95.
        ''' Server: Requires Windows Server 2003, Windows 2000 Server, or Windows NT Server.
        ''' </remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Kernel32.dll", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Sub GlobalMemoryStatus(ByRef lpBuffer As MEMORYSTATUS)
        End Sub

        '''**************************************************************************
        ''' ;MEMORYSTATUSEX
        ''' <summary>
        ''' Contains information about the current state of both physical and virtual memory, including extended memory.
        ''' </summary>
        <StructLayout(LayoutKind.Sequential)> _
        Friend Structure MEMORYSTATUSEX
            'typedef struct _MEMORYSTATUSEX {  
            '   DWORD dwLength;                     Size of the structure. Must set before calling GlobalMemoryStatusEx.
            '   DWORD dwMemoryLoad;                 Number between 0 and 100 on current memory utilization.
            '   DWORDLONG ullTotalPhys;             Total size of physical memory.
            '   DWORDLONG ullAvailPhys;             Total size of available physical memory.
            '   DWORDLONG ullTotalPageFile;         Size of committed memory limit.
            '   DWORDLONG ullAvailPageFile;         Size of available memory to committed (ullTotalPageFile max).
            '   DWORDLONG ullTotalVirtual;          Total size of user potion of virtual address space of calling process.
            '   DWORDLONG ullAvailVirtual;          Total size of unreserved and uncommitted memory in virtual address space.
            '   DWORDLONG ullAvailExtendedVirtual;  Total size of unreserved and uncommitted memory in extended portion of virual address.
            '} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

            Friend dwLength As UInt32
            Friend dwMemoryLoad As UInt32
            Friend ullTotalPhys As UInt64
            Friend ullAvailPhys As UInt64
            Friend ullTotalPageFile As UInt64
            Friend ullAvailPageFile As UInt64
            Friend ullTotalVirtual As UInt64
            Friend ullAvailVirtual As UInt64
            Friend ullAvailExtendedVirtual As UInt64

            Friend Sub Init()
                dwLength = CType(Marshal.SizeOf(GetType(MEMORYSTATUSEX)), UInt32)
            End Sub
        End Structure

        '''**************************************************************************
        ''' ;GlobalMemoryStatusEx
        ''' <summary>
        ''' Obtains information about the system's current usage of both physical and virtual memory.
        ''' </summary>
        ''' <param name="lpBuffer">Pointer to a MEMORYSTATUSEX structure.</param>
        ''' <returns>True if the function successes. Otherwise, False.</returns>
        ''' <remarks>
        ''' Requirement:
        ''' Client: Requires Windows XP or Windows 2000 Professional.
        ''' Server: Requires Windows Server 2003 or Windows 2000 Server.
        ''' </remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Kernel32.dll", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Function GlobalMemoryStatusEx(ByRef lpBuffer As MEMORYSTATUSEX) As <MarshalAsAttribute(UnmanagedType.Bool)> Boolean
        End Function



        '''****************************************************************************
        ''';ConvertStringSecurityDescriptorToSecurityDescriptor
        ''' <summary>
        ''' Takes a SDDL string and converts it to a pointer to a security descriptor
        ''' </summary>
        ''' <param name="StringSecurityDescriptor"></param>
        ''' <param name="StringSDRevision"></param>
        ''' <param name="SecurityDescriptor"></param>
        ''' <param name="SecurityDescriptorSize"></param>
        ''' <returns></returns>
        ''' <remarks>Only supported on Win2000+
        ''' see p. 186-187 in writing secure code vol 2 on the SDDL string
        '''see winerror.h for getlastwin32error meanings
        '''see sddl.h for info on SDL_VERSION
        '''see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/secauthz/security/ace_strings.asp for how to build the SDL string
        ''' </remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Advapi32.dll", CharSet:=CharSet.Unicode, ThrowonUnmappableChar:=True, SetLastError:=True)> _
        Friend Shared Function ConvertStringSecurityDescriptorToSecurityDescriptor(ByVal StringSecurityDescriptor As String, ByVal StringSDRevision As UInteger, ByRef SecurityDescriptor As IntPtr, ByVal SecurityDescriptorSize As IntPtr) As <MarshalAsAttribute(UnmanagedType.Bool)> Boolean
        End Function

        '''****************************************************************************
        ''';MoveFileEx
        ''' <summary>
        ''' The MoveFileEx function moves an existing file or directory.
        ''' http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/movefileex.asp
        ''' </summary>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("kernel32", _
             PreserveSig:=True, _
             CharSet:=CharSet.Auto, _
             EntryPoint:="MoveFileEx", _
             BestFitMapping:=False, _
             ThrowOnUnmappableChar:=True, _
             SetLastError:=True)> _
         Friend Shared Function MoveFileEx( _
             ByVal lpExistingFileName As String, _
             ByVal lpNewFileName As String, _
             ByVal dwFlags As Integer) As <MarshalAs(UnmanagedType.Bool)> Boolean
        End Function

        ''' ;New
        ''' <summary>
        ''' FxCop violation: Avoid uninstantiated internal class. 
        ''' Adding a private constructor to prevent the compiler from generating a default constructor.
        ''' </summary>
        Private Sub New()
        End Sub
    End Class



End Namespace

