' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Reflection
Imports System.Diagnostics
Imports System.Collections
Imports System.Collections.Generic
#If Not TELESTO Then
Imports System.Runtime.Remoting
#End If

Imports Microsoft.VisualBasic.CompilerServices.NewLateBinding
Imports Microsoft.VisualBasic.CompilerServices.OverloadResolution
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Utils

Namespace Microsoft.VisualBasic.CompilerServices

    Friend Class Symbols
        ' Prevent creation.
        Private Sub New()
        End Sub

        Friend Enum UserDefinedOperator As SByte
            UNDEF
            Narrow
            Widen
            IsTrue
            IsFalse
            Negate
            [Not]
            UnaryPlus
            Plus
            Minus
            Multiply
            Divide
            Power
            IntegralDivide
            Concatenate
            ShiftLeft
            ShiftRight
            Modulus
            [Or]
            [Xor]
            [And]
            [Like]
            Equal
            NotEqual
            Less
            LessEqual
            GreaterEqual
            Greater
            MAX
        End Enum

        Friend Shared ReadOnly NoArguments As Object() = {}
        Friend Shared ReadOnly NoArgumentNames As String() = {}
        Friend Shared ReadOnly NoTypeArguments As Type() = {}
        Friend Shared ReadOnly NoTypeParameters As Type() = {}

        Friend Shared ReadOnly OperatorCLSNames As String()
        Friend Shared ReadOnly OperatorNames As String()

        Shared Sub New()
            OperatorCLSNames = New String(UserDefinedOperator.MAX - 1) {}
            OperatorCLSNames(UserDefinedOperator.Narrow) = "op_Explicit"
            OperatorCLSNames(UserDefinedOperator.Widen) = "op_Implicit"
            OperatorCLSNames(UserDefinedOperator.IsTrue) = "op_True"
            OperatorCLSNames(UserDefinedOperator.IsFalse) = "op_False"
            OperatorCLSNames(UserDefinedOperator.Negate) = "op_UnaryNegation"
            OperatorCLSNames(UserDefinedOperator.Not) = "op_OnesComplement"
            OperatorCLSNames(UserDefinedOperator.UnaryPlus) = "op_UnaryPlus"
            OperatorCLSNames(UserDefinedOperator.Plus) = "op_Addition"
            OperatorCLSNames(UserDefinedOperator.Minus) = "op_Subtraction"
            OperatorCLSNames(UserDefinedOperator.Multiply) = "op_Multiply"
            OperatorCLSNames(UserDefinedOperator.Divide) = "op_Division"
            OperatorCLSNames(UserDefinedOperator.Power) = "op_Exponent"
            OperatorCLSNames(UserDefinedOperator.IntegralDivide) = "op_IntegerDivision"
            OperatorCLSNames(UserDefinedOperator.Concatenate) = "op_Concatenate"
            OperatorCLSNames(UserDefinedOperator.ShiftLeft) = "op_LeftShift"
            OperatorCLSNames(UserDefinedOperator.ShiftRight) = "op_RightShift"
            OperatorCLSNames(UserDefinedOperator.Modulus) = "op_Modulus"
            OperatorCLSNames(UserDefinedOperator.Or) = "op_BitwiseOr"
            OperatorCLSNames(UserDefinedOperator.Xor) = "op_ExclusiveOr"
            OperatorCLSNames(UserDefinedOperator.And) = "op_BitwiseAnd"
            OperatorCLSNames(UserDefinedOperator.Like) = "op_Like"
            OperatorCLSNames(UserDefinedOperator.Equal) = "op_Equality"
            OperatorCLSNames(UserDefinedOperator.NotEqual) = "op_Inequality"
            OperatorCLSNames(UserDefinedOperator.Less) = "op_LessThan"
            OperatorCLSNames(UserDefinedOperator.LessEqual) = "op_LessThanOrEqual"
            OperatorCLSNames(UserDefinedOperator.GreaterEqual) = "op_GreaterThanOrEqual"
            OperatorCLSNames(UserDefinedOperator.Greater) = "op_GreaterThan"


            OperatorNames = New String(UserDefinedOperator.MAX - 1) {}
            OperatorNames(UserDefinedOperator.Narrow) = "CType"
            OperatorNames(UserDefinedOperator.Widen) = "CType"
            OperatorNames(UserDefinedOperator.IsTrue) = "IsTrue"
            OperatorNames(UserDefinedOperator.IsFalse) = "IsFalse"
            OperatorNames(UserDefinedOperator.Negate) = "-"
            OperatorNames(UserDefinedOperator.Not) = "Not"
            OperatorNames(UserDefinedOperator.UnaryPlus) = "+"
            OperatorNames(UserDefinedOperator.Plus) = "+"
            OperatorNames(UserDefinedOperator.Minus) = "-"
            OperatorNames(UserDefinedOperator.Multiply) = "*"
            OperatorNames(UserDefinedOperator.Divide) = "/"
            OperatorNames(UserDefinedOperator.Power) = "^"
            OperatorNames(UserDefinedOperator.IntegralDivide) = "\"
            OperatorNames(UserDefinedOperator.Concatenate) = "&"
            OperatorNames(UserDefinedOperator.ShiftLeft) = "<<"
            OperatorNames(UserDefinedOperator.ShiftRight) = ">>"
            OperatorNames(UserDefinedOperator.Modulus) = "Mod"
            OperatorNames(UserDefinedOperator.Or) = "Or"
            OperatorNames(UserDefinedOperator.Xor) = "Xor"
            OperatorNames(UserDefinedOperator.And) = "And"
            OperatorNames(UserDefinedOperator.Like) = "Like"
            OperatorNames(UserDefinedOperator.Equal) = "="
            OperatorNames(UserDefinedOperator.NotEqual) = "<>"
            OperatorNames(UserDefinedOperator.Less) = "<"
            OperatorNames(UserDefinedOperator.LessEqual) = "<="
            OperatorNames(UserDefinedOperator.GreaterEqual) = ">="
            OperatorNames(UserDefinedOperator.Greater) = ">"
        End Sub

        Friend Shared Function IsUnaryOperator(ByVal Op As UserDefinedOperator) As Boolean
            Select Case Op
                Case UserDefinedOperator.Narrow, _
                     UserDefinedOperator.Widen, _
                     UserDefinedOperator.IsTrue, _
                     UserDefinedOperator.IsFalse, _
                     UserDefinedOperator.Negate, _
                     UserDefinedOperator.Not, _
                     UserDefinedOperator.UnaryPlus

                    Return True

            End Select
            Return False
        End Function

        Friend Shared Function IsBinaryOperator(ByVal Op As UserDefinedOperator) As Boolean
            Select Case Op
                Case UserDefinedOperator.Plus, _
                     UserDefinedOperator.Minus, _
                     UserDefinedOperator.Multiply, _
                     UserDefinedOperator.Divide, _
                     UserDefinedOperator.Power, _
                     UserDefinedOperator.IntegralDivide, _
                     UserDefinedOperator.Concatenate, _
                     UserDefinedOperator.ShiftLeft, _
                     UserDefinedOperator.ShiftRight, _
                     UserDefinedOperator.Modulus, _
                     UserDefinedOperator.Or, _
                     UserDefinedOperator.Xor, _
                     UserDefinedOperator.And, _
                     UserDefinedOperator.Like, _
                     UserDefinedOperator.Equal, _
                     UserDefinedOperator.NotEqual, _
                     UserDefinedOperator.Less, _
                     UserDefinedOperator.LessEqual, _
                     UserDefinedOperator.GreaterEqual, _
                     UserDefinedOperator.Greater

                    Return True

            End Select
            Return False
        End Function

        Friend Shared Function IsUserDefinedOperator(ByVal Method As MethodBase) As Boolean
            Return Method.IsSpecialName AndAlso Method.Name.StartsWith("op_", StringComparison.Ordinal)
        End Function

        Friend Shared Function IsNarrowingConversionOperator(ByVal Method As MethodBase) As Boolean
            Return Method.IsSpecialName AndAlso Method.Name.Equals(OperatorCLSNames(UserDefinedOperator.Narrow))
        End Function

        Friend Shared Function MapToUserDefinedOperator(ByVal Method As MethodBase) As UserDefinedOperator
            Debug.Assert(IsUserDefinedOperator(Method), "expected operator here")

            For Cursor As Integer = UserDefinedOperator.UNDEF + 1 To UserDefinedOperator.MAX - 1
                If Method.Name.Equals(OperatorCLSNames(Cursor)) Then

                    Dim ParamCount As Integer = Method.GetParameters.Length
                    Dim Op As UserDefinedOperator = CType(Cursor, UserDefinedOperator)

                    If (ParamCount = 1 AndAlso IsUnaryOperator(Op)) OrElse _
                       (ParamCount = 2 AndAlso IsBinaryOperator(Op)) Then
                        'Match found, so quit loop early.
                        Return Op
                    End If

                End If
            Next

            Return UserDefinedOperator.UNDEF
        End Function

        Friend Shared Function GetTypeCode(ByVal Type As System.Type) As TypeCode
            Return System.Type.GetTypeCode(Type)
        End Function

        Friend Shared Function MapTypeCodeToType(ByVal TypeCode As TypeCode) As Type

            Select Case TypeCode

                Case TypeCode.Boolean : Return GetType(Boolean)
                Case TypeCode.SByte : Return GetType(SByte)
                Case TypeCode.Byte : Return GetType(Byte)
                Case TypeCode.Int16 : Return GetType(Short)
                Case TypeCode.UInt16 : Return GetType(UShort)
                Case TypeCode.Int32 : Return GetType(Integer)
                Case TypeCode.UInt32 : Return GetType(UInteger)
                Case TypeCode.Int64 : Return GetType(Long)
                Case TypeCode.UInt64 : Return GetType(ULong)
                Case TypeCode.Decimal : Return GetType(Decimal)
                Case TypeCode.Single : Return GetType(Single)
                Case TypeCode.Double : Return GetType(Double)
                Case TypeCode.DateTime : Return GetType(Date)
                Case TypeCode.Char : Return GetType(Char)
                Case TypeCode.String : Return GetType(String)
                Case TypeCode.Object : Return GetType(Object)
                Case TypeCode.DBNull : Return GetType(System.DBNull)

                Case TypeCode.Empty
                    'fall through

            End Select

            Return Nothing
        End Function

        Friend Shared Function IsRootObjectType(ByVal Type As System.Type) As Boolean
            Return Type Is GetType(Object)
        End Function

        Friend Shared Function IsRootEnumType(ByVal Type As System.Type) As Boolean
            Return Type Is GetType(System.Enum)
        End Function

        Friend Shared Function IsValueType(ByVal Type As System.Type) As Boolean
            Return Type.IsValueType
        End Function

        Friend Shared Function IsEnum(ByVal Type As System.Type) As Boolean
            Return Type.IsEnum
        End Function

        Friend Shared Function IsArrayType(ByVal Type As System.Type) As Boolean
            Return Type.IsArray
        End Function

        Friend Shared Function IsStringType(ByVal Type As System.Type) As Boolean
            Return Type Is GetType(String)
        End Function

        Friend Shared Function IsCharArrayRankOne(ByVal Type As System.Type) As Boolean
            Return Type Is GetType(Char())
        End Function

        Friend Shared Function IsIntegralType(ByVal TypeCode As System.TypeCode) As Boolean
            Select Case TypeCode
                Case TypeCode.SByte, _
                     TypeCode.Byte, _
                     TypeCode.Int16, _
                     TypeCode.UInt16, _
                     TypeCode.Int32, _
                     TypeCode.UInt32, _
                     TypeCode.Int64, _
                     TypeCode.UInt64

                    Return True

                Case TypeCode.Empty, _
                     TypeCode.Object, _
                     TypeCode.DBNull, _
                     TypeCode.Boolean, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double, _
                     TypeCode.DateTime, _
                     TypeCode.Char, _
                     TypeCode.String

                    'Fall through to end.
            End Select

            Return False
        End Function

