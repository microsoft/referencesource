//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Definitions of global string constants that get added into the
//  string pool when the compiler is created.
//
//-------------------------------------------------------------------------------------------------

#if !defined(DEFINE_STRING_CONSTANT)

// Macro used to access a string constant.
#define STRING_CONST(pCompiler, name)    pCompiler->GetStringConstant(STRING_CONST_##name)
#define STRING_CONST2(pStringPool, name) pStringPool->GetStringConstant(STRING_CONST_##name)
#define CONCAT_STRING_CONST(pCompiler, name1, name2) pCompiler->ConcatStrings(pCompiler->GetStringConstant(STRING_CONST_##name1), L".", pCompiler->GetStringConstant(STRING_CONST_##name2)) 

#if IDE 

// Macro used to access a string constant.
#define IDE_STRING_CONST(name)    GetCompilerPackage()->GetStringConstant(STRING_CONST_##name)
#define STRING_CONST2(pStringPool, name) pStringPool->GetStringConstant(STRING_CONST_##name)

#endif IDE

// Create the enum of all of the available string constants.
#define DEFINE_STRING_CONSTANT(name, string) STRING_CONST_##name,

enum STRING_CONSTANTS
{
#include "StringConstants.h"
    STRING_CONST_MAX
};

#undef DEFINE_STRING_CONSTANT

#else

// Put the definitions of your string constants here.

//                     Constant name            Constant string
//                     -----------------------  ------------------------
DEFINE_STRING_CONSTANT(EmptyString               , "")

// Visual Basic Runtime strings

DEFINE_STRING_CONSTANT(Microsoft                 , "Microsoft")
DEFINE_STRING_CONSTANT(VisualBasic               , "VisualBasic")
DEFINE_STRING_CONSTANT(CompilerServices          , "CompilerServices")
DEFINE_STRING_CONSTANT(MicrosoftVisualBasic      , "Microsoft.VisualBasic")
DEFINE_STRING_CONSTANT(VBCompilerServices        , "Microsoft.VisualBasic.CompilerServices")
DEFINE_STRING_CONSTANT(Collection                , "Collection")
DEFINE_STRING_CONSTANT(_Collection               , "_Collection")
DEFINE_STRING_CONSTANT(chr                       , "chr")
DEFINE_STRING_CONSTANT(chrw                      , "chrw")
DEFINE_STRING_CONSTANT(asc                       , "asc")
DEFINE_STRING_CONSTANT(ascw                      , "ascw")
DEFINE_STRING_CONSTANT(StringType                , "StringType")
DEFINE_STRING_CONSTANT(AssignMid                 , "MidStmtStr")
DEFINE_STRING_CONSTANT(Conversions               , "Conversions")
DEFINE_STRING_CONSTANT(LateBinderConversions     , "LateBinderConversions")
DEFINE_STRING_CONSTANT(ChangeType                , "ChangeType")
DEFINE_STRING_CONSTANT(ToGenericParameter        , "ToGenericParameter")
DEFINE_STRING_CONSTANT(BackingFieldInitFlag      , "$Init")
DEFINE_STRING_CONSTANT(BackingFieldInitFlagClass , "StaticLocalInitFlag")
DEFINE_STRING_CONSTANT(State                     , "State")
DEFINE_STRING_CONSTANT(IncompleteInitialization  , "IncompleteInitialization")
DEFINE_STRING_CONSTANT(IIf                       , "IIf")
DEFINE_STRING_CONSTANT(Interaction               , "Interaction")

// CLR strings

DEFINE_STRING_CONSTANT(ComDomain                 , "System")
DEFINE_STRING_CONSTANT(ComObject                 , "Object")
DEFINE_STRING_CONSTANT(ComValueType              , "ValueType")
DEFINE_STRING_CONSTANT(ComEnum                   , "Enum")
DEFINE_STRING_CONSTANT(ComString                 , "String")
DEFINE_STRING_CONSTANT(EnumValueMember           , "value__")
DEFINE_STRING_CONSTANT(ComClassType              , "Type")

DEFINE_STRING_CONSTANT(ComFieldInfoType          , "FieldInfo")
DEFINE_STRING_CONSTANT(ComMethodInfoType         , "MethodInfo")
DEFINE_STRING_CONSTANT(ComMethodBaseType         , "MethodBase")
DEFINE_STRING_CONSTANT(ComConstructorInfoType    , "ConstructorInfo")

DEFINE_STRING_CONSTANT(ComException              , "Exception")
DEFINE_STRING_CONSTANT(Void                      , "Void")

