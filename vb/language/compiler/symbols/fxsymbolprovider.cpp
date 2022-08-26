//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  The type tables for intrinsic types, and helper methods.
//
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"

using namespace FX;

// ----------------------------------------------------------------------------
// Map vtypes to CLR names
// ----------------------------------------------------------------------------

const WCHAR * g_wszCLRNameOfVtype[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  WIDE(clrname),
    #include "TypeTables.h"
    #undef DEF_TYPE
};

const WCHAR * g_wszVBNameOfVtype[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  WIDE(vbname),
    #include "TypeTables.h"
    #undef DEF_TYPE
};

struct WellKnownFrameworkTypeName
{
    WCHAR *namespaceName;
    WCHAR *typeName;
};

WellKnownFrameworkTypeName g_FrameWorkTypeNames[] =
{
    #define DEF_CLRSYM(type, clrscope, clrname, innetcf, runtimever) { WIDE(clrscope), WIDE(clrname) },
    #include "TypeTables.h"
    #undef DEF_CLRSYM
};

// ----------------------------------------------------------------------------
// Construct the FX symbol provider and initialize our data.
// ----------------------------------------------------------------------------
FXSymbolProvider::FXSymbolProvider(Compiler * pCompiler) :
    m_pCompiler(pCompiler)
{
    AssertIfNull(pCompiler);

    ZeroMemory(m_NullableIntrinsicTypes, sizeof(BCSYM_GenericTypeBinding*) * (t_max + 1));

    // Initialize the cache of FX symbols that we refer to during semantic analysis and code-gen
    for (int Symbol = 0; Symbol < TypeMax; Symbol++)
    {
        // Setting Ownership to true just means that the list will delete anything left in it when
        // the CSingleList destructor runs.  We are still responsible for deleting the memory of guys
        // we remove
        m_FXTypes[Symbol].SetOwnership(true);
        VSASSERT( m_FXTypes[Symbol].NumberOfEntries() == 0, "Why isn't m_FXType[] clear?" );
    } 

    for (unsigned typ = t_UNDEF; typ < t_max; typ++)
    {
        if (*g_wszCLRNameOfVtype[typ])
        {
            m_Intrinsics[typ].m_pstrName = m_pCompiler->AddString(g_wszCLRNameOfVtype[typ]);
        }

        if (*g_wszVBNameOfVtype[typ])
        {
            m_Intrinsics[typ].m_pstrVBName = m_pCompiler->AddString(g_wszVBNameOfVtype[typ]);
        }
    }

    // Preload the bad name

    m_Intrinsics[t_bad].m_pnamed = Symbols::GetGenericBadNamedRoot();
}

// ----------------------------------------------------------------------------
// Return the printable name of a type from the CLR name.
// ----------------------------------------------------------------------------
STRING *FXSymbolProvider::GetTypeName(TypeName type)
{
    WellKnownFrameworkTypeName *pFXName = &(g_FrameWorkTypeNames[type]);
    
    StringBuffer typeName;
    unsigned numTypeArgs;
    typeName.AppendString(GetActualTypeNameFromEmittedTypeName(m_pCompiler->AddString(pFXName->typeName), -1, m_pCompiler, &numTypeArgs));

    if (numTypeArgs > 0)
    {
        typeName.AppendChar(L'(');
        typeName.AppendSTRING(m_pCompiler->TokenToString(tkOF));
        typeName.AppendChar(L' ');

        for (unsigned i = 0; i < numTypeArgs - 1; i++)
        {
            typeName.AppendChar(L',');
        }

        typeName.AppendChar(L')');
    }

    return m_pCompiler->AddString(typeName.GetString());
}

// ----------------------------------------------------------------------------
// Return the namespace of a type.
// ----------------------------------------------------------------------------
STRING *FXSymbolProvider::GetNamespaceName(TypeName type)
{
    return m_pCompiler->AddString(g_FrameWorkTypeNames[type].namespaceName);
}

// ----------------------------------------------------------------------------
// Return the fully qualified name of a type.
// ----------------------------------------------------------------------------
STRING *FXSymbolProvider::GetFullyQualifiedName(TypeName type)
{
    StringBuffer typeName;
    typeName.AppendString(GetNamespaceName(type));
    typeName.AppendString(L".");
    typeName.AppendString(GetTypeName(type));
    return m_pCompiler->AddString(typeName.GetString());
}

