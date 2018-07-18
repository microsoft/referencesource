' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.Utils
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils

Imports System
#If TELESTO Then
Imports System.Diagnostics
#End If
Imports System.Runtime.InteropServices
Imports System.Security
Imports System.Security.Permissions
Imports System.Runtime.ConstrainedExecution

Namespace Microsoft.VisualBasic
    
    Public NotInheritable Class ErrObject

        ' Error object private values
        Private m_curException As Exception
        Private m_curErl As Integer
        Private m_curNumber As Integer
        Private m_curDescription As String
        Private m_NumberIsSet As Boolean
        Private m_ClearOnCapture As Boolean
        Private m_DescriptionIsSet As Boolean

#If Not TELESTO Then
        Private m_curSource As String
        Private m_SourceIsSet As Boolean
        Private m_curHelpFile As String
        Private m_curHelpContext As Integer
        Private m_HelpFileIsSet As Boolean
        Private m_HelpContextIsSet As Boolean
#End If

        Friend Sub New()
            Me.Clear() 'need to do this so the fields are set to Empty string, not Nothing
        End Sub


        '============================================================================
        ' ErrObject functions.
        '============================================================================
        Public ReadOnly Property Erl() As Integer
            Get
                Return m_curErl
            End Get
        End Property



        Public Property Number() As Integer
            Get
                If m_NumberIsSet Then
                    Return m_curNumber
                End If

                If Not m_curException Is Nothing Then
                    Me.Number = MapExceptionToNumber(m_curException)
                    Return m_curNumber
                Else
                    'The default case.  NOTE:  falling into the default does not "Set" the property.
                    'We only get here if the Err object was previously cleared.
                    Return 0
                End If
            End Get

            Set(ByVal Value As Integer)
                m_curNumber = MapErrorNumber(Value)
                m_NumberIsSet = True
            End Set
        End Property


#If Not TELESTO Then 'There is no concept of a Source on exceptions in Telesto
        Public Property Source() As String
            Get
                'Return the current Source if we've already calculated it.
                If m_SourceIsSet Then
                    Return m_curSource
                End If


                If Not m_curException Is Nothing Then
                    Me.Source = m_curException.Source
                    Return m_curSource
                Else
                    'The default case.  NOTE:  falling into the default does not "Set" the property.
                    'We only get here if the Err object was previously cleared.
                    '
                    Return ""
                End If
            End Get

            Set(ByVal Value As String)
                m_curSource = Value
                m_SourceIsSet = True
            End Set
        End Property
#End If 'not TELESTO

        ''' <summary>
        ''' Determines what the correct error description should be.
        ''' If we don't have an exception that we are responding to then
        ''' we don't do anything to the message.
        ''' If we do have an exception pending, we morph the description
        ''' to match the corresponding VB error.
        ''' We also special case HRESULT exceptions to map to a VB description
        ''' if we have one.
        ''' </summary>
        ''' <param name="Msg"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        Private Function FilterDefaultMessage(ByVal Msg As String) As String
            Dim NewMsg As String

            'This is one of the default messages, 
            If m_curException Is Nothing Then
                'Leave message as is
                Return Msg
            End If

            Dim tmpNumber As Integer = Me.Number

            If Msg Is Nothing OrElse Msg.Length = 0 Then
                Msg = GetResourceString("ID" & CStr(tmpNumber))
            ElseIf System.String.CompareOrdinal("Exception from HRESULT: 0x", 0, Msg, 0, Math.Min(Msg.Length, 26)) = 0 Then
                NewMsg = GetResourceString("ID" & CStr(m_curNumber), False)
                If Not NewMsg Is Nothing Then
                    Msg = NewMsg
                End If
            End If

            Return Msg
        End Function


        Public Property Description() As String
            Get
                If m_DescriptionIsSet Then
                    Return m_curDescription
                End If

                If Not m_curException Is Nothing Then
                    Me.Description = FilterDefaultMessage(m_curException.Message)
                    Return m_curDescription
                Else
                    'The default case.  NOTE:  falling into the default does not "Set" the property.
                    'We only get here if the Err object was previously cleared.
                    Return ""
                End If
            End Get

            Set(ByVal Value As String)
                m_curDescription = Value
                m_DescriptionIsSet = True
            End Set
        End Property

