' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Security
Imports System.Security.Permissions
Imports System.Globalization
Imports System.Text
Imports System.Collections
Imports System.Threading
Imports System.Runtime.CompilerServices
Imports System.Runtime.InteropServices
Imports System.Runtime.Versioning
Imports System.Diagnostics

#If Not TELESTO Then
Imports System.ComponentModel
Imports System.Drawing
Imports System.Windows.Forms
Imports Microsoft.Win32
#End If

Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic

    Public Module Interaction
#If Not LATEBINDING Then
#If Not TELESTO Then
        Private m_SortedEnvList As System.Collections.SortedList 'SECURITY! We asserted to get this list.  Whenever you hand out portions of it you must demand the appropriate EnvironmentPermission

        '============================================================================
        ' Application/system interaction functions.
        '============================================================================

        'No HostProtection attribute because the code has a demand for UnmanagedCode.
        <SecuritySafeCritical()> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        <ResourceExposure(ResourceScope.None)> _
        <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
        Public Function Shell(ByVal PathName As String, Optional ByVal Style As AppWinStyle = AppWinStyle.MinimizedFocus, Optional ByVal Wait As Boolean = False, Optional ByVal Timeout As Integer = -1) As Integer
            Dim StartupInfo As New NativeTypes.STARTUPINFO
            Dim ProcessInfo As New NativeTypes.PROCESS_INFORMATION
            Dim ok As Integer
            Dim safeProcessHandle As New NativeTypes.LateInitSafeHandleZeroOrMinusOneIsInvalid()
            Dim safeThreadHandle As New NativeTypes.LateInitSafeHandleZeroOrMinusOneIsInvalid()
            Dim ErrorCode As Integer = 0

            ' UNDONE:M3:
            ' - use WaitOne or do DoEvents during the waiting infinite case below so that entire app-domain
            '       is not unloaded in abort cases. Suggested by Christopher Brummer. Will wait for FX
            '
            ' M2 - we are okay for now with the ExtermalProcessMgmt demand. This demand should always
            ' be present.

            Try 'We demand UI because we don't want to allow shelling out UI in a server or non-UI app
                Call (New UIPermission(UIPermissionWindow.AllWindows)).Demand()
            Catch e As Exception
                Throw e
            End Try

            If (PathName Is Nothing) Then
                Throw New NullReferenceException(GetResourceString(ResID.Argument_InvalidNullValue1, "Pathname"))
            End If

            If (Style < 0 OrElse Style > 9) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Style"))
            End If

            NativeMethods.GetStartupInfo(StartupInfo)
            Try
                StartupInfo.dwFlags = NativeTypes.STARTF_USESHOWWINDOW  ' we want to specify the initial window style (minimized, etc) so set this bit.
                StartupInfo.wShowWindow = Style

                'We have to have unmanaged permissions to do this, so asking for path permissions would be redundant
                'Note: We are using the StartupInfo (defined in nativeTypes.StartupInfo) in CreateProcess() even though this version
                'of the StartupInfo type uses Intptr instead of String because GetStartupInfo() above requires that version so we don't
                'free the string fields since the API manages it instead.  But its ok here because we are just passing along the memory
                'that GetStartupInfo() allocated along to CreateProcess() which just reads the string fields.

                RuntimeHelpers.PrepareConstrainedRegions()
                Try
                Finally
                    ok = NativeMethods.CreateProcess(Nothing, PathName, Nothing, Nothing, False, NativeTypes.NORMAL_PRIORITY_CLASS, Nothing, Nothing, StartupInfo, ProcessInfo)
                    If ok = 0 Then
                        ErrorCode = Marshal.GetLastWin32Error()
                    End If
                    If ProcessInfo.hProcess <> IntPtr.Zero AndAlso ProcessInfo.hProcess <> NativeTypes.INVALID_HANDLE Then
                        safeProcessHandle.InitialSetHandle(ProcessInfo.hProcess)
                    End If
                    If ProcessInfo.hThread <> IntPtr.Zero AndAlso ProcessInfo.hThread <> NativeTypes.INVALID_HANDLE Then
                        safeThreadHandle.InitialSetHandle(ProcessInfo.hThread)
                    End If
                End Try

                Try
                    If (ok <> 0) Then
                        If Wait Then
                            ' Is infinite wait okay here ?
                            ' This is okay since this is marked as requiring the HostPermission with ExternalProcessMgmt rights
                            ok = NativeMethods.WaitForSingleObject(safeProcessHandle, Timeout)

                            If ok = 0 Then 'succeeded
                                'Process ran to completion
                                Shell = 0
                            Else
                                'Wait timedout
                                Shell = ProcessInfo.dwProcessId
                            End If
                        Else
                            NativeMethods.WaitForInputIdle(safeProcessHandle, 10000)
                            Shell = ProcessInfo.dwProcessId
                        End If
                    Else
                        'Check for a win32 error access denied. If it is, make and throw the exception.
                        'If not, throw FileNotFound
                        Const ERROR_ACCESS_DENIED As Integer = 5
                        If ErrorCode = ERROR_ACCESS_DENIED Then
                            Throw VbMakeException(vbErrors.PermissionDenied)
                        End If

                        Throw VbMakeException(vbErrors.FileNotFound)
                    End If
                Finally
                    safeProcessHandle.Close() ' Close the process handle will not cause the process to stop.
                    safeThreadHandle.Close()
                End Try
            Finally
                StartupInfo.Dispose()
            End Try
        End Function


        'No HostProtection attribute because the code a demand for UnmanagedCode.
        <SecuritySafeCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        Public Sub AppActivate(ByVal ProcessId As Integer)
            'As an optimization, we will only check the UI permission once we actually know we found the app to activate - we'll do that in AppActivateHelper

            Dim ProcessIdOwningWindow As Integer
            'Note, a process can have multiple windows.  What we want to do is dig through to find one
            'that we can actually activate.  So first ignore all the ones that are not visible and don't support mouse
            'or keyboard input
            Dim WindowHandle As IntPtr = NativeMethods.GetWindow(NativeMethods.GetDesktopWindow(), NativeTypes.GW_CHILD)

            Do While (IntPtr.op_Inequality(WindowHandle, IntPtr.Zero))
                SafeNativeMethods.GetWindowThreadProcessId(WindowHandle, ProcessIdOwningWindow)
                If (ProcessIdOwningWindow = ProcessId) AndAlso SafeNativeMethods.IsWindowEnabled(WindowHandle) AndAlso SafeNativeMethods.IsWindowVisible(WindowHandle) Then
                    Exit Do 'We found a window belonging to the desired process that we can actually activate and will support user input
                End If

                'keep rummaging through windows looking for one that belongs to the process we are after
                WindowHandle = NativeMethods.GetWindow(WindowHandle, NativeTypes.GW_HWNDNEXT)
            Loop

            'If we didn't find a window during the pass above, try the less desirable route of finding any window that belongs to the process
            If IntPtr.op_Equality(WindowHandle, IntPtr.Zero) Then
                WindowHandle = NativeMethods.GetWindow(NativeMethods.GetDesktopWindow(), NativeTypes.GW_CHILD)

                Do While IntPtr.op_Inequality(WindowHandle, IntPtr.Zero)
                    SafeNativeMethods.GetWindowThreadProcessId(WindowHandle, ProcessIdOwningWindow)
                    If (ProcessIdOwningWindow = ProcessId) Then
                        Exit Do
                    End If

                    'keep rummaging through windows looking for one that belongs to the process we are after
                    WindowHandle = NativeMethods.GetWindow(WindowHandle, NativeTypes.GW_HWNDNEXT)
                Loop
            End If

            If IntPtr.op_Equality(WindowHandle, IntPtr.Zero) Then 'we never found a window belonging to the desired process
                Throw New ArgumentException(GetResourceString(ResID.ProcessNotFound, CStr(ProcessId)))
            Else
                AppActivateHelper(WindowHandle)
            End If
        End Sub


        'No HostProtection attribute because the code a demand for UnmanagedCode.
        <SecuritySafeCritical()> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Public Sub AppActivate(ByVal Title As String)
            'As an optimization, we will only check the UI permission once we actually know we found the app to activate - we'll do that in AppActivateHelper
            Dim WindowHandle As IntPtr = NativeMethods.FindWindow(Nothing, Title) 'see if we can find the window using an exact match on the title
            Const MAX_TITLE_LENGTH As Integer = 511 'CONSIDER:  - this is bizarre.  Eventually might want to do the trick in c:\vs\ndp\Designer\CompMod\System\ComponentModel\Design\MultilineStringEditor.cs (search for getWindowText)

            '  if no match, search through all parent windows
            If IntPtr.op_Equality(WindowHandle, IntPtr.Zero) Then
                Dim AppTitle As String = String.Empty
                ' Old implementation uses MAX_TITLE_LENGTH characters, INCLUDING NULL character.
                ' Interop code will extend string builder to handle NULL character.
                Dim AppTitleBuilder As New StringBuilder(MAX_TITLE_LENGTH)
                Dim AppTitleLength As Integer
                Dim TitleLength As Integer = Len(Title)

                'Loop through all children of the desktop
                WindowHandle = NativeMethods.GetWindow(NativeMethods.GetDesktopWindow(), NativeTypes.GW_CHILD)
                Do While IntPtr.op_Inequality(WindowHandle, IntPtr.Zero)
                    '  get the window caption and test for a left-aligned substring
                    AppTitleLength = NativeMethods.GetWindowText(WindowHandle, AppTitleBuilder, AppTitleBuilder.Capacity)
                    AppTitle = AppTitleBuilder.ToString()

                    If AppTitleLength >= TitleLength Then
                        If String.Compare(AppTitle, 0, Title, 0, TitleLength, StringComparison.OrdinalIgnoreCase) = 0 Then
                            Exit Do 'found one
                        End If
                    End If

                    'keep looking
                    WindowHandle = NativeMethods.GetWindow(WindowHandle, NativeTypes.GW_HWNDNEXT)
                Loop

                If IntPtr.op_Equality(WindowHandle, IntPtr.Zero) Then
                    ' We didn't find it so try right aligned
                    WindowHandle = NativeMethods.GetWindow(NativeMethods.GetDesktopWindow(), NativeTypes.GW_CHILD)

                    Do While IntPtr.op_Inequality(WindowHandle, IntPtr.Zero)
                        '  get the window caption and test for a right-aligned substring
                        AppTitleLength = NativeMethods.GetWindowText(WindowHandle, AppTitleBuilder, AppTitleBuilder.Capacity)
                        AppTitle = AppTitleBuilder.ToString()

                        If AppTitleLength >= TitleLength Then
                            If String.Compare(Right(AppTitle, TitleLength), 0, Title, 0, TitleLength, StringComparison.OrdinalIgnoreCase) = 0 Then
                                Exit Do 'found a match
                            End If
                        End If

                        'keep looking
                        WindowHandle = NativeMethods.GetWindow(WindowHandle, NativeTypes.GW_HWNDNEXT)
                    Loop
                End If
            End If

            If IntPtr.op_Equality(WindowHandle, IntPtr.Zero) Then 'no match
                Throw New ArgumentException(GetResourceString(ResID.ProcessNotFound, Title))
            Else
                AppActivateHelper(WindowHandle)
            End If
        End Sub

        <SecuritySafeCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Private Sub AppActivateHelper(ByVal hwndApp As IntPtr)
            Try
                Call (New UIPermission(UIPermissionWindow.AllWindows)).Demand() 'We only check the UI permission once we actually know we found the app to activate - we'll do that in AppActivateHelper
            Catch e As Exception
                Throw e
            End Try

            '  if no window with name (full or truncated) or task id, return an error
            '  if the window is not enabled or not visible, get the first window owned by it that is not enabled or not visible
            Dim hwndOwned As IntPtr
            If (Not SafeNativeMethods.IsWindowEnabled(hwndApp) OrElse Not SafeNativeMethods.IsWindowVisible(hwndApp)) Then
                '  scan to the next window until failure
                hwndOwned = NativeMethods.GetWindow(hwndApp, NativeTypes.GW_HWNDFIRST)
                Do While IntPtr.op_Inequality(hwndOwned, IntPtr.Zero)
                    If IntPtr.op_Equality(NativeMethods.GetWindow(hwndOwned, NativeTypes.GW_OWNER), hwndApp) Then
                        If (Not SafeNativeMethods.IsWindowEnabled(hwndOwned) OrElse Not SafeNativeMethods.IsWindowVisible(hwndOwned)) Then
                            hwndApp = hwndOwned
                            hwndOwned = NativeMethods.GetWindow(hwndApp, NativeTypes.GW_HWNDFIRST)
                        Else
                            Exit Do
                        End If
                    End If
                    hwndOwned = NativeMethods.GetWindow(hwndOwned, NativeTypes.GW_HWNDNEXT)
                Loop

                '  if scan failed, return an error
                If IntPtr.op_Equality(hwndOwned, IntPtr.Zero) Then
                    Throw New ArgumentException(GetResourceString(ResID.ProcessNotFound))
                End If

                '  set active window to the owned one
                hwndApp = hwndOwned
            End If

            ' SetActiveWindow on Win32 only activates the Window - it does
            ' not bring to to the foreground unless the window belongs to
            ' the current thread.  NativeMethods.SetForegroundWindow() activates the
            ' window, moves it to the foreground, and bumps the priority
            ' of the thread which owns the window.

            Dim dwDummy As Integer ' dummy arg for SafeNativeMethods.GetWindowThreadProcessId

            ' Attach ourselves to the window we want to set focus to
            NativeMethods.AttachThreadInput(0, SafeNativeMethods.GetWindowThreadProcessId(hwndApp, dwDummy), 1)
            ' Make him foreground and give him focus, this will occur
            ' synchronously because we are attached.
            NativeMethods.SetForegroundWindow(hwndApp)
            NativeMethods.SetFocus(hwndApp)
            ' Unattach ourselves from the window
            NativeMethods.AttachThreadInput(0, SafeNativeMethods.GetWindowThreadProcessId(hwndApp, dwDummy), 0)
        End Sub




        Private m_CommandLine As String

        <SecuritySafeCritical()>
        Public Function Command() As String

            'Do this demand so we don't have access to cached m_CommandLine when we go through here without permissions
            Call (New EnvironmentPermission(EnvironmentPermissionAccess.Read, "Path")).Demand()

            If m_CommandLine Is Nothing Then
                Dim s As String = Environment.CommandLine

                'The first element of the array is the .exe name
                '  we must remove this when building the return value
                If (s Is Nothing) OrElse (s.Length = 0) Then
                    Return ""
                End If

                'The following code must remove the application name from the command line
                ' without disturbing the arguments (trailing and embedded spaces)
                '
                'We also need to handle embedded spaces in the application name
                ' as well as skipping over quotations used around embedded spaces within
                ' the application name
                '  examples:
                '       f:\"Program Files"\Microsoft\foo.exe  a b  d   e  f 
                '       "f:\"Program Files"\Microsoft\foo.exe" a b  d   e  f 
                '       f:\Program Files\Microsoft\foo.exe                  a b  d   e  f 
                Dim LengthOfAppName, j As Integer

                'Remove the app name from the arguments
                LengthOfAppName = Environment.GetCommandLineArgs(0).Length

                Do
                    j = s.IndexOf(ChrW(34), j)
                    If j >= 0 AndAlso j <= LengthOfAppName Then
                        s = s.Remove(j, 1)
                    End If
                Loop While (j >= 0 AndAlso j <= LengthOfAppName)

                If j = 0 OrElse j > s.Length Then
                    m_CommandLine = ""
                Else
                    m_CommandLine = LTrim(s.Substring(LengthOfAppName))
                End If
            End If
            Return m_CommandLine
        End Function


        'No HostProtection attribute because the code already directly or indirectly has a demand.
        <SecuritySafeCritical()> _
        Public Function Environ(ByVal Expression As Integer) As String

            'Validate index - Note that unlike the fx, this is a legacy VB function and the index is 1 based.
            If Expression <= 0 OrElse Expression > 255 Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_Range1toFF1, "Expression"))
            End If

            If m_SortedEnvList Is Nothing Then
                SyncLock m_EnvironSyncObject
                    If m_SortedEnvList Is Nothing Then
                        'Constructing the sorted environment list is extremely slow, so we keep a copy around. This list must be alphabetized to match vb5/vb6 behavior
                        Call New System.Security.Permissions.EnvironmentPermission(PermissionState.Unrestricted).Assert() 'We control to whom we allow access to m_SortedEnvList
                        m_SortedEnvList = New System.Collections.SortedList(Environment.GetEnvironmentVariables())
                        System.Security.PermissionSet.RevertAssert()
                    End If
                End SyncLock
            End If

            If Expression > m_SortedEnvList.Count Then
                Return ""
            End If

            Dim EnvVarName As String = m_SortedEnvList.GetKey(Expression - 1).ToString()
            Dim EnvVarValue As String = m_SortedEnvList.GetByIndex(Expression - 1).ToString()
            Call New EnvironmentPermission(EnvironmentPermissionAccess.Read, EnvVarName).Demand() 'make sure they have permission to read it.
            Return (EnvVarName & "=" & EnvVarValue)
        End Function

        Private m_EnvironSyncObject As New Object

        'No security demand in this version because the frameworks will do a demand on GetEnvironmentVariable
        'No HostProtection attribute because the code already directly or indirectly has a demand.
        Public Function Environ(ByVal Expression As String) As String
            Expression = Trim(Expression)

            If Expression.Length = 0 Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Expression"))
            End If

            Return Environment.GetEnvironmentVariable(Expression)
        End Function


        '============================================================================
        ' User interaction functions.
        '============================================================================

        'No HostProtection attribute because the code already directly or indirectly has a demand.
        <SecuritySafeCritical()> _
        Public Sub Beep()

            Try
                '*** SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK ***
                'Note that if you were to call system.Media.MessageBeep.Play you wouldn't get the security check.  They don't 
                'check anything at all.
                Call (New UIPermission(UIPermissionWindow.SafeSubWindows)).Demand()
            Catch ex1 As System.Security.SecurityException
                Try
                    Call (New UIPermission(UIPermissionWindow.SafeTopLevelWindows)).Demand()
                Catch ex2 As System.Security.SecurityException
                    'Just ignore if not sufficient rights
                    Exit Sub
                End Try
            End Try

            UnsafeNativeMethods.MessageBeep(0)
        End Sub


        Private NotInheritable Class InputBoxHandler
            Private m_Prompt As String
            Private m_Title As String
            Private m_DefaultResponse As String
            Private m_XPos As Integer
            Private m_YPos As Integer
            Private m_Result As String
            Private m_ParentWindow As System.Windows.Forms.IWin32Window
            Private m_Exception As Exception

            Sub New(ByVal Prompt As String, ByVal Title As String, ByVal DefaultResponse As String, ByVal XPos As Integer, ByVal YPos As Integer, ByVal ParentWindow As System.Windows.Forms.IWin32Window)
                m_Prompt = Prompt
                m_Title = Title
                m_DefaultResponse = DefaultResponse
                m_XPos = XPos
                m_YPos = YPos
                m_ParentWindow = ParentWindow
            End Sub

            <STAThread()> Public Sub StartHere()
                Try
                    m_Result = InternalInputBox(m_Prompt, m_Title, m_DefaultResponse, m_XPos, m_YPos, m_ParentWindow)
                Catch ex As Exception
                    m_Exception = ex
                End Try
            End Sub

            Public ReadOnly Property Result() As String
                Get
                    Return m_Result
                End Get
            End Property

            Friend ReadOnly Property Exception As Exception
                Get
                    Return m_Exception
                End Get
            End Property
        End Class


        <HostProtection(Resources:=HostProtectionResource.UI)> _
        Public Function InputBox(ByVal Prompt As String, Optional ByVal Title As String = "", Optional ByVal DefaultResponse As String = "", Optional ByVal XPos As Integer = -1, Optional ByVal YPos As Integer = -1) As String
            Dim vbhost As CompilerServices.IVbHost
            Dim ParentWindow As System.Windows.Forms.IWin32Window = Nothing

            vbhost = CompilerServices.HostServices.VBHost
            If vbhost IsNot Nothing Then 'If we are hosted then we want to use the host as the parent window.  If no parent window that's fine.
                ParentWindow = vbhost.GetParentWindow()
            End If

            If Title.Length = 0 Then
                If vbhost Is Nothing Then
                    Title = GetTitleFromAssembly(System.Reflection.Assembly.GetCallingAssembly())
                Else
                    Title = vbhost.GetWindowTitle()
                End If
            End If

            'Threading state can only be set once, and will most often be already set
            'but set to STA and check if it isn't STA, then we need to start another thread
            'to display the InputBox
            'CONSIDER :  use the threadpool for performance?
            If System.Threading.Thread.CurrentThread.GetApartmentState() <> Threading.ApartmentState.STA Then
                Dim InputHandler As New InputBoxHandler(Prompt, Title, DefaultResponse, XPos, YPos, ParentWindow)
                Dim thread As New Threading.Thread(New Threading.ThreadStart(AddressOf InputHandler.StartHere))
                thread.Start()
                Thread.Join()

                If InputHandler.Exception IsNot Nothing Then
                    Throw InputHandler.Exception
                End If

                Return InputHandler.Result
            Else
                Return InternalInputBox(Prompt, Title, DefaultResponse, XPos, YPos, ParentWindow)
            End If
        End Function

        Private Function GetTitleFromAssembly(ByVal CallingAssembly As System.Reflection.Assembly) As String

            Dim Title As String

            'Get the Assembly name of the calling assembly 
            'Assembly.GetName requires PathDiscovery permission so we try this first
            'and if it throws we catch the security exception and parse the name
            'from the full assembly name
            Try
                Title = CallingAssembly.GetName().Name
            Catch ex As SecurityException
                Dim FullName As String = CallingAssembly.FullName

                'Find the text up to the first comma. Note, this fails if the assembly has
                'a comma in its name
                Dim FirstCommaLocation As Integer = FullName.IndexOf(","c)
                If FirstCommaLocation >= 0 Then
                    Title = FullName.Substring(0, FirstCommaLocation)
                Else
                    'The name is not in the format we're expecting so return an empty string
                    Title = ""
                End If
            End Try

            Return Title

        End Function

        Private Function InternalInputBox(ByVal Prompt As String, ByVal Title As String, ByVal DefaultResponse As String, ByVal XPos As Integer, ByVal YPos As Integer, ByVal ParentWindow As System.Windows.Forms.IWin32Window) As String
            Dim Box As VBInputBox = New VBInputBox(Prompt, Title, DefaultResponse, XPos, YPos)
            Box.ShowDialog(ParentWindow)

            ' Fix FxCop violation DisposeObjectsBeforeLosingScope
            InternalInputBox = Box.Output
            Box.Dispose()
        End Function

        <HostProtection(Resources:=HostProtectionResource.UI)> _
        Public Function MsgBox(ByVal Prompt As Object, Optional ByVal Buttons As MsgBoxStyle = MsgBoxStyle.OKOnly, Optional ByVal Title As Object = Nothing) As MsgBoxResult
            Dim sPrompt As String = Nothing
            Dim sTitle As String
            Dim vbhost As CompilerServices.IVbHost
            Dim ParentWindow As System.Windows.Forms.IWin32Window = Nothing

            vbhost = CompilerServices.HostServices.VBHost
            If Not vbhost Is Nothing Then
                ParentWindow = vbhost.GetParentWindow()
            End If

            'Only allow legal button combinations to be set, one choice from each group
            'These bit constants are defined in System.Windows.Forms.MessageBox
            'Low-order 4 bits (0x000f), legal values: 0, 1, 2, 3, 4, 5
            '     next 4 bits (0x00f0), legal values: 0, &H10, &H20, &H30, &H40
            '     next 4 bits (0x0f00), legal values: 0, &H100, &H200
            If ((Buttons And &HFI) > MsgBoxStyle.RetryCancel) OrElse ((Buttons And &HF0I) > MsgBoxStyle.Information) _
                OrElse ((Buttons And &HF00I) > MsgBoxStyle.DefaultButton3) Then
                Buttons = MsgBoxStyle.OKOnly
            End If

            Try
                If Not Prompt Is Nothing Then
                    sPrompt = DirectCast(Conversions.ChangeType(Prompt, GetType(String)), String)
                End If
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValueType2, "Prompt", "String"))
            End Try

            Try
                If Title Is Nothing Then
                    If vbhost Is Nothing Then
                        sTitle = GetTitleFromAssembly(System.Reflection.Assembly.GetCallingAssembly())
                    Else
                        sTitle = vbhost.GetWindowTitle()
                    End If
                Else
                    sTitle = CStr(Title) 'allows the title to be an expression, e.g. msgbox(prompt, Title:=1+5)
                End If
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValueType2, "Title", "String"))
            End Try

            Return CType(System.Windows.Forms.MessageBox.Show(ParentWindow, sPrompt, sTitle, _
                 CType(Buttons And &HF, System.Windows.Forms.MessageBoxButtons), _
                 CType(Buttons And &HF0, System.Windows.Forms.MessageBoxIcon), _
                 CType(Buttons And &HF00, System.Windows.Forms.MessageBoxDefaultButton), _
                 CType(Buttons And &HFFFFF000, System.Windows.Forms.MessageBoxOptions)), _
                 MsgBoxResult)
        End Function