#if 0 then
        Friend Shared Function IsIntegralType(ByVal Type As System.Type) As Boolean
            Return IsIntegralType(GetTypeCode(Type))
        End Function
#end if

        Friend Shared Function IsNumericType(ByVal TypeCode As System.TypeCode) As Boolean
            Select Case TypeCode
                Case TypeCode.SByte, _
                     TypeCode.Byte, _
                     TypeCode.Int16, _
                     TypeCode.UInt16, _
                     TypeCode.Int32, _
                     TypeCode.UInt32, _
                     TypeCode.Int64, _
                     TypeCode.UInt64, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double

                    Return True

                Case TypeCode.Empty, _
                     TypeCode.Object, _
                     TypeCode.DBNull, _
                     TypeCode.Boolean, _
                     TypeCode.DateTime, _
                     TypeCode.Char, _
                     TypeCode.String

                    'Fall through to end.
            End Select

            Return False
        End Function

        Friend Shared Function IsNumericType(ByVal Type As System.Type) As Boolean
            Return IsNumericType(GetTypeCode(Type))
        End Function

        Friend Shared Function IsIntrinsicType(ByVal TypeCode As System.TypeCode) As Boolean
            Select Case TypeCode
                Case TypeCode.Boolean, _
                     TypeCode.SByte, _
                     TypeCode.Byte, _
                     TypeCode.Int16, _
                     TypeCode.UInt16, _
                     TypeCode.Int32, _
                     TypeCode.UInt32, _
                     TypeCode.Int64, _
                     TypeCode.UInt64, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double, _
                     TypeCode.DateTime, _
                     TypeCode.Char, _
                     TypeCode.String

                    Return True

                Case TypeCode.Empty, _
                     TypeCode.Object, _
                     TypeCode.DBNull

                    'Fall through to end.
            End Select

            Return False
        End Function

        Friend Shared Function IsIntrinsicType(ByVal Type As System.Type) As Boolean
            Return IsIntrinsicType(GetTypeCode(Type)) AndAlso Not IsEnum(Type)
        End Function

#if 0 then
        Friend Shared Function IsUnsignedType(ByVal TypeCode As System.TypeCode) As Boolean
            Select Case TypeCode
                Case TypeCode.Byte, _
                     TypeCode.UInt16, _
                     TypeCode.UInt32, _
                     TypeCode.UInt64
                    Return True

                Case TypeCode.Empty, _
                     TypeCode.Object, _
                     TypeCode.DBNull, _
                     TypeCode.Boolean, _
                     TypeCode.SByte, _
                     TypeCode.Int16, _
                     TypeCode.Int32, _
                     TypeCode.Int64, _
                     TypeCode.Decimal, _
                     TypeCode.Single, _
                     TypeCode.Double, _
                     TypeCode.DateTime, _
                     TypeCode.Char, _
                     TypeCode.String

                    'Fall through to end.
            End Select

            Return False
        End Function

        ' Friend Shared Function IsUnsignedType(ByVal Type As System.Type) As Boolean
        '     Return IsUnsignedType(GetTypeCode(Type))
        ' End Function
#end if

        Friend Shared Function IsClass(ByVal Type As System.Type) As Boolean
            Return Type.IsClass OrElse IsRootEnumType(Type)
        End Function

        Friend Shared Function IsClassOrValueType(ByVal Type As System.Type) As Boolean
            Return IsValueType(Type) OrElse IsClass(Type)
        End Function

        Friend Shared Function IsInterface(ByVal Type As System.Type) As Boolean
            Return Type.IsInterface
        End Function

        Friend Shared Function IsClassOrInterface(ByVal Type As System.Type) As Boolean
            Return IsClass(Type) OrElse IsInterface(Type)
        End Function

        Friend Shared Function IsReferenceType(ByVal Type As System.Type) As Boolean
            Return IsClass(Type) OrElse IsInterface(Type)
        End Function

        Friend Shared Function IsGenericParameter(ByVal Type As System.Type) As Boolean
            Return Type.IsGenericParameter
        End Function

        'SHIQIC: Port this function when build with clr4.0
#If Not TELESTO Then
        Friend Shared Function IsEquivalentType(ByVal Left As System.Type, ByVal Right As System.Type) As Boolean
            ' Type.IsEquivalentTo(Type) doesn't work properly for instantiated
            ' generic types other than interfaces. For example:
            '   1. IList(Of NoPiaType) should be equivalent to IList(Of PiaType)
            '   2. List(Of NoPiaType) should be equivalent to List(Of PiaType)
            ' In CLR 4, the first example works, but the second doesn't. We workaround it here.
            ' When this is fixed in CLR we can remove this code.
            If IsInstantiatedGeneric(Left) AndAlso Not Left.IsInterface AndAlso _
                IsInstantiatedGeneric(Right) AndAlso Not Right.IsInterface Then

                ' Compare generic type defintion
                If Not IsEquivalentType(Left.GetGenericTypeDefinition, Right.GetGenericTypeDefinition) Then
                    Return False
                End If

                ' Compare generic arguments
                Dim LeftArgs As Type() = Left.GetGenericArguments
                Dim RightArgs As Type() = Right.GetGenericArguments
                If LeftArgs.Length <> RightArgs.Length Then
                    Return False
                End If

                For i As Integer = 0 To LeftArgs.Length - 1
                    If Not IsEquivalentType(LeftArgs(i), RightArgs(i)) Then
                        Return False
                    End If
                Next

                Return True
            End If
            Return Left.IsEquivalentTo(Right)
        End Function
#End If

