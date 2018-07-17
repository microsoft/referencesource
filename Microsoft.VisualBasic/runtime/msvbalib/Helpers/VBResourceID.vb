' Copyright (c) Microsoft Corporation.  All rights reserved.

Namespace Microsoft.VisualBasic.CompilerServices
    '**************************************************************************
    ';ResID
    '
    'Remarks: 
    '   This class is use internally inside this Dll. It contains the constant
    '   strings matching the resource name defined in Microsoft.VisualBasic.txt.
    '   The purpose is to reduce typing errors. Everytime you add a new string
    '   resource to Microsoft.VisualBasic.txt, add a constant with the same name
    '   of your string resource name to this class. For example, if your
    '   string resource name is YourResourceName, add a constant like this
    '       Const YourResourceName As String = "YourResourceName"
    '   Then you can access the resource using
    '       ResourceLoader.GetString(ResourceID.YourResourceName)
    '   Note that this class is divided into two sections:  The runtime ids and
    '   the My.Net ids.  All My.Net ids go in the nested MY class
    '**************************************************************************
#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Friend NotInheritable Class ResID
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Friend NotInheritable Class ResID
#End If

#If Not TELESTO Then
        Friend Const Argument_InvalidVbStrConv As String = "Argument_InvalidVbStrConv"
        Friend Const Argument_StrConvSCandTC As String = "Argument_StrConvSCandTC"
        Friend Const Argument_SCNotSupported As String = "Argument_SCNotSupported"
        Friend Const Argument_TCNotSupported As String = "Argument_TCNotSupported"
        Friend Const Argument_JPNNotSupported As String = "Argument_JPNNotSupported"
        Friend Const Argument_IllegalWideNarrow As String = "Argument_IllegalWideNarrow"
        Friend Const Argument_LocalNotSupported As String = "Argument_LocalNotSupported"
        Friend Const Argument_WideNarrowNotApplicable As String = "Argument_WideNarrowNotApplicable"
        Friend Const Argument_IllegalKataHira As String = "Argument_IllegalKataHira"
        Friend Const Argument_PathNullOrEmpty As String = "Argument_PathNullOrEmpty"
        Friend Const Argument_PathNullOrEmpty1 As String = "Argument_PathNullOrEmpty1"
        Friend Const Argument_InvalidPathChars1 As String = "Argument_InvalidPathChars1"
        Friend Const FileSystem_IllegalInputAccess As String = "FileSystem_IllegalInputAccess"
        Friend Const FileSystem_IllegalOutputAccess As String = "FileSystem_IllegalOutputAccess"
        Friend Const FileSystem_IllegalAppendAccess As String = "FileSystem_IllegalAppendAccess"
        Friend Const FileSystem_FileAlreadyOpen1 As String = "FileSystem_FileAlreadyOpen1"
        Friend Const DIR_IllegalCall As String = "DIR_IllegalCall"
        Friend Const KILL_NoFilesFound1 As String = "KILL_NoFilesFound1"
        Friend Const FileSystem_DriveNotFound1 As String = "FileSystem_DriveNotFound1"
        Friend Const FileSystem_FileNotFound1 As String = "FileSystem_FileNotFound1"
        Friend Const FileSystem_PathNotFound1 As String = "FileSystem_PathNotFound1"
        Friend Const Financial_CalcDivByZero As String = "Financial_CalcDivByZero"
        Friend Const Financial_CannotCalculateNPer As String = "Financial_CannotCalculateNPer"
        Friend Const Financial_CannotCalculateRate As String = "Financial_CannotCalculateRate"
        Friend Const Rate_NPerMustBeGTZero As String = "Rate_NPerMustBeGTZero"
        Friend Const PPMT_PerGT0AndLTNPer As String = "PPMT_PerGT0AndLTNPer"
        Friend Const Financial_LifeNEZero As String = "Financial_LifeNEZero"
        Friend Const Financial_ArgGEZero1 As String = "Financial_ArgGEZero1"
        Friend Const Financial_ArgGTZero1 As String = "Financial_ArgGTZero1"
        Friend Const Financial_PeriodLELife As String = "Financial_PeriodLELife"
        Friend Const Argument_Range1toFF1 As String = "Argument_Range1toFF1"
        Friend Const Interaction_ResKeyNotCreated1 As String = "Interaction_ResKeyNotCreated1"
        Friend Const Argument_LCIDNotSupported1 As String = "Argument_LCIDNotSupported1"
        Friend Const ProcessNotFound As String = "ProcessNotFound"
        Friend Const SetLocalDateFailure As String = "SetLocalDateFailure"
        Friend Const SetLocalTimeFailure As String = "SetLocalTimeFailure"
        Friend Const Argument_UnsupportedFieldType2 As String = "Argument_UnsupportedFieldType2"
        Friend Const Argument_UnsupportedIOType1 As String = "Argument_UnsupportedIOType1"
        Friend Const UseFilePutObject As String = "UseFilePutObject"
        Friend Const FileIO_StringLengthExceeded As String = "FileIO_StringLengthExceeded"
        Friend Const MissingMember_MemberSetNotFoundOnType2 As String = "MissingMember_MemberSetNotFoundOnType2"
        Friend Const MissingMember_MemberLetNotFoundOnType2 As String = "MissingMember_MemberLetNotFoundOnType2"
        Friend Const Argument_InvalidNamedArg2 As String = "Argument_InvalidNamedArg2"
        Friend Const NoMethodTakingXArguments2 As String = "NoMethodTakingXArguments2"
        Friend Const AmbiguousCall2 As String = "AmbiguousCall2"
        Friend Const AmbiguousCall_WideningConversion2 As String = "AmbiguousCall_WideningConversion2"
        Friend Const NamedArgumentAlreadyUsed1 As String = "NamedArgumentAlreadyUsed1"
        Friend Const NamedArgumentOnParamArray As String = "NamedArgumentOnParamArray"
        Friend Const LinguisticRequirements As String = "LinguisticRequirements"
        Friend Const Argument_ArrayNotInitialized As String = "Argument_ArrayNotInitialized"
        Friend Const InvalidCast_FromToArg4 As String = "InvalidCast_FromToArg4"
        Friend Const Argument_ArrayDimensionsDontMatch As String = "Argument_ArrayDimensionsDontMatch"
        Friend Const AmbiguousMatch_NarrowingConversion1 As String = "AmbiguousMatch_NarrowingConversion1"
        Friend Const AmbiguousCall_ExactMatch2 As String = "AmbiguousCall_ExactMatch2"
        Friend Const Invalid_VBFixedArray As String = "Invalid_VBFixedArray"
        Friend Const Invalid_VBFixedString As String = "Invalid_VBFixedString"
        Friend Const Argument_UnsupportedArrayDimensions As String = "Argument_UnsupportedArrayDimensions"
        Friend Const Argument_InvalidFixedLengthString As String = "Argument_InvalidFixedLengthString"
        Friend Const Argument_IllegalNestedType2 As String = "Argument_IllegalNestedType2"
        Friend Const Argument_PutObjectOfValueType1 As String = "Argument_PutObjectOfValueType1"
        Friend Const FileOpenedNoRead As String = "FileOpenedNoRead"
        Friend Const FileOpenedNoWrite As String = "FileOpenedNoWrite"
        Friend Const Security_LateBoundCallsNotPermitted As String = "Security_LateBoundCallsNotPermitted"
        Friend Const Serialization_MissingCultureInfo As String = "Serialization_MissingCultureInfo"
        Friend Const Serialization_MissingKeys As String = "Serialization_MissingKeys"
        Friend Const Serialization_MissingValues As String = "Serialization_MissingValues"
        Friend Const Serialization_KeyValueDifferentSizes As String = "Serialization_KeyValueDifferentSizes"
        Friend Const NoValidOperator_OneOperand As String = "NoValidOperator_OneOperand"
        Friend Const NoValidOperator_TwoOperands As String = "NoValidOperator_TwoOperands"
