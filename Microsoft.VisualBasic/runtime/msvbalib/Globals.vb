' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports Microsoft.VisualBasic.CompilerServices
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic
#If Not LATEBINDING Then
    Public Enum VariantType
        [Empty] = 0
        [Null] = 1
        [Short] = 2
        [Integer] = 3
        [Single] = 4
        [Double] = 5
        [Currency] = 6
        [Date] = 7
        [String] = 8
        [Object] = 9
        [Error] = 10
        [Boolean] = 11
        [Variant] = 12
        [DataObject] = 13
        [Decimal] = 14
        [Byte] = 17
        [Char] = 18
        [Long] = 20
        [UserDefinedType] = 36
        [Array] = 8192
    End Enum
#End If

#If Not TELESTO Then
    Public Enum AppWinStyle As Short
        Hide = 0
        NormalFocus = 1
        MinimizedFocus = 2
        MaximizedFocus = 3
        NormalNoFocus = 4
        MinimizedNoFocus = 6
    End Enum
#End If


    Public Enum CallType
        Method = 1
        [Get] = 2
        [Let] = 4
        [Set] = 8
    End Enum


#If Not LATEBINDING Then
    Public Enum CompareMethod
        [Binary] = 0
        [Text] = 1
    End Enum



    Public Enum DateFormat
        GeneralDate = 0
        LongDate = 1
        ShortDate = 2
        LongTime = 3
        ShortTime = 4
    End Enum



    Public Enum FirstDayOfWeek
        System = 0
        Sunday = 1
        Monday = 2
        Tuesday = 3
        Wednesday = 4
        Thursday = 5
        Friday = 6
        Saturday = 7
    End Enum


#If Not TELESTO Then
    <Flags()> Public Enum FileAttribute
        [Normal] = 0
        [ReadOnly] = 1
        [Hidden] = 2
        [System] = 4
        [Volume] = 8
        [Directory] = 16
        [Archive] = 32
    End Enum
#End If


    Public Enum FirstWeekOfYear
        System = 0
        Jan1 = 1
        FirstFourDays = 2
        FirstFullWeek = 3
    End Enum


#If Not TELESTO Then
    <Flags()> Public Enum VbStrConv
        [None] = 0
        [Uppercase] = 1
        [Lowercase] = 2
        [ProperCase] = 3
        [Wide] = 4
        [Narrow] = 8
        [Katakana] = 16
        [Hiragana] = 32
        '[Unicode]      = 64 'OBSOLETE
        '[FromUnicode]   = 128 'OBSOLETE
        [SimplifiedChinese] = 256
        [TraditionalChinese] = 512
        [LinguisticCasing] = 1024
    End Enum
#End If


    Public Enum TriState
        [False] = 0
        [True] = -1
        [UseDefault] = -2
    End Enum



    Public Enum DateInterval
        [Year] = 0
        [Quarter] = 1
        [Month] = 2
        [DayOfYear] = 3
        [Day] = 4
        [WeekOfYear] = 5
        [Weekday] = 6
        [Hour] = 7
        [Minute] = 8
        [Second] = 9
    End Enum