#End If ' Not TELESTO

        '============================================================================
        ' String functions.
        '============================================================================
        Public Function Choose(ByVal Index As Double, ByVal ParamArray Choice() As Object) As Object

            Dim FixedIndex As Integer = CInt(Fix(Index) - 1) 'ParamArray is 0 based, but Choose assumes 1 based 

            If Choice.Rank <> 1 Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_RankEQOne1, "Choice"))
            ElseIf FixedIndex < 0 OrElse FixedIndex > Choice.GetUpperBound(0) Then
                Return Nothing
            End If

            Return Choice(FixedIndex)
        End Function

        Public Function IIf(ByVal Expression As Boolean, ByVal TruePart As Object, ByVal FalsePart As Object) As Object
            If Expression Then
                Return TruePart
            End If

            Return FalsePart
        End Function
#End If 'Not LATEBINDING


        Friend Function IIf(Of T)(ByVal Condition As Boolean, ByVal TruePart As T, ByVal FalsePart As T) As T
            If Condition Then
                Return TruePart
            End If

            Return FalsePart
        End Function

#If Not LATEBINDING Then

        Public Function Partition(ByVal Number As Long, ByVal Start As Long, ByVal [Stop] As Long, ByVal Interval As Long) As String
            'CONSIDER: Change to use StringBuilder
            Dim Lower As Long
            Dim Upper As Long
            Dim NoUpper As Boolean
            Dim NoLower As Boolean
            Dim Buffer As String = Nothing
            Dim Buffer1 As String
            Dim Buffer2 As String
            Dim Spaces As Long

            'Validate arguments
            If (Start < 0) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Start"))
            End If

            If ([Stop] <= Start) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Stop"))
            End If

            If (Interval < 1) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Interval"))
            End If

            'Check for before-first and after-last ranges
            If Number < Start Then
                Upper = Start - 1
                NoLower = True
            ElseIf Number > [Stop] Then
                Lower = [Stop] + 1
                NoUpper = True
            ElseIf Interval = 1 Then 'This is a special case
                Lower = Number
                Upper = Number
            Else
                'Calculate the upper and lower ranges
                'Note the use of Integer division "\" which truncates to whole number
                Lower = ((Number - Start) \ Interval) * Interval + Start
                Upper = Lower + Interval - 1

                'Adjust for first and last ranges
                If Upper > [Stop] Then
                    Upper = [Stop]
                End If

                If Lower < Start Then
                    Lower = Start
                End If
            End If

            'Build-up the string.  Calculate number of spaces needed: VB3 uses Stop + 1.
            'This may seem bogus but it has to be this way for VB3 compatibilty.
            Buffer1 = CStr([Stop] + 1)
            Buffer2 = CStr([Start] - 1)

            If Len(Buffer1) > Len(Buffer2) Then
                Spaces = Len(Buffer1)
            Else
                Spaces = Len(Buffer2)
            End If

            'Handle case where Upper is -1 and Stop < 9
            If NoLower Then
                Buffer1 = CStr(Upper)
                If Spaces < Len(Buffer1) Then
                    Spaces = Len(Buffer1)
                End If
            End If

            'Insert lower-end of partition range.
            If NoLower Then
                InsertSpaces(Buffer, Spaces)
            Else
                InsertNumber(Buffer, Lower, Spaces)
            End If

            'Insert the partition 
            Buffer = Buffer & ":"

            'Insert upper-end of partition range
            If NoUpper Then
                InsertSpaces(Buffer, Spaces)
            Else
                InsertNumber(Buffer, Upper, Spaces)
            End If

            Return Buffer
        End Function


        Private Sub InsertSpaces(ByRef Buffer As String, ByVal Spaces As Long)
            Do While Spaces > 0 'consider:  - use stringbuilder
                Buffer = Buffer & " "
                Spaces = Spaces - 1
            Loop
        End Sub


        Private Sub InsertNumber(ByRef Buffer As String, ByVal Num As Long, ByVal Spaces As Long)
            Dim Buffer1 As String 'consider:  - use stringbuilder

            'Convert number to a string
            Buffer1 = CStr(Num)

            'Insert leading spaces
            InsertSpaces(Buffer, Spaces - Len(Buffer1))

            'Append string
            Buffer = Buffer & Buffer1
        End Sub


        Public Function Switch(ByVal ParamArray VarExpr() As Object) As Object
            Dim Elements As Integer
            Dim Index As Integer

            If VarExpr Is Nothing Then
                Return Nothing
            End If

            Elements = VarExpr.Length
            Index = 0

            'Ensure we have an even number of arguments (0 based)
            If (Elements Mod 2) <> 0 Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "VarExpr"))
            End If

            Do While Elements > 0
                If CBool(VarExpr(Index)) Then
                    Return VarExpr(Index + 1)
                End If

                Index += 2
                Elements -= 2
            Loop

            Return Nothing 'If nothing matched above
        End Function

