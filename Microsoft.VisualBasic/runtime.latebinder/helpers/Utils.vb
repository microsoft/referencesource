' Copyright (c) Microsoft Corporation.  All rights reserved.




Imports System
Imports System.Security
Imports System.Security.Permissions
Imports System.Text
Imports System.Globalization
Imports System.Runtime.InteropServices
Imports System.Reflection
Imports System.Diagnostics
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Symbols
'Imports System.Runtime.ConstrainedExecution

Namespace Microsoft.VisualBasic.CompilerServices

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
    Public NotInheritable Class Utils
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Partial Public NotInheritable Class Utils
#End If

        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend Const SEVERITY_ERROR As Integer = &H80000000I
        Friend Const FACILITY_CONTROL As Integer = &HA0000I
        Friend Const FACILITY_RPC As Integer = &H10000I
        Friend Const FACILITY_ITF As Integer = &H40000I
        Friend Const SCODE_FACILITY As Integer = &H1FFF0000I
        Private Const ERROR_INVALID_PARAMETER As Integer = 87

        Friend Const chPeriod As Char = "."c
        Friend Const chSpace As Char = ChrW(32)
        Friend Const chIntlSpace As Char = ChrW(&H3000)
        Friend Const chZero As Char = "0"c
        Friend Const chHyphen As Char = "-"c
        Friend Const chPlus As Char = "+"c
        Friend Const chLetterA As Char = "A"c
        Friend Const chLetterZ As Char = "Z"c
        Friend Const chColon As Char = ":"c
        Friend Const chSlash As Char = "/"c
        Friend Const chBackslash As Char = "\"c
        Friend Const chTab As Char = ControlChars.Tab
        Friend Const chCharH0A As Char = ChrW(&HA)
        Friend Const chCharH0B As Char = ChrW(&HB)
        Friend Const chCharH0C As Char = ChrW(&HC)
        Friend Const chCharH0D As Char = ChrW(&HD)
        Friend Const chLineFeed As Char = ChrW(10)
        Friend Const chDblQuote As Char = ChrW(34)

        Friend Const chGenericManglingChar As Char = "`"c

        Friend Const OptionCompareTextFlags As CompareOptions = (CompareOptions.IgnoreCase Or CompareOptions.IgnoreWidth Or CompareOptions.IgnoreKanaType)

        ' DON'T ACCESS DIRECTLY! Go through the property below
        Private Shared m_VBAResourceManager As System.Resources.ResourceManager
        Private Shared m_TriedLoadingResourceManager As Boolean
        Private Shared ReadOnly ResourceManagerSyncObj As Object = New Object

        Private Shared m_DebugResourceManager As System.Resources.ResourceManager
        Private Shared m_TriedLoadingDebugResourceManager As Boolean
        Private Shared ReadOnly DebugResourceManagerSyncObj As Object = New Object

        Private Shared m_FallbackResourceManager As System.Resources.ResourceManager
        Private Shared m_TriedLoadingFallbackResourceManager As Boolean
        Private Shared ReadOnly FallbackResourceManagerSyncObj As Object = New Object

        Private Const ResourceMsgDefault As String = "Message text unavailable.  Resource file 'Microsoft.VisualBasic resources' not found."
        Private Const VBDefaultErrorID As String = "ID95"
        Friend Shared m_achIntlSpace() As Char = {chSpace, chIntlSpace}
        Private Shared ReadOnly VoidType As Type = System.Type.GetType("System.Void")
        Private Shared m_VBRuntimeAssembly As System.Reflection.Assembly

        '============================================================================
        ' Shared Error functions
        '============================================================================

        Friend Shared ReadOnly Property VBAResourceManager() As System.Resources.ResourceManager
            Get

                If Not m_VBAResourceManager Is Nothing Then
                    Return m_VBAResourceManager
                End If

                SyncLock ResourceManagerSyncObj
                    If Not m_TriedLoadingResourceManager Then
                        Try
                            m_VBAResourceManager = New System.Resources.ResourceManager("Microsoft.VisualBasic.LateBinder", VBRuntimeAssembly)
                        Catch ex As StackOverflowException
                            Throw ex
                        Catch ex As OutOfMemoryException
                            Throw ex
                        Catch ex As System.Threading.ThreadAbortException
                            Throw ex
                        Catch
                        End Try
                        m_TriedLoadingResourceManager = True
                    End If
                End SyncLock

                Return m_VBAResourceManager
            End Get
        End Property

        Friend Shared ReadOnly Property DebugResourceManager() As System.Resources.ResourceManager
            Get

                If Not m_DebugResourceManager Is Nothing Then
                    Return m_DebugResourceManager
                End If

                SyncLock DebugResourceManagerSyncObj
                    If Not m_TriedLoadingDebugResourceManager Then
                        Try
                            Dim assemblyString As String = ("Microsoft.VisualBasic.debug.resources, Version=2.0.5.0, Culture=en-US, PublicKeyToken=31bf3856ad364e35")
                            Dim a As Reflection.Assembly = Reflection.Assembly.Load(assemblyString)

                            Dim baseName As String = "Microsoft.VisualBasic.debug"
                            m_DebugResourceManager = New System.Resources.ResourceManager(baseName, a)
                        Catch ex As StackOverflowException
                            Throw ex
                        Catch ex As OutOfMemoryException
                            Throw ex
                        Catch ex As System.Threading.ThreadAbortException
                            Throw ex
                        Catch
                        End Try
                        m_TriedLoadingDebugResourceManager = True
                    End If
                End SyncLock

                Return m_DebugResourceManager
            End Get
        End Property


        Friend Shared ReadOnly Property FallbackResourceManager() As System.Resources.ResourceManager
            Get

                If Not m_FallbackResourceManager Is Nothing Then
                    Return m_FallbackResourceManager
                End If

                SyncLock FallbackResourceManagerSyncObj
                    If Not m_TriedLoadingFallbackResourceManager Then
                        Try
                            m_FallbackResourceManager = New System.Resources.ResourceManager("mscorlib", GetType(Object).Assembly)
                        Catch ex As StackOverflowException
                            Throw ex
                        Catch ex As OutOfMemoryException
                            Throw ex
                        Catch ex As System.Threading.ThreadAbortException
                            Throw ex
                        Catch
                        End Try
                        m_TriedLoadingFallbackResourceManager = True
                    End If
                End SyncLock

                Return m_FallbackResourceManager
            End Get
        End Property

        'max allowed length of arguments when preparing Uri
        'Same value as in System and System.Core
        Private Const trimsize As Integer = 1024
        Private Shared Function GetFallbackMessage(ByVal name As String, ByVal ParamArray args() As Object) As String
            Dim result As String = Nothing
            Dim curCultureInfo As Globalization.CultureInfo = GetCultureInfo()

            If FallbackResourceManager IsNot Nothing Then
                Dim fallbackStr As String
                fallbackStr = FallbackResourceManager.GetString("NoDebugResources", Nothing)

                If fallbackStr IsNot Nothing Then
                    ' build up arg string
                    Dim sb As New Text.StringBuilder()
                    If args IsNot Nothing Then
                        For i As Integer = 0 To args.Length - 1
                            Dim value As String = TryCast(args(i), String)
                            If value IsNot Nothing Then
                                If value.Length <= trimsize Then
                                    sb.Append(value)
                                Else
                                    sb.Append(value.Substring(0, trimsize - 3) + "...")
                                End If
                                If i < args.Length - 1 Then
                                    sb.Append(curCultureInfo.TextInfo.ListSeparator)
                                End If
                            End If
                        Next
                    End If
                    Dim argStr As String = sb.ToString()
                    If argStr Is Nothing Then
                        argStr = ""
                    End If
                    result = String.Format(curCultureInfo, fallbackStr, name, argStr, GetAssemblyFileVersion(), "Microsoft.VisualBasic.dll", UriEncode(name))
                End If
            End If

            'last-ditch effort; just give back name
            If result Is Nothing Then
                result = name
            End If
            Return result
        End Function

        Private Shared Function GetAssemblyFileVersion() As String
            Dim attributes As Object() = VBRuntimeAssembly.GetCustomAttributes(GetType(Reflection.AssemblyFileVersionAttribute), False)
            If attributes.Length <> 1 Then
                Return ""
            End If
            Dim fileVersionAttribute As Reflection.AssemblyFileVersionAttribute = TryCast(attributes(0), Reflection.AssemblyFileVersionAttribute)
            If fileVersionAttribute Is Nothing Then
                Return ""
            End If
            Return fileVersionAttribute.Version
        End Function

        Private Shared Function UriEncode(ByVal url As String) As String
            If url Is Nothing Then
                Return Nothing
            End If

            Dim bytes As Byte() = System.Text.Encoding.UTF8.GetBytes(url)
            Dim cSpaces As Integer = 0
            Dim cUnsafe As Integer = 0
            Dim count As Integer = bytes.Length


            ' count them first
            For i As Integer = 0 To count - 1
                Dim ch As Char = ChrW(bytes(i))

                If ch = " "c Then
                    cSpaces += 1
                ElseIf Not IsSafe(ch) Then
                    cUnsafe += 1
                End If
            Next
            ' nothing to expand?
            Dim skipExpand As Boolean = (cSpaces = 0 AndAlso cUnsafe = 0)

            If Not skipExpand Then
                ' expand not 'safe' characters into %XX, spaces to +s
                Dim expandedBytes(count + cUnsafe * 2) As Byte
                Dim pos As Integer = 0

                For i As Integer = 0 To count - 1
                    Dim b As Byte = bytes(i)
                    Dim ch As Char = ChrW(b)

                    If IsSafe(ch) Then
                        expandedBytes(pos) = b
                        pos += 1
                    ElseIf ch = " "c Then
                        expandedBytes(pos) = AscW("+"c)
                        pos += 1
                    Else
                        expandedBytes(pos) = AscW("%"c)
                        pos += 1
                        expandedBytes(pos) = CByte(AscW(IntToHex((b >> 4) And &HF)))
                        pos += 1
                        expandedBytes(pos) = CByte(AscW(IntToHex(b And &HF)))
                        pos += 1
                    End If
                Next
                bytes = expandedBytes
            End If

            Return Text.Encoding.UTF8.GetString(bytes, 0, bytes.Length)
        End Function

        Private Shared Function IntToHex(ByVal n As Integer) As Char
            System.Diagnostics.Debug.Assert(n < &H10)

            If n <= 9 Then
                Return ChrW(n + AscW("0"c))
            Else
                Return ChrW(n - 10 + AscW("a"c))
            End If
        End Function


        ' Set of safe chars, from RFC 1738.4 minus '+'
        Private Shared Function IsSafe(ByVal ch As Char) As Boolean
            If ch >= "a"c AndAlso ch <= "z"c OrElse ch >= "A"c AndAlso ch <= "Z"c OrElse ch >= "0"c AndAlso ch <= "9"c Then
                Return True
            End If

            Select Case ch
                Case "-"c, "_"c, "."c, "!"c, "*"c, "\"c, "("c, ")"c
                    Return True
            End Select

            Return False
        End Function

        Friend Shared Function GetResourceString(ByVal ResourceId As vbErrors) As String
            Return GetResourceString("ID" & CStr(ResourceId))
        End Function


