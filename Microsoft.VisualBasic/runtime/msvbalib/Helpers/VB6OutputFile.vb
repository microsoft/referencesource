' Copyright (c) Microsoft Corporation.  All rights reserved.


Imports System
Imports System.Text
Imports System.Globalization
Imports System.IO
Imports System.Security

Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

#Region " BACKWARDS COMPATIBILITY "

    'WARNING WARNING WARNING WARNING WARNING
    'This code exists to support Everett compiled applications.  Make sure you understand
    'the backwards compatibility ramifications of any edit you make in this region.
    'WARNING WARNING WARNING WARNING WARNING

    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Friend Class VB6OutputFile

        '============================================================================
        ' Declarations
        '============================================================================
        Inherits VB6File



        '============================================================================
        ' Constructor
        '============================================================================
        Friend Sub New()
            MyBase.New()
        End Sub



        Friend Sub New(ByVal FileName As String, ByVal share As OpenShare, ByVal fAppend As Boolean)
            MyBase.New(FileName, OpenAccess.Write, share, -1)
            m_fAppend = fAppend
        End Sub



        '============================================================================
        ' Operations
        '============================================================================
        Friend Overrides Sub OpenFile()
            'MyBase.OpenFile()

            Try
                If m_fAppend Then
                    'consider checking WRITE if cannot open READWRITE
                    'Note: COM+BUG - OpenOrCreate is working like create, so we need to make a temporary workaround
                    If File.Exists(m_sFullPath) Then
                        m_file = New FileStream(m_sFullPath, FileMode.Open, CType(m_access, FileAccess), CType(m_share, FileShare))
                    Else
                        m_file = New FileStream(m_sFullPath, FileMode.Create, CType(m_access, FileAccess), CType(m_share, FileShare))
                    End If
                Else
                    m_file = New FileStream(m_sFullPath, FileMode.Create, CType(m_access, FileAccess), CType(m_share, FileShare))
                End If
            Catch ex As FileNotFoundException
                Throw VbMakeException(ex, vbErrors.FileNotFound)
            Catch ex As SecurityException
                Throw VbMakeException(ex, vbErrors.FileNotFound)
            Catch ex As DirectoryNotFoundException
                Throw VbMakeException(ex, vbErrors.PathNotFound)
            Catch ex As IOException
                Throw VbMakeException(ex, vbErrors.PathFileAccess)
            End Try

            m_Encoding = GetFileIOEncoding()
            m_sw = New StreamWriter(m_file, m_Encoding)
            m_sw.AutoFlush = True

            If m_fAppend Then
                'Now position at end of file
                Dim lEndOfFile As Long
                lEndOfFile = m_file.Length
                m_file.Position = lEndOfFile
                m_position = lEndOfFile
            End If
        End Sub



        Friend Overrides Sub WriteLine(ByVal s As String)
            If s Is Nothing Then
                m_sw.WriteLine()
                m_position += 2
            Else
                If m_bPrint AndAlso (m_lWidth <> 0) Then
                    If m_lCurrentColumn >= m_lWidth Then
                        m_sw.WriteLine()
                        m_position += 2
                    End If
                End If

                m_sw.WriteLine(s)
                Diagnostics.Debug.Assert(Not m_Encoding Is Nothing)
                m_position += m_Encoding.GetByteCount(s) + 2
            End If

            m_lCurrentColumn = 0
        End Sub



        Friend Overrides Sub WriteString(ByVal s As String)
            If (s Is Nothing) OrElse (s.Length = 0) Then
                Exit Sub
            End If

            If m_bPrint AndAlso (m_lWidth <> 0) Then
                If (m_lCurrentColumn >= m_lWidth) OrElse _
                   (m_lCurrentColumn <> 0 AndAlso (m_lCurrentColumn + s.Length) > m_lWidth) Then
                    m_sw.WriteLine()
                    m_position += 2
                    m_lCurrentColumn = 0
                End If
            End If

            m_sw.Write(s)
            Diagnostics.Debug.Assert(Not m_Encoding Is Nothing)
            Dim ByteLength As Integer = m_Encoding.GetByteCount(s)
            m_position += ByteLength
            m_lCurrentColumn += s.Length
        End Sub



        Friend Overrides Function CanWrite() As Boolean
            CanWrite = True
        End Function



        Public Overrides Function GetMode() As OpenMode
            If m_fAppend Then
                GetMode = OpenMode.Append
            Else
                GetMode = OpenMode.Output
            End If
        End Function



        Friend Overrides Function EOF() As Boolean
            EOF = True
        End Function



        Friend Overrides Function LOC() As Long
            Return ((m_position + 127) \ 128)
        End Function



    End Class

#End Region

End Namespace
