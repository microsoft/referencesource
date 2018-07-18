' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict On

Imports System
Imports System.Collections
Imports System.Windows.Forms
Imports System.IO
Imports System.Security
Imports System.Security.Permissions
Imports System.Diagnostics

Imports Microsoft.VisualBasic
Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils

Namespace Microsoft.VisualBasic

    '****************************************************************************
    ''';AudioPlayMode
    ''' <summary>
    '''  Enum for three ways to play a .wav file
    ''' </summary>
    ''' <remarks></remarks>
    Public Enum AudioPlayMode
        '!!!!!!!!!!!!!!!! Any changes to this enum must be reflected in ValidateAudioPlayModeEnum()
        WaitToComplete = 0 'Synchronous
        Background = 1     'Asynchronous
        BackgroundLoop = 2 'Asynchronous and looping
    End Enum

    Namespace Devices

        '''**************************************************************************
        ''';Audio
        ''' <summary>
        '''  An object that makes it easy to play wav files
        ''' </summary>
        ''' <remarks></remarks>
        <HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
        Public Class Audio

            '* PUBLIC **************************************************************

            '''**********************************************************************
            ''';New
            ''' <summary>
            '''  Creates a new Audio object
            ''' </summary>
            ''' <remarks></remarks>
            Public Sub New()
            End Sub

            '''**********************************************************************
            ''';Play
            ''' <summary>
            '''  Plays a .wav file in background mode
            ''' </summary>
            ''' <param name="location">The name of the file</param>
            ''' <remarks></remarks>
            Public Sub Play(ByVal location As String)
                Play(location, AudioPlayMode.Background)
            End Sub

            '''**********************************************************************
            ''';Play
            ''' <summary>
            ''' Plays a .wav file in the passed in mode
            ''' </summary>
            ''' <param name="location">The name of the file</param>
            ''' <param name="playMode">
            ''' An enum value representing the mode, Background (async), 
            ''' WaitToComplete (sync) or BackgroundLoop
            ''' </param>
            ''' <remarks></remarks>
            Public Sub Play(ByVal location As String, ByVal playMode As AudioPlayMode)
                ValidateAudioPlayModeEnum(playMode, "playMode")
                Dim safeFilename As String = ValidateFilename(location)
                Dim sound As System.Media.SoundPlayer = New System.Media.SoundPlayer(safeFilename)
                Play(sound, playMode)
            End Sub

            '''**********************************************************************
            ''';Play
            ''' <summary>
            '''   Plays a Byte array representation of a .wav file in the passed in mode
            ''' </summary>
            ''' <param name="data">The array representing the .wav file</param>
            ''' <param name="playMode">The mode in which the array should be played</param>
            ''' <remarks></remarks>
            Public Sub Play(ByVal data() As Byte, ByVal playMode As AudioPlayMode)
                If data Is Nothing Then
                    Throw GetArgumentNullException("data")
                End If
                ValidateAudioPlayModeEnum(playMode, "playMode")

                Dim soundStream As IO.MemoryStream = New IO.MemoryStream(data)
                Play(soundStream, playMode)
                soundStream.Close()
            End Sub

            '''**********************************************************************
            ''';Play
            ''' <summary>
            '''  Plays a stream representation of a .wav file in the passed in mode
            ''' </summary>
            ''' <param name="stream">The stream representing the .wav file</param>
            ''' <param name="playMode">The mode in which the stream should be played</param>
            ''' <remarks></remarks>
            Public Sub Play(ByVal stream As IO.Stream, ByVal playMode As AudioPlayMode)
                ValidateAudioPlayModeEnum(playMode, "playMode")
                If stream Is Nothing Then
                    Throw GetArgumentNullException("stream")
                End If

                Play(New System.Media.SoundPlayer(stream), playMode)
            End Sub

            '''**********************************************************************
            ''';PlaySystemSound
            ''' <summary>
            '''   Plays a system messageBeep sound.
            ''' </summary>
            ''' <param name="systemSound">The sound to be played</param>
            ''' <remarks>Plays the sound asysnchronously</remarks>
            Public Sub PlaySystemSound(ByVal systemSound As System.Media.SystemSound)
                If systemSound Is Nothing Then
                    Throw GetArgumentNullException("systemSound")
                End If

                systemSound.Play()

            End Sub

            '''**********************************************************************
            ''';Stop
            ''' <summary>
            '''  Stops the play of any playing sound
            ''' </summary>
            ''' <remarks></remarks>
            Public Sub [Stop]()
                Dim sound As New System.Media.SoundPlayer()
                InternalStop(sound)
            End Sub

            '* PRIVATE *************************************************************

            ''' <summary>
            '''  Plays the passed in SoundPlayer in the passed in mode
            ''' </summary>
            ''' <param name="sound">The SoundPlayer to play</param>
            ''' <param name="mode">The mode in which to play the sound</param>
            ''' <remarks></remarks>
            Private Sub Play(ByVal sound As System.Media.SoundPlayer, ByVal mode As AudioPlayMode)

                Debug.Assert(sound IsNot Nothing, "There's no SoundPlayer")
                Debug.Assert([Enum].IsDefined(GetType(AudioPlayMode), mode), "Enum value is out of range")

                ' Stopping the sound ensures it's safe to dispose it. This could happen when we change the value of m_Sound below
                If m_Sound IsNot Nothing Then
                    InternalStop(m_Sound)
                End If

                m_Sound = sound

                Select Case mode
                    Case AudioPlayMode.WaitToComplete
                        m_Sound.PlaySync()
                    Case AudioPlayMode.Background
                        m_Sound.Play()
                    Case AudioPlayMode.BackgroundLoop
                        m_Sound.PlayLooping()
                    Case Else
                        Debug.Fail("Unknown AudioPlayMode")
                End Select

            End Sub

            '''**********************************************************************
            ''';InternalStop
            ''' <summary>
            ''' SoundPlayer.Stop requires unmanaged code permissions. This method allows us to wrap calls to SoundPlayer.Stop
            ''' with the appropriate Demand/Assert
            ''' </summary>
            ''' <param name="sound"></param>
            ''' <remarks></remarks>
            <SecuritySafeCritical()> _
            Private Shared Sub InternalStop(ByVal sound As System.Media.SoundPlayer)

                ' Stop requires unmanaged code permission. Stop demands SafeSubWindows permissions, so we don't need to do it here                     
                Call New System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode).Assert()
                Try
                    sound.Stop()
                Finally
                    System.Security.CodeAccessPermission.RevertAssert()
                End Try
            End Sub

            '''**********************************************************************
            ''';ValidateFilename
            ''' <summary>
            '''  Gets the full name and path for the file. Throws if unable to get full name and path
            ''' </summary>
            ''' <param name="location">The filename being tested</param>
            ''' <returns>A full name and path of the file</returns>
            ''' <remarks></remarks>
            Private Function ValidateFilename(ByVal location As String) As String
                If location = "" Then
                    Throw GetArgumentNullException("location")
                End If

                Return location
            End Function

            '''**************************************************************************
            ''' ;ValidateAudioPlayModeEnum
            ''' <summary>
            ''' Validates that the value being passed as an AudioPlayMode enum is a legal value
            ''' </summary>
            ''' <param name="value"></param>
            ''' <remarks></remarks>
            Private Sub ValidateAudioPlayModeEnum(ByVal value As AudioPlayMode, ByVal paramName As String)
                If value < AudioPlayMode.WaitToComplete OrElse value > AudioPlayMode.BackgroundLoop Then
                    Throw New System.ComponentModel.InvalidEnumArgumentException(paramName, DirectCast(value, Integer), GetType(AudioPlayMode))
                End If
            End Sub

            ' Object that plays the sounds. We use a private member so we can ensure we have a reference for async plays
            Private m_Sound As System.Media.SoundPlayer

        End Class 'Audio
    End Namespace
End Namespace