// ----------------------------------------------------------------------------
// When we import MsCorLib.dll, and we import the System.Object class, we
// need to mark it as being the special Object symbol.
// We also map the table of well known .NET symbols to the symbol table
// element we imported that represnets System.Object.
// ----------------------------------------------------------------------------

bool FXSymbolProvider::LoadType
(
    _In_z_ STRING *Namespace,
    BCSYM_NamedRoot *Class,
    CompilerProject *OwningProject // the project from which we are loading this symbol
)
{
    STRING *ClassName = Class->GetName();
    TypeName SymbolType;

    SymbolType = LoadType(Namespace, ClassName);

    if (SymbolType != TypeMax)
    {
        // Track the project that owns this symbol
        WellKnownFrameworkType  *pWellKnownFrameworkType  = new(zeromemory)WellKnownFrameworkType ; // we will need to delete this entry when we remove it
        pWellKnownFrameworkType->pCompilerProject = OwningProject;
        pWellKnownFrameworkType->pFXType = Class;
        m_FXTypes[SymbolType].InsertFirst( pWellKnownFrameworkType );

        // If this is System.Object, mark it special
        if (SymbolType == ObjectType)
        {
            Class->PClass()->SetIsObject(true);
        }
        else if (SymbolType == ValueTypeType)
        {
            Class->PClass()->SetIsCOMValueType(true);
        }
        return true;
    }

    return false;
}

