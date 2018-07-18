' Copyright (c) Microsoft Corporation.  All rights reserved.
Option Explicit On
Option Strict On

Imports System.Collections.Specialized
Imports System.ComponentModel
Imports System.Drawing
Imports System.IO
Imports System.Security.Permissions
Imports System.Windows.Forms

Namespace Microsoft.VisualBasic.MyServices

    '''*****************************************************************************
    ''';ClipboardProxy
    ''' <summary>
    ''' A class that wraps System.Windows.Forms.Clipboard so that
    ''' a clipboard can be instanced.
    ''' </summary>
    ''' <remarks></remarks>
    <EditorBrowsable(EditorBrowsableState.Never), HostProtection(Resources:=HostProtectionResource.ExternalProcessMgmt)> _
    Public Class ClipboardProxy

        '==PUBLIC*******************************************************************

        '''*************************************************************************
        ''';New
        ''' <summary>
        ''' Only Allows instantiation of the class
        ''' </summary>
        ''' <remarks></remarks>
        Friend Sub New()
        End Sub

        '''*************************************************************************
        ''';GetText
        ''' <summary>
        ''' Gets text from the clipbaord
        ''' </summary>
        ''' <returns>The text as a String</returns>
        ''' <remarks></remarks>
        Public Function GetText() As String
            Return Clipboard.GetText()
        End Function

        '''*************************************************************************
        ''';GetText
        ''' <summary>
        ''' Gets text from the clipboard saved in the passed in format
        ''' </summary>
        ''' <param name="format">The type of text to get</param>
        ''' <returns>The text as a String</returns>
        ''' <remarks></remarks>
        Public Function GetText(ByVal format As TextDataFormat) As String
            Return Clipboard.GetText(format)
        End Function

        '''*************************************************************************
        ''';ContainsText
        ''' <summary>
        ''' Indicates whether or not text is available on the clipboard
        ''' </summary>
        ''' <returns>True if text is available, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsText() As Boolean
            Return Clipboard.ContainsText
        End Function

        '''*************************************************************************
        ''';ContainsText
        ''' <summary>
        ''' Indicates whether or not text is available on the clipboard in 
        ''' the passed in format
        ''' </summary>
        ''' <param name="format">The type of text being checked for</param>
        ''' <returns>True if text is available, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsText(ByVal format As TextDataFormat) As Boolean
            Return Clipboard.ContainsText(format)
        End Function

        '''*************************************************************************
        ''';SetText
        ''' <summary>
        ''' Saves the passed in String to the clipboard
        ''' </summary>
        ''' <param name="text">The String to save</param>
        ''' <remarks></remarks>
        Public Sub SetText(ByVal text As String)
            Clipboard.SetText(text)
        End Sub

        '''*************************************************************************
        ''';SetText
        ''' <summary>
        ''' Saves the passed in String to the clipboard in the passed in format
        ''' </summary>
        ''' <param name="text">The String to save</param>
        ''' <param name="format">The format in which to save the String</param>
        ''' <remarks></remarks>
        Public Sub SetText(ByVal text As String, ByVal format As TextDataFormat)
            Clipboard.SetText(text, format)
        End Sub

        '''*************************************************************************
        ''';GetImage
        ''' <summary>
        ''' Gets an Image from the clipboard
        ''' </summary>
        ''' <returns>The image</returns>
        ''' <remarks></remarks>
        Public Function GetImage() As Image
            Return Clipboard.GetImage()
        End Function

        '''*************************************************************************
        ''';ContainsImage
        ''' <summary>
        ''' Indicate whether or not an image has been saved to the clipboard
        ''' </summary>
        ''' <returns>True if an image is available, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsImage() As Boolean
            Return Clipboard.ContainsImage()
        End Function

        '''************************************************************************
        ''';SetImage
        ''' <summary>
        ''' Saves the passed in image to the clipboard
        ''' </summary>
        ''' <param name="image">The image to be saved</param>
        ''' <remarks></remarks>
        Public Sub SetImage(ByVal image As Image)
            Clipboard.SetImage(image)
        End Sub

        '''************************************************************************
        ''';GetAudioStream
        ''' <summary>
        ''' Gets an audio stream from the clipboard
        ''' </summary>
        ''' <returns>The audio stream as a Stream</returns>
        ''' <remarks></remarks>
        Public Function GetAudioStream() As Stream
            Return Clipboard.GetAudioStream()
        End Function

        '''************************************************************************
        ''';ContainsAudio
        ''' <summary>
        ''' Indicates whether or not there's an audio stream saved to the clipboard
        ''' </summary>
        ''' <returns>True if an audio stream is available, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsAudio() As Boolean
            Return Clipboard.ContainsAudio()
        End Function

        '''***********************************************************************
        ''';SetAudio
        ''' <summary>
        ''' Saves the passed in audio byte array to the clipboard
        ''' </summary>
        ''' <param name="audioBytes">The byte array to be saved</param>
        ''' <remarks></remarks>
        Public Sub SetAudio(ByVal audioBytes As Byte())
            Clipboard.SetAudio(audioBytes)
        End Sub

        '''***********************************************************************
        ''';SetAudio
        ''' <summary>
        ''' Saves the passed in audio stream to the clipboard
        ''' </summary>
        ''' <param name="audioStream">The stream to be saved</param>
        ''' <remarks></remarks>
        Public Sub SetAudio(ByVal audioStream As Stream)
            Clipboard.SetAudio(audioStream)
        End Sub

        '''***********************************************************************
        ''';GetFileDropList
        ''' <summary>
        ''' Gets a file drop list from the clipboard
        ''' </summary>
        ''' <returns>The list of file paths as a StringCollection</returns>
        ''' <remarks></remarks>
        Public Function GetFileDropList() As StringCollection
            Return Clipboard.GetFileDropList()
        End Function

        '''***********************************************************************
        ''';ContainsFileDropList
        ''' <summary>
        ''' Indicates whether or not a file drop list has been saved to the clipboard
        ''' </summary>
        ''' <returns>True if a file drop list is available, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsFileDropList() As Boolean
            Return Clipboard.ContainsFileDropList()
        End Function

        '''***********************************************************************
        ''';SetFileDropList
        ''' <summary>
        ''' Saves the passed in file drop list to the clipboard
        ''' </summary>
        ''' <param name="filePaths">The file drop list as a StringCollection</param>
        ''' <remarks></remarks>
        Public Sub SetFileDropList(ByVal filePaths As StringCollection)
            Clipboard.SetFileDropList(filePaths)
        End Sub

        '''***********************************************************************
        ''';GetData
        ''' <summary>
        ''' Gets data from the clipboard that's been saved in the passed in format.
        ''' </summary>
        ''' <param name="format">The type of data being sought</param>
        ''' <returns>The data</returns>
        ''' <remarks></remarks>
        Public Function GetData(ByVal format As String) As Object
            Return Clipboard.GetData(format)
        End Function

        '''***********************************************************************
        ''';ContainsData
        ''' <summary>
        ''' Indicates whether or not there is data on the clipboard in the passed in format
        ''' </summary>
        ''' <param name="format"></param>
        ''' <returns>True if there's data in the passed in format, otherwise False</returns>
        ''' <remarks></remarks>
        Public Function ContainsData(ByVal format As String) As Boolean
            Return Clipboard.ContainsData(format)
        End Function

        '''***********************************************************************
        ''';SetData
        ''' <summary>
        ''' Saves the passed in data to the clipboard in the passed in format
        ''' </summary>
        ''' <param name="format">The format in which to save the data</param>
        ''' <param name="data">The data to be saved</param>
        ''' <remarks></remarks>
        Public Sub SetData(ByVal format As String, ByVal data As Object)
            Clipboard.SetData(format, data)
        End Sub

        '''***********************************************************************
        ''';Clear
        ''' <summary>
        ''' Removes everything from the clipboard
        ''' </summary>
        ''' <remarks></remarks>
        Public Sub Clear()
            Clipboard.Clear()
        End Sub

        '''***********************************************************************
        ''';GetDataObject
        ''' <summary>
        ''' Gets a Data Object from the clipboard.
        ''' </summary>
        ''' <returns>The data object</returns>
        ''' <remarks>This gives the ability to save an object in multiple formats</remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Public Function GetDataObject() As IDataObject
            Return Clipboard.GetDataObject()
        End Function

        '''***********************************************************************
        ''';SetDataObject
        ''' <summary>
        ''' Saves a DataObject to the clipboard
        ''' </summary>
        ''' <param name="data">The data object to be saved</param>
        ''' <remarks>This gives the ability to save an object in multiple formats</remarks>
        <EditorBrowsable(EditorBrowsableState.Advanced)> _
        Public Sub SetDataObject(ByVal data As DataObject)
            Clipboard.SetDataObject(data)
        End Sub
    End Class
End Namespace