#If LATEBINDING
        Friend Shared Function IsCollectionInterface(ByVal Type As System.Type) As Boolean
            If Type.IsInterface AndAlso
               ((Type.IsGenericType AndAlso
                   (Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IList(Of )) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.ICollection(Of )) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IEnumerable(Of )) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IReadOnlyList(Of )) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IReadOnlyCollection(Of )) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IDictionary(Of ,)) OrElse
                    Type.GetGenericTypeDefinition() Is GetType(System.Collections.Generic.IReadOnlyDictionary(Of ,)))) OrElse
                Type Is GetType(System.Collections.IList) OrElse
                Type Is GetType(System.Collections.ICollection) OrElse
                Type Is GetType(System.Collections.IEnumerable) OrElse
                Type Is GetType(System.ComponentModel.INotifyPropertyChanged) OrElse
                Type Is GetType(System.Collections.Specialized.INotifyCollectionChanged)) Then
                Return True
            End If

            Return False
        End Function
#Else
        Friend Shared Function IsCollectionInterface(ByVal Type As System.Type) As Boolean
            If Type.IsInterface AndAlso
               ((Type.IsGenericType AndAlso
                   (Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IList(Of )) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.ICollection(Of )) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IEnumerable(Of )) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IReadOnlyList(Of )) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IReadOnlyCollection(Of )) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IDictionary(Of ,)) OrElse
                    Type.GetGenericTypeDefinition() = GetType(System.Collections.Generic.IReadOnlyDictionary(Of ,)))) OrElse
                Type = GetType(System.Collections.IList) OrElse
                Type = GetType(System.Collections.ICollection) OrElse
                Type = GetType(System.Collections.IEnumerable) OrElse
                Type = GetType(System.ComponentModel.INotifyPropertyChanged) OrElse
                Type = GetType(System.Collections.Specialized.INotifyCollectionChanged)) Then
                Return True
            End If

            Return False
        End Function
#End If
        Friend Shared Function [Implements](ByVal Implementor As System.Type, ByVal [Interface] As System.Type) As Boolean

            Debug.Assert(Not IsInterface(Implementor), "interfaces can't implement, so why call this?")
            Debug.Assert(IsInterface([Interface]), "expected interface, not " & [Interface].FullName)

            For Each Implemented As Type In Implementor.GetInterfaces
                'CONSIDER: the call to getinterfaces is expensive, and may involve doing a QueryInterface.  how to combine?
#If TELESTO Then
                If Implemented Is [Interface]  Then
#Else
                ' Check identity and NoPIA type equivalency
                If Implemented Is [Interface] OrElse IsEquivalentType(Implemented, [Interface]) Then
#End If
                    Return True
                End If
            Next

            Return False

        End Function

        Friend Shared Function IsOrInheritsFrom(ByVal Derived As System.Type, ByVal Base As System.Type) As Boolean
            Debug.Assert((Not Derived.IsByRef) AndAlso (Not Derived.IsPointer))
            Debug.Assert((Not Base.IsByRef) AndAlso (Not Base.IsPointer))

            If Derived Is Base Then Return True

            If Derived.IsGenericParameter() Then
                If IsClass(Base) AndAlso _
                   (CBool(Derived.GenericParameterAttributes() And GenericParameterAttributes.NotNullableValueTypeConstraint)) AndAlso _
                   IsOrInheritsFrom(GetType(System.ValueType), Base) Then
                    Return True
                End If

                For Each TypeConstraint As Type In Derived.GetGenericParameterConstraints
                    If IsOrInheritsFrom(TypeConstraint, Base) Then
                        Return True
                    End If
                Next

            ElseIf IsInterface(Derived) Then
                If IsInterface(Base) Then
                    'CONSIDER: the call to getinterfaces is expensive, and may involve doing a QueryInterface.  how to combine?
                    For Each BaseInterface As Type In Derived.GetInterfaces
                        If BaseInterface Is Base Then
                            Return True
                        End If
                    Next
                End If

            ElseIf IsClass(Base) AndAlso IsClassOrValueType(Derived) Then
                Return Derived.IsSubclassOf(Base)
            End If

            Return False
        End Function

        Friend Shared Function IsGeneric(ByVal Type As Type) As Boolean
            Return Type.IsGenericType
        End Function

        Friend Shared Function IsInstantiatedGeneric(ByVal Type As Type) As Boolean
            Return Type.IsGenericType AndAlso (Not Type.IsGenericTypeDefinition)
        End Function

        Friend Shared Function IsGeneric(ByVal Method As MethodBase) As Boolean
            Return Method.IsGenericMethod
        End Function

        Friend Shared Function IsGeneric(ByVal Member As MemberInfo) As Boolean
            'Returns True whether Method is an instantiated or uninstantiated generic method.
            Dim Method As MethodBase = TryCast(Member, MethodBase)
            If Method Is Nothing Then Return False
            Return IsGeneric(Method)
        End Function

#If DEBUG Then
        Friend Shared Function IsInstantiatedGeneric(ByVal Method As MethodBase) As Boolean
            Return Method.IsGenericMethod AndAlso (Not Method.IsGenericMethodDefinition)
        End Function
#End If

        Friend Shared Function IsRawGeneric(ByVal Method As MethodBase) As Boolean
            Return Method.IsGenericMethod AndAlso Method.IsGenericMethodDefinition
        End Function

        Friend Shared Function GetTypeParameters(ByVal Member As MemberInfo) As Type()
            Dim Method As MethodBase = TryCast(Member, MethodBase)
            If Method Is Nothing Then Return NoTypeParameters
            Return Method.GetGenericArguments
        End Function

        Friend Shared Function GetTypeParameters(ByVal Type As Type) As Type()
            Debug.Assert(Type.GetGenericTypeDefinition Is Nothing, "expected unbound generic type")
            Return Type.GetGenericArguments
        End Function

        Friend Shared Function GetTypeArguments(ByVal Type As Type) As Type()
            Debug.Assert(Type.GetGenericTypeDefinition IsNot Nothing, "expected bound generic type")
            Return Type.GetGenericArguments
        End Function

        Friend Shared Function GetInterfaceConstraints(ByVal GenericParameter As Type) As Type()
            'Returns the interface constraints for the type parameter.
            Debug.Assert(IsGenericParameter(GenericParameter), "expected type parameter")
            Return GenericParameter.GetInterfaces()
        End Function

        Friend Shared Function GetClassConstraint(ByVal GenericParameter As Type) As Type
            'Returns the class constraint for the type parameter, Nothing if it has
            'no class constraint.
            Debug.Assert(IsGenericParameter(GenericParameter), "expected type parameter")

            'Type parameters with no class constraint have System.Object as their base type.
            Dim ClassConstraint As Type = GenericParameter.BaseType
            If IsRootObjectType(ClassConstraint) Then Return Nothing
            Return ClassConstraint
        End Function

        Friend Shared Function IndexIn(ByVal PossibleGenericParameter As Type, ByVal GenericMethodDef As MethodBase) As Integer
            'Returns the index of PossibleGenericParameter in Method.  If the generic param cannot be found,
            'returns -1

            Debug.Assert(GenericMethodDef IsNot Nothing AndAlso IsRawGeneric(GenericMethodDef), "Uninstantiated generic expected!!!")

            If IsGenericParameter(PossibleGenericParameter) AndAlso _
               PossibleGenericParameter.DeclaringMethod IsNot Nothing AndAlso _
               AreGenericMethodDefsEqual(PossibleGenericParameter.DeclaringMethod, GenericMethodDef) Then
                Return PossibleGenericParameter.GenericParameterPosition
            End If
            Return -1
        End Function

        Friend Shared Function RefersToGenericParameter(ByVal ReferringType As Type, ByVal Method As MethodBase) As Boolean
            'Given ReferringType, determine if it contains any usages of the generic parameters of Method.
            'For example, the referring types T and C1(Of T) and T() refer to a generic param of Sub Foo(Of T).

            If Not IsRawGeneric(Method) Then Return False

            If ReferringType.IsByRef Then ReferringType = GetElementType(ReferringType)

            If IsGenericParameter(ReferringType) Then
                'Is T a generic parameter of Method?

                Debug.Assert(ReferringType.DeclaringMethod.IsGenericMethodDefinition, "Unexpected generic method instantiation!!!")

                If AreGenericMethodDefsEqual(ReferringType.DeclaringMethod, Method) Then
                    Return True
                End If

            ElseIf IsGeneric(ReferringType) Then
                'For C1(Of T, U, V), recurse on T, U, and V.
                For Each Param As Type In GetTypeArguments(ReferringType)
                    If RefersToGenericParameter(Param, Method) Then
                        Return True
                    End If
                Next

            ElseIf IsArrayType(ReferringType) Then
                'For T(), recurse on T.
                Return RefersToGenericParameter(ReferringType.GetElementType, Method)

            End If

            Return False

        End Function

        'Is T a generic parameter of Type. Note that the clr way of representing type params will
        'cause us to return true for the copies of the type params of all the parent types that are
        'on the passed in Typ. Note that this clr behavior has been retained because in the run time
        'for the uses of this function, this functionality is desired.
        '
        Friend Shared Function RefersToGenericParameterCLRSemantics(ByVal ReferringType As Type, ByVal Typ As Type) As Boolean
            'Given ReferringType, determine if it contains any usages of the generic parameters of Typ.
            'For example, the referring types T and C1(Of T) and T() refer to a generic param of Class Cls1(Of T).

            If ReferringType.IsByRef Then ReferringType = GetElementType(ReferringType)

            If IsGenericParameter(ReferringType) Then
                'Is T a generic parameter of Type. Note that the clr way of representing type params will
                'return true for the copies of the type params of all the parent types that are on the 
                'passed in Typ.

                If ReferringType.DeclaringType Is Typ Then
                    Return True
                End If

            ElseIf IsGeneric(ReferringType) Then
                'For C1(Of T, U, V), recurse on T, U, and V.
                For Each Param As Type In GetTypeArguments(ReferringType)
                    If RefersToGenericParameterCLRSemantics(Param, Typ) Then
                        Return True
                    End If
                Next

            ElseIf IsArrayType(ReferringType) Then
                'For T(), recurse on T.
                Return RefersToGenericParameterCLRSemantics(ReferringType.GetElementType, Typ)

            End If

            Return False

        End Function

        'Friend Shared Function AreMethodsEqual(ByVal Method1 As MethodBase, ByVal Method2 As MethodBase) As Boolean
        '    ' Need to do this kind of comparison because the MethodInfo obtained for a
        '    ' base method through type1 is not the same as that obtained from type2
        '    '
        '    ' UNDONE: - currently there is no way to compare generic method instantions
        '    ' because methodhandles for different instantiations might be equal if based on clr
        '    ' optimizations, they end up sharing the same IL
        '    '
        '    Return _
        '        Method1 Is Method2 OrElse _
        '        (Method1 IsNot Nothing AndAlso _
        '         Method2 IsNot Nothing AndAlso _
        '         (Method1.MethodHandle.Equals(Method2.MethodHandle) AndAlso _
        '          ((Method1.IsGenericMethodDefinition AndAlso _
        '            Method2.IsGenericMethodDefinition) OrElse _
        '           (Not Method1.HasGenericArguments AndAlso _
        '            Not Method2.HasGenericArguments))))
        'End Function

        Friend Shared Function AreGenericMethodDefsEqual(ByVal Method1 As MethodBase, ByVal Method2 As MethodBase) As Boolean
            Debug.Assert(Method1 IsNot Nothing AndAlso IsRawGeneric(Method1), "Generic method def expected!!!")
            Debug.Assert(Method2 IsNot Nothing AndAlso IsRawGeneric(Method2), "Generic method def expected!!!")

            ' Need to do this kind of comparison because the MethodInfo obtained for a
            ' base method through type1 is not the same as that obtained from type2
            '
            Return _
                Method1 Is Method2 OrElse _
                Method1.MetadataToken = Method2.MetadataToken
        End Function

        Friend Shared Function IsShadows(ByVal Method As MethodBase) As Boolean
            If Method.IsHideBySig Then Return False
            If Method.IsVirtual AndAlso (Method.Attributes And MethodAttributes.NewSlot) = 0 Then

                'Only the most derived Overrides member shows up in the member list returned by reflection.
                'However, we have to check the most base (Overridable) member because the Shadowing information
                'is stored only there.
                If (DirectCast(Method, MethodInfo).GetBaseDefinition().Attributes And MethodAttributes.NewSlot) = 0 Then
                    Return False
                End If
            End If
            Return True
        End Function

        Friend Shared Function IsShared(ByVal Member As MemberInfo) As Boolean

            Select Case Member.MemberType
                Case MemberTypes.Method
                    Return DirectCast(Member, MethodInfo).IsStatic

                Case MemberTypes.Field
                    Return DirectCast(Member, FieldInfo).IsStatic

                Case MemberTypes.Constructor
                    Return DirectCast(Member, ConstructorInfo).IsStatic

                Case MemberTypes.Property
                    Return DirectCast(Member, PropertyInfo).GetGetMethod.IsStatic

                Case Else