#If Not TELESTO Then

    Public Enum DueDate
        EndOfPeriod = 0
        BegOfPeriod = 1
    End Enum


    Public Enum OpenMode
        [Input] = 1
        [Output] = 2
        [Random] = 4
        [Append] = 8
        [Binary] = 32
    End Enum



    Friend Enum OpenModeTypes
        [Input] = 1
        [Output] = 2
        [Random] = 4
        [Append] = 8
        [Binary] = 32
        [Any] = -1
    End Enum



    Public Enum OpenAccess
        [Default] = -1
        [Read] = System.IO.FileAccess.Read
        [ReadWrite] = System.IO.FileAccess.ReadWrite
        [Write] = System.IO.FileAccess.Write
    End Enum



    Public Enum OpenShare
        [Default] = -1
        [Shared] = System.IO.FileShare.ReadWrite
        [LockRead] = System.IO.FileShare.Write
        [LockReadWrite] = System.IO.FileShare.None
        [LockWrite] = System.IO.FileShare.Read
    End Enum



    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public Structure TabInfo
        Public Column As Short
    End Structure



    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public Structure SpcInfo
        Public Count As Short
    End Structure

    Public Enum MsgBoxResult
        Ok = 1
        Cancel = 2
        Abort = 3
        Retry = 4
        Ignore = 5
        Yes = 6
        No = 7
    End Enum

    <Flags()> _
    Public Enum MsgBoxStyle
        'You may BitOr one value from each group
        'Button group: Lower 4 bits, &H00F 
        OkOnly = &H0I
        OkCancel = &H1I
        AbortRetryIgnore = &H2I
        YesNoCancel = &H3I
        YesNo = &H4I
        RetryCancel = &H5I

        'Icon Group: Middle 4 bits &H0F0
        Critical = &H10I     'Same as Windows.Forms.MessageBox.IconError
        Question = &H20I     'Same As Windows.MessageBox.IconQuestion
        Exclamation = &H30I  'Same As Windows.MessageBox.IconExclamation
        Information = &H40I  'Same As Windows.MessageBox.IconInformation

        'Default Group: High 4 bits &HF00
        DefaultButton1 = 0
        DefaultButton2 = &H100I
        DefaultButton3 = &H200I
        'UNSUPPORTED IN VB7
        'DefaultButton4 = &H300I

        ApplicationModal = &H0I
        SystemModal = &H1000I

        MsgBoxHelp = &H4000I
        MsgBoxRight = &H80000I
        MsgBoxRtlReading = &H100000I
        MsgBoxSetForeground = &H10000I
    End Enum



    ' -------------------------------------------------------------------
    ' VBFixedString is used by the runtime to determine 
    ' if the field should be written/read without the string length descriptor.
    ' -------------------------------------------------------------------
    <System.AttributeUsage(System.AttributeTargets.Field, Inherited:=False, AllowMultiple:=False)> _
    Public NotInheritable Class VBFixedStringAttribute
        Inherits System.Attribute

        Private m_Length As Integer

        Public ReadOnly Property Length() As Integer
            Get
                Return m_Length
            End Get
        End Property


        Public Sub New(ByVal Length As Integer)
            If (Length < 1 OrElse Length > System.Int16.MaxValue) Then
                Throw New ArgumentException(GetResourceString(ResID.Invalid_VBFixedString))
            End If
            m_Length = Length
        End Sub
    End Class



    ' -------------------------------------------------------------------
    ' VBFixedArray is used by the runtime to determine 
    ' if the array should be written/read without the array descriptor.
    ' -------------------------------------------------------------------
    <System.AttributeUsage(System.AttributeTargets.Field, Inherited:=False, AllowMultiple:=False)> _
    Public NotInheritable Class VBFixedArrayAttribute
        Inherits System.Attribute

        Friend FirstBound As Integer
        Friend SecondBound As Integer

        Public ReadOnly Property Bounds() As Integer()
            Get
                If Me.SecondBound = -1 Then
                    Return New Integer() {Me.FirstBound}
                Else
                    Return New Integer() {Me.FirstBound, Me.SecondBound}
                End If
            End Get
        End Property

        Public ReadOnly Property Length() As Integer
            Get
                If Me.SecondBound = -1 Then
                    Return (Me.FirstBound + 1)
                Else
                    Return (Me.FirstBound + 1) * (Me.SecondBound + 1)
                End If
            End Get
        End Property

        Public Sub New(ByVal UpperBound1 As Integer)

            'Validate all the bounds
            If UpperBound1 < 0 Then
                Throw New ArgumentException(GetResourceString(ResID.Invalid_VBFixedArray))
            End If

            Me.FirstBound = UpperBound1
            Me.SecondBound = -1

        End Sub

        Public Sub New(ByVal UpperBound1 As Integer, ByVal UpperBound2 As Integer)

            'Validate all the bounds
            If UpperBound1 < 0 OrElse UpperBound2 < 0 Then
                Throw New ArgumentException(GetResourceString(ResID.Invalid_VBFixedArray))
            End If

            Me.FirstBound = UpperBound1
            Me.SecondBound = UpperBound2

        End Sub

    End Class



    ' -------------------------------------------------------------------
    ' ComClass is used by the VB compiler to mark a public class 
    ' that will be exposed via COM interop.
    ' -------------------------------------------------------------------
    <System.AttributeUsage(System.AttributeTargets.Class, Inherited:=False, AllowMultiple:=False)> _
    Public NotInheritable Class ComClassAttribute
        Inherits System.Attribute

        Private m_ClassID As String
        Private m_InterfaceID As String
        Private m_EventID As String
        Private m_InterfaceShadows As Boolean = False



        Public Sub New()
        End Sub



        Public Sub New(ByVal _ClassID As String)
            m_ClassID = _ClassID
        End Sub



        Public Sub New(ByVal _ClassID As String, ByVal _InterfaceID As String)
            m_ClassID = _ClassID
            m_InterfaceID = _InterfaceID
        End Sub



        Public Sub New(ByVal _ClassID As String, ByVal _InterfaceID As String, ByVal _EventId As String)
            m_ClassID = _ClassID
            m_InterfaceID = _InterfaceID
            m_EventID = _EventId
        End Sub



        Public ReadOnly Property ClassID() As String
            Get
                Return m_ClassID
            End Get
        End Property



        Public ReadOnly Property InterfaceID() As String
            Get
                Return m_InterfaceID
            End Get
        End Property



        Public ReadOnly Property EventID() As String
            Get
                Return m_EventID
            End Get
        End Property



        Public Property InterfaceShadows() As Boolean
            Get
                Return m_InterfaceShadows
            End Get
            Set(ByVal Value As Boolean)
                m_InterfaceShadows = Value
            End Set
        End Property
    End Class