#If TELESTO Then
        'FIXME: <System.ComponentModel.EditorBrowsable(ComponentModel.EditorBrowsableState.Never)> 
        Friend Shared Function GetResourceString(ByVal ResourceKey As String) As String
#Else
        <System.ComponentModel.EditorBrowsable(ComponentModel.EditorBrowsableState.Never)> _
        Friend Shared Function GetResourceString(ByVal ResourceKey As String) As String
#End If

            Dim s As String = Nothing

            Try
                If VBAResourceManager IsNot Nothing Then
                    s = VBAResourceManager.GetString(ResourceKey, Nothing)
                End If
                If s Is Nothing And DebugResourceManager IsNot Nothing Then
                    s = DebugResourceManager.GetString(ResourceKey, Nothing)
                End If
                ' this may be unknown error, so try getting default message
                If s Is Nothing And DebugResourceManager IsNot Nothing Then
                    s = DebugResourceManager.GetString(VBDefaultErrorID)
                End If

                'if we have found nothing, most likely the debug resources are missing.
                'get a fallback message.
                If s Is Nothing Then
                    s = GetFallbackMessage(ResourceKey)
                End If
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                s = ResourceMsgDefault
            End Try

            Return s
        End Function

        Friend Shared Function GetResourceString(ByVal ResourceKey As String, ByVal NotUsed As Boolean) As String
            'This version does NOT return a default message if not found.
            Dim s As String = Nothing

            Try
                If VBAResourceManager IsNot Nothing Then
                    s = VBAResourceManager.GetString(ResourceKey, Nothing)
                End If
                If s Is Nothing And DebugResourceManager IsNot Nothing Then
                    s = DebugResourceManager.GetString(ResourceKey, Nothing)
                End If
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                s = Nothing
            End Try
            Return s
        End Function

        '*****************************************************************************
        ';GetResourceString
        '
        'Summary: Retrieves a resource string and formats it by replacing placeholders
        '         with params. For example if the unformatted string is
        '         "Hello, {0}" then GetString("StringID", "World") will return "Hello, World"
        '         This one is exposed because I have to be able to get at localized error
        '         strings from the MY template
        '  Param: ID - Identifier for the string to be retrieved
        '  Param: Args - An array of params used to replace placeholders. 
        'Returns: The resource string if found or an error message string
        '*****************************************************************************
        Public Shared Function GetResourceString(ByVal ResourceKey As String, ByVal ParamArray Args() As String) As String

            Debug.Assert(Not ResourceKey = "", "ResourceKey is missing")
            Debug.Assert(Not Args Is Nothing, "No Args")

            Dim UnformattedString As String = Nothing
            Dim FormattedString As String = Nothing
            Try
                If VBAResourceManager IsNot Nothing Then
                    UnformattedString = VBAResourceManager.GetString(ResourceKey, Nothing)
                End If
                If UnformattedString Is Nothing And DebugResourceManager IsNot Nothing Then
                    UnformattedString = DebugResourceManager.GetString(ResourceKey, Nothing)
                End If

                'if we have found nothing, most likely the debug resources are missing.
                'get a fallback message.
                If UnformattedString Is Nothing Then
                    UnformattedString = GetFallbackMessage(ResourceKey, Args)
                Else
                    'Replace plceholders with items from the passed in array
                    FormattedString = String.Format(GetCultureInfo(), UnformattedString, Args)
                End If

                'Rethrow hosting exceptions
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch ex As Exception
            End Try

            'Return the string if we have one otherwise return a default error message
            If FormattedString IsNot Nothing Then
                Return FormattedString
            ElseIf UnformattedString IsNot Nothing Then
                Return UnformattedString
            Else
                Return ResourceKey
            End If
        End Function