DEFINE_STRING_CONSTANT(SystemObject              , "System.Object")
DEFINE_STRING_CONSTANT(SystemArray               , "System.Array")
DEFINE_STRING_CONSTANT(SystemBoolean             , "System.Boolean")
DEFINE_STRING_CONSTANT(SystemSByte               , "System.SByte")
DEFINE_STRING_CONSTANT(SystemByte                , "System.Byte")
DEFINE_STRING_CONSTANT(SystemInt16               , "System.Int16")
DEFINE_STRING_CONSTANT(SystemUInt16              , "System.UInt16")
DEFINE_STRING_CONSTANT(SystemInt32               , "System.Int32")
DEFINE_STRING_CONSTANT(SystemUInt32              , "System.UInt32")
DEFINE_STRING_CONSTANT(SystemInt64               , "System.Int64")
DEFINE_STRING_CONSTANT(SystemUInt64              , "System.UInt64")
DEFINE_STRING_CONSTANT(SystemSingle              , "System.Single")
DEFINE_STRING_CONSTANT(SystemDouble              , "System.Double")
DEFINE_STRING_CONSTANT(SystemDateTime            , "System.DateTime")
DEFINE_STRING_CONSTANT(SystemDecimal             , "System.Decimal")
DEFINE_STRING_CONSTANT(SystemChar                , "System.Char")
DEFINE_STRING_CONSTANT(SystemDrawing             , "System.Drawing")
DEFINE_STRING_CONSTANT(ComDiagnosticsDomain      , "System.Diagnostics")
DEFINE_STRING_CONSTANT(ComInteropServicesDomain  , "System.Runtime.InteropServices")
DEFINE_STRING_CONSTANT(ComRuntimeCompilerServicesDomain, "System.Runtime.CompilerServices")
DEFINE_STRING_CONSTANT(ComSecurityPermissionsDomain, "System.Security.Permissions")
DEFINE_STRING_CONSTANT(ComSecurityDomain         , "System.Security")
DEFINE_STRING_CONSTANT(WinRTInteropServicesDomain, "System.Runtime.InteropServices.WindowsRuntime")
DEFINE_STRING_CONSTANT(SystemWindowsForms        , "System.Windows.Forms")
DEFINE_STRING_CONSTANT(SystemComObject           , "System.__ComObject")
DEFINE_STRING_CONSTANT(SystemVoid                , "System.Void")
DEFINE_STRING_CONSTANT(SystemString              , "System.String")
DEFINE_STRING_CONSTANT(SystemCore                , "System.Core")

DEFINE_STRING_CONSTANT(ComDBNull                 , "DBNull")
DEFINE_STRING_CONSTANT(ComIntPtr                 , "IntPtr")
DEFINE_STRING_CONSTANT(ComUIntPtr                , "UIntPtr")
DEFINE_STRING_CONSTANT(ComSecurityAction         , "SecurityAction")
DEFINE_STRING_CONSTANT(ComEventArgs              , "EventArgs")
DEFINE_STRING_CONSTANT(DispatchWrapper           , "DispatchWrapper")
DEFINE_STRING_CONSTANT(UnknownWrapper            , "UnknownWrapper")
DEFINE_STRING_CONSTANT(ComMarshalByRefObject     , "MarshalByRefObject")
DEFINE_STRING_CONSTANT(ComTypedReference         , "TypedReference")
DEFINE_STRING_CONSTANT(ComWeakReference          , "WeakReference")
DEFINE_STRING_CONSTANT(ComArgIterator            , "ArgIterator")
DEFINE_STRING_CONSTANT(ComRuntimeArgumentHandle  , "RuntimeArgumentHandle")
DEFINE_STRING_CONSTANT(ComGenericNullable        , "Nullable`1")
DEFINE_STRING_CONSTANT(ComGenericIEquatable      , "IEquatable`1")
DEFINE_STRING_CONSTANT(ComGenericICollection     , "ICollection`1")
DEFINE_STRING_CONSTANT(ComGenericCollection      , "Collection`1")
DEFINE_STRING_CONSTANT(ComGenericReadOnlyCollection,"ReadOnlyCollection`1")
DEFINE_STRING_CONSTANT(ComGenericIList           , "IList`1")
DEFINE_STRING_CONSTANT(ComGenericIReadOnlyList   , "IReadOnlyList`1")
DEFINE_STRING_CONSTANT(ComGenericIReadOnlyCollection , "IReadOnlyCollection`1")
DEFINE_STRING_CONSTANT(ComGenericList            , "List`1")
DEFINE_STRING_CONSTANT(ComGenericIDictionary     , "IDictionary`2")
DEFINE_STRING_CONSTANT(ComGenericKeyValuePair    , "KeyValuePair`2")

DEFINE_STRING_CONSTANT(DefaultCharSetAttribute   , "DefaultCharSetAttribute")
DEFINE_STRING_CONSTANT(MarshalAsAttribute        , "MarshalAsAttribute")
DEFINE_STRING_CONSTANT(Marshal                   , "Marshal")

DEFINE_STRING_CONSTANT(WindowsRuntimeMarshal     , "WindowsRuntimeMarshal")
DEFINE_STRING_CONSTANT(AddEventHandler           , "AddEventHandler")
DEFINE_STRING_CONSTANT(RemoveEventHandler        , "RemoveEventHandler")
DEFINE_STRING_CONSTANT(EventRegistrationToken    , "EventRegistrationToken")
DEFINE_STRING_CONSTANT(EventRegistrationTokenTable, "EventRegistrationTokenTable`1")
DEFINE_STRING_CONSTANT(InvocationList            ,"InvocationList")