#If Not TELESTO Then 'There is no concept of a HelpFile on exceptions in Telesto
        Public Property HelpFile() As String
            Get
                If m_HelpFileIsSet Then
                    Return m_curHelpFile
                End If

                If Not m_curException Is Nothing Then
                    ParseHelpLink(m_curException.HelpLink)
                    Return m_curHelpFile
                Else
                    'The default case.  NOTE:  falling into the default does not "Set" the property.
                    'We only get here if the Err object was previously cleared.
                    '
                    Return ""
                End If
            End Get

            Set(ByVal Value As String)
                m_curHelpFile = Value

                m_HelpFileIsSet = True
            End Set
        End Property


        Private Function MakeHelpLink(ByVal HelpFile As String, ByVal HelpContext As Integer) As String
            Return HelpFile & "#" & CStr(HelpContext)
        End Function

        Private Sub ParseHelpLink(ByVal HelpLink As String)

            Diagnostics.Debug.Assert((Not m_HelpContextIsSet) OrElse (Not m_HelpFileIsSet), "Why is this getting called?")

            If HelpLink Is Nothing OrElse HelpLink.Length = 0 Then

                If Not m_HelpContextIsSet Then
                    Me.HelpContext = 0
                End If
                If Not m_HelpFileIsSet Then
                    Me.HelpFile = ""
                End If

            Else

                Dim iContext As Integer = m_InvariantCompareInfo.IndexOf(HelpLink, "#", Globalization.CompareOptions.Ordinal)

                If iContext <> -1 Then
                    If Not m_HelpContextIsSet Then
                        If iContext < HelpLink.Length Then
                            Me.HelpContext = CInt(HelpLink.Substring(iContext + 1))
                        Else
                            Me.HelpContext = 0
                        End If
                    End If
                    If Not m_HelpFileIsSet Then
                        Me.HelpFile = HelpLink.Substring(0, iContext)
                    End If
                Else
                    If Not m_HelpContextIsSet Then
                        Me.HelpContext = 0
                    End If
                    If Not m_HelpFileIsSet Then
                        Me.HelpFile = HelpLink
                    End If
                End If

            End If

        End Sub



        Public Property HelpContext() As Integer
            Get
                If m_HelpContextIsSet Then
                    Return m_curHelpContext
                End If

                If Not m_curException Is Nothing Then
                    ParseHelpLink(m_curException.HelpLink)
                    Return m_curHelpContext

                Else
                    'The default case.  NOTE:  falling into the default does not "Set" the property.
                    'We only get here if the Err object was previously cleared.
                    '
                    Return 0
                End If

                Return m_curHelpContext
            End Get

            Set(ByVal Value As Integer)
                m_curHelpContext = Value
                m_HelpContextIsSet = True
            End Set
        End Property