#If Not LATEBINDING Then
        ' *** VB6 COMMENTS FOR STDFORMAT FUNCTION ***
        ' writing "standard format".  We must use '.' for decimal and we must not
        ' have a leading zero.  First, replace the system decimal with a period.
        ' second.   Strip the leading zero if one exists.  This is post-processing
        ' work to deal with standard OLE functionality where all variant conversions
        ' are based on the system LCID but where Str$()/Write# is supposed to always
        ' use a fixed format.

        Friend Shared Function StdFormat(ByVal s As String) As String
            Dim nfi As NumberFormatInfo
            Dim iIndex As Integer
            Dim c0, c1, c2 As Char
            Dim sb As StringBuilder

            nfi = Threading.Thread.CurrentThread.CurrentCulture.NumberFormat
            iIndex = s.IndexOf(nfi.NumberDecimalSeparator)

            If iIndex = -1 Then
                Return s
            End If

            Try
                c0 = s.Chars(0)
                c1 = s.Chars(1)
                c2 = s.Chars(2)
            Catch ex As StackOverflowException
                Throw ex
            Catch ex As OutOfMemoryException
                Throw ex
            Catch ex As System.Threading.ThreadAbortException
                Throw ex
            Catch
                'Ignore, should default to 0 values
            End Try

            If s.Chars(iIndex) = chPeriod Then
                'Optimization: no period replacement needed
                'avoids creating stringbuilder and copying string 

                'If format is "0.xxxx" then replace 0 with space 
                If c0 = chZero AndAlso c1 = chPeriod Then
                    Return s.Substring(1)

                    'If format is "-0.xxxx", "+0.xxxx", " 0.xxxx" then shift everything down over the zero
                ElseIf (c0 = chHyphen OrElse c0 = chPlus OrElse c0 = chSpace) AndAlso c1 = chZero AndAlso c2 = chPeriod Then
                    'Fall down below and use a stringbuilder
                Else
                    'No change
                    Return s
                End If
            End If

            sb = New StringBuilder(s)
            sb.Chars(iIndex) = chPeriod ' change decimal separator to "."

            'If format is "0.xxxx" then replace 0 with space 
            If (c0 = chZero AndAlso c1 = chPeriod) Then
                StdFormat = sb.ToString(1, sb.Length - 1)
                'If format is "-0.xxxx", "+0.xxxx", " 0.xxxx" then shift everything down over the zero
            ElseIf (c0 = chHyphen OrElse c0 = chPlus OrElse c0 = chSpace) AndAlso c1 = chZero AndAlso c2 = chPeriod Then
                sb.Remove(1, 1)
                StdFormat = sb.ToString()
            Else
                StdFormat = sb.ToString()
            End If
        End Function
#If Not TELESTO Then
        Friend Shared Function OctFromLong(ByVal Val As Long) As String
            'System.Radix is being removed from the .NET platform, so compute this locally.
            Dim Buffer As String = ""
            Dim ModVal As Integer
            Dim CharZero As Integer = Convert.ToInt32(chZero)
            Dim Negative As Boolean

            If Val < 0 Then
                Val = Int64.MaxValue + Val + 1
                Negative = True
            End If

            'Pull apart the number and put the digits (in reverse order) into the buffer.
            Do
                ModVal = CInt(Val Mod 8)
                Val = Val >> 3
                Buffer = Buffer & ChrW(ModVal + CharZero)
            Loop While Val > 0

            Buffer = StrReverse(Buffer)

            If Negative Then
                Buffer = "1" & Buffer
            End If

            Return Buffer
        End Function

        Friend Shared Function OctFromULong(ByVal Val As ULong) As String
            'System.Radix is being removed from the .NET platform, so compute this locally.
            Dim Buffer As String = ""
            Dim ModVal As Integer
            Dim CharZero As Integer = Convert.ToInt32(chZero)

            'Pull apart the number and put the digits (in reverse order) into the buffer.
            Do
                ModVal = CInt(Val Mod 8UL)
                Val = Val >> 3
                Buffer = Buffer & ChrW(ModVal + CharZero)
            Loop While Val <> 0UL

            Buffer = StrReverse(Buffer)

            Return Buffer
        End Function
#End If 'not TELESTO

#If Not TELESTO Then 'In TELESTO we don't allow them to set the time or date
        '*** SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK ***
        <SecuritySafeCritical()> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode), _
            Diagnostics.DebuggerHiddenAttribute()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Friend Shared Sub SetTime(ByVal dtTime As DateTime)
            Dim systime As New NativeTypes.SystemTime

            SafeNativeMethods.GetLocalTime(systime)

            systime.wHour = CShort(dtTime.Hour)
            systime.wMinute = CShort(dtTime.Minute)
            systime.wSecond = CShort(dtTime.Second)
            systime.wMilliseconds = CShort(dtTime.Millisecond)

            If UnsafeNativeMethods.SetLocalTime(systime) = 0 Then
                If Marshal.GetLastWin32Error() = ERROR_INVALID_PARAMETER Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue))
                Else
                    Throw New SecurityException(GetResourceString(ResID.SetLocalTimeFailure))
                End If
            End If

        End Sub

        '*** SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK - SECURITY CHECK ***
        <SecuritySafeCritical()> _
        <SecurityPermissionAttribute(SecurityAction.Demand, Flags:=SecurityPermissionFlag.UnmanagedCode), _
            Diagnostics.DebuggerHiddenAttribute()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Friend Shared Sub SetDate(ByVal vDate As DateTime)
            Dim systime As New NativeTypes.SystemTime

            SafeNativeMethods.GetLocalTime(systime)

            systime.wYear = CShort(vDate.Year)
            systime.wMonth = CShort(vDate.Month)
            systime.wDay = CShort(vDate.Day)

            If UnsafeNativeMethods.SetLocalTime(systime) = 0 Then
                If Marshal.GetLastWin32Error() = ERROR_INVALID_PARAMETER Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue))
                Else
                    Throw New SecurityException(GetResourceString(ResID.SetLocalDateFailure))
                End If
            End If

        End Sub