DEFINE_STRING_CONSTANT(ComSequence               , "Sequence")
DEFINE_STRING_CONSTANT(WhereMethod               , "Where")
DEFINE_STRING_CONSTANT(SelectMethod              , "Select")
DEFINE_STRING_CONSTANT(DistinctMethod            , "Distinct")
DEFINE_STRING_CONSTANT(OrderByMethod             , "OrderBy")
DEFINE_STRING_CONSTANT(OrderByDescendingMethod   , "OrderByDescending")
DEFINE_STRING_CONSTANT(ThenByMethod              , "ThenBy")
DEFINE_STRING_CONSTANT(ThenByDescendingMethod    , "ThenByDescending")
DEFINE_STRING_CONSTANT(SelectManyMethod          , "SelectMany")
DEFINE_STRING_CONSTANT(ElementAtMethod           , "ElementAtOrDefault")
DEFINE_STRING_CONSTANT(GroupByMethod             , "GroupBy")
DEFINE_STRING_CONSTANT(CastMethod                , "Cast")
DEFINE_STRING_CONSTANT(It                        , "$VB$It")
DEFINE_STRING_CONSTANT(It1                       , "$VB$It1")
DEFINE_STRING_CONSTANT(It2                       , "$VB$It2")
DEFINE_STRING_CONSTANT(FieldIt1                  , "VBIt1")
DEFINE_STRING_CONSTANT(FieldIt2                  , "VBIt2")
DEFINE_STRING_CONSTANT(Key                       , "$VB$Key")
DEFINE_STRING_CONSTANT(Group                     , "$VB$Group")
DEFINE_STRING_CONSTANT(Group2                    , "$VB$Group2")
DEFINE_STRING_CONSTANT(FieldGroup2               , "VBGroup2")
DEFINE_STRING_CONSTANT(AsQueryableMethod         , "AsQueryable")
DEFINE_STRING_CONSTANT(AsEnumerableMethod        , "AsEnumerable")
DEFINE_STRING_CONSTANT(JoinMethod                , "Join")
DEFINE_STRING_CONSTANT(GroupJoinMethod           , "GroupJoin")
DEFINE_STRING_CONSTANT(TakeMethod                , "Take")
DEFINE_STRING_CONSTANT(TakeWhileMethod           , "TakeWhile")
DEFINE_STRING_CONSTANT(SkipMethod                , "Skip")
DEFINE_STRING_CONSTANT(SkipWhileMethod           , "SkipWhile")
DEFINE_STRING_CONSTANT(ToArrayMethod             , "ToArray")
DEFINE_STRING_CONSTANT(ToListMethod              , "ToList")

// System.Array

DEFINE_STRING_CONSTANT(ComArray                  , "Array")
DEFINE_STRING_CONSTANT(Length                    , "Length")
DEFINE_STRING_CONSTANT(LongLength                , "LongLength")

// System.String

DEFINE_STRING_CONSTANT(Concat                    , "Concat")
DEFINE_STRING_CONSTANT(Chars                     , "Chars")

// Delegates

DEFINE_STRING_CONSTANT(ComDelegate               , "Delegate")
DEFINE_STRING_CONSTANT(ComMDelegate              , "MulticastDelegate")
DEFINE_STRING_CONSTANT(DelegateInvoke            , "Invoke")
DEFINE_STRING_CONSTANT(DelegateTargetObject      , "TargetObject")
DEFINE_STRING_CONSTANT(DelegateTargetMethod      , "TargetMethod")
DEFINE_STRING_CONSTANT(DelegateCallback          , "DelegateCallback")
DEFINE_STRING_CONSTANT(DelegateAsyncState        , "DelegateAsyncState")
DEFINE_STRING_CONSTANT(DelegateAsyncResult       , "DelegateAsyncResult")
DEFINE_STRING_CONSTANT(DelegateBeginInvoke       , "BeginInvoke")
DEFINE_STRING_CONSTANT(DelegateEndInvoke         , "EndInvoke")
DEFINE_STRING_CONSTANT(ComAsyncCallback          , "AsyncCallback")
DEFINE_STRING_CONSTANT(ComIAsyncResult           , "IAsyncResult")
DEFINE_STRING_CONSTANT(CreateDelegate            , "CreateDelegate")

// Collections, disposal, and enumeration strings

DEFINE_STRING_CONSTANT(ComCollectionsDomain      , "System.Collections")
DEFINE_STRING_CONSTANT(ComCollectionObjectModelDomain, "System.Collections.ObjectModel")
DEFINE_STRING_CONSTANT(ComGenericCollectionsDomain, "System.Collections.Generic")
DEFINE_STRING_CONSTANT(ComSpecializedCollectionsDomain, "System.Collections.Specialized")
DEFINE_STRING_CONSTANT(ComComponentModelDomain   , "System.ComponentModel")
DEFINE_STRING_CONSTANT(SystemIEnumerator         , "System.Collections.IEnumerator")
DEFINE_STRING_CONSTANT(ComArrayList              , "ArrayList")
DEFINE_STRING_CONSTANT(ComIEnumerator            , "IEnumerator")
DEFINE_STRING_CONSTANT(ForEachMoveNext           , "MoveNext")

DEFINE_STRING_CONSTANT(ForEachCurrent            , "Current")
DEFINE_STRING_CONSTANT(ComIEnumerable            , "IEnumerable")
DEFINE_STRING_CONSTANT(ComGenericIEnumerable     , "IEnumerable`1")

DEFINE_STRING_CONSTANT(ComGenericIEnumerator     , "IEnumerator`1")

DEFINE_STRING_CONSTANT(ForEachGetEnumerator      , "GetEnumerator")
DEFINE_STRING_CONSTANT(ComIDisposable            , "IDisposable")
DEFINE_STRING_CONSTANT(ComDispose                , "Dispose")
DEFINE_STRING_CONSTANT(SystemIDisposableDispose  , "System.IDisposable.Dispose")