TypeName FXSymbolProvider::LoadType
(
    _In_z_ STRING *Namespace,
    _In_z_ STRING *ClassName
)
{
    TypeName SymbolType = TypeMax;

    if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComException), ClassName))
        {
            SymbolType = ExceptionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDelegate), ClassName))
        {
            SymbolType = DelegateType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMDelegate), ClassName))
        {
            SymbolType = MultiCastDelegateType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIDisposable), ClassName))
        {
            SymbolType = IDisposableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComClassType), ClassName))
        {
            SymbolType = TypeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComValueType), ClassName))
        {
            SymbolType = ValueTypeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComEnum), ClassName))
        {
            SymbolType = EnumType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComArray), ClassName))
        {
            SymbolType = ArrayType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComObject), ClassName))
        {
            SymbolType = ObjectType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAsyncCallback), ClassName))
        {
            SymbolType = AsyncCallbackType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIAsyncResult), ClassName))
        {
            SymbolType = IAsyncResultType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAttribute), ClassName))
        {
            SymbolType = AttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAttributeUsageAttribute), ClassName))
        {
            SymbolType = AttributeUsageAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, Void), ClassName))
        {
            SymbolType = VoidType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIntPtr), ClassName))
        {
            SymbolType = IntPtrType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComUIntPtr), ClassName))
        {
            SymbolType = UIntPtrType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDBNull), ClassName))
        {
            SymbolType = DBNullType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComEventArgs), ClassName))
        {
            SymbolType = EventArgsType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComCLSCompliantAttribute), ClassName))
        {
            SymbolType = CLSCompliantAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComWeakReference), ClassName))
        {
            SymbolType = WeakReferenceType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComTypedReference), ClassName))
        {
            SymbolType = TypedReferenceType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComArgIterator), ClassName))
        {
            SymbolType = ArgIteratorType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComRuntimeArgumentHandle), ClassName))
        {
            SymbolType = RuntimeArgumentHandleType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericNullable), ClassName))
        {
            SymbolType = GenericNullableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericFunc), ClassName))
        {
            SymbolType = GenericFuncType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericFunc2), ClassName))
        {
            SymbolType = GenericFunc2Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericFunc3), ClassName))
        {
            SymbolType = GenericFunc3Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericFunc4), ClassName))
        {
            SymbolType = GenericFunc4Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericFunc5), ClassName))
        {
            SymbolType = GenericFunc5Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAction), ClassName))
        {
            SymbolType = ActionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIEquatable), ClassName))
        {
            SymbolType = GenericIEquatableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple1), ClassName))
        {
            SymbolType = GenericTuple1Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple2), ClassName))
        {
            SymbolType = GenericTuple2Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple3), ClassName))
        {
            SymbolType = GenericTuple3Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple4), ClassName))
        {
            SymbolType = GenericTuple4Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple5), ClassName))
        {
            SymbolType = GenericTuple5Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple6), ClassName))
        {
            SymbolType = GenericTuple6Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple7), ClassName))
        {
            SymbolType = GenericTuple7Type;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTuple8), ClassName))
        {
            SymbolType = GenericTuple8Type;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComReflectionDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAssemblyDelaySignAttribute), ClassName))
        {
            SymbolType = AssemblyDelaySignAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAssemblyKeyFileAttribute), ClassName))
        {
            SymbolType = AssemblyKeyFileAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAssemblyKeyNameAttribute), ClassName))
        {
            SymbolType = AssemblyKeyNameAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAssemblyVersionAttribute), ClassName))
        {
            SymbolType = AssemblyVersionAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAssemblyFlagsAttribute), ClassName))
        {
            SymbolType = AssemblyFlagsAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMissing), ClassName))
        {
            SymbolType = MissingType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComFieldInfoType), ClassName))
        {
            SymbolType = FieldInfoType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMethodInfoType), ClassName))
        {
            SymbolType = MethodInfoType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMethodBaseType), ClassName))
        {
            SymbolType = MethodBaseType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComConstructorInfoType), ClassName))
        {
            SymbolType = ConstructorInfoType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMethodBaseType), ClassName))
        {
            SymbolType = MethodBaseType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComInteropServicesDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, DispatchWrapper), ClassName))
        {
            SymbolType = DispatchWrapperType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, UnknownWrapper), ClassName))
        {
            SymbolType = UnknownWrapperType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, DefaultCharSetAttribute), ClassName))
        {
            SymbolType = DefaultCharSetAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, MarshalAsAttribute), ClassName))
        {
            SymbolType = MarshalAsType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, Marshal), ClassName))
        {
            SymbolType = MarshalType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, WinRTInteropServicesDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, WindowsRuntimeMarshal), ClassName))
        {
            SymbolType = WindowsRuntimeMarshalType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationToken), ClassName))
        {
            SymbolType = EventRegistrationTokenType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, EventRegistrationTokenTable), ClassName))
        {
            SymbolType = EventRegistrationTokenTableType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecurityPermissionsDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecurityAttribute), ClassName))
        {
            SymbolType = SecurityAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecurityAction), ClassName))
        {
            SymbolType = SecurityActionType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecurityDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecurityCriticalAttribute), ClassName))
        {
            SymbolType = SecurityCriticalAttributeType;
        }               
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSecuritySafeCriticalAttribute), ClassName))
        {
            SymbolType = SecuritySafeCriticalAttributeType;
        }       
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComCollectionsDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIEnumerable), ClassName))
        {
            SymbolType = IEnumerableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIEnumerator), ClassName))
        {
            SymbolType = IEnumeratorType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComArrayList), ClassName))
        {
            SymbolType = ArrayListType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComICollection), ClassName))
        {
            SymbolType = ICollectionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIDictionary), ClassName))
        {
            SymbolType = IDictionaryType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDictionaryEntry), ClassName))
        {
            SymbolType = DictionaryEntryType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIList), ClassName))
        {
            SymbolType = IListType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericCollectionsDomain), Namespace))
    {
        // 






        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIEnumerable), ClassName))
        {
            SymbolType = GenericIEnumerableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIEnumerator), ClassName))
        {
            SymbolType = GenericIEnumeratorType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericICollection), ClassName))
        {
            SymbolType = GenericICollectionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIList), ClassName))
        {
            SymbolType = GenericIListType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIReadOnlyList), ClassName))
        {
            SymbolType = GenericIReadOnlyListType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIReadOnlyCollection), ClassName))
        {
            SymbolType = GenericIReadOnlyCollectionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericList), ClassName))
        {
            SymbolType = GenericListType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIDictionary), ClassName))
        {
            SymbolType = GenericIDictionaryType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericKeyValuePair), ClassName))
        {
            SymbolType = GenericKeyValuePairType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComCollectionObjectModelDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericCollection), ClassName))
        {
            SymbolType = GenericCollectionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericReadOnlyCollection), ClassName))
        {
            SymbolType = GenericReadOnlyCollectionType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDiagnosticsDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDebuggerNonUserCodeAttribute), ClassName))
        {
            SymbolType = DebuggerNonUserCodeAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDebuggerStepThroughAttribute), ClassName))
        {
            SymbolType = DebuggerStepThroughAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDebuggerDisplayAttribute), ClassName))
        {
            SymbolType = DebuggerDisplayAttributeType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComRuntimeCompilerServicesDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComCompilationRelaxationsAttribute), ClassName))
        {
            SymbolType = CompilationRelaxationsAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComCompilerGeneratedAttribute), ClassName))
        {
            SymbolType = CompilerGeneratedAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComRuntimeCompatibilityAttribute), ClassName))
        {
            SymbolType = RuntimeCompatibilityAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAsyncTaskMethodBuilder), ClassName))
        {
            SymbolType = AsyncTaskMethodBuilderType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericAsyncTaskMethodBuilder), ClassName))
        {
            SymbolType = GenericAsyncTaskMethodBuilderType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAsyncVoidMethodBuilder), ClassName))
        {
            SymbolType = AsyncVoidMethodBuilderType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIsVolatile), ClassName))
        {
            SymbolType = IsVolatileType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComINotifyCompletion), ClassName))
        {
            SymbolType = INotifyCompletionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComICriticalNotifyCompletion), ClassName))
        {
            SymbolType = ICriticalNotifyCompletionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIAsyncStateMachine), ClassName))
        {
            SymbolType = IAsyncStateMachineType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComAsyncStateMachineAttribute), ClassName))
        {
            SymbolType = AsyncStateMachineAttributeType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIteratorStateMachineAttribute), ClassName))
        {
            SymbolType = IteratorStateMachineAttributeType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSystemExpressionsDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComNewExpression), ClassName))
        {
            SymbolType = NewExpressionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComElementInit), ClassName))
        {
            SymbolType = ElementInitType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComExpression), ClassName))
        {
            SymbolType = ExpressionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericExpression), ClassName))
        {
            SymbolType = GenericExpressionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComExpressionLambdaExpression), ClassName))
        {
            SymbolType = LambdaExpressionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComParameterExpression), ClassName))
        {
            SymbolType = ParameterExpressionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComMemberBinding), ClassName))
        {
            SymbolType = MemberBindingType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSystemLinqDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComIQueryable), ClassName))
        {
            SymbolType = IQueryableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericIQueryable), ClassName))
        {
            SymbolType = GenericIQueryableType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComLinqEnumerable), ClassName))
        {
            SymbolType = LinqEnumerableType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComSystemThreadingTasksDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComTask), ClassName))
        {
            SymbolType = TaskType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComGenericTask), ClassName))
        {
            SymbolType = GenericTaskType;
        }
    }
    else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, WindowsFoundationDomain), Namespace))
    {
        if (StringPool::IsEqual(STRING_CONST(m_pCompiler, IAsyncAction), ClassName))
        {
            SymbolType = IAsyncActionType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, IAsyncActionWithProgress), ClassName))
        {
            SymbolType = IAsyncActionWithProgressType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, IAsyncOperation), ClassName))
        {
            SymbolType = IAsyncOperationType;
        }
        else if (StringPool::IsEqual(STRING_CONST(m_pCompiler, IAsyncOperationWithProgress), ClassName))
        {
            SymbolType = IAsyncOperationWithProgressType;
        }
    }

    return SymbolType;
}