#End If
        Friend Shared Function GetDateTimeFormatInfo() As DateTimeFormatInfo
            Return System.Threading.Thread.CurrentThread.CurrentCulture.DateTimeFormat
        End Function

        Public Shared Sub ThrowException(ByVal hr As Integer)
            Throw VbMakeException(hr)
        End Sub

#If Not TELESTO Then
        Friend Shared Function MapHRESULT(ByVal lNumber As Integer) As Integer
            If lNumber > 0 Then
                Return lNumber
            End If

            If (lNumber And SCODE_FACILITY) = FACILITY_CONTROL Then
                Return (lNumber And &HFFFFI)
            End If

            Select Case lNumber

                ' FACILITY_NULL errors
                Case E_NOTIMPL
                    MapHRESULT = vbErrors.NotYetImplemented

                Case E_NOINTERFACE
                    MapHRESULT = vbErrors.OLENotSupported

                Case E_ABORT
                    MapHRESULT = vbErrors.Abort

                    ' FACILITY_DISPATCH - IDispatch errors.
                Case DISP_E_UNKNOWNINTERFACE
                    MapHRESULT = vbErrors.OLENoPropOrMethod
                Case DISP_E_MEMBERNOTFOUND
                    MapHRESULT = vbErrors.OLENoPropOrMethod
                Case DISP_E_PARAMNOTFOUND
                    MapHRESULT = vbErrors.NamedParamNotFound
                Case DISP_E_TYPEMISMATCH
                    MapHRESULT = vbErrors.TypeMismatch
                Case DISP_E_UNKNOWNNAME
                    MapHRESULT = vbErrors.OLENoPropOrMethod
                Case DISP_E_NONAMEDARGS
                    MapHRESULT = vbErrors.NamedArgsNotSupported
                Case DISP_E_BADVARTYPE
                    MapHRESULT = vbErrors.InvalidTypeLibVariable
                Case DISP_E_OVERFLOW
                    MapHRESULT = vbErrors.Overflow
                Case DISP_E_BADINDEX
                    MapHRESULT = vbErrors.OutOfBounds
                Case DISP_E_UNKNOWNLCID
                    MapHRESULT = vbErrors.LocaleSettingNotSupported
                Case DISP_E_ARRAYISLOCKED
                    MapHRESULT = vbErrors.ArrayLocked
                Case DISP_E_BADPARAMCOUNT
                    MapHRESULT = vbErrors.FuncArityMismatch
                Case DISP_E_PARAMNOTOPTIONAL
                    MapHRESULT = vbErrors.ParameterNotOptional
                Case DISP_E_NOTACOLLECTION
                    MapHRESULT = vbErrors.NotEnum
                Case DISP_E_DIVBYZERO
                    MapHRESULT = vbErrors.DivByZero
                    ' FACILITY_DISPATCH - Typelib errors.
                Case TYPE_E_BUFFERTOOSMALL
                    MapHRESULT = vbErrors.BufferTooSmall
                Case &H80028017I
                    MapHRESULT = vbErrors.IdentNotMember
                Case TYPE_E_INVDATAREAD
                    MapHRESULT = vbErrors.InvDataRead
                Case TYPE_E_UNSUPFORMAT
                    MapHRESULT = vbErrors.UnsupFormat
                Case TYPE_E_REGISTRYACCESS
                    MapHRESULT = vbErrors.RegistryAccess
                Case TYPE_E_LIBNOTREGISTERED
                    MapHRESULT = vbErrors.LibNotRegistered
                Case TYPE_E_UNDEFINEDTYPE
                    MapHRESULT = vbErrors.UndefinedType
                Case TYPE_E_QUALIFIEDNAMEDISALLOWED
                    MapHRESULT = vbErrors.QualifiedNameDisallowed
                Case TYPE_E_INVALIDSTATE
                    MapHRESULT = vbErrors.InvalidState
                Case TYPE_E_WRONGTYPEKIND
                    MapHRESULT = vbErrors.WrongTypeKind
                Case TYPE_E_ELEMENTNOTFOUND
                    MapHRESULT = vbErrors.ElementNotFound
                Case TYPE_E_AMBIGUOUSNAME
                    MapHRESULT = vbErrors.AmbiguousName
                Case TYPE_E_NAMECONFLICT
                    MapHRESULT = vbErrors.ModNameConflict
                Case TYPE_E_UNKNOWNLCID
                    MapHRESULT = vbErrors.UnknownLcid
                Case TYPE_E_DLLFUNCTIONNOTFOUND
                    MapHRESULT = vbErrors.InvalidDllFunctionName
                Case TYPE_E_BADMODULEKIND
                    MapHRESULT = vbErrors.BadModuleKind
                Case TYPE_E_SIZETOOBIG
                    MapHRESULT = vbErrors.SizeTooBig
                Case TYPE_E_TYPEMISMATCH
                    MapHRESULT = vbErrors.TypeMismatch
                Case TYPE_E_OUTOFBOUNDS
                    MapHRESULT = vbErrors.OutOfBounds
                Case TYPE_E_IOERROR
                    MapHRESULT = vbErrors.IOError
                Case TYPE_E_CANTCREATETMPFILE
                    MapHRESULT = vbErrors.CantCreateTmpFile
                Case TYPE_E_CANTLOADLIBRARY
                    MapHRESULT = vbErrors.DLLLoadErr
                Case TYPE_E_INCONSISTENTPROPFUNCS
                    MapHRESULT = vbErrors.InconsistentPropFuncs
                Case TYPE_E_CIRCULARTYPE
                    MapHRESULT = vbErrors.CircularType

                    ' FACILITY_STORAGE errors
                Case STG_E_INVALIDFUNCTION
                    MapHRESULT = vbErrors.BadFunctionId
                Case STG_E_FILENOTFOUND
                    MapHRESULT = vbErrors.FileNotFound
                Case STG_E_PATHNOTFOUND
                    MapHRESULT = vbErrors.PathNotFound
                Case STG_E_TOOMANYOPENFILES
                    MapHRESULT = vbErrors.TooManyFiles
                Case STG_E_ACCESSDENIED
                    MapHRESULT = vbErrors.PermissionDenied
                Case STG_E_INVALIDHANDLE
                    MapHRESULT = vbErrors.ReadFault
                Case STG_E_INSUFFICIENTMEMORY
                    MapHRESULT = vbErrors.OutOfMemory
                Case STG_E_NOMOREFILES
                    MapHRESULT = vbErrors.TooManyFiles
                Case STG_E_DISKISWRITEPROTECTED
                    MapHRESULT = vbErrors.PermissionDenied
                Case STG_E_SEEKERROR
                    MapHRESULT = vbErrors.SeekErr
                Case STG_E_WRITEFAULT
                    MapHRESULT = vbErrors.WriteFault
                Case STG_E_READFAULT
                    MapHRESULT = vbErrors.ReadFault
                Case STG_E_SHAREVIOLATION
                    MapHRESULT = vbErrors.PathFileAccess
                Case STG_E_LOCKVIOLATION
                    MapHRESULT = vbErrors.PermissionDenied
                Case STG_E_FILEALREADYEXISTS
                    MapHRESULT = vbErrors.FileAlreadyExists
                Case STG_E_MEDIUMFULL
                    MapHRESULT = vbErrors.DiskFull
                Case STG_E_INVALIDHEADER
                    MapHRESULT = vbErrors.InvDataRead
                Case STG_E_INVALIDNAME
                    MapHRESULT = vbErrors.FileNotFound
                Case STG_E_UNKNOWN
                    MapHRESULT = vbErrors.InvDataRead
                Case STG_E_UNIMPLEMENTEDFUNCTION
                    MapHRESULT = vbErrors.NotYetImplemented
                Case STG_E_INUSE
                    MapHRESULT = vbErrors.PermissionDenied
                Case STG_E_NOTCURRENT
                    MapHRESULT = vbErrors.PermissionDenied
                Case STG_E_REVERTED
                    MapHRESULT = vbErrors.WriteFault
                Case STG_E_CANTSAVE
                    MapHRESULT = vbErrors.IOError
                Case STG_E_OLDFORMAT
                    MapHRESULT = vbErrors.UnsupFormat
                Case STG_E_OLDDLL
                    MapHRESULT = vbErrors.UnsupFormat
                Case STG_E_SHAREREQUIRED
                    MapHRESULT = vbErrors.ShareRequired
                Case STG_E_NOTFILEBASEDSTORAGE
                    MapHRESULT = vbErrors.UnsupFormat
                Case STG_E_EXTANTMARSHALLINGS
                    MapHRESULT = vbErrors.UnsupFormat

                    ' FACILITY_ITF errors.
                Case CLASS_E_NOTLICENSED
                    MapHRESULT = vbErrors.CantCreateObject
                Case REGDB_E_CLASSNOTREG
                    MapHRESULT = vbErrors.CantCreateObject
                Case MK_E_UNAVAILABLE
                    MapHRESULT = vbErrors.CantCreateObject
                Case MK_E_INVALIDEXTENSION
                    MapHRESULT = vbErrors.OLEFileNotFound
                Case MK_E_CANTOPENFILE
                    MapHRESULT = vbErrors.OLEFileNotFound
                Case CO_E_CLASSSTRING
                    MapHRESULT = vbErrors.CantCreateObject
                Case CO_E_APPNOTFOUND
                    MapHRESULT = vbErrors.CantCreateObject
                Case CO_E_APPDIDNTREG
                    MapHRESULT = vbErrors.CantCreateObject

                    ' FACILITY_WIN32 errors
                Case E_ACCESSDENIED
                    MapHRESULT = vbErrors.PermissionDenied
                Case E_OUTOFMEMORY
                    MapHRESULT = vbErrors.OutOfMemory
                Case E_INVALIDARG
                    MapHRESULT = vbErrors.IllegalFuncCall
                Case &H800706BAI
                    MapHRESULT = vbErrors.ServerNotFound

                    ' FACILITY_WINDOWS - I don't know why this differs from FACILITY_WIN32
                Case CO_E_SERVER_EXEC_FAILURE
                    MapHRESULT = vbErrors.CantCreateObject

                Case Else

                    MapHRESULT = lNumber

            End Select

        End Function
