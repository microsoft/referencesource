' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.IO
Imports System.Runtime.InteropServices
Imports System.Diagnostics
Imports System.Security

Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

    Friend Enum vbErrors
        None = 0
        ReturnWOGoSub = 3
        IllegalFuncCall = 5
        Overflow = 6
        OutOfMemory = 7
        OutOfBounds = 9
        ArrayLocked = 10
        DivByZero = 11
        TypeMismatch = 13
        OutOfStrSpace = 14
        ExprTooComplex = 16
        CantContinue = 17
        UserInterrupt = 18
        ResumeWOErr = 20
        OutOfStack = 28
        UNDONE = 29
        UndefinedProc = 35
        TooManyClients = 47
        DLLLoadErr = 48
        DLLBadCallingConv = 49
        InternalError = 51
        BadFileNameOrNumber = 52
        FileNotFound = 53
        BadFileMode = 54
        FileAlreadyOpen = 55
        IOError = 57
        FileAlreadyExists = 58
        BadRecordLen = 59
        DiskFull = 61
        EndOfFile = 62
        BadRecordNum = 63
        TooManyFiles = 67
        DevUnavailable = 68
        PermissionDenied = 70
        DiskNotReady = 71
        DifferentDrive = 74
        PathFileAccess = 75
        PathNotFound = 76
        ObjNotSet = 91
        IllegalFor = 92
        BadPatStr = 93
        CantUseNull = 94
        UserDefined = 95
        AdviseLimit = 96
        BadCallToFriendFunction = 97
        CantPassPrivateObject = 98
        DLLCallException = 99
        DoesntImplementICollection = 100
        Abort = 287
        InvalidFileFormat = 321
        CantCreateTmpFile = 322
        InvalidResourceFormat = 325
        InvalidPropertyValue = 380
        InvalidPropertyArrayIndex = 381
        SetNotSupportedAtRuntime = 382
        SetNotSupported = 383
        NeedPropertyArrayIndex = 385
        SetNotPermitted = 387
        GetNotSupportedAtRuntime = 393
        GetNotSupported = 394
        PropertyNotFound = 422
        NoSuchControlOrProperty = 423
        NotObject = 424
        CantCreateObject = 429
        OLENotSupported = 430
        OLEFileNotFound = 432
        OLENoPropOrMethod = 438
        OLEAutomationError = 440
        LostTLB = 442
        OLENoDefault = 443
        ActionNotSupported = 445
        NamedArgsNotSupported = 446
        LocaleSettingNotSupported = 447
        NamedParamNotFound = 448
        ParameterNotOptional = 449
        FuncArityMismatch = 450
        NotEnum = 451
        InvalidOrdinal = 452
        InvalidDllFunctionName = 453
        CodeResourceNotFound = 454
        CodeResourceLockError = 455
        DuplicateKey = 457
        InvalidTypeLibVariable = 458
        ObjDoesNotSupportEvents = 459
        InvalidClipboardFormat = 460
        IdentNotMember = 461
        ServerNotFound = 462
        ObjNotRegistered = 463
        InvalidPicture = 481
        PrinterError = 482
        CantSaveFileToTemp = 735
        SearchTextNotFound = 744
        ReplacementsTooLong = 746

        NotYetImplemented = 32768
        FileNotFoundWithName = 40243
        CantFindDllEntryPoint = 59201

        SeekErr = 32771
        ReadFault = 32772
        WriteFault = 32773
        BadFunctionId = 32774
        FileLockViolation = 32775
        ShareRequired = 32789
        BufferTooSmall = 32790
        InvDataRead = 32792
        UnsupFormat = 32793
        RegistryAccess = 32796
        LibNotRegistered = 32797
        Usage = 32799
        UndefinedType = 32807
        QualifiedNameDisallowed = 32808
        InvalidState = 32809
        WrongTypeKind = 32810
        ElementNotFound = 32811
        AmbiguousName = 32812
        ModNameConflict = 32813
        UnknownLcid = 32814
        BadModuleKind = 35005
        NoContainingLib = 35009
        BadTypeId = 35010
        BadLibId = 35011
        Eof = 35012
        SizeTooBig = 35013
        ExpectedFuncNotModule = 35015
        ExpectedFuncNotRecord = 35016
        ExpectedFuncNotProject = 35017
        ExpectedFuncNotVar = 35018
        ExpectedTypeNotProj = 35019
        UnsuitableFuncPropMatch = 35020
        BrokenLibRef = 35021
        UnsupportedTypeLibFeature = 35022
        ModuleAsType = 35024
        InvalidTypeInfoKind = 35025
        InvalidTypeLibFunction = 35026
        OperationNotAllowedInDll = 40035
        CompileError = 40036
        CantEvalWatch = 40037
        MissingVbaTypeLib = 40038
        UserReset = 40040
        MissingEndBrack = 40041
        IncorrectTypeChar = 40042
        InvalidNumLit = 40043
        IllegalChar = 40044
        IdTooLong = 40045
        StatementTooComplex = 40046
        ExpectedTokens = 40047
        InconsistentPropFuncs = 40067
        CircularType = 40068
        AccessViolation = &H80004003 'This is E_POINTER.  This is what VB6 returns from err.Number when calling into a .NET assembly that throws an AccessViolation
        LastTrappable = ReplacementsTooLong
    End Enum