#If TELESTO Then
                    Debug.Assert(False, "unexpected membertype") ' Silverlight CLR does not have Debug.Fail.
#Else
                    Debug.Fail("unexpected membertype")
#End If
            End Select

            Return False

        End Function

        Friend Shared Function IsParamArray(ByVal Parameter As ParameterInfo) As Boolean
            Return IsArrayType(Parameter.ParameterType) AndAlso Parameter.IsDefined(GetType(ParamArrayAttribute), False)
        End Function

        Friend Shared Function GetElementType(ByVal Type As System.Type) As Type
            Debug.Assert(Type.HasElementType, "expected type with element type")
            Return Type.GetElementType
        End Function

        Friend Shared Function AreParametersAndReturnTypesValid( _
            ByVal Parameters As ParameterInfo(), _
            ByVal ReturnType As Type) As Boolean

            If ReturnType IsNot Nothing AndAlso (ReturnType.IsPointer OrElse ReturnType.IsByRef) Then
                Return False
            End If

            If Parameters IsNot Nothing Then
                For Each Parameter As ParameterInfo In Parameters
                    If Parameter.ParameterType.IsPointer Then
                        Return False
                    End If
                Next
            End If

            Return True
        End Function

        Friend Shared Sub GetAllParameterCounts( _
            ByVal Parameters As ParameterInfo(), _
            ByRef RequiredParameterCount As Integer, _
            ByRef MaximumParameterCount As Integer, _
            ByRef ParamArrayIndex As Integer)


            Debug.Assert(Parameters IsNot Nothing, "expected parameter array")

            MaximumParameterCount = Parameters.Length

            'All optional parameters are grouped at the end, so the index of the
            'last non-optional (+1) gives us the count of required parameters.
            For Index As Integer = MaximumParameterCount - 1 To 0 Step -1
                If Not Parameters(Index).IsOptional Then
                    RequiredParameterCount = Index + 1
                    Exit For
                End If
            Next

            'Only the last parameter can be a ParamArray, so check it.
            If MaximumParameterCount <> 0 AndAlso IsParamArray(Parameters(MaximumParameterCount - 1)) Then
                ParamArrayIndex = MaximumParameterCount - 1
                RequiredParameterCount -= 1
            End If
        End Sub

        Friend Shared Function IsNonPublicRuntimeMember(ByVal Member As MemberInfo) As Boolean

            'Disallow latebound calls to internal Microsoft.VisualBasic types
            Dim DeclaringType As System.Type = Member.DeclaringType

            ' VSW#430608: For nested types IsNotPublic doesn't return the right value so
            ' we need to use Not IsPublic. 
            '
            ' The following code will only allow calls to members of top level public types
            ' in the runtime library. Read the reflection documentation and test with
            ' nested types before changing this code.

            Return Not DeclaringType.IsPublic AndAlso DeclaringType.Assembly Is Utils.VBRuntimeAssembly

        End Function

        'this is a utility function, so it doesn't really belong in Symbols, but...
        Friend Shared Function HasFlag(ByVal Flags As BindingFlags, ByVal FlagToTest As BindingFlags) As Boolean
            Return CBool(Flags And FlagToTest)
        End Function

        Friend NotInheritable Class Container

            Private Class InheritanceSorter : Implements IComparer(Of MemberInfo)

                Private Sub New()
                End Sub

                Private Function Compare(ByVal Left As MemberInfo, ByVal Right As MemberInfo) As Integer Implements IComparer(Of MemberInfo).Compare
                    Dim LeftType As Type = Left.DeclaringType
                    Dim RightType As Type = Right.DeclaringType

#If BINDING_LOG Then
                'Console.WriteLine("compare: " & LeftType.Name & " " & RightType.Name)
#End If
                    If LeftType Is RightType Then Return 0
                    If LeftType.IsSubclassOf(RightType) Then Return -1

                    'Necessary to return 1 only for RightType.IsSubclassOf(LeftType)?  If no inheritance
                    'relationhip exists, which is possible when members come from IReflect, returning 1
                    'is still okay. Returning 1 in this IReflect case will not cause qsort to never terminate.
                    Return 1
                End Function

                Friend Shared ReadOnly Instance As InheritanceSorter = New InheritanceSorter

            End Class

            Private ReadOnly m_Instance As Object
            Private ReadOnly m_Type As Type
            Private ReadOnly m_IReflect As IReflect
            Private ReadOnly m_UseCustomReflection As Boolean

            Friend Sub New(ByVal Instance As Object)

                If Instance Is Nothing Then
                    Throw VbMakeException(vbErrors.ObjNotSet)
                End If

                m_Instance = Instance
                m_Type = Instance.GetType

                ' For a System.Type Object, we always use the underlying System.Type's IReflect implementation, because a System.Type's Implementation
                ' returns information about the Type it represents and not its own information. If we did not do this, latebound calls to a System.Type
                ' Object would fail.

                ' We don't support this for COM Objects because this is not a valid COM scenario and the performance cost is intolerable

                m_UseCustomReflection = False

#If TELESTO Then
                If Not m_Type.IsCOMObject AndAlso Not TypeOf Instance Is System.Type Then 'No RemotingServices in Telesto
