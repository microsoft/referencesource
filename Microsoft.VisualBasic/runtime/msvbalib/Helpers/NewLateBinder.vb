' Copyright (c) Microsoft Corporation.  All rights reserved.

Imports System
Imports System.Collections
Imports System.Collections.Generic
Imports System.Diagnostics
Imports System.Dynamic
Imports System.Globalization
Imports System.Reflection
Imports System.Runtime.InteropServices
Imports System.Security.Permissions

Imports Microsoft.VisualBasic.CompilerServices.Symbols
Imports Microsoft.VisualBasic.CompilerServices.OverloadResolution
Imports Microsoft.VisualBasic.CompilerServices.ExceptionUtils
Imports Microsoft.VisualBasic.CompilerServices.Utils
Imports System.Runtime.Versioning

#Const NEW_BINDER = True
#Const BINDING_LOG = False

'REVIEW VSW#395752: set up logging mechanism

'REVIEW VSW#395753:  attributes on all the members
'REVIEW VSW#395754:  thread safety for everything
<Assembly: TargetFramework("WindowsPhone,Version=v8.0", FrameworkDisplayName:="Windows Phone 8.0")> 

Namespace Microsoft.VisualBasic.CompilerServices

#If TELESTO Then
    'FIXME: <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> 
    Public NotInheritable Class NewLateBinding
#Else
    <System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Never)> _
    Public NotInheritable Class NewLateBinding
#End If
        ' Prevent creation.
        Private Sub New()
        End Sub

        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function LateCanEvaluate( _
                ByVal instance As Object, _
                ByVal type As System.Type, _
                ByVal memberName As String, _
                ByVal arguments As Object(), _
                ByVal allowFunctionEvaluation As Boolean, _
                ByVal allowPropertyEvaluation As Boolean) As Boolean

            Dim BaseReference As Container
            If type IsNot Nothing Then
                BaseReference = New Container(type)
            Else
                BaseReference = New Container(instance)
            End If

            Dim Members As MemberInfo() = BaseReference.GetMembers(memberName, False)

            If Members.Length = 0 Then
                Return True
            End If

            ' This is a field access
            If Members(0).MemberType = MemberTypes.Field Then
                If arguments.Length = 0 Then
                    Return True
                Else
                    Dim FieldValue As Object = BaseReference.GetFieldValue(DirectCast(Members(0), FieldInfo))
                    BaseReference = New Container(FieldValue)
                    If BaseReference.IsArray Then
                        Return True
                    End If
                    Return allowPropertyEvaluation
                End If
            End If

            ' This is a method invocation
            If Members(0).MemberType = MemberTypes.Method Then
                Return allowFunctionEvaluation
            End If

            ' This is a property access
            If Members(0).MemberType = MemberTypes.Property Then
                Return allowPropertyEvaluation
            End If

            Return True
        End Function


        'CONSIDER: Consider a pair of overloads - one that takes an instance, one that takes a type, since they are mutually exclusive.
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function LateCall( _
                ByVal Instance As Object, _
                ByVal Type As System.Type, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As System.Type(), _
                ByVal CopyBack As Boolean(), _
                ByVal IgnoreReturn As Boolean) As Object

#If Not NEW_BINDER Then
            Return LateBinding.InternalLateCall(Instance, Type, MemberName, Arguments, ArgumentNames, CopyBack, False)
#End If
            If Arguments Is Nothing Then Arguments = NoArguments
            If ArgumentNames Is Nothing Then ArgumentNames = NoArgumentNames
            If TypeArguments Is Nothing Then TypeArguments = NoTypeArguments

            Dim BaseReference As Container
            If Type IsNot Nothing Then
                BaseReference = New Container(Type)
            Else
                BaseReference = New Container(Instance)
            End If

            If BaseReference.IsCOMObject AndAlso Not BaseReference.IsWindowsRuntimeObject Then
#If Not TELESTO Then
                'UNDONE:  BAIL for now -- call the old binder.
                Return LateBinding.InternalLateCall(Instance, _
                    Type, MemberName, Arguments, ArgumentNames, CopyBack, IgnoreReturn)
#Else
                Throw New InvalidOperationException("Never expected to see a COM object in Telesto") ' Unexpected scenario. No need to loc this.