DEFINE_STRING_CONSTANT(ComICollection            , "ICollection")
DEFINE_STRING_CONSTANT(ComIDictionary            , "IDictionary")
DEFINE_STRING_CONSTANT(ComDictionaryEntry        , "DictionaryEntry")
DEFINE_STRING_CONSTANT(ComIList                  , "IList")
DEFINE_STRING_CONSTANT(ComIReadOnlyList          , "IReadOnlyList")
DEFINE_STRING_CONSTANT(ComIReadOnlyCollection    , "IReadOnlyCollection")
DEFINE_STRING_CONSTANT(ComIReadOnlyDictionary    , "IReadOnlyDictionary")
DEFINE_STRING_CONSTANT(ComINotifyPropertyChanged , "INotifyPropertyChanged")
DEFINE_STRING_CONSTANT(ComINotifyCollectionChanged, "INotifyCollectionChanged")

// Attribute strings

DEFINE_STRING_CONSTANT(ComAttribute              , "Attribute")
DEFINE_STRING_CONSTANT(ComSecurityAttribute      , "SecurityAttribute")
DEFINE_STRING_CONSTANT(ComSecurityCriticalAttribute  , "SecurityCriticalAttribute")
DEFINE_STRING_CONSTANT(ComSecuritySafeCriticalAttribute  , "SecuritySafeCriticalAttribute")
DEFINE_STRING_CONSTANT(ComAttributeUsageAttribute, "AttributeUsageAttribute")
DEFINE_STRING_CONSTANT(ComCLSCompliantAttribute  , "CLSCompliantAttribute")
DEFINE_STRING_CONSTANT(ComDebuggerNonUserCodeAttribute, "DebuggerNonUserCodeAttribute")
DEFINE_STRING_CONSTANT(ComDebuggerStepThroughAttribute, "DebuggerStepThroughAttribute")
DEFINE_STRING_CONSTANT(ComDebuggerDisplayAttribute, "DebuggerDisplayAttribute")
DEFINE_STRING_CONSTANT(ComCompilationRelaxationsAttribute, "CompilationRelaxationsAttribute")
DEFINE_STRING_CONSTANT(ComCompilerGeneratedAttribute, "CompilerGeneratedAttribute")
DEFINE_STRING_CONSTANT(ComRuntimeCompatibilityAttribute, "RuntimeCompatibilityAttribute")
DEFINE_STRING_CONSTANT(ComInternalsVisibleToAttribute, "InternalsVisibleToAttribute")
DEFINE_STRING_CONSTANT(ComAssemblyDelaySignAttribute, "AssemblyDelaySignAttribute")
DEFINE_STRING_CONSTANT(ComAssemblyKeyFileAttribute  , "AssemblyKeyFileAttribute")
DEFINE_STRING_CONSTANT(ComAssemblyKeyNameAttribute  , "AssemblyKeyNameAttribute")
DEFINE_STRING_CONSTANT(ComAssemblyVersionAttribute  , "AssemblyVersionAttribute")
DEFINE_STRING_CONSTANT(ComAssemblyFlagsAttribute	, "AssemblyFlagsAttribute")

DEFINE_STRING_CONSTANT(ComAsyncStateMachineAttribute, "AsyncStateMachineAttribute")
DEFINE_STRING_CONSTANT(ComIteratorStateMachineAttribute, "IteratorStateMachineAttribute")



// SyncLock strings

DEFINE_STRING_CONSTANT(SystemThreading           , "System.Threading")
DEFINE_STRING_CONSTANT(SyncLockMonitor           , "Monitor")
DEFINE_STRING_CONSTANT(SystemThreadingMonitor    , "System.Threading.Monitor")
DEFINE_STRING_CONSTANT(SyncLockEnter             , "Enter")
DEFINE_STRING_CONSTANT(SyncLockExit              , "Exit")

// Missing strings

DEFINE_STRING_CONSTANT(ComReflectionDomain       , "System.Reflection")
DEFINE_STRING_CONSTANT(ComMissing                , "Missing")
DEFINE_STRING_CONSTANT(Value                     , "Value")
DEFINE_STRING_CONSTANT(SystemReflectionMissingValue, "System.Reflection.Missing.Value")

// EnC strings
DEFINE_STRING_CONSTANT(ENCTrackingList           , "__ENCList")
DEFINE_STRING_CONSTANT(ENCTrackingListIterator   , "__ENCIterate")
DEFINE_STRING_CONSTANT(ENCTrackingListAddTo      , "__ENCAddToList")
DEFINE_STRING_CONSTANT(ENCUpdateHandlers         , "__ENCUpdateHandlers")
DEFINE_STRING_CONSTANT(ENCHiddenRefresh          , "__ENCHiddenRefresh")
DEFINE_STRING_CONSTANT(ENCSharedHiddenRefresh    , "__ENCSharedHiddenRefresh")

// Strings for general use