#End If

        Friend Const [False] As String = "False"
        Friend Const [True] As String = "True"
        Friend Const Argument_GEZero1 As String = "Argument_GEZero1"
        Friend Const Argument_GTZero1 As String = "Argument_GTZero1"
        Friend Const Argument_LengthGTZero1 As String = "Argument_LengthGTZero1"
        Friend Const Argument_RangeTwoBytes1 As String = "Argument_RangeTwoBytes1"
        Friend Const Argument_MinusOneOrGTZero1 As String = "Argument_MinusOneOrGTZero1"
        Friend Const Argument_GEMinusOne1 As String = "Argument_GEMinusOne1"
        Friend Const Argument_GEOne1 As String = "Argument_GEOne1"
        Friend Const Argument_RankEQOne1 As String = "Argument_RankEQOne1"
        Friend Const Argument_IComparable2 As String = "Argument_IComparable2"
        Friend Const Argument_NotNumericType2 As String = "Argument_NotNumericType2"
        Friend Const Argument_InvalidValue1 As String = "Argument_InvalidValue1"
        Friend Const Argument_InvalidValueType2 As String = "Argument_InvalidValueType2"
        Friend Const Argument_InvalidValue As String = "Argument_InvalidValue"
        Friend Const Collection_BeforeAfterExclusive As String = "Collection_BeforeAfterExclusive"
        Friend Const Collection_DuplicateKey As String = "Collection_DuplicateKey"
        Friend Const ForLoop_CommonType2 As String = "ForLoop_CommonType2"
        Friend Const ForLoop_CommonType3 As String = "ForLoop_CommonType3"
        Friend Const ForLoop_ConvertToType3 As String = "ForLoop_ConvertToType3"
        Friend Const ForLoop_OperatorRequired2 As String = "ForLoop_OperatorRequired2"
        Friend Const ForLoop_UnacceptableOperator2 As String = "ForLoop_UnacceptableOperator2"
        Friend Const ForLoop_UnacceptableRelOperator2 As String = "ForLoop_UnacceptableRelOperator2"
        Friend Const InternalError As String = "InternalError"
        Friend Const MaxErrNumber As String = "MaxErrNumber"
        Friend Const Argument_InvalidNullValue1 As String = "Argument_InvalidNullValue1"
        Friend Const Argument_InvalidRank1 As String = "Argument_InvalidRank1"
        Friend Const Argument_Range0to99_1 As String = "Argument_Range0to99_1"
        Friend Const Array_RankMismatch As String = "Array_RankMismatch"
        Friend Const Array_TypeMismatch As String = "Array_TypeMismatch"
        Friend Const InvalidCast_FromTo As String = "InvalidCast_FromTo"
        Friend Const InvalidCast_FromStringTo As String = "InvalidCast_FromStringTo"
        Friend Const Argument_InvalidDateValue1 As String = "Argument_InvalidDateValue1"
        Friend Const ArgumentNotNumeric1 As String = "ArgumentNotNumeric1"
        Friend Const Argument_IndexLELength2 As String = "Argument_IndexLELength2"
        Friend Const MissingMember_NoDefaultMemberFound1 As String = "MissingMember_NoDefaultMemberFound1"
        Friend Const MissingMember_MemberNotFoundOnType2 As String = "MissingMember_MemberNotFoundOnType2"
        Friend Const IntermediateLateBoundNothingResult1 As String = "IntermediateLateBoundNothingResult1"
        Friend Const OnOffFormatStyle As String = "OnOffFormatStyle"
        Friend Const YesNoFormatStyle As String = "YesNoFormatStyle"
        Friend Const TrueFalseFormatStyle As String = "TrueFalseFormatStyle"
        Friend Const Argument_CollectionIndex As String = "Argument_CollectionIndex"
        Friend Const RValueBaseForValueType As String = "RValueBaseForValueType"
        Friend Const ExpressionNotProcedure As String = "ExpressionNotProcedure"
        Friend Const LateboundCallToInheritedComClass As String = "LateboundCallToInheritedComClass"
        Friend Const MissingMember_ReadOnlyField2 As String = "MissingMember_ReadOnlyField2"
        Friend Const Argument_InvalidNamedArgs As String = "Argument_InvalidNamedArgs"
        Friend Const SyncLockRequiresReferenceType1 As String = "SyncLockRequiresReferenceType1"
        Friend Const NullReference_InstanceReqToAccessMember1 As String = "NullReference_InstanceReqToAccessMember1"
        Friend Const MatchArgumentFailure2 As String = "MatchArgumentFailure2"
        Friend Const NoGetProperty1 As String = "NoGetProperty1"
        Friend Const NoSetProperty1 As String = "NoSetProperty1"
        Friend Const MethodAssignment1 As String = "MethodAssignment1"

        Friend Const NoViableOverloadCandidates1 As String = "NoViableOverloadCandidates1"
        Friend Const NoArgumentCountOverloadCandidates1 As String = "NoArgumentCountOverloadCandidates1"
        Friend Const NoTypeArgumentCountOverloadCandidates1 As String = "NoTypeArgumentCountOverloadCandidates1"
        Friend Const NoCallableOverloadCandidates2 As String = "NoCallableOverloadCandidates2"
        Friend Const NoNonNarrowingOverloadCandidates2 As String = "NoNonNarrowingOverloadCandidates2"
        Friend Const NoMostSpecificOverload2 As String = "NoMostSpecificOverload2"
        Friend Const AmbiguousCast2 As String = "AmbiguousCast2"

        Friend Const NotMostSpecificOverload As String = "NotMostSpecificOverload"

        Friend Const NamedParamNotFound2 As String = "NamedParamNotFound2"
        Friend Const NamedParamArrayArgument1 As String = "NamedParamArrayArgument1"
        Friend Const NamedArgUsedTwice2 As String = "NamedArgUsedTwice2"
        Friend Const OmittedArgument1 As String = "OmittedArgument1"
        Friend Const OmittedParamArrayArgument As String = "OmittedParamArrayArgument"

        Friend Const ArgumentMismatch3 As String = "ArgumentMismatch3"
        Friend Const ArgumentMismatchAmbiguous3 As String = "ArgumentMismatchAmbiguous3"
        Friend Const ArgumentNarrowing3 As String = "ArgumentNarrowing3"
        Friend Const ArgumentMismatchCopyBack3 As String = "ArgumentMismatchCopyBack3"
        Friend Const ArgumentMismatchAmbiguousCopyBack3 As String = "ArgumentMismatchAmbiguousCopyBack3"
        Friend Const ArgumentNarrowingCopyBack3 As String = "ArgumentNarrowingCopyBack3"

        Friend Const UnboundTypeParam1 As String = "UnboundTypeParam1"
        Friend Const TypeInferenceFails1 As String = "TypeInferenceFails1"
        Friend Const FailedTypeArgumentBinding As String = "FailedTypeArgumentBinding"
        Friend Const UnaryOperand2 As String = "UnaryOperand2"
        Friend Const BinaryOperands3 As String = "BinaryOperands3"
        Friend Const NoValidOperator_StringType1 As String = "NoValidOperator_StringType1"
        Friend Const NoValidOperator_NonStringType1 As String = "NoValidOperator_NonStringType1"

        Friend Const PropertySetMissingArgument1 As String = "PropertySetMissingArgument1"
        Friend Const EmptyPlaceHolderMessage As String = "EmptyPlaceHolderMessage"

        Friend Const WebNotSupportedOnThisSKU As String = "WebNotSupportedOnThisSKU"

        '======================= MY.NET IDs GO HERE ==========================