#End If 'Not TELESTO
            Else
                Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
                If idmop IsNot Nothing AndAlso TypeArguments Is NoTypeArguments Then
                    Return IDOBinder.IDOCall(idmop, MemberName, Arguments, ArgumentNames, CopyBack, IgnoreReturn)
                Else
                    Return ObjectLateCall(Instance, Type, MemberName, Arguments, _
                        ArgumentNames, TypeArguments, CopyBack, IgnoreReturn)
                End If
            End If
        End Function

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function FallbackCall( _
                ByVal Instance As Object, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal IgnoreReturn As Boolean) As Object

            Return ObjectLateCall(Instance, Nothing, MemberName, Arguments, _
                ArgumentNames, NoTypeArguments, IDOBinder.GetCopyBack(), IgnoreReturn)
        End Function 'FallbackCall

        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Private Shared Function ObjectLateCall( _
                ByVal Instance As Object, _
                ByVal Type As System.Type, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As System.Type(), _
                ByVal CopyBack As Boolean(), _
                ByVal IgnoreReturn As Boolean) As Object

            Dim BaseReference As Container
            If Type IsNot Nothing Then
                BaseReference = New Container(Type)
            Else
                BaseReference = New Container(Instance)
            End If

            Dim InvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty
            If IgnoreReturn Then InvocationFlags = InvocationFlags Or BindingFlags.IgnoreReturn

            Dim Failure As ResolutionFailure

            Return _
                CallMethod( _
                    BaseReference, _
                    MemberName, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    CopyBack, _
                    InvocationFlags, _
                    True, _
                    Failure)
        End Function 'ObjectLateCall

        'Quick check to determine if FallbackCall will succeed
        Friend Shared Function CanBindCall(ByVal Instance As Object, ByVal MemberName As String, ByVal Arguments As Object(), ByVal ArgumentNames As String(), ByVal IgnoreReturn As Boolean) As Boolean
            Dim BaseReference As New Container(Instance)
            Dim InvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty
            If IgnoreReturn Then InvocationFlags = InvocationFlags Or BindingFlags.IgnoreReturn

            Dim Failure As ResolutionFailure
            Dim Members As MemberInfo() = BaseReference.GetMembers(MemberName, False)
            If Members Is Nothing OrElse Members.Length = 0 Then
                Return False
            End If

            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    NoTypeArguments, _
                    InvocationFlags, _
                    False, _
                    Failure)

            Return Failure = ResolutionFailure.None
        End Function

        ' LateCallInvokeDefault is used to optionally invoke the default action on a call target.
        ' If the arguemnts are non-empty, then it isn't optional, and is treated
        ' as an error if there is no default action.
        ' Currently we can get here only in the process of execution of NewLateBinding.LateCall. 
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function LateCallInvokeDefault( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean) As Object

            Return InternalLateInvokeDefault(Instance, Arguments, ArgumentNames, ReportErrors, IDOBinder.GetCopyBack())
        End Function 'LateCallInvokeDefault

        ' LateGetInvokeDefault is used to optionally invoke the default action.
        ' If the arguemnts are non-empty, then it isn't optional, and is treated
        ' as an error if there is no default action.
        ' Currently we can get here only in the process of execution of NewLateBinding.LateGet. 
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function LateGetInvokeDefault( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean) As Object

            ' Dev10 #614719
            ' According to a comment in VBGetBinder.FallbackInvoke, this function is called when
            ' "The DLR was able to resolve o.member, but not o.member(args)"
            ' When NewLateBinding.LateGet is evaluating similar expression itself, it never tries to invoke default action 
            ' if arguments are not empty. It simply returns result of evaluating o.member. I believe, it makes sense
            ' to follow the same logic here. I.e., if there are no arguments, simply return the instance unless it is an IDO.

            If IDOUtils.TryCastToIDMOP(Instance) IsNot Nothing OrElse _
                (Arguments IsNot Nothing AndAlso Arguments.Length > 0) _
            Then
                Return InternalLateInvokeDefault(Instance, Arguments, ArgumentNames, ReportErrors, IDOBinder.GetCopyBack())
            Else
                Return Instance
            End If
        End Function 'LateGetInvokeDefault

        Private Shared Function InternalLateInvokeDefault( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean, _
                ByVal CopyBack As Boolean()) As Object

            Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
            If idmop IsNot Nothing Then
                Return IDOBinder.IDOInvokeDefault(idmop, Arguments, ArgumentNames, ReportErrors, CopyBack)
            Else
                Return ObjectLateInvokeDefault(Instance, Arguments, ArgumentNames, ReportErrors, CopyBack)
            End If
        End Function 'InternalLateInvokeDefault

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function FallbackInvokeDefault1( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean) As Object

            ' Try using the IDO index operation (in case it's an IDO array)
            Return IDOBinder.IDOFallbackInvokeDefault(DirectCast(Instance, IDynamicMetaObjectProvider), Arguments, ArgumentNames, ReportErrors, IDOBinder.GetCopyBack())
        End Function 'FallbackInvokeDefault

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function FallbackInvokeDefault2( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean) As Object

            Return ObjectLateInvokeDefault(Instance, Arguments, ArgumentNames, ReportErrors, IDOBinder.GetCopyBack())
        End Function 'FallbackInvokeDefault

        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Private Shared Function ObjectLateInvokeDefault( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean, _
                ByVal CopyBack As Boolean()) As Object

            Dim BaseReference As Container = New Container(Instance)
            Dim Failure As ResolutionFailure
            Dim Result As Object = InternalLateIndexGet( _
                Instance, Arguments, ArgumentNames, _
                ReportErrors OrElse Arguments.Length <> 0 OrElse BaseReference.IsArray, _
                Failure, CopyBack)
            Return If(Failure = ResolutionFailure.None, Result, Instance)
        End Function 'ObjectLateInvokeDefault

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Function LateIndexGet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String) As Object

            Return InternalLateInvokeDefault(Instance, Arguments, ArgumentNames, True, Nothing)
        End Function 'LateIndexGet

        Private Shared Function LateIndexGet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal CopyBack As Boolean()) As Object

            Return InternalLateInvokeDefault(Instance, Arguments, ArgumentNames, True, CopyBack)
        End Function 'LateIndexGet

        Private Shared Function InternalLateIndexGet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal ReportErrors As Boolean, _
                ByRef Failure As ResolutionFailure, _
                ByVal CopyBack As Boolean()) As Object

            Failure = ResolutionFailure.None

#If Not NEW_BINDER Then
            Return LateBinding.LateIndexGet(Instance, Arguments, ArgumentNames)
#End If
            If Arguments Is Nothing Then Arguments = NoArguments
            If ArgumentNames Is Nothing Then ArgumentNames = NoArgumentNames

            Dim BaseReference As Container = New Container(Instance)

            If BaseReference.IsCOMObject AndAlso Not BaseReference.IsWindowsRuntimeObject Then
#If Not TELESTO Then
                'UNDONE:  BAIL for now -- call the old binder.
                Return LateBinding.LateIndexGet(Instance, Arguments, ArgumentNames)
#Else
                Throw New InvalidOperationException("Never expected to see a COM object in Telesto") ' Unexpected scenario. No need to loc this.
#End If 'Not TELESTO
            End If

            'An r-value expression o(a) has two possible forms:
            '    1: o(a)    array lookup--where o is an array object and a is a set of indices
            '    2: o.d(a)  default member access--where o has default method/property d

            If BaseReference.IsArray Then
                'This is an array lookup o(a).

                If ArgumentNames.Length > 0 Then
                    Failure = ResolutionFailure.InvalidArgument

                    If ReportErrors Then
                        Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNamedArgs))
                    End If

                    Return Nothing
                End If
                '#579308 Initialize the copy back array to all ByVal
                ResetCopyback(CopyBack)
                Return BaseReference.GetArrayValue(Arguments)
            End If

            'This is a default member access o.d(a), which is a call to method "".

            Return _
                CallMethod( _
                    BaseReference, _
                    "", _
                    Arguments, _
                    ArgumentNames, _
                    NoTypeArguments, _
                    CopyBack, _
                    BindingFlags.InvokeMethod Or BindingFlags.GetProperty, _
                    ReportErrors, _
                    Failure)
        End Function 'InternalLateIndexGet

        Friend Shared Function CanBindInvokeDefault( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal ReportErrors As Boolean) As Boolean

            Dim BaseReference As Container = New Container(Instance)
            ReportErrors = ReportErrors OrElse Arguments.Length <> 0 OrElse BaseReference.IsArray

            If Not ReportErrors Then
                Return True
            End If

            'An r-value expression o(a) has two possible forms:
            '    1: o(a)    array lookup--where o is an array object and a is a set of indices
            '    2: o.d(a)  default member access--where o has default method/property d

            If BaseReference.IsArray Then
                'This is an array lookup o(a).
                Return ArgumentNames.Length = 0
            End If

            'This is a default member access o.d(a), which is a call to method "".
            Return CanBindCall(Instance, "", Arguments, ArgumentNames, False)
        End Function

        Friend Shared Sub ResetCopyback(ByVal CopyBack As Boolean())
            If CopyBack IsNot Nothing Then
                ' Initialize the copy back array to all ByVal.
                For Index As Integer = 0 To CopyBack.Length - 1
                    CopyBack(Index) = False
                Next
            End If
        End Sub 'ResetCopyback

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Function LateGet( _
                ByVal Instance As Object, _
                ByVal Type As System.Type, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal CopyBack As Boolean()) As Object