#End If 'not TELESTO
#End If
        Friend Shared Function GetCultureInfo() As CultureInfo
            Return System.Threading.Thread.CurrentThread.CurrentCulture
        End Function

#If Not TELESTO Then
        <HostProtection(Resources:=HostProtectionResource.SelfAffectingThreading)> _
        Public Shared Function SetCultureInfo(ByVal Culture As CultureInfo) As System.Object
            Dim PreviousCulture As CultureInfo = System.Threading.Thread.CurrentThread.CurrentCulture
            System.Threading.Thread.CurrentThread.CurrentCulture = Culture
            Return PreviousCulture
        End Function
#End If
        Friend Shared Function GetInvariantCultureInfo() As CultureInfo
            Return CultureInfo.InvariantCulture
        End Function

        Friend Shared ReadOnly Property VBRuntimeAssembly() As System.Reflection.Assembly
            Get
                If Not m_VBRuntimeAssembly Is Nothing Then
                    Return m_VBRuntimeAssembly
                End If

                ' if the cached assembly ref has not been set, then set it here
                m_VBRuntimeAssembly = System.Reflection.Assembly.GetExecutingAssembly()
                Return m_VBRuntimeAssembly
            End Get
        End Property
#If Not LATEBINDING Then
        'Helper that gets called for Redim
        Public Shared Function CopyArray(ByVal arySrc As System.Array, ByVal aryDest As System.Array) As System.Array

            If arySrc Is Nothing Then
                Return aryDest
            End If

            Dim lLength As Integer

            lLength = arySrc.Length
            If lLength = 0 Then
                Return aryDest
            End If

            If aryDest.Rank() <> arySrc.Rank() Then
                Throw VbMakeException(New InvalidCastException(GetResourceString(ResID.Array_RankMismatch)), vbErrors.OutOfBounds)
            End If

            'Validate the upper has not changed
            Dim iDim As Integer
            For iDim = 0 To aryDest.Rank() - 2 'Do not check last dimension
                If aryDest.GetUpperBound(iDim) <> arySrc.GetUpperBound(iDim) Then
                    Throw VbMakeException(New ArrayTypeMismatchException(GetResourceString(ResID.Array_TypeMismatch)), vbErrors.OutOfBounds)
                End If
            Next iDim

            If lLength > aryDest.Length Then
                lLength = aryDest.Length
            End If

            'if this is multi-dimensional, we have to do our own copy
            'REVIEW VSW#395788:  the BCL should have a member that does this for us
            If arySrc.Rank > 1 Then

                Dim LastRank As Integer = arySrc.Rank
                Dim lenSrcLastRank As Integer = arySrc.GetLength(LastRank - 1)
                Dim lenDestLastRank As Integer = aryDest.GetLength(LastRank - 1)

                'if the last rank has 0 size, then this array has no elements, so just return
                If lenDestLastRank = 0 Then
                    Return aryDest
                End If

                'get the correct copy length, regardless if the user increased or decreased the last rank's size
                Dim lenCopy As Integer = System.Math.Min(lenSrcLastRank, lenDestLastRank)

                Dim i As Integer
                'split the source array into chunks the size of the last rank and copy each chunk one-by-one
                For i = 0 To (arySrc.Length \ lenSrcLastRank) - 1
                    System.Array.Copy(arySrc, i * lenSrcLastRank, aryDest, i * lenDestLastRank, lenCopy)
                Next i

            Else
                System.Array.Copy(arySrc, aryDest, lLength)
            End If

            Return aryDest

        End Function