BCSYM_NamedRoot* FXSymbolProvider::GetNullableIntrinsicSymbol(Vtypes vtype)
{
    AssertIfFalse(vtype >= 0 && vtype < t_max);

    // Lazy initialization of nullable intrinsic types.

    BCSYM_NamedRoot *pSymbol = m_NullableIntrinsicTypes[vtype];

    if (!pSymbol)
    {
        // Get underlying intrinsic type first.

        BCSYM_NamedRoot *pNamed = GetType(vtype);
        AssertIfFalse(pNamed && pNamed->GetCompilerFile());

        // Make sure System.Nullable(Of T) is there and can be used.

        if (pNamed && pNamed->GetCompilerFile() && IsTypeAvailable(GenericNullableType))
        {
            // Use same symbols as underlying intrinsic type is using.

            Symbols symbols(
                m_pCompiler,
                pNamed->GetCompilerFile()->SymbolStorage(),
                NULL,
                pNamed->GetCompilerFile()->GetCurrentGenericBindingCache()
             );

            // Generate and cache a nullable wrapper.

            pSymbol = symbols.GetGenericBinding(
                false,
                GetType(GenericNullableType),
                (BCSYM**)&pNamed,
                1,
                NULL,
                true
            );
        }

        // If failed, return bad symbol.
        if (!pSymbol)
        {
            pSymbol = Symbols::GetGenericBadNamedRoot();
        }

        // Cache the result.
        m_NullableIntrinsicTypes[vtype] = pSymbol;
    }

    // Always returns non-NULL.
    AssertIfNull(pSymbol);
    return pSymbol;
}