#If Not NEW_BINDER Then
            Return LateBinding.LateGet(Instance, Type, MemberName, Arguments, ArgumentNames, CopyBack)
#End If

            If Arguments Is Nothing Then Arguments = NoArguments
            If ArgumentNames Is Nothing Then ArgumentNames = NoArgumentNames
            If TypeArguments Is Nothing Then TypeArguments = NoTypeArguments

            Dim BaseReference As Container
            If Type IsNot Nothing Then
                BaseReference = New Container(Type)
            Else
                BaseReference = New Container(Instance)
            End If

            Dim InvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty

            If BaseReference.IsCOMObject AndAlso Not BaseReference.IsWindowsRuntimeObject Then
#If Not TELESTO Then
                'UNDONE:  BAIL for now -- call the old binder.
                Return LateBinding.LateGet(Instance, Type, MemberName, Arguments, ArgumentNames, CopyBack)
                'Return BaseReference.InvokeCOMMethod2(MemberName, Arguments, ArgumentNames, CopyBack, InvocationFlags)
#Else
                Throw New InvalidOperationException("Never expected to see a COM object in Telesto") ' Unexpected scenario. No need to loc this.
#End If 'Not TELESTO
            Else
                Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
                If idmop IsNot Nothing AndAlso TypeArguments Is NoTypeArguments Then
                    Return IDOBinder.IDOGet(idmop, MemberName, Arguments, ArgumentNames, CopyBack)
                Else
                    Return ObjectLateGet(Instance, Type, MemberName, Arguments, ArgumentNames, TypeArguments, CopyBack)
                End If
            End If
        End Function 'LateGet

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Function FallbackGet( _
                ByVal Instance As Object, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String()) As Object

            Return ObjectLateGet(Instance, Nothing, MemberName, Arguments, ArgumentNames, NoTypeArguments, IDOBinder.GetCopyBack())
        End Function 'FallbackGet

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Private Shared Function ObjectLateGet( _
                ByVal Instance As Object, _
                ByVal Type As System.Type, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal CopyBack As Boolean()) As Object

            'TODO(ngafter): should probably pass in InvocationFlags and BaseReference
            Dim BaseReference As Container
            If Type IsNot Nothing Then
                BaseReference = New Container(Type)
            Else
                BaseReference = New Container(Instance)
            End If

            Dim InvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty

            Dim Members As MemberInfo() = BaseReference.GetMembers(MemberName, True)

            If Members(0).MemberType = MemberTypes.Field Then
                If TypeArguments.Length > 0 Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
                End If

                Dim FieldValue As Object = BaseReference.GetFieldValue(DirectCast(Members(0), FieldInfo))
                If Arguments.Length = 0 Then
                    'This is a simple field access.
                    Return FieldValue
                Else
                    'This is an indexed field access.
                    Return LateIndexGet(FieldValue, Arguments, ArgumentNames, CopyBack)
                End If
            End If

            If ArgumentNames.Length > Arguments.Length OrElse _
               (CopyBack IsNot Nothing AndAlso CopyBack.Length <> Arguments.Length) Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
            End If

            Dim Failure As OverloadResolution.ResolutionFailure
            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    InvocationFlags, _
                    False, _
                    Failure)

            If Failure = OverloadResolution.ResolutionFailure.None Then
                Return BaseReference.InvokeMethod(TargetProcedure, Arguments, CopyBack, InvocationFlags)

            ElseIf Arguments.Length > 0 AndAlso Members.Length = 1 AndAlso IsZeroArgumentCall(Members(0)) Then
                ' Dev10 #579405: For default property transformation the group should contain just 1 item
                '                and that item should take no arguments.

                TargetProcedure = _
                    ResolveCall( _
                        BaseReference, _
                        MemberName, _
                        Members, _
                        NoArguments, _
                        NoArgumentNames, _
                        TypeArguments, _
                        InvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then
                    Dim Result As Object = BaseReference.InvokeMethod(TargetProcedure, NoArguments, Nothing, InvocationFlags)

                    'For backwards compatibility, throw a missing member exception if the intermediate result is Nothing.
                    If Result Is Nothing Then
                        Throw New  _
                            MissingMemberException( _
                                GetResourceString( _
                                    ResID.IntermediateLateBoundNothingResult1, _
                                    TargetProcedure.ToString, _
                                    BaseReference.VBFriendlyName))
                    End If

                    Result = InternalLateIndexGet( _
                                Result, _
                                Arguments, _
                                ArgumentNames, _
                                False, _
                                Failure, _
                                CopyBack)

                    If Failure = ResolutionFailure.None Then
                        Return Result
                    End If
                End If

            End If

            'Every attempt to make this work failed.  Redo the original call resolution to generate errors.
            ResolveCall( _
                BaseReference, _
                MemberName, _
                Members, _
                Arguments, _
                ArgumentNames, _
                TypeArguments, _
                InvocationFlags, _
                True, _
                Failure)
#If TELESTO Then
            Debug.Assert(False, "the resolution should have thrown an exception")
#Else
            Debug.Fail("the resolution should have thrown an exception")
#End If
            Throw New InternalErrorException()
        End Function 'ObjectLateGet

        'Quick check to determine if FallbackGet will succeed
        Friend Shared Function CanBindGet(ByVal Instance As Object, ByVal MemberName As String, ByVal Arguments As Object(), ByVal ArgumentNames As String()) As Boolean
            Dim BaseReference As New Container(Instance)
            Dim InvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty

            Dim Failure As ResolutionFailure
            Dim Members As MemberInfo() = BaseReference.GetMembers(MemberName, False)
            If Members Is Nothing OrElse Members.Length = 0 Then
                Return False
            End If

            If Members(0).MemberType = MemberTypes.Field Then
                'There may be additional work after the field get, but as far
                'as we're concerned the binding succeeded
                Return True
            End If

            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    NoTypeArguments, _
                    InvocationFlags, _
                    False, _
                    Failure)

            If Failure = OverloadResolution.ResolutionFailure.None Then
                Return True
            End If

            If Arguments.Length > 0 AndAlso Members.Length = 1 AndAlso IsZeroArgumentCall(Members(0)) Then
                ' Dev10 #579405: For default property transformation the group should contain just 1 item
                '                and that item should take no arguments.

                TargetProcedure = _
                    ResolveCall( _
                        BaseReference, _
                        MemberName, _
                        Members, _
                        NoArguments, _
                        NoArgumentNames, _
                        NoTypeArguments, _
                        InvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then
                    'There will be additional work after the first call, but as
                    'far as we're concerned the binding succeeded.
                    Return True
                End If
            End If

            'Every attempt at binding failed, return false so we use the IDO's error
            Return False
        End Function

        ' Determines if the member is a zero argument method or property
        Friend Shared Function IsZeroArgumentCall(ByVal Member As MemberInfo) As Boolean
            Return ((Member.MemberType = MemberTypes.Method AndAlso _
                        DirectCast(Member, MethodInfo).GetParameters().Length = 0) OrElse _
                    (Member.MemberType = MemberTypes.Property AndAlso _
                        DirectCast(Member, PropertyInfo).GetIndexParameters().Length = 0))
        End Function

        'UNDONE: temporary entry point to get the compiler hookup working.
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub LateIndexSetComplex( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
            If idmop IsNot Nothing Then
                Call IDOBinder.IDOIndexSetComplex(idmop, Arguments, ArgumentNames, OptimisticSet, RValueBase)
            Else
                Call ObjectLateIndexSetComplex(Instance, Arguments, ArgumentNames, OptimisticSet, RValueBase)
                Return
            End If
        End Sub

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub FallbackIndexSetComplex( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            ObjectLateIndexSetComplex(Instance, Arguments, ArgumentNames, OptimisticSet, RValueBase)
        End Sub 'FallbackIndexSetComplex

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Friend Shared Sub ObjectLateIndexSetComplex( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

#If Not NEW_BINDER Then
            LateBinding.LateIndexSetComplex(Instance, Arguments, ArgumentNames, OptimisticSet, RValueBase)
            Return
#End If
            'UNDONE: compiler is still loading Optimistic set, but this should be renamed to
            'Report Errors.  But changing that while turning on the new latebinder would require
            'a toolset update, which would be bad.  For now, make a temp.
            'Dim ReportErrors As Boolean = Not OptimisticSet

            If Arguments Is Nothing Then Arguments = NoArguments
            If ArgumentNames Is Nothing Then ArgumentNames = NoArgumentNames

            Dim BaseReference As Container = New Container(Instance)

            'An l-value expression o(a) has two possible forms:
            '    1: o(a) = v    array lookup--where o is an array object and a is a set of indices
            '    2: o.d(a) = v  default member access--where o has default method/property d

            If BaseReference.IsArray Then
                'This is an array lookup and assignment o(a) = v.

                If ArgumentNames.Length > 0 Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidNamedArgs))
                End If

                BaseReference.SetArrayValue(Arguments)
                Return
            End If

            If ArgumentNames.Length > Arguments.Length Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
            End If

            If Arguments.Length < 1 Then
                'We're binding to a Set, we must have at least the Value argument.
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
            End If

            Dim MethodName As String = ""

            If BaseReference.IsCOMObject AndAlso Not BaseReference.IsWindowsRuntimeObject Then
#If Not TELESTO Then
                'UNDONE:  BAIL for now -- call the old binder.
                LateBinding.LateIndexSetComplex(Instance, Arguments, ArgumentNames, OptimisticSet, RValueBase)
                Return
#Else
                Throw New InvalidOperationException("Never expected to see a COM object in Telesto") ' Unexpected scenario. No need to loc this.
#End If
#If 0 Then
                Try
                    BaseReference.InvokeCOMMethod( _
                        MethodName, _
                        Arguments, _
                        ArgumentNames, _
                        Nothing, _
                        GetPropertyPutFlags(Arguments(Arguments.Length - 1)))

                    If RValueBase AndAlso BaseReference.IsValueType Then
                        Throw New Exception( _
                                GetResourceString( _
                                    ResID.RValueBaseForValueType, _
                                    BaseReference.VBFriendlyName, _
                                    BaseReference.VBFriendlyName))
                    End If
                Catch ex As System.MissingMemberException When OptimisticSet = True
                    'A missing member exception means it has no Set member.  Silently handle the exception.
                End Try
                Return
#End If

            Else
                Dim InvocationFlags As BindingFlags = BindingFlags.SetProperty

                Dim Members As MemberInfo() = BaseReference.GetMembers(MethodName, True) 'MethodName is set during this call.

                Dim Failure As OverloadResolution.ResolutionFailure
                Dim TargetProcedure As Method = _
                    ResolveCall( _
                        BaseReference, _
                        MethodName, _
                        Members, _
                        Arguments, _
                        ArgumentNames, _
                        NoTypeArguments, _
                        InvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then

                    If RValueBase AndAlso BaseReference.IsValueType Then
                        Throw New Exception( _
                                GetResourceString( _
                                    ResID.RValueBaseForValueType, _
                                    BaseReference.VBFriendlyName, _
                                    BaseReference.VBFriendlyName))
                    End If

                    BaseReference.InvokeMethod(TargetProcedure, Arguments, Nothing, InvocationFlags)
                    Return

                ElseIf OptimisticSet Then
                    Return

                Else
                    'Redo the resolution to generate errors.
                    ResolveCall( _
                        BaseReference, _
                        MethodName, _
                        Members, _
                        Arguments, _
                        ArgumentNames, _
                        NoTypeArguments, _
                        InvocationFlags, _
                        True, _
                        Failure)
                End If
            End If


#If TELESTO Then
            Debug.Assert(False, "the resolution should have thrown an exception - should never reach here")
#Else
            Debug.Fail("the resolution should have thrown an exception - should never reach here")
#End If
            Throw New InternalErrorException()

        End Sub

        'Determines if ObjectLateIndexSetComplex can succeed
        'Used by IDOBinder
        Friend Shared Function CanIndexSetComplex( _
                ByVal Instance As Object, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean) As Boolean

            Dim BaseReference As Container = New Container(Instance)

            'An l-value expression o(a) has two possible forms:
            '    1: o(a) = v    array lookup--where o is an array object and a is a set of indices
            '    2: o.d(a) = v  default member access--where o has default method/property d

            If BaseReference.IsArray Then
                'This is an array lookup and assignment o(a) = v.
                Return ArgumentNames.Length = 0
            End If

            Dim MethodName As String = ""
            Dim InvocationFlags As BindingFlags = BindingFlags.SetProperty

            Dim Members As MemberInfo() = BaseReference.GetMembers(MethodName, False) 'MethodName is set during this call.
            If Members Is Nothing OrElse Members.Length = 0 Then
                Return False
            End If

            Dim Failure As OverloadResolution.ResolutionFailure
            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MethodName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    NoTypeArguments, _
                    InvocationFlags, _
                    False, _
                    Failure)

            If Failure = OverloadResolution.ResolutionFailure.None Then
                If RValueBase AndAlso BaseReference.IsValueType Then
                    Return False
                End If

                Return True
            End If

            Return OptimisticSet
        End Function

        'UNDONE: should remove this helper.
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub LateIndexSet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String)

            Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
            If idmop IsNot Nothing Then
                IDOBinder.IDOIndexSet(idmop, Arguments, ArgumentNames)
                Return
            Else
                ObjectLateIndexSet(Instance, Arguments, ArgumentNames)
                Return
            End If
        End Sub 'LateIndexSet

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute()> <DebuggerStepThroughAttribute()> _
        Public Shared Sub FallbackIndexSet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String)

            ObjectLateIndexSet(Instance, Arguments, ArgumentNames)
        End Sub 'FallbackIndexSet

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Private Shared Sub ObjectLateIndexSet( _
                ByVal Instance As Object, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String)