#End If
        Friend Shared Function ToHalfwidthNumbers(ByVal s As String, ByVal culture As CultureInfo) As String

#If TELESTO Then 'Telesto doesn't have OS support for doing this mapping
            Return s
#Else
            Const LANG_CHINESE As Integer = &H4I
            Const LANG_JAPANESE As Integer = &H11I
            Const LANG_KOREAN As Integer = &H12I

            Dim lcid As Integer = culture.LCID
            Dim langid As Integer = (lcid And &H3FF)

            If langid <> LANG_CHINESE AndAlso langid <> LANG_JAPANESE AndAlso langid <> LANG_KOREAN Then
                Return s
            End If

            Return vbLCMapString( _
                culture, _
                NativeTypes.LCMAP_HALFWIDTH, _
                s)
#End If

#If 0 Then
            'Keep this around for a while
            'The above code is compatible with VB6, but to be more
            'unicode aware, all languages should support fullwidth numbers &HFF10 - &HFF19
            'The problem arises when fullwidth decimal and other symbols are used
            'we need to understand what rules should apply when converting these to
            'halfwidth values
            For i = 0 To s.Length - 1
                ch = s.Chars(i)
                If Convert.ToInt32(ch) > 255 Then
                    If Char.IsDigit(ch) Then
                        If sb Is Nothing Then
                            sb = New Text.StringBuilder(s)
                        End If
                        sb.Chars(i) = Convert.ToChar(CShort(Char.GetNumericValue(ch) + &h30))
                    ElseIf ch = ChrW(&HFF0E) Then
                        If sb Is Nothing Then
                            sb = New Text.StringBuilder(s)
                        End If
                        sb.Chars(i) = "."c
                    End If
                End If

            Next i
            If sb Is Nothing Then
                Return s
            End If
            Return sb.ToString()
#End If
        End Function

        'CONSIDER: Seems odd that this function would throw exceptions when the name suggests that
        'no exceptions will be thrown in failure cases.
        Friend Shared Function IsHexOrOctValue(ByVal Value As String, ByRef i64Value As Int64) As Boolean

            Dim ch As Char
            Dim Length As Integer
            Dim FirstNonspace As Integer
            Dim TmpValue As String

            Length = Value.Length

            Do While (FirstNonspace < Length)
                ch = Value.Chars(FirstNonspace)
                'We check that the length is at least FirstNonspace + 2 because otherwise the function
                'will throw undesired exceptions.
                If ch = "&"c AndAlso FirstNonspace + 2 < Length Then
                    GoTo GetSpecialValue
                End If
                If ch <> chSpace AndAlso ch <> chIntlSpace Then
                    Return False
                End If
                FirstNonspace += 1
            Loop

            Return False

GetSpecialValue:
            ch = System.Char.ToLower(Value.Chars(FirstNonspace + 1), CultureInfo.InvariantCulture)

            TmpValue = ToHalfwidthNumbers(Value.Substring(FirstNonspace + 2), GetCultureInfo())
            If ch = "h"c Then
                i64Value = System.Convert.ToInt64(TmpValue, 16)
            ElseIf ch = "o"c Then
                i64Value = System.Convert.ToInt64(TmpValue, 8)
            Else
                Throw New FormatException
            End If
            Return True
        End Function

        'CONSIDER: Seems odd that this function would throw exceptions when the name suggests that
        'no exceptions will be thrown in failure cases.
        Friend Shared Function IsHexOrOctValue(ByVal Value As String, ByRef ui64Value As UInt64) As Boolean

            Dim ch As Char
            Dim Length As Integer
            Dim FirstNonspace As Integer
            Dim TmpValue As String

            Length = Value.Length

            Do While (FirstNonspace < Length)
                ch = Value.Chars(FirstNonspace)
                'We check that the length is at least FirstNonspace + 2 because otherwise the function
                'will throw undesired exceptions.
                If ch = "&"c AndAlso FirstNonspace + 2 < Length Then
                    GoTo GetSpecialValue
                End If
                If ch <> chSpace AndAlso ch <> chIntlSpace Then
                    Return False
                End If
                FirstNonspace += 1
            Loop

            Return False

GetSpecialValue:
            ch = System.Char.ToLower(Value.Chars(FirstNonspace + 1), CultureInfo.InvariantCulture)

            TmpValue = ToHalfwidthNumbers(Value.Substring(FirstNonspace + 2), GetCultureInfo())
            If ch = "h"c Then
                ui64Value = System.Convert.ToUInt64(TmpValue, 16)
            ElseIf ch = "o"c Then
                ui64Value = System.Convert.ToUInt64(TmpValue, 8)
            Else
                Throw New FormatException
            End If
            Return True
        End Function


        Friend Shared Function VBFriendlyName(ByVal Obj As Object) As String
            If Obj Is Nothing Then
                Return "Nothing"
            End If

            Return VBFriendlyName(Obj.GetType, Obj)
        End Function

        Friend Shared Function VBFriendlyName(ByVal typ As System.Type) As String
            Return VBFriendlyNameOfType(typ)
        End Function

        Friend Shared Function VBFriendlyName(ByVal typ As System.Type, ByVal o As Object) As String
#If Not TELESTO Then 'No COM in Telesto
            If typ.IsCOMObject AndAlso (typ.FullName = "System.__ComObject") Then
                Return TypeNameOfCOMObject(o, False)
            End If