#If TELESTO Then
    Friend NotInheritable Class ExceptionUtils 'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class ExceptionUtils
#End If

        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend Const E_NOTIMPL As Integer = &H80004001I
        Friend Const E_NOINTERFACE As Integer = &H80004002I
        Friend Const E_POINTER As Integer = &H80004003I
        Friend Const E_ABORT As Integer = &H80004004I
        ' FACILITY_DISPATCH - IDispatch errors.
        Friend Const DISP_E_UNKNOWNINTERFACE As Integer = &H80020001I
        Friend Const DISP_E_MEMBERNOTFOUND As Integer = &H80020003I
        Friend Const DISP_E_PARAMNOTFOUND As Integer = &H80020004I
        Friend Const DISP_E_TYPEMISMATCH As Integer = &H80020005I
        Friend Const DISP_E_UNKNOWNNAME As Integer = &H80020006I
        Friend Const DISP_E_NONAMEDARGS As Integer = &H80020007I
        Friend Const DISP_E_BADVARTYPE As Integer = &H80020008I
        Friend Const DISP_E_OVERFLOW As Integer = &H8002000AI
        Friend Const DISP_E_BADINDEX As Integer = &H8002000BI
        Friend Const DISP_E_UNKNOWNLCID As Integer = &H8002000CI
        Friend Const DISP_E_ARRAYISLOCKED As Integer = &H8002000DI
        Friend Const DISP_E_BADPARAMCOUNT As Integer = &H8002000EI
        Friend Const DISP_E_PARAMNOTOPTIONAL As Integer = &H8002000FI
        Friend Const DISP_E_NOTACOLLECTION As Integer = &H80020011I
        Friend Const DISP_E_DIVBYZERO As Integer = &H80020012I