#If Not NEW_BINDER Then
            LateBinding.LateIndexSet(Instance, Arguments, ArgumentNames)
            Return
#End If

            ObjectLateIndexSetComplex(Instance, Arguments, ArgumentNames, False, False)
            Return
        End Sub 'ObjectLateIndexSet

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub LateSetComplex( _
                ByVal Instance As Object, _
                ByVal Type As Type, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal TypeArguments() As Type, _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
            If idmop IsNot Nothing AndAlso TypeArguments Is Nothing Then
                IDOBinder.IDOSetComplex(idmop, MemberName, Arguments, ArgumentNames, OptimisticSet, RValueBase)
            Else
                ObjectLateSetComplex(Instance, Type, _
                    MemberName, Arguments, ArgumentNames, TypeArguments, OptimisticSet, RValueBase)
                Return
            End If
        End Sub

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub FallbackSetComplex( _
                ByVal Instance As Object, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object, _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

            ObjectLateSetComplex( _
                Instance, Nothing, MemberName, Arguments, New String() {}, _
                NoTypeArguments, OptimisticSet, RValueBase)
        End Sub 'FallbackSetComplex

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Friend Shared Sub ObjectLateSetComplex( _
                ByVal Instance As Object, _
                ByVal Type As Type, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal TypeArguments() As Type, _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean)

#If Not NEW_BINDER Then
            LateBinding.LateSetComplex(Instance, Type, MemberName, Arguments, ArgumentNames, OptimisticSet, RValueBase)
            Return
#End If
            Const DefaultCallType As CallType = CType(0, CallType)
            LateSet(Instance, Type, MemberName, Arguments, ArgumentNames, TypeArguments, OptimisticSet, RValueBase, DefaultCallType)
        End Sub

        'UNDONE: temporary entry point to get the compiler hookup working.
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub LateSet( _
                ByVal Instance As Object, _
                ByVal Type As Type, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal TypeArguments As Type())

#If Not NEW_BINDER Then
            LateBinding.LateSet(Instance, Type, MemberName, Arguments, ArgumentNames)
            Return
#End If
            Dim idmop As IDynamicMetaObjectProvider = IDOUtils.TryCastToIDMOP(Instance)
            If idmop IsNot Nothing AndAlso TypeArguments Is Nothing Then
                IDOBinder.IDOSet(idmop, MemberName, ArgumentNames, Arguments)
            Else
                ObjectLateSet(Instance, Type, MemberName, Arguments, ArgumentNames, TypeArguments)
                Return
            End If
        End Sub

        'This method is only called from DynamicMethods generated at runtime
        <Obsolete("do not use this method", True)> _
        <System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)> _
        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub FallbackSet( _
                ByVal Instance As Object, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object)

            ObjectLateSet(Instance, Nothing, MemberName, Arguments, NoArgumentNames, NoTypeArguments)
        End Sub 'FallbackSet

        Friend Shared Sub ObjectLateSet( _
                ByVal Instance As Object, _
                ByVal Type As Type, _
                ByVal MemberName As String, _
                ByVal Arguments() As Object, _
                ByVal ArgumentNames() As String, _
                ByVal TypeArguments As Type())

#If Not NEW_BINDER Then
            LateBinding.LateSet(Instance, Type, MemberName, Arguments, ArgumentNames)
            Return
#End If

            Const DefaultCallType As CallType = CType(0, CallType)
            LateSet(Instance, Type, MemberName, Arguments, ArgumentNames, _
                TypeArguments, False, False, DefaultCallType)
            Return
        End Sub

        <DebuggerHiddenAttribute(), DebuggerStepThroughAttribute()> _
        Public Shared Sub LateSet( _
                ByVal Instance As Object, _
                ByVal Type As Type, _
                ByVal MemberName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal OptimisticSet As Boolean, _
                ByVal RValueBase As Boolean, _
                ByVal CallType As CallType)

            'UNDONE: compiler is still loading Optimistic set, but this should be renamed to
            'Report Errors.  But changing that while turning on the new latebinder would require
            'a toolset update, which would be bad.  For now, make a temp.
            'Dim ReportErrors As Boolean = Not OptimisticSet


            If Arguments Is Nothing Then Arguments = NoArguments
            If ArgumentNames Is Nothing Then ArgumentNames = NoArgumentNames
            If TypeArguments Is Nothing Then TypeArguments = NoTypeArguments

            Dim BaseReference As Container
            If Type IsNot Nothing Then
                BaseReference = New Container(Type)
            Else
                BaseReference = New Container(Instance)
            End If

            Dim InvocationFlags As BindingFlags

            If BaseReference.IsCOMObject AndAlso Not BaseReference.IsWindowsRuntimeObject Then
#If Not TELESTO Then
                'UNDONE:  BAIL for now -- call the old binder.
                Try
                    'CONSIDER  (5/9/2001):
                    '   this really needs to be done in two steps:
                    '         step 1:  can the Set succeed?
                    '         step 2:  perform the Set
                    '   the rvaluebase check would be done between 1 and 2

                    LateBinding.InternalLateSet(Instance, Type, MemberName, Arguments, ArgumentNames, OptimisticSet, CallType)

                    If RValueBase AndAlso Type.IsValueType Then
                        'note that objType is passed byref to InternalLateSet and that it 
                        'should be valid by the time we get to this point
                        Throw New Exception(GetResourceString(ResID.RValueBaseForValueType, VBFriendlyName(Type, Instance), VBFriendlyName(Type, Instance)))
                    End If
                Catch ex As System.MissingMemberException When OptimisticSet = True
                    'A missing member exception means it has no Set member.  Silently handle the exception.
                End Try

                Return
#Else
                Throw New InvalidOperationException
#End If 'Not TELESTO
#If 0 Then
                If CallType = CallType.Set Then
                    InvocationFlags = InvocationFlags Or BindingFlags.PutRefDispProperty
                    If Arguments(Arguments.GetUpperBound(0)) Is Nothing Then
                        Arguments(Arguments.GetUpperBound(0)) = New DispatchWrapper(Nothing)
                    End If
                ElseIf CallType = CallType.Let Then
                    InvocationFlags = InvocationFlags Or BindingFlags.PutDispProperty
                Else
                    InvocationFlags = InvocationFlags Or GetPropertyPutFlags(Arguments(Arguments.GetUpperBound(0)))
                End If

                'UNDONE
                BaseReference.InvokeCOMMethod2(MemberName, Arguments, ArgumentNames, Nothing, InvocationFlags)
                Return
#End If
            End If

            ' If we have a IDO that implements TryGetMember for a property but not TrySetMember then we could land up
            ' here and with an optimistic set but we don't want to throw an exception if the property is not found.
            ' Swallow the exception and return quietly if this is a readonly IDO property doing an optimistic set.
            Dim Members As MemberInfo() = BaseReference.GetMembers(MemberName, Not OptimisticSet)
 
            If Members.Length = 0 And OptimisticSet Then
                Return
            End If

            If Members(0).MemberType = MemberTypes.Field Then

                If TypeArguments.Length > 0 Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
                End If

                If Arguments.Length = 1 Then
                    If RValueBase AndAlso BaseReference.IsValueType Then
                        Throw New Exception( _
                                GetResourceString( _
                                    ResID.RValueBaseForValueType, _
                                    BaseReference.VBFriendlyName, _
                                    BaseReference.VBFriendlyName))
                    End If
                    'This is a simple field set.
                    BaseReference.SetFieldValue(DirectCast(Members(0), FieldInfo), Arguments(0))
                    Return
                Else
                    'This is an indexed field set.
                    Dim FieldValue As Object = BaseReference.GetFieldValue(DirectCast(Members(0), FieldInfo))
                    LateIndexSetComplex(FieldValue, Arguments, ArgumentNames, OptimisticSet, True)
                    Return
                End If
            End If

            InvocationFlags = BindingFlags.SetProperty

            If ArgumentNames.Length > Arguments.Length Then
                Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
            End If

            Dim Failure As OverloadResolution.ResolutionFailure
            Dim TargetProcedure As Method

            If TypeArguments.Length = 0 Then

                TargetProcedure = _
                    ResolveCall( _
                        BaseReference, _
                        MemberName, _
                        Members, _
                        Arguments, _
                        ArgumentNames, _
                        NoTypeArguments, _
                        InvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then
                    If RValueBase AndAlso BaseReference.IsValueType Then
                        Throw New Exception( _
                                GetResourceString( _
                                    ResID.RValueBaseForValueType, _
                                    BaseReference.VBFriendlyName, _
                                    BaseReference.VBFriendlyName))
                    End If

                    BaseReference.InvokeMethod(TargetProcedure, Arguments, Nothing, InvocationFlags)
                    Return
                End If

            End If

            Dim SecondaryInvocationFlags As BindingFlags = _
                    BindingFlags.InvokeMethod Or BindingFlags.GetProperty

            If Failure = OverloadResolution.ResolutionFailure.None OrElse Failure = OverloadResolution.ResolutionFailure.MissingMember Then

                TargetProcedure = _
                    ResolveCall( _
                        BaseReference, _
                        MemberName, _
                        Members, _
                        NoArguments, _
                        NoArgumentNames, _
                        TypeArguments, _
                        SecondaryInvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then
                    Dim Result As Object = _
                        BaseReference.InvokeMethod(TargetProcedure, NoArguments, Nothing, SecondaryInvocationFlags)

                    'For backwards compatibility, throw a missing member exception if the intermediate result is Nothing.
                    If Result Is Nothing Then
                        Throw New  _
                            MissingMemberException( _
                                GetResourceString( _
                                    ResID.IntermediateLateBoundNothingResult1, _
                                    TargetProcedure.ToString, _
                                    BaseReference.VBFriendlyName))
                    End If

                    LateIndexSetComplex(Result, Arguments, ArgumentNames, OptimisticSet, True)
                    Return
                End If
            End If

            If OptimisticSet Then
                Return
            End If

            'Everything failed, so give errors. Redo the first attempt to generate the errors.
            If TypeArguments.Length = 0 Then
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    InvocationFlags, _
                    True, _
                    Failure)

            Else
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    NoArguments, _
                    NoArgumentNames, _
                    TypeArguments, _
                    SecondaryInvocationFlags, _
                    True, _
                    Failure)
            End If

#If TELESTO Then
            Debug.Assert(False, "the resolution should have thrown an exception")
#Else
            Debug.Fail("the resolution should have thrown an exception")
#End If
            Throw New InternalErrorException()
            Return
        End Sub

        'Determines if LateSet will succeed. Used by IDOBinder.
        Friend Shared Function CanBindSet(ByVal Instance As Object, ByVal MemberName As String, ByVal Value As Object, ByVal OptimisticSet As Boolean, ByVal RValueBase As Boolean) As Boolean
            Dim BaseReference As New Container(Instance)
            Dim Arguments As Object() = {Value}

            Dim Members As MemberInfo() = BaseReference.GetMembers(MemberName, False)
            If Members Is Nothing OrElse Members.Length = 0 Then
                Return False
            End If

            If Members(0).MemberType = MemberTypes.Field Then
                If Arguments.Length = 1 AndAlso RValueBase AndAlso BaseReference.IsValueType Then
                    Return False
                End If

                'There may be more work (for indexed fields), but if we got
                'this far we consider it success.
                Return True
            End If

            Dim Failure As OverloadResolution.ResolutionFailure
            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MemberName, _
                    Members, _
                    Arguments, _
                    NoArgumentNames, _
                    NoTypeArguments, _
                    BindingFlags.SetProperty, _
                    False, _
                    Failure)

            If Failure = OverloadResolution.ResolutionFailure.None Then
                If RValueBase AndAlso BaseReference.IsValueType Then
                    Return False
                End If

                Return True
            End If

            Dim SecondaryInvocationFlags As BindingFlags = BindingFlags.InvokeMethod Or BindingFlags.GetProperty

            If Failure = OverloadResolution.ResolutionFailure.MissingMember Then
                TargetProcedure = _
                    ResolveCall( _
                        BaseReference, _
                        MemberName, _
                        Members, _
                        NoArguments, _
                        NoArgumentNames, _
                        NoTypeArguments, _
                        SecondaryInvocationFlags, _
                        False, _
                        Failure)

                If Failure = OverloadResolution.ResolutionFailure.None Then
                    'There is work (to call the method/prop), but if we got
                    'this far we consider it success.
                    Return True
                End If
            End If

            'Everything failed, so use the IDO's error if any
            'Unless we're doing an optimistic set, in which case this is considered success.
            Return OptimisticSet
        End Function

        Private Shared Function CallMethod( _
                ByVal BaseReference As Container, _
                ByVal MethodName As String, _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As System.Type(), _
                ByVal CopyBack As Boolean(), _
                ByVal InvocationFlags As BindingFlags, _
                ByVal ReportErrors As Boolean, _
                ByRef Failure As ResolutionFailure) As Object

            Debug.Assert(BaseReference IsNot Nothing, "Nothing unexpected")
            Debug.Assert(Arguments IsNot Nothing, "Nothing unexpected")
            Debug.Assert(ArgumentNames IsNot Nothing, "Nothing unexpected")
            Debug.Assert(TypeArguments IsNot Nothing, "Nothing unexpected")

            Failure = ResolutionFailure.None

            If ArgumentNames.Length > Arguments.Length OrElse _
               (CopyBack IsNot Nothing AndAlso CopyBack.Length <> Arguments.Length) Then
                Failure = ResolutionFailure.InvalidArgument

                If ReportErrors Then
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
                End If

                Return Nothing
            End If

            If HasFlag(InvocationFlags, BindingFlags.SetProperty) AndAlso Arguments.Length < 1 Then
                Failure = ResolutionFailure.InvalidArgument

                If ReportErrors Then
                    'If we're binding to a Set, we must have at least the Value argument.
                    Throw New ArgumentException(GetResourceString(ResID.Argument_InvalidValue)) 'CONSIDER: a better error message
                End If

                Return Nothing
            End If