#Else
                If Not m_Type.IsCOMObject AndAlso _
                   Not RemotingServices.IsTransparentProxy(Instance) AndAlso _
                   Not TypeOf Instance Is System.Type Then
#End If
                    m_IReflect = TryCast(Instance, IReflect)

                    If m_IReflect IsNot Nothing Then
                        m_UseCustomReflection = True
                    End If
                End If

                If Not m_UseCustomReflection Then
                    m_IReflect = DirectCast(m_Type, IReflect)
                End If

                CheckForClassExtendingCOMClass()
            End Sub

            Friend Sub New(ByVal Type As Type)

                If Type Is Nothing Then
                    Throw VbMakeException(vbErrors.ObjNotSet)
                End If

                m_Instance = Nothing
                m_Type = Type
                m_IReflect = DirectCast(Type, IReflect)
                Debug.Assert(m_IReflect.UnderlyingSystemType Is m_Type, "system.type is returning a different type?")
                m_UseCustomReflection = False

                CheckForClassExtendingCOMClass()
            End Sub

            Friend ReadOnly Property IsCOMObject() As Boolean
                Get
                    Return m_Type.IsCOMObject
                End Get
            End Property

            ' Try to determine if this object represents a WindowsRuntime object - i.e. it either
            ' is coming from a WinMD file or is derived from a class coming from a WinMD.
            ' The logic here matches the CLR's logic of finding a WinRT object.

            Friend ReadOnly Property IsWindowsRuntimeObject() As Boolean
                Get
                    Dim curType As Type = m_Type
                    While curType IsNot Nothing
                        If curType.Attributes.HasFlag(System.Reflection.TypeAttributes.WindowsRuntime) Then
                            ' Found a WinRT COM object
                            Return True
                        ElseIf curType.Attributes.HasFlag(System.Reflection.TypeAttributes.Import) Then
                            ' Found a class that is actually imported from COM but not WinRT
                            ' this is definitely a non-WinRT COM object
                            Return False
                        End If
                        curType = curType.BaseType
                    End While
                    Return False

                End Get
            End Property

            Friend ReadOnly Property VBFriendlyName() As String
                Get
                    Return Utils.VBFriendlyName(m_Type, m_Instance)
                End Get
            End Property

            Friend ReadOnly Property IsArray() As Boolean
                Get
                    Return IsArrayType(m_Type) AndAlso m_Instance IsNot Nothing
                End Get
            End Property

            Friend ReadOnly Property IsValueType() As Boolean
                Get
                    Return Symbols.IsValueType(m_Type) AndAlso m_Instance IsNot Nothing
                End Get
            End Property

            Private Const DefaultLookupFlags As BindingFlags = _
                BindingFlags.IgnoreCase Or _
                BindingFlags.FlattenHierarchy Or _
                BindingFlags.Public Or _
                BindingFlags.Static Or _
                BindingFlags.Instance

            Private Shared ReadOnly NoMembers As MemberInfo() = {}

            ' CONSIDER: Move this function directly into OverloadResolution so
            ' that we don't expand all the signatures. It would require adding
            ' a flag to GetMembers to either do the filtering or not.
            Private Shared Function FilterInvalidMembers(ByVal Members As MemberInfo()) As MemberInfo()

                If Members Is Nothing OrElse Members.Length = 0 Then
                    Return Nothing
                End If

                Dim ValidMemberCount As Integer = 0
                Dim MemberIndex As Integer = 0

                For MemberIndex = 0 To Members.Length - 1
                    Dim Parameters As ParameterInfo() = Nothing
                    Dim ReturnType As Type = Nothing

                    Select Case Members(MemberIndex).MemberType

                        Case MemberTypes.Constructor, _
                             MemberTypes.Method

                            Dim CurrentMethod As MethodInfo = DirectCast(Members(MemberIndex), MethodInfo)

                            Parameters = CurrentMethod.GetParameters
                            ReturnType = CurrentMethod.ReturnType

                        Case MemberTypes.Property

                            Dim PropertyBlock As PropertyInfo = DirectCast(Members(MemberIndex), PropertyInfo)
                            Dim GetMethod As MethodInfo = PropertyBlock.GetGetMethod

                            If GetMethod IsNot Nothing Then
                                Parameters = GetMethod.GetParameters
                            Else
                                Dim SetMethod As MethodInfo = PropertyBlock.GetSetMethod
                                Dim SetParameters As ParameterInfo() = SetMethod.GetParameters

                                Parameters = New ParameterInfo(SetParameters.Length - 2) {}
                                System.Array.Copy(SetParameters, Parameters, Parameters.Length)
                            End If

                            ReturnType = PropertyBlock.PropertyType

                        Case MemberTypes.Field
                            ReturnType = DirectCast(Members(MemberIndex), FieldInfo).FieldType

                    End Select

                    If AreParametersAndReturnTypesValid(Parameters, ReturnType) Then
                        ValidMemberCount += 1
                    Else
                        Members(MemberIndex) = Nothing
                    End If
                Next

                If ValidMemberCount = Members.Length Then
                    Return Members
                ElseIf ValidMemberCount > 0 Then

                    Dim ValidMembers(ValidMemberCount - 1) As MemberInfo
                    Dim ValidMemberIndex As Integer = 0

                    For MemberIndex = 0 To Members.Length - 1
                        If Members(MemberIndex) IsNot Nothing Then
                            ValidMembers(ValidMemberIndex) = Members(MemberIndex)
                            ValidMemberIndex += 1
                        End If
                    Next

                    Return ValidMembers
                End If

                Return Nothing
            End Function

            ' For a WinRT object, we want to treat members of it's collection interfaces as members of the object 
            ' itself. So GetMembers calls here to find the member in all the collection interfaces that this object 
            ' implements.
            Friend Function LookupWinRTCollectionInterfaceMembers(ByVal MemberName As String) As List(Of MemberInfo)
                Debug.Assert(Me.IsWindowsRuntimeObject(), "Expected a Windows Runtime Object")

                Dim Result As New List(Of MemberInfo)
                For Each Implemented As Type In m_Type.GetInterfaces()
                    If IsCollectionInterface(Implemented) Then
                        Dim members As MemberInfo() = Implemented.GetMember(MemberName, DefaultLookupFlags)
                        If (members IsNot Nothing) Then
                            Result.AddRange(members)
                        End If
                    End If
                Next

                Return Result
            End Function

            Friend Function LookupNamedMembers(ByVal MemberName As String) As MemberInfo()
                'Returns an array of members matching MemberName sorted by inheritance (most derived first).
                'If no members match MemberName, returns an empty array.

                Dim Result As MemberInfo()

                If IsGenericParameter(m_Type) Then
                    'Getting the members of a generic parameter follows a special rule.
                    'In a Latebound context, only members of the class constraint are
                    'applicable. We will ignore interface constraints. Also, custom
                    'Reflection can't be involved, so no need to use that.

                    Dim ClassConstraint As Type = GetClassConstraint(m_Type)
                    If ClassConstraint IsNot Nothing Then
                        Result = ClassConstraint.GetMember(MemberName, DefaultLookupFlags)
                    Else
                        Result = Nothing
                    End If
                Else
                    Result = m_IReflect.GetMember(MemberName, DefaultLookupFlags)
                End If

                If Me.IsWindowsRuntimeObject() Then
                    Dim CollectionMethods As List(Of MemberInfo) = LookupWinRTCollectionInterfaceMembers(MemberName)
                    If Result IsNot Nothing Then
                        CollectionMethods.AddRange(Result)
                    End If

                    Result = CollectionMethods.ToArray()
                End If

                Result = FilterInvalidMembers(Result)

                If Result Is Nothing Then
                    Result = NoMembers
                ElseIf Result.Length > 1 Then
                    Array.Sort(Of MemberInfo)(Result, InheritanceSorter.Instance)
                End If

                Return Result
            End Function

            ' For a WinRT object, we want to treat members of it's collection interfaces as members of the object 
            ' itself. Search through all the collection interfaces for default members.
            Private Function LookupWinRTCollectionDefaultMembers(ByRef DefaultMemberName As String) As List(Of MemberInfo)
                Debug.Assert(Me.IsWindowsRuntimeObject(), "Expected a Windows Runtime Object")

                Dim Result As New List(Of MemberInfo)
                For Each Implemented As Type In m_Type.GetInterfaces()
                    If IsCollectionInterface(Implemented) Then
                        Dim members As MemberInfo() = LookupDefaultMembers(DefaultMemberName, Implemented)
                        If (members IsNot Nothing) Then
                            Result.AddRange(members)
                        End If
                    End If
                Next

                Return Result
            End Function

            Private Function LookupDefaultMembers(ByRef DefaultMemberName As String, ByVal SearchType As Type) As MemberInfo()
                'Returns an array of default members sorted by inheritance (most derived first).
                'If no members match MemberName, returns an empty array.
                'The default member name is determined by walking up the inheritance hierarchy looking
                'for a DefaultMemberAttribute.

                Dim PotentialDefaultMemberName As String = Nothing

                'Find the default member name.
                Dim Current As Type = SearchType
                Do
                    'CONSIDER: if one exists, use the generic form of GetCustomAttributes.
                    Dim Attributes As Object() = Current.GetCustomAttributes(GetType(DefaultMemberAttribute), False)

                    If Attributes IsNot Nothing AndAlso Attributes.Length > 0 Then
                        PotentialDefaultMemberName = DirectCast(Attributes(0), DefaultMemberAttribute).MemberName
                        Exit Do
                    End If
                    Current = Current.BaseType

                Loop While Current IsNot Nothing AndAlso Not IsRootObjectType(Current)

                If PotentialDefaultMemberName IsNot Nothing Then
                    Dim Result As MemberInfo() = Current.GetMember(PotentialDefaultMemberName, DefaultLookupFlags)

                    Result = FilterInvalidMembers(Result)

                    If Result IsNot Nothing Then
                        DefaultMemberName = PotentialDefaultMemberName
                        If Result.Length > 1 Then
                            Array.Sort(Result, InheritanceSorter.Instance)
                        End If
                        Return Result
                    End If
                End If

                Return NoMembers
            End Function

            Friend Function GetMembers( _
                ByRef MemberName As String, _
                ByVal ReportErrors As Boolean) As MemberInfo()

                Dim Result As MemberInfo()
                If MemberName Is Nothing Then MemberName = ""

                If MemberName = "" Then

                    If m_UseCustomReflection Then
                        Result = Me.LookupNamedMembers(MemberName)
                    Else
                        Result = Me.LookupDefaultMembers(MemberName, m_Type) 'MemberName is set during this call.
                    End If

                    If Me.IsWindowsRuntimeObject() Then
                        Dim CollectionMethods As List(Of MemberInfo) = LookupWinRTCollectionDefaultMembers(MemberName)
                        If Result IsNot Nothing Then
                            CollectionMethods.AddRange(Result)
                        End If

                        Result = CollectionMethods.ToArray()
                    End If

                    If Result.Length = 0 Then
                        If ReportErrors Then
                            Throw New MissingMemberException( _
                                GetResourceString(ResID.MissingMember_NoDefaultMemberFound1, Me.VBFriendlyName))
                        End If

                        Return Result
                    End If

                    If m_UseCustomReflection Then MemberName = Result(0).Name

                Else
                    Result = Me.LookupNamedMembers(MemberName)

                    If Result.Length = 0 Then
                        If ReportErrors Then
                            Throw New MissingMemberException( _
                                GetResourceString(ResID.MissingMember_MemberNotFoundOnType2, MemberName, Me.VBFriendlyName))
                        End If

                        Return Result
                    End If
                End If

                Return Result
            End Function

            Private Sub CheckForClassExtendingCOMClass()
                If Me.IsCOMObject AndAlso Not Me.IsWindowsRuntimeObject AndAlso m_Type.FullName <> "System.__ComObject" AndAlso m_Type.BaseType.FullName <> "System.__ComObject" Then
                    Throw New InvalidOperationException(GetResourceString(ResID.LateboundCallToInheritedComClass))
                End If
            End Sub


            Friend Function GetFieldValue(ByVal Field As FieldInfo) As Object
                If m_Instance Is Nothing AndAlso Not IsShared(Field) Then
                    'Reference to non-shared member '|1' requires an object reference.
                    Throw New NullReferenceException( _
                        GetResourceString(ResID.NullReference_InstanceReqToAccessMember1, FieldToString(Field)))
                End If
                '
                'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '
                If IsNonPublicRuntimeMember(Field) Then
                    'No message text intentional - Default BCL message used
                    Throw New MissingMemberException
                End If
                '
                'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '
                Return Field.GetValue(m_Instance)
            End Function

            Friend Sub SetFieldValue(ByVal Field As FieldInfo, ByVal Value As Object)
                If Field.IsInitOnly Then
                    'REVIEW: Should this really be MissingMemberException, or something else?
                    Throw New MissingMemberException( _
                        GetResourceString(ResID.MissingMember_ReadOnlyField2, Field.Name, Me.VBFriendlyName))
                End If

                If m_Instance Is Nothing AndAlso Not IsShared(Field) Then
                    'Reference to non-shared member '|1' requires an object reference.
                    Throw New NullReferenceException( _
                        GetResourceString(ResID.NullReference_InstanceReqToAccessMember1, FieldToString(Field)))
                End If
                '
                'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '
                If IsNonPublicRuntimeMember(Field) Then
                    'No message text intentional - Default BCL message used
                    Throw New MissingMemberException
                End If
                '
                'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '
                Field.SetValue(m_Instance, Conversions.ChangeType(Value, Field.FieldType))
                Return
            End Sub

            Friend Function GetArrayValue(ByVal Indices As Object()) As Object
                Debug.Assert(Me.IsArray, "expected array when getting array value")
                Debug.Assert(Indices IsNot Nothing, "expected valid indices")


                Dim ArrayInstance As Array = DirectCast(m_Instance, System.Array)
                Dim Rank As Integer = ArrayInstance.Rank

                If Indices.Length <> Rank Then
                    Throw New RankException  'UNDONE: more detailed information here?
                End If

                'We use ChangeType to handle potential user-defined conversion operators.
                Dim ZerothIndex As Integer = _
                    DirectCast(Conversions.ChangeType(Indices(0), GetType(Integer)), Integer)

                If Rank = 1 Then
                    Return ArrayInstance.GetValue(ZerothIndex)
                Else
                    Dim FirstIndex As Integer = _
                        DirectCast(Conversions.ChangeType(Indices(1), GetType(Integer)), Integer)

                    If Rank = 2 Then
                        Return ArrayInstance.GetValue(ZerothIndex, FirstIndex)
                    Else
                        Dim SecondIndex As Integer = _
                            DirectCast(Conversions.ChangeType(Indices(2), GetType(Integer)), Integer)

                        If Rank = 3 Then
                            Return ArrayInstance.GetValue(ZerothIndex, FirstIndex, SecondIndex)
                        Else
                            Dim IndexArray As Integer() = New Integer(Rank - 1) {}
                            IndexArray(0) = ZerothIndex : IndexArray(1) = FirstIndex : IndexArray(2) = SecondIndex

                            For i As Integer = 3 To Rank - 1
                                IndexArray(i) = _
                                    DirectCast(Conversions.ChangeType(Indices(i), GetType(Integer)), Integer)
                            Next

                            Return ArrayInstance.GetValue(IndexArray)
                        End If
                    End If
                End If

            End Function

            Friend Sub SetArrayValue(ByVal Arguments As Object())
                'The last argument is the Value to be stored into the array. The other arguments are
                'the indices into the array.
                Debug.Assert(Me.IsArray, "expected array when setting array value")
                Debug.Assert(Arguments IsNot Nothing, "expected valid indices")


                Dim ArrayInstance As Array = DirectCast(m_Instance, System.Array)
                Dim Rank As Integer = ArrayInstance.Rank

                If Arguments.Length - 1 <> Rank Then
                    Throw New RankException  'UNDONE: more detailed information here?
                End If

                'To ensure order of evaulation, we must evaluate the Value argument after
                'evaluating each index argument.
                Dim Value As Object = Arguments(Arguments.Length - 1)
                Dim ElementType As Type = m_Type.GetElementType

                'We use ChangeType to handle potential user-defined conversion operators.
                Dim ZerothIndex As Integer = _
                    DirectCast(Conversions.ChangeType(Arguments(0), GetType(Integer)), Integer)

                If Rank = 1 Then
                    ArrayInstance.SetValue(Conversions.ChangeType(Value, ElementType), ZerothIndex)
                    Return
                Else
                    Dim FirstIndex As Integer = _
                        DirectCast(Conversions.ChangeType(Arguments(1), GetType(Integer)), Integer)

                    If Rank = 2 Then
                        ArrayInstance.SetValue(Conversions.ChangeType(Value, ElementType), ZerothIndex, FirstIndex)
                        Return
                    Else
                        Dim SecondIndex As Integer = _
                            DirectCast(Conversions.ChangeType(Arguments(2), GetType(Integer)), Integer)

                        If Rank = 3 Then
                            ArrayInstance.SetValue(Conversions.ChangeType(Value, ElementType), ZerothIndex, FirstIndex, SecondIndex)
                            Return
                        Else
                            Dim IndexArray As Integer() = New Integer(Rank - 1) {}
                            IndexArray(0) = ZerothIndex : IndexArray(1) = FirstIndex : IndexArray(2) = SecondIndex

                            For i As Integer = 3 To Rank - 1
                                IndexArray(i) = _
                                    DirectCast(Conversions.ChangeType(Arguments(i), GetType(Integer)), Integer)
                            Next

                            ArrayInstance.SetValue(Conversions.ChangeType(Value, ElementType), IndexArray)
                            Return
                        End If
                    End If
                End If

            End Sub

            Friend Function InvokeMethod( _
                ByVal TargetProcedure As Method, _
                ByVal Arguments As Object(), _
                ByVal CopyBack As Boolean(), _
                ByVal Flags As BindingFlags) As Object


                Dim CallTarget As MethodBase = GetCallTarget(TargetProcedure, Flags)
                Debug.Assert(CallTarget IsNot Nothing, "must have valid MethodBase")

                Debug.Assert(Not TargetProcedure.IsGeneric OrElse _
                             DirectCast(TargetProcedure.AsMethod, MethodInfo).GetGenericMethodDefinition IsNot Nothing, _
                             "expected bound generic method by this point")

                Dim CallArguments As Object() = _
                    ConstructCallArguments(TargetProcedure, Arguments, Flags)

                If m_Instance Is Nothing AndAlso Not IsShared(CallTarget) Then
                    'Reference to non-shared member '|1' requires an object reference.
                    Throw New NullReferenceException( _
                        GetResourceString(ResID.NullReference_InstanceReqToAccessMember1, TargetProcedure.ToString))
                End If
                '
                'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '
                If IsNonPublicRuntimeMember(CallTarget) Then
                    'No message text intentional - Default BCL message used
                    Throw New MissingMemberException
                End If
                '
                'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                '

                Dim Result As Object
                Try
                    Result = CallTarget.Invoke(m_Instance, CallArguments)

                Catch ex As TargetInvocationException When ex.InnerException IsNot Nothing
                    'For backwards compatiblity, throw the inner exception of a TargetInvocationException.
                    Throw ex.InnerException

                End Try

                ReorderArgumentArray(TargetProcedure, CallArguments, Arguments, CopyBack, Flags)
                Return Result
            End Function