#If Not LATEBINDING Then
        ' FACILITY_DISPATCH - Typelib errors.
        Friend Const TYPE_E_BUFFERTOOSMALL As Integer = &H80028016I
        Friend Const TYPE_E_INVDATAREAD As Integer = &H80028018I
        Friend Const TYPE_E_UNSUPFORMAT As Integer = &H80028019I
        Friend Const TYPE_E_REGISTRYACCESS As Integer = &H8002801CI
        Friend Const TYPE_E_LIBNOTREGISTERED As Integer = &H8002801DI
        Friend Const TYPE_E_UNDEFINEDTYPE As Integer = &H80028027I
        Friend Const TYPE_E_QUALIFIEDNAMEDISALLOWED As Integer = &H80028028I
        Friend Const TYPE_E_INVALIDSTATE As Integer = &H80028029I
        Friend Const TYPE_E_WRONGTYPEKIND As Integer = &H8002802AI
        Friend Const TYPE_E_ELEMENTNOTFOUND As Integer = &H8002802BI
        Friend Const TYPE_E_AMBIGUOUSNAME As Integer = &H8002802CI
        Friend Const TYPE_E_NAMECONFLICT As Integer = &H8002802DI
        Friend Const TYPE_E_UNKNOWNLCID As Integer = &H8002802EI
        Friend Const TYPE_E_DLLFUNCTIONNOTFOUND As Integer = &H8002802FI
        Friend Const TYPE_E_BADMODULEKIND As Integer = &H800288BDI
        Friend Const TYPE_E_SIZETOOBIG As Integer = &H800288C5I
        Friend Const TYPE_E_TYPEMISMATCH As Integer = &H80028CA0I
        Friend Const TYPE_E_OUTOFBOUNDS As Integer = &H80028CA1I
        Friend Const TYPE_E_IOERROR As Integer = &H80028CA2I
        Friend Const TYPE_E_CANTCREATETMPFILE As Integer = &H80028CA3I
        Friend Const TYPE_E_CANTLOADLIBRARY As Integer = &H80029C4AI
        Friend Const TYPE_E_INCONSISTENTPROPFUNCS As Integer = &H80029C83I
        Friend Const TYPE_E_CIRCULARTYPE As Integer = &H80029C84I

        ' FACILITY_STORAGE errors
        Friend Const STG_E_INVALIDFUNCTION As Integer = &H80030001I
        Friend Const STG_E_FILENOTFOUND As Integer = &H80030002I
        Friend Const STG_E_PATHNOTFOUND As Integer = &H80030003I
        Friend Const STG_E_TOOMANYOPENFILES As Integer = &H80030004I
        Friend Const STG_E_ACCESSDENIED As Integer = &H80030005I
        Friend Const STG_E_INVALIDHANDLE As Integer = &H80030006I
        Friend Const STG_E_INSUFFICIENTMEMORY As Integer = &H80030008I
        Friend Const STG_E_NOMOREFILES As Integer = &H80030012I
        Friend Const STG_E_DISKISWRITEPROTECTED As Integer = &H80030013I
        Friend Const STG_E_SEEKERROR As Integer = &H80030019I
        Friend Const STG_E_WRITEFAULT As Integer = &H8003001DI
        Friend Const STG_E_READFAULT As Integer = &H8003001EI
        Friend Const STG_E_SHAREVIOLATION As Integer = &H80030020I
        Friend Const STG_E_LOCKVIOLATION As Integer = &H80030021I
        Friend Const STG_E_FILEALREADYEXISTS As Integer = &H80030050I
        Friend Const STG_E_MEDIUMFULL As Integer = &H80030070I
        Friend Const STG_E_INVALIDHEADER As Integer = &H800300FBI
        Friend Const STG_E_INVALIDNAME As Integer = &H800300FCI
        Friend Const STG_E_UNKNOWN As Integer = &H800300FDI
        Friend Const STG_E_UNIMPLEMENTEDFUNCTION As Integer = &H800300FEI
        Friend Const STG_E_INUSE As Integer = &H80030100I
        Friend Const STG_E_NOTCURRENT As Integer = &H80030101I
        Friend Const STG_E_REVERTED As Integer = &H80030102I
        Friend Const STG_E_CANTSAVE As Integer = &H80030103I
        Friend Const STG_E_OLDFORMAT As Integer = &H80030104I
        Friend Const STG_E_OLDDLL As Integer = &H80030105I
        Friend Const STG_E_SHAREREQUIRED As Integer = &H80030106I
        Friend Const STG_E_NOTFILEBASEDSTORAGE As Integer = &H80030107I
        Friend Const STG_E_EXTANTMARSHALLINGS As Integer = &H80030108I

        ' FACILITY_ITF errors.
        Friend Const CLASS_E_NOTLICENSED As Integer = &H80040112I
        Friend Const REGDB_E_CLASSNOTREG As Integer = &H80040154I
        Friend Const MK_E_UNAVAILABLE As Integer = &H800401E3I
        Friend Const MK_E_INVALIDEXTENSION As Integer = &H800401E6I
        Friend Const MK_E_CANTOPENFILE As Integer = &H800401EAI
        Friend Const CO_E_CLASSSTRING As Integer = &H800401F3I
        Friend Const CO_E_APPNOTFOUND As Integer = &H800401F5I
        Friend Const CO_E_APPDIDNTREG As Integer = &H800401FEI

        ' FACILITY_WIN32 errors
        Friend Const E_ACCESSDENIED As Integer = &H80070005I
        Friend Const E_OUTOFMEMORY As Integer = &H8007000EI
        Friend Const E_INVALIDARG As Integer = &H80070057I

        ' FACILITY_WINDOWS - I don't know why this differs from FACILITY_WIN32
        Friend Const CO_E_SERVER_EXEC_FAILURE As Integer = &H80080005I

