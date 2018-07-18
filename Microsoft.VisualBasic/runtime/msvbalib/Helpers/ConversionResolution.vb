' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Reflection
Imports System.Diagnostics
Imports System.Collections.Generic

Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.ConversionResolution
Imports Microsoft.VisualBasic.CompilerServices.OperatorCaches

Namespace Microsoft.VisualBasic.CompilerServices

    Friend Class ConversionResolution
        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend Enum ConversionClass As SByte
            Bad
            Identity
            [Widening]
            [Narrowing]
            None
            Ambiguous
        End Enum

        Private Shared ReadOnly ConversionTable As ConversionClass()()
        Friend Shared ReadOnly NumericSpecificityRank As Integer()
        Friend Shared ReadOnly ForLoopWidestTypeCode As TypeCode()()

        Shared Sub New()
            Const Max As Integer = TypeCode.String

            Const Bad_ As ConversionClass = ConversionClass.Bad
            Const Iden As ConversionClass = ConversionClass.Identity
            Const Wide As ConversionClass = ConversionClass.Widening
            Const Narr As ConversionClass = ConversionClass.Narrowing
            Const None As ConversionClass = ConversionClass.None

            'Columns represent Source type, Rows represent Target type.
            '                                 empty obj   dbnul bool  char  sbyte byte  short ushrt int   uint  lng   ulng  sng   dbl   dec   date        str
            ConversionTable = New ConversionClass(Max)() _
                { _
                    New ConversionClass(Max) {Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_}, _
                    New ConversionClass(Max) {Bad_, Iden, Bad_, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Bad_, Wide}, _
                    New ConversionClass(Max) {Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Iden, None, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, None, Iden, None, None, None, None, None, None, None, None, None, None, None, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Iden, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Narr, Iden, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Iden, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Narr, Wide, Narr, Iden, Narr, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Wide, Wide, Iden, Narr, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Narr, Wide, Narr, Wide, Narr, Iden, Narr, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Wide, Wide, Wide, Wide, Iden, Narr, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Narr, Wide, Narr, Wide, Narr, Wide, Narr, Iden, Narr, Narr, Narr, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Iden, Narr, Wide, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Iden, Wide, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, None, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Narr, Narr, Iden, None, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, None, None, None, None, None, None, None, None, None, None, None, None, None, Iden, Bad_, Narr}, _
                    New ConversionClass(Max) {Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_, Bad_}, _
                    New ConversionClass(Max) {Bad_, Narr, Bad_, Narr, Wide, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Bad_, Iden} _
                }

            'This table is the relative ordering of the specificity of types. It is used during
            'overload resolution to answer the question: 'Of numeric types a and b, which is more specific?'.
            '
            'The general rules encoded in this table are:
            '    Smaller types are more specific than larger types.
            '    Signed types are more specific than unsigned types of equal or greater widths,
            '    with the exception of Byte which is more specific than SByte (for backwards compatibility).

            NumericSpecificityRank = New Integer(Max) {}
            NumericSpecificityRank(TypeCode.Byte) = 1
            NumericSpecificityRank(TypeCode.SByte) = 2
            NumericSpecificityRank(TypeCode.Int16) = 3
            NumericSpecificityRank(TypeCode.UInt16) = 4
            NumericSpecificityRank(TypeCode.Int32) = 5
            NumericSpecificityRank(TypeCode.UInt32) = 6
            NumericSpecificityRank(TypeCode.Int64) = 7
            NumericSpecificityRank(TypeCode.UInt64) = 8
            NumericSpecificityRank(TypeCode.Decimal) = 9
            NumericSpecificityRank(TypeCode.Single) = 10
            NumericSpecificityRank(TypeCode.Double) = 11

            ' This table specifies the "widest" type to be used in For Loops
            ' It should match the results of the Add Operator.

            ForLoopWidestTypeCode = New TypeCode(Max)() _
                { _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int16,   TypeCode.Empty, TypeCode.SByte,   TypeCode.Int16,   TypeCode.Int16,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.SByte,   TypeCode.Empty, TypeCode.SByte,   TypeCode.Int16,   TypeCode.Int16,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int16,   TypeCode.Empty, TypeCode.Int16,   TypeCode.Byte,    TypeCode.Int16,   TypeCode.UInt16,  TypeCode.Int32,   TypeCode.UInt32,  TypeCode.Int64,   TypeCode.UInt64,  TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int16,   TypeCode.Empty, TypeCode.Int16,   TypeCode.Int16,   TypeCode.Int16,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int32,   TypeCode.Empty, TypeCode.Int32,   TypeCode.UInt16,  TypeCode.Int32,   TypeCode.UInt16,  TypeCode.Int32,   TypeCode.UInt32,  TypeCode.Int64,   TypeCode.UInt64,  TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int32,   TypeCode.Empty, TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int32,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int64,   TypeCode.Empty, TypeCode.Int64,   TypeCode.UInt32,  TypeCode.Int64,   TypeCode.UInt32,  TypeCode.Int64,   TypeCode.UInt32,  TypeCode.Int64,   TypeCode.UInt64,  TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Int64,   TypeCode.Empty, TypeCode.Int64,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Int64,   TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Decimal, TypeCode.Empty, TypeCode.Decimal, TypeCode.UInt64,  TypeCode.Decimal, TypeCode.UInt64,  TypeCode.Decimal, TypeCode.UInt64,  TypeCode.Decimal, TypeCode.UInt64,  TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Single,  TypeCode.Empty, TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single,  TypeCode.Single, TypeCode.Double, TypeCode.Single,  TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Double,  TypeCode.Empty, TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double,  TypeCode.Double, TypeCode.Double, TypeCode.Double,  TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Decimal, TypeCode.Empty, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Decimal, TypeCode.Single, TypeCode.Double, TypeCode.Decimal, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty}, _
                    New TypeCode(Max) {TypeCode.Empty, TypeCode.Empty, TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,   TypeCode.Empty,  TypeCode.Empty,  TypeCode.Empty,   TypeCode.Empty, TypeCode.Empty, TypeCode.Empty} _
                }

            VerifyTypeCodeEnum()

