' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict On

Imports System
Imports System.Drawing
Imports System.Globalization
Imports System.Security
Imports System.Threading
Imports System.Windows.Forms

Namespace Microsoft.VisualBasic.MyServices.Internal

    '''*************************************************************************
    ''';ProgressDialog
    ''' <summary>
    '''  A dialog that shows progress used for Network.Download and Network.Upload
    ''' </summary>
    ''' <remarks></remarks>
    Friend Class ProgressDialog
        Inherits System.Windows.Forms.Form

        '==PUBLIC**************************************************************

        '**********************************************************************
        ''';UserCancelledEvent
        ''' <summary>
        ''' Event raised when user cancels the dialog or closes it before the operation is completed
        ''' </summary>
        ''' <remarks></remarks>
        Public Event UserHitCancel()

        '''*********************************************************************
        ''';New
        ''' <summary>
        '''  Constructor
        ''' </summary>
        ''' <remarks></remarks>
        Friend Sub New()
            MyBase.New()
            InitializeComponent()
        End Sub

        '''*********************************************************************
        ''';Increment
        ''' <summary>
        '''  Increments the progress bar by the passed in amount
        ''' </summary>
        ''' <param name="incrementAmount">The amount to increment the bar</param>
        ''' <remarks>
        '''  This method should never be called directly. It should be called with
        '''  an InvokeBegin by a secondary thread.
        '''</remarks>
        Public Sub Increment(ByVal incrementAmount As Integer)
            Me.ProgressBarWork.Increment(incrementAmount)
        End Sub

        '''*********************************************************************
        ''';CloseDialog
        ''' <summary>
        '''  Closes the Progress Dialog
        ''' </summary>
        ''' <remarks>
        '''  This method should never be called directly. It should be called with
        '''  an InvokeBegin by a secondary thread.
        '''</remarks>
        Public Sub CloseDialog()
            m_CloseDialogInvoked = True
            Me.Close()
        End Sub

        '''*********************************************************************
        ''';ShowProgressDialog
        ''' <summary>
        '''  Displays the progress dialog modally
        ''' </summary>
        ''' <remarks>This method should be called on the main thread after the worker thread has been started</remarks>
        Public Sub ShowProgressDialog()
            Try
                If Not m_Closing Then
                    Me.ShowDialog()
                End If
            Finally
                FormClosableSemaphore.Set()
            End Try
        End Sub

        '''**********************************************************************
        ''';LabelText
        ''' <summary>
        '''  Sets the text of the label (Usually something like Copying x to y)
        ''' </summary>
        ''' <value>The value to set the label to</value>
        ''' <remarks>This should only be called on the main thread before showing the dialog</remarks>
        Public Property LabelText() As String
            Get
                Return Me.LabelInfo.Text
            End Get
            Set(ByVal Value As String)
                Me.LabelInfo.Text = Value
            End Set
        End Property

        '''**********************************************************************
        ''';FormClosableSemaphore
        ''' <summary>
        ''' Used to set or get the semaphore which signals when the dialog
        ''' is in a closable state.
        ''' </summary>
        ''' <value>The ManualResetEvent</value>
        ''' <remarks></remarks>
        Public ReadOnly Property FormClosableSemaphore() As ManualResetEvent
            Get
                Return m_FormClosableSemaphore
            End Get
        End Property

        '''**********************************************************************
        ''';IndicateClosing
        ''' <summary>
        '''  Inform the diaog that CloseDialog will soon be called
        ''' </summary>
        ''' <remarks>
        '''  This method should be called directly from the secondary thread. We want
        '''  to indicate we're closing as soon as we can so w don't show the dialog when we
        '''  don't need to (when the work is finished before we can show the dialog)
        '''</remarks>
        Public Sub IndicateClosing()
            m_Closing = True
        End Sub

        '''**********************************************************************
        ''';UserCancelled
        ''' <summary>
        '''  Indicated if the user has clicked the cancel button
        ''' </summary>
        ''' <value>True if the user has cancelled, otherwise False</value>
        ''' <remarks>
        '''  The secondary thread checks this property directly. If it's True, the thread
        '''  breaks out of its loop.
        '''</remarks>
        Public ReadOnly Property UserCanceledTheDialog() As Boolean
            Get
                Return m_Canceled
            End Get
        End Property

        '==PROTECTED************************************************************

        '''*********************************************************************
        ''';CreateParams
        ''' <summary>
        ''' This enables a dialog with a close button, sizable borders, and no icon
        ''' </summary>
        ''' <value></value>
        ''' <remarks></remarks>
        Protected Overrides ReadOnly Property CreateParams() As CreateParams
            <SecuritySafeCritical()> _
            Get
                Dim cp As CreateParams = MyBase.CreateParams
                cp.Style = cp.Style Or WS_THICKFRAME
                Return cp
            End Get
        End Property

        '==PRIVATE**************************************************************

        '''*********************************************************************
        ''';ButtonCloseDialog_Click
        ''' <summary>
        '''  Handles user clicking Cancel. Sets a flag read by secondary thread.
        ''' </summary>
        ''' <param name="sender">The cancel button</param>
        ''' <param name="e">Arguments</param>
        ''' <remarks></remarks>
        Private Sub ButtonCloseDialog_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ButtonCloseDialog.Click
            Me.ButtonCloseDialog.Enabled = False
            m_Canceled = True
            RaiseEvent UserHitCancel()
        End Sub

        '''*********************************************************************
        ''';ProgressDialog_FormClosing
        ''' <summary>
        ''' Indicates the form is closing
        ''' </summary>
        ''' <param name="sender"></param>
        ''' <param name="e"></param>
        ''' <remarks>
        ''' We listen for this event since we want to make closing the dialog before it's
        ''' finished behave the same as a cancel
        '''</remarks>
        Private Sub ProgressDialog_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing
            If e.CloseReason = CloseReason.UserClosing And Not m_CloseDialogInvoked Then
                ' If the progress bar isn't finished and the user hasn't already cancelled
                If Me.ProgressBarWork.Value < 100 And Not m_Canceled Then
                    ' Cancel the Close since we want the dialog to be closed from a call from the 
                    ' secondary thread
                    e.Cancel = True

                    ' Indicate the user has cancelled. We'll actually close the dialog from WebClientCopy
                    m_Canceled = True
                    RaiseEvent UserHitCancel()
                End If
            End If
        End Sub

        '''*********************************************************************
        ''';ProgressDialog_Resize
        ''' <summary>
        ''' Ensure the label resizes with the dialog
        ''' </summary>
        ''' <param name="sender"></param>
        ''' <param name="e"></param>
        ''' <remarks>
        ''' Since the label has AutoSize set to True we have to set the maximum size so the label
        ''' will grow down rather than off the dialog. As the size of the dialog changes, the maximum
        ''' size needs to be adjusted.
        ''' </remarks>
        Private Sub ProgressDialog_Resize(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Resize
            Me.LabelInfo.MaximumSize = New Size(Me.ClientSize.Width - BORDER_SIZE, 0)
        End Sub

        '''*********************************************************************
        ''';ProgressDialog_Activated
        ''' <summary>
        '''  Exits the monitor when we're activated
        ''' </summary>
        ''' <param name="sender">Dialog</param>
        ''' <param name="e">Arguments</param>
        ''' <remarks></remarks>
        Private Sub ProgressDialog_Activated(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Shown
            m_FormClosableSemaphore.Set()
        End Sub

        ' Indicates whether or not the dialog is closing
        Private m_Closing As Boolean

        ' Indicates whether or not the user has cancelled the copy
        Private m_Canceled As Boolean = False

        ' Used to signal when the dialog is in a closable state. The dialog is in a closable
        ' state when it has been activated or when it has been flagged to be closed before 
        ' ShowDialog has been called
        Private m_FormClosableSemaphore As ManualResetEvent = New ManualResetEvent(False)

        ' Indicates CloseDialog has been called
        Private m_CloseDialogInvoked As Boolean

        ' Constant used to get resizable dialog with border style set to fixed dialog.
        Private Const WS_THICKFRAME As Integer = &H40000

        ' Border area for label (10 pixels on each side)
        Private Const BORDER_SIZE As Integer = 20


#Region " Windows Form Designer generated code "

        'Form overrides dispose to clean up the component list.
        Protected Overloads Overrides Sub Dispose(ByVal disposing As Boolean)
            If disposing Then
                If Not (components Is Nothing) Then
                    components.Dispose()
                End If
                If m_FormClosableSemaphore isNot Nothing then
                    m_FormClosableSemaphore.Dispose()
                    m_FormClosableSemaphore = Nothing
                End If
            End If
            MyBase.Dispose(disposing)
        End Sub
        Friend WithEvents LabelInfo As System.Windows.Forms.Label
        Friend WithEvents ProgressBarWork As System.Windows.Forms.ProgressBar
        Friend WithEvents ButtonCloseDialog As System.Windows.Forms.Button

        'Required by the Windows Form Designer
        Private components As System.ComponentModel.IContainer

        'NOTE: The following procedure is required by the Windows Form Designer
        'It can be modified using the Windows Form Designer.  
        'Do not modify it using the code editor.
        <System.Diagnostics.DebuggerStepThrough()> _
        Private Sub InitializeComponent()
            Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(ProgressDialog))
            Me.LabelInfo = New System.Windows.Forms.Label
            Me.ProgressBarWork = New System.Windows.Forms.ProgressBar
            Me.ButtonCloseDialog = New System.Windows.Forms.Button
            Me.SuspendLayout()
            '
            'LabelInfo
            '
            resources.ApplyResources(Me.LabelInfo, "LabelInfo", CultureInfo.CurrentUICulture)
            Me.LabelInfo.MaximumSize = New System.Drawing.Size(300, 0)
            Me.LabelInfo.Name = "LabelInfo"
            '
            'ProgressBarWork
            '
            resources.ApplyResources(Me.ProgressBarWork, "ProgressBarWork", CultureInfo.CurrentUICulture)
            Me.ProgressBarWork.Name = "ProgressBarWork"
            '
            'ButtonCloseDialog
            '
            resources.ApplyResources(Me.ButtonCloseDialog, "ButtonCloseDialog", CultureInfo.CurrentUICulture)
            Me.ButtonCloseDialog.Name = "ButtonCloseDialog"
            '
            'ProgressDialog
            '
            resources.ApplyResources(Me, "$this", CultureInfo.CurrentUICulture)
            Me.Controls.Add(Me.ButtonCloseDialog)
            Me.Controls.Add(Me.ProgressBarWork)
            Me.Controls.Add(Me.LabelInfo)
            Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
            Me.MaximizeBox = False
            Me.MinimizeBox = False
            Me.Name = "ProgressDialog"
            Me.ShowInTaskbar = False
            Me.ResumeLayout(False)
            Me.PerformLayout()

        End Sub

#End Region

    End Class
End Namespace