#End If 'not TELESTO

        Public Function GetException() As Exception
            Return m_curException
        End Function

        ''' <summary>
        ''' VB calls clear whenever it executes any type of Resume statement, Exit Sub, Exit funcion, exit Property, or
        ''' any On Error statement.
        ''' </summary>
        ''' <remarks></remarks>
#If TELESTO Then
        Public Sub Clear()
#Else
        <SecuritySafeCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Public Sub Clear()

            ' The Try/Finally and constrained regions calls guarantee success under high
            ' stress conditions by enabling eager jitting of the finally block

            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
#End If
            Try
            Finally
                'CONSIDER:  do we even care about CLEARING the fields if clearing the flags are enough (aside from m_curException)?
                m_curException = Nothing
                m_curNumber = 0
#If Not TELESTO Then
                m_curSource = ""
                m_curHelpFile = ""
                m_curHelpContext = 0
                m_SourceIsSet = False
                m_HelpFileIsSet = False
                m_HelpContextIsSet = False
#End If
                m_curDescription = ""
                m_curErl = 0
                m_NumberIsSet = False
                m_DescriptionIsSet = False
                m_ClearOnCapture = True
            End Try
        End Sub

#If TELESTO Then
         ''' <summary>
        ''' This function is called when the Raise code command is executed
        ''' </summary>
        ''' <param name="Number">The error code being raised</param>
        ''' <param name="Description">If not supplied, we try to look one up based on the error code being raised</param>
        ''' <remarks></remarks>
        Public Sub Raise(ByVal Number As Integer, Optional ByVal Description As Object = Nothing)
#Else
        ''' <summary>
        ''' This function is called when the Raise code command is executed
        ''' </summary>
        ''' <param name="Number">The error code being raised</param>
        ''' <param name="Source">If not supplied we take the name from the assembly</param>
        ''' <param name="Description">If not supplied, we try to look one up based on the error code being raised</param>
        ''' <param name="HelpFile"></param>
        ''' <param name="HelpContext"></param>
        ''' <remarks></remarks>
        Public Sub Raise(ByVal Number As Integer, _
                         Optional ByVal Source As Object = Nothing, _
                         Optional ByVal Description As Object = Nothing, _
                         Optional ByVal HelpFile As Object = Nothing, _
                         Optional ByVal HelpContext As Object = Nothing)
#End If

            If Number = 0 Then
                'This is only called by Raise, so Raise(0) should give the following exception
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Number"))
            End If
            Me.Number = Number

#If Not TELESTO Then 'Telesto doesn't have the notion of Source or Help file links
            If Not Source Is Nothing Then
                Me.Source = CStr(Source)
            Else
                Dim vbhost As CompilerServices.IVbHost
                vbhost = CompilerServices.HostServices.VBHost
                If vbhost Is Nothing Then
                    Dim FullName As String
                    Dim CommaPos As Integer

                    FullName = System.Reflection.Assembly.GetCallingAssembly().FullName
                    CommaPos = InStr(FullName, ",")
                    If CommaPos < 1 Then
                        Me.Source = FullName
                    Else
                        Me.Source = Left(FullName, CommaPos - 1)
                    End If
                Else
                    Me.Source = vbhost.GetWindowTitle()
                End If
            End If

            If Not HelpFile Is Nothing Then
                Me.HelpFile = CStr(HelpFile)
            End If

            If Not HelpContext Is Nothing Then
                Me.HelpContext = CInt(HelpContext)
            End If
#End If 'not TELESTO

            If Not Description Is Nothing Then
                Me.Description = CStr(Description)
            ElseIf Not m_DescriptionIsSet Then
                'Set the Description here so the exception object contains the right message
                Me.Description = GetResourceString(CType(m_curNumber, vbErrors))
            End If

            Dim e As Exception
            e = MapNumberToException(m_curNumber, m_curDescription)
#If Not TELESTO Then
            e.Source = m_curSource
            e.HelpLink = MakeHelpLink(m_curHelpFile, m_curHelpContext)
#End If
            m_ClearOnCapture = False
            Throw e
        End Sub

#If Not TELESTO Then ' DevDiv Bugs 117407 - Remove LastDllError from Silverlight.
        ReadOnly Property LastDllError() As Integer
            <SecurityCritical()> _
            Get
                Return Marshal.GetLastWin32Error()
            End Get
        End Property
#End If

        Friend Sub SetUnmappedError(ByVal Number As Integer)
            Me.Clear()
            Me.Number = Number
            m_ClearOnCapture = False
        End Sub



        'a function like this that can be used by the runtime to generate errors which will also do a clear would be nice.
        Friend Function CreateException(ByVal Number As Integer, ByVal Description As String) As System.Exception
            Me.Clear()
            Me.Number = Number

            If Number = 0 Then
                'This is only called by Error xxxx, zero is not a valid exception number
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Number"))
            End If

            Dim e As Exception = MapNumberToException(m_curNumber, Description)
            m_ClearOnCapture = False
            Return e
        End Function