#End If
            Return VBFriendlyNameOfType(typ)
        End Function

        Friend Shared Function VBFriendlyNameOfType(ByVal typ As System.Type, Optional ByVal FullName As Boolean = False) As String

            Dim Result As String
            Dim ArraySuffix As String

            ArraySuffix = GetArraySuffixAndElementType(typ)

            Debug.Assert(typ IsNot Nothing AndAlso Not typ.IsArray, "Error in array type processing!!!")


            Dim tc As TypeCode
            If typ.IsEnum Then
                tc = TypeCode.Object
            Else
                tc = Type.GetTypeCode(typ)
            End If

            Select Case tc

                Case TypeCode.Boolean : Result = "Boolean"
                Case TypeCode.SByte : Result = "SByte"
                Case TypeCode.Byte : Result = "Byte"
                Case TypeCode.Int16 : Result = "Short"
                Case TypeCode.UInt16 : Result = "UShort"
                Case TypeCode.Int32 : Result = "Integer"
                Case TypeCode.UInt32 : Result = "UInteger"
                Case TypeCode.Int64 : Result = "Long"
                Case TypeCode.UInt64 : Result = "ULong"
                Case TypeCode.Decimal : Result = "Decimal"
                Case TypeCode.Single : Result = "Single"
                Case TypeCode.Double : Result = "Double"
                Case TypeCode.DateTime : Result = "Date"
                Case TypeCode.Char : Result = "Char"
                Case TypeCode.String : Result = "String"
                Case TypeCode.DBNull : Result = "DBNull"

                Case Else

                    If IsGenericParameter(typ) Then
                        Result = typ.Name
                        Exit Select
                    End If

                    Dim Qualifier As String = Nothing 'yes, defaults to nothing but makes a warning go away about use before assignment
                    Dim Name As String

                    Dim GenericArgsSuffix As String = GetGenericArgsSuffix(typ)

                    If FullName Then
                        ' WORK AROUND: System.Type.IsNested is not available in Silverlight CLR.
                        If typ.DeclaringType IsNot Nothing Then
                            Qualifier = VBFriendlyNameOfType(typ.DeclaringType, FullName:=True)
                            Name = typ.Name
                        Else
                            Name = typ.FullName
                            ' Some types do not have FullName
                            If Name Is Nothing Then
                                Name = typ.Name
                            End If
                        End If
                    Else
                        Name = typ.Name
                    End If

                    If GenericArgsSuffix IsNot Nothing Then
                        Dim ManglingCharIndex As Integer = Name.LastIndexOf(chGenericManglingChar)

                        If ManglingCharIndex <> -1 Then
                            Name = Name.Substring(0, ManglingCharIndex)
                        End If

                        Result = Name & GenericArgsSuffix
                    Else
                        Result = Name
                    End If

                    If Qualifier IsNot Nothing Then
                        Result = Qualifier & chPeriod & Result
                    End If

            End Select


            If ArraySuffix IsNot Nothing Then
                Result = Result & ArraySuffix
            End If

            Return Result
        End Function

        Private Shared Function GetArraySuffixAndElementType(ByRef typ As Type) As String

            If Not typ.IsArray Then
                Return Nothing
            End If

            Dim ArraySuffix As New Text.StringBuilder

            'Notice the reversing - VB array notation is reverse of clr array notation
            'i.e. (,)() in VB is [][,] in clr
            '
            Do

                ArraySuffix.Append("(")
                ArraySuffix.Append(","c, typ.GetArrayRank() - 1)
                ArraySuffix.Append(")")

                typ = typ.GetElementType

            Loop While typ.IsArray

            Return ArraySuffix.ToString()
        End Function

        Private Shared Function GetGenericArgsSuffix(ByVal typ As Type) As String

            If Not typ.IsGenericType Then
                Return Nothing
            End If

            Dim TypeArgs As Type() = typ.GetGenericArguments
            Dim TotalTypeArgsCount As Integer = TypeArgs.Length
            Dim TypeArgsCount As Integer = TotalTypeArgsCount

            ' WORK AROUND: System.Type.IsNested is not available in Silverlight CLR.
            If typ.DeclaringType IsNot Nothing AndAlso typ.DeclaringType.IsGenericType Then
                TypeArgsCount = TypeArgsCount - typ.DeclaringType.GetGenericArguments().Length
            End If

            If TypeArgsCount = 0 Then
                Return Nothing
            End If

            Dim GenericArgsSuffix As New Text.StringBuilder
            GenericArgsSuffix.Append("(Of ")

            For i As Integer = TotalTypeArgsCount - TypeArgsCount To TotalTypeArgsCount - 1

                GenericArgsSuffix.Append(VBFriendlyNameOfType(TypeArgs(i)))

                If i <> TotalTypeArgsCount - 1 Then
                    GenericArgsSuffix.Append(","c)
                End If
            Next

            GenericArgsSuffix.Append(")")

            Return GenericArgsSuffix.ToString
        End Function

        Friend Shared Function ParameterToString(ByVal Parameter As ParameterInfo) As String

            Dim ResultString As String = ""
            Dim ParameterType As Type = Parameter.ParameterType

            If Parameter.IsOptional Then
                ResultString &= "["
            End If

            If ParameterType.IsByRef Then
                ResultString &= "ByRef "
                ParameterType = ParameterType.GetElementType
            ElseIf IsParamArray(Parameter) Then
                ResultString &= "ParamArray "
            End If

            ResultString &= Parameter.Name & " As " & VBFriendlyNameOfType(ParameterType, FullName:=True)

            If Parameter.IsOptional Then

                Dim DefaultValue As Object = Parameter.DefaultValue

                If DefaultValue Is Nothing Then
                    ResultString &= " = Nothing"
                Else
                    Dim DefaultValueType As System.Type = DefaultValue.GetType
                    If DefaultValueType IsNot VoidType Then
                        If IsEnum(DefaultValueType) Then
#If TELESTO Then
                            Throw new InvalidOperationException() 'FIXME: System.Enum.GetName() is not supported on TELESTO
#Else
                            ResultString &= " = " & System.Enum.GetName(DefaultValueType, DefaultValue)