// ----------------------------------------------------------------------------
// Is this an intrinsic symbol? If so, store away
// ----------------------------------------------------------------------------

bool FXSymbolProvider::LoadIntrinsicType
(
    _In_z_ STRING *Namespace,
    BCSYM_Class *Class   // Potential intrinsic symbol
)
{
    if (StringPool::IsEqual(STRING_CONST(m_pCompiler, ComDomain), Namespace))
    {
        STRING *ClassName = Class->GetName();
        for (int typ = t_UNDEF; typ < t_max; typ++)
        {
            if (StringPool::IsEqual(m_Intrinsics[typ].m_pstrName, ClassName))
            {
                Vtypes vtype = (Vtypes)typ;

                VSASSERT(Class->IsClass(), "All intrinsic types are classes or value classes");

                m_Intrinsics[vtype].m_pnamed = Class;

                // Modify the Vtype of the symbol to reflect the intrinsic type.
                Class->PClass()->SetIsIntrinsicType();
                Class->PClass()->SetVtype(vtype);

                // Change the name of the symbol to the VB intrinsic name.
                Class->SetName(m_pCompiler->TokenToString(g_rgtokITypes[vtype]));
                return true;
            }
        }
    }

    return false;
}

struct WellKnownFXTypeHelper
{
public:
    
    // ----------------------------------------------------------------------------
    // Initialize the iterator. Go through the list and construct a new list of 
    // symbols that are valid for the current project.
    // ----------------------------------------------------------------------------
    WellKnownFXTypeHelper(CSingleList<WellKnownFrameworkType> *pList, CompilerProject *pCurrentProject) :
      m_NumberOfMatches(0), m_pFirstMatchingType(NULL)
      {
          CSingleListIter<WellKnownFrameworkType> iterList(pList);
          WellKnownFrameworkType *pType;

          while(pType = iterList.Next())
          {
              // Skip over a symbol if the current project does not reference this symbol's project. The 
              // case where we should not do this check are: 
              // If we are not currently compiling a project (EE).
              // If while compiling a project, FXSymbolProvider of another host was called (happens while traversing namespace rings)
              // and if the current project is not a metadata file (IsProjectReference won't return the right value for metadataprojects)
              if (pCurrentProject && 
                  pCurrentProject->GetCompilerHost() == pType->pCompilerProject->GetCompilerHost() &&
                  !pCurrentProject->IsMetaData() &&
                  pCurrentProject != pType->pCompilerProject &&
                  !pCurrentProject->IsProjectReferenced(pType->pCompilerProject))
              {
                  continue;
              }
              else
              {
                  m_NumberOfMatches++;
                  if ( m_NumberOfMatches > 1 )
                  {
                      // It is invalid to have more than one match, so don't bother continuing to iterate.
                      return;
                  }
                  m_pFirstMatchingType = pType->pFXType;
              }
          }
    }

    BCSYM_NamedRoot* GetType()
    {
        VSASSERT( m_NumberOfMatches == 1 && m_pFirstMatchingType, "Type is either not available, or it is ambiguous" );
        return m_pFirstMatchingType;
    }

    int NumberOfMatches()
    {
        return m_NumberOfMatches;
    }

private:
    ULONG m_NumberOfMatches;
    BCSYM_NamedRoot *m_pFirstMatchingType;
};

