' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Security
Imports System.Security.Permissions
Imports System.Text
Imports System.Runtime.InteropServices
Imports System.Runtime.ConstrainedExecution
Imports System.Runtime.Versioning

Namespace Microsoft.VisualBasic.CompilerServices

    <ComVisible(False)> _
    <SuppressUnmanagedCodeSecurityAttribute()> _
    Friend NotInheritable Class UnsafeNativeMethods

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Ansi Function LCMapStringA _
                Lib "kernel32" Alias "LCMapStringA" (ByVal Locale As Integer, ByVal dwMapFlags As Integer, _
                    <MarshalAs(UnmanagedType.LPArray)> ByVal lpSrcStr As Byte(), ByVal cchSrc As Integer, <MarshalAs(UnmanagedType.LPArray)> ByVal lpDestStr As Byte(), ByVal cchDest As Integer) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <PreserveSig()> _
        Friend Declare Auto Function LCMapString _
                Lib "kernel32" (ByVal Locale As Integer, ByVal dwMapFlags As Integer, _
                    ByVal lpSrcStr As String, ByVal cchSrc As Integer, ByVal lpDestStr As String, ByVal cchDest As Integer) As Integer

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("oleaut32", PreserveSig:=True, CharSet:=CharSet.Unicode, EntryPoint:="VarParseNumFromStr")> _
        Friend Shared Function VarParseNumFromStr( _
                <[In](), MarshalAs(UnmanagedType.LPWStr)> ByVal str As String, _
                ByVal lcid As Integer, _
                ByVal dwFlags As Integer, _
                <MarshalAs(UnmanagedType.LPArray)> ByVal numprsPtr As Byte(), _
                <MarshalAs(UnmanagedType.LPArray)> ByVal digits As Byte()) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("oleaut32", PreserveSig:=False, CharSet:=CharSet.Unicode, EntryPoint:="VarNumFromParseNum")> _
        Friend Shared Function VarNumFromParseNum( _
                <MarshalAs(UnmanagedType.LPArray)> ByVal numprsPtr As Byte(), _
                <MarshalAs(UnmanagedType.LPArray)> ByVal DigitArray As Byte(), _
                ByVal dwVtBits As Int32) As Object
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("oleaut32", PreserveSig:=False, CharSet:=CharSet.Unicode, EntryPoint:="VariantChangeType")> _
        Friend Shared Sub VariantChangeType( _
            <Out()> ByRef dest As Object, _
            <[In]()> ByRef Src As Object, _
            ByVal wFlags As Int16, _
            ByVal vt As Int16)
        End Sub

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("user32", PreserveSig:=True, CharSet:=CharSet.Unicode, EntryPoint:="MessageBeep")> _
               Friend Shared Function MessageBeep(ByVal uType As Integer) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("kernel32", PreserveSig:=True, CharSet:=CharSet.Unicode, EntryPoint:="SetLocalTime", SetLastError:=True)> _
               Friend Shared Function SetLocalTime(ByVal systime As NativeTypes.SystemTime) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("kernel32", PreserveSig:=True, CharSet:=CharSet.Auto, EntryPoint:="MoveFile", BestFitMapping:=False, ThrowOnUnmappableChar:=True, SetLastError:=True)> _
        Friend Shared Function MoveFile(<[In](), MarshalAs(UnmanagedType.LPTStr)> ByVal lpExistingFileName As String, _
                <[In](), MarshalAs(UnmanagedType.LPTStr)> ByVal lpNewFileName As String) As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("kernel32", PreserveSig:=True, CharSet:=CharSet.Unicode, EntryPoint:="GetLogicalDrives")> _
        Friend Shared Function GetLogicalDrives() As Integer
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Kernel32", EntryPoint:="CreateFileMapping", CharSet:=CharSet.Auto, BestFitMapping:=False, SetLastError:=True)> _
        Friend Shared Function CreateFileMapping(ByVal hFile As HandleRef, <MarshalAs(UnmanagedType.LPStruct)> ByVal lpAttributes As NativeTypes.SECURITY_ATTRIBUTES, ByVal flProtect As Integer, ByVal dwMaxSizeHi As Integer, ByVal dwMaxSizeLow As Integer, ByVal lpName As String) As Win32.SafeHandles.SafeFileHandle
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Kernel32", EntryPoint:="OpenFileMapping", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Function OpenFileMapping(ByVal dwDesiredAccess As Integer, <MarshalAs(UnmanagedType.Bool)> ByVal bInheritHandle As Boolean, ByVal lpName As String) As Win32.SafeHandles.SafeFileHandle
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("Kernel32", EntryPoint:="MapViewOfFile", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Function MapViewOfFile(ByVal hFileMapping As IntPtr, ByVal dwDesiredAccess As Integer, ByVal dwFileOffsetHigh As Integer, ByVal dwFileOffsetLow As Integer, ByVal dwNumberOfBytesToMap As UintPtr) As SafeMemoryMappedViewOfFileHandle
        End Function

        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)> _
        <DllImport("Kernel32", EntryPoint:="UnmapViewOfFile", CharSet:=CharSet.Auto, SetLastError:=True)> _
        Friend Shared Function UnmapViewOfFile(ByVal pvBaseAddress As IntPtr) As <MarshalAsAttribute(UnmanagedType.Bool)> Boolean
        End Function

        Public Const MEMBERID_NIL As Integer = 0
        Public Const LCID_US_ENGLISH As Integer = &H409


        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
        Public Enum tagSYSKIND
            SYS_WIN16 = 0
            SYS_MAC = 2
        End Enum

        ' REVIEW :  - c# version was class, does it make a difference?
        '    [StructLayout(LayoutKind.Sequential)]
        '    Public class  tagTLIBATTR {
        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
        Public Structure tagTLIBATTR
            Public guid As Guid
            Public lcid As Integer
            Public syskind As tagSYSKIND
            <MarshalAs(UnmanagedType.U2)> Public wMajorVerNum As Short
            <MarshalAs(UnmanagedType.U2)> Public wMinorVerNum As Short
            <MarshalAs(UnmanagedType.U2)> Public wLibFlags As Short
        End Structure

        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never), _
         ComImport(), _
         Guid("00020403-0000-0000-C000-000000000046"), _
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)> _
        Public Interface ITypeComp

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteBind( _
                   <[In](), MarshalAs(UnmanagedType.LPWStr)> ByVal szName As String, _
                   <[In](), MarshalAs(UnmanagedType.U4)> ByVal lHashVal As Integer, _
                   <[In](), MarshalAs(UnmanagedType.U2)> ByVal wFlags As Short, _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTInfo As ITypeInfo(), _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pDescKind As ComTypes.DESCKIND(), _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppFuncDesc As ComTypes.FUNCDESC(), _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppVarDesc As ComTypes.VARDESC(), _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTypeComp As ITypeComp(), _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pDummy As Integer())

            <SecurityCritical()> _
            Sub RemoteBindType( _
                   <[In](), MarshalAs(UnmanagedType.LPWStr)> ByVal szName As String, _
                   <[In](), MarshalAs(UnmanagedType.U4)> ByVal lHashVal As Integer, _
                   <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTInfo As ITypeInfo())
        End Interface



        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never), _
         ComImport(), _
         Guid("00020400-0000-0000-C000-000000000046"), _
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)> _
        Public Interface IDispatch

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetTypeInfoCount() As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetTypeInfo( _
                    <[In]()> ByVal index As Integer, _
                    <[In]()> ByVal lcid As Integer, _
                    <[Out](), MarshalAs(UnmanagedType.Interface)> ByRef pTypeInfo As ITypeInfo) As Integer

            ' WARNING :  - This api NOT COMPLETELY DEFINED, DO NOT CALL!
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetIDsOfNames() As Integer

            ' WARNING :  - This api NOT COMPLETELY DEFINED, DO NOT CALL!
            <PreserveSig()> _
            <SecurityCritical()> _
            Function Invoke() As Integer
        End Interface



        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never), _
         ComImport(), _
         Guid("00020401-0000-0000-C000-000000000046"), _
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)> _
        Public Interface ITypeInfo
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetTypeAttr( _
                    <Out()> ByRef pTypeAttr As IntPtr) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetTypeComp( _
                    <Out()> ByRef pTComp As ITypeComp) As Integer


            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetFuncDesc( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out()> ByRef pFuncDesc As IntPtr) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetVarDesc( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out()> ByRef pVarDesc As IntPtr) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetNames( _
                    <[In]()> ByVal memid As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal rgBstrNames As String(), _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal cMaxNames As Integer, _
                    <Out(), MarshalAs(UnmanagedType.U4)> ByRef cNames As Integer) As Integer

            <Obsolete("Bad signature, second param type should be Byref. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetRefTypeOfImplType( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out()> ByRef pRefType As Integer) As Integer

            <Obsolete("Bad signature, second param type should be Byref. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetImplTypeFlags( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out()> ByVal pImplTypeFlags As Integer) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetIDsOfNames( _
                    <[In]()> ByVal rgszNames As IntPtr, _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal cNames As Integer, _
                    <Out()> ByRef pMemId As IntPtr) As Integer

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function Invoke() As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetDocumentation( _
                     <[In]()> ByVal memid As Integer, _
                     <Out(), MarshalAs(UnmanagedType.BStr)> ByRef pBstrName As String, _
                     <Out(), MarshalAs(UnmanagedType.BStr)> ByRef pBstrDocString As String, _
                     <Out(), MarshalAs(UnmanagedType.U4)> ByRef pdwHelpContext As Integer, _
                     <Out(), MarshalAs(UnmanagedType.BStr)> ByRef pBstrHelpFile As String) As Integer

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetDllEntry( _
                    <[In]()> ByVal memid As Integer, _
                    <[In]()> ByVal invkind As ComTypes.INVOKEKIND, _
                    <Out(), MarshalAs(UnmanagedType.BStr)> ByVal pBstrDllName As String, _
                    <Out(), MarshalAs(UnmanagedType.BStr)> ByVal pBstrName As String, _
                    <Out(), MarshalAs(UnmanagedType.U2)> ByVal pwOrdinal As Short) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetRefTypeInfo( _
                     <[In]()> ByVal hreftype As IntPtr, _
                     <Out()> ByRef pTypeInfo As ITypeInfo) As Integer

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function AddressOfMember() As Integer

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function CreateInstance( _
                    <[In]()> ByRef pUnkOuter As IntPtr, _
                    <[In]()> ByRef riid As Guid, _
                    <Out(), MarshalAs(UnmanagedType.IUnknown)> ByVal ppvObj As Object) As Integer

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetMops( _
                    <[In]()> ByVal memid As Integer, _
                    <Out(), MarshalAs(UnmanagedType.BStr)> ByVal pBstrMops As String) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Function GetContainingTypeLib( _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTLib As ITypeLib(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pIndex As Integer()) As Integer

            <PreserveSig()> _
            <SecurityCritical()> _
            Sub ReleaseTypeAttr(ByVal typeAttr As IntPtr)

            <PreserveSig()> _
            <SecurityCritical()> _
            Sub ReleaseFuncDesc(ByVal funcDesc As IntPtr)

            <PreserveSig()> _
            <SecurityCritical()> _
            Sub ReleaseVarDesc(ByVal varDesc As IntPtr)
        End Interface



        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never), _
         ComImport(), _
         Guid("B196B283-BAB4-101A-B69C-00AA00341D07"), _
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)> _
        Public Interface IProvideClassInfo
            <SecurityCritical()> _
            Function GetClassInfo() As <MarshalAs(UnmanagedType.Interface)> ITypeInfo
        End Interface



        <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never), _
         ComImport(), _
         Guid("00020402-0000-0000-C000-000000000046"), _
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)> _
        Public Interface ITypeLib
            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteGetTypeInfoCount( _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pcTInfo As Integer())

            <SecurityCritical()> _
            Sub GetTypeInfo( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTInfo As ITypeInfo())

            <SecurityCritical()> _
            Sub GetTypeInfoType( _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal index As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pTKind As ComTypes.TYPEKIND())

            <SecurityCritical()> _
            Sub GetTypeInfoOfGuid( _
                    <[In]()> ByRef guid As Guid, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTInfo As ITypeInfo())

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteGetLibAttr( _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTLibAttr As tagTLIBATTR(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pDummy As Integer())

            <SecurityCritical()> _
            Sub GetTypeComp( _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTComp As ITypeComp())

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteGetDocumentation( _
            ByVal index As Integer, _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal refPtrFlags As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pBstrName As String(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pBstrDocString As String(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pdwHelpContext As Integer(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pBstrHelpFile As String())

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteIsName( _
                    <[In](), MarshalAs(UnmanagedType.LPWStr)> ByVal szNameBuf As String, _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal lHashVal As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pfName As IntPtr(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pBstrLibName As String())

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub RemoteFindName( _
                    <[In](), MarshalAs(UnmanagedType.LPWStr)> ByVal szNameBuf As String, _
                    <[In](), MarshalAs(UnmanagedType.U4)> ByVal lHashVal As Integer, _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal ppTInfo As ITypeInfo(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal rgMemId As Integer(), _
                    <[In](), Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pcFound As Short(), _
                    <Out(), MarshalAs(UnmanagedType.LPArray)> ByVal pBstrLibName As String())

            <Obsolete("Bad signature. Fix and verify signature before use.", True)> _
            <SecurityCritical()> _
            Sub LocalReleaseTLibAttr()
        End Interface

        '*****************************************************************************
        ';GetKeyState
        '
        'Summary: 
        '   Gets the state of the specified key on the keyboard when the function 
        '   is called.
        'Params: 
        '   KeyCode - Integer representing the key in question.
        'Returns: 
        '   The high order byte is 1 if the key is down. The low order byte is one
        '   if the key is toggled on (i.e. for keys like CapsLock)
        '*****************************************************************************
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.Machine)> _
        <DllImport("User32.dll", ExactSpelling:=True, CharSet:=CharSet.Auto)> _
        Friend Shared Function GetKeyState(ByVal KeyCode As Integer) As Short
        End Function

        '''*************************************************************************
        ''';LocalFree
        ''' <summary>
        ''' Frees memory allocated from the local heap. i.e. frees memory allocated
        ''' by LocalAlloc or LocalReAlloc.n
        ''' </summary>
        ''' <param name="LocalHandle"></param>
        ''' <returns></returns>
        ''' <remarks></remarks>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("kernel32", ExactSpelling:=True, setlasterror:=True)> _
        Friend Shared Function LocalFree(ByVal LocalHandle As IntPtr) As IntPtr
        End Function

        ''' **************************************************************************
        ''' ;GetDiskFreeSpaceEx
        ''' <summary>
        ''' Used to determine how much free space is on a disk
        ''' </summary>
        ''' <param name="Directory">Path including drive we're getting information about</param>
        ''' <param name="UserSpaceFree">The amount of free sapce available to the current user</param>
        ''' <param name="TotalUserSpace">The total amount of space on the disk relative to the current user</param>
        ''' <param name="TotalFreeSpace">The amount of free spave on the disk.</param>
        ''' <returns>True if function succeeds in getting info otherwise False</returns>
        <SecurityCritical()> _
        <ResourceExposure(ResourceScope.None)> _
        <DllImport("Kernel32.dll", CharSet:=CharSet.Auto, BestFitMapping:=False, SetLastError:=True)> _
        Friend Shared Function GetDiskFreeSpaceEx(ByVal Directory As String, ByRef UserSpaceFree As Long, ByRef TotalUserSpace As Long, ByRef TotalFreeSpace As Long) As <MarshalAs(UnmanagedType.Bool)> Boolean
        End Function

        '''*************************************************************************
        ''' ;New
        ''' <summary>
        ''' FxCop violation: Avoid uninstantiated internal class. 
        ''' Adding a private constructor to prevent the compiler from generating a default constructor.
        ''' </summary>
        <SecurityCritical()> _
        Private Sub New()
        End Sub
    End Class

End Namespace