#If 0 Then
            If BaseReference.IsCOMObject Then
                Return _
                    BaseReference.InvokeCOMMethod( _
                        MethodName, _
                        Arguments, _
                        ArgumentNames, _
                        CopyBack, _
                        InvocationFlags)
            End If
#End If

            Dim Members As MemberInfo() = BaseReference.GetMembers(MethodName, ReportErrors)

            If Members Is Nothing OrElse Members.Length = 0 Then
                Failure = ResolutionFailure.MissingMember

                If ReportErrors Then
#If TELESTO Then
                    Debug.Assert(False, "If ReportErrors is True, GetMembers should have thrown above")
#Else
                    Debug.Fail("If ReportErrors is True, GetMembers should have thrown above")
#End If
                    Members = BaseReference.GetMembers(MethodName, True)
                End If

                Return Nothing
            End If

            Dim TargetProcedure As Method = _
                ResolveCall( _
                    BaseReference, _
                    MethodName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    InvocationFlags, _
                    ReportErrors, _
                    Failure)

            If Failure = ResolutionFailure.None Then
                Return BaseReference.InvokeMethod(TargetProcedure, Arguments, CopyBack, InvocationFlags)
            End If

            Return Nothing
        End Function

        Friend Shared Function MatchesPropertyRequirements(ByVal TargetProcedure As Method, ByVal Flags As BindingFlags) As MethodInfo
            Debug.Assert(TargetProcedure.IsProperty, "advertised property method isn't.")

            If HasFlag(Flags, BindingFlags.SetProperty) Then
                Return TargetProcedure.AsProperty.GetSetMethod
            Else
                Return TargetProcedure.AsProperty.GetGetMethod
            End If
        End Function

        Friend Shared Function ReportPropertyMismatch(ByVal TargetProcedure As Method, ByVal Flags As BindingFlags) As Exception
            Debug.Assert(TargetProcedure.IsProperty, "advertised property method isn't.")

            If HasFlag(Flags, BindingFlags.SetProperty) Then
                Debug.Assert(TargetProcedure.AsProperty.GetSetMethod Is Nothing, "expected error condition")
                'UNDONE: what's the right type of exception to throw for invalid targets?  It shouldn't be MissingMemberException.
                Return New MissingMemberException( _
                    GetResourceString(ResID.NoSetProperty1, TargetProcedure.AsProperty.Name))
            Else
                Debug.Assert(TargetProcedure.AsProperty.GetGetMethod Is Nothing, "expected error condition")
                'UNDONE: what's the right type of exception to throw for invalid targets?  It shouldn't be MissingMemberException.
                Return New MissingMemberException( _
                    GetResourceString(ResID.NoGetProperty1, TargetProcedure.AsProperty.Name))
            End If
        End Function

        Friend Shared Function ResolveCall( _
                ByVal BaseReference As Container, _
                ByVal MethodName As String, _
                ByVal Members As MemberInfo(), _
                ByVal Arguments As Object(), _
                ByVal ArgumentNames As String(), _
                ByVal TypeArguments As Type(), _
                ByVal LookupFlags As BindingFlags, _
                ByVal ReportErrors As Boolean, _
                ByRef Failure As OverloadResolution.ResolutionFailure) As Method

            Debug.Assert(BaseReference IsNot Nothing, "expected a base reference")
            Debug.Assert(MethodName IsNot Nothing, "expected method name")
            Debug.Assert(Members IsNot Nothing AndAlso Members.Length > 0, "expected members")
            Debug.Assert(Arguments IsNot Nothing AndAlso _
                         ArgumentNames IsNot Nothing AndAlso _
                         TypeArguments IsNot Nothing AndAlso _
                         ArgumentNames.Length <= Arguments.Length, _
                         "expected valid argument arrays")

            Failure = OverloadResolution.ResolutionFailure.None

            If Members(0).MemberType <> MemberTypes.Method AndAlso _
               Members(0).MemberType <> MemberTypes.Property Then

                Failure = OverloadResolution.ResolutionFailure.InvalidTarget
                If ReportErrors Then
                    'This expression is not a procedure, but occurs as the target of a procedure call.
                    Throw New ArgumentException( _
                        GetResourceString(ResID.ExpressionNotProcedure, MethodName, BaseReference.VBFriendlyName))
                End If
                Return Nothing
            End If

            'When binding to Property Set accessors, strip off the last Value argument
            'because it does not participate in overload resolution.

            Dim SavedArguments As Object()
            Dim ArgumentCount As Integer = Arguments.Length
            Dim LastArgument As Object = Nothing

            If HasFlag(LookupFlags, BindingFlags.SetProperty) Then
                If Arguments.Length = 0 Then
                    Failure = OverloadResolution.ResolutionFailure.InvalidArgument

                    If ReportErrors Then
                        Throw New InvalidCastException( _
                            GetResourceString(ResID.PropertySetMissingArgument1, MethodName))
                    End If

                    Return Nothing
                End If

                SavedArguments = Arguments
                Arguments = New Object(ArgumentCount - 2) {}
                System.Array.Copy(SavedArguments, Arguments, Arguments.Length)
                LastArgument = SavedArguments(ArgumentCount - 1)
            End If

            Dim ResolutionResult As Method = _
                ResolveOverloadedCall( _
                    MethodName, _
                    Members, _
                    Arguments, _
                    ArgumentNames, _
                    TypeArguments, _
                    LookupFlags, _
                    ReportErrors, _
                    Failure,
                    BaseReference)

            Debug.Assert(Failure = OverloadResolution.ResolutionFailure.None OrElse Not ReportErrors, _
                         "if resolution failed, an exception should have been thrown")

            If Failure <> OverloadResolution.ResolutionFailure.None Then
                Debug.Assert(ResolutionResult Is Nothing, "resolution failed so should have no result")
                Return Nothing
            End If

            Debug.Assert(ResolutionResult IsNot Nothing, "resolution didn't fail, so should have result")