#If Not TELESTO Then ' used for Everett FlowControl only
        Friend Shared Function MakeException1(ByVal hr As Integer, ByVal Parm1 As String) As Exception

            Dim sMsg As String
            Dim i As Integer

            If hr > 0 AndAlso hr <= &HFFFFI Then
                sMsg = GetResourceString(CType(hr, vbErrors))
            Else
                sMsg = ""
            End If

            'Insert Parm1 into message 
            i = sMsg.IndexOf("%1", StringComparison.OrdinalIgnoreCase)
            If i >= 0 Then
                sMsg = sMsg.Substring(0, i) + Parm1 + sMsg.Substring(i + 2)
            End If

            Return VbMakeExceptionEx(hr, sMsg)

        End Function
#End If
#End If
        Friend Shared Function VbMakeException(ByVal hr As Integer) As System.Exception
            Dim sMsg As String

            If hr > 0 AndAlso hr <= &HFFFFI Then
                sMsg = GetResourceString(CType(hr, vbErrors))
            Else
                sMsg = ""
            End If
            VbMakeException = VbMakeExceptionEx(hr, sMsg)
        End Function

#If Not LATEBINDING Then
        Friend Shared Function VbMakeException(ByVal ex As Exception, ByVal hr As Integer) As System.Exception
            Err().SetUnmappedError(hr)
            Return ex
        End Function
#End If

        Friend Shared Function VbMakeExceptionEx(ByVal Number As Integer, ByVal sMsg As String) As System.Exception
            Dim VBDefinedError As Boolean

            VbMakeExceptionEx = BuildException(Number, sMsg, VBDefinedError)

            If VBDefinedError Then
#If Not LATEBINDING Then
                Err().SetUnmappedError(Number)