#If Not TELESTO Then
        Friend NotInheritable Class MyID

            ' Mouse errors.
            Friend Const Mouse_NoMouseIsPresent As String = "Mouse_NoMouseIsPresent"
            Friend Const Mouse_NoWheelIsPresent As String = "Mouse_NoWheelIsPresent"

            ' FileSystem exceptions.
            Friend Const IO_SpecialDirectoryNotExist As String = "IO_SpecialDirectoryNotExist"
            Friend Const IO_SpecialDirectory_MyDocuments As String = "IO_SpecialDirectory_MyDocuments"
            Friend Const IO_SpecialDirectory_MyMusic As String = "IO_SpecialDirectory_MyMusic"
            Friend Const IO_SpecialDirectory_MyPictures As String = "IO_SpecialDirectory_MyPictures"
            Friend Const IO_SpecialDirectory_Desktop As String = "IO_SpecialDirectory_Desktop"
            Friend Const IO_SpecialDirectory_Programs As String = "IO_SpecialDirectory_Programs"
            Friend Const IO_SpecialDirectory_ProgramFiles As String = "IO_SpecialDirectory_ProgramFiles"
            Friend Const IO_SpecialDirectory_Temp As String = "IO_SpecialDirectory_Temp"
            Friend Const IO_SpecialDirectory_AllUserAppData As String = "IO_SpecialDirectory_AllUserAppData"
            Friend Const IO_SpecialDirectory_UserAppData As String = "IO_SpecialDirectory_UserAppData"

            Friend Const IO_FileExists_Path As String = "IO_FileExists_Path"
            Friend Const IO_FileNotFound_Path As String = "IO_FileNotFound_Path"
            Friend Const IO_DirectoryExists_Path As String = "IO_DirectoryExists_Path"
            Friend Const IO_DirectoryIsRoot_Path As String = "IO_DirectoryIsRoot_Path"
            Friend Const IO_DirectoryNotFound_Path As String = "IO_DirectoryNotFound_Path"
            Friend Const IO_GetParentPathIsRoot_Path As String = "IO_GetParentPathIsRoot_Path"

            Friend Const IO_ArgumentIsPath_Name_Path As String = "IO_ArgumentIsPath_Name_Path"

            Friend Const IO_CopyMoveRecursive As String = "IO_CopyMoveRecursive"
            Friend Const IO_CyclicOperation As String = "IO_CyclicOperation"
            Friend Const IO_SourceEqualsTargetDirectory As String = "IO_SourceEqualsTargetDirectory"
            Friend Const IO_GetFiles_NullPattern As String = "IO_GetFiles_NullPattern"
            Friend Const IO_DevicePath As String = "IO_DevicePath"
            Friend Const IO_FilePathException As String = "IO_FilePathException"

            'General errors
            Friend Const General_ArgumentNullException As String = "General_ArgumentNullException"
            Friend Const General_ArgumentEmptyOrNothing_Name As String = "General_ArgumentEmptyOrNothing_Name"
            Friend Const General_PropertyNothing As String = "General_PropertyNothing"

            'Application Log errors
            Friend Const ApplicationLog_FreeSpaceError As String = "ApplicationLog_FreeSpaceError"
            Friend Const ApplicationLog_FileExceedsMaximumSize As String = "ApplicationLog_FileExceedsMaximumSize"
            Friend Const ApplicationLog_ReservedSpaceEncroached As String = "ApplicationLog_ReservedSpaceEncroached"
            Friend Const ApplicationLog_NegativeNumber As String = "ApplicationLog_NegativeNumber"

            Friend Const ApplicationLogNumberTooSmall As String = "ApplicationLogNumberTooSmall"
            Friend Const ApplicationLogBaseNameNull As String = "ApplicationLogBaseNameNull"
            Friend Const ApplicationLog_ExhaustedPossibleStreamNames As String = "ApplicationLog_ExhaustedPossibleStreamNames"

            'Network Strings
            Friend Const Network_InvalidUriString As String = "Network_InvalidUriString"
            Friend Const Network_BadConnectionTimeout As String = "Network_BadConnectionTimeout"
            Friend Const Network_NetworkNotAvailable As String = "Network_NetworkNotAvailable"
            Friend Const Network_UploadAddressNeedsFilename As String = "Network_UploadAddressNeedsFilename"
            Friend Const Network_DownloadNeedsFilename As String = "Network_DownloadNeedsFilename"

            'Progress Dialog
            Friend Const ProgressDialogDownloadingTitle As String = "ProgressDialogDownloadingTitle"
            Friend Const ProgressDialogUploadingTitle As String = "ProgressDialogUploadingTitle"
            Friend Const ProgressDialogDownloadingLabel As String = "ProgressDialogDownloadingLabel"
            Friend Const ProgressDialogUploadingLabel As String = "ProgressDialogUploadingLabel"

            'Diagnostic Information errors.
            Friend Const DiagnosticInfo_Memory As String = "DiagnosticInfo_Memory"
            Friend Const DiagnosticInfo_FullOSName As String = "DiagnosticInfo_FullOSName"

            ' Parser Errors
            Friend Const TextFieldParser_StreamNotReadable As String = "TextFieldParser_StreamNotReadable"
            Friend Const TextFieldParser_NumberOfCharsMustBePositive As String = "TextFieldParser_NumberOfCharsMustBePositive"
            Friend Const TextFieldParser_BufferExceededMaxSize As String = "TextFieldParser_BufferExceededMaxSize"
            Friend Const TextFieldParser_MaxLineSizeExceeded As String = "TextFieldParser_MaxLineSizeExceeded"
            Friend Const TextFieldParser_FieldWidthsNothing As String = "TextFieldParser_FieldWidthsNothing"
            Friend Const TextFieldParser_FieldWidthsMustPositive As String = "TextFieldParser_FieldWidthsMustPositive"
            Friend Const TextFieldParser_DelimitersNothing As String = "TextFieldParser_DelimitersNothing"
            Friend Const TextFieldParser_IllegalDelimiter As String = "TextFieldParser_IllegalDelimiter"
            Friend Const TextFieldParser_DelimiterNothing As String = "TextFieldParser_DelimiterNothing"
            Friend Const TextFieldParser_InvalidComment As String = "TextFieldParser_InvalidComment"
            Friend Const TextFieldParser_MalFormedDelimitedLine As String = "TextFieldParser_MalFormedDelimitedLine"
            Friend Const TextFieldParser_MalFormedFixedWidthLine As String = "TextFieldParser_MalFormedFixedWidthLine"
            Friend Const TextFieldParser_MalformedExtraData As String = "TextFieldParser_MalformedExtraData"
            Friend Const TextFieldParser_WhitespaceInToken As String = "TextFieldParser_WhitespaceInToken"
            Friend Const TextFieldParser_EndCharsInDelimiter As String = "TextFieldParser_EndCharsInDelimiter"

            'Application Model errors.
            Friend Const AppModel_CantGetMemoryMappedFile As String = "AppModel_CantGetMemoryMappedFile"
            Friend Const AppModel_NoStartupForm As String = "AppModel_NoStartupForm"
            Friend Const AppModel_SingleInstanceCantConnect As String = "AppModel_SingleInstanceCantConnect"
            Friend Const AppModel_SplashAndMainFormTheSame As String = "AppModel_SplashAndMainFormTheSame"

            ' Other exceptions
            Friend Const EnvVarNotFound_Name As String = "EnvVarNotFound_Name"

            '''*************************************************************************
            ''' ;New
            ''' <summary>
            ''' FxCop violation: Avoid uninstantiated internal class. 
            ''' Adding a private constructor to prevent the compiler from generating a default constructor.
            ''' </summary>
            Private Sub New()
            End Sub
        End Class 'MyID
#End If 'Not TELESTO

        '''*************************************************************************
        ''' ;New
        ''' <summary>
        ''' FxCop violation: Avoid uninstantiated internal class. 
        ''' Adding a private constructor to prevent the compiler from generating a default constructor.
        ''' </summary>
        Private Sub New()
        End Sub
    End Class
End Namespace
