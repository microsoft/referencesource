' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Collections.Generic
Imports System.Collections.ObjectModel
Imports System.Diagnostics
Imports System.Dynamic
Imports System.Linq.Expressions
Imports System.Reflection
Imports System.Runtime.CompilerServices

Imports Microsoft.VisualBasic.CompilerServices.NewLateBinding
Imports Microsoft.VisualBasic.CompilerServices.Symbols

Namespace Microsoft.VisualBasic.CompilerServices

    Friend Class IDOBinder

        Private Sub New()
            Throw New InternalErrorException()
        End Sub

        Private Structure SaveCopyBack
            Implements IDisposable

            ' We need to pass the CopyBack value from the VB binder through
            ' the DLR and into the Fallback.  Unfortunately the DLR APIs provide
            ' no obvious way to get the value from one place to the other.  So
            ' we store its value in a ThreadLocal here.
            <ThreadStatic()> _
            Private Shared SavedCopyBack As Boolean()

            Private oldCopyBack As Boolean()

            Public Sub New(ByVal copyBack As Boolean())
                ' Save values of thread statics
                oldCopyBack = SavedCopyBack

                ' Set new values
                SavedCopyBack = copyBack
            End Sub

            Public Sub Dispose() Implements System.IDisposable.Dispose
                ' Restore values of thread statics
                SavedCopyBack = oldCopyBack
            End Sub

            Friend Shared Function GetCopyBack() As Boolean()
                Return SavedCopyBack
            End Function
        End Structure

        ' A sentinel returned when no such member is found.
        Friend Shared ReadOnly missingMemberSentinel As Object = New Object()


        Friend Shared Function GetCopyBack() As Boolean()
            Return SaveCopyBack.GetCopyBack()
        End Function

        Friend Shared Function IDOCall( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal CopyBack As Boolean(), _
                ByVal IgnoreReturn As Boolean) As Object

            Dim s As New SaveCopyBack(CopyBack)
            Using s
                Dim CallInfo As CallInfo = Nothing
                Dim PackedArguments As Object() = Nothing
                IDOUtils.PackArguments(0, ArgumentNames, Arguments, PackedArguments, CallInfo)
                Try
                    Return IDOUtils.CreateRefCallSiteAndInvoke( _
                        New VBCallBinder(MemberName, CallInfo, IgnoreReturn), _
                        Instance, PackedArguments)
                Finally
                    IDOUtils.CopyBackArguments(CallInfo, PackedArguments, Arguments)
                End Try
            End Using
        End Function 'IDOCall

        Friend Shared Function IDOGet( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal CopyBack As Boolean()) As Object

            Dim s As New SaveCopyBack(CopyBack)
            Using s
                Dim PackedArguments As Object() = Nothing
                Dim CallInfo As CallInfo = Nothing
                IDOUtils.PackArguments(0, ArgumentNames, Arguments, PackedArguments, CallInfo)
                Try
                    Return IDOUtils.CreateRefCallSiteAndInvoke( _
                        New VBGetBinder(MemberName, CallInfo), _
                        Instance, PackedArguments)
                Finally
                    IDOUtils.CopyBackArguments(CallInfo, PackedArguments, Arguments)
                End Try
            End Using
        End Function 'IDOGet

        Friend Shared Function IDOInvokeDefault( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean, _
                ByVal CopyBack As Boolean()) As Object

            Dim s As New SaveCopyBack(CopyBack)
            Using s
                Dim PackedArguments As Object() = Nothing
                Dim CallInfo As CallInfo = Nothing
                IDOUtils.PackArguments(0, ArgumentNames, Arguments, PackedArguments, CallInfo)
                Try
                    Return IDOUtils.CreateRefCallSiteAndInvoke( _
                        New VBInvokeDefaultBinder(CallInfo, ReportErrors), _
                        Instance, PackedArguments)
                Finally
                    IDOUtils.CopyBackArguments(CallInfo, PackedArguments, Arguments)
                End Try
            End Using
        End Function 'IDOInvokeDefault

        Friend Shared Function IDOFallbackInvokeDefault( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean, _
                ByVal CopyBack As Boolean()) As Object

            Dim s As New SaveCopyBack(CopyBack)
            Using s
                Dim PackedArguments As Object() = Nothing
                Dim CallInfo As CallInfo = Nothing
                IDOUtils.PackArguments(0, ArgumentNames, Arguments, PackedArguments, CallInfo)
                Try
                    Return IDOUtils.CreateRefCallSiteAndInvoke( _
                        New VBInvokeDefaultFallbackBinder(CallInfo, ReportErrors), _
                        Instance, PackedArguments)
                Finally
                    IDOUtils.CopyBackArguments(CallInfo, PackedArguments, Arguments)
                End Try
            End Using

        End Function 'IDOFallbackInvokeDefault

        Friend Shared Sub IDOSet( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal MemberName As String, _
                ByVal ArgumentNames() As String, _
                ByVal Arguments As Object())

            Dim s As New SaveCopyBack(Nothing)
            Using s
                If Arguments.Length = 1 Then
                    IDOUtils.CreateFuncCallSiteAndInvoke( _
                        New VBSetBinder(MemberName), Instance, Arguments)
                Else
                    ' Look for a DLR member that might be an array
                    Dim member As Object = IDOUtils.CreateFuncCallSiteAndInvoke( _
                        New VBGetMemberBinder(MemberName), Instance, NoArguments)
                    If member Is IDOBinder.missingMemberSentinel Then ' found no DLR member by this name
                        NewLateBinding.ObjectLateSet( _
                            Instance, Nothing, MemberName, Arguments, ArgumentNames, NoTypeArguments)
                    Else
                        ' Treat the found DLR member as an array
                        NewLateBinding.LateIndexSet(member, Arguments, ArgumentNames)
                    End If
                End If
            End Using
        End Sub 'IDOSet

        Friend Shared Sub IDOSetComplex( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            Dim s As New SaveCopyBack(Nothing)
            Using s
                If Arguments.Length = 1 Then
                    IDOUtils.CreateFuncCallSiteAndInvoke( _
                        New VBSetComplexBinder(MemberName, OptimisticSet, RValueBase), Instance, Arguments)
                Else
                    ' Look for a DLR member that might be an array
                    Dim member As Object = IDOUtils.CreateFuncCallSiteAndInvoke( _
                        New VBGetMemberBinder(MemberName), Instance, NoArguments)
                    If member Is IDOBinder.missingMemberSentinel Then ' found no DLR member by this name
                        NewLateBinding.ObjectLateSetComplex( _
                            Instance, Nothing, MemberName, Arguments, _
                            ArgumentNames, NoTypeArguments, OptimisticSet, RValueBase)
                    Else
                        ' Treat the found DLR member as an array
                        NewLateBinding.LateIndexSetComplex( _
                            member, Arguments, ArgumentNames, OptimisticSet, RValueBase)
                    End If
                End If
            End Using
        End Sub 'IDOSetComplex

        Friend Shared Sub IDOIndexSet( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String())

            Dim s As New SaveCopyBack(Nothing)
            Using s
                Dim PackedArguments As Object() = Nothing
                Dim CallInfo As CallInfo = Nothing
                IDOUtils.PackArguments(1, ArgumentNames, Arguments, PackedArguments, CallInfo)
                IDOUtils.CreateFuncCallSiteAndInvoke( _
                    New VBIndexSetBinder(CallInfo), _
                    Instance, PackedArguments)
            End Using
        End Sub 'IDOIndexSet

        Friend Shared Sub IDOIndexSetComplex( _
                ByVal Instance As IDynamicMetaObjectProvider, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            Dim s As New SaveCopyBack(Nothing)
            Using s
                Dim PackedArguments As Object() = Nothing
                Dim CallInfo As CallInfo = Nothing
                IDOUtils.PackArguments(1, ArgumentNames, Arguments, PackedArguments, CallInfo)
                IDOUtils.CreateFuncCallSiteAndInvoke( _
                    New VBIndexSetComplexBinder(CallInfo, OptimisticSet, RValueBase), _
                    Instance, PackedArguments)
            End Using
        End Sub 'IDOIndexSetComplex

        Friend Shared Function UserDefinedConversion( _
                ByVal Expression As IDynamicMetaObjectProvider, _
                ByVal TargetType As System.Type) As Object

            Return IDOUtils.CreateConvertCallSiteAndInvoke( _
                New VBConversionBinder(TargetType), _
                Expression)
        End Function 'UserDefinedConversion

        Friend Shared Function InvokeUserDefinedOperator( _
                ByVal Op As UserDefinedOperator, _
                ByVal Arguments As Object()) As Object

            Dim linqOp As ExpressionType? = IDOUtils.LinqOperator(Op)
            If linqOp Is Nothing Then
                Return Operators.InvokeObjectUserDefinedOperator(Op, Arguments)
            Else
                Dim linqOperator As ExpressionType = CType(linqOp, ExpressionType)
                Dim opBinder As CallSiteBinder
                If Arguments.Length = 1 Then
                    opBinder = New VBUnaryOperatorBinder(Op, linqOperator)
                Else
                    opBinder = New VBBinaryOperatorBinder(Op, linqOperator)
                End If
                Dim Instance As Object = Arguments(0)
                Dim Args As Object() = _
                    If(Arguments.Length = 1, NoArguments, New Object() {Arguments(1)})
                Return IDOUtils.CreateFuncCallSiteAndInvoke(opBinder, Instance, Args)
            End If
        End Function 'InvokeUserDefinedOperator
    End Class 'IDOBinder

    Friend Class VBCallBinder
        Inherits InvokeMemberBinder

        Private ReadOnly _ignoreReturn As Boolean

        Sub New(ByVal MemberName As String, _
                ByVal CallInfo As CallInfo, _
                ByVal IgnoreReturn As Boolean)

            MyBase.New(MemberName, True, CallInfo)
            _ignoreReturn = IgnoreReturn
        End Sub

        Public Overloads Overrides Function FallbackInvokeMember( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs() As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedArgs) Then
                Return Me.Defer(target, packedArgs)
            End If

            Dim arguments As Expression() = Nothing
            Dim argNames As String() = Nothing
            Dim argValues As Object() = Nothing
            IDOUtils.UnpackArguments(packedArgs, Me.CallInfo, arguments, argNames, argValues)

            If errorSuggestion IsNot Nothing AndAlso Not CanBindCall(target.Value, Name, argValues, argNames, _ignoreReturn) Then
                Return errorSuggestion 'Binding will fail; use the IDO-provided error
            End If

            Dim result As ParameterExpression = Expression.Variable(GetType(Object), "result")
            Dim array As ParameterExpression = Expression.Variable(GetType(Object()), "array")

            Dim fallback As Expression = _
                Expression.Call( _
                    GetType(NewLateBinding).GetMethod("FallbackCall"), _
                    target.Expression(), _
                    Expression.Constant(Name, GetType(String)), _
                    Expression.Assign( _
                        array, _
                        Expression.NewArrayInit(GetType(Object), arguments) _
                    ), _
                    Expression.Constant(argNames, GetType(String())), _
                    Expression.Constant(_ignoreReturn, GetType(Boolean)) _
                )

            Return New DynamicMetaObject( _
                Expression.Block( _
                    New ParameterExpression() {result, array}, _
                    Expression.Assign(result, fallback), _
                    IDOUtils.GetWriteBack(arguments, array), _
                    result _
                ), _
                IDOUtils.CreateRestrictions(target, packedArgs) _
            )
        End Function 'FallbackInvokeMember

        Public Overloads Overrides Function FallbackInvoke( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs() As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            Return New VBInvokeBinder(Me.CallInfo, True).FallbackInvoke(target, packedArgs, errorSuggestion)
        End Function 'FallbackInvoke

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBCallBinder = TryCast(_other, VBCallBinder)
            Return other IsNot Nothing AndAlso String.Equals(Name, other.Name) AndAlso CallInfo.Equals(other.CallInfo) AndAlso _ignoreReturn = other._ignoreReturn
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBCallBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Name.GetHashCode() Xor CallInfo.GetHashCode() Xor _ignoreReturn.GetHashCode()
        End Function
    End Class 'VBCallBinder

    Friend Class VBGetBinder
        Inherits InvokeMemberBinder

        Sub New(ByVal MemberName As String, _
                ByVal CallInfo As CallInfo)
            MyBase.New(MemberName, True, CallInfo)
        End Sub 'New

        Public Overloads Overrides Function FallbackInvokeMember( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs() As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedArgs) Then
                Return Me.Defer(target, packedArgs)
            End If

            Dim arguments As Expression() = Nothing
            Dim argNames As String() = Nothing
            Dim argValues As Object() = Nothing
            IDOUtils.UnpackArguments(packedArgs, Me.CallInfo, arguments, argNames, argValues)

            If errorSuggestion IsNot Nothing AndAlso Not CanBindGet(target.Value, Name, argValues, argNames) Then
                Return errorSuggestion 'Binding will fail; use the IDO-provided error
            End If

            Dim result As ParameterExpression = Expression.Variable(GetType(Object), "result")
            Dim array As ParameterExpression = Expression.Variable(GetType(Object()), "array")

            Dim fallback As Expression = _
                Expression.Call( _
                    GetType(NewLateBinding).GetMethod("FallbackGet"), _
                    target.Expression(), _
                    Expression.Constant(Name), _
                    Expression.Assign( _
                        array, _
                        Expression.NewArrayInit(GetType(Object), arguments) _
                    ), _
                    Expression.Constant(argNames, GetType(String())) _
                )

            Return New DynamicMetaObject( _
                Expression.Block( _
                    New ParameterExpression() {result, array}, _
                    Expression.Assign(result, fallback), _
                    IDOUtils.GetWriteBack(arguments, array), _
                    result _
                ), _
                IDOUtils.CreateRestrictions(target, packedArgs) _
            )
        End Function 'FallbackInvokeMember

        Public Overrides Function FallbackInvoke( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs() As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            Return New VBInvokeBinder(Me.CallInfo, False).FallbackInvoke(target, packedArgs, errorSuggestion)
        End Function 'FallbackInvoke

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBGetBinder = TryCast(_other, VBGetBinder)
            Return other IsNot Nothing AndAlso String.Equals(Name, other.Name) AndAlso CallInfo.Equals(other.CallInfo)
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBGetBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Name.GetHashCode() Xor CallInfo.GetHashCode()
        End Function
    End Class 'VBGetBinder


    ' Implements FallbackInvoke for VBCallBinder and VBGetBinder
    Class VBInvokeBinder
        Inherits InvokeBinder

        ' True if this is coming from LateCall, false if it's for LateGet
        Private ReadOnly _lateCall As Boolean

        Public Sub New(ByVal CallInfo As CallInfo, ByVal LateCall As Boolean)
            MyBase.New(CallInfo)
            _lateCall = LateCall
        End Sub

        Public Overloads Overrides Function FallbackInvoke( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs() As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedArgs) Then
                Return Me.Defer(target, packedArgs)
            End If

            ' The DLR resolved o.member, but not o.member(args).  We need to apply
            ' the default action.  If there are no args and no default action, though,
            ' it's an error (hence ReportErrors = True).  These semantics are embedded in
            ' a new internal-only entry point, "LateCallInvokeDefault".

            Dim arguments As Expression() = Nothing
            Dim argNames As String() = Nothing
            Dim argValues As Object() = Nothing
            IDOUtils.UnpackArguments(packedArgs, Me.CallInfo, arguments, argNames, argValues)

            If errorSuggestion IsNot Nothing AndAlso Not CanBindInvokeDefault(target.Value, argValues, argNames, _lateCall) Then
                Return errorSuggestion 'Use the IDO-provided error
            End If

            Dim result As ParameterExpression = Expression.Variable(GetType(Object), "result")
            Dim array As ParameterExpression = Expression.Variable(GetType(Object()), "array")

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod(If(_lateCall, "LateCallInvokeDefault", "LateGetInvokeDefault")), _
                target.Expression(), _
                Expression.Assign( _
                    array, _
                    Expression.NewArrayInit(GetType(Object), arguments) _
                ), _
                Expression.Constant(argNames, GetType(String())), _
                Expression.Constant(_lateCall) _
            )

            Return New DynamicMetaObject( _
                Expression.Block( _
                    New ParameterExpression() {result, array}, _
                    Expression.Assign(result, fallback), _
                    IDOUtils.GetWriteBack(arguments, array), _
                    result _
                ), _
                IDOUtils.CreateRestrictions(target, packedArgs) _
            )
        End Function 'FallbackInvoke

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBInvokeBinder = TryCast(_other, VBInvokeBinder)
            Return other IsNot Nothing AndAlso CallInfo.Equals(other.CallInfo) AndAlso _lateCall.Equals(other._lateCall)
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBGetBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor CallInfo.GetHashCode() Xor _lateCall.GetHashCode()
        End Function
    End Class

    Class VBInvokeDefaultBinder
        Inherits InvokeBinder

        Private ReadOnly _reportErrors As Boolean

        Sub New(ByVal CallInfo As CallInfo, ByVal ReportErrors As Boolean)
            MyBase.New(CallInfo)
            Me._reportErrors = ReportErrors
        End Sub 'New

        Public Overloads Overrides Function FallbackInvoke( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs As DynamicMetaObject(), _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedArgs) Then
                Return Me.Defer(target, packedArgs)
            End If

            Dim arguments As Expression() = Nothing
            Dim argNames As String() = Nothing
            Dim argValues As Object() = Nothing
            IDOUtils.UnpackArguments(packedArgs, Me.CallInfo, arguments, argNames, argValues)

            If errorSuggestion IsNot Nothing AndAlso Not CanBindInvokeDefault(target.Value, argValues, argNames, _reportErrors) Then
                Return errorSuggestion 'Use the IDO-provided error
            End If

            Dim result As ParameterExpression = Expression.Variable(GetType(Object), "result")
            Dim array As ParameterExpression = Expression.Variable(GetType(Object()), "array")

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackInvokeDefault1"), _
                target.Expression(), _
                Expression.Assign( _
                    array, _
                    Expression.NewArrayInit(GetType(Object), arguments) _
                ), _
                Expression.Constant(argNames, GetType(String())), _
                Expression.Constant(_reportErrors) _
            )

            Return New DynamicMetaObject( _
                Expression.Block( _
                    New ParameterExpression() {result, array}, _
                    Expression.Assign(result, fallback), _
                    IDOUtils.GetWriteBack(arguments, array), _
                    result _
                ), _
                IDOUtils.CreateRestrictions(target, packedArgs) _
            )
        End Function 'FallbackInvoke

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBInvokeDefaultBinder = TryCast(_other, VBInvokeDefaultBinder)
            Return other IsNot Nothing AndAlso CallInfo.Equals(other.CallInfo) AndAlso _reportErrors = other._reportErrors
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBInvokeDefaultBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor CallInfo.GetHashCode() Xor _reportErrors.GetHashCode()
        End Function
    End Class 'VBInvokeDefaultBinder

    Class VBInvokeDefaultFallbackBinder
        Inherits GetIndexBinder

        Private ReadOnly _reportErrors As Boolean

        Sub New(ByVal CallInfo As CallInfo, ByVal ReportErrors As Boolean)
            MyBase.New(CallInfo)
            Me._reportErrors = ReportErrors
        End Sub 'New

        Public Overrides Function FallbackGetIndex( _
                ByVal target As DynamicMetaObject, _
                ByVal packedArgs As DynamicMetaObject(), _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedArgs) Then
                Return Me.Defer(target, packedArgs)
            End If

            Dim arguments As Expression() = Nothing
            Dim argNames As String() = Nothing
            Dim argValues As Object() = Nothing
            IDOUtils.UnpackArguments(packedArgs, Me.CallInfo, arguments, argNames, argValues)

            If errorSuggestion IsNot Nothing AndAlso Not CanBindInvokeDefault(target.Value, argValues, argNames, _reportErrors) Then
                Return errorSuggestion 'Use the IDO-provided error
            End If

            Dim result As ParameterExpression = Expression.Variable(GetType(Object), "result")
            Dim array As ParameterExpression = Expression.Variable(GetType(Object()), "array")

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackInvokeDefault2"), _
                target.Expression(), _
                    Expression.Assign( _
                        array, _
                        Expression.NewArrayInit(GetType(Object), arguments) _
                    ), _
                Expression.Constant(argNames, GetType(String())), _
                Expression.Constant(_reportErrors) _
            )

            Return New DynamicMetaObject( _
                Expression.Block( _
                    New ParameterExpression() {result, array}, _
                    Expression.Assign(result, fallback), _
                    IDOUtils.GetWriteBack(arguments, array), _
                    result _
                ), _
                IDOUtils.CreateRestrictions(target, packedArgs) _
            )
        End Function 'FallbackGetIndex

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBInvokeDefaultFallbackBinder = TryCast(_other, VBInvokeDefaultFallbackBinder)
            Return other IsNot Nothing AndAlso CallInfo.Equals(other.CallInfo) AndAlso _reportErrors = other._reportErrors
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBInvokeDefaultFallbackBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor CallInfo.GetHashCode() Xor _reportErrors.GetHashCode()
        End Function
    End Class 'VBInvokeDefaultFallbackBinder

    Class VBSetBinder
        Inherits SetMemberBinder

        Sub New(ByVal MemberName As String)
            MyBase.New(Name:=MemberName, IgnoreCase:=True)
        End Sub 'New

        Public Overloads Overrides Function FallbackSetMember( _
                    ByVal target As DynamicMetaObject, _
                    ByVal value As DynamicMetaObject, _
                    ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, value:=value) Then
                Return Me.Defer(target, value)
            End If

            If errorSuggestion IsNot Nothing AndAlso Not CanBindSet(target.Value, Name, value.Value, False, False) Then
                Return errorSuggestion 'Binding will fail; use the IDO-provided error
            End If

            Dim valueExpression As Expression = IDOUtils.ConvertToObject(value.Expression())
            Dim arguments() As Expression = {valueExpression}

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackSet"), _
                target.Expression(), _
                Expression.Constant(Name), _
                Expression.NewArrayInit(GetType(Object), arguments) _
            )

            Return New DynamicMetaObject( _
                Expression.Block(fallback, valueExpression), _
                IDOUtils.CreateRestrictions(target, value:=value) _
            )
        End Function

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBSetBinder = TryCast(_other, VBSetBinder)
            Return other IsNot Nothing AndAlso String.Equals(Name, other.Name)
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBSetBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Name.GetHashCode()
        End Function
    End Class 'VBSetBinder

    Class VBSetComplexBinder
        Inherits SetMemberBinder

        Private ReadOnly _optimisticSet As Boolean
        Private ReadOnly _rValueBase As Boolean

        Sub New(ByVal MemberName As String, ByVal OptimisticSet As Boolean, ByVal RValueBase As Boolean)
            MyBase.New(Name:=MemberName, IgnoreCase:=True)
            Me._optimisticSet = OptimisticSet
            Me._rValueBase = RValueBase
        End Sub 'New

        Public Overloads Overrides Function FallbackSetMember( _
                    ByVal target As DynamicMetaObject, _
                    ByVal value As DynamicMetaObject, _
                    ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, value:=value) Then
                Return Me.Defer(target, value)
            End If

            If errorSuggestion IsNot Nothing AndAlso Not CanBindSet(target.Value, Name, value.Value, _optimisticSet, _rValueBase) Then
                Return errorSuggestion 'Binding will fail; use the IDO-provided error
            End If

            Dim valueExpression As Expression = IDOUtils.ConvertToObject(value.Expression())
            Dim arguments() As Expression = {valueExpression}

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackSetComplex"), _
                target.Expression(), _
                Expression.Constant(Name), _
                Expression.NewArrayInit(GetType(Object), arguments), _
                Expression.Constant(_optimisticSet), _
                Expression.Constant(_rValueBase) _
            )

            Return New DynamicMetaObject( _
                Expression.Block(fallback, valueExpression), _
                IDOUtils.CreateRestrictions(target, value:=value) _
            )
        End Function

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBSetComplexBinder = TryCast(_other, VBSetComplexBinder)
            Return other IsNot Nothing AndAlso String.Equals(Name, other.Name) AndAlso _optimisticSet = other._optimisticSet AndAlso _rValueBase = other._rValueBase
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBSetComplexBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Name.GetHashCode() Xor _optimisticSet.GetHashCode() Xor _rValueBase.GetHashCode()
        End Function
    End Class 'VBSetComplexBinder

    ' Used to fetch a DLR field
    Class VBGetMemberBinder
        Inherits GetMemberBinder