DEFINE_STRING_CONSTANT(Version                   , "Version")
DEFINE_STRING_CONSTANT(MyBase                    , "MyBase")
DEFINE_STRING_CONSTANT(MyClass                   , "MyClass")
DEFINE_STRING_CONSTANT(Me                        , "Me")
DEFINE_STRING_CONSTANT(New                       , "New")
DEFINE_STRING_CONSTANT(Constructor               , ".ctor")
DEFINE_STRING_CONSTANT(SharedConstructor         , ".cctor")
DEFINE_STRING_CONSTANT(ArrayGet                  , "Get")
DEFINE_STRING_CONSTANT(ArrayAddress              , "Address")
DEFINE_STRING_CONSTANT(ArraySet                  , "Set")
DEFINE_STRING_CONSTANT(AsToken                   , "As")
DEFINE_STRING_CONSTANT(EndToken                  , "End")
DEFINE_STRING_CONSTANT(IfToken                   , "If")
DEFINE_STRING_CONSTANT(Main                      , "Main")
DEFINE_STRING_CONSTANT(Form                      , "Form")
DEFINE_STRING_CONSTANT(ClassInitialize           , "Class_Initialize")
DEFINE_STRING_CONSTANT(ClassTerminate            , "Class_Terminate")
DEFINE_STRING_CONSTANT(LetPrefix                 , "let_")
DEFINE_STRING_CONSTANT(Param                     , "Param")
DEFINE_STRING_CONSTANT(OperatorPrefix            , "op_")
DEFINE_STRING_CONSTANT(DebugException            , "$Exception")
DEFINE_STRING_CONSTANT(Add                       , "Add")
DEFINE_STRING_CONSTANT(Disposed                  , "disposedValue")
DEFINE_STRING_CONSTANT(Finalize                  , "Finalize")
DEFINE_STRING_CONSTANT(StartupMyFormFactory      , "STARTUP_MY_FORM_FACTORY")
DEFINE_STRING_CONSTANT(DeleteImplicit            , "$$delete")
DEFINE_STRING_CONSTANT(InitializeComponent       , "InitializeComponent")
DEFINE_STRING_CONSTANT(System                    , "System")
DEFINE_STRING_CONSTANT(Runtime                   , "Runtime")
DEFINE_STRING_CONSTANT(Extension                 , "Extension")
DEFINE_STRING_CONSTANT(Extensions                , "Extensions")
DEFINE_STRING_CONSTANT(CType                     , "CType")
DEFINE_STRING_CONSTANT(HasValue                  , "HasValue")
DEFINE_STRING_CONSTANT(GetValueOrDefault         , "GetValueOrDefault")
DEFINE_STRING_CONSTANT(Global                    , "Global")

// Strings concerning badness

DEFINE_STRING_CONSTANT(BadNamespace              , "<BadNamespace>")
DEFINE_STRING_CONSTANT(BadName                   , "<bad name>")
DEFINE_STRING_CONSTANT(UnnamedNamespaceErrName   , "<Default>")

// String for a parameter with the name 'value' (i.e property set)
DEFINE_STRING_CONSTANT(ValueParam                , "value")

DEFINE_STRING_CONSTANT(My                        , "My")

 // Xml Literal string constants
DEFINE_STRING_CONSTANT(SystemXml                , "System.Xml")
DEFINE_STRING_CONSTANT(SystemXmlLinq            , "System.Xml.Linq")
DEFINE_STRING_CONSTANT(ComXmlDomain             , "Xml")

DEFINE_STRING_CONSTANT(ComXmlObject             , "XObject")
DEFINE_STRING_CONSTANT(ComXmlContainer          , "XContainer")
DEFINE_STRING_CONSTANT(ComXmlDocument           , "XDocument")
DEFINE_STRING_CONSTANT(ComXmlDeclaration        , "XDeclaration")
DEFINE_STRING_CONSTANT(ComXmlElement            , "XElement")
DEFINE_STRING_CONSTANT(ComXmlName               , "XName")
DEFINE_STRING_CONSTANT(ComXmlNamespace          , "XNamespace")
DEFINE_STRING_CONSTANT(ComXmlElementSequence    , "XElementSequence")
DEFINE_STRING_CONSTANT(ComXmlAttribute          , "XAttribute")
DEFINE_STRING_CONSTANT(ComXmlComment            , "XComment")
DEFINE_STRING_CONSTANT(ComXmlText               , "XText")
DEFINE_STRING_CONSTANT(ComXmlCData              , "XCData")
DEFINE_STRING_CONSTANT(ComXmlProcessingInstruction, "XProcessingInstruction")
DEFINE_STRING_CONSTANT(ComXmlHelper             , "InternalXmlHelper")
DEFINE_STRING_CONSTANT(ComMyXmlHelper           , "My.InternalXmlHelper")

DEFINE_STRING_CONSTANT(XmlDeclVersion           , "version")
DEFINE_STRING_CONSTANT(XmlDeclEncoding          , "encoding")
DEFINE_STRING_CONSTANT(XmlDeclStandalone        , "standalone")
DEFINE_STRING_CONSTANT(V10                      , "1.0")
DEFINE_STRING_CONSTANT(Yes                      , "yes")
DEFINE_STRING_CONSTANT(No                       , "no")
DEFINE_STRING_CONSTANT(XmlNs                    , "xmlns")
DEFINE_STRING_CONSTANT(Xml                      , "xml")
DEFINE_STRING_CONSTANT(Lang                     , "lang")
DEFINE_STRING_CONSTANT(Space                    , "space")
DEFINE_STRING_CONSTANT(Preserve                 , "preserve")
DEFINE_STRING_CONSTANT(Default                  , "default")
DEFINE_STRING_CONSTANT(amp                      , "amp")
DEFINE_STRING_CONSTANT(apos                     , "apos")
DEFINE_STRING_CONSTANT(gt                       , "gt")
DEFINE_STRING_CONSTANT(lt                       , "lt")
DEFINE_STRING_CONSTANT(quot                     , "quot")
DEFINE_STRING_CONSTANT(DOCTYPE                  , "DOCTYPE")