#If Not TELESTO Then

        '============================================================================
        ' Registry functions.
        '============================================================================

        <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
        Public Sub DeleteSetting(ByVal AppName As String, Optional ByVal Section As String = Nothing, Optional ByVal Key As String = Nothing)
            Dim AppSection As String
            Dim UserKey As RegistryKey
            Dim AppSectionKey As RegistryKey = Nothing

            CheckPathComponent(AppName)
            AppSection = FormRegKey(AppName, Section)

            Try
                UserKey = Registry.CurrentUser

                If IsNothing(Key) OrElse (Key.Length = 0) Then
                    UserKey.DeleteSubKeyTree(AppSection)
                Else
                    AppSectionKey = UserKey.OpenSubKey(AppSection, True)
                    If AppSectionKey Is Nothing Then
                        Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Section"))
                    End If

                    AppSectionKey.DeleteValue(Key)
                End If

            Catch ex As Exception
                Throw ex
            Finally
                If AppSectionKey IsNot Nothing Then
                    AppSectionKey.Close()
                End If
            End Try
        End Sub


        'No HostProtection attribute because the code already directly or indirectly has a demand.
        Public Function GetAllSettings(ByVal AppName As String, ByVal Section As String) As String(,)
            Dim rk As RegistryKey
            Dim sAppSect As String
            Dim i As Integer
            Dim lUpperBound As Integer
            Dim sValueNames() As String
            Dim sValues(,) As String
            Dim o As Object
            Dim sName As String

            ' Check for empty string in path
            CheckPathComponent(AppName)
            CheckPathComponent(Section)
            sAppSect = FormRegKey(AppName, Section)
            rk = Registry.CurrentUser.OpenSubKey(sAppSect)


            If rk Is Nothing Then
                Return Nothing
            End If

            GetAllSettings = Nothing
            Try
                If rk.ValueCount <> 0 Then
                    sValueNames = rk.GetValueNames()
                    lUpperBound = sValueNames.GetUpperBound(0)
                    ReDim sValues(lUpperBound, 1)

                    For i = 0 To lUpperBound
                        sName = sValueNames(i)

                        'Assign name
                        sValues(i, 0) = sName

                        'Assign value
                        o = rk.GetValue(sName)

                        If (Not o Is Nothing) AndAlso (TypeOf o Is String) Then
                            sValues(i, 1) = o.ToString()
                        End If
                    Next i

                    GetAllSettings = sValues
                End If

            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex

            Catch ex As Exception
                'Consume the exception

            Finally
                rk.Close()
            End Try
        End Function


        Public Function GetSetting(ByVal AppName As String, ByVal Section As String, ByVal Key As String, Optional ByVal [Default] As String = "") As String
            Dim rk As RegistryKey = Nothing
            Dim sAppSect As String
            Dim o As Object

            'Check for empty strings
            CheckPathComponent(AppName)
            CheckPathComponent(Section)
            CheckPathComponent(Key)
            If [Default] Is Nothing Then
                [Default] = ""
            End If

            'Open the sub key
            sAppSect = FormRegKey(AppName, Section)
            Try
                rk = Registry.CurrentUser.OpenSubKey(sAppSect)    'By default, does not request write permission

                'Get the key's value
                If rk Is Nothing Then
                    Return [Default]
                End If

                o = rk.GetValue(Key, [Default])
            Finally
                If rk IsNot Nothing Then
                    rk.Close()
                End If
            End Try

            If o Is Nothing Then
                Return Nothing
            ElseIf TypeOf o Is String Then ' - odd that this is required to be a string when it isn't in GetAllSettings() above...
                Return DirectCast(o, String)
            Else
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue))
            End If
        End Function


        'No HostProtection attribute because the code already directly or indirectly has a demand.
        Public Sub SaveSetting(ByVal AppName As String, ByVal Section As String, ByVal Key As String, ByVal Setting As String)
            Dim rk As RegistryKey
            Dim sIniSect As String

            ' Check for empty string in path
            CheckPathComponent(AppName)
            CheckPathComponent(Section)
            CheckPathComponent(Key)

            sIniSect = FormRegKey(AppName, Section)
            rk = Registry.CurrentUser.CreateSubKey(sIniSect)

            If rk Is Nothing Then
                'Subkey could not be created
                Throw New ArgumentException(GetResourceString(ResID.Interaction_ResKeyNotCreated1, sIniSect))
            End If

            Try
                rk.SetValue(Key, Setting)
            Catch ex As Exception
                'CONSIDER: Should we throw a different exception?
                Throw ex
            Finally
                rk.Close()
            End Try
        End Sub

        '============================================================================
        ' Private functions.
        '============================================================================
        Private Function FormRegKey(ByVal sApp As String, ByVal sSect As String) As String
            Const REGISTRY_INI_ROOT As String = "Software\VB and VBA Program Settings"
            'Forms the string for the key value
            If IsNothing(sApp) OrElse (sApp.Length = 0) Then
                FormRegKey = REGISTRY_INI_ROOT
            ElseIf IsNothing(sSect) OrElse (sSect.Length = 0) Then
                FormRegKey = REGISTRY_INI_ROOT & "\" & sApp
            Else
                FormRegKey = REGISTRY_INI_ROOT & "\" & sApp & "\" & sSect
            End If
        End Function


        Private Sub CheckPathComponent(ByVal s As String)
            If (s Is Nothing) OrElse (s.Length = 0) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_PathNullOrEmpty))
            End If
        End Sub


        <System.Runtime.InteropServices.ComVisibleAttribute(True), _
         System.Runtime.InteropServices.GuidAttribute("0000010B-0000-0000-C000-000000000046"), _
         System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)> _
        Private Interface IPersistFile

            Sub GetClassID(ByRef pClassID As Guid)
            Sub IsDirty()
            Sub Load(ByVal pszFileName As String, ByVal dwMode As Integer)
            Sub Save(ByVal pszFileName As String, ByVal fRemember As Integer)
            Sub SaveCompleted(ByVal pszFileName As String)
            Function GetCurFile() As String
        End Interface

        <SecuritySafeCritical()> _
        <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        Public Function CreateObject(ByVal ProgId As String, Optional ByVal ServerName As String = "") As Object
            'Creates local or remote COM2 objects.  Should not be used to create COM+ objects.
            'Applications that need to be STA should set STA either on their Sub Main via STAThreadAttribute
            'or through Thread.CurrentThread.ApartmentState - the VB runtime will not change this.
            'DO NOT SET THREAD STATE - Thread.CurrentThread.ApartmentState = ApartmentState.STA

            Dim t As Type

            If ProgId.Length = 0 Then
                Throw VbMakeException(vbErrors.CantCreateObject)
            End If

            If ServerName Is Nothing OrElse ServerName.Length = 0 Then
                ServerName = Nothing
            Else
                'Does the ServerName match the MachineName?
                If String.Compare(Environment.MachineName, ServerName, StringComparison.OrdinalIgnoreCase) = 0 Then
                    ServerName = Nothing
                End If
            End If

            Try
                If ServerName Is Nothing Then
                    t = Type.GetTypeFromProgID(ProgId)
                Else
                    t = Type.GetTypeFromProgID(ProgId, ServerName, True)
                End If

                Return System.Activator.CreateInstance(t)
            Catch e As COMException
                If e.ErrorCode = &H800706BA Then                    '&H800706BA = The RPC Server is unavailable
                    Throw VbMakeException(vbErrors.ServerNotFound)
                Else
                    Throw VbMakeException(vbErrors.CantCreateObject)
                End If
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch e As Exception
                Throw VbMakeException(vbErrors.CantCreateObject)
            End Try
        End Function

        <SecuritySafeCritical()> _
        <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode)> _
        Public Function GetObject(Optional ByVal PathName As String = Nothing, Optional ByVal [Class] As String = Nothing) As Object
            'Only works for Com2 objects, not for COM+ objects.
            Dim o As Object
            Dim t As Type
            Dim Persist As IPersistFile

            If Len([Class]) = 0 Then
                Try
                    Return Marshal.BindToMoniker([PathName])
                Catch ex As StackOverflowException
                    Throw ex
                Catch ex As OutOfMemoryException
                    Throw ex
                Catch ex As System.Threading.ThreadAbortException
                    Throw ex
                Catch
                    Throw VbMakeException(vbErrors.CantCreateObject)
                End Try
            Else
                If PathName Is Nothing Then
                    Try
                        Return Marshal.GetActiveObject([Class])
                    Catch ex As StackOverflowException
                        Throw ex
                    Catch ex As OutOfMemoryException
                        Throw ex
                    Catch ex As System.Threading.ThreadAbortException
                        Throw ex
                    Catch
                        Throw VbMakeException(vbErrors.CantCreateObject)
                    End Try
                ElseIf Len(PathName) = 0 Then
                    Try
                        t = Type.GetTypeFromProgID([Class])
                        Return System.Activator.CreateInstance(t)
                    Catch ex As StackOverflowException
                        Throw ex
                    Catch ex As OutOfMemoryException
                        Throw ex
                    Catch ex As System.Threading.ThreadAbortException
                        Throw ex
                    Catch
                        Throw VbMakeException(vbErrors.CantCreateObject)
                    End Try
                Else
                    Try
                        o = Marshal.GetActiveObject([Class])
                        Persist = CType(o, IPersistFile)
                    Catch ex As StackOverflowException
                        Throw ex
                    Catch ex As OutOfMemoryException
                        Throw ex
                    Catch ex As System.Threading.ThreadAbortException
                        Throw ex
                    Catch
                        Throw VbMakeException(vbErrors.OLEFileNotFound)
                    End Try

                    Try
                        Persist.Load([PathName], 0)
                    Catch ex As StackOverflowException
                        Throw ex
                    Catch ex As OutOfMemoryException
                        Throw ex
                    Catch ex As System.Threading.ThreadAbortException
                        Throw ex
                    Catch
                        Throw VbMakeException(vbErrors.CantCreateObject)
                    End Try

                    Return Persist
                End If
            End If
        End Function