#If TELESTO Then
        'No ReliabilityContract defined on Telesto
        <SecurityCritical()> _
        Friend Overloads Sub CaptureException(ByVal ex As Exception)
#Else
        <SecurityCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Friend Overloads Sub CaptureException(ByVal ex As Exception)

            ' The Try/Finally and constrained regions calls guarantee success under high
            ' stress conditions by enabling eager jitting of the finally block
            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
#End If

            Try
            Finally
                'if we've already captured this exception, then we're done
                If ex IsNot m_curException Then
                    If m_ClearOnCapture Then
                        Me.Clear()
                    Else
                        m_ClearOnCapture = True   'False only used once - set this flag back to the default
                    End If
                    m_curException = ex
                End If
            End Try
        End Sub

#If TELESTO Then
        'No ReliabilityContract defined on Telesto
        <SecurityCritical()> _      
        Friend Overloads Sub CaptureException(ByVal ex As Exception, ByVal lErl As Integer)
#Else
        <SecurityCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        Friend Overloads Sub CaptureException(ByVal ex As Exception, ByVal lErl As Integer)

            ' The Try/Finally and constrained regions calls guarantee success under high stress conditions by enabling eager jitting of the finally block
            System.Runtime.CompilerServices.RuntimeHelpers.PrepareConstrainedRegions()
#End If
            Try
            Finally
                CaptureException(ex)
                m_curErl = lErl  'This is the only place where the line number can be set
            End Try
        End Sub


        Private Function MapExceptionToNumber(ByVal e As Exception) As Integer
#If TELESTO Then
            Debug.Assert(e IsNot Nothing, "Exception shouldn't be Nothing")
#Else
            Diagnostics.Debug.Assert(e IsNot Nothing, "Exception shouldn't be Nothing")
#End If
            Dim typ As Type = e.GetType()

            If typ Is GetType(System.IndexOutOfRangeException) Then
                Return vbErrors.OutOfBounds
            ElseIf typ Is GetType(System.RankException) Then
                Return vbErrors.OutOfBounds
            ElseIf typ Is GetType(System.DivideByZeroException) Then
                Return vbErrors.DivByZero
            ElseIf typ Is GetType(System.OverflowException) Then
                Return vbErrors.Overflow
            ElseIf typ Is GetType(System.NotFiniteNumberException) Then
                Dim exNotFiniteNumber As NotFiniteNumberException = CType(e, NotFiniteNumberException)
#If Not TELESTO Then
                If exNotFiniteNumber.OffendingNumber = 0 Then
                    Return vbErrors.DivByZero
                Else
                    Return vbErrors.Overflow
                End If
#Else
                'Telesto doesn't have the OffendingNumbermember on System.NotFiniteNumberException.
                'Which seems like a good thing because the value is zero by default if the exception isn't initially constructed
                'by passing in the offending value.  And we already have a divide by zero exception...  So I think it is a 'code improvement' 
                'to get rid of logic based on what is in OffendingNumber.
                Return vbErrors.Overflow
#End If
            ElseIf typ Is GetType(System.NullReferenceException) Then
                Return vbErrors.ObjNotSet
            ElseIf TypeOf e Is System.AccessViolationException Then
                Return vbErrors.AccessViolation
            ElseIf typ Is GetType(System.InvalidCastException) Then
                Return vbErrors.TypeMismatch
            ElseIf typ Is GetType(System.NotSupportedException) Then
                Return vbErrors.TypeMismatch
#If Not TELESTO Then
            ElseIf typ Is GetType(System.Runtime.InteropServices.COMException) Then
                Dim comex As COMException = CType(e, COMException)
                Return CompilerServices.Utils.MapHRESULT(comex.ErrorCode)