#If 0 Then
            Friend Function InvokeCOMMethod( _
                ByVal MethodName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal CopyBack As Boolean(), _
                ByVal InvocationFlags As BindingFlags) As Object


                Debug.Assert(Me.IsCOMObject, "this function intended for COM objects only")

                If MethodName Is Nothing Then MethodName = ""
                Dim Modifiers As ParameterModifier() = Nothing

                If CopyBack IsNot Nothing AndAlso _
                m_Instance IsNot Nothing AndAlso _
                Not Runtime.Remoting.RemotingServices.IsTransparentProxy(m_Instance) Then

                    Dim Modifier As ParameterModifier = New ParameterModifier(Arguments.Length)
                    Modifiers = New ParameterModifier() {Modifier}

                    'Set all flags to ByRef, except for Missing arguments.
                    For Index As Integer = 0 To Arguments.Length - 1
                        If Arguments(Index) IsNot System.Reflection.Missing.Value Then
                            Modifier.Item(Index) = CopyBack(Index)
                        End If
                    Next

                End If

                Try
                    '
                    'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                    '
                    Call (New SecurityPermission(PermissionState.Unrestricted)).Demand()
                    Return _
                        m_IReflect.InvokeMember( _
                            MethodName, _
                            InvocationFlags Or DefaultLookupFlags, _
                            Nothing, _
                            m_Instance, _
                            Arguments, _
                            Modifiers, _
                            Nothing, _
                            ArgumentNames)
                    '
                    'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                    '
                Catch InvocationError As Exception When IsMissingMemberException(InvocationError)
                    Throw _
                        New MissingMemberException( _
                            GetResourceString( _
                                ResID.MissingMember_MemberNotFoundOnType2, _
                                MethodName, _
                                Me.VBFriendlyName), _
                            InvocationError)

                Catch InvocationError As TargetInvocationException
                    Throw InvocationError.InnerException

                End Try

            End Function

            Friend Function InvokeCOMMethod2( _
                ByVal MethodName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal CopyBack As Boolean(), _
                ByVal InvocationFlags As BindingFlags) As Object


                Debug.Assert(Me.IsCOMObject, "this function intended for COM objects only")

                If MethodName Is Nothing Then MethodName = ""
                Dim Modifiers As ParameterModifier() = Nothing

                Try
                    'Return binder.InvokeMember(name, flags, objType, objIReflect, o, args, paramnames)

                    If CopyBack IsNot Nothing AndAlso _
                    m_Instance IsNot Nothing AndAlso _
                    Not Runtime.Remoting.RemotingServices.IsTransparentProxy(m_Instance) Then

                        Dim Modifier As ParameterModifier = New ParameterModifier(Arguments.Length)
                        Modifiers = New ParameterModifier() {Modifier}

                        'Set all flags to ByRef, except for Missing arguments.
                        For Index As Integer = 0 To Arguments.Length - 1
                            If Arguments(Index) IsNot System.Reflection.Missing.Value Then
                                Modifier.Item(Index) = CopyBack(Index)
                            End If
                        Next

                    End If

                    Try
                        '
                        'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                        '
                        Call (New SecurityPermission(PermissionState.Unrestricted)).Demand()
                        Return _
                            m_IReflect.InvokeMember( _
                                MethodName, _
                                InvocationFlags Or DefaultLookupFlags, _
                                Nothing, _
                                m_Instance, _
                                Arguments, _
                                Modifiers, _
                                Nothing, _
                                ArgumentNames)
                        '
                        'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                        '
                    Catch InvocationError As MissingMemberException
                        Throw _
                            New MissingMemberException( _
                                GetResourceString( _
                                    ResID.MissingMember_MemberNotFoundOnType2, _
                                    MethodName, _
                                    Me.VBFriendlyName), _
                                InvocationError)
                    End Try
                    '
                    '
                    ' There may be a property or field that returns an array or object with a default member
                    ' We get the field or property then try using a LateIndexGet
                    'UNDONE: handle this in the binder code once the com+ team has completed the Beta2 DCR work
                Catch ex As Exception When IsMissingMemberException(ex)

                    Dim SecondaryResult As Object

                    Try
                        Modifiers = Nothing

                        If CopyBack IsNot Nothing AndAlso _
                        m_Instance IsNot Nothing AndAlso _
                        Not Runtime.Remoting.RemotingServices.IsTransparentProxy(m_Instance) Then

                            Dim Modifier As ParameterModifier = New ParameterModifier(Arguments.Length)
                            Modifiers = New ParameterModifier() {Modifier}

                            'Set all flags to ByRef, except for Missing arguments.
                            For Index As Integer = 0 To Arguments.Length - 1
                                If Arguments(Index) IsNot System.Reflection.Missing.Value Then
                                    Modifier.Item(Index) = CopyBack(Index)
                                End If
                            Next

                        End If

                        Try
                            '
                            'BEGIN: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                            '
                            Call (New SecurityPermission(PermissionState.Unrestricted)).Demand()
                            SecondaryResult = _
                                m_IReflect.InvokeMember( _
                                    MethodName, _
                                    InvocationFlags Or DefaultLookupFlags, _
                                    Nothing, _
                                    m_Instance, _
                                    Nothing, _
                                    Modifiers, _
                                    Nothing, _
                                    Nothing)
                            '
                            'END: SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY SECURITY
                            '
                        Catch InvocationError As MissingMemberException
                            Throw _
                                New MissingMemberException( _
                                    GetResourceString( _
                                        ResID.MissingMember_MemberNotFoundOnType2, _
                                        MethodName, _
                                        Me.VBFriendlyName), _
                                    InvocationError)
                        End Try
                    Catch exInner As AccessViolationException
                        Throw exInner
                    Catch exInner As StackOverflowException
                        Throw exInner
                    Catch exInner As OutOfMemoryException
                        Throw exInner
                    Catch exInner As System.Threading.ThreadAbortException
                        Throw exInner
                    Catch
                        SecondaryResult = Nothing
                    End Try

                    If SecondaryResult Is Nothing Then
                        Throw _
                            New MissingMemberException( _
                                GetResourceString( _
                                    ResID.MissingMember_MemberNotFoundOnType2, _
                                    MethodName, _
                                    Me.VBFriendlyName))
                    Else
                        Try
                            Return NewLateBinding.LateIndexGet(SecondaryResult, Arguments, ArgumentNames)
                        Catch exInner As Exception When IsMissingMemberException(exInner) AndAlso (TypeOf ex Is MissingMemberException)
                            Throw ex
                        End Try
                    End If

                Catch ex As TargetInvocationException
                    Throw ex.InnerException
                End Try

            End Function