#Region " BACKWARDS COMPATIBILITY "

        'WARNING WARNING WARNING WARNING WARNING
        'This code exists to support Everett compiled applications.  Make sure you understand
        'the backwards compatibility ramifications of any edit you make in this region.
        'WARNING WARNING WARNING WARNING WARNING

        '============================================================================
        ' Object/latebound functions.
        '============================================================================
        Public Function CallByName(ByVal ObjectRef As System.Object, ByVal ProcName As String, ByVal UseCallType As CallType, ByVal ParamArray Args() As Object) As Object
            Select Case UseCallType

                Case CallType.Method
                    'Need to use LateGet, because we are returning a value
                    Return CompilerServices.LateBinding.InternalLateCall(ObjectRef, Nothing, ProcName, Args, Nothing, Nothing, False)

                Case CallType.Get
                    Return CompilerServices.LateBinding.LateGet(ObjectRef, Nothing, ProcName, Args, Nothing, Nothing)

                Case CallType.Let, _
                     CallType.Set
                    CompilerServices.LateBinding.InternalLateSet(ObjectRef, Nothing, ProcName, Args, Nothing, False, UseCallType)
                    Return Nothing

                Case Else
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "CallType"))
            End Select
        End Function
#End Region

#End If 'Not TELESTO
#End If 'Not LATEBINDING Then
    End Module

End Namespace