#End If
            ElseIf typ Is GetType(System.Runtime.InteropServices.SEHException) Then
                Return vbErrors.DLLCallException
            ElseIf typ Is GetType(System.DllNotFoundException) Then
                Return vbErrors.FileNotFound
            ElseIf typ Is GetType(System.EntryPointNotFoundException) Then
                Return vbErrors.InvalidDllFunctionName
                '
                'Must fall after EntryPointNotFoundException because of inheritance
                '
            ElseIf typ Is GetType(System.TypeLoadException) Then
                Return vbErrors.CantCreateObject
            ElseIf typ Is GetType(System.OutOfMemoryException) Then
                Return vbErrors.OutOfMemory
            ElseIf typ Is GetType(System.FormatException) Then
                Return vbErrors.TypeMismatch
            ElseIf typ Is GetType(System.IO.DirectoryNotFoundException) Then
                Return vbErrors.PathNotFound
            ElseIf typ Is GetType(System.IO.IOException) Then
                Return vbErrors.IOError
            ElseIf typ Is GetType(System.IO.FileNotFoundException) Then
                Return vbErrors.FileNotFound
            ElseIf TypeOf e Is MissingMemberException Then
                Return vbErrors.OLENoPropOrMethod
#If Not TELESTO Then
            ElseIf TypeOf e Is Runtime.InteropServices.InvalidOleVariantTypeException Then
                Return vbErrors.InvalidTypeLibVariable