#If DEBUG Then
            VerifyForLoopWidestType()
#End If
        End Sub

#If DEBUG Then
        <System.Diagnostics.ConditionalAttribute("DEBUG")> _
        Private Shared Sub VerifyForLoopWidestType()
            Const Max As Integer = TypeCode.String

            For Index1 As Integer = 0 To Max
                Dim tc1 As TypeCode = CType(Index1, TypeCode)

                If IsNumericType(tc1) Then
                    For Index2 As Integer = 0 To Max
                        Dim tc2 As TypeCode = CType(Index2, TypeCode)

                        If IsNumericType(tc2) Then
                            Dim tc as TypeCode = ForLoopWidestTypeCode(tc1)(tc2)

                            Dim Type1 As Type = MapTypeCodeToType(tc1)
                            Dim Type2 As Type = MapTypeCodeToType(tc2)

                            Dim o1 As Object = 0
                            Dim o2 As Object = 0

                            o1 = CType(o1, IConvertible).ToType(Type1, Nothing)
                            o2 = CType(o2, IConvertible).ToType(Type2, Nothing)

                            Dim Result As Object = Operators.AddObject(o1, o2)

                            Debug.Assert(GetTypeCode(Result.GetType()) = tc, "Widest type is invalid")
                        End If
                    Next
                End If
            Next
        End Sub
#End If

        <System.Diagnostics.ConditionalAttribute("DEBUG")> _
        Private Shared Sub VerifyTypeCodeEnum()
            Debug.Assert(TypeCode.Empty = 0, "wrong value!")
            Debug.Assert(TypeCode.Object = 1, "wrong value!")
            Debug.Assert(TypeCode.Boolean = 3, "yte is wrong value!")
            Debug.Assert(TypeCode.Char = 4, "wrong value!")
            Debug.Assert(TypeCode.SByte = 5, "wrong value!")
            Debug.Assert(TypeCode.Byte = 6, "wrong value!")
            Debug.Assert(TypeCode.Int16 = 7, "wrong value!")
            Debug.Assert(TypeCode.UInt16 = 8, "wrong value!")
            Debug.Assert(TypeCode.Int32 = 9, "wrong value!")
            Debug.Assert(TypeCode.UInt32 = 10, "wrong value!")
            Debug.Assert(TypeCode.Int64 = 11, "wrong value!")
            Debug.Assert(TypeCode.UInt64 = 12, "wrong value!")
            Debug.Assert(TypeCode.Single = 13, "wrong value!")
            Debug.Assert(TypeCode.Double = 14, "wrong value!")
            Debug.Assert(TypeCode.Decimal = 15, "wrong value!")
            Debug.Assert(TypeCode.DateTime = 16, "wrong value!")
            Debug.Assert(TypeCode.String = 18, "wrong value!")
        End Sub

        Friend Shared Function ClassifyConversion(ByVal TargetType As System.Type, ByVal SourceType As System.Type, ByRef OperatorMethod As Method) As ConversionClass
            'This function classifies the nature of the conversion from the source type to the target
            'type. If such a conversion requires a user-defined conversion, it will be supplied as an
            'out parameter.

            Debug.Assert(Not TargetType.IsByRef AndAlso Not SourceType.IsByRef, "ByRef types unexpected.")

            Dim Result As ConversionClass = ClassifyPredefinedConversion(TargetType, SourceType)

            If Result = ConversionClass.None AndAlso _
               Not IsInterface(SourceType) AndAlso _
               Not IsInterface(TargetType) AndAlso _
               (IsClassOrValueType(SourceType) OrElse IsClassOrValueType(TargetType)) AndAlso _
               Not (IsIntrinsicType(SourceType) AndAlso IsIntrinsicType(TargetType)) Then

                Result = ClassifyUserDefinedConversion(TargetType, SourceType, OperatorMethod)
            End If

            Return Result

        End Function

        Friend Shared Function ClassifyIntrinsicConversion(ByVal TargetTypeCode As System.TypeCode, ByVal SourceTypeCode As System.TypeCode) As ConversionClass

            Debug.Assert(IsIntrinsicType(TargetTypeCode) AndAlso IsIntrinsicType(SourceTypeCode), "expected intrinsics here")
            Return ConversionTable(TargetTypeCode)(SourceTypeCode)
        End Function

        Friend Shared Function ClassifyPredefinedCLRConversion(ByVal TargetType As System.Type, ByVal SourceType As System.Type) As ConversionClass
            ' This function classifies all intrinsic CLR conversions, such as inheritance,
            ' implementation, and array covariance.

            Debug.Assert(Not TargetType.IsByRef AndAlso Not SourceType.IsByRef, "ByRef types unexpected.")

            'CONSIDER: we can we use IsAssignableFrom to cut out a number of these checks (probably the widening ones)?

            ' *IDENTITY*
            If TargetType Is SourceType Then Return ConversionClass.Identity

            ' *INHERITANCE*
            If IsRootObjectType(TargetType) OrElse IsOrInheritsFrom(SourceType, TargetType) Then
                Return ConversionClass.Widening
            End If

            If IsRootObjectType(SourceType) OrElse IsOrInheritsFrom(TargetType, SourceType) Then
                Return ConversionClass.Narrowing
            End If

            ' *INTERFACE IMPLEMENTATION*
            If IsInterface(SourceType) Then

                If IsClass(TargetType) OrElse IsArrayType(TargetType) OrElse IsGenericParameter(TargetType) Then
                    ' Even if a class is marked NotInheritable, it can still be a COM class and implement
                    ' any interface dynamically at runtime, so we must allow a narrowing conversion.

                    Return ConversionClass.Narrowing
                End If

                If IsInterface(TargetType) Then
                    Return ConversionClass.Narrowing
                End If

                If IsValueType(TargetType) Then
                    If [Implements](TargetType, SourceType) Then
                        Return ConversionClass.Narrowing
                    Else
                        Return ConversionClass.None
                    End If
                End If