#If BINDING_LOG Then
            Console.WriteLine("== RESULT ==")
            Console.WriteLine(ResolutionResult.DeclaringType.Name & "::" & ResolutionResult.ToString)
            Console.WriteLine()
#End If

            'Overload resolution will potentially select one method before validating arguments.
            'Validate those arguments now.
            'CONSIDER: move the overload list construction up and out of overload resolution and into this function.
            If Not ResolutionResult.ArgumentsValidated Then

                If Not CanMatchArguments(ResolutionResult, Arguments, ArgumentNames, TypeArguments, False, Nothing) Then

                    Failure = OverloadResolution.ResolutionFailure.InvalidArgument

                    If ReportErrors Then
                        Dim ErrorMessage As String = ""
                        Dim Errors As New List(Of String)

                        Dim Result As Boolean = _
                            CanMatchArguments(ResolutionResult, Arguments, ArgumentNames, TypeArguments, False, Errors)

                        Debug.Assert(Result = False AndAlso Errors.Count > 0, "expected this candidate to fail")

                        For Each ErrorString As String In Errors
                            ErrorMessage &= vbCrLf & "    " & ErrorString
                        Next

                        ErrorMessage = GetResourceString(ResID.MatchArgumentFailure2, ResolutionResult.ToString, ErrorMessage)
                        'We are missing a member which can match the arguments, so throw a missing member exception.
                        'CONSIDER  2/26/2004: InvalidCastException is thrown only for back compat.  It would
                        'be nice if the latebinder had its own set of exceptions to throw.
                        Throw New InvalidCastException(ErrorMessage)
                    End If

                    Return Nothing
                End If

            End If

            'Once we've gotten this far, we've selected a member. From this point on, we determine
            'if the member can be called given the context.

            'Check that the resulting binding makes sense in the current context.
            If ResolutionResult.IsProperty Then
                If MatchesPropertyRequirements(ResolutionResult, LookupFlags) Is Nothing Then
                    Failure = OverloadResolution.ResolutionFailure.InvalidTarget
                    If ReportErrors Then
                        Throw ReportPropertyMismatch(ResolutionResult, LookupFlags)
                    End If
                    Return Nothing
                End If
            Else
                Debug.Assert(ResolutionResult.IsMethod, "must be a method")
                If HasFlag(LookupFlags, BindingFlags.SetProperty) Then
                    Failure = OverloadResolution.ResolutionFailure.InvalidTarget
                    If ReportErrors Then
                        'Methods can't be targets of assignments.
                        'UNDONE: what's the right type of exception to throw for invalid targets?  It shouldn't be MissingMemberException.
                        Throw New MissingMemberException( _
                            GetResourceString(ResID.MethodAssignment1, ResolutionResult.AsMethod.Name))
                    End If
                    Return Nothing
                End If
            End If

            If HasFlag(LookupFlags, BindingFlags.SetProperty) Then
                'Need to match the Value argument for the property set call.
                Debug.Assert(GetCallTarget(ResolutionResult, LookupFlags).Name.StartsWith("set_"), "expected set accessor")

                Dim Parameters As ParameterInfo() = GetCallTarget(ResolutionResult, LookupFlags).GetParameters
                Dim LastParameter As ParameterInfo = Parameters(Parameters.Length - 1)
                If Not CanPassToParameter( _
                            ResolutionResult, _
                            LastArgument, _
                            LastParameter, _
                            False, _
                            False, _
                            Nothing, _
                            Nothing, _
                            Nothing) Then

                    Failure = OverloadResolution.ResolutionFailure.InvalidArgument

                    If ReportErrors Then
                        Dim ErrorMessage As String = ""
                        Dim Errors As New List(Of String)

                        Dim Result As Boolean = _
                            CanPassToParameter( _
                                ResolutionResult, _
                                LastArgument, _
                                LastParameter, _
                                False, _
                                False, _
                                Errors, _
                                Nothing, _
                                Nothing)

                        Debug.Assert(Result = False AndAlso Errors.Count > 0, "expected this candidate to fail")

                        For Each ErrorString As String In Errors
                            ErrorMessage &= vbCrLf & "    " & ErrorString
                        Next

                        ErrorMessage = GetResourceString(ResID.MatchArgumentFailure2, ResolutionResult.ToString, ErrorMessage)
                        'The selected member can't handle the type of the Value argument, so this is an argument exception.
                        'CONSIDER  2/26/2004: InvalidCastException is thrown only for back compat.  It would
                        'be nice if the latebinder had its own set of exceptions to throw.
                        Throw New InvalidCastException(ErrorMessage)
                    End If

                    Return Nothing
                End If
            End If

            Return ResolutionResult
        End Function

        Friend Shared Function GetCallTarget(ByVal TargetProcedure As Method, ByVal Flags As BindingFlags) As MethodBase
            If TargetProcedure.IsMethod Then Return TargetProcedure.AsMethod
            If TargetProcedure.IsProperty Then Return MatchesPropertyRequirements(TargetProcedure, Flags)