#End If

        End Class

        Friend NotInheritable Class Method

            Private m_Item As MemberInfo                     'The underlying method or property reflection object.
            Private m_RawItem As MethodBase                  'The unsubstituted raw generic method.
            Private m_Parameters As ParameterInfo()          'The parameters used for this method by overload resolution.
            Private m_RawParameters As ParameterInfo()       'The unsubstituted raw parameters of a generic method.
            Private m_RawParametersFromType As ParameterInfo() 'The unsubstituted raw parameters of a generic method in the raw type.
            Private m_RawDeclaringType As Type               'The uninstantiated type containing this method.

            Friend ReadOnly ParamArrayIndex As Integer       'The index of the ParamArray in the parameters array, -1 if method has no ParamArray.
            Friend ReadOnly ParamArrayExpanded As Boolean    'Indicates if this method's ParamArray should be considered in its expanded form.

            Friend NotCallable As Boolean                    'Indicates if this method has been rejected as uncallable.
            Friend RequiresNarrowingConversion As Boolean    'Indicates if an argument requires narrowing one of the method's parameters.
            Friend AllNarrowingIsFromObject As Boolean       'Indicates if the type of all arguments which require narrowing to this method's parameters are Object.
            Friend LessSpecific As Boolean                   'Indicates is this method loses the competition for most specific procedure.

            Friend ArgumentsValidated As Boolean             'Indicates if the arguments have been validated against this method.
            Friend NamedArgumentMapping As Integer()         'Table of indices into the argument array for mapping named arguments to parameters.
            Friend TypeArguments As Type()                   'Set of type arguments either supplied or inferred for this method.
            Friend ArgumentMatchingDone As Boolean           'Indicates whether the argument matching task (CanMatchArguments) has already been completed for this Method

            Private Sub New( _
                    ByVal Parameters As ParameterInfo(), _
                    ByVal ParamArrayIndex As Integer, _
                    ByVal ParamArrayExpanded As Boolean)

                Me.m_Parameters = Parameters
                Me.m_RawParameters = Parameters
                Me.ParamArrayIndex = ParamArrayIndex
                Me.ParamArrayExpanded = ParamArrayExpanded

                Me.AllNarrowingIsFromObject = True  'Assume True until non-object narrowing is encountered.
            End Sub

            Friend Sub New( _
                    ByVal Method As MethodBase, _
                    ByVal Parameters As ParameterInfo(), _
                    ByVal ParamArrayIndex As Integer, _
                    ByVal ParamArrayExpanded As Boolean)

                MyClass.New(Parameters, ParamArrayIndex, ParamArrayExpanded)
                Me.m_Item = Method
                Me.m_RawItem = Method
            End Sub

            Friend Sub New( _
                    ByVal [Property] As PropertyInfo, _
                    ByVal Parameters As ParameterInfo(), _
                    ByVal ParamArrayIndex As Integer, _
                    ByVal ParamArrayExpanded As Boolean)

                MyClass.New(Parameters, ParamArrayIndex, ParamArrayExpanded)
                Me.m_Item = [Property]
            End Sub

            Friend ReadOnly Property Parameters() As ParameterInfo()
                Get
                    Return m_Parameters
                End Get
            End Property

            Friend ReadOnly Property RawParameters() As ParameterInfo()
                Get
                    'After a generic method has been bound, we still need access
                    'to the raw, unbound parameters.
                    Return m_RawParameters
                End Get
            End Property

            Friend ReadOnly Property RawParametersFromType() As ParameterInfo()
                Get
                    If m_RawParametersFromType Is Nothing Then
                        If Not IsProperty Then
                            Dim MethodToken As Integer = m_Item.MetadataToken
                            Dim DeclaringType As Type = m_Item.DeclaringType

                            Dim RawMethod As MethodBase = DeclaringType.Module.ResolveMethod(MethodToken, Nothing, Nothing)

                            m_RawParametersFromType = RawMethod.GetParameters()
                        Else
                            m_RawParametersFromType = m_RawParameters
                        End If
                    End If

                    Return m_RawParametersFromType
                End Get
            End Property

            Friend ReadOnly Property DeclaringType() As Type
                Get
                    Return m_Item.DeclaringType
                End Get
            End Property

            Friend ReadOnly Property RawDeclaringType() As Type
                Get
                    If m_RawDeclaringType Is Nothing Then
                        Dim DeclaringType As Type = m_Item.DeclaringType
                        Dim TypeToken As Integer = DeclaringType.MetadataToken

                        m_RawDeclaringType = DeclaringType.Module.ResolveType(TypeToken, Nothing, Nothing)
                    End If

                    Return m_RawDeclaringType
                End Get
            End Property

            Friend ReadOnly Property HasParamArray() As Boolean
                Get
                    Return ParamArrayIndex > -1
                End Get
            End Property

            'UNDONE: this is slow -- can't it be made better by caching the value away?
            Friend ReadOnly Property HasByRefParameter() As Boolean
                Get
                    For Each Parameter As ParameterInfo In Parameters
                        If Parameter.ParameterType.IsByRef Then
                            Return True
                        End If
                    Next
                    Return False
                End Get
            End Property

            Friend ReadOnly Property IsProperty() As Boolean
                Get
                    Return m_Item.MemberType = MemberTypes.Property
                End Get
            End Property

            Friend ReadOnly Property IsMethod() As Boolean
                Get
                    Return m_Item.MemberType = MemberTypes.Method OrElse _
                           m_Item.MemberType = MemberTypes.Constructor
                End Get
            End Property

            Friend ReadOnly Property IsGeneric() As Boolean
                Get
                    Return Symbols.IsGeneric(m_Item)
                End Get
            End Property

            Friend ReadOnly Property TypeParameters() As Type()
                Get
                    'CONSIDER: for performance, cache away this result since each consecutive call creates
                    'a clone of the type parameter array.
                    Return Symbols.GetTypeParameters(m_Item)
                End Get
            End Property

            Friend Function BindGenericArguments() As Boolean
                'This function instantiates a generic method with the type arguments supplied or inferred
                'for this method.
                '
                'ISSUE  (3/4/2004): Constructing the generic binding using Reflection peforms
                '                          constraint checking. This is bad if the binding will be used
                '                          to resolve overloaded calls since constraints should not participate
                '                          in the selection process. Instead, constraints should be checked
                '                          after overload resolution has selected a method. For now, there is
                '                          nothing reasonble we can do since Reflection does not allow the 
                '                          instantiation of generic methods with arguments that violate the
                '                          constraints. If a violation occurs, catch the exception and return
                '                          false signifying that the binding failed.

                Debug.Assert(Me.ArgumentsValidated, "can't bind without validating arguments")
                Debug.Assert(Me.IsMethod, "binding to a non-method")
                Try
                    'We use the original raw generic method so we can rebind an already bound Method.
                    m_Item = DirectCast(m_RawItem, MethodInfo).MakeGenericMethod(TypeArguments)
                    m_Parameters = Me.AsMethod.GetParameters
                    Return True
                Catch ex As ArgumentException
                    Return False
                End Try
            End Function

            Friend Function AsMethod() As MethodBase
                Debug.Assert(Me.IsMethod, "casting a non-method to a method")
                Return TryCast(m_Item, MethodBase)
            End Function

            Friend Function AsProperty() As PropertyInfo
                Debug.Assert(Me.IsProperty, "casting a non-property to a property")
                Return TryCast(m_Item, PropertyInfo)
            End Function

            Public Shared Operator =(ByVal Left As Method, ByVal Right As Method) As Boolean
                Return Left.m_Item Is Right.m_Item
            End Operator

            Public Shared Operator <>(ByVal Left As Method, ByVal right As Method) As Boolean
                Return Left.m_Item IsNot right.m_Item
            End Operator

            Public Shared Operator =(ByVal Left As MemberInfo, ByVal Right As Method) As Boolean
                Return Left Is Right.m_Item
            End Operator

            Public Shared Operator <>(ByVal Left As MemberInfo, ByVal Right As Method) As Boolean
                Return Left IsNot Right.m_Item
            End Operator

            Public Overrides Function ToString() As String
                Return MemberToString(m_Item)
            End Function