// ----------------------------------------------------------------------------
// Some symbols are not available on other platforms, such as CF (Compact Frameworks).
// Use this function to determine if the Type is available.
// ----------------------------------------------------------------------------
bool FXSymbolProvider::IsTypeAvailable(TypeName Type)
{
    if (Type >= 0 && Type < _countof(m_FXTypes))
    {
        // Must be public and we must have only imported the symbol from one project
        // If we imported it multiple times we can't guarantee this type is the FX
        // type we expected so we say it isn't available.

        CompilerProject *pCurrentProject = m_pCompiler->GetProjectBeingCompiled();
        WellKnownFXTypeHelper wellKnownFXTypeHelper( &m_FXTypes[Type], pCurrentProject );
        
        if ( wellKnownFXTypeHelper.NumberOfMatches() == 1 )
        {
            BCSYM_NamedRoot *pType = wellKnownFXTypeHelper.GetType();
            if ( pType->GetAccess() == ACCESS_Public )
            {
                return true;
            }
        }
    }

    return false;
}

// ----------------------------------------------------------------------------
// Retrieve types stored in the table.
// ----------------------------------------------------------------------------

BCSYM_NamedRoot* FXSymbolProvider::GetType(TypeName Type)
{
    // If we imported a fx symbol multiple times we can't guarantee it is the FX type we are expecting.
    // So we say it isn't available in that case.
    if (Type >= 0 && Type < _countof(m_FXTypes))
    {
        CompilerProject *pCurrentProject = m_pCompiler->GetProjectBeingCompiled();
        WellKnownFXTypeHelper wellKnownFXTypeHelper( &m_FXTypes[Type], pCurrentProject );

        if ( wellKnownFXTypeHelper.NumberOfMatches() == 1 )
        {
            BCSYM_NamedRoot *pType = wellKnownFXTypeHelper.GetType();
            return pType;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------------
// Helper to report errors about missing or amiguous types.
// ----------------------------------------------------------------------------
void FXSymbolProvider::ReportTypeMissingErrors(TypeName Type, ErrorTable *pErrorTable, const Location &ErrorLocation)
{
    if ( Type >= 0 && Type < _countof(m_FXTypes) && pErrorTable )
    {
        CompilerProject *pCurrentProject = m_pCompiler->GetProjectBeingCompiled();
        WellKnownFXTypeHelper wellKnownFXTypeHelper( &m_FXTypes[Type], pCurrentProject );

        if ( wellKnownFXTypeHelper.NumberOfMatches() == 0 )
        {
            pErrorTable->CreateError(ERRID_UndefinedType1, (Location *) &ErrorLocation, GetFullyQualifiedName(Type));
        }
        else
        {
            VSASSERT( wellKnownFXTypeHelper.NumberOfMatches() == 2, "wellKnownFXTypeHelper should have found 2 matches" );
            pErrorTable->CreateError(ERRID_AmbiguousInNamespace2, 
                (Location *) &ErrorLocation,
                GetTypeName(Type),
                GetNamespaceName(Type));
        }
    }
}

// ----------------------------------------------------------------------------
// Remove any well-known framework types that we keep around for semantics and
// code-gen purposes, that belong to a project that is being unloaded
// ----------------------------------------------------------------------------
void FXSymbolProvider::RemoveFXTypesForProject(
    CompilerProject *Project // The project that is being unloaded
)
{
    // Don't remove symbols contributed by mscorlib.dll  We assume that this dll
    // doesn't get decompiled and reloaded during the normal course of events
    // in the IDE.  By not removing the symbols that would have come from mscorlib.dll,
    // we can assert in LoadTypes() that our assumption is holding that mscorlib.dll
    // doesn't get decompiled (unless the compiler is shutting down) because we'll 
    // assert on the mscorlib.dll types if loaded a second time.
    if ( Project->GetCompilerHost()->GetComPlusProject() == Project )
    {
        // Note - m_FXTypes[] has ownership of the elements inside the list so
        // it will delete the memory associated with those items when the list is destroyed.
        return;
    }

    
    // Find all the symbols that came from the project being unloaded
    for (int Symbol = 0; Symbol < TypeMax; Symbol++)
    {
        CSingleListIter<WellKnownFrameworkType > FXTypeIter(& m_FXTypes[Symbol]);
        while (WellKnownFrameworkType *pType = FXTypeIter.Next())
        {
            if ( pType->pCompilerProject == Project )
            {
                m_FXTypes[Symbol].Remove( pType );
                delete pType; // we own the memory for the WellKnownFrameworkType entries we put in this list
                break; // projects are only in the list for a given symbol once
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Return a nullable of a type.
// ----------------------------------------------------------------------------

BCSYM_NamedRoot* FXSymbolProvider::GetNullableType(BCSYM* pNamed, Symbols* pSymbols)
{
    AssertIfNull( pNamed );
    AssertIfNull( pSymbols );
    AssertIfFalse( pNamed->IsType()) ;

    BCSYM_NamedRoot* pSymbol = NULL;

    // Make sure System.Nullable(Of T) is there and can be used.
    if( pNamed != NULL &&
        !pNamed->IsBad() &&
        IsTypeAvailable(GenericNullableType))
    {
        // Generate and cache a nullable wrapper.
        pSymbol = pSymbols->GetGenericBinding(
            false, // IsBad
            GetType(GenericNullableType), // Generic
            (BCSYM**)&pNamed, // Arguments
            1, // ArgumentCount
            NULL, // ParentBinding
            true // AllocAndCopyArgumentsToNewList
        );
    }

    // If failed, return bad symbol.
    if (pSymbol == NULL)
    {
        pSymbol = Symbols::GetGenericBadNamedRoot();
    }

    AssertIfNull(pSymbol);
    return pSymbol;
}

// ----------------------------------------------------------------------------
// Return a System.Func(Of ...) symbol.
// ----------------------------------------------------------------------------

BCSYM_NamedRoot* FXSymbolProvider::GetFuncSymbol(UINT nArgs, BCSYM* typeArgs[], Symbols* pSymbols)
{
    AssertIfNull( typeArgs );
    AssertIfNull( pSymbols );
    VSASSERT( nArgs > 0 && nArgs <= 5, "Func can have at most 5 type parameters" );

    BCSYM_NamedRoot* pSymbol = NULL;

    TypeName Map[ 5 ];
    Map[ 0 ] = GenericFuncType;
    Map[ 1 ] = GenericFunc2Type;
    Map[ 2 ] = GenericFunc3Type;
    Map[ 3 ] = GenericFunc4Type;
    Map[ 4 ] = GenericFunc5Type;

    // Make sure System.Func(Of ...) is there and can be used.
    if( typeArgs != NULL &&
        nArgs > 0 && nArgs <= 5 &&
        IsTypeAvailable(Map[ nArgs - 1 ] ))
    {
        pSymbol = pSymbols->GetGenericBinding(
            false, // IsBad
            GetType( Map[ nArgs - 1 ] ),
            typeArgs, // Arguments
            nArgs, // ArgumentCount
            NULL, // ParentBinding
            true // AllocAndCopyArgumentsToNewList
        );
    }

    // If failed, return bad symbol.
    if (pSymbol == NULL)
    {
        pSymbol = Symbols::GetGenericBadNamedRoot();
    }

    AssertIfNull(pSymbol);
    return pSymbol;
}

// ----------------------------------------------------------------------------
// Return a System.Tuple(Of ...) symbol.
// ----------------------------------------------------------------------------
BCSYM_NamedRoot* FXSymbolProvider::GetTupleSymbol(UINT nArgs, BCSYM* typeArgs[], Symbols* pSymbols)
{
    AssertIfNull(typeArgs);
    AssertIfNull(pSymbols);
    VSASSERT(nArgs > 0 && nArgs <= 8, "Tuple can have at most 8 type parameters");

    BCSYM_NamedRoot* pSymbol = NULL;

    TypeName Map[8];
    Map[0] = GenericTuple1Type;
    Map[1] = GenericTuple2Type;
    Map[2] = GenericTuple3Type;
    Map[3] = GenericTuple4Type;
    Map[4] = GenericTuple5Type;
    Map[5] = GenericTuple6Type;
    Map[6] = GenericTuple7Type;
    Map[7] = GenericTuple8Type;

    // Make sure System.Tuple(Of ...) is there and can be used.
    if (typeArgs != NULL &&
        nArgs > 0 && nArgs <= 8 &&
        IsTypeAvailable(Map[nArgs - 1]))
    {
        pSymbol = pSymbols->GetGenericBinding(
            false, // IsBad
            GetType(Map[nArgs - 1]),
            typeArgs, // Arguments
            nArgs, // ArgumentCount
            NULL, // ParentBinding
            true // AllocAndCopyArgumentsToNewList
        );
    }

    // If failed, return bad symbol.
    if (pSymbol == NULL)
    {
        pSymbol = Symbols::GetGenericBadNamedRoot();
    }

    AssertIfNull(pSymbol);
    return pSymbol;
}
