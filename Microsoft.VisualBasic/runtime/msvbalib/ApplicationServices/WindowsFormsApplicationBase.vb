' Copyright (c) Microsoft Corporation.  All rights reserved.
 '******************************************************************************

Option Strict On
Option Explicit On

Imports System
Imports System.Reflection
Imports System.Threading
Imports System.Diagnostics
Imports System.ComponentModel
Imports System.Globalization
Imports System.Security
Imports System.Security.AccessControl
Imports System.Security.Permissions
Imports System.Runtime.Remoting
Imports System.Runtime.Remoting.Lifetime
Imports System.Runtime.Remoting.Channels
Imports System.Runtime.Remoting.Channels.Tcp
Imports System.Runtime.Versioning
Imports Microsoft.Win32.SafeHandles
Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.ApplicationServices

    '!!!!!!!!!! Any changes to this enum must be reflected in ValidateAuthenticationModeEnumValue()
    Public Enum AuthenticationMode
        Windows
        ApplicationDefined
    End Enum

    '!!!!!!!!!! Any changes to this enum must be reflected in ValidateShutdownModeEnumValue()
    Public Enum ShutdownMode
        AfterMainFormCloses
        AfterAllFormsClose
    End Enum

#Region "Application model Event Delegates and Event Argument definitions"

    '--- Event Argument definitions

    '''**************************************************************************
    ''' ;UnhandledExceptionEventArgs 
    ''' <summary>
    ''' Provides the exception encountered along with a flag on whether to abort the program
    ''' </summary>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced), System.Runtime.InteropServices.ComVisible(False)> _
    Public Class UnhandledExceptionEventArgs : Inherits System.Threading.ThreadExceptionEventArgs

        Sub New(ByVal exitApplication As Boolean, ByVal exception As System.Exception)
            MyBase.new(exception)
            m_ExitApplication = exitApplication
        End Sub

        '''**************************************************************************
        ''' ;ExitApplication
        ''' <summary>
        ''' Indicates whether the application should exit upon exiting the exception handler
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Public Property ExitApplication() As Boolean
            Get
                Return m_ExitApplication
            End Get
            Set(ByVal value As Boolean)
                m_ExitApplication = value
            End Set
        End Property

        Private m_ExitApplication As Boolean

    End Class

    '''**************************************************************************
    ''' ;StartupEventArgs
    ''' <summary>
    ''' Provides context for the Startup event.
    ''' </summary>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced), System.Runtime.InteropServices.ComVisible(False)> _
    Public Class StartupEventArgs : Inherits System.ComponentModel.CancelEventArgs

        '''**************************************************************************
        ''' ;New
        ''' <summary>
        ''' Create a new instance of the StartupEventArgs.
        ''' </summary>
        ''' <param name="Args"></param>
        ''' <remarks></remarks>
        Public Sub New(ByVal args As System.Collections.ObjectModel.ReadOnlyCollection(Of String))
            If args Is Nothing Then
                args = New System.Collections.ObjectModel.ReadOnlyCollection(Of String)(Nothing)
            End If
            m_CommandLine = args
        End Sub

        '''**************************************************************************
        ''' ;CommandLine
        ''' <summary>
        ''' Returns the command line sent to this application
        ''' </summary>
        ''' <remarks></remarks>
        Public ReadOnly Property CommandLine() As System.Collections.ObjectModel.ReadOnlyCollection(Of String)
            Get
                Return m_CommandLine
            End Get
        End Property

        Private m_CommandLine As System.Collections.ObjectModel.ReadOnlyCollection(Of String)
    End Class

    '''**************************************************************************
    ''' ;StartupNextInstanceEventArgs
    ''' <summary>
    ''' Provides context for the StartupNextInstance event.
    ''' </summary>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced)> Public Class StartupNextInstanceEventArgs : Inherits EventArgs

        '''**************************************************************************
        ''' ;New
        ''' <summary>
        ''' Create a new instance of the StartupNextInstanceEventArgs.
        ''' </summary>
        ''' <param name="Args"></param>
        ''' <remarks></remarks>
        Public Sub New(ByVal args As System.Collections.ObjectModel.ReadOnlyCollection(Of String), ByVal bringToForegroundFlag As Boolean)
            If args Is Nothing Then
                args = New System.Collections.ObjectModel.ReadOnlyCollection(Of String)(Nothing)
            End If
            m_CommandLine = args
            m_BringToForeground = bringToForegroundFlag
        End Sub

        '''**************************************************************************
        ''' ;BringToForeground
        ''' <summary>
        ''' Indicates whether we will bring the application to the foreground when processing the
        ''' StartupNextInstance event.
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Public Property BringToForeground() As Boolean
            Get
                Return m_BringToForeground
            End Get
            Set(ByVal value As Boolean)
                m_BringToForeground = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;CommandLine
        ''' <summary>
        ''' Returns the command line sent to this application
        ''' </summary>
        ''' <remarks>I'm using Me.CommandLine so that it is consistent with my.net and to assure they 
        ''' always return the same values</remarks>
        Public ReadOnly Property CommandLine() As System.Collections.ObjectModel.ReadOnlyCollection(Of String)
            Get
                Return m_CommandLine
            End Get
        End Property

        Private m_BringToForeground As Boolean
        Private m_CommandLine As System.Collections.ObjectModel.ReadOnlyCollection(Of String)
    End Class

    '--- Event Delegate definitions

    '''**************************************************************************
    ''' ;StartupEventHandler
    ''' <summary>
    ''' Signature for the Startup event handler
    ''' </summary>
    ''' <param name="Sender"></param>
    ''' <param name="e"></param>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced)> Public Delegate Sub StartupEventHandler(ByVal sender As Object, ByVal e As StartupEventArgs)

    '''**************************************************************************
    ''' ;StartupNextInstanceEventHandler
    ''' <summary>
    ''' Signature for the StartupNextInstance event handler
    ''' </summary>
    ''' <param name="sender"></param>
    ''' <param name="e"></param>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced)> Public Delegate Sub StartupNextInstanceEventHandler(ByVal sender As Object, ByVal e As StartupNextInstanceEventArgs)

    '''**************************************************************************
    ''' ;ShutdownEventHandler
    ''' <summary>
    ''' Signature for the Shutdown event handler
    ''' </summary>
    ''' <param name="Sender"></param>
    ''' <param name="e"></param>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced)> Public Delegate Sub ShutdownEventHandler(ByVal sender As Object, ByVal e As EventArgs)

    '''**************************************************************************
    ''' ;UnhandledExceptionEventHandler
    ''' <summary>
    ''' Signature for the UnhandledException event handler
    ''' </summary>    ''' <param name="Sender"></param>
    ''' <param name="e"></param>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Advanced)> Public Delegate Sub UnhandledExceptionEventHandler(ByVal sender As Object, ByVal e As UnhandledExceptionEventArgs)
#End Region

    '''**************************************************************************
    ''' ;NoStartupFormException 
    ''' <summary>
    ''' Exception for when the WinForms VB application model isn't supplied with a startup form
    ''' </summary>
    ''' <remarks></remarks>
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <System.Serializable()> _
    Public Class NoStartupFormException : Inherits System.Exception

        '''********************************************************************
        ''';New
        ''' <summary>
        '''  Creates a new exception
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
            MyBase.New(GetResourceString(ResID.MyID.AppModel_NoStartupForm))
        End Sub

        Public Sub New(ByVal message As String)
            MyBase.New(message)
        End Sub

        Public Sub New(ByVal message As String, ByVal inner As System.Exception)
            MyBase.New(message, inner)
        End Sub

        ' Deserialization constructor must be defined since we are serializable
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Protected Sub New(ByVal info As System.Runtime.Serialization.SerializationInfo, ByVal context As System.Runtime.Serialization.StreamingContext)
            MyBase.New(info, context)
        End Sub
    End Class

    '''**************************************************************************
    ''' ;CantStartSingleInstanceException
    ''' <summary>
    ''' Exception for when we launch a single-instance application and it can't connect with the 
    ''' original instance.
    ''' </summary>
    ''' <remarks></remarks>
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <System.Serializable()> _
    Public Class CantStartSingleInstanceException : Inherits System.Exception

        '''********************************************************************
        ''';New
        ''' <summary>
        '''  Creates a new exception
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub New()
            MyBase.New(GetResourceString(ResID.MyID.AppModel_SingleInstanceCantConnect))
        End Sub

        Public Sub New(ByVal message As String)
            MyBase.New(message)
        End Sub

        Public Sub New(ByVal message As String, ByVal inner As System.Exception)
            MyBase.New(message, inner)
        End Sub

        ' Deserialization constructor must be defined since we are serializable
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Protected Sub New(ByVal info As System.Runtime.Serialization.SerializationInfo, ByVal context As System.Runtime.Serialization.StreamingContext)
            MyBase.New(info, context)
        End Sub
    End Class


    '''**************************************************************************
    ''' ;WindowsFormsApplicationBase 
    ''' <summary>
    ''' Provides the infrastructure for the VB Windows Forms application model
    ''' </summary>
    ''' <remarks>Don't put access on this definition.  The expanding class will define it
    ''' Note that this class is not safe for YUKON.  That's ok because this app model is only
    ''' used in Windows FORMS projects and YUKON doesn't allow WinForms projects.  So we won't be 
    ''' using the application model in YUKON.  If we ever reconsider, we'll have to rework how we handle 
    ''' exceptions in DoApplicationModel() </remarks>
    <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
     Public Class WindowsFormsApplicationBase : Inherits ConsoleApplicationBase

        Public Event Startup As StartupEventHandler
        Public Event StartupNextInstance As StartupNextInstanceEventHandler
        Public Event Shutdown As ShutdownEventHandler

        ' ;Event NetworkAvailabilityChanged 
        Public Custom Event NetworkAvailabilityChanged As Global.Microsoft.VisualBasic.Devices.NetworkAvailableEventHandler
            'This is a custom event because we want to hook up the NetworkAvailabilityChanged event only if the user writes a handler for it.
            'The reason being that it is very expensive to handle and kills our application startup perf.
            AddHandler(ByVal value As Global.Microsoft.VisualBasic.Devices.NetworkAvailableEventHandler)
                SyncLock m_NetworkAvailChangeLock
                    If m_NetworkAvailabilityEventHandlers Is Nothing Then m_NetworkAvailabilityEventHandlers = New System.Collections.ArrayList
                    m_NetworkAvailabilityEventHandlers.Add(value)
                    m_TurnOnNetworkListener = True 'We don't want to create the network object now - it takes a snapshot of the executionContext and our IPrincipal isn't on the thread yet.  We know we need to create it and we will at the appropriate time
                    If m_NetworkObject Is Nothing And m_FinishedOnInitilaize = True Then 'But the user may be doing an Addhandler of their own in which case we need to make sure to honor the request.  If we aren't past OnInitialize() yet we shouldn't do it but the flag above catches that case
                        m_NetworkObject = New Microsoft.VisualBasic.Devices.Network
                        AddHandler m_NetworkObject.NetworkAvailabilityChanged, AddressOf Me.NetworkAvailableEventAdaptor
                    End If
                End SyncLock
            End AddHandler

            RemoveHandler(ByVal value As Global.Microsoft.VisualBasic.Devices.NetworkAvailableEventHandler)
                If m_NetworkAvailabilityEventHandlers IsNot Nothing AndAlso m_NetworkAvailabilityEventHandlers.Count > 0 Then
                    m_NetworkAvailabilityEventHandlers.Remove(value)
                    'Last one to leave, turn out the lights...
                    If m_NetworkAvailabilityEventHandlers.Count = 0 Then
                        RemoveHandler m_NetworkObject.NetworkAvailabilityChanged, AddressOf Me.NetworkAvailableEventAdaptor
                        If m_NetworkObject IsNot Nothing Then
                            m_NetworkObject.DisconnectListener() 'Stop listening to network change events because we are going to go away
                            m_NetworkObject = Nothing 'no sense hanging on to this if nobody is listening.
                        End If
                    End If
                End If
            End RemoveHandler

            RaiseEvent(ByVal sender As Object, ByVal e As Global.Microsoft.VisualBasic.Devices.NetworkAvailableEventArgs)
                If m_NetworkAvailabilityEventHandlers IsNot Nothing Then
                    For Each handler As Global.Microsoft.VisualBasic.Devices.NetworkAvailableEventHandler In m_NetworkAvailabilityEventHandlers
                        Try
                            If handler IsNot Nothing Then handler.Invoke(sender, e)
                        Catch ex As Exception
                            If Not OnUnhandledException(New UnhandledExceptionEventArgs(True, ex)) Then
                                Throw 'the user didn't write a handler so throw the error up the chain
                            End If
                        End Try
                    Next
                End If
            End RaiseEvent
        End Event

        ';Event UnhandledException 
        Public Custom Event UnhandledException As UnhandledExceptionEventHandler
            'This is a custom event because we want to hook up System.Windows.Forms.Application.ThreadException only if the user writes a
            'handler for this event.  We only want to hook the ThreadException event if the user is handling this event because the act of listening to
            'Application.ThreadException causes WinForms to snuff exceptions and we only want WinForms to do that if we are assured that the user wrote their own handler
            'to deal with the error instead
            AddHandler(ByVal value As UnhandledExceptionEventHandler)
                If m_UnhandledExceptionHandlers Is Nothing Then m_UnhandledExceptionHandlers = New System.Collections.ArrayList
                m_UnhandledExceptionHandlers.Add(value)
                'Only add the listener once so we don't fire the UnHandledException event over and over for the same exception
                If m_UnhandledExceptionHandlers.Count = 1 Then AddHandler System.Windows.Forms.Application.ThreadException, AddressOf Me.OnUnhandledExceptionEventAdaptor
            End AddHandler

            RemoveHandler(ByVal value As UnhandledExceptionEventHandler)
                If m_UnhandledExceptionHandlers IsNot Nothing AndAlso m_UnhandledExceptionHandlers.Count > 0 Then
                    m_UnhandledExceptionHandlers.Remove(value)
                    'Last one to leave, turn out the lights...
                    If m_UnhandledExceptionHandlers.Count = 0 Then RemoveHandler System.Windows.Forms.Application.ThreadException, AddressOf Me.OnUnhandledExceptionEventAdaptor
                End If
            End RemoveHandler

            RaiseEvent(ByVal sender As Object, ByVal e As UnhandledExceptionEventArgs)
                If m_UnhandledExceptionHandlers IsNot Nothing Then
                    m_ProcessingUnhandledExceptionEvent = True 'In the case that we throw from the unhandled exception handler, we don't want to run the unhandled exception handler again
                    For Each handler As UnhandledExceptionEventHandler In m_UnhandledExceptionHandlers
                        If handler IsNot Nothing Then handler.Invoke(sender, e)
                    Next
                    m_ProcessingUnhandledExceptionEvent = False 'Now that we are out of the unhandled exception handler, treat exceptions normally again.
                End If
            End RaiseEvent
        End Event

        '= PUBLIC =============================================================

        '''**************************************************************************
        ''' ;New
        ''' <summary>
        ''' Constructs the application Shutdown/Startup model object
        ''' </summary>
        ''' <remarks>
        ''' We have to have a parameterless ctor because the platform specific Application object
        ''' derives from this one and it doesn't define a ctor because the partial class generated by the
        ''' designer does that to configure the application. </remarks>
        Public Sub New()
            Me.New(AuthenticationMode.Windows)
        End Sub

        '''**************************************************************************
        ''' ;New
        ''' <summary>
        ''' Constructs the application Shutdown/Startup model object
        ''' </summary>
        ''' <remarks></remarks>
        <SecuritySafeCritical()> _
        Public Sub New(ByVal authenticationMode As AuthenticationMode)
            MyBase.New()
            m_Ok2CloseSplashScreen = True 'Default to true in case there is no splash screen so we won't block forever waiting for it to appear.
            ValidateAuthenticationModeEnumValue(authenticationMode, "authenticationMode")

            'Setup Windows Authentication if that's what the user wanted.  Note, we want to do this now, before the Network object gets created because
            'the network object will be doing a AsyncOperationsManager.CreateOperation() which captures the execution context.  So we have to have our 
            'principal on the thread before that happens.  
            If authenticationMode = authenticationMode.Windows Then
                Try
                    'Consider:  - sadly, a call to: System.Security.SecurityManager.IsGranted(New SecurityPermission(SecurityPermissionFlag.ControlPrincipal))  
                    'will only check THIS caller so you'll always get TRUE.  What is needed is a way to get to the value of this on a demand basis.  So I try/catch instead for now
                    'but would rather be able to IF my way around this block.
                    System.Threading.Thread.CurrentPrincipal = New System.Security.Principal.WindowsPrincipal(System.Security.Principal.WindowsIdentity.GetCurrent)
                Catch ex As System.Security.SecurityException
                End Try
            End If

            m_AppContext = New WinFormsAppContext(Me)

            'We need to set the WindowsFormsSynchronizationContext because the network object is going to get created after this ctor runs
            '(network gets created during event hookup) and we need the context in place for it to latch on to.  The WindowsFormsSynchronizationContext
            'won't otherwise get created until OnCreateMainForm() when the startup form is created and by then it is too late.
            'When the startup form gets created, WinForms is going to push our context into the previous context and then restore it when Application.Run() exits.
            Call New System.Security.Permissions.UIPermission(UIPermissionWindow.AllWindows).Assert()
            m_AppSyncronizationContext = AsyncOperationManager.SynchronizationContext
            AsyncOperationManager.SynchronizationContext = New System.Windows.Forms.WindowsFormsSynchronizationContext()
            System.Security.PermissionSet.RevertAssert() 'CLR also reverts if we throw or when we return from this function
        End Sub

        '''**************************************************************************
        ''' ;Run 
        ''' <summary>
        ''' Entry point to kick off the VB Startup/Shutdown Application model
        ''' </summary>
        ''' <param name="commandLine">The command line from Main()</param>
        ''' <remarks></remarks>
        <SecuritySafeCritical()> _
        Public Sub Run(ByVal commandLine As String())
            'Microsoft.VisualBasic.MsgBox("Attach Debugger") 'jtw - uncomment to facilitate debugging single-instance applications
            'Prime the command line args with what we recieve from Main() so that Click-Once windows apps don't have to do a System.Environment call which would require permissions.
            MyBase.InternalCommandLine = New System.Collections.ObjectModel.ReadOnlyCollection(Of String)(commandLine)

            'Is this a single-instance application?
            If Not Me.IsSingleInstance Then
                DoApplicationModel() 'This isn't a Single-Instance application
            Else 'This is a Single-Instance application
                Dim ApplicationInstanceID As String = GetApplicationInstanceID(Assembly.GetCallingAssembly) 'Note: Must pass the calling assembly from here so we can get the running app.  Otherwise, can break single instance - see hosting problem in Whidbey 415231
                m_MemoryMappedID = ApplicationInstanceID & "Map"
                Dim SemaphoreID As String = ApplicationInstanceID & "Event"
                Dim MessageRecievedSemaphoreID As String = ApplicationInstanceID & "Event2"
                m_StartNextInstanceCallback = New System.Threading.SendOrPostCallback(AddressOf Me.OnStartupNextInstanceMarshallingAdaptor)

                'Create our event and ACL it down so only the current user has rights to use it
                Call New SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.ControlPrincipal).Assert() 'I need to get the current identity so I can do my ACLS
                Dim CurrentUser As String = System.Security.Principal.WindowsIdentity.GetCurrent().Name()
                Dim OperatingSystemSupportsAuthentication As Boolean = CurrentUser <> "" 'The OS must support the concept of a NTUser to support authentication on the server and in this case the app is a server
                System.Security.CodeAccessPermission.RevertAssert()

                Dim WeAreTheFirstInstance As Boolean
                If OperatingSystemSupportsAuthentication = True Then 'We can only use an ACL if we have a user (EventWaitHandleAccessRule creates a NTAccount object)
                    'Build an ACL so we can lock down our semaphore.  Don't want external users to be able to signal/unsignal our event which leads to crashes or denial of service, respectively.
                    Dim WaitHandleRules As EventWaitHandleAccessRule = New EventWaitHandleAccessRule(CurrentUser, EventWaitHandleRights.FullControl, AccessControlType.Allow)
                    Dim EventWaitHandleSecurity As New EventWaitHandleSecurity()
                    Call New SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.ControlPrincipal).Assert() 'I need to get the current identity so I can do my ACLS
                    EventWaitHandleSecurity.AddAccessRule(WaitHandleRules)
                    System.Security.CodeAccessPermission.RevertAssert()

                    'Event names are scoped such that two different users logged onto Windows at the same time will get unique events even though the event names are the same.
                    'EventWaitHandle uses the SafeWaitHandle internally and correct creation pattern so this is the safe way to prevent a handle leak.
                    m_FirstInstanceSemaphore = New System.Threading.EventWaitHandle(False, Threading.EventResetMode.ManualReset, SemaphoreID, WeAreTheFirstInstance, EventWaitHandleSecurity)
                    m_MessageRecievedSemaphore = New System.Threading.EventWaitHandle(False, Threading.EventResetMode.AutoReset, MessageRecievedSemaphoreID, False, EventWaitHandleSecurity)
                Else
                    'win 95/98/me don't support ACLS
                    m_FirstInstanceSemaphore = New System.Threading.EventWaitHandle(False, Threading.EventResetMode.ManualReset, SemaphoreID, WeAreTheFirstInstance)
                    m_MessageRecievedSemaphore = New System.Threading.EventWaitHandle(False, Threading.EventResetMode.AutoReset, MessageRecievedSemaphoreID)
                End If

                If WeAreTheFirstInstance Then
                    '--- This is the first instance of a single-instance application to run.  This is the instance that subsequent instances will attach to.
                    Try
                        Dim ServerChannel As TcpServerChannel = DirectCast(RegisterChannel(ChannelType.Server, OperatingSystemSupportsAuthentication), TcpServerChannel) 'Register the server channel that will listen for incoming connection messages from clients
                        Dim Communicator As New RemoteCommunicator(Me, m_MessageRecievedSemaphore)
                        Dim NameOfRemoteCommunicator As String = ApplicationInstanceID & ".rem"

                        Call New System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.RemotingConfiguration).Assert()
                        RemotingServices.Marshal(Communicator, NameOfRemoteCommunicator) 'Convert this marshal by ref object into an instance of an ObjRef class which can be serialized for transmission between application domains (publishes this with the remoting layer so that it knows about this object - this essentially maps the object to the URL.  At creation the object was ready to go - this just tells the remoting layer that the class is ready to be connected to
                        System.Security.CodeAccessPermission.RevertAssert()

                        'Stashes the URL for our remote object where we can get at it on subsequent launches of this app
                        Dim URLofRemoteCommunicatorObject As String = ServerChannel.GetUrlsForUri(NameOfRemoteCommunicator)(0)
                        WriteUrlToMemoryMappedFile(URLofRemoteCommunicatorObject) 'subsequent instances will get the memory mapped file to find the URL for our remote object
                        m_FirstInstanceSemaphore.Set() 'We are now far enough along to allow subsequent instances to attach to this one.
                        DoApplicationModel()
                    Finally 'Application has exited
                        If m_MessageRecievedSemaphore IsNot Nothing Then
                            m_MessageRecievedSemaphore.Close()
                        End If
                        If m_FirstInstanceSemaphore IsNot Nothing Then 'we let this go more aggressively earlier during OnRun() but only if we shutdown cleanly so check again.
                            m_FirstInstanceSemaphore.Close() 'Let go so that subsequent instances don't try to connect to this process which is on its way out.
                        End If
                        If m_FirstInstanceMemoryMappedFileHandle IsNot Nothing AndAlso Not m_FirstInstanceMemoryMappedFileHandle.IsInvalid Then
                            m_FirstInstanceMemoryMappedFileHandle.Close()
                        End If
                    End Try
                Else '--- We are launching a subsequent instance.
                    'Wait until the original app is on its feet so we can attach to it.  We need to prevent a race condition where a subsequent instance starts up before this original
                    '  instance is ready to be attached to.  Surprisingly, this race can easily occur.  When opening several files with a single-instance app from file explorer, for instance.
                    '  See VS Whidbey #346690
                    '  The timeout is only there so we don't wait forever on a horked semaphore in case something bad happened and couldn't get cleaned up for whatever reason in the first instance.
                    Dim ObtainedSignal As Boolean = m_FirstInstanceSemaphore.WaitOne(SECOND_INSTANCE_TIMEOUT, False)
                    If Not ObtainedSignal Then Throw New CantStartSingleInstanceException

                    'We are good to attach to the original instance
                    RegisterChannel(ChannelType.Client, OperatingSystemSupportsAuthentication) 'register a client channel to communicate with the remote object.  We can't use a default channel because we need authentication turned on to match the server channel we established during the 1st instance
                    Dim URLofRemoteCommunicatorObject As String = ReadUrlFromMemoryMappedFile()
                    If URLofRemoteCommunicatorObject Is Nothing Then
                        Throw New CantStartSingleInstanceException
                    End If
                    Dim Communicator As RemoteCommunicator = DirectCast(RemotingServices.Connect(GetType(RemoteCommunicator), URLofRemoteCommunicatorObject), RemoteCommunicator)

                    'To run single instance in low-trust (e.g. InternetZone), we need the following permissions:
                    Dim PermissionsToDoRemoting As New System.Security.PermissionSet(PermissionState.None)
                    PermissionsToDoRemoting.AddPermission(New System.Security.Permissions.SecurityPermission(SecurityPermissionFlag.SerializationFormatter Or SecurityPermissionFlag.ControlPrincipal Or SecurityPermissionFlag.UnmanagedCode))
                    PermissionsToDoRemoting.AddPermission(New System.Net.DnsPermission(PermissionState.Unrestricted)) 'Unrestricted is required
                    PermissionsToDoRemoting.AddPermission(New System.Net.SocketPermission(System.Net.NetworkAccess.Connect, Net.TransportType.Tcp, HOST_NAME, System.Net.SocketPermission.AllPorts)) 'I use AllPorts because we don't know which port we will have hooked up on.  We get whatever port was available at runtime
                    PermissionsToDoRemoting.AddPermission(New System.Security.Permissions.EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERNAME")) 'Environment permissions
                    PermissionsToDoRemoting.Assert()

                    Communicator.RunNextInstance(MyBase.CommandLineArgs) 'Fires the StartupNextInstance event in the original instance of the app, then returns.
                    System.Security.PermissionSet.RevertAssert() 'revert all previous asserts for the current frame.  No need to finally block this - CLR removes asserts if an exception is thrown.

                    'Because we posted through a <OneWay> remoting call, we need to provide time for the message to be sent before we exit this process.  To guarantee that, we'll wait until the message gets through
                    ObtainedSignal = m_MessageRecievedSemaphore.WaitOne(ATTACH_TIMEOUT, False)
                    If Not ObtainedSignal Then Throw New CantStartSingleInstanceException
                End If
            End If 'Single-Instance application
        End Sub

        '''**************************************************************************
        ''' ;OpenForms
        ''' <summary>
        ''' Returns the collection of forms that are open.  We no longer have thread
        ''' affinity meaning that this is the WinForms collection that contains Forms that may
        ''' have been opened on another thread then the one we are calling in on right now.
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Public ReadOnly Property OpenForms() As System.Windows.Forms.FormCollection
            Get
                Return System.Windows.Forms.Application.OpenForms
            End Get
        End Property

        '''**************************************************************************
        ''' ;MainForm
        ''' <summary>
        ''' Provides access to the main form for this application
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Protected Property MainForm() As System.Windows.Forms.Form
            Get
                Return IIf(m_AppContext IsNot Nothing, m_AppContext.MainForm, Nothing)
            End Get
            Set(ByVal value As System.Windows.Forms.Form)
                If value Is Nothing Then
                    Throw ExceptionUtils.GetArgumentNullException("MainForm", ResID.MyID.General_PropertyNothing, "MainForm")
                End If
                If value Is m_SplashScreen Then
                    Throw New ArgumentException(GetResourceString(ResID.MyID.AppModel_SplashAndMainFormTheSame))
                End If
                m_AppContext.MainForm = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;SplashScreen
        ''' <summary>
        ''' Provides access to the splash screen for this application
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Public Property SplashScreen() As System.Windows.Forms.Form
            Get
                Return m_SplashScreen
            End Get
            Set(ByVal value As System.Windows.Forms.Form)
                If value IsNot Nothing AndAlso value Is m_AppContext.MainForm Then 'allow for the case where they set splash screen = nothing and mainForm is currently nothing
                    Throw New ArgumentException(GetResourceString(ResID.MyID.AppModel_SplashAndMainFormTheSame))
                End If
                m_SplashScreen = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;MinimumSplashScreenDisplayTime
        ''' <summary>
        ''' The splash screen timeout specifies whether there is a minimum time that the splash
        ''' screen should be displayed for.  When not set then the splash screen is hidden
        ''' as soon as the main form becomes active.
        ''' </summary>
        ''' <value>The minimum amount of time, in milliseconds, to display the splash screen.</value>
        ''' <remarks></remarks>
        Public Property MinimumSplashScreenDisplayTime() As Integer
            Get
                Return m_MinimumSplashExposure
            End Get
            Set(ByVal value As Integer)
                m_MinimumSplashExposure = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;UseCompatibleTextRendering
        ''' <summary>
        ''' Whidbey tried to change the text rendering engine from GDI+ (used in Everett) to GDI. 
        ''' This turned out to be a breaking change that didn't work out.
        ''' The idea is that new Whidbey apps will use the new GDI renderer by default,
        ''' but there must be a way to lock back to use the Everett GDI+ renderer.
        ''' The user can shadow this function to return True if they want their app
        ''' to use the Everett GDI+ render.  We read this function in Main() (My template) to 
        ''' determine how to set the text rendering flag on the WinForms application object.
        ''' </summary>
        ''' <value></value>
        ''' <returns>True - Use Everett GDI+ renderer.  False - use the Whidbey GDI renderer</returns>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> Protected Shared ReadOnly Property UseCompatibleTextRendering() As Boolean
            Get
                Return False
            End Get
        End Property

        '''**************************************************************************
        ''' ;ApplicationContext
        ''' <summary>
        ''' Provides the WinForms application context that we are running on
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> Public ReadOnly Property ApplicationContext() As System.Windows.Forms.ApplicationContext
            Get
                Return m_AppContext
            End Get
        End Property

        '''**************************************************************************
        ''' ;SaveMySettingsOnExit
        ''' <summary>
        ''' Informs My.Settings whether to save the settings on exit or not
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Public Property SaveMySettingsOnExit() As Boolean
            Get
                Return m_SaveMySettingsOnExit
            End Get
            Set(ByVal value As Boolean)
                m_SaveMySettingsOnExit = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;DoEvents
        ''' <summary>
        '''  Processes all windows messages currently in the message queue
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub DoEvents()
            System.Windows.Forms.Application.DoEvents()
        End Sub

        '= PROTECTED ==========================================================

        '''**************************************************************************
        ''' ;OnInitialize
        ''' <summary>
        ''' This exposes the first in a series of extensibility points for the Startup process.  By default, it shows
        ''' the splash screen and does rudimentary processing of the command line to see if /nosplash or its
        ''' variants was passed in.
        ''' </summary>
        ''' <param name="commandLineArgs"></param>
        ''' <returns>Returning True indicates that we should continue on with the application Startup sequence</returns>
        ''' <remarks>This extensibility point is exposed for people who want to override the Startup sequence at the earliest possible point
        ''' to </remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced), Global.System.STAThread()> _
        Protected Overridable Function OnInitialize(ByVal commandLineArgs As System.Collections.ObjectModel.ReadOnlyCollection(Of String)) As Boolean
            ' EnableVisualStyles
            If m_EnableVisualStyles Then
                System.Windows.Forms.Application.EnableVisualStyles()
            End If

            'We'll handle /nosplash for you
            If Not (commandLineArgs.Contains("/nosplash") OrElse Me.CommandLineArgs.Contains("-nosplash")) Then
                ShowSplashScreen()
            End If

            m_FinishedOnInitilaize = True 'we are now at a point where we can allow the network object to be created since the iprincipal is on the thread by now.
            Return True 'true means to not bail out but keep on running after OnIntiailize() finishes
        End Function

        '''**************************************************************************
        ''' ;OnStartup
        ''' <summary>
        ''' Extensibility point which raises the Startup event
        ''' </summary>
        ''' <param name="eventArgs"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> Protected Overridable Function OnStartup(ByVal eventArgs As StartupEventArgs) As Boolean
            eventArgs.Cancel = False
            'It is important not to create the network object until the ExecutionContext has everything on it.  By now the principal will be on the thread so
            'we can create the network object.  The timing is important because the network object has an AsyncOperationsManager in it that marshals
            'the network changed event to the main thread.  The asycnOperationsManager does a CreateOperation() which makes a copy of the executionContext
            'That execution context shows up on your thread during the callback so I delay creating the network object (and consequently the capturing of the
            'execution context) until the principal has been set on the thread.
            'this avoid the problem in VS whidbey 458908 where My.User isn't set during the NetworkAvailabilityChanged event.  This problem would just extend
            'itself to any future callback that involved the asyncOperationsManager so this is where we need to create objects that have a asyncOperationsContext
            'in them.
            If m_TurnOnNetworkListener = True And m_NetworkObject Is Nothing Then 'the is nothing check is to avoid hooking the object more than once
                m_NetworkObject = New Microsoft.VisualBasic.Devices.Network
                AddHandler m_NetworkObject.NetworkAvailabilityChanged, AddressOf Me.NetworkAvailableEventAdaptor
            End If
            RaiseEvent Startup(Me, eventArgs)
            Return Not eventArgs.Cancel
        End Function

        '''**************************************************************************
        ''' ;OnStartupNextInstance
        ''' <summary>
        ''' Extensibility point which raises the StartupNextInstance 
        ''' </summary>
        ''' <param name="eventArgs"></param>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        <SecuritySafeCritical()> _
        Protected Overridable Sub OnStartupNextInstance(ByVal eventArgs As StartupNextInstanceEventArgs)
            RaiseEvent StartupNextInstance(Me, eventArgs)
            'Activate the original instance
            Call New System.Security.Permissions.UIPermission(UIPermissionWindow.SafeSubWindows Or UIPermissionWindow.SafeTopLevelWindows).Assert()
            If eventArgs.BringToForeground = True AndAlso Me.MainForm IsNot Nothing Then
                If MainForm.WindowState = System.Windows.Forms.FormWindowState.Minimized Then
                    MainForm.WindowState = System.Windows.Forms.FormWindowState.Normal
                End If
                MainForm.Activate()
            End If
        End Sub

        '''**************************************************************************
        ''' ;OnRun
        ''' <summary>
        ''' At this point, the command line args should have been processed and the application will create the
        ''' main form and enter the message loop.  
        ''' </summary>
        ''' <remarks></remarks>
        <SecuritySafeCritical()> _
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Overridable Sub OnRun()
            If Me.MainForm Is Nothing Then
                OnCreateMainForm() 'A designer overrides OnCreateMainForm() to set the main form we are supposed to use
                If Me.MainForm Is Nothing Then Throw New NoStartupFormException

                'When we have a splash screen that hasn't timed out before the main form is ready to paint, we want to
                'block the main form from painting.  To do that I let the form get past the Load() event and hold it until
                'the splash screen goes down.  Then I let the main form continue it's startup sequence.  The ordering of
                'Form startup events for reference is: Ctor(), Load Event, Layout event, Shown event, Activated event, Paint event
                AddHandler Me.MainForm.Load, AddressOf MainFormLoadingDone
            End If

            'Run() eats all exceptions (unless running under the debugger) If the user wrote an UnhandledException handler we will hook 
            'the System.Windows.Forms.Application.ThreadException event (see Public Custom Event UnhandledException) which will raise our
            'UnhandledException Event.  If our user didn't write an UnhandledException event, then we land in the try/catch handler for Forms.Application.Run()
            Try
                System.Windows.Forms.Application.Run(m_AppContext)
            Finally
                'When Run() returns, the context we pushed in our ctor (which was a WindowsFormsSynchronizationContext) is restored.  But we are going to dispose it
                'so we need to disconnect the network listener so that it can't fire any events in response to changing network availability conditions through a dead context.  VSWHIDBEY #343374
                If m_NetworkObject IsNot Nothing Then m_NetworkObject.DisconnectListener()

                'The app is exiting - if another instance has launched, don't let it attach to this one.
                If m_FirstInstanceSemaphore IsNot Nothing Then
                    m_FirstInstanceSemaphore.Close()
                    m_FirstInstanceSemaphore = Nothing
                End If

                AsyncOperationManager.SynchronizationContext = m_AppSyncronizationContext 'Restore the prior sync context
                m_AppSyncronizationContext = Nothing
            End Try
        End Sub

        '''**************************************************************************
        ''' ;OnCreateSplashScreen
        ''' <summary>
        ''' A designer will override this method and provide a splash screen if this application has one.
        ''' </summary>
        ''' <remarks>For instance, a designer would override this method and emit: Me.Splash = new Splash
        ''' where Splash was designated in the application designer as being the splash screen for this app</remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Overridable Sub OnCreateSplashScreen()
        End Sub

        '''**************************************************************************
        ''' ;OnCreateMainForm
        ''' <summary>
        ''' Provides a hook that designers will override to set the main form.
        ''' </summary>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Overridable Sub OnCreateMainForm()
        End Sub

        '''**************************************************************************
        ''' ;OnShutdown
        ''' <summary>
        ''' The last in a series of extensibility points for the Shutdown process
        ''' </summary>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Overridable Sub OnShutdown()
            RaiseEvent Shutdown(Me, System.EventArgs.Empty)
        End Sub

        '''**************************************************************************
        ''' ;OnUnhandledException
        ''' <summary>
        ''' Raises the UnHandled exception event and exits the application if the event handler indicated
        ''' that execution shouldn't continue
        ''' </summary>
        ''' <param name="e"></param>
        ''' <returns>True indicates the exception event was raised / False it was not</returns>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Overridable Function OnUnhandledException(ByVal e As UnhandledExceptionEventArgs) As Boolean
            If m_UnhandledExceptionHandlers IsNot Nothing AndAlso m_UnhandledExceptionHandlers.Count > 0 Then 'Does the user have a handler for this event?
                'We don't put a try/catch around the handler event so that exceptions in there will bubble out - else we will have a recursive exception handler
                RaiseEvent UnhandledException(Me, e)
                If e.ExitApplication = True Then System.Windows.Forms.Application.Exit()
                Return True 'User handled the event
            End If
            Return False 'Nobody was listening to the UnhandledException event
        End Function

        '''**************************************************************************
        ''' ;ShowSplashScreen
        ''' <summary>
        ''' Uses the extensibility model to see if there is a splash screen provided for this app and if there is,
        ''' displays it.
        ''' </summary>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Protected Sub ShowSplashScreen()
            If Not m_DidSplashScreen Then
                m_DidSplashScreen = True
                If m_SplashScreen Is Nothing Then
                    OnCreateSplashScreen() 'If the user specified a splash screen, the designer will have overriden this method to set it
                End If
                If m_SplashScreen IsNot Nothing Then
                    'Some splash screens have minimum face time they are supposed to get.  We'll set up a time to let us know when we can take it down.
                    If m_MinimumSplashExposure > 0 Then
                        m_Ok2CloseSplashScreen = False 'Don't close until the timer expires.
                        m_SplashTimer = New System.Timers.Timer(m_MinimumSplashExposure)
                        AddHandler m_SplashTimer.Elapsed, AddressOf MinimumSplashExposureTimeIsUp
                        m_SplashTimer.AutoReset = False
                        'We'll enable it in DisplaySplash() once the splash screen thread gets running
                    Else
                        m_Ok2CloseSplashScreen = True 'No timeout so just close it when then main form comes up
                    End If
                    'Run the splash screen on another thread so we don't starve it for events and painting while the main form gets its act together
                    Dim SplashThread As New System.Threading.Thread(AddressOf DisplaySplash)
                    SplashThread.Start()
                End If
            End If
        End Sub

        '''**************************************************************************
        ''' ;HideSplashScreen
        ''' <summary>
        ''' Hide the splash screen.  The splash screen was created on another thread 
        ''' thread (main thread) than the one it was run on (secondary thread for the
        ''' splash screen so it doesn't block app startup. We need to invoke the close.
        ''' This function gets called from the main thread by the app fx.
        ''' </summary>
        ''' <remarks></remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        <SecuritySafeCritical()> _
        Protected Sub HideSplashScreen()
            SyncLock m_SplashLock 'This ultimately wasn't necessary.  I suppose we better keep it for backwards compat
                'Dev10 590587 - we now activate the main form before calling Dispose on the Splash screen. (we're just
                '       swapping the order of the two If blocks). This is to fix the issue where the main form 
                '       doesn't come to the front after the Splash screen disappears
                If Me.MainForm IsNot Nothing Then
                    Call New System.Security.Permissions.UIPermission(UIPermissionWindow.AllWindows).Assert()
                    Me.MainForm.Activate()
                    System.Security.PermissionSet.RevertAssert() 'CLR also reverts if we throw or when we return from this function 
                End If
                If m_SplashScreen IsNot Nothing AndAlso Not m_SplashScreen.IsDisposed Then
                    Dim TheBigGoodbye As New DisposeDelegate(AddressOf m_SplashScreen.Dispose)
                    m_SplashScreen.Invoke(TheBigGoodbye)
                    m_SplashScreen = Nothing
                End If
            End SyncLock
        End Sub

        '''**************************************************************************
        ''' ;ShutdownStyle
        ''' <summary>
        '''  Determines when this application will terminate (when the main form goes down, all forms)
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Protected Friend Property ShutdownStyle() As ShutdownMode
            Get
                Return m_ShutdownStyle
            End Get
            Set(ByVal value As ShutdownMode)
                ValidateShutdownModeEnumValue(value, "value")
                m_ShutdownStyle = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;EnableVisualStyles
        ''' <summary>
        ''' Determines whether this application will use the XP Windows styles for windows, controls, etc.
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Protected Property EnableVisualStyles() As Boolean
            Get
                Return m_EnableVisualStyles
            End Get
            Set(ByVal value As Boolean)
                m_EnableVisualStyles = value
            End Set
        End Property

        '''**************************************************************************
        ''' ;IsSingleInstance
        ''' <summary>
        ''' 
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Advanced)> Protected Property IsSingleInstance() As Boolean
            Get
                Return m_IsSingleInstance
            End Get
            Set(ByVal value As Boolean)
                m_IsSingleInstance = value
            End Set
        End Property

        '= Private ==========================================================

        '''**************************************************************************
        ''' ;ValidateAuthenticationModeEnumValue 
        ''' <summary>
        ''' Validates that the value being passed as an AuthenticationMode enum is a legal value
        ''' </summary>
        ''' <param name="value"></param>
        ''' <remarks></remarks>
        Private Sub ValidateAuthenticationModeEnumValue(ByVal value As AuthenticationMode, ByVal paramName As String)
            If value < ApplicationServices.AuthenticationMode.Windows OrElse value > ApplicationServices.AuthenticationMode.ApplicationDefined Then
                Throw New System.ComponentModel.InvalidEnumArgumentException(paramName, value, GetType(AuthenticationMode))
            End If
        End Sub

        '''**************************************************************************
        ''' ;ValidateShutdownModeEnumValue 
        ''' <summary>
        ''' Validates that the value being passed as an ShutdownMode enum is a legal value
        ''' </summary>
        ''' <param name="value"></param>
        ''' <remarks></remarks>
        Private Sub ValidateShutdownModeEnumValue(ByVal value As ShutdownMode, ByVal paramName As String)
            If value < ShutdownMode.AfterMainFormCloses OrElse value > ShutdownMode.AfterAllFormsClose Then
                Throw New System.ComponentModel.InvalidEnumArgumentException(paramName, value, GetType(ShutdownMode))
            End If
        End Sub

        '''**************************************************************************
        ''' ;DisplaySplash
        ''' <summary>
        ''' Displays the splash screen.  We get called here from a different thread than what the
        ''' main form is starting up on.  This allows us to process events for the Splash screen so
        ''' it doesn't freeze up while the main form is getting it together.
        ''' </summary>
        ''' <remarks></remarks>
        Private Sub DisplaySplash()
            Debug.Assert(m_SplashScreen IsNot Nothing, "We should have never get here if there is no splash screen")
            If m_SplashTimer IsNot Nothing Then 'We only have a timer if there is a minimum time that the splash screen is supposed to be displayed.
                m_SplashTimer.Enabled = True 'enable the timer now that we are about to show the splash screen
            End If
            System.Windows.Forms.Application.Run(m_SplashScreen)
        End Sub

        '''**************************************************************************
        ''' ;MinimumSplashExposureTimeIsUp
        ''' <summary>
        ''' If a splash screen has a minimum time out, then once that is up we check to see whether
        ''' we should close the splash screen.  If the main form has activated then we close it.
        ''' Note that we are getting called on a secondary thread here which isn't necessairly
        ''' associated with any form.  Don't touch forms from this function.
        ''' </summary>
        ''' <remarks></remarks>
        Private Sub MinimumSplashExposureTimeIsUp(ByVal sender As Object, ByVal e As System.Timers.ElapsedEventArgs)
            If m_SplashTimer IsNot Nothing Then 'We only have a timer if there was a minimum timeout on the splash screen
                m_SplashTimer.Dispose()
                m_SplashTimer = Nothing
            End If
            m_Ok2CloseSplashScreen = True
        End Sub

        '''**************************************************************************
        ''' ;MainFormLoadingDone
        ''' <summary>
        ''' The Load() event happens before the Shown and Paint events.  When we get called here
        ''' we know that the form load event is done and that the form is about to paint
        ''' itself for the first time.
        ''' We can now hide the splash screen.
        ''' Note that this function gets called from the main thread - the same thread
        ''' that creates the startup form.
        ''' </summary>
        ''' <param name="sender"></param>
        ''' <param name="e"></param>
        ''' <remarks></remarks>
        Private Sub MainFormLoadingDone(ByVal sender As Object, ByVal e As System.EventArgs)
            RemoveHandler Me.MainForm.Load, AddressOf MainFormLoadingDone 'We don't want this event to call us again.

            'block until the splash screen time is up.  See MinimumSplashExposureTimeIsUp() which releases us
            While Not m_Ok2CloseSplashScreen
                DoEvents() 'In case Load() event, which we are waiting for, is doing stuff that requires windows messages.  our timer message doesn't count because it is on another thread.
            End While

            HideSplashScreen()
        End Sub

        '''**************************************************************************
        ''' ;WinFormsAppContext 
        ''' <summary>
        ''' Encapsulates an ApplicationContext.  We have our own to get the shutdown behaviors we
        ''' offer in the application model.  This derivation of the ApplicationContext listens for when
        ''' the main form closes and provides for shutting down when the main form closes or the
        ''' last form closes, depending on the mode this application is running in.
        ''' </summary>
        ''' <remarks></remarks>
        Private Class WinFormsAppContext : Inherits System.Windows.Forms.ApplicationContext
            Sub New(ByVal App As WindowsFormsApplicationBase)
                m_App = App
            End Sub

            '''**************************************************************************
            ''' ;OnMainFormClosed
            ''' <summary>
            ''' Handles the two types of application shutdown: 
            '''   1 - shutdown when the main form closes
            '''   2 - shutdown only after the last form closes
            ''' </summary>
            ''' <param name="sender"></param>
            ''' <param name="e"></param>
            ''' <remarks></remarks>
            <SecuritySafeCritical()> _
            Protected Overrides Sub OnMainFormClosed(ByVal sender As Object, ByVal e As System.EventArgs)
                If m_App.ShutdownStyle = ShutdownMode.AfterMainFormCloses Then
                    MyBase.OnMainFormClosed(sender, e)
                Else 'identify a new main form so we can keep running
                    Call New System.Security.Permissions.UIPermission(UIPermissionWindow.AllWindows).Assert()
                    Dim forms As System.Windows.Forms.FormCollection = System.Windows.Forms.Application.OpenForms
                    System.Security.PermissionSet.RevertAssert() 'CLR also reverts if we throw or when we return from this function.
                    If forms.Count > 0 Then
                        'Note: Initially I used Process::MainWindowHandle to obtain an open form.  But that is bad for two reasons:
                        '1 - It appears to be broken and returns NULL sometimes even when there is still a window around.  WinForms people are looking at that issue.
                        '2 - It returns the first window it hits from enum thread windows, which is not necessarily a windows forms form, so that doesn't help us even if it did work
                        'all the time.  So I'll use one of our open forms.  We may not necessairily get a visible form here but that's ok.  Some apps may run on an invisible window
                        'and we need to keep them going until all windows close.
                        Me.MainForm = forms(0)
                    Else
                        MyBase.OnMainFormClosed(sender, e)
                    End If
                End If
            End Sub

            Private m_App As WindowsFormsApplicationBase
        End Class 'WinFormsAppContext

        '''**************************************************************************
        ''' ;OnUnhandledExceptionAdaptor
        ''' <summary>
        ''' Handles the Windows.Forms.Application.ThreadException event and raises our Unhandled
        ''' exception event
        ''' </summary>
        ''' <param name="e"></param>
        ''' <remarks>Our UnHandledException event has a different signature then the Windows.Forms.Application
        ''' unhandled exception event so we do the translation here before raising our event.
        ''' </remarks>
        Private Sub OnUnhandledExceptionEventAdaptor(ByVal sender As Object, ByVal e As Threading.ThreadExceptionEventArgs)
            OnUnhandledException(New Microsoft.VisualBasic.ApplicationServices.UnhandledExceptionEventArgs(True, e.Exception))
        End Sub

        '''**************************************************************************
        ''' ;OnStartupNextInstanceMarshallingAdaptor
        ''' <summary>
        '''  The call we get from the Async Operations manager has a different signature then what
        ''' we'd like to pass to our OnStartupNextInstanceMarshallingAdaptor.  So we do the translation
        ''' here and then call OnStartupNextInstance
        ''' </summary>
        ''' <param name="args"></param>
        ''' <remarks></remarks>
        Private Sub OnStartupNextInstanceMarshallingAdaptor(ByVal args As Object)
            OnStartupNextInstance(New StartupNextInstanceEventArgs(CType(args, System.Collections.ObjectModel.ReadOnlyCollection(Of String)), True)) 'by default, we set BringToFront as True since that's the behavior most people will want
        End Sub

        '''**************************************************************************
        ''' ;NetworkAvailableAdaptor
        ''' <summary>
        ''' Handles the Network.NetworkAvailability event (on the correct thread) and raises the
        ''' NetworkAvailabilityChanged event
        ''' </summary>
        ''' <param name="Sender">Contains the Network instance that raised the event</param>
        ''' <param name="e">Contains whether the network is available or not</param>
        ''' <remarks></remarks>
        Private Sub NetworkAvailableEventAdaptor(ByVal sender As Object, ByVal e As Microsoft.VisualBasic.Devices.NetworkAvailableEventArgs)
            RaiseEvent NetworkAvailabilityChanged(sender, e)
        End Sub

        Private Const HOST_NAME As String = "127.0.0.1" '127.0.0.1 is a loopback network connection to your own machine.  If you telnet, ftp, etc. to 127.0.0.1 you are connected to your own machine.  There's no place like 127.0.0.1 ;-)  Xp SP2 prevents connections to all IP addresses that are in the loopback address range except for 127.0.0.1.
        Private Const SECOND_INSTANCE_TIMEOUT As Integer = 2500 'milliseconds.  How long a subsequent instance will wait for the original instance to get on its feet.  This is only useful if the 1st instance went belly up and stranded the semaphore for some reason - we just wouldn't want to hang forever is all. 
        Private Const ATTACH_TIMEOUT As Integer = 2500 'millisconds.  How long we wait for the remoting infrastructure to acknowledge that it sent the remote call to the 1st instance.  A 1/10 of a second should be sufficient so 2.5 seconds is really allowing for a margin of error
        Private m_UnhandledExceptionHandlers As System.Collections.ArrayList
        Private m_ProcessingUnhandledExceptionEvent As Boolean
        Private m_TurnOnNetworkListener As Boolean 'Tracks whether we need to create the network object so we can listen to the NetworkAvailabilityChanged event
        Private m_FinishedOnInitilaize As Boolean 'Whether we have made it through the processing of OnInitialize 
        Private m_NetworkAvailabilityEventHandlers As System.Collections.ArrayList
        Private m_FirstInstanceSemaphore As System.Threading.EventWaitHandle 'Used to determine if we are the first instance or not and to prevent race conditions if we are launching subsequent instances.  Owned by the first instance to run
        Private m_MessageRecievedSemaphore As System.Threading.EventWaitHandle 'Used to let us know when the first instance has been contacted
        Private m_NetworkObject As Microsoft.VisualBasic.Devices.Network
        Private m_MemoryMappedID As String 'global OS handles must have a unique ID
        <SecurityCritical()> _
        Private m_FirstInstanceMemoryMappedFileHandle As SafeFileHandle 'Used to communicate the URL of the single instance between processes.  We hang on to it for the life of the process so that it is available to subsequent instances.  It's a SafeHandle so it'll be sure to release the handle when the app exits.
        Private m_IsSingleInstance As Boolean 'whether this app runs using Word like instancing behavior
        Private m_ShutdownStyle As ShutdownMode 'defines when the application decides to close
        Private m_EnableVisualStyles As Boolean 'whether to use Windows XP styles
        Private m_DidSplashScreen As Boolean 'we only need to show the splash screen once.  Protect the user from himself if they are overriding our app model.
        Private Delegate Sub DisposeDelegate() 'used to marshal a call to Dispose on the Splash Screen
        Private m_Ok2CloseSplashScreen As Boolean 'For splash screens with a minimum display time, this let's us know when that time has expired and it is ok to close the splash screen.
        Private m_SplashScreen As System.Windows.Forms.Form
        Private m_MinimumSplashExposure As Integer = 2000 'Minimum amount of time to show the splash screen.  0 means hide as soon as the app comes up.
        Private m_SplashTimer As Timers.Timer
        Private m_SplashLock As New Object
        Private m_AppContext As WinFormsAppContext
        Private m_AppSyncronizationContext As SynchronizationContext
        Private m_NetworkAvailChangeLock As New Object 'sync object
        Private m_SaveMySettingsOnExit As Boolean 'Informs My.Settings whether to save the settings on exit or not
        Private m_StartNextInstanceCallback As Threading.SendOrPostCallback 'Used for marshalling the start next instance event to the foreground thread

        'REMOTING SUPPORT ---------

        '''**************************************************************************
        ''' ;RunNextInstanceDelegate
        ''' <summary>
        ''' Provides the delegate to the entry point for subseqent instances of a single instance
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Private ReadOnly Property RunNextInstanceDelegate() As System.Threading.SendOrPostCallback
            Get
                Return m_StartNextInstanceCallback
            End Get
        End Property

        '''**************************************************************************
        ''' ;ReadUrlFromMemoryMappedFile
        ''' <summary>
        ''' For single instance applications, when subsequent instances start up they
        ''' need to get to the URL of the remoting object that will connect them to the
        ''' original instance.  I store the URL of the remote object in a memory-mapped
        ''' file which this function reads the URL from.
        ''' </summary>
        ''' <returns></returns>
        ''' <remarks></remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
        Private Function ReadUrlFromMemoryMappedFile() As String

            Debug.Assert(m_MemoryMappedID IsNot Nothing, "You can't call this function unless you've first written to the memory mapped file")

            Const FILE_MAP_READ As Integer = &H4
            Dim URL As String

            'OPEN THE EXISTING MEMORY MAPPED FILE THAT HAS THE URL STOWED IN IT.  WE NEED IT TO CONTACT THE ORIGINAL INSTANCE
            'This gets tricky as far as managing the OS handles goes. We'll have another handle out on our memory mapped file and we are about to
            'get a handle for the mapped view of it.  We need both closed by the time we leave this method so we don't leak handles and don't leak
            'the memory that we map into this process from the memory mapped file.  I'm using SafeHandle classes under the covers to assure that we'll
            'release everything even in the face of abject calamity.
            Using MemoryMappedFileHandle As SafeFileHandle = UnsafeNativeMethods.OpenFileMapping(FILE_MAP_READ, False, m_MemoryMappedID)
                If MemoryMappedFileHandle.IsInvalid Then
                    Return Nothing
                End If

                'READ THE URL OUT OF THE MEMORY MAPPED FILE
                'Note, the fact that StartAddressOfMappedView is in a using block gaurantees that it will be kept alive past the
                'time of the call to MapViewOfFile - so we don't need to worry about a race condition with the finalizer in
                'StartAddressOfMappedView releasing the handle during the time we need access to it.
                Using StartAddressOfMappedView As SafeMemoryMappedViewOfFileHandle = _
                   UnsafeNativeMethods.MapViewOfFile(MemoryMappedFileHandle.DangerousGetHandle, FILE_MAP_READ, 0, 0, UIntPtr.Zero)

                    If StartAddressOfMappedView.IsInvalid Then
                        'This exception does a get last error and generates the appropriate message for it
                        Throw ExceptionUtils.GetWin32Exception(ResID.MyID.AppModel_CantGetMemoryMappedFile)
                    End If

                    URL = System.Runtime.InteropServices.Marshal.PtrToStringUni(StartAddressOfMappedView.DangerousGetHandle)
                End Using 'Releases StartAddressOfMappedView which Unmaps the memory mapped file view
            End Using 'Releases MemoryMappedFileHandle

            Return URL
        End Function

        '''**************************************************************************
        ''' ;WriteUrlToMemoryMappedFile
        ''' <summary>
        ''' For single instance applications, when subsequent instances start up they
        ''' need to get to the URL of the remoting object that will connect them to the
        ''' original instance.  I store the URL of the remote object in a memory-mapped
        ''' file which subsequent instances can read from.
        ''' </summary>
        ''' <param name="URL"></param>
        ''' <remarks></remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <ResourceConsumption(ResourceScope.Machine, ResourceScope.Machine)> _
        Private Sub WriteUrlToMemoryMappedFile(ByVal URL As String)

            Debug.Assert(m_FirstInstanceMemoryMappedFileHandle Is Nothing, "m_MemoryMappedFileHandle shouldn't be set already")

            Const SDDL_REVISION_1 As Integer = 1 'see sddl.h for info on SDDL_REVISION_1 value  These days it is 1.
            Const PAGE_READWRITE As Integer = &H4
            Const FILE_MAP_WRITE As Integer = &H2
            Const SYSTEM_PAGING_FILE_ID As Integer = &HFFFFFFFF

            'This doesn't need to be a SafeHandle--it's just a constant that represents the Paging file.  It's not a handle to an actual OS resource
            'But it does need to be a HandleRef so it doesn't get collected during the PINVOKE calls
            Dim PAGE_FILE_HANDLE As New System.Runtime.InteropServices.HandleRef(Nothing, New IntPtr(SYSTEM_PAGING_FILE_ID))

            Using SecurityAttributes As New NativeTypes.SECURITY_ATTRIBUTES
                SecurityAttributes.bInheritHandle = False 'no inheritance of permissions desired for the memory mapped file handle
                'see p. 186-187 in writing secure code vol 2 on the SDDL string
                'see winerror.h for system.runtime.interop.marshal.getlastwin32error() meanings if !ok
                'see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/secauthz/security/ace_strings.asp for how to build the SDL string.
                Dim SecurityDescriptorIsOK As Boolean
                Try
                    Const RIGHTS As String = "D:(A;;GA;;;CO)(A;;GR;;;AU)"
                    'D: Means we are defining a DACL (discretionary access control list)
                    '(A;;GA;;;CO) means A=Allow GA=general access to CO=creater owner (that's us)
                    '(A;;GR;;;AU) means A=Allow GR=read rights to AU=authenticated users
                    'The main point here is to keep people from scribbing on the contents of the memory mapped file.  We have other mitigations
                    'if they read the memory mapped file because they have to have the same identity as the creator of the m.m. file in order to
                    'make use of it in our remoting mechanism.
                    'SECURITY REVIEW:  - I'm new to the security description string.  Make sure this is ok.
                    'NOTE: This method allocates memory that SecurityAttributes.lpSecurityDescriptor points to.  Fortunately, SecurityAttributes frees the memory for us in its Dispose()

                    Call New SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert()
                    SecurityDescriptorIsOK = NativeMethods.ConvertStringSecurityDescriptorToSecurityDescriptor(RIGHTS, SDDL_REVISION_1, SecurityAttributes.lpSecurityDescriptor, IntPtr.Zero)
                    System.Security.CodeAccessPermission.RevertAssert()

                    'The SDDL language isn't supported on platforms < win2000.  ACLS aren't supported on win 95/98/me/CE so this really only affects NT4
                Catch ex As System.EntryPointNotFoundException
                    SecurityAttributes.lpSecurityDescriptor = IntPtr.Zero 'use the default ACLS the user
                Catch ex As System.DllNotFoundException
                    SecurityAttributes.lpSecurityDescriptor = IntPtr.Zero 'use the default ACLS the user
                End Try

                If Not SecurityDescriptorIsOK Then
                    SecurityAttributes.lpSecurityDescriptor = IntPtr.Zero 'use the default ACLS of the user
                End If

                'This is a SafeFileHandle so we won't leak the handle in the event of an async exception.  It lives for the duration of this process &
                'is released during the finally block for the app.  In the event of an async exception that prevents the app's finally from running, it 
                'gets released during critical finalization as it is a SafeHandle.
                m_FirstInstanceMemoryMappedFileHandle = UnsafeNativeMethods.CreateFileMapping( _
                            PAGE_FILE_HANDLE, SecurityAttributes, PAGE_READWRITE, 0, (URL.Length + 1) * 2, m_MemoryMappedID)

                If m_FirstInstanceMemoryMappedFileHandle.IsInvalid Then
                    Throw ExceptionUtils.GetWin32Exception(ResID.MyID.AppModel_CantGetMemoryMappedFile)
                End If
            End Using 'SecurityAttributes - we are agressive about disposing this as it hangs on to unmanaged memory

            'I'm using a SafeHandle below so that we are guaranteed that the view will get unmapped even in the event of a calamity.
            'In the single-process case this isn't that interesting since the OS will reclaim everything anyway when the process dies.
            'But if we were running in an AppDomain that is getting created/destroyed repeatedly then we'd have 
            'an ongoing memory leak since app domain unloads don't reclaim lost OS handles and unmanaged memory since the process
            'is still hanging around.
            Using StartAddressOfMappedFile As SafeMemoryMappedViewOfFileHandle = UnsafeNativeMethods.MapViewOfFile( _
                m_FirstInstanceMemoryMappedFileHandle.DangerousGetHandle, FILE_MAP_WRITE, 0, 0, UIntPtr.Zero)

                If StartAddressOfMappedFile.IsInvalid Then
                    Throw ExceptionUtils.GetWin32Exception(ResID.MyID.AppModel_CantGetMemoryMappedFile)
                End If

                Dim UrlChars() As Char = URL.ToCharArray()
                System.Runtime.InteropServices.Marshal.Copy(UrlChars, 0, StartAddressOfMappedFile.DangerousGetHandle, UrlChars.Length)
            End Using 'Releases StartAddressOfMappedFile which unmaps the memory for the view.  
            'Note that though we've unmapped the view, because we are hanging on to the file handle in m_FirstInstanceMemoryMappedFileHandle
            'we won't lose our memory mapped file until this instance goes down.  Which is what we want because subsequent instances that launch will need
            'the memory mapped file so they can read the URL we just wrote.  When the original instance (this one) goes down then we release the memory mapped file.
        End Sub

        '''**************************************************************************
        ''' ;RegisterChannel
        ''' <summary>
        ''' Provides an authenticated (via windows impersonation) channel for remoting purposes
        ''' This channel is tied to LOCALHOST
        ''' </summary>
        ''' <returns>A channel that can be used as either a server or client channel</returns>
        ''' <param name="ChannelType">First instance will create a server channel, subsequent instances will create a client channel. VSWhidbey 563668.</param>
        ''' <param name="ChannelIsSecure">Whether we can use authentication on the channel or not</param>
        ''' <remarks>Attempts to connect to a port opened on localhost from another machine
        ''' will fail - which helps us secure this port.  But a normal user in a terminal-server session 
        ''' can see the port and talk to it.  Therefore we made an authenticated channel so that we
        ''' can know who is making the request over the channel</remarks>
        <SecurityCritical()> _
        Private Function RegisterChannel(ByVal ChannelType As ChannelType, ByVal ChannelIsSecure As Boolean) As IChannel
            'To run single instance in low-trust (e.g. InternetZone), we need the following permissions (differs from connecting - don't need DNS permission.  See where we do this in Run())
            Dim PermissionsToDoRemoting As New System.Security.PermissionSet(PermissionState.None)
            PermissionsToDoRemoting.AddPermission(New System.Security.Permissions.SecurityPermission(SecurityPermissionFlag.SerializationFormatter Or SecurityPermissionFlag.ControlPrincipal Or SecurityPermissionFlag.UnmanagedCode))
            PermissionsToDoRemoting.AddPermission(New System.Net.SocketPermission(Net.NetworkAccess.Accept, Net.TransportType.Tcp, HOST_NAME, 0))
            PermissionsToDoRemoting.AddPermission(New System.Security.Permissions.EnvironmentPermission(EnvironmentPermissionAccess.Read, "USERNAME"))
            PermissionsToDoRemoting.AddPermission(New System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.RemotingConfiguration))
            PermissionsToDoRemoting.Assert()

            ' See http://msdn2.microsoft.com/en-us/library/kw7c6kwc.aspx for channel properties.
            Dim ChannelProperties As System.Collections.IDictionary = New System.Collections.Hashtable(3)
            ChannelProperties.Add("bindTo", HOST_NAME)
            ChannelProperties.Add("port", 0) 'Listen on any open port.  
            ChannelProperties.Add("name", String.Empty) ' ignore names but avoid naming collisions, this will avoid an exception when the application registers a default channel by itself VSWhidbey 563668.
            If ChannelIsSecure = True Then 'Only >= NT support authentication on the server
                ChannelProperties.Add("secure", True) 'we use authentication to keep people from talking to our port from other terminal server instances
                ChannelProperties.Add("tokenimpersonationlevel", System.Security.Principal.TokenImpersonationLevel.Impersonation)
                ChannelProperties.Add("impersonate", True)
            End If
            Dim AuthenticatingChannel As IChannel = Nothing
            ' Create the specific TCP channel. This helps when the application register a generic channel / client channel to do remoting itself.
            ' The remoting framework will not pick our server channel to use to talk to the customer's server. VSWhidbey 563668.
            If ChannelType = ChannelType.Server Then
                AuthenticatingChannel = New TcpServerChannel(ChannelProperties, Nothing)
            Else
                AuthenticatingChannel = New TcpClientChannel(ChannelProperties, Nothing)
            End If
            ChannelServices.RegisterChannel(AuthenticatingChannel, ChannelIsSecure)

            System.Security.PermissionSet.RevertAssert()
            Return AuthenticatingChannel
        End Function

        '''**************************************************************************
        ''' ;RemoteCommunicator 
        ''' <summary>
        ''' This class is used in the Single-Instance application scenario.  It marshals a cross process
        ''' (using remoting) from a subsequent instance of an application to the original instance and
        ''' instigates the StartupNextInstance() event being fired on the original instance.
        ''' </summary>
        ''' <remarks></remarks>
        Private Class RemoteCommunicator : Inherits System.MarshalByRefObject

            '''**************************************************************************
            ''' ;New 
            ''' <summary>
            ''' Constructs a new RemoteCommunicator
            ''' </summary>
            ''' <remarks>Internal class used to encapsulate marshaling the single instance app event
            ''' from a subsequent process to the original process
            ''' </remarks>
            <SecurityCritical()> _
            Friend Sub New(ByVal appObject As WindowsFormsApplicationBase, ByVal ConnectionMadeSemaphore As System.Threading.EventWaitHandle)
                Call New SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.ControlPrincipal).Assert() 'I need to get the current identity so I can do my ACLS
                m_OriginalUser = System.Security.Principal.WindowsIdentity.GetCurrent
                System.Security.CodeAccessPermission.RevertAssert()

                m_AsyncOp = AsyncOperationManager.CreateOperation(Nothing) 'We need to hang on to the syncronization context associated with the thread the network object is created on
                m_StartNextInstanceDelegate = appObject.RunNextInstanceDelegate
                m_ConnectionMadeSemaphore = ConnectionMadeSemaphore
            End Sub

            '''**************************************************************************
            ''' ;RunNextInstance
            ''' <summary>
            ''' Makes sure that the right person is trying to start up a subsequent instance and marshal
            ''' the call to the original instance.
            ''' </summary>
            ''' <param name="Args"></param>
            ''' <remarks>
            ''' In systems before NT, there is no security.  Attempts to get the current user
            ''' will return "" so verification 'succeeds' on those earlier systems.
            ''' </remarks>
            <SecuritySafeCritical()> _
            <System.Runtime.Remoting.Messaging.OneWay()> _
            Public Sub RunNextInstance(ByVal Args As System.Collections.ObjectModel.ReadOnlyCollection(Of String))
                'Prevent the vulnerability in which a normal user could terminal server into a server box and invoke (if they figured out the port and our objects)
                'our 2nd instance mechanism, passing whatever command line args they want to a running instance.  We mitigate that threat by using a channel
                'that(uses) impersonation for authentication purposes.  To prevent somebody else from talking to our remote object, make sure that our caller
                'is the same user that the 1st instance launched under
                Call New SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.ControlPrincipal).Assert() 'I need to get the current identity so I can do my ACLS
                If m_OriginalUser.User <> System.Security.Principal.WindowsIdentity.GetCurrent.User Then
                    Return 'don't tell them what happened - just don't connect.  Somebody is trying to spoof us.  CLR clears security assert as we return so no revert needed.
                End If
                m_ConnectionMadeSemaphore.Set()
                System.Security.CodeAccessPermission.RevertAssert()
                m_AsyncOp.Post(m_StartNextInstanceDelegate, Args) 'Marshal the call over to the original instance
            End Sub

            '''**************************************************************************
            ''' ;InitializeLifetimeService
            ''' <summary>
            ''' Set our lease so that it doesn't expire.  The remote object will be destroyed when the application goes down
            ''' </summary>
            ''' <returns></returns>
            ''' <remarks></remarks>
            <SecurityCritical()> _
            Public Overrides Function InitializeLifetimeService() As Object
                Return Nothing
            End Function

            Private m_StartNextInstanceDelegate As Threading.SendOrPostCallback
            Private m_AsyncOp As System.ComponentModel.AsyncOperation 'Used for marshalling the start next instance call to the foreground thread
            Private m_OriginalUser As System.Security.Principal.WindowsIdentity 'Track who started the first instance so we can identify spoofers trying to call in on our remote object
            Private m_ConnectionMadeSemaphore As System.Threading.EventWaitHandle 'Note that we don't manage the lifetime of the ConnectionMadeSemaphore handle so don't try to close it in this class.
        End Class 'RemoteCommunicator

        '''**************************************************************************
        ''' ;DoApplicationModel
        ''' <summary>
        ''' Runs the user's program through the VB Startup/Shutdown application model 
        ''' </summary>
        ''' <remarks></remarks>
        Private Sub DoApplicationModel()

            Dim EventArgs As New StartupEventArgs(MyBase.CommandLineArgs)

            'Only do the try/catch if we aren't running under the debugger.  If we do try/catch under the debugger the debugger never gets a crack at exceptions which breaks the exception helper
            If Not System.Diagnostics.Debugger.IsAttached Then
                'NO DEBUGGER ATTACHED - we use a catch so that we can run our UnhandledException code
                'Note - Sadly, code changes within this IF (that don't pertain to exception handling) need to be mirrored in the ELSE debugger attached clause below
                Try
                    If OnInitialize(MyBase.CommandLineArgs) Then
                        If OnStartup(EventArgs) = True Then
                            OnRun()
                            OnShutdown()
                        End If
                    End If
                Catch ex As System.Exception
                    'This catch is for exceptions that happen during the On* methods above, but have occurred outside of the message pump (which exceptions we would
                    'have already seen via our hook of System.Windows.Forms.Application.ThreadException)
                    If m_ProcessingUnhandledExceptionEvent Then
                        Throw 'If the UnhandledException handler threw for some reason, throw that error out to the system.
                    Else 'We had an exception, but not during the OnUnhandledException handler so give the user a chance to look at what happened in the UnhandledException event handler
                        If Not OnUnhandledException(New UnhandledExceptionEventArgs(True, ex)) = True Then
                            Throw 'the user didn't write a handler so throw the error out to the system
                        End If
                    End If
                End Try
            Else 'DEBUGGER ATTACHED - we don't have an uber catch when debugging so the exception will bubble out to the exception helper 
                'We also don't hook up the Application.ThreadException event because WinForms ignores it when we are running under the debugger
                If OnInitialize(MyBase.CommandLineArgs) Then
                    If OnStartup(EventArgs) = True Then
                        OnRun()
                        OnShutdown()
                    End If
                End If
            End If
        End Sub

        '''**************************************************************************
        ''' ;GetApplicationInstanceID
        ''' <summary>
        ''' Generates the name for the remote singleton that we use to channel multiple instances
        ''' to the same application model thread.
        ''' </summary>
        ''' <returns></returns>
        ''' <remarks></remarks>
        <SecurityCritical()> _
        Private Function GetApplicationInstanceID(ByVal Entry As Assembly) As String
            'CONSIDER: We may want to make this public so users can set up what single instance means to them, e.g. for us, seperate paths mean different instances, etc.

            Dim Permissions As New System.Security.PermissionSet(PermissionState.None)
            Permissions.AddPermission(New FileIOPermission(PermissionState.Unrestricted)) 'Chicken and egg problem.  All I need is PathDiscovery for the location of this assembly but to get the location of the assembly (see Getname below) I need to know the path which I can't get without asserting...  
            Permissions.AddPermission(New SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode))
            Permissions.Assert()

            Dim Guid As System.Guid = System.Runtime.InteropServices.Marshal.GetTypeLibGuidForAssembly(Entry)
            Dim Version As String = Entry.GetName.Version.ToString
            Dim VersionParts As String() = Version.Split(CType(".", Char()))
            System.Security.PermissionSet.RevertAssert()

            'Note: We used to make the terminal server session ID part of the key.  It turns out to be unnecessary and the call to 
            'NativeMethods.ProcessIdToSessionId(System.Diagnostics.Process.GetCurrentProcess.Id, TerminalSessionID) was not supported on Win98, anyway.
            'It turns out that terminal server sessions, even when you are logged in as the same user to multiple terminal server sessions on the same
            'machine, are separate.  So you can have session 1 running as  and have a global system object named "FOO" that won't conflict with
            'any other global system object named "FOO" whether it be in session 2 running as  or session n running as whoever.
            'So it isn't necessary to make the session id part of the unique name that identifies a 

            Return Guid.ToString + VersionParts(0) + "." + VersionParts(1)  'Re: version parts, we have the major, minor, build, revision.  We key off major+minor.
        End Function

        ''' ;ChannelType
        ''' <summary>
        ''' Used in RegisterChannel method. Generate either TcpServerChannel or TcpClientChannel
        ''' </summary>
        Private Enum ChannelType As Byte
            Client = 1
            Server = 0
        End Enum
    End Class 'WindowsFormsApplicationBase
End Namespace