#If TELESTO Then
        Implements IInvokeOnGetBinder
#End If
        Public Sub New(ByVal name As String)
            MyBase.New(name, True)
        End Sub 'New

        Public Overrides Function FallbackGetMember( _
                ByVal target As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If errorSuggestion IsNot Nothing Then
                Return errorSuggestion
            End If

            ' Return a flag indicating no such DLR field exists
            Return New DynamicMetaObject(Expression.Constant(IDOBinder.missingMemberSentinel), IDOUtils.CreateRestrictions(target))
        End Function 'FallbackGetMember

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBGetMemberBinder = TryCast(_other, VBGetMemberBinder)
            Return other IsNot Nothing AndAlso String.Equals(Name, other.Name)
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBGetMemberBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Name.GetHashCode()
        End Function
        
#If TELESTO Then
        ' Silverlight COM binding needs to know that it should not invoke a
        ' default property, and instead wait until we provide the indexing arguments
        Private ReadOnly Property InvokeOnGet() As Boolean Implements IInvokeOnGetBinder.InvokeOnGet
            Get
                Return False
            End Get
        End Property
#End If
    End Class 'VBGetMemberBinder

    Class VBConversionBinder
        Inherits ConvertBinder

        Sub New(ByVal T As Type)
            MyBase.New(T, True)
        End Sub 'New

        Public Overrides Function FallbackConvert( _
                ByVal target As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target) Then
                Return Me.Defer(target)
            End If

            If errorSuggestion IsNot Nothing AndAlso Not Conversions.CanUserDefinedConvert(target.Value, Me.Type()) Then
                'Can't convert, use the error provided by the IDO
                Return errorSuggestion
            End If

            Dim fallback As Expression = Expression.Call( _
                GetType(Conversions).GetMethod("FallbackUserDefinedConversion"), _
                target.Expression(), _
                Expression.Constant(Me.Type(), GetType(System.Type)) _
            )

            Return New DynamicMetaObject(Expression.Convert(fallback, ReturnType), IDOUtils.CreateRestrictions(target))

        End Function 'FallbackConvert

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBConversionBinder = TryCast(_other, VBConversionBinder)
        ' = Operator is not defined in .net 3.5. TELESTO needs to be built with .net 3.5 tools.
#If TELESTO Then
            Return other IsNot Nothing AndAlso Type.Equals(other.Type)
#Else
            Return other IsNot Nothing AndAlso Type = other.Type
#End If
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBConversionBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor Type.GetHashCode()
        End Function
    End Class 'VBConversionBinder

    Class VBUnaryOperatorBinder
        Inherits UnaryOperationBinder

        Private ReadOnly _Op As UserDefinedOperator

        Sub New(ByVal Op As UserDefinedOperator, ByVal LinqOp As ExpressionType)
            MyBase.New(LinqOp)
            _Op = Op
        End Sub 'New

        Public Overrides Function FallbackUnaryOperation( _
                ByVal target As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target) Then
                Return Me.Defer(target)
            End If

            If errorSuggestion IsNot Nothing AndAlso Operators.GetCallableUserDefinedOperator(_Op, target.Value) Is Nothing Then
                'Can't bind, use the error provided by the IDO
                Return errorSuggestion
            End If

            Dim fallback As Expression = Expression.Call( _
                GetType(Operators).GetMethod("FallbackInvokeUserDefinedOperator"), _
                Expression.Constant(_Op, GetType(Object)), _
                Expression.NewArrayInit(GetType(Object), New Expression() {IDOUtils.ConvertToObject(target.Expression)}) _
            )

            Return New DynamicMetaObject(fallback, IDOUtils.CreateRestrictions(target))
        End Function 'FallbackUnaryOperator

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBUnaryOperatorBinder = TryCast(_other, VBUnaryOperatorBinder)
            Return other IsNot Nothing AndAlso _Op = other._Op AndAlso Operation = other.Operation
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBUnaryOperatorBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor _Op.GetHashCode() Xor Operation.GetHashCode()
        End Function
    End Class 'VBUnaryOperatorBinder

    Class VBBinaryOperatorBinder
        Inherits BinaryOperationBinder

        Private ReadOnly _Op As UserDefinedOperator

        Sub New(ByVal Op As UserDefinedOperator, ByVal LinqOp As ExpressionType)
            MyBase.New(LinqOp)
            _Op = Op
        End Sub 'New

        Public Overrides Function FallbackBinaryOperation( _
                ByVal target As DynamicMetaObject, _
                ByVal arg As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, value:=arg) Then
                Return Me.Defer(target, arg)
            End If

            If errorSuggestion IsNot Nothing AndAlso Operators.GetCallableUserDefinedOperator(_Op, target.Value, arg.Value) Is Nothing Then
                'Can't bind, use the error provided by the IDO
                Return errorSuggestion
            End If

            Dim fallback As Expression = Expression.Call( _
                GetType(Operators).GetMethod("FallbackInvokeUserDefinedOperator"), _
                Expression.Constant(_Op, GetType(Object)), _
                Expression.NewArrayInit(GetType(Object), New Expression() { _
                            IDOUtils.ConvertToObject(target.Expression), _
                            IDOUtils.ConvertToObject(arg.Expression)}) _
            )

            Return New DynamicMetaObject(fallback, IDOUtils.CreateRestrictions(target, value:=arg))
        End Function 'FallbackUnaryOperator

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBBinaryOperatorBinder = TryCast(_other, VBBinaryOperatorBinder)
            Return other IsNot Nothing AndAlso _Op = other._Op AndAlso Operation = other.Operation
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBBinaryOperatorBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor _Op.GetHashCode() Xor Operation.GetHashCode()
        End Function
    End Class 'VBBinaryOperatorBinder

    Class VBIndexSetBinder
        Inherits SetIndexBinder

        Sub New(ByVal CallInfo As CallInfo)
            MyBase.New(CallInfo)
        End Sub 'New

        Public Overrides Function FallbackSetIndex( _
                ByVal target As DynamicMetaObject, _
                ByVal packedIndexes As DynamicMetaObject(), _
                ByVal value As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedIndexes, value) Then
                Array.Resize(packedIndexes, packedIndexes.Length + 1)
                packedIndexes(packedIndexes.Length - 1) = value
                Return Me.Defer(target, packedIndexes)
            End If

            Dim indexNames As String() = Nothing
            Dim indexes As Expression() = Nothing
            Dim indexValues As Object() = Nothing

            IDOUtils.UnpackArguments(packedIndexes, Me.CallInfo, indexes, indexNames, indexValues)

            Dim indexValuesPlusValue(indexValues.Length) As Object
            indexValues.CopyTo(indexValuesPlusValue, 0)
            indexValuesPlusValue(indexValues.Length) = value.Value

            If errorSuggestion IsNot Nothing AndAlso _
                Not CanIndexSetComplex(target.Value, indexValuesPlusValue, indexNames, False, False) Then
                Return errorSuggestion 'Use the IDO-provided error
            End If

            Dim valueExpression As Expression = IDOUtils.ConvertToObject(value.Expression)
            Dim indexesPlusValue(indexes.Length) As Expression
            indexes.CopyTo(indexesPlusValue, 0)
            indexesPlusValue(indexes.Length) = valueExpression

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackIndexSet"), _
                target.Expression(), _
                Expression.NewArrayInit(GetType(Object), indexesPlusValue), _
                Expression.Constant(indexNames, GetType(String())) _
            )

            Return New DynamicMetaObject( _
                Expression.Block(fallback, valueExpression), _
                IDOUtils.CreateRestrictions(target, packedIndexes, value) _
            )
        End Function 'FallbackSetIndex

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBIndexSetBinder = TryCast(_other, VBIndexSetBinder)
            Return other IsNot Nothing AndAlso CallInfo.Equals(other.CallInfo)
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBIndexSetBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor CallInfo.GetHashCode()
        End Function
    End Class 'VBIndexSetBinder

    Class VBIndexSetComplexBinder
        Inherits SetIndexBinder

        Private ReadOnly _optimisticSet As Boolean
        Private ReadOnly _rValueBase As Boolean

        Sub New(ByVal CallInfo As CallInfo, ByVal OptimisticSet As Boolean, ByVal RValueBase As Boolean)
            MyBase.New(CallInfo)
            Me._optimisticSet = OptimisticSet
            Me._rValueBase = RValueBase
        End Sub 'New

        Public Overrides Function FallbackSetIndex( _
                ByVal target As DynamicMetaObject, _
                ByVal packedIndexes As DynamicMetaObject(), _
                ByVal value As DynamicMetaObject, _
                ByVal errorSuggestion As DynamicMetaObject) As DynamicMetaObject

            If IDOUtils.NeedsDeferral(target, packedIndexes, value) Then
                Array.Resize(packedIndexes, packedIndexes.Length + 1)
                packedIndexes(packedIndexes.Length - 1) = value
                Return Me.Defer(target, packedIndexes)
            End If

            Dim indexNames As String() = Nothing
            Dim indexes As Expression() = Nothing
            Dim indexValues As Object() = Nothing

            IDOUtils.UnpackArguments(packedIndexes, Me.CallInfo, indexes, indexNames, indexValues)

            Dim indexValuesPlusValue(indexValues.Length) As Object
            indexValues.CopyTo(indexValuesPlusValue, 0)
            indexValuesPlusValue(indexValues.Length) = value.Value

            If errorSuggestion IsNot Nothing AndAlso _
                Not CanIndexSetComplex(target.Value, indexValuesPlusValue, indexNames, _optimisticSet, _rValueBase) Then
                Return errorSuggestion 'Use the IDO-provided error
            End If

            Dim valueExpression As Expression = IDOUtils.ConvertToObject(value.Expression)
            Dim indexesPlusValue(indexes.Length) As Expression
            indexes.CopyTo(indexesPlusValue, 0)
            indexesPlusValue(indexes.Length) = valueExpression

            Dim fallback As Expression = Expression.Call( _
                GetType(NewLateBinding).GetMethod("FallbackIndexSetComplex"), _
                target.Expression(), _
                Expression.NewArrayInit(GetType(Object), indexesPlusValue), _
                Expression.Constant(indexNames, GetType(String())), _
                Expression.Constant(_optimisticSet), _
                Expression.Constant(_rValueBase) _
            )

            Return New DynamicMetaObject( _
                Expression.Block(fallback, valueExpression), _
                IDOUtils.CreateRestrictions(target, packedIndexes, value) _
            )
        End Function 'FallbackSetIndex

        ' Implement value equality. This is used so we can discover previously produced rules.
        ' See comment at IOUtils.GetCachedBinder, which explains the caching in more detail.
        Public Overrides Function Equals(ByVal _other As Object) As Boolean
            Dim other As VBIndexSetComplexBinder = TryCast(_other, VBIndexSetComplexBinder)
            Return other IsNot Nothing AndAlso CallInfo.Equals(other.CallInfo) AndAlso _optimisticSet = other._optimisticSet AndAlso _rValueBase = other._rValueBase
        End Function

        Private Shared ReadOnly _hash As Integer = GetType(VBIndexSetComplexBinder).GetHashCode()
        Public Overrides Function GetHashCode() As Integer
            Return _hash Xor CallInfo.GetHashCode() Xor _optimisticSet.GetHashCode() Xor _rValueBase.GetHashCode()
        End Function
    End Class 'VBIndexSetComplexBinder

    Friend Class IDOUtils

        Private Sub New()
            Throw New InternalErrorException()
        End Sub

        ' Each binder will cache up to 128 of it's most recently used rules.
        ' So by caching the 64 most recently used binders, we limit the total
        ' number of rules to 8k.
        Private Shared binderCache As New CacheSet(Of CallSiteBinder)(64)

        '
        ' Look for an existing compatible binder in the cache. If we find one,
        ' we can reuse the rules that it produced. If we don't find a match,
        ' then add this binder to the cache.
        '
        ' Compatibility is determined by the Equals method on the binders. Two
        ' binders should compare equal if they would produce the same rule for
        ' the same arguments. In practice, this is true if all of their
        ' instance fields are equal.
        '
        ' Consider this example:
        '   x.Foo(a)
        '   y.Foo(b)
        '
        ' Both of these call sites are calling "Foo" with one argument, so they
        ' can potentially use the same generated rule. Constrast with:
        '   z.Foo(c, d)
        '
        ' Now we have two arguments, so we can't share rules with the other two
        ' call sites.
        '
        Private Shared Function GetCachedBinder(ByVal Action As CallSiteBinder) As CallSiteBinder
            Return binderCache.GetExistingOrAdd(Action)
        End Function 'GetAtomizedBinder

        ' This method checks whether an object is an instance of IDynamicMetaObjectProvider.
        ' Apparently, for remote objects (objects in a different process), CLR will report
        ' allow cast to an interface (isinst instruction returns non-null) even though the object
        ' doesn't implement the interface. Therefore we are checking that the object resides 
        ' in the same app domain in addition to implementing the IDynamicMetaObjectProcider interface.
        Friend Shared Function TryCastToIDMOP(ByVal o As Object) As IDynamicMetaObjectProvider
            Dim ido As IDynamicMetaObjectProvider = TryCast(o, IDynamicMetaObjectProvider)