#End If
            End If

        End Function


        Friend Shared Function BuildException(ByVal Number As Integer, ByVal Description As String, ByRef VBDefinedError As Boolean) As System.Exception

            VBDefinedError = True

            Select Case Number

                Case vbErrors.None

                Case vbErrors.ReturnWOGoSub, _
                    vbErrors.ResumeWOErr, _
                    vbErrors.CantUseNull, _
                    vbErrors.DoesntImplementICollection
                    Return New InvalidOperationException(Description)

                Case vbErrors.IllegalFuncCall, _
                    vbErrors.NamedParamNotFound, _
                    vbErrors.NamedArgsNotSupported, _
                    vbErrors.ParameterNotOptional
                    Return New ArgumentException(Description)

                Case vbErrors.OLENoPropOrMethod
                    Return New MissingMemberException(Description)

                Case vbErrors.Overflow
                    Return New OverflowException(Description)

                Case vbErrors.OutOfMemory, vbErrors.OutOfStrSpace
                    Return New OutOfMemoryException(Description)

                Case vbErrors.OutOfBounds
                    Return New IndexOutOfRangeException(Description)

                Case vbErrors.DivByZero
                    Return New DivideByZeroException(Description)

                Case vbErrors.TypeMismatch
                    Return New InvalidCastException(Description)

                Case vbErrors.OutOfStack
                    Return New StackOverflowException(Description)

                Case vbErrors.DLLLoadErr
                    Return New TypeLoadException(Description)

                Case vbErrors.FileNotFound
                    Return New IO.FileNotFoundException(Description)

                Case vbErrors.EndOfFile
                    Return New IO.EndOfStreamException(Description)

                Case vbErrors.IOError, _
                    vbErrors.BadFileNameOrNumber, _
                    vbErrors.BadFileMode, _
                    vbErrors.FileAlreadyOpen, _
                    vbErrors.FileAlreadyExists, _
                    vbErrors.BadRecordLen, _
                    vbErrors.DiskFull, _
                    vbErrors.BadRecordNum, _
                    vbErrors.TooManyFiles, _
                    vbErrors.DevUnavailable, _
                    vbErrors.PermissionDenied, _
                    vbErrors.DiskNotReady, _
                    vbErrors.DifferentDrive, _
                    vbErrors.PathFileAccess
                    Return New IO.IOException(Description)

                Case vbErrors.PathNotFound, _
                    vbErrors.OLEFileNotFound
                    Return New IO.FileNotFoundException(Description)

                Case vbErrors.ObjNotSet
                    Return New NullReferenceException(Description)

                Case vbErrors.PropertyNotFound
                    Return New MissingFieldException(Description)

                Case vbErrors.CantCreateObject, _
                    vbErrors.ServerNotFound
                    Return New Exception(Description)

                Case vbErrors.AccessViolation
                    Return New AccessViolationException() 'We never want a custom description here.  Use the localized message that comes for free inside the exception

                Case Else
                    'Fall below to default
                    VBDefinedError = False
                    Return New Exception(Description)
            End Select

#If TELESTO Then
            VBDefinedError = False
            Return New Exception(Description)
#Else
            Debug.Fail("Should not get here")
            Return Nothing
#End If
        End Function

        '= PUBLIC =============================================================


        '= FRIENDS ============================================================