#If TELESTO Then
                Debug.Assert(False,"all conversions from interface should have been handled by now")
#Else
                Debug.Fail("all conversions from interface should have been handled by now")
#End If
                Return ConversionClass.Narrowing
            End If

            If IsInterface(TargetType) Then

                If (IsArrayType(SourceType)) Then
                    Return _
                        ClassifyCLRArrayToInterfaceConversion(TargetType, Sourcetype)
                End If

                If IsValueType(SourceType) Then
                    If [Implements](SourceType, TargetType) Then
                        Return ConversionClass.Widening
                    Else
                        Return ConversionClass.None
                    End If
                End If

                If IsClass(SourceType) Then
                    If [Implements](SourceType, TargetType) Then
                        Return ConversionClass.Widening
                    Else
                        Return ConversionClass.Narrowing
                    End If
                End If

                'generic params are handled later
            End If

            ' *ENUMERATION*
            If IsEnum(SourceType) OrElse IsEnum(TargetType) Then

                If GetTypeCode(SourceType) = GetTypeCode(TargetType) Then
                    If IsEnum(TargetType) Then
                        Return ConversionClass.Narrowing
                    Else
                        Return ConversionClass.Widening
                    End If
                End If

                Return ConversionClass.None
            End If

            ' *GENERIC PARAMETERS*
            If IsGenericParameter(SourceType) Then
                If Not IsClassOrInterface(TargetType) Then
                    Return ConversionClass.None
                End If

                'Return the best conversion from any constraint type to the target type.
                For Each InterfaceConstraint As Type In GetInterfaceConstraints(SourceType)
                    Dim Classification As ConversionClass = _
                        ClassifyPredefinedConversion(TargetType, InterfaceConstraint)

                    If Classification = ConversionClass.Widening OrElse _
                       Classification = ConversionClass.Identity Then
                        'A conversion from a constraint type cannot be an identity conversion
                        '(because a conversion operation is necessary in the generated code),
                        'so don't allow it to look any better than Widening.
                        Return ConversionClass.Widening
                    End If
                Next

                Dim ClassConstraint As Type = GetClassConstraint(SourceType)
                If ClassConstraint IsNot Nothing Then
                    Dim Classification As ConversionClass = _
                        ClassifyPredefinedConversion(TargetType, ClassConstraint)

                    If Classification = ConversionClass.Widening OrElse _
                       Classification = ConversionClass.Identity Then
                        'A conversion from a constraint type cannot be an identity conversion
                        '(because a conversion operation is necessary in the generated code),
                        'so don't allow it to look any better than Widening.
                        Return ConversionClass.Widening
                    End If
                End If

                Return IIf(IsInterface(TargetType), ConversionClass.Narrowing, ConversionClass.None)
            End If

            If IsGenericParameter(TargetType) Then
                Debug.Assert(Not IsInterface(SourceType), _
                             "conversions from interfaces should have been handled by now")

                'If one of the constraint types is a class type, a narrowing conversion exists from that class type.
                Dim ClassConstraint As Type = GetClassConstraint(TargetType)
                If ClassConstraint IsNot Nothing AndAlso IsOrInheritsFrom(ClassConstraint, SourceType) Then
                    Return ConversionClass.Narrowing
                End If

                Return ConversionClass.None
            End If

            ' *ARRAY COVARIANCE*
            If IsArrayType(SourceType) AndAlso IsArrayType(TargetType) Then

                If SourceType.GetArrayRank = TargetType.GetArrayRank Then

                    ' The element types must either be the same or
                    ' the source element type must extend or implement the
                    ' target element type. (VB implements array covariance.)

                    Return _
                        ClassifyCLRConversionForArrayElementTypes( _
                            TargetType.GetElementType, _
                            SourceType.GetElementType)

                End If

                Return ConversionClass.None
            End If

            Return ConversionClass.None

        End Function

        Private Shared Function ClassifyCLRArrayToInterfaceConversion(ByVal TargetInterface As System.Type, ByVal SourceArrayType As System.Type) As ConversionClass

            Debug.Assert(IsInterface(TargetInterface), "Non-Interface type unexpected!!!")
            Debug.Assert(IsArrayType(SourceArrayType), "Non-Array type unexpected!!!")

            ' No need to get to System.Array, [Implements] works for arrays with respect to the interfaces on System.Array
            '
            If ([Implements](SourceArrayType, TargetInterface))
                Return ConversionClass.Widening
            End If

            ' Multi-dimensional arrays do not support IList<T>
            '
            If (SourceArrayType.GetArrayRank > 1)
                Return ConversionClass.Narrowing
            End If


            ' Check for the conversion from the Array of element type T to
            ' 1. IList(Of T) - Widening
            ' 2. Some interface that IList(Of T) inherits from - Widening
            ' 3. IList(Of SomeType that T inherits from) - Widening
            '    yes, generics covariance is allowed in the array case
            ' 4. Some interface that IList(Of SomeType that T inherits from)
            '    inherits from - Widening
            ' 5. Some interface that inherits from IList(Of T) - Narrowing
            ' 6. Some interface that inherits from IList(Of SomeType that T inherits from)
            '    - Narrowing
            '
            ' 5 and 6 are not checked for explicitly since from array to interface that
            ' the array does not widen to, we anyway return narrowing.
            '

            Dim SourceElementType As Type = SourceArrayType.GetElementType
            Dim Conversion As ConversionClass = ConversionClass.None

            If (TargetInterface.IsGenericType AndAlso Not TargetInterface.IsGenericTypeDefinition) Then

                Dim RawTargetInterface As Type = TargetInterface.GetGenericTypeDefinition()

                If (RawTargetInterface Is GetType(System.Collections.Generic.IList(Of )) OrElse _
                    RawTargetInterface Is GetType(System.Collections.Generic.ICollection(Of )) OrElse _
                    RawTargetInterface Is GetType(System.Collections.Generic.IEnumerable(Of ))) Then

                    Conversion = _
                        ClassifyCLRConversionForArrayElementTypes( _
                            TargetInterface.GetGenericArguments()(0), _
                            SourceElementType)
                End If

            Else
                Conversion = _
                    ClassifyPredefinedCLRConversion( _
                        TargetInterface, _
                        GetType(System.Collections.Generic.IList(Of )).MakeGenericType(New Type() {SourceElementType}))
            End If


            If (Conversion = ConversionClass.Identity OrElse _
                Conversion = ConversionClass.Widening)

                Return ConversionClass.Widening
            End If

            Return ConversionClass.Narrowing

        End Function


        Private Shared Function ClassifyCLRConversionForArrayElementTypes(ByVal TargetElementType As System.Type, ByVal SourceElementType As System.Type) As ConversionClass

            ' The element types must either be the same or
            ' the source element type must extend or implement the
            ' target element type. (VB implements array covariance.)

            ' Generic params are handled correctly here.

            If IsReferenceType(SourceElementType) AndAlso _
               IsReferenceType(TargetElementType) Then
                Return ClassifyPredefinedCLRConversion(TargetElementType, SourceElementType)
            End If

            If IsValueType(SourceElementType) AndAlso _
               IsValueType(TargetElementType) Then
                Return ClassifyPredefinedCLRConversion(TargetElementType, SourceElementType)
            End If

            ' Bug VSWhidbey 369131.
            ' Array co-variance and back-casting special case for generic parameters.
            '
            If IsGenericParameter(SourceElementType) AndAlso _
               IsGenericParameter(TargetElementType) Then

                If SourceElementType Is TargetElementType Then
                    Return ConversionClass.Identity
                End If

                If IsReferenceType(SourceElementType) AndAlso _
                   IsOrInheritsFrom(SourceElementType, TargetElementType) Then
                    Return ConversionClass.Widening
                End If

                If IsReferenceType(TargetElementType) AndAlso _
                   IsOrInheritsFrom(TargetElementType, SourceElementType) Then
                    Return ConversionClass.Narrowing
                End If
            End If

            Return ConversionClass.None
        End Function


        Friend Shared Function ClassifyPredefinedConversion(ByVal TargetType As System.Type, ByVal SourceType As System.Type) As ConversionClass
            ' This function classifies all intrinsic language conversions, such as inheritance,
            ' implementation, array covariance, and conversions between intrinsic types.

            Debug.Assert(Not TargetType.IsByRef AndAlso Not SourceType.IsByRef, "ByRef types unexpected.")

            ' Make an easy reference comparison for a common case.  More complicated type comparisons will happen later.
            If TargetType Is SourceType Then Return ConversionClass.Identity

            Dim SourceTypeCode As TypeCode = GetTypeCode(SourceType)
            Dim TargetTypeCode As TypeCode = GetTypeCode(TargetType)

            If (IsIntrinsicType(SourceTypeCode) AndAlso IsIntrinsicType(TargetTypeCode)) Then

                If IsEnum(TargetType) Then
                    If IsIntegralType(SourceTypeCode) AndAlso IsIntegralType(TargetTypeCode) Then
                        ' Conversion from an integral type (including an Enum type)
                        ' to an Enum type (that has an integral underlying type)
                        ' is narrowing. Enums do not necessarily have integral underlying types.
                        Return ConversionClass.Narrowing
                    End If
                End If

                If SourceTypeCode = TargetTypeCode AndAlso IsEnum(SourceType) Then
                    ' Conversion from an Enum to it's underlying type is widening.
                    ' If we used ClassifyIntrinsicConversion, this kind of conversion
                    ' would be classified as identity, and that would not be good.
                    ' Catch this case here.
                    Return ConversionClass.Widening
                End If

                Return ClassifyIntrinsicConversion(TargetTypeCode, SourceTypeCode)

            End If

            ' Try VB specific conversions from String-->Char() or Char()-->String.

            If IsCharArrayRankOne(SourceType) AndAlso IsStringType(TargetType) Then
                ' Array of Char widens to String.
                Return ConversionClass.Widening
            End If

            If IsCharArrayRankOne(TargetType) AndAlso IsStringType(SourceType) Then
                ' String narrows to array of Char.
                Return ConversionClass.Narrowing
            End If

            Return ClassifyPredefinedCLRConversion(TargetType, SourceType)

        End Function

        Private Shared Function CollectConversionOperators( _
            ByVal TargetType As System.Type, _
            ByVal SourceType As System.Type, _
            ByRef FoundTargetTypeOperators As Boolean, _
            ByRef FoundSourceTypeOperators As Boolean) As List(Of Method)

            'Find all Widening and Narrowing conversion operators. Combine the lists
            'with the Widening operators grouped at the front.

            'From the perspective of VB, intrinsic types have no conversion operators.
            'Substitute in Object for these types.
            If IsIntrinsicType(TargetType) Then TargetType = GetType(Object)
            If IsIntrinsicType(SourceType) Then SourceType = GetType(Object)

            Dim Result As List(Of Method) = _
                Operators.CollectOperators( _
                    UserDefinedOperator.Widen, _
                    TargetType, _
                    SourceType, _
                    FoundTargetTypeOperators, _
                    FoundSourceTypeOperators)

            Dim NarrowingOperators As List(Of Method) = _
                Operators.CollectOperators( _
                    UserDefinedOperator.Narrow, _
                    TargetType, _
                    SourceType, _
                    FoundTargetTypeOperators, _
                    FoundSourceTypeOperators)

            Result.AddRange(NarrowingOperators)
            Return Result
        End Function

        Private Shared Function Encompasses(ByVal Larger As System.Type, ByVal Smaller As System.Type) As Boolean
            'Definition: LARGER is said to encompass SMALLER if SMALLER widens to or is LARGER.

            'CONSIDER: since determining encompasses is quite commonly used,
            'and only depends on widening or identity, a special function for classifying
            'just predefined widening conversions could be a performance gain.
            Dim Result As ConversionClass = _
                ClassifyPredefinedConversion(Larger, Smaller)

            Return Result = ConversionClass.Widening OrElse Result = ConversionClass.Identity
        End Function

        Private Shared Function NotEncompasses(ByVal Larger As System.Type, ByVal Smaller As System.Type) As Boolean
            'Definition: LARGER is said to not encompass SMALLER if SMALLER narrows to or is LARGER.

            'CONSIDER: since determining encompasses is quite commonly used,
            'and only depends on widening or identity, a special function for classifying
            'just predefined narrowing conversions could be a performance gain.
            Dim Result As ConversionClass = _
                ClassifyPredefinedConversion(Larger, Smaller)

            Return Result = ConversionClass.Narrowing OrElse Result = ConversionClass.Identity
        End Function


        Private Shared Function MostEncompassing(ByVal Types As List(Of System.Type)) As System.Type
            'Given a set TYPES, determine the most encompassing type. An element
            'CANDIDATE of TYPES is said to be most encompassing if no other element of
            'TYPES encompasses CANDIDATE.

            Debug.Assert(Types.Count > 0, "unexpected empty set")
            Dim MaxEncompassing As System.Type = Types.Item(0)

            For Index As Integer = 1 To Types.Count - 1
                Dim Candidate As Type = Types.Item(Index)

                If Encompasses(Candidate, MaxEncompassing) Then
                    Debug.Assert(Candidate Is MaxEncompassing OrElse Not Encompasses(MaxEncompassing, Candidate), _
                                 "surprisingly, two types encompass each other")
                    MaxEncompassing = Candidate
                ElseIf Not Encompasses(MaxEncompassing, Candidate) Then
                    'We have detected more than one most encompassing type in the set.
                    'Return Nothing to indicate this error condition.
                    Return Nothing
                End If
            Next

            Return MaxEncompassing
        End Function


        Private Shared Function MostEncompassed(ByVal Types As List(Of System.Type)) As System.Type
            'Given a set TYPES, determine the most encompassed type. An element
            'CANDIDATE of TYPES is said to be most encompassed if CANDIDATE encompasses
            'no other element of TYPES.

            Debug.Assert(Types.Count > 0, "unexpected empty set")

            Dim MaxEncompassed As System.Type = Types.Item(0)

            For Index As Integer = 1 To Types.Count - 1
                Dim Candidate As Type = Types.Item(Index)

                If Encompasses(MaxEncompassed, Candidate) Then
                    Debug.Assert(Candidate Is MaxEncompassed OrElse Not Encompasses(Candidate, MaxEncompassed), _
                                 "surprisingly, two types encompass each other")
                    MaxEncompassed = Candidate
                ElseIf Not Encompasses(Candidate, MaxEncompassed) Then
                    'We have detected more than one most encompassed type in the set.
                    'Return Nothing to indicate this error condition.
                    Return Nothing
                End If
            Next

            Return MaxEncompassed
        End Function

        Private Shared Sub FindBestMatch( _
            ByVal TargetType As Type, _
            ByVal SourceType As Type, _
            ByVal SearchList As List(Of Method), _
            ByVal ResultList As List(Of Method), _
            ByRef GenericMembersExistInList As Boolean)

            'Given a set of conversion operators which convert from INPUT to RESULT, return the set
            'of operators for which INPUT is SOURCE and RESULT is TARGET.

            For Each Item As Method In SearchList
                Dim Current As MethodBase = Item.AsMethod
                Dim InputType As System.Type = Current.GetParameters(0).ParameterType
                Dim ResultType As System.Type = DirectCast(Current, MethodInfo).ReturnType

                If InputType Is SourceType AndAlso ResultType Is TargetType Then
                    InsertInOperatorListIfLessGenericThanExisting(Item, ResultList, GenericMembersExistInList)
                End If
            Next
            Return

        End Sub

        Private Shared Sub InsertInOperatorListIfLessGenericThanExisting( _
            Byval OperatorToInsert As Method, _
            ByVal OperatorList As List(Of Method), _
            Byref GenericMembersExistInList As Boolean)

            If IsGeneric(OperatorToInsert.DeclaringType) Then
                GenericMembersExistInList = True
            End If

            If GenericMembersExistInList Then

                For i as Integer = OperatorList.Count -1 to 0 Step -1

                    Dim Existing As Method = OperatorList.Item(i)
                    Dim LeastGeneric As Method = OverloadResolution.LeastGenericProcedure(Existing, OperatorToInsert)

                    If LeastGeneric Is Existing Then
                        ' An existing one is less generic than the current operator being
                        ' considered, so skip adding the current operator to the operator
                        ' list.
                        Return

                    Else If LeastGeneric IsNot Nothing Then
                        ' The current operator is less generic than an existing operator,
                        ' so remove the existing operator from the list and continue to
                        ' check if any other exisiting operator can be removed from the
                        ' result set.
                        '
                        OperatorList.Remove(Existing)
                    End If
                Next
            End If

            OperatorList.Add(OperatorToInsert)
        End Sub

        Private Shared Function ResolveConversion( _
            ByVal TargetType As System.Type, _
            ByVal SourceType As System.Type, _
            ByVal OperatorSet As List(Of Method), _
            ByVal WideningOnly As Boolean, _
            ByRef ResolutionIsAmbiguous As Boolean) As List(Of Method)


            'This function resolves which user-defined conversion operator contained in the input set
            'can be used to perform the conversion from source type S to target type T.
            '
            'The algorithm defies succinct explaination, but roughly:
            '
            'Conversions of the form S-->T use only one user-defined conversion at a time, i.e.,
            'user-defined conversions are not chained together.  It may be necessary to convert to and
            'from intermediate types using predefined conversions to match the signature of the
            'user-defined conversion exactly, so the conversion "path" is comprised of at most three
            'parts:
            '
            '    1) [ predefined conversion  S-->Sx ]
            '    2) User-defined conversion Sx-->Tx
            '    3) [ predefined conversion Tx-->T  ]
            '
            '    Where Sx is the intermediate source type
            '      and Tx is the intermediate target type
            '
            '    Steps 1 and 3 are optional given S == Sx or Tx == T.
            '
            'Much of the algorithm below concerns itself with finding Sx and Tx.  The rules are:
            '
            '    - If a conversion operator in the set converts from S, then Sx is S.
            '    - If a conversion operator in the set converts to T, then Tx is T.
            '    - Otherwise Sx and Tx are the "closest" types to S and T.  If multiple types are
            '      equally close, the conversion is ambiguous.
            '
            'Each operator presents a possibility for Sx (the parameter type of the operator).  Given
            'these choices, the "closest" type to S is the smallest (most encompassed) type that S
            'widens to.  If S widens to none of the possible types, then the "closest" type to S is
            'the largest (most encompassing) type that widens to S.  In this way, the algorithm
            'always prefers widening from S over narrowing from S.
            '
            'Similarily, each operator presents a possibility for Tx (the return type of the operator).
            'Given these choices, the "closest" type to T is the largest (most encompassing) type that
            'widens to T.  If none of the possible types widen to T, then the "closest" type to T is
            'the smallest (most encompassed) type that T widens to.  In this way, the algorithm
            'always prefers widening to T over narrowing to T.
            '
            'Upon deciding Sx and Tx, if one operator's operands exactly match Sx and Tx, then that
            'operator is chosen.  If no operators match, or if multiple operators match, the conversion
            'is impossible.
            '
            'Refer to the language specification as it covers all details of the algorithm.

            ResolutionIsAmbiguous = False

            Dim MostSpecificSourceType As System.Type = Nothing
            Dim MostSpecificTargetType As System.Type = Nothing

            Dim GenericOperatorChoicesFound As Boolean = False
            Dim OperatorChoices As List(Of Method) = New List(Of Method)(OperatorSet.Count)
            Dim Candidates As List(Of Method) = New List(Of Method)(OperatorSet.Count)

            Dim SourceBases As List(Of Type) = New List(Of Type)(OperatorSet.Count)
            Dim TargetDeriveds As List(Of Type) = New List(Of Type)(OperatorSet.Count)
            Dim SourceDeriveds As List(Of Type) = Nothing
            Dim TargetBases As List(Of Type) = Nothing

            If Not WideningOnly Then
                SourceDeriveds = New List(Of Type)(OperatorSet.Count)
                TargetBases = New List(Of Type)(OperatorSet.Count)
            End If

            'To minimize the number of calls to Encompasses, we categorize conversions
            'into three flavors:
            '
            '   1) Base of Source to Derived of Target (only flavor that can be completely widening)
            '   2) Base of Source to Base of Target
            '   3) Derived of Source to Base of Target
            '
            'For each flavor, we place the input and result type into the corresponding
            'type set. Then we calculate most encompassing/encompassed using the type sets.

            For Each CurrentMethod As Method In OperatorSet

                Dim Current As MethodBase = CurrentMethod.AsMethod

                'Performance trick: the operators are grouped by widening and then narrowing
                'conversions. If we are iterating over just widening conversions, we are done
                'once we find a narrowing conversion.
                If WideningOnly AndAlso IsNarrowingConversionOperator(Current) Then Exit For

                Dim InputType As System.Type = Current.GetParameters(0).ParameterType
                Dim ResultType As System.Type = DirectCast(Current, MethodInfo).ReturnType

                If (IsGeneric(Current) OrElse _
                       IsGeneric(Current.DeclaringType)) AndAlso _
                   ClassifyPredefinedConversion(ResultType, InputType) <> ConversionClass.None Then
                    Continue For
                End If

                If InputType Is SourceType AndAlso ResultType Is TargetType Then
                    InsertInOperatorListIfLessGenericThanExisting(CurrentMethod, OperatorChoices, GenericOperatorChoicesFound)

                ElseIf OperatorChoices.Count = 0 Then

                    If Encompasses(InputType, SourceType) AndAlso Encompasses(TargetType, ResultType) Then
                        'Check SourceBase->TargetDerived flavor.

                        Candidates.Add(CurrentMethod)
                        If InputType Is SourceType Then MostSpecificSourceType = InputType Else SourceBases.Add(InputType)
                        If ResultType Is TargetType Then MostSpecificTargetType = ResultType Else TargetDeriveds.Add(ResultType)

                    ElseIf Not WideningOnly AndAlso _
                           Encompasses(InputType, SourceType) AndAlso NotEncompasses(TargetType, ResultType) Then
                        'Check SourceBase->TargetBase flavor.

                        Candidates.Add(CurrentMethod)
                        If InputType Is SourceType Then MostSpecificSourceType = InputType Else SourceBases.Add(InputType)
                        If ResultType Is TargetType Then MostSpecificTargetType = ResultType Else TargetBases.Add(ResultType)

                    ElseIf Not WideningOnly AndAlso _
                           NotEncompasses(InputType, SourceType) AndAlso NotEncompasses(TargetType, ResultType) Then
                        'Check SourceDerived->TargetBase flavor.

                        Candidates.Add(CurrentMethod)
                        If InputType Is SourceType Then MostSpecificSourceType = InputType Else SourceDeriveds.Add(InputType)
                        If ResultType Is TargetType Then MostSpecificTargetType = ResultType Else TargetBases.Add(ResultType)

                    End If

                End If
            Next

            'Now attempt to find the most specific types Sx and Tx by analyzing the type sets
            'we built up in the code above.

            If OperatorChoices.Count = 0 AndAlso Candidates.Count > 0 Then

                If MostSpecificSourceType Is Nothing Then
                    If SourceBases.Count > 0 Then
                        MostSpecificSourceType = MostEncompassed(SourceBases)
                    Else
                        Debug.Assert(Not WideningOnly AndAlso SourceDeriveds.Count > 0, "unexpected state")
                        MostSpecificSourceType = MostEncompassing(SourceDeriveds)
                    End If
                End If

                If MostSpecificTargetType Is Nothing Then
                    If TargetDeriveds.Count > 0 Then
                        MostSpecificTargetType = MostEncompassing(TargetDeriveds)
                    Else
                        Debug.Assert(Not WideningOnly AndAlso TargetBases.Count > 0, "unexpected state")
                        MostSpecificTargetType = MostEncompassed(TargetBases)
                    End If
                End If

                If MostSpecificSourceType Is Nothing OrElse MostSpecificTargetType Is Nothing Then
                    ResolutionIsAmbiguous = True
                    Return New List(Of Method)
                End If

                FindBestMatch(MostSpecificTargetType, MostSpecificSourceType, Candidates, OperatorChoices, GenericOperatorChoicesFound)

            End If

            If OperatorChoices.Count > 1 Then
                ResolutionIsAmbiguous = True
            End If

            Return OperatorChoices

        End Function

        Friend Shared Function ClassifyUserDefinedConversion( _
            ByVal TargetType As System.Type, _
            ByVal SourceType As System.Type, _
            ByRef OperatorMethod As Method) As ConversionClass

            Dim Result As ConversionClass

            'Check if we have done this classification before.
            SyncLock (ConversionCache)
                'First check if both types have no user-defined conversion operators. If so, they cannot
                'convert to each other with user-defined operators.
                If UnconvertibleTypeCache.Lookup(TargetType) AndAlso UnconvertibleTypeCache.Lookup(SourceType) Then
                    Return ConversionClass.None
                End If

                'Now check if we have recently resolved this conversion.
                If ConversionCache.Lookup(TargetType, SourceType, Result, OperatorMethod) Then
                    Return Result
                End If
            End SyncLock

            'Perform the expensive work to resolve the user-defined conversion.
            Dim FoundTargetTypeOperators As Boolean = False
            Dim FoundSourceTypeOperators As Boolean = False
            Result = _
                DoClassifyUserDefinedConversion( _
                    TargetType, _
                    SourceType, _
                    OperatorMethod, _
                    FoundTargetTypeOperators, _
                    FoundSourceTypeOperators)

            'Save away the results.
            SyncLock (ConversionCache)
                'Remember which types have no operators so we can avoid re-doing the work next time.
                If Not FoundTargetTypeOperators Then
                    UnconvertibleTypeCache.Insert(TargetType)
                End If

                If Not FoundSourceTypeOperators Then
                    UnconvertibleTypeCache.Insert(SourceType)
                End If

                If FoundTargetTypeOperators OrElse FoundSourceTypeOperators Then
                    'Cache the result of the resolution so we can avoid re-doing the work next time, but
                    'only when conversion operators were found (otherwise, the type caches will catch this
                    'the next time).
                    ConversionCache.Insert(TargetType, SourceType, Result, OperatorMethod)
                End If
            End SyncLock

            Return Result
        End Function

        Private Shared Function DoClassifyUserDefinedConversion( _
            ByVal TargetType As System.Type, _
            ByVal SourceType As System.Type, _
            ByRef OperatorMethod As Method, _
            ByRef FoundTargetTypeOperators As Boolean, _
            ByRef FoundSourceTypeOperators As Boolean) As ConversionClass

            'Classifies the conversion from Source to Target using user-defined conversion operators.
            'If such a conversion exists, it will be supplied as an out parameter.
            '
            'The result is a widening conversion from Source to Target if such a conversion exists.
            'Otherwise the result is a narrowing conversion if such a conversion exists.  Otherwise
            'no conversion is possible.  We perform this two pass process because the conversion
            '"path" is not affected by the user implicitly or explicitly specifying the conversion.
            '
            'In other words, a safe (widening) conversion is always taken regardless of whether
            'Option Strict is on or off.

            Debug.Assert(ClassifyPredefinedConversion(TargetType, SourceType) = ConversionClass.None, _
                         "predefined conversion is possible, so why try user-defined?")

            OperatorMethod = Nothing

            Dim OperatorSet As List(Of Method) = _
                CollectConversionOperators( _
                    TargetType, _
                    SourceType, _
                    FoundTargetTypeOperators, _
                    FoundSourceTypeOperators)

            If OperatorSet.Count = 0 Then
                'No conversion operators, so no conversion is possible.
                Return ConversionClass.None
            End If

            Dim ResolutionIsAmbiguous As Boolean = False

            Dim OperatorChoices As List(Of Method) = _
                ResolveConversion( _
                    TargetType, _
                    SourceType, _
                    OperatorSet, _
                    True, _
                    ResolutionIsAmbiguous)

            If OperatorChoices.Count = 1 Then
                OperatorMethod = OperatorChoices.Item(0)
                OperatorMethod.ArgumentsValidated = True
                'The result from the first pass is necessarily widening.
                Return ConversionClass.Widening

            ElseIf OperatorChoices.Count = 0 AndAlso Not ResolutionIsAmbiguous Then

                Debug.Assert(OperatorSet.Count > 0, "expected operators")

                'Second pass: if the first pass failed, attempt to find a conversion
                'considering BOTH widening and narrowing.

                OperatorChoices = _
                    ResolveConversion( _
                        TargetType, _
                        SourceType, _
                        OperatorSet, _
                        False, _
                        ResolutionIsAmbiguous)

                If OperatorChoices.Count = 1 Then
                    OperatorMethod = OperatorChoices.Item(0)
                    OperatorMethod.ArgumentsValidated = True
                    'The result from the second pass is necessarily narrowing.
                    Return ConversionClass.Narrowing

                ElseIf OperatorChoices.Count = 0 Then
                    'No conversion possible.
                    Return ConversionClass.None

                End If

            End If

            'CONSIDER: If error reporting is improved for conversion resolution,
            ' create a useful error message here.
            'Conversion is ambiguous.
            Return ConversionClass.Ambiguous

        End Function


    End Class

    Friend Class OperatorCaches
        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend NotInheritable Class FixedList

            Private Structure Entry
                Friend TargetType As Type
                Friend SourceType As Type
                Friend Classification As ConversionClass
                Friend OperatorMethod As Method
                Friend [Next] As Integer
                Friend Previous As Integer
            End Structure

            Private ReadOnly m_List As Entry()
            Private ReadOnly m_Size As Integer
            Private m_First As Integer
            Private m_Last As Integer
            Private m_Count As Integer

            Private Const DefaultSize As Integer = 50

            Friend Sub New()
                MyClass.New(DefaultSize)
            End Sub

            Friend Sub New(ByVal Size As Integer)
                'Populate the cache list with the maximum number of entires.
                'This simplifies the insertion code for a small upfront cost.
                m_Size = Size

                m_List = New Entry(m_Size - 1) {}
                For Index As Integer = 0 To m_Size - 2
                    m_List(Index).Next = Index + 1
                Next
                For Index As Integer = m_Size - 1 To 1 Step -1
                    m_List(Index).Previous = Index - 1
                Next
                m_List(0).Previous = m_Size - 1
                m_Last = m_Size - 1
            End Sub

            Private Sub MoveToFront(ByVal Item As Integer)
                'Remove Item from its position in the list and move it to the front.
                If Item = m_First Then Return

                Dim [Next] As Integer = m_List(Item).Next
                Dim Previous As Integer = m_List(Item).Previous

                m_List(Previous).Next = [Next]
                m_List([Next]).Previous = Previous

                m_List(m_First).Previous = Item
                m_List(m_Last).Next = Item

                m_List(Item).Next = m_First
                m_List(Item).Previous = m_Last

                m_First = Item
            End Sub

            Friend Sub Insert( _
                ByVal TargetType As Type, _
                ByVal SourceType As Type, _
                ByVal Classification As ConversionClass, _
                ByVal OperatorMethod As Method)

                If m_Count < m_Size Then m_Count += 1

                'Replace the least used conversion in the list with a new conversion, and move
                'that entry to the front.

                Dim Item As Integer = m_Last
                m_First = Item
                m_Last = m_List(m_Last).Previous

                m_List(Item).TargetType = TargetType
                m_List(Item).SourceType = SourceType
                m_List(Item).Classification = Classification
                m_List(Item).OperatorMethod = OperatorMethod
            End Sub

            Friend Function Lookup( _
                ByVal TargetType As Type, _
                ByVal SourceType As Type, _
                ByRef Classification As ConversionClass, _
                ByRef OperatorMethod As Method) As Boolean

                Dim Item As Integer = m_First
                Dim Iteration As Integer = 0

                Do While Iteration < m_Count
                    If TargetType Is m_List(Item).TargetType AndAlso SourceType Is m_List(Item).SourceType Then
                        Classification = m_List(Item).Classification
                        OperatorMethod = m_List(Item).OperatorMethod
                        MoveToFront(Item)
                        Return True
                    End If
                    Item = m_List(Item).Next
                    Iteration += 1
                Loop

                Classification = ConversionClass.Bad
                OperatorMethod = Nothing
                Return False
            End Function

        End Class

        Friend NotInheritable Class FixedExistanceList

            Private Structure Entry
                Friend Type As Type
                Friend [Next] As Integer
                Friend Previous As Integer
            End Structure

            Private ReadOnly m_List As Entry()
            Private ReadOnly m_Size As Integer
            Private m_First As Integer
            Private m_Last As Integer
            Private m_Count As Integer

            Private Const DefaultSize As Integer = 50

            Friend Sub New()
                MyClass.New(DefaultSize)
            End Sub

            Friend Sub New(ByVal Size As Integer)
                'Populate the list with the maximum number of entires.
                'This simplifies the insertion code for a small upfront cost.
                m_Size = Size

                m_List = New Entry(m_Size - 1) {}
                For Index As Integer = 0 To m_Size - 2
                    m_List(Index).Next = Index + 1
                Next
                For Index As Integer = m_Size - 1 To 1 Step -1
                    m_List(Index).Previous = Index - 1
                Next
                m_List(0).Previous = m_Size - 1
                m_Last = m_Size - 1
            End Sub

            Private Sub MoveToFront(ByVal Item As Integer)
                'Remove Item from its position in the list and move it to the front.
                If Item = m_First Then Return

                Dim [Next] As Integer = m_List(Item).Next
                Dim Previous As Integer = m_List(Item).Previous

                m_List(Previous).Next = [Next]
                m_List([Next]).Previous = Previous

                m_List(m_First).Previous = Item
                m_List(m_Last).Next = Item

                m_List(Item).Next = m_First
                m_List(Item).Previous = m_Last

                m_First = Item
            End Sub

            Friend Sub Insert(ByVal Type As Type)

                If m_Count < m_Size Then m_Count += 1

                'Replace the least used conversion in the cache with a new conversion, and move
                'that entry to the front.

                Dim Item As Integer = m_Last
                m_First = Item
                m_Last = m_List(m_Last).Previous

                m_List(Item).Type = Type
            End Sub

            Friend Function Lookup(ByVal Type As Type) As Boolean

                Dim Item As Integer = m_First
                Dim Iteration As Integer = 0

                Do While Iteration < m_Count
                    If Type Is m_List(Item).Type Then
                        MoveToFront(Item)
                        Return True
                    End If
                    Item = m_List(Item).Next
                    Iteration += 1
                Loop

                Return False
            End Function

        End Class

        Friend Shared ReadOnly ConversionCache As FixedList
        Friend Shared ReadOnly UnconvertibleTypeCache As FixedExistanceList

        Shared Sub New()
            ConversionCache = New FixedList
            UnconvertibleTypeCache = New FixedExistanceList
        End Sub

    End Class

End Namespace