#If Not TELESTO Then
            If ido IsNot Nothing AndAlso Not System.Runtime.Remoting.RemotingServices.IsObjectOutOfAppDomain(o) Then
#Else
            If ido IsNot Nothing Then 'No RemotingServices in Telesto                
#End If
                Return ido
            Else
                Return Nothing
            End If
        End Function

        ' Convert from VB's UserDefinedOperator to Linq operator type
        Friend Shared Function LinqOperator(ByVal vbOperator As UserDefinedOperator) As ExpressionType?

            Select Case vbOperator
                Case UserDefinedOperator.Negate
                    Return ExpressionType.Negate
                Case UserDefinedOperator.Not
                    Return ExpressionType.Not
                Case UserDefinedOperator.UnaryPlus
                    Return ExpressionType.UnaryPlus
                Case UserDefinedOperator.Plus
                    Return ExpressionType.Add
                Case UserDefinedOperator.Minus
                    Return ExpressionType.Subtract
                Case UserDefinedOperator.Multiply
                    Return ExpressionType.Multiply
                Case UserDefinedOperator.Divide
                    Return ExpressionType.Divide
                Case UserDefinedOperator.Power
                    Return ExpressionType.Power
                Case UserDefinedOperator.ShiftLeft
                    Return ExpressionType.LeftShift
                Case UserDefinedOperator.ShiftRight
                    Return ExpressionType.RightShift
                Case UserDefinedOperator.Modulus
                    Return ExpressionType.Modulo
                Case UserDefinedOperator.Or
                    Return ExpressionType.Or
                Case UserDefinedOperator.Xor
                    Return ExpressionType.ExclusiveOr
                Case UserDefinedOperator.And
                    Return ExpressionType.And
                Case UserDefinedOperator.Equal
                    Return ExpressionType.Equal
                Case UserDefinedOperator.NotEqual
                    Return ExpressionType.NotEqual
                Case UserDefinedOperator.Less
                    Return ExpressionType.LessThan
                Case UserDefinedOperator.LessEqual
                    Return ExpressionType.LessThanOrEqual
                Case UserDefinedOperator.GreaterEqual
                    Return ExpressionType.GreaterThanOrEqual
                Case UserDefinedOperator.Greater
                    Return ExpressionType.GreaterThan
                Case Else
                    Return Nothing
            End Select
        End Function 'LinqOperator

        'If the CallSite had byref arguments, the values in packedArgs may be updated
        'We need to propegate those changes back to the original arguments array.
        Shared Sub CopyBackArguments(ByVal callInfo As CallInfo, ByVal packedArgs As Object(), ByVal args As Object())
            If packedArgs IsNot args Then
                'This works like UnpackArguments, but just copies the values
                '
                'We need to reorder the args if we have any named args so it matches
                'what the Late* entry point expects, which is named args first.
                'Input order is:  P1, P2, P3, N1, N2 [, V]
                'Output order is: N1, N2, P1, P2, P3 [, V]
                '(where V is an the value argument for things like SetIndex)
                Dim argCount As Integer = packedArgs.Length
                Dim normalArgCount As Integer = callInfo.ArgumentCount
                Dim positionalArgCount As Integer = argCount - callInfo.ArgumentNames.Count

                For i As Integer = 0 To argCount - 1
                    args(i) = packedArgs(If(i < normalArgCount, (i + positionalArgCount) Mod normalArgCount, i))
                Next
            End If
        End Sub

        'Pack arguments from VB libraries for DLR
        Shared Sub PackArguments( _
                ByVal valueArgs As Integer, _
                ByVal argNames As String(), _
                ByVal args As Object(), _
                ByRef packedArgs As Object(), _
                ByRef callInfo As CallInfo)

            'There is some inconsistency in the handling of argNames, sometimes it
            'has been normalized to non-null by this point, sometimes not.
            If argNames Is Nothing Then
                argNames = New String(-1) {}
            End If

            callInfo = New CallInfo(args.Length - valueArgs, argNames)

            If argNames.Length > 0 Then
                'Arguments are passed to NewLateBinder a counterintuitive way, with
                'named arguments first in the array. So we need to reorder them to get
                'correct interop.
                'See ExpressionSemantics.cpp, ConstructLateBoundArgumentList
                packedArgs = New Object(args.Length - 1) {}

                'Input order is:  N1, N2, P1, P2, P3 [, V]
                'Output order is: P1, P2, P3, N1, N2 [, V]
                '(where V is an the value argument for things like SetIndex)
                Dim normalArgCount As Integer = args.Length - valueArgs
                For i As Integer = 0 To normalArgCount - 1
                    packedArgs(i) = args((i + argNames.Length) Mod normalArgCount)
                Next i
                ' Copy the value arguments (for SetIndex*), if any
                For i As Integer = normalArgCount To args.Length - 1
                    packedArgs(i) = args(i)
                Next
            Else
                packedArgs = args
            End If
        End Sub 'PackArguments

        'Unpack arguments from DLR for VB libraries
        Shared Sub UnpackArguments( _
                ByVal packedArgs As DynamicMetaObject(), _
                ByVal callInfo As CallInfo, _
                ByRef args As Expression(), _
                ByRef argNames As String(), _
                ByRef argValues As Object())

            'See comment for PackArguments
            'We need to reorder the args if we have any named args so it matches
            'what the Late* entry point expects, which is named args first.
            'Input order is:  P1, P2, P3, N1, N2 [, V]
            'Output order is: N1, N2, P1, P2, P3 [, V]
            '(where V is an the value argument for things like SetIndex)

            Dim argCount As Integer = packedArgs.Length
            Dim normalArgCount As Integer = CallInfo.ArgumentCount
            args = New Expression(argCount - 1) {}
            argValues = New Object(argCount - 1) {}

            Dim namedArgCount As Integer = CallInfo.ArgumentNames.Count
            Dim positionalArgCount As Integer = argCount - namedArgCount

            For i As Integer = 0 To normalArgCount - 1
                Dim p As DynamicMetaObject = packedArgs((i + positionalArgCount) Mod normalArgCount)
                args(i) = p.Expression
                argValues(i) = p.Value
            Next
            ' Copy the value arguments (for SetIndex*), if any
            For i As Integer = normalArgCount To argCount - 1
                Dim p As DynamicMetaObject = packedArgs(i)
                args(i) = p.Expression
                argValues(i) = p.Value
            Next

            ' Binding functions expect non-null names
            argNames = New String(namedArgCount - 1) {}
            CallInfo.ArgumentNames.CopyTo(argNames, 0)
        End Sub 'UnpackArguments

        Shared Function GetWriteBack(ByVal arguments() As Expression, ByVal array As ParameterExpression) As Expression
            Dim writeback As New List(Of Expression)
            For i As Integer = 0 To arguments.Length - 1
                Dim arg As ParameterExpression = TryCast(arguments(i), ParameterExpression)
                If arg IsNot Nothing AndAlso arg.IsByRef Then
                    writeback.Add(Expression.Assign(arg, Expression.ArrayIndex(array, Expression.Constant(i))))
                End If
            Next
            Select Case writeback.Count
                Case 0
                    Return Expression.Empty()
                Case 1
                    Return writeback(0)
                Case Else
                    Return Expression.Block(writeback)
            End Select
        End Function

        'Convert expression to Object if its type is not Object already.
        Shared Function ConvertToObject(ByVal valueExpression As Expression) As Expression
            Return If(valueExpression.Type.Equals(GetType(Object)), valueExpression, Expression.Convert(valueExpression, GetType(Object)))
        End Function 'ConvertToObject

        Friend Delegate Function SiteDelegate0(ByVal Site As CallSite, ByVal Instance As Object) As Object
        Friend Delegate Function SiteDelegate1(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object) As Object
        Friend Delegate Function SiteDelegate2(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object) As Object
        Friend Delegate Function SiteDelegate3(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object, ByRef Arg2 As Object) As Object
        Friend Delegate Function SiteDelegate4(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object, ByRef Arg2 As Object, ByRef Arg3 As Object) As Object
        Friend Delegate Function SiteDelegate5(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object, ByRef Arg2 As Object, ByRef Arg3 As Object, ByRef Arg4 As Object) As Object
        Friend Delegate Function SiteDelegate6(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object, ByRef Arg2 As Object, ByRef Arg3 As Object, ByRef Arg4 As Object, ByRef Arg5 As Object) As Object
        Friend Delegate Function SiteDelegate7(ByVal Site As CallSite, ByVal Instance As Object, ByRef Arg0 As Object, ByRef Arg1 As Object, ByRef Arg2 As Object, ByRef Arg3 As Object, ByRef Arg4 As Object, ByRef Arg5 As Object, ByRef Arg6 As Object) As Object

        Shared Function CreateRefCallSiteAndInvoke( _
                ByVal Action As CallSiteBinder, _
                ByVal Instance As Object, _
                ByVal Arguments As Object()) As Object

            Action = GetCachedBinder(Action)

            Select Case Arguments.Length
                Case 0
                    Dim c As CallSite(Of SiteDelegate0) = CallSite(Of SiteDelegate0).Create(Action)
                    Return c.Target.Invoke(c, Instance)
                Case 1
                    Dim c As CallSite(Of SiteDelegate1) = CallSite(Of SiteDelegate1).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0))
                Case 2
                    Dim c As CallSite(Of SiteDelegate2) = CallSite(Of SiteDelegate2).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1))
                Case 3
                    Dim c As CallSite(Of SiteDelegate3) = CallSite(Of SiteDelegate3).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2))
                Case 4
                    Dim c As CallSite(Of SiteDelegate4) = CallSite(Of SiteDelegate4).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3))
                Case 5
                    Dim c As CallSite(Of SiteDelegate5) = CallSite(Of SiteDelegate5).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4))
                Case 6
                    Dim c As CallSite(Of SiteDelegate6) = CallSite(Of SiteDelegate6).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4), Arguments(5))
                Case 7
                    Dim c As CallSite(Of SiteDelegate7) = CallSite(Of SiteDelegate7).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4), Arguments(5), Arguments(6))
                Case Else
                    Dim signature(Arguments.Length + 2) As Type
                    Dim refObject As Type = GetType(Object).MakeByRefType()
                    signature(0) = GetType(CallSite)                    ' First argument is a call site
                    signature(1) = GetType(Object)                      ' Second is the instance (ByVal)
                    signature(signature.Length - 1) = GetType(Object)   ' Last type is the return type
                    For i As Integer = 2 To signature.Length - 2        ' All arguments are ByRef
                        signature(i) = refObject
                    Next

                    Dim c As CallSite = CallSite.Create(Expression.GetDelegateType(signature), Action)
                    Dim args(Arguments.Length + 1) As Object
                    args(0) = c
                    args(1) = Instance
                    Arguments.CopyTo(args, 2)
                    Dim siteTarget As System.Delegate = DirectCast(c.GetType().GetField("Target").GetValue(c), System.Delegate)
                    Try
                        Dim result As Object = siteTarget.DynamicInvoke(args)
                        Array.Copy(args, 2, Arguments, 0, Arguments.Length)
                        Return result
                    Catch ie As TargetInvocationException
                        Throw ie.InnerException
                    End Try
            End Select
        End Function 'CreateRefCallSiteAndInvoke

        Shared Function CreateFuncCallSiteAndInvoke( _
                       ByVal Action As CallSiteBinder, _
                       ByVal Instance As Object, _
                       ByVal Arguments As Object()) As Object

            Action = GetCachedBinder(Action)

            Select Case Arguments.Length
                Case 0
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object)) = _
                        CallSite(Of Func(Of CallSite, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance)
                Case 1
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0))
                Case 2
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1))
                Case 3
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2))
                Case 4
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3))
                Case 5
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4))
                Case 6
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4), Arguments(5))
                Case 7
                    Dim c As CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object, Object, Object)) = _
                             CallSite(Of Func(Of CallSite, Object, Object, Object, Object, Object, Object, Object, Object, Object)).Create(Action)
                    Return c.Target.Invoke(c, Instance, Arguments(0), Arguments(1), Arguments(2), Arguments(3), Arguments(4), Arguments(5), Arguments(6))
                Case Else
                    Dim delegateArgTypes(Arguments.Length + 2) As Type
                    delegateArgTypes(0) = GetType(CallSite)
                    For i As Integer = 1 To delegateArgTypes.Length - 1
                        delegateArgTypes(i) = GetType(Object)
                    Next
                    Dim c As CallSite = CallSite.Create(Expression.GetDelegateType(delegateArgTypes), Action)
                    Dim args(Arguments.Length + 1) As Object
                    args(0) = c
                    args(1) = Instance
                    Arguments.CopyTo(args, 2)
                    Dim siteTarget As System.Delegate = _
                        CType(c.GetType().GetField("Target").GetValue(c), System.Delegate)
                    Try
                        Return siteTarget.DynamicInvoke(args)
                    Catch ie As TargetInvocationException
                        Throw ie.InnerException
                    End Try
            End Select
        End Function 'CreateFuncCallSiteAndInvoke

        ' The type of the Convert call site must match the type we are converting to
        Shared Function CreateConvertCallSiteAndInvoke( _
                ByVal Action As ConvertBinder, _
                ByVal Instance As Object) As Object

            ' Create the call site for performing the conversion
            Dim delegateArgTypes(2) As Type
            delegateArgTypes(0) = GetType(CallSite)
            delegateArgTypes(1) = GetType(Object)
            delegateArgTypes(2) = Action.Type
            Dim c As CallSite = CallSite.Create(Expression.GetFuncType(delegateArgTypes), GetCachedBinder(Action))

            ' Invoke it through reflection
            Dim args(1) As Object
            args(0) = c
            args(1) = Instance
            Dim siteTarget As System.Delegate = _
                CType(c.GetType().GetField("Target").GetValue(c), System.Delegate)
            Try
                Return siteTarget.DynamicInvoke(args)
            Catch ie As TargetInvocationException
                Throw ie.InnerException
            End Try
        End Function 'CreateConvertCallSiteAndInvoke


        ''' <summary>
        ''' Adds the exact type restriction on the target of the dynamic operation, and merges all of the
        ''' restrictions together
        ''' </summary>
        ''' <param name="target">The DynamicMetaObject representing the target of the operation</param>
        ''' <param name="args">The DynamicMetaObjects representing the arguments of the operation</param>
        ''' <param name="value">The DynamicMetaObject representing another other argument, usually the value of a set</param>
        ''' <returns>New set of restrictions which includes the exact type restriction on the target.</returns>
        ''' <remarks>
        ''' The dynamic binding produced by the binder is applicable to the exact type of the target object.
        ''' This method will add the binding restriction on the exact type of the target (all FallbackXXX
        ''' methods call this).
        '''</remarks>
        Friend Shared Function CreateRestrictions( _
            ByVal target As DynamicMetaObject, _
            Optional ByVal args As DynamicMetaObject() = Nothing, _
            Optional ByVal value As DynamicMetaObject = Nothing) As BindingRestrictions

            Dim r As BindingRestrictions = CreateRestriction(target)
            If args IsNot Nothing Then
                For Each arg As DynamicMetaObject In args
                    r = r.Merge(CreateRestriction(arg))
                Next
            End If
            If value IsNot Nothing Then
                r = r.Merge(CreateRestriction(value))
            End If
            Return r
        End Function

        Private Shared Function CreateRestriction(ByVal metaObject As DynamicMetaObject) As BindingRestrictions
            If metaObject.Value Is Nothing Then
                Return metaObject.Restrictions.Merge( _
                    BindingRestrictions.GetInstanceRestriction(metaObject.Expression, Nothing))
            Else
                Return metaObject.Restrictions.Merge( _
                    BindingRestrictions.GetTypeRestriction(metaObject.Expression, metaObject.LimitType))
            End If
        End Function

        Friend Shared Function NeedsDeferral( _
            ByVal target As DynamicMetaObject, _
            Optional ByVal args As DynamicMetaObject() = Nothing, _
            Optional ByVal value As DynamicMetaObject = Nothing) As Boolean

            If Not target.HasValue Then
                Return True
            End If
            If value IsNot Nothing AndAlso Not value.HasValue Then
                Return True
            End If
            If args IsNot Nothing Then
                For Each a As DynamicMetaObject In args
                    If Not a.HasValue Then
                        Return True
                    End If
                Next
            End If
            Return False
        End Function

    End Class 'IDOUtils


    ''' <summary>
    ''' Provides a set-like object used for caches which holds onto a maximum
    ''' number of elements specified at construction time.
    ''' 
    ''' This class is thread safe.
    ''' </summary>
    Friend NotInheritable Class CacheSet(Of T)
        Private ReadOnly _dict As New Dictionary(Of T, LinkedListNode(Of T))
        Private ReadOnly _list As New LinkedList(Of T)
        Private ReadOnly _maxSize As Integer

        ''' <summary>
        ''' Creates a dictionary-like object used for caches.
        ''' </summary>
        ''' <param name="maxSize">The maximum number of elements to store.</param>
        Friend Sub New(ByVal maxSize As Integer)
            _maxSize = maxSize
        End Sub

        ''' <summary>
        ''' Tries to get the entry associated with 'key'. If it already exists,
        ''' the existing value will be returned. Otherwise it will be added,
        ''' removing the oldest element in the cache if it has reached capacity.
        ''' </summary>
        Friend Function GetExistingOrAdd(ByVal key As T) As T
            SyncLock Me
                Dim node As LinkedListNode(Of T) = Nothing
                If _dict.TryGetValue(key, node) Then
                    ' Found a match, move it to the head of the list
                    If node.Previous IsNot Nothing Then
                        _list.Remove(node)
                        _list.AddFirst(node)
                    End If
                    Return node.Value
                ElseIf _dict.Count = _maxSize Then
                    ' We're at capacity, remove the last element to make room
                    _dict.Remove(_list.Last.Value)
                    _list.RemoveLast()
                End If

                ' Add a new entry to the head of the list
                node = New LinkedListNode(Of T)(key)
                _dict.Add(key, node)
                _list.AddFirst(node)
                Return key
            End SyncLock
        End Function
    End Class

End Namespace