DEFINE_STRING_CONSTANT(XmlNamespaceURI          , "http://www.w3.org/XML/1998/namespace")
DEFINE_STRING_CONSTANT(XmlNsNamespaceURI        , "http://www.w3.org/2000/xmlns/")

DEFINE_STRING_CONSTANT(XmlGetMethod             , "Get")
DEFINE_STRING_CONSTANT(XmlLoadMethod            , "Load")
DEFINE_STRING_CONSTANT(XmlElementMethod         , "Element")
DEFINE_STRING_CONSTANT(XmlElementsMethod        , "Elements")
DEFINE_STRING_CONSTANT(XmlDescendantsMethod     , "Descendants")
DEFINE_STRING_CONSTANT(XmlAttributeValueMethod  , "AttributeValue")
DEFINE_STRING_CONSTANT(XmlFirstMethod           , "First")
DEFINE_STRING_CONSTANT(XmlContentMethod         , "Content")
DEFINE_STRING_CONSTANT(XmlCreateAttributeMethod , "CreateAttribute")
DEFINE_STRING_CONSTANT(XmlCreateNamespaceMethod , "CreateNamespaceAttribute")
DEFINE_STRING_CONSTANT(XmlRemoveNamespacesMethod , "RemoveNamespaceAttributes")
DEFINE_STRING_CONSTANT(XmlGetNameMethod         , "GetName")
DEFINE_STRING_CONSTANT(XmlGetNamespaceMethod    , "GetNamespace")


DEFINE_STRING_CONSTANT(Lambda                   , "Lambda")

// Sequence operator string constants
DEFINE_STRING_CONSTANT(ComSystemLinqDomain      , "System.Linq")
DEFINE_STRING_CONSTANT(ComLinqDomain            , "Linq")
DEFINE_STRING_CONSTANT(ComMemberBinding         , "MemberBinding")
DEFINE_STRING_CONSTANT(ComGenericFunc           , "Func`1")
DEFINE_STRING_CONSTANT(ComGenericFunc2          , "Func`2")
DEFINE_STRING_CONSTANT(ComGenericFunc3          , "Func`3")
DEFINE_STRING_CONSTANT(ComGenericFunc4          , "Func`4")
DEFINE_STRING_CONSTANT(ComGenericFunc5          , "Func`5")
DEFINE_STRING_CONSTANT(ComAction                , "Action")

DEFINE_STRING_CONSTANT(ComIQueryable            , "IQueryable")
DEFINE_STRING_CONSTANT(ComGenericIQueryable     , "IQueryable`1")
DEFINE_STRING_CONSTANT(ComLinqEnumerable        , "Enumerable")

DEFINE_STRING_CONSTANT(ComSystemExpressionsDomain, "System.Linq.Expressions")
DEFINE_STRING_CONSTANT(ComElementInit           , "ElementInit")
DEFINE_STRING_CONSTANT(ComExpressionsDomain     , "Expressions")
DEFINE_STRING_CONSTANT(ComNewExpression         , "NewExpression")