#End If 'NOT TELESTO

    '''**************************************************************************
    ''' ;MyGroupCollectionAttribute 
    ''' <summary>
    ''' This attribute is put on an empty 'container class' that the compiler then fills with
    ''' properties that return instances of all the types found in the project which derive
    ''' from the TypeToCollect argument.
    ''' 
    ''' This is how My.Forms is built, for instance.
    ''' </summary>
    ''' <remarks>
    ''' WARNING: Do not rename this attribute or move it out of this module.  Otherwise there
    ''' are compiler changes that will need to be made
    ''' </remarks>
#If TELESTO Then
    'FIXME    <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Advanced)> _
    <AttributeUsage(AttributeTargets.Class, AllowMultiple:=False, Inherited:=False)> _
    Public NotInheritable Class MyGroupCollectionAttribute : Inherits Attribute
#Else
    <AttributeUsage(AttributeTargets.Class, AllowMultiple:=False, Inherited:=False)> _
    <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Advanced)> _
    Public NotInheritable Class MyGroupCollectionAttribute : Inherits Attribute
#End If

        '''**************************************************************************
        ''' ;New
        ''' <summary>
        ''' </summary>
        ''' <param name="TypeToCollect">Compiler will generate accessors for classes that derived from this type</param>
        ''' <param name="CreateInstanceMethodName">Name of the factory method to create the instances</param>
        ''' <param name="DisposeInstanceMethodName">Name of the method that will dispose of the instances</param>
        ''' <param name="DefaultInstanceAlias">"Name of the My.* method to call to get the default instance for the types in the container</param>
        Public Sub New(ByVal typeToCollect As String, ByVal createInstanceMethodName As String, _
                                ByVal disposeInstanceMethodName As String, ByVal defaultInstanceAlias As String)

            m_NameOfBaseTypeToCollect = typeToCollect
            m_NameOfCreateMethod = createInstanceMethodName
            m_NameOfDisposeMethod = disposeInstanceMethodName
            m_DefaultInstanceAlias = defaultInstanceAlias

        End Sub

        '''**************************************************************************
        ''' ;MyGroupName
        ''' <summary>
        ''' The name of the base type we are trying to collect
        ''' </summary>
        Public ReadOnly Property MyGroupName() As String
            Get
                Return m_NameOfBaseTypeToCollect
            End Get
        End Property


        '''**************************************************************************
        ''' ;CreateMethod
        ''' <summary>
        ''' Name of the factory method to create the instances
        ''' </summary>
        Public ReadOnly Property CreateMethod() As String
            Get
                Return m_NameOfCreateMethod
            End Get
        End Property


        '''**************************************************************************
        ''' ;DisposeMethod
        ''' <summary>
        ''' Name of the method that will dispose of the instances
        ''' </summary>
        Public ReadOnly Property DisposeMethod() As String
            Get
                Return m_NameOfDisposeMethod
            End Get
        End Property

        '''**************************************************************************
        ''' ;DefaultInstanceAlias
        ''' <summary>
        ''' Provides the name of the My.* methods to call to get the 'default instance' 
        ''' </summary>
        Public ReadOnly Property DefaultInstanceAlias() As String
            Get
                Return m_DefaultInstanceAlias
            End Get
        End Property

        Private m_NameOfBaseTypeToCollect, m_NameOfCreateMethod, m_NameOfDisposeMethod, m_DefaultInstanceAlias As String
    End Class 'MyGroupCollectionAttribute

    '''**************************************************************************
    ''' ;HideModuleNameAttribute
    ''' <summary>
    ''' When applied to a module, Intellisense will hide the module from
    ''' the statement completion list, but not the contents of the module.
    ''' </summary>
    ''' <remarks>
    ''' WARNING: Do not rename this attribute or move it out of this module.  Otherwise there
    ''' are compiler changes that will need to be made
    ''' </remarks>
    <AttributeUsage(AttributeTargets.Class, AllowMultiple:=False, Inherited:=False)> _
    Public NotInheritable Class HideModuleNameAttribute
        Inherits System.Attribute

    End Class

    Public Module Globals
        Public ReadOnly Property ScriptEngine() As String
            Get
                Return "VB"
            End Get
        End Property


        Public ReadOnly Property ScriptEngineMajorVersion() As Integer
            Get
                Return CompilerServices._Version.Major
            End Get
        End Property



        Public ReadOnly Property ScriptEngineMinorVersion() As Integer
            Get
                Return CompilerServices._Version.Minor
            End Get
        End Property


        Public ReadOnly Property ScriptEngineBuildVersion() As Integer
            Get
                Return CompilerServices._Version.Build
            End Get
        End Property
    End Module
#End If
End Namespace