#If TELESTO Then
            Debug.Assert(False, "not a method or property??")
#Else
            Debug.Fail("not a method or property??")
#End If
            Return Nothing
        End Function

        Friend Shared Function ConstructCallArguments( _
            ByVal TargetProcedure As Method, _
            ByVal Arguments As Object(), _
            ByVal LookupFlags As BindingFlags) As Object()

            Debug.Assert(TargetProcedure IsNot Nothing AndAlso Arguments IsNot Nothing, "expected arguments")


            Dim Parameters As ParameterInfo() = GetCallTarget(TargetProcedure, LookupFlags).GetParameters
            Dim CallArguments As Object() = New Object(Parameters.Length - 1) {}

            Dim SavedArguments As Object()
            Dim ArgumentCount As Integer = Arguments.Length
            Dim LastArgument As Object = Nothing

            If HasFlag(LookupFlags, BindingFlags.SetProperty) Then
                Debug.Assert(Arguments.Length > 0, "must have an argument for property set Value")
                SavedArguments = Arguments
                Arguments = New Object(ArgumentCount - 2) {}
                System.Array.Copy(SavedArguments, Arguments, Arguments.Length)
                LastArgument = SavedArguments(ArgumentCount - 1)
            End If

            MatchArguments(TargetProcedure, Arguments, CallArguments)

            If HasFlag(LookupFlags, BindingFlags.SetProperty) Then
                'Need to match the Value argument for the property set call.
                Debug.Assert(GetCallTarget(TargetProcedure, LookupFlags).Name.StartsWith("set_"), "expected set accessor")

                Dim LastParameter As ParameterInfo = Parameters(Parameters.Length - 1)
                CallArguments(Parameters.Length - 1) = _
                    PassToParameter(LastArgument, LastParameter, LastParameter.ParameterType)
            End If

            Return CallArguments
        End Function

    End Class
End Namespace