DEFINE_STRING_CONSTANT(ComExpressionAdd         , "Add")
DEFINE_STRING_CONSTANT(ComExpressionAddChecked  , "AddChecked")
DEFINE_STRING_CONSTANT(ComExpressionAnd         , "And")
DEFINE_STRING_CONSTANT(ComExpressionAndAlso     , "AndAlso")
DEFINE_STRING_CONSTANT(ComExpressionArrayIndex  , "ArrayIndex")
DEFINE_STRING_CONSTANT(ComExpressionArrayLength , "ArrayLength")
DEFINE_STRING_CONSTANT(ComExpressionBind        , "Bind")
DEFINE_STRING_CONSTANT(ComExpressionCall        , "Call")
DEFINE_STRING_CONSTANT(ComExpressionCallVirtual , "Call")
DEFINE_STRING_CONSTANT(ComExpressionCoalesce    , "Coalesce")
DEFINE_STRING_CONSTANT(ComExpressionCast        , "Convert")
DEFINE_STRING_CONSTANT(ComExpressionCondition   , "Condition")
DEFINE_STRING_CONSTANT(ComExpressionConstant    , "Constant")
DEFINE_STRING_CONSTANT(ComExpressionConvert     , "Convert")
DEFINE_STRING_CONSTANT(ComExpressionConvertChecked, "ConvertChecked")
DEFINE_STRING_CONSTANT(ComExpressionDivide      , "Divide")
DEFINE_STRING_CONSTANT(ComExpressionEQ          , "Equal")
DEFINE_STRING_CONSTANT(ComExpressionElementInit , "ElementInit")
DEFINE_STRING_CONSTANT(ComExpression            , "Expression")
DEFINE_STRING_CONSTANT(ComGenericExpression     , "Expression`1")
DEFINE_STRING_CONSTANT(ComExpressionField       , "Field")
DEFINE_STRING_CONSTANT(ComExpressionGT          , "GreaterThan")
DEFINE_STRING_CONSTANT(ComExpressionGE          , "GreaterThanOrEqual")
DEFINE_STRING_CONSTANT(ComExpressionInvoke      , "Invoke")
DEFINE_STRING_CONSTANT(ComExpressionLambda      , "Lambda")
DEFINE_STRING_CONSTANT(ComExpressionLambdaExpression, "LambdaExpression")
DEFINE_STRING_CONSTANT(ComExpressionLE          , "LessThanOrEqual")
DEFINE_STRING_CONSTANT(ComExpressionLeftShift   , "LeftShift")
DEFINE_STRING_CONSTANT(ComExpressionLT          , "LessThan")
DEFINE_STRING_CONSTANT(ComExpressionLike        , "Like")
DEFINE_STRING_CONSTANT(ComExpressionListInit    , "ListInit")
DEFINE_STRING_CONSTANT(ComExpressionMemberInit  , "MemberInit")
DEFINE_STRING_CONSTANT(ComExpressionModulo      , "Modulo")
DEFINE_STRING_CONSTANT(ComExpressionMultiply    , "Multiply")
DEFINE_STRING_CONSTANT(ComExpressionMultiplyChecked, "MultiplyChecked")
DEFINE_STRING_CONSTANT(ComExpressionNE          , "NotEqual")
DEFINE_STRING_CONSTANT(ComExpressionNegate      , "Negate")
DEFINE_STRING_CONSTANT(ComExpressionNegateChecked , "NegateChecked")
DEFINE_STRING_CONSTANT(ComExpressionNew         , "New")
DEFINE_STRING_CONSTANT(ComExpressionNewArrayBounds , "NewArrayBounds")
DEFINE_STRING_CONSTANT(ComExpressionNewArrayInit, "NewArrayInit")
DEFINE_STRING_CONSTANT(ComExpressionNot         , "Not")
DEFINE_STRING_CONSTANT(ComExpressionOr          , "Or")
DEFINE_STRING_CONSTANT(ComExpressionOrElse      , "OrElse")
DEFINE_STRING_CONSTANT(ComExpressionParameter   , "Parameter")
DEFINE_STRING_CONSTANT(ComParameterExpression   , "ParameterExpression")
DEFINE_STRING_CONSTANT(ComExpressionPower       , "Power")
DEFINE_STRING_CONSTANT(ComExpressionPlus        , "UnaryPlus")
DEFINE_STRING_CONSTANT(ComExpressionProperty    , "Property")
DEFINE_STRING_CONSTANT(ComExpressionQuote       , "Quote")
DEFINE_STRING_CONSTANT(ComExpressionRightShift  , "RightShift")
DEFINE_STRING_CONSTANT(ComExpressionSubtract    , "Subtract")
DEFINE_STRING_CONSTANT(ComExpressionSubtractChecked, "SubtractChecked")
DEFINE_STRING_CONSTANT(ComExpressionTypeAs      , "TypeAs")
DEFINE_STRING_CONSTANT(ComExpressionTypeIs      , "TypeIs")
DEFINE_STRING_CONSTANT(ComExpressionXor         , "ExclusiveOr")
DEFINE_STRING_CONSTANT(CoalesceLambdaArg        , "CoalesceLHS")

// Dev10 #850039 "risky" functions from Microsoft.VisualBasic.FileSystem.
// !!! ORDER IS IMPORTANT !!! 
// MSVBFSDir should be first
// MSVBFSSetAttr should be last
// Compiler::Compiler relies on this.
DEFINE_STRING_CONSTANT(MSVBFSDir,"Dir")
DEFINE_STRING_CONSTANT(MSVBFSFileOpen, "FileOpen")
DEFINE_STRING_CONSTANT(MSVBFSFileClose, "FileClose")
DEFINE_STRING_CONSTANT(MSVBFSFileGetObject, "FileGetObject")
DEFINE_STRING_CONSTANT(MSVBFSFileGet, "FileGet")
DEFINE_STRING_CONSTANT(MSVBFSFilePutObject, "FilePutObject")
DEFINE_STRING_CONSTANT(MSVBFSPrint, "Print")
DEFINE_STRING_CONSTANT(MSVBFSPrintLine, "PrintLine")
DEFINE_STRING_CONSTANT(MSVBFSInput, "Input")
DEFINE_STRING_CONSTANT(MSVBFSWrite, "Write")
DEFINE_STRING_CONSTANT(MSVBFSWriteLine, "WriteLine")
DEFINE_STRING_CONSTANT(MSVBFSInputString, "InputString")
DEFINE_STRING_CONSTANT(MSVBFSFileAttr, "FileAttr")
DEFINE_STRING_CONSTANT(MSVBFSLineInput, "LineInput")
DEFINE_STRING_CONSTANT(MSVBFSEOF, "EOF")
DEFINE_STRING_CONSTANT(MSVBFSReset, "Reset")
DEFINE_STRING_CONSTANT(MSVBFSLock, "Lock")
DEFINE_STRING_CONSTANT(MSVBFSUnlock, "Unlock")
DEFINE_STRING_CONSTANT(MSVBFSLoc, "Loc")
DEFINE_STRING_CONSTANT(MSVBFSLOF, "LOF")
DEFINE_STRING_CONSTANT(MSVBFSSeek, "Seek")
DEFINE_STRING_CONSTANT(MSVBFSFileWidth, "FileWidth")
DEFINE_STRING_CONSTANT(MSVBFSFreeFile, "FreeFile")
DEFINE_STRING_CONSTANT(MSVBFSFilePut,"FilePut")
DEFINE_STRING_CONSTANT(MSVBFSFileCopy, "FileCopy")
DEFINE_STRING_CONSTANT(MSVBFSRename, "Rename")
DEFINE_STRING_CONSTANT(MSVBFSKill, "Kill")
DEFINE_STRING_CONSTANT(MSVBFSSetAttr, "SetAttr")