#If BINDING_LOG Then
            Private Function BoolStr(ByVal x As Boolean) As String
                If x Then Return "T" Else Return "F"
            End Function

            Friend Function DumpContents() As String
                Dim result As String = IIf(Me.IsMethod, "Meth", "Prop")
                result &= " " & m_Item.Name & " PAExpanded:" & BoolStr(ParamArrayExpanded) & " Container:" & m_Item.DeclaringType.Name & " ("
                For Each p As ParameterInfo In Parameters
                    result &= p.ParameterType.Name & ","
                Next
                result &= ")"
                result &= " PAIndex:" & CStr(ParamArrayIndex)
                result &= " NotCallable:" & BoolStr(NotCallable)
                result &= " ReqNar:" & BoolStr(RequiresNarrowingConversion)

                If RequiresNarrowingConversion Then
                    result &= " AllNarFromObj:" & BoolStr(AllNarrowingIsFromObject)
                End If

                Return result
            End Function
#End If

        End Class

        Friend NotInheritable Class TypedNothing
            'A class which represents a Nothing reference but with a particular type.
            'Normally, a Nothing reference converts to any type. However, during operator
            'resolution Nothing should match only one type.  This class acts as a place holder
            'for Nothing in the argument array and stores the Type which "Nothing" should have.

            Friend ReadOnly Type As Type

            Friend Sub New(ByVal Type As Type)
                Me.Type = Type
            End Sub
        End Class

    End Class

End Namespace