#End If
                        Else
                            ResultString &= " = " & CStr(DefaultValue)
                        End If
                    End If
                End If

                ResultString &= "]"
            End If

            Return ResultString
        End Function

        Public Shared Function MethodToString(ByVal Method As Reflection.MethodBase) As String

            Dim ReturnType As System.Type = Nothing
            Dim First As Boolean
            MethodToString = ""

            If Method.MemberType = MemberTypes.Method Then ReturnType = DirectCast(Method, MethodInfo).ReturnType

            If Method.IsPublic Then
                MethodToString &= "Public "
            ElseIf Method.IsPrivate Then
                MethodToString &= "Private "
            ElseIf Method.IsAssembly Then
                MethodToString &= "Friend "
            End If

            If (Method.Attributes And System.Reflection.MethodAttributes.Virtual) <> 0 Then
                If Not Method.DeclaringType.IsInterface Then
                    MethodToString &= "Overrides "
                End If
            ElseIf IsShared(Method) Then
                MethodToString &= "Shared "
            End If

            Dim Op As UserDefinedOperator = UserDefinedOperator.UNDEF
            If IsUserDefinedOperator(Method) Then
                Op = MapToUserDefinedOperator(Method)
            End If

            If Op <> UserDefinedOperator.UNDEF Then
                If Op = UserDefinedOperator.Narrow Then
                    MethodToString &= "Narrowing "
                ElseIf Op = UserDefinedOperator.Widen Then
                    MethodToString &= "Widening "
                End If
                MethodToString &= "Operator "
            ElseIf ReturnType Is Nothing OrElse ReturnType Is VoidType Then
                MethodToString &= "Sub "
            Else
                MethodToString &= "Function "
            End If

            If Op <> UserDefinedOperator.UNDEF Then
                MethodToString &= OperatorNames(Op)
            ElseIf Method.MemberType = MemberTypes.Constructor Then
                MethodToString &= "New"
            Else
                MethodToString &= Method.Name
            End If

            If IsGeneric(Method) Then
                MethodToString &= "(Of "
                First = True
                For Each t As Type In GetTypeParameters(Method)
                    If Not First Then MethodToString &= ", " Else First = False
                    MethodToString &= VBFriendlyNameOfType(t)
                Next
                MethodToString &= ")"
            End If

            MethodToString &= "("
            First = True

            For Each Parameter As ParameterInfo In Method.GetParameters()

                If Not First Then
                    MethodToString &= ", "
                Else
                    First = False
                End If

                MethodToString &= ParameterToString(Parameter)
            Next

            MethodToString &= ")"

            If ReturnType Is Nothing OrElse ReturnType Is VoidType Then
                'Sub has no return type
            Else
                MethodToString &= " As " & VBFriendlyNameOfType(ReturnType, FullName:=True)
            End If

        End Function

        Private Enum PropertyKind
            ReadWrite
            [ReadOnly]
            [WriteOnly]
        End Enum

        Friend Shared Function PropertyToString(ByVal Prop As Reflection.PropertyInfo) As String

            Dim ResultString As String = ""

            Dim Kind As PropertyKind = PropertyKind.ReadWrite
            Dim Parameters As ParameterInfo()
            Dim PropertyType As Type

            'Most of the work will be done using the Getter or Setter.
            Dim Accessor As MethodInfo = Prop.GetGetMethod

            If Accessor IsNot Nothing Then
                If Prop.GetSetMethod IsNot Nothing Then
                    Kind = PropertyKind.ReadWrite
                Else
                    Kind = PropertyKind.ReadOnly
                End If

                Parameters = Accessor.GetParameters
                PropertyType = Accessor.ReturnType
            Else
                Kind = PropertyKind.WriteOnly

                Accessor = Prop.GetSetMethod
                Dim SetParameters As ParameterInfo() = Accessor.GetParameters
                Parameters = New ParameterInfo(SetParameters.Length - 2) {}
                System.Array.Copy(SetParameters, Parameters, Parameters.Length)
                PropertyType = SetParameters(SetParameters.Length - 1).ParameterType
            End If

            ResultString &= "Public "

            If (Accessor.Attributes And MethodAttributes.Virtual) <> 0 Then
                If Not Prop.DeclaringType.IsInterface Then
                    ResultString &= "Overrides "
                End If
            ElseIf IsShared(Accessor) Then
                ResultString &= "Shared "
            End If

            If Kind = PropertyKind.ReadOnly Then ResultString &= "ReadOnly "
            If Kind = PropertyKind.WriteOnly Then ResultString &= "WriteOnly "

            ResultString &= "Property " & Prop.Name & "("

            Dim First As Boolean = True

            For Each Parameter As ParameterInfo In Parameters
                If Not First Then ResultString &= ", " Else First = False

                ResultString &= ParameterToString(Parameter)
            Next

            ResultString &= ") As " & VBFriendlyNameOfType(PropertyType, FullName:=True)

            Return ResultString
        End Function

#If Not TELESTO Then 'Used by old Everett helper function.

        Friend Shared Function AdjustArraySuffix(ByVal sRank As String) As String
            Dim OneChar As Char
            Dim RevResult As String = Nothing
            Dim length As Integer = sRank.Length
            While length > 0
                OneChar = sRank.Chars(length - 1)
                Select Case OneChar
                    Case ")"c
                        RevResult = RevResult + "("c
                    Case "("c
                        RevResult = RevResult + ")"c
                    Case ","c
                        RevResult = RevResult + OneChar
                    Case Else
                        RevResult = OneChar + RevResult
                End Select
                length = length - 1
            End While
            Return RevResult
        End Function

#End If 'NOT TELESTO

        Friend Shared Function MemberToString(ByVal Member As MemberInfo) As String
            Select Case Member.MemberType
                Case MemberTypes.Method, MemberTypes.Constructor
                    Return MethodToString(DirectCast(Member, MethodBase))

                Case MemberTypes.Field
                    Return FieldToString(DirectCast(Member, FieldInfo))

                Case MemberTypes.Property
                    Return PropertyToString(DirectCast(Member, PropertyInfo))

                Case Else
                    Return Member.Name
            End Select
        End Function

        Friend Shared Function FieldToString(ByVal Field As FieldInfo) As String
            Dim rtype As System.Type
            FieldToString = ""

            rtype = Field.FieldType

            If Field.IsPublic Then
                FieldToString &= "Public "
            ElseIf Field.IsPrivate Then
                FieldToString &= "Private "
            ElseIf Field.IsAssembly Then
                FieldToString &= "Friend "
            ElseIf Field.IsFamily Then
                FieldToString &= "Protected "
            ElseIf Field.IsFamilyOrAssembly Then
                FieldToString &= "Protected Friend "
            End If

            FieldToString &= Field.Name
            FieldToString &= " As "
            FieldToString &= VBFriendlyNameOfType(rtype, FullName:=True)
        End Function
    End Class

#If Not TELESTO Then 'Used by Single Instance code in My.Application
    <SecurityCritical()> _
    Friend NotInheritable Class SafeMemoryMappedViewOfFileHandle : Inherits Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid

        Friend Sub New()
            MyBase.New(True)
        End Sub

        Friend Sub New(ByVal handle As System.IntPtr, ByVal ownsHandle As Boolean)
            MyBase.New(ownsHandle)
            SetHandle(handle)
        End Sub

        <SecurityCritical()> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ResourceConsumption(ResourceScope.Machine)> _
        Protected Overrides Function ReleaseHandle() As Boolean
            Try
                If UnsafeNativeMethods.UnmapViewOfFile(handle) Then
                    Return True
                End If
                Return False
            Finally
                handle = IntPtr.Zero 'either way mark this as invalid now
            End Try
        End Function
    End Class
#End If '#if Not TELESTO
End Namespace