#End If
            Else
                Return vbErrors.IllegalFuncCall   'Generic error
            End If

        End Function



        Private Function MapNumberToException(ByVal Number As Integer, _
                                              ByVal Description As String) As System.Exception
            Return ExceptionUtils.BuildException(Number, Description, False)
        End Function



        Friend Function MapErrorNumber(ByVal Number As Integer) As Integer
            If Number > 65535 Then
                ' Number cannot be greater than 65535.
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue1, "Number"))
            End If

            If Number >= 0 Then
                Return Number
            End If

            'strip off top two bytes if FACILITY_CONTROL is set
            If (Number And SCODE_FACILITY) = FACILITY_CONTROL Then
                Return (Number And &HFFFFI)
            End If

            Select Case Number

                ' FACILITY_NULL errors
                Case E_NOTIMPL : Return vbErrors.ActionNotSupported '&H80004001
                Case E_NOINTERFACE : Return vbErrors.OLENotSupported '&H80004002
                Case E_POINTER : Return vbErrors.AccessViolation '&H80004003
                Case E_ABORT : Return vbErrors.Abort      '&H80004004

                    ' FACILITY_DISPATCH - IDispatch errors.
                Case DISP_E_UNKNOWNINTERFACE : Return vbErrors.OLENoPropOrMethod '&H80020001
                Case DISP_E_MEMBERNOTFOUND : Return vbErrors.OLENoPropOrMethod '&H80020003
                Case DISP_E_PARAMNOTFOUND : Return vbErrors.NamedParamNotFound '&H80020004
                Case DISP_E_TYPEMISMATCH : Return vbErrors.TypeMismatch '&H80020005
                Case DISP_E_UNKNOWNNAME : Return vbErrors.OLENoPropOrMethod '&H80020006
                Case DISP_E_NONAMEDARGS : Return vbErrors.NamedArgsNotSupported '&H80020007
                Case DISP_E_BADVARTYPE : Return vbErrors.InvalidTypeLibVariable '&H80020008
                Case DISP_E_OVERFLOW : Return vbErrors.Overflow '&H8002000A
                Case DISP_E_BADINDEX : Return vbErrors.OutOfBounds '&H8002000B
                Case DISP_E_UNKNOWNLCID : Return vbErrors.LocaleSettingNotSupported '&H8002000C
                Case DISP_E_ARRAYISLOCKED : Return vbErrors.ArrayLocked '&H8002000D
                Case DISP_E_BADPARAMCOUNT : Return vbErrors.FuncArityMismatch '&H8002000E
                Case DISP_E_PARAMNOTOPTIONAL : Return vbErrors.ParameterNotOptional '&H8002000F
                Case DISP_E_NOTACOLLECTION : Return vbErrors.NotEnum '&H80020011
                Case DISP_E_DIVBYZERO : Return vbErrors.DivByZero '&H80020012

                    ' FACILITY_DISPATCH - Typelib errors
                Case TYPE_E_BUFFERTOOSMALL : Return vbErrors.BufferTooSmall '&H80028016
                Case &H80028017I : Return vbErrors.IdentNotMember '&H80028017
                Case TYPE_E_INVDATAREAD : Return vbErrors.InvDataRead '&H80028018
                Case TYPE_E_UNSUPFORMAT : Return vbErrors.UnsupFormat '&H80028019
                Case TYPE_E_REGISTRYACCESS : Return vbErrors.RegistryAccess '&H8002801C
                Case TYPE_E_LIBNOTREGISTERED : Return vbErrors.LibNotRegistered '&H8002801D
                Case TYPE_E_UNDEFINEDTYPE : Return vbErrors.UndefinedType '&H80028027
                Case TYPE_E_QUALIFIEDNAMEDISALLOWED : Return vbErrors.QualifiedNameDisallowed '&H80028028
                Case TYPE_E_INVALIDSTATE : Return vbErrors.InvalidState '&H80028029
                Case TYPE_E_WRONGTYPEKIND : Return vbErrors.WrongTypeKind '&H8002802A
                Case TYPE_E_ELEMENTNOTFOUND : Return vbErrors.ElementNotFound '&H8002802B
                Case TYPE_E_AMBIGUOUSNAME : Return vbErrors.AmbiguousName '&H8002802C
                Case TYPE_E_NAMECONFLICT : Return vbErrors.ModNameConflict '&H8002802D
                Case TYPE_E_UNKNOWNLCID : Return vbErrors.UnknownLcid '&H8002802E
                Case TYPE_E_DLLFUNCTIONNOTFOUND : Return vbErrors.InvalidDllFunctionName '&H8002802F
                Case TYPE_E_BADMODULEKIND : Return vbErrors.BadModuleKind '&H800288BD
                Case TYPE_E_SIZETOOBIG : Return vbErrors.SizeTooBig '&H800288C5
                Case TYPE_E_TYPEMISMATCH : Return vbErrors.TypeMismatch '&H80028CA0
                Case TYPE_E_OUTOFBOUNDS : Return vbErrors.OutOfBounds '&H80028CA1
                Case TYPE_E_IOERROR : Return vbErrors.IOError '&H80028CA2
                Case TYPE_E_CANTCREATETMPFILE : Return vbErrors.CantCreateTmpFile '&H80028CA3
                Case TYPE_E_CANTLOADLIBRARY : Return vbErrors.DLLLoadErr '&H80029C4A
                Case TYPE_E_INCONSISTENTPROPFUNCS : Return vbErrors.InconsistentPropFuncs '&H80029C83
                Case TYPE_E_CIRCULARTYPE : Return vbErrors.CircularType '&H80029C84

                    ' FACILITY_STORAGE errors
                Case STG_E_INVALIDFUNCTION : Return vbErrors.BadFunctionId '&H80030001
                Case STG_E_FILENOTFOUND : Return vbErrors.FileNotFound '&H80030002
                Case STG_E_PATHNOTFOUND : Return vbErrors.PathNotFound '&H80030003
                Case STG_E_TOOMANYOPENFILES : Return vbErrors.TooManyFiles '&H80030004
                Case STG_E_ACCESSDENIED : Return vbErrors.PermissionDenied '&H80030005
                Case STG_E_INVALIDHANDLE : Return vbErrors.ReadFault '&H80030006
                Case STG_E_INSUFFICIENTMEMORY : Return vbErrors.OutOfMemory '&H80030008
                Case STG_E_NOMOREFILES : Return vbErrors.TooManyFiles '&H80030012
                Case STG_E_DISKISWRITEPROTECTED : Return vbErrors.PermissionDenied '&H80030013
                Case STG_E_SEEKERROR : Return vbErrors.SeekErr '&H80030019
                Case STG_E_WRITEFAULT : Return vbErrors.WriteFault '&H8003001D
                Case STG_E_READFAULT : Return vbErrors.ReadFault '&H8003001E
                Case STG_E_SHAREVIOLATION : Return vbErrors.PathFileAccess '&H80030020
                Case STG_E_LOCKVIOLATION : Return vbErrors.PermissionDenied '&H80030021
                Case STG_E_FILEALREADYEXISTS : Return vbErrors.FileAlreadyExists '&H80030050
                Case STG_E_MEDIUMFULL : Return vbErrors.DiskFull '&H80030070
                Case STG_E_INVALIDHEADER : Return vbErrors.InvDataRead '&H800300FB
                Case STG_E_INVALIDNAME : Return vbErrors.FileNotFound '&H800300FC
                Case STG_E_UNKNOWN : Return vbErrors.InvDataRead '&H800300FD
                Case STG_E_UNIMPLEMENTEDFUNCTION : Return vbErrors.NotYetImplemented '&H800300FE
                Case STG_E_INUSE : Return vbErrors.PermissionDenied '&H80030100
                Case STG_E_NOTCURRENT : Return vbErrors.PermissionDenied '&H80030101
                Case STG_E_REVERTED : Return vbErrors.WriteFault '&H80030102
                Case STG_E_CANTSAVE : Return vbErrors.IOError '&H80030103
                Case STG_E_OLDFORMAT : Return vbErrors.UnsupFormat '&H80030104
                Case STG_E_OLDDLL : Return vbErrors.UnsupFormat '&H80030105
                Case STG_E_SHAREREQUIRED : Return vbErrors.ShareRequired '&H80030106
                Case STG_E_NOTFILEBASEDSTORAGE : Return vbErrors.UnsupFormat '&H80030107
                Case STG_E_EXTANTMARSHALLINGS : Return vbErrors.UnsupFormat '&H80030108

                    ' FACILITY_ITF errors.
                Case CLASS_E_NOTLICENSED : Return vbErrors.CantCreateObject '&H80040112
                Case REGDB_E_CLASSNOTREG : Return vbErrors.CantCreateObject '&H80040154
                Case MK_E_UNAVAILABLE : Return vbErrors.CantCreateObject '&H800401E3
                Case MK_E_INVALIDEXTENSION : Return vbErrors.OLEFileNotFound '&H800401E6
                Case MK_E_CANTOPENFILE : Return vbErrors.OLEFileNotFound '&H800401EA
                Case CO_E_CLASSSTRING : Return vbErrors.CantCreateObject '&H800401F3 
                Case CO_E_APPNOTFOUND : Return vbErrors.CantCreateObject '&H800401F5
                Case CO_E_APPDIDNTREG : Return vbErrors.CantCreateObject '&H800401FE

                    ' FACILITY_WIN32 errors
                Case E_ACCESSDENIED : Return vbErrors.PermissionDenied '&H80070005
                Case E_OUTOFMEMORY : Return vbErrors.OutOfMemory '&H8007000E
                Case E_INVALIDARG : Return vbErrors.IllegalFuncCall '&H80070057
                Case &H800706BAI : Return vbErrors.ServerNotFound '&H800706BA

                    ' FACILITY_WINDOWS - I don't know why this differs from FACILITY_WIN32
                Case CO_E_SERVER_EXEC_FAILURE : Return vbErrors.CantCreateObject '&H80080005

                Case Else
                    Return Number
            End Select
        End Function

    End Class
End Namespace