#If Not TELESTO Then

        '''**************************************************************************
        ''' ;GetArgumentExceptionWithArgName
        ''' <summary>
        ''' Return a new instance of ArgumentException with the message from resource file and the Exception.ArgumentName property set.
        ''' </summary>
        ''' <param name="ArgumentName">The name of the argument (paramemter). Not localized.</param>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of ArgumentException.</returns>
        ''' <remarks>This is the prefered way to construct an argument exception.</remarks>
        Friend Shared Function GetArgumentExceptionWithArgName(ByVal ArgumentName As String, _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As ArgumentException

            Return New ArgumentException(GetResourceString(ResourceID, PlaceHolders), ArgumentName)
        End Function

        '''**************************************************************************
        ''' ;GetArgumentNullException
        ''' <summary>
        ''' Return a new instance of ArgumentNullException with message: "Argument cannot be Nothing."
        ''' </summary>
        ''' <param name="ArgumentName">The name of the argument (paramemter). Not localized.</param>
        ''' <returns>A new instance of ArgumentNullException.</returns>
        Friend Shared Function GetArgumentNullException(ByVal ArgumentName As String) As ArgumentNullException

            Return New ArgumentNullException(ArgumentName, GetResourceString(ResID.MyID.General_ArgumentNullException))
        End Function

        '''**************************************************************************
        ''' ;GetArgumentNullException
        ''' <summary>
        ''' Return a new instance of ArgumentNullException with the message from resource file.
        ''' </summary>
        ''' <param name="ArgumentName">The name of the argument (paramemter). Not localized.</param>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of ArgumentNullException.</returns>
        Friend Shared Function GetArgumentNullException(ByVal ArgumentName As String, _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As ArgumentNullException

            Return New ArgumentNullException(ArgumentName, GetResourceString(ResourceID, PlaceHolders))
        End Function

        '''**************************************************************************
        ''' ;GetDirectoryNotFoundException
        ''' <summary>
        ''' Return a new instance of IO.DirectoryNotFoundException with the message from resource file.
        ''' </summary>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of IO.DirectoryNotFoundException.</returns>
        Friend Shared Function GetDirectoryNotFoundException( _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As IO.DirectoryNotFoundException

            Return New IO.DirectoryNotFoundException(GetResourceString(ResourceID, PlaceHolders))
        End Function

        '''**************************************************************************
        ''' ;GetFileNotFoundException
        ''' <summary>
        ''' Return a new instance of IO.FileNotFoundException with the message from resource file.
        ''' </summary>
        ''' <param name="FileName">The file name (path) of the not found file.</param>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of IO.FileNotFoundException.</returns>
        Friend Shared Function GetFileNotFoundException(ByVal FileName As String, _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As IO.FileNotFoundException

            Return New IO.FileNotFoundException(GetResourceString(ResourceID, PlaceHolders), FileName)
        End Function

        '''**************************************************************************
        ''' ;GetInvalidOperationException
        ''' <summary>
        ''' Return a new instance of InvalidOperationException with the message from resource file.
        ''' </summary>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of InvalidOperationException.</returns>
        Friend Shared Function GetInvalidOperationException( _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As InvalidOperationException

            Return New InvalidOperationException(GetResourceString(ResourceID, PlaceHolders))
        End Function

        '''**************************************************************************
        ''' ;GetIOException
        ''' <summary>
        ''' Return a new instance of IO.IOException with the message from resource file.
        ''' </summary>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of IO.IOException.</returns>
        Friend Shared Function GetIOException(ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As IO.IOException

            Return New IO.IOException(GetResourceString(ResourceID, PlaceHolders))
        End Function

#If 0 Then ': Nobody is using this anymore (fxcop reported it)
        '''**************************************************************************
        ''' ;GetSecurityException
        ''' <summary>
        ''' Return a new instance of Security.SecurityException with the message from resource file.
        ''' </summary>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of Security.SecurityException.</returns>
        Friend Shared Function GetSecurityException(ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) _
            As Security.SecurityException

            Return New Security.SecurityException(GetResourceString(ResourceID, PlaceHolders))
        End Function
#End If

        '''**************************************************************************
        ''' ;GetWin32Exception
        ''' <summary>
        ''' Return a new instance of Win32Exception with the message from resource file and the last Win32 error.
        ''' </summary>
        ''' <param name="ResourceID">The resource ID. Use CompilerServices.ResID.xxx</param>
        ''' <param name="PlaceHolders">Strings that will replace place holders in the resource string, if any.</param>
        ''' <returns>A new instance of Win32Exception.</returns>
        ''' <remarks>There is no way to exclude the Win32 error so this function will call Marshal.GetLastWin32Error all the time.</remarks>
        <SecurityCritical()> _
        Friend Shared Function GetWin32Exception( _
            ByVal ResourceID As String, ByVal ParamArray PlaceHolders() As String) As ComponentModel.Win32Exception

            Return New ComponentModel.Win32Exception(Marshal.GetLastWin32Error(), GetResourceString(ResourceID, PlaceHolders))
        End Function
#End If
    End Class

#If TELESTO Then
    'FIXME <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    'Note that objects aren't serializable in Telesto
    Public NotInheritable Class InternalErrorException
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    <System.Serializable()> _
    Public NotInheritable Class InternalErrorException
#End If

        Inherits System.Exception

#If Not TELESTO Then 'Telesto doesn't support serialization
        ' FxCop: deserialization constructor must be defined as Private.
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Private Sub New(ByVal info As System.Runtime.Serialization.SerializationInfo, ByVal context As System.Runtime.Serialization.StreamingContext)
            MyBase.New(info, context)
        End Sub
#End If

#If TELESTO Then
        'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> 
        Public Sub New(ByVal message As String)
#Else
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Public Sub New(ByVal message As String)
#End If
            MyBase.New(message)
        End Sub

#If TELESTO Then
        'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> 
        Public Sub New(ByVal message As String, ByVal innerException As System.Exception)
#Else
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)> _
        Public Sub New(ByVal message As String, ByVal innerException As System.Exception)
#End If
            MyBase.New(message, innerException)
        End Sub

        ' default constructor
        Public Sub New()
            MyBase.New(GetResourceString(ResID.InternalError))
        End Sub
    End Class

End Namespace