// Dev10 #850039 "risky" functions from Microsoft.VisualBasic.ApplicationServices.ApplicationBase.
DEFINE_STRING_CONSTANT(MSVBASABInfo, "get_Info")

// Dev10 #850039 "risky" functions from Microsoft.VisualBasic.ApplicationServices.WindowsFormsApplicationBase.
DEFINE_STRING_CONSTANT(MSVBASWFABRun, "Run")

// Dev10 #850039 "risky" functions from Microsoft.VisualBasic.ErrObject.
DEFINE_STRING_CONSTANT(MSVBERROBJRaise, "Raise")

// Async and Iterator state-machine strings
// Convention: strings that are up to us the compiler, are left as string constants here.
DEFINE_STRING_CONSTANT(ComSystemThreadingTasksDomain             , "System.Threading.Tasks")
DEFINE_STRING_CONSTANT(ComTask                                   , "Task")
DEFINE_STRING_CONSTANT(ComGenericTask                            , "Task`1")
DEFINE_STRING_CONSTANT(ComAsyncTaskMethodBuilder                 , "AsyncTaskMethodBuilder")
DEFINE_STRING_CONSTANT(ComGenericAsyncTaskMethodBuilder          , "AsyncTaskMethodBuilder`1")
DEFINE_STRING_CONSTANT(ComAsyncVoidMethodBuilder                 , "AsyncVoidMethodBuilder")
DEFINE_STRING_CONSTANT(ComINotifyCompletion                      , "INotifyCompletion")
DEFINE_STRING_CONSTANT(ComICriticalNotifyCompletion              , "ICriticalNotifyCompletion")
DEFINE_STRING_CONSTANT(ComIAsyncStateMachine                     , "IAsyncStateMachine")

DEFINE_STRING_CONSTANT(StateMachineClassFieldBuilder             , "$Builder")
DEFINE_STRING_CONSTANT(StateMachineClassFieldState               , "$State")
DEFINE_STRING_CONSTANT(StateMachineClassFieldDisposing           , "$Disposing")  // WARNING: this text literal is also embedded in ResumableMethodLowerer::AddDisposeMethodToStateMachineClass
DEFINE_STRING_CONSTANT(StateMachineClassFieldCurrent             , "$Current") // WARNING: this text literal is also embedded in ResumableMethodLowerer::AddCurrentPropertyToStateMachineClass
DEFINE_STRING_CONSTANT(StateMachineClassFieldInitialThreadId     , "$InitialThreadId") // WARNING: this text literal is also embedded in ResumableMethodLowerer::AddConstructorToStateMachineClass
DEFINE_STRING_CONSTANT(StateMachineClassFieldMoveNextDelegate    , "$MoveNextDelegate")
DEFINE_STRING_CONSTANT(StateMachineVariableName                  , "$sm")
DEFINE_STRING_CONSTANT(StateMachineDoFinallyBodies               , "VB$doFinallyBodies")
DEFINE_STRING_CONSTANT(StateMachineExceptionName                 , "$ex")
DEFINE_STRING_CONSTANT(StateMachineClassFieldStack               , "$Stack")
DEFINE_STRING_CONSTANT(StateMachineReturnTemp                    , "VB$returnTemp")
DEFINE_STRING_CONSTANT(AsyncBuilderSetNotificationForWaitCompletion       , "SetNotificationForWaitCompletion")

// Tuple-related strings
// The order of these is important; do not change it.
DEFINE_STRING_CONSTANT(ComGenericTuple1     , "Tuple`1")
DEFINE_STRING_CONSTANT(ComGenericTuple2     , "Tuple`2")
DEFINE_STRING_CONSTANT(ComGenericTuple3     , "Tuple`3")
DEFINE_STRING_CONSTANT(ComGenericTuple4     , "Tuple`4")
DEFINE_STRING_CONSTANT(ComGenericTuple5     , "Tuple`5")
DEFINE_STRING_CONSTANT(ComGenericTuple6     , "Tuple`6")
DEFINE_STRING_CONSTANT(ComGenericTuple7     , "Tuple`7")
DEFINE_STRING_CONSTANT(ComGenericTuple8     , "Tuple`8")

DEFINE_STRING_CONSTANT(TupleItem1           , "Item1")
DEFINE_STRING_CONSTANT(TupleItem2           , "Item2")
DEFINE_STRING_CONSTANT(TupleItem3           , "Item3")
DEFINE_STRING_CONSTANT(TupleItem4           , "Item4")
DEFINE_STRING_CONSTANT(TupleItem5           , "Item5")
DEFINE_STRING_CONSTANT(TupleItem6           , "Item6")
DEFINE_STRING_CONSTANT(TupleItem7           , "Item7")
DEFINE_STRING_CONSTANT(TupleItem8           , "Rest")

// Windows Runtime async interfaces
DEFINE_STRING_CONSTANT(WindowsFoundationDomain, "Windows.Foundation")
DEFINE_STRING_CONSTANT(IAsyncAction         , "IAsyncAction")
DEFINE_STRING_CONSTANT(IAsyncActionWithProgress, "IAsyncActionWithProgress`1")
DEFINE_STRING_CONSTANT(IAsyncOperation      , "IAsyncOperation`1")
DEFINE_STRING_CONSTANT(IAsyncOperationWithProgress, "IAsyncOperationWithProgress`2")

DEFINE_STRING_CONSTANT(ComIsVolatile, "IsVolatile")

#endif
