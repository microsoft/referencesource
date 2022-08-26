//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Definition of the interface used to access the symbols.
//
//  How to create a new kind of symbol:
//  Two categories of symbol kinds are:
//    (1) Creatable kinds - You will actually allocate a symbol of this kind.
//    (2) Abstract kinds - This symbol kind is used only as a base class
//        for other symbol kinds.  It is never allocated directly.
//  
//  Two other ways of dividing symbol kinds is:
//    (1) Symbol kinds from which other kinds derive (non-leaf).
//    (2) Symbol kinds from which no other kind derives (leaf).
//  
//  It is possible for a kind to either be:
//    Creatable leaf (e.g. NewedMemberVar)
//    Creatable non-leaf (e.g. MemberVar, which has some derived classes).
//    Abstract non-leaf (e.g. Proc)
//  
//  It makes no sense for there to be an abstract leaf.
//  
//  For any new kind of symbol, do the following:
//    Add a pair of method declarations to the BCSYM class:
//       bool Is<kind>();
//       class BCSYM_<kind> *P<kind>();
//  
//    Define the BCSYM_<kind> class somewhere in this file, below the BCSYM class.
//    The BCSYM_<kind> class must derive from BCSYM or one of its derived classes.
//    Make sure the base classes are marked as public bases.
//  
//  If the new kind of symbol is creatable:
//    Go to biltypes.h and add a DEF_BCSYM line.  Remember where you put it
//    relative to the other symbol kinds.
//  
//    Go to bcsym.cpp and add a line to the s_rgBilkindInfo array in exactly
//    the same relative position as you added the DEF_BCSYM line (this array
//    is indexed by SYM_* enums).  Make sure the values of the array element
//    are correct for the new kind.  If you don't understand the values, talk
//    to someone who does.
//  
//    If the new kind of symbol is a leaf kind, implement the Is<kind> and
//    P<kind> methods as inline functions near the bottom of this file.  The
//    implementation should look like this (only for a creatable leaf kind):
//       inline bool Is<kind>()
//       {
//         KindEquals(<kind>);
//       }
//       inline BCSYM_<kind> *P<kind>()
//       {
//         CastSymbolKind(<kind>);
//       }
//  
//  If the new kind of symbol is a non-leaf (creatable or not):
//    Add a corresponding member to the BilkindInfo struct in this file.
//  
//    Add a corresponding column to the s_rgBilkindInfo array.  Set the values
//    of the new column appropriately (true iff the SYM_* kind derives, at least
//    indirectly, from the new kind).
//  
//    Implement the Is<kind> and P<kind> methods as inline functions near the
//    bottom of this file.  The implementation should look like this:
//       inline bool Is<kind>()
//       {
//         return DoesKindSupport(<kind>);
//       }
//       inline BCSYM_<kind> *P<kind>()
//       {
//         CastSymbolKind(<kind>);
//       }
//  
//-------------------------------------------------------------------------------------------------

// 




#pragma once

#if HOSTED
#include "VBHostedInterop.h"
#endif


struct PartialGenericBinding;
class GenericBindingInfo;
struct Location;
class TemporaryManager;
struct Temporary;
class IBitVector;
struct MethodScope;
struct BlockScope;

namespace ILTree
{
    struct ProcedureBlock;
    struct AttributeApplicationExpression;
}


//#define TAGSYMBOLENTRYFUNCTION 1
#if TAGSYMBOLENTRYFUNCTION
#define SymbolEntryFunction SymbolEntry(__FILE__, __LINE__, __FUNCTION__);
#else TAGSYMBOLENTRYFUNCTION
#define SymbolEntryFunction
#endif TAGSYMBOLENTRYFUNCTION



//****************************************************************************
// Aliases to prettify ugly names
//****************************************************************************

typedef class BCSYM_Hash Scope;
typedef class BCSYM Type;
typedef STRING Identifier;
typedef class BCSYM Symbol;
typedef class BCSYM_NamedRoot Declaration;
typedef class BCSYM_Variable Variable;
typedef class BCSYM_Member Member;
typedef class BCSYM_VariableWithValue Constant;
typedef class BCSYM_Expression SymbolicValue;
typedef class BCSYM_Param Parameter;
typedef class BCSYM_Proc Procedure;
typedef class BCSYM_MethodImpl ProcedureDefinition;
typedef class BCSYM_Property Property;
typedef class BCSYM_Class ClassOrRecordType;
typedef class BCSYM_Interface InterfaceType;
typedef class BCSYM_Implements InterfaceList;
typedef class BCSYM_HandlesList HandlesList;
typedef class BCSYM_ArrayType ArrayType;
typedef class BCSYM_PointerType PointerType;
typedef class BCSYM_Container Container;
typedef class BCSYM_Namespace Namespace;
typedef class BCSYM_UserDefinedOperator Operator;
typedef class BCSYM_GenericBinding GenericBinding;
typedef class BCSYM_GenericTypeBinding GenericTypeBinding;
typedef class BCSYM_GenericParam GenericParameter;
typedef class BCSYM_GenericConstraint GenericConstraint;
typedef class BCSYM_GenericTypeConstraint GenericTypeConstraint;
typedef class BCSYM_GenericNonTypeConstraint GenericNonTypeConstraint;
typedef class BCSYM_ExtensionCallLookupResult ExtensionCallLookupResult;
typedef class BCSYM_LiftedOperatorMethod LiftedOperatorMethod;

#define MAXBINDINGBITS 4

// Global functions

int _cdecl SortSymbolsByName(
    const void * arg1,
    const void * arg2);

// Compare two named roots by name using the current user's locale settings.
int __cdecl SortSymbolsByNameLocaleSensitive(
    const void * arg1,
    const void * arg2);

int _cdecl SortSymbolsBySourceFile(
    const void * arg1,
    const void * arg2);

// Compare two named roots only by location. No breaking of ties.
int _cdecl SortSymbolsByLocationUnstable(
    const void * arg1,
    const void * arg2);

// Compare two named roots first by location and then break ties by name.
int _cdecl SortSymbolsByLocation(
    const void * arg1,
    const void * arg2);

// Compare two named roots first by location and then break ties by name
// where the source file sort is case insensitive
int _cdecl SortSymbolsByLocationCaseInsensitive(
    const void * arg1,
    const void * arg2);

// Compare two named roots first by name and then by location.
int _cdecl SortSymbolsByNameAndLocation(
    const void * arg1,
    const void * arg2);

//  External Classes

class CompilerFile;
class CompilerProject;
class CompilerHost;
class ErrorTable;
class AttrVals;
class WellKnownAttrVals;
class MetaDataFile;
class SourceFile;
class Bindable;
struct StructCycleNode;
class XMLDocNode;

#define FRIENDS \
friend struct Symbols; \
friend class BCSYM; \
friend class BCSYM_NamedRoot; \
friend class CompilerProject; \
friend class SourceFile; \
friend LineMarkerTable; \
friend class Declared; \
friend class Semantics; \
friend class Bindable; \
friend class CDebugParsedExpression;\
friend class CVBDbgee; \
friend class Builder; \
friend class ClosureBase; \
friend class Closure; \
friend class GenericContext; \
friend class ClosureGenericIterator; \
friend class ClosureRoot;  \
friend class MyBaseMyClassStubContext; \
friend class BoundTreeVisitor;  \
friend class BoundTreeTypeVisitor;

#define DBGFRIENDS \
friend class CVBDbgee; \


// Forward declares.

struct BCITER_CHILD;
struct BCITER_CHILD_ALL;
struct BCITER_Parameters;
struct BCITER_ImplInterfaces;
struct ImportedTarget;
struct SymbolList;
class Semantics;

#define VBStaticPrefix (L"$STATIC$")
#define VBStaticDelimeter (L"$")
#define VBStaticDelimeterChar (L'$')
#define VBStaticPrefixLength 8

class LineMarker;
class StringBuffer;
class SourceFile;
class Closure;
class ClosureBase;

class BCSYM_EventDecl;
class BCSYM_Container;
class BCSYM_GenericParam;
class BCSYM_GenericBinding;
class BCSYM_Implements;

struct Symbols;
class GenericBindingCache;

template <int SIZE, class KEY_TYPE, class VALUE_TYPE>
class FixedSizeHashTable;

struct GenericParamsInferredTypesInfo
{
private:
    typedef FixedSizeHashTable<256, BCSYM_GenericParam *, BCSYM *> GenericParamsInferredTypesHash;

public:
    GenericParamsInferredTypesInfo();

    void AddTypeInferredForGenericParam(
        BCSYM_GenericParam * GenericParam,
        BCSYM * Type);

    BCSYM * FindTypeInferredForGenericParam(BCSYM_GenericParam * GenericParam);

    NorlsAllocator *GetAllocator()
    {
        return &m_Allocator;
    }

private:
    void Init();

    bool m_IsInitialized;
    NorlsAllocator m_Allocator;
    GenericParamsInferredTypesHash *m_InferredTypesHash;

};

//-------------------------------------------------------------------------------------------------
//
// Holds the debug information for a method.
//
struct MethodDebugInfo : public CSingleLink<MethodDebugInfo>
{

    STRING            *m_pstrDocumentName;  // Document that contains this method
    unsigned long      m_ulCurrNoLines;     // Current number of lines in the line table
    unsigned long      m_ulNoLines;         // Number of lines in the line table

    // Sequence table.
    unsigned int      *m_rglLineTable;      // Pointer to the line#s
    unsigned int      *m_rglOffsetTable;    // Pointer to the offsets
    unsigned int      *m_rglColumnTable;    // Pointer to the columns
    unsigned int      *m_rglEndLineTable;   // Pointer to the end line#s
    unsigned int      *m_rglEndColumnTable; // Pointer to the end columns
};

struct AsyncMethodDebugInfo
{
    BCSYM_Proc        *m_pKickoffProc;          // Original async method which has become the kickoff stub method
    unsigned long      m_ulNoAwaits;            // Number of awaits
    unsigned int       m_catchHandlerOffset;    // IL Offset of generated catch handler (for Async Subs only)

    unsigned int      *m_rglYieldOffsets;       // Yield IL offsets for each await
    unsigned int      *m_rglBreakpointOffsets;  // Resume IL offsets for each await
};

enum BindspaceType
{
    BINDSPACE_IgnoreSymbol = 0,  // Used to take a symbol out of consideration for binding.
    BINDSPACE_Normal       = 1,  // The usual variable/function/module namespace.
    BINDSPACE_Type         = 2,  // Contains named types.
    BINDSPACE_MAX = BINDSPACE_Type,
};

enum SimpleBindFlag
{
    SIMPLEBIND_Default                 = 0x0000,
    SIMPLEBIND_IgnoreOtherPartialTypes = 0x0001,
    SIMPLEBIND_CaseSensitive           = 0x0002
};

// Make sure the BindSpaceEnum doesn't go beyond the size we have allocated.
COMPILE_ASSERT(BINDSPACE_MAX < (1 << MAXBINDINGBITS));

// Used to determine dynamic type info at runtime.  Used instead of dynamic_cast<>
// because our BCSYM_ classes don't have virtual functions ( a requirement for using
// dynamic_cast<> ).  So a table of these will be allocated and the bit flags used to
// determine at runtime what behaviors a BCSYM object supports.  Table set up in bcsym.cpp
struct BilkindInfo
{
    bool m_isType:1; // Is this symbol a type
    bool m_isSimpleType:1; // Is this symbol a/derived from BCSYM_SimpleType
    bool m_isNamedRoot:1; // Is this symbol a/derived from BCSYM_NamedRoot
    bool m_isContainer:1; // Is this symbol a/derived from BCSYM_Container
    bool m_isClass:1; // Is this symbol a/derived from BCSYM_Class
    bool m_isMember:1; // Can this type be used as a class member?
    bool m_isProc:1; // Is this symobl a/derived from BCSYM_Proc
    bool m_isVariable:1; // Is this symbol a/derived from BCSYM_Variable
    bool m_isVariableWithValue:1; // Is this symbol a/derived from BCSYM_VariableWithValue
    bool m_isMethodDecl:1; // Is this symbol a/derived from BCSYM_MethodDecl
    bool m_isParam:1; // Is this symbol a/derived from BCSYM_Param
    bool m_hasDerivers:1; // Does this symbol have classes that derive from it
    bool m_isExpression:1; // Is this symbol a/derived from BCSYM_Expression
    bool m_isMethodImpl:1; // Is this symbol a/derived from BCSYM_MethodImpl
    bool m_isGenericConstraint:1; // Is this symbol a/derived from BCSYM_GenericConstraint
    bool m_isGenericBinding:1; // Is this symbol a/derived from BCSYM_GenericBinding
    bool m_isGenericBadNamedRoot:1; // Is this symbol a/derived from BCSYM_GenericBadNamedRoot
    bool m_isArrayType:1;
#if DEBUG
    BilKind m_kind;
#endif // DEBUG
};

//-------------------------------------------------------------------------------------------------
//
// ConstantValue is a compiler-specific representation of a typed constant value.  It is similar in
// nature to VARIANT, but is not encumbered by legacy semantics.
//
struct ConstantValue
{
    union
    {
        // All integral values are stored correctly widened to 64 bits.
        __int64 Integral;
        float Single;
        double Double;

        struct
        {
            const WCHAR *Spelling;                // Null Terminated
            unsigned LengthInCharacters;    // This is deliberately not size_t.
        } String;

        DECIMAL Decimal;

        struct
        {
            __int64 First;
            __int64 Second;
        } Mask;
    };

    Vtypes TypeCode;

    ConstantValue()
    {
        TypeCode = t_bad;
        Mask.First = 0;
        Mask.Second = 0;
    }

    ConstantValue( _In_ Compiler* pCompiler, _In_ const VARIANT& vt );

    // Two ConstantValues are equal only if their type codes and values are equal.
    static
    bool Equals(
        const ConstantValue &Left,
        const ConstantValue &Right);

    // Convert the Constant to a Variant
    void VariantFromConstant(VARIANT &VariantValue);

    // Copy the current constant into the destination
    void CopyConstant(
        ConstantValue * pDestination,
        NorlsAllocator * pAllocator);

    void SetVType(Vtypes VType)
    {
        TypeCode = VType;
    }

};

//-------------------------------------------------------------------------------------------------
//
// Different kinds of variables.
//
enum VARIABLEKIND
{
    VAR_Const,
    VAR_Member,
    VAR_WithEvents,
    VAR_Local,
    VAR_Param,
    VAR_FunctionResult
};

//-------------------------------------------------------------------------------------------------
//
// Different results from Compare.
//
enum Equality
{
    EQ_Match                            = 0x00000,  // The two symbols are identical
    EQ_Shape                            = 0x00001,  // Basic shapes of the symbols are different.  Other flags may not be set.
    EQ_Name                             = 0x00002,  // "sub foo()" and "sub bar()"
    EQ_ParamName                        = 0x00004,  // "sub foo(x)" and "sub bar(y)"
    EQ_Flags                            = 0x00008,  // "overloads sub foo" and "sub foo"
    EQ_Byref                            = 0x00010,  // "sub foo(byval x)" and "sub foo(byref x)"
    EQ_Optional                         = 0x00020,  // "sub foo(optional y as integer)" and "sub foo()"
    EQ_Return                           = 0x00040,  // "sub foo()", "function foo()" or "property foo() as integer", "Property foo() as string"
    EQ_Property                         = 0x00080,  // "ReadOnly property foo()" and "WriteOnly property foo()", etc.
    EQ_Default                          = 0x00100,  // "default sub foo()", "sub foo()"
    EQ_Bad                              = 0x00200,  // Compare a Bad to anything else ( maybe a Bad, too )
    EQ_ParamDefaultVal                  = 0x00400,  // "sub foo(optional x=1)" and "sub foo(optional x=2)"
    EQ_ParamArrayVsArray                = 0x00800,  // "sub foo(x()) and sub foo(paramArray x())
    EQ_OptionalTypes                    = 0x01000,  // "sub foo(optional x as integer=1)" and "sub foo(optional x as short=1)"
    EQ_GenericTypeParams                = 0x02000,  // The symbols differ by their types being different type parameters of enclosing generic
                                                    // types which could possibly be the same for some generic bindings.
    EQ_GenericMethodTypeParams          = 0x04000,  // The symbols differ by their types being different type parameters of generic methods
                                                    // both of which have identical numeric positions.
    EQ_GenericMethodTypeParamCount      = 0x08000,  // The method symbols differ by the number of generic type parameters.
    EQ_GenericTypeParamsForReturn       = 0x10000,  // The equivalent of EQ_GenericTypeParams for the return type
    EQ_GenericMethodTypeParamsForReturn = 0x20000   // The equivalent of EQ_GenericMethodTypeParams for the return type
};

//-------------------------------------------------------------------------------------------------
//
// Different kinds of synthetic code we can produce.
//
enum SyntheticKind
{
    SYNTH_None,
    SYNTH_New,
    SYNTH_SharedNew,
    SYNTH_WithEventsGet,
    SYNTH_WithEventsSet,
    SYNTH_AddEvent,
    SYNTH_RemoveEvent,
    SYNTH_FormMain,
    SYNTH_MyGroupCollectionGet,
    SYNTH_MyGroupCollectionSet,
    SYNTH_TransientSymbol,
    SYNTH_TransientNoIntCheckSymbol,
    SYNTH_AutoPropertyGet,
    SYNTH_AutoPropertySet,

#if IDE 
    SYNTH_IterateENCTrackingList,
    SYNTH_ENCUpdateHandler,
    SYNTH_ENCHiddenRefresh,
    SYNTH_ENCSharedHiddenRefresh,
    SYNTH_AddToENCTrackingList,
#endif IDE
};

//-------------------------------------------------------------------------------------------------
//
// Different kinds of tip we provide from GetBasicRep()
//
enum TIPKIND
{
    TIP_Normal,                 //default
    TIP_MyBase,                 //hack for mybase
    TIP_MyClass,                //hack for myclass
    TIP_OptionCompareText,      //Use Microsoft.VisualBasic.CompareMethod.Text
    TIP_UseNamedTypes,          // If the bound symbol is bad, use the namedtype info instead.
    TIP_ExtensionCall,          //Suppress first parameter for an extension call
    TIP_ExtensionAggregateOperator, // Extension method used as an aggregate operator
    TIP_AggregateOperator,          // Non extension method used as an aggregate operator
    TIP_AggregateOperatorParam,
    TIP_AnonymousDelegate,
};


//-------------------------------------------------------------------------------------------------
//
// Manages a singly linked list of symbol table symbols
//
struct SymbolList
{
public:

    SymbolList()
    : m_First(NULL)
    , m_Count(0)
    {
    }

    void Clear()
    {
        m_First = NULL;
        m_Count = 0;
    }

    BCSYM_NamedRoot * GetFirst()
    {
        VSASSERT(Verify(), "");
        return m_First;
    }

    unsigned GetCount()
    {
        VSASSERT(Verify(), "");
        return m_Count;
    }

    void AddToFront(_Out_ BCSYM_NamedRoot * NewItem);
    void AddToEnd(_In_ BCSYM_NamedRoot * NewItem);
    void Remove(BCSYM_NamedRoot * ItemToRemove);
    void RemoveAliased(BCSYM_NamedRoot * ItemToRemove);

#if DEBUG
    bool Contains(BCSYM_NamedRoot * Item);
#endif

private:

    BCSYM_NamedRoot *m_First;
    unsigned m_Count;

#if DEBUG
    bool Verify();
#endif
};

//-------------------------------------------------------------------------------------------------
//
// A GenericParams holds a set of generic parameters in two ways--a list in declaration
// order and a hash table for name lookup. (Following the lead of the rest of the symbol
// table, the list could be eliminated and an iterator that traverses the hash table
// in declaration order provided.)
//
struct GenericParams
{
    // The list of generic parameters in declaration order.
    BCSYM_GenericParam *m_FirstGenericParam;

    // Generic parameters go into this hash.
    BCSYM_Hash *m_GenericParamsHash;
};

//-------------------------------------------------------------------------------------------------
//
// The generic symbol.  All other symbol types derive from this.
//
class BCSYM
{
    DBGFRIENDS;
public:
#if DEBUG
    virtual BilKind AddBCSYMVTableForDebug() { // so debugger can show correct class in watch window
        return GetKind();   // some code so debugger can differentiate between this bcsym and biltree vtables
    }
#endif

    bool IsAttribute();
    // Returns the SYM_* constant denoting the kind of this symbol.
    BilKind GetKind() const
    {
        SymbolEntryFunction;
        VSASSERT( this && m_skKind >= SYM_Uninitialized && m_skKind < SYM_Max, "Bad symbol kind");
        return (BilKind)m_skKind;
    }

      //
      // Generic type methods
      //

    // Is this a type?  The generic type methods can only be called if this is true.
    bool IsType();

    // The vtype classification of this type.
    Vtypes GetVtype();

    // Is this the intrinsic type symbol for "System.Object"?
    bool IsObject();

    // Is this the intrinsic type symbol for "System.ValueType"?
    bool IsCOMValueType();

    // Is this a structure?
    bool IsStruct();

    // Is this a delegate?
    bool IsDelegate() const;

    // Is this an enum?
    bool IsEnum();

    bool IsLocal();

    bool IsAnonymousType();

    bool IsAnonymousDelegate();

    BCSYM * DigThroughArrayLiteralType(Symbols * pSymbols);

    // If this is a named type, dig through to the real type.
    // (resolving it first, if it hasn't yet been bound)
    // It is safe to call this on a NULL pointer.
    BCSYM * 
#if HOSTED
        __stdcall
#else
        _fastcall 
#endif
            DigThroughNamedType();

    // If this is an alias, named type, com named type, enum,
    // dig through the alias to the real type.
    // It is safe to call this on a NULL pointer.
    BCSYM * 
#if HOSTED
        __stdcall
#else
        _fastcall 
#endif
            DigThroughAlias();

    BCSYM * 
#if HOSTED
        __stdcall
#else
        _fastcall 
#endif
            DigThroughAliasToleratingNullResult();

    // Get to the root type, digging through everything we can.
    // It is safe to call this on a NULL pointer.
    BCSYM * _fastcall ChaseToType();

    //chases through pointer types to the underlying raw root
    BCSYM * _fastcall ChaseThroughPointerTypes();

    //chases through pointer types and generic type bindings to the underlying type.
    BCSYM * _fastcall ChaseThroughPointerTypesAndGenericTypeBindings();

    // Get to the raw namedtype, digging through everything we can.
    // It is safe to call this on a NULL pointer.
    BCSYM * _fastcall ChaseToNamedType();

    // Get location of the raw namedtype.
    // It is safe to call this on a NULL pointer.
    Location * _fastcall GetTypeReferenceLocation();

    // Is this symbol an intrinsic type that maps on to a COM+ type?
    bool IsIntrinsicType();

    bool IsTypeExtension();
    bool IsExtensionMethod(bool IgnoreFlagAndCheckAttrManually = false);

    //Obtains the object to use when binding against this symbol's members.
    //If the symbol has members that can be bound against it will return
    //a non null value in either ppHash or ppGenericParam, otherwise both will be null.
    //The arguments returned are suitable for use as the second and third arguments to Semantics::InterpretName
    //This method may be called on a null pointer.
    void GetBindingSource
    (
        CompilerHost * pCompilerHost,
        BCSYM_Hash ** ppHash, //[OUT] pointer to the hash to be returned
        BCSYM_GenericParam ** ppGenericParam //[OUT] pointer to the generic param to be returned
    );

    //Indicates wether or not the symbol represents an entity that needs to be "unwrapped"
    //before it can be bound against.
    //This method can be called on a null pointer.
    bool IsIndirectlyBindable();

    //Chase through a symbol down to something that can be bound against.
    //Unlike ChaseToType and ChaseToNamedType, this does not return the element type for array instances, but instead
    //returns System.Array
    //This method may be called on a null pointer.
    BCSYM * ChaseToBindableType(CompilerHost * pCompilerHost);

    static
    unsigned CompareProcs(
        BCSYM_Proc * Proc1,
        GenericBindingInfo Binding1,
        BCSYM_Proc * Proc2,
        GenericBindingInfo Binding2,
        Symbols * Symbols,
        Compiler* Compiler);

    // Compares two procs to see if they're shaped the same.
    static
    unsigned CompareProcs(
        BCSYM_Proc * Proc1,
        GenericBindingInfo Binding1,
        BCSYM_Proc * Proc2,
        GenericBindingInfo Binding2,
        Symbols * Symbols,
        bool ignoreOverloadDifferences = false,
        bool ignoreFirstParameterOfProc1 = false,
        bool ignoreFirstParameterOfProc2 = false);

    // Compares two params to see if they're shaped the same.
    static
    unsigned CompareParams(
        BCSYM_Param * Param1,
        GenericBindingInfo Binding1,
        BCSYM_Param * Param2,
        GenericBindingInfo Binding2,
        Symbols * Symbols);

    // Compare two referenced types.
    static
    unsigned CompareReferencedTypes(
        BCSYM * psym1,
        GenericBindingInfo pbind1,
        BCSYM * psym2,
        GenericBindingInfo pbind2,
        Symbols * psymbols);

    void BuildReferencedParameterSet(ISet<BCSYM_GenericParam *> * pSet);

private:

    // Compares two procs to see if they're shaped the same.
    static
    unsigned CompareProcs(
        BCSYM_Proc * Proc1,
        GenericBindingInfo Binding1,
        BCSYM_Proc * Proc2,
        GenericBindingInfo Binding2,
        Symbols * Symbols,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes,
        bool ignoreOverloadDifferences = false,
        bool ignoreFirstParameterOfProc1 = false,
        bool ignoreFirstParameterOfProc2 = false);

    // Compares two params to see if they're shaped the same.
    static
    unsigned CompareParams(
        BCSYM_Param * Param1,
        GenericBindingInfo Binding1,
        BCSYM_Param * Param2,
        GenericBindingInfo Binding2,
        Symbols * Symbols,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes);

    // Compare two referenced types.
    static
    unsigned CompareReferencedTypes(
        BCSYM * psym1,
        GenericBindingInfo pbind1,
        BCSYM * psym2,
        GenericBindingInfo pbind2,
        Symbols * psymbols,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes);

    static
    unsigned CompareGenericParamAndType(
        BCSYM_GenericParam * Type1,
        GenericBindingInfo Binding1,
        BCSYM * Type2,
        GenericBindingInfo Binding2,
        Symbols * Symbols,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes);

    static
    BCSYM * GetInferredTypeForGenericParam(
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes,
        BCSYM_GenericParam * GenericParam,
        BCSYM_GenericParam * &LastDependentOnGenericParam);

    static
    bool InferTypeForGenericParam(
        BCSYM * Type,
        BCSYM_GenericParam * GenericParam,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes);

    static
    bool DoesTypeDependOnGenericParam(
        BCSYM * Type,
        BCSYM_GenericParam * GenericParam,
        GenericParamsInferredTypesInfo * GenericParamsInferredTypes);

public:

    // Compares two type symbols to see if they refer to the same type.
    static
    bool AreTypesEqual(
        BCSYM * ptyp1,
        BCSYM * ptyp2);

    static
    bool AreTypesEqual(
        BCSYM * ptyp1,
        BCSYM * ptyp2,
        bool fDigIntoArraysAndGenericBindings);

    // Return TRUE if this is a bad named root
    inline bool IsBad() const;

    // Returns TRUE if this symbol has a source location associated with it.
    bool HasLocation() const
    {
        SymbolEntryFunction;
        return m_hasLocation;
    }
    void SetHasLocation(bool value)
    {
        SymbolEntryFunction;
        m_hasLocation = value; 
    }

    //  Returns the source location for this symbol.
    Location *GetLocation();

    //  Returns the source TrackedLocation for this symbol.
    TrackedLocation *GetTrackedLocation();

    void SetLocation(const Location *ploc)
    {
        SymbolEntryFunction;
        VSASSERT(HasLocation(), "Symbol has no location allocated, can't set it.");
        *GetLocation() = *ploc;
    }

    void SetLocation(const Location *ploc, bool fLocationInherited)
    {
        SymbolEntryFunction;
        SetLocation(ploc);
        SetIsLocationInherited(fLocationInherited);
    }

    bool IsLocationInherited() const
    {
        SymbolEntryFunction;
        return m_isLocationInherited;
    }
  
    // Returns a buffer that contains the basic representation for
    // this symbol.  The string buffer must be passed in.
    //
    void GetBasicRep(
        Compiler * pCompiler,
        BCSYM_Container * pContextContainer,
        StringBuffer * pbuf,
        BCSYM_GenericBinding * pGenericBindingContext = NULL,
        _In_opt_z_ STRING * pstrNewName = NULL,
        bool fExpand = true,
        TIPKIND Special = TIP_Normal,
        IReadonlyBitVector * pGenericParamsToExclude = NULL,
        bool fQualifyExpansion = false);

    static
    void StringFromExpression(
        class BCSYM_Expression * Expression,
        StringBuffer * StringRep,
        bool fUseText = false);

protected:
    static bool IsVbConst(WCHAR wcChar);
    static void ExpandVbStringConst(_In_count_(lLength) const WCHAR *wszString, long lLength, StringBuffer *StringRep);

public:
    static
    BCSYM * FillInArray(
        Compiler * pCompiler,
        BCSYM * pType,
        StringBuffer * pbuf);
    
    static
    void FillInProc(
        Compiler * pCompiler,
        BCSYM_Container * pContextContainer,
        BCSYM_Proc * pproc,
        StringBuffer * pbuf,
        BCSYM_GenericBinding * pGenericBindingContext = NULL,
        TIPKIND Special = TIP_Normal,
        IReadonlyBitVector * pGenericParamsToExclude = NULL,
        _In_opt_z_ STRING* ParameterReplaceString = NULL,
        bool FillReturnType = true);


    static
    void BCSYM::FillInParameterAndReturnType
    (
        Compiler *pCompiler,
        BCSYM_Container *pContextContainer,
        BCSYM_Proc * pproc,
        StringBuffer *pbuf,
        BCSYM_GenericBinding *pGenericBindingContext,
        TIPKIND Special,
        _In_opt_z_ STRING* ParameterReplaceString,
        bool FillReturnType);

    static
    void FillInGenericParams(
        Compiler * pCompiler,
        BCSYM_Container * pContextContainer,
        BCSYM * psym,
        StringBuffer * pbuf,
        BCSYM_GenericBinding * pGenericBindingContext = NULL,
        TIPKIND Special = TIP_Normal,
        IReadonlyBitVector * pGenericParamsToExclude = NULL);

    static
    void FillInAnonymousDelegate(
        Compiler * pCompiler,
        BCSYM_Container * pContextContainer,
        BCSYM_Class * pAnonymousDelegate,
        StringBuffer * pbuf,
        BCSYM_GenericBinding * pGenericBindingContext = NULL);

    // Type name to use in error reporting
    STRING *GetErrorName(Compiler *pCompiler);

    bool IsGeneric();
    BCSYM_GenericParam *GetFirstGenericParam();
    BCSYM_Hash *GetGenericParamsHash();
    unsigned GetGenericParamCount();

    bool AreAttributesEmitted();

    BCSYM_Implements *GetFirstImplements();

    // The Global.FullyQualifiedName of the symbol.
    // The symbol must be a generic binding or a named root.
    STRING * GetGlobalQualifiedName(BCSYM_GenericBinding * pGenericContext = NULL);

  // For each BCSYM_* type, there is a pair of functions defined below.
  //
  // The Is<kind> function asks a BCSYM if it is of the particular kind of symbol.
  // The Is<kind> method cannot be called on a NULL pointer.
  //
  // The P<kind> function asserts that the symbol pointer is NULL or is of the
  // requested kind.  Then it casts the symbol to that kind and returns.
  // It is ok to call a P<kind> function on a NULL pointer (NULL is returned
  // in that case).
  //
  // These functions are actually implemented as inlines near the bottom of this file.

    bool IsSimpleType();
    class BCSYM_SimpleType *PSimpleType();
    bool IsVoidType();
    class BCSYM_VoidType *PVoidType();
    bool IsPointerType();
    class BCSYM_PointerType *PPointerType();
    bool IsNamedType() const;
    class BCSYM_NamedType *PNamedType() const;
    bool IsArrayType() const;
    class BCSYM_ArrayType *PArrayType() const;
    bool IsArrayLiteralType() const;
    class BCSYM_ArrayLiteralType * PArrayLiteralType() const;

    bool IsNamedRoot() const;
    class BCSYM_NamedRoot *PNamedRoot() const;
    bool IsGenericBadNamedRoot();
    class BCSYM_GenericBadNamedRoot *PGenericBadNamedRoot();
    bool IsHash();
    class BCSYM_Hash *PHash();
    bool IsAlias();
    class BCSYM_Alias *PAlias();
    bool IsContainer() const;
    class BCSYM_Container *PContainer() const;
    bool IsCCContainer();
    class BCSYM_CCContainer *PCCContainer();
    bool IsClass() const;
    class BCSYM_Class *PClass() const;
    bool IsMember();
    class BCSYM_Member *PMember();
    bool IsProc();
    class BCSYM_Proc *PProc();
    bool IsProperty();
    class BCSYM_Property *PProperty();
    bool IsMethodImpl();
    class BCSYM_MethodImpl *PMethodImpl();
    bool IsSyntheticMethod();
    class BCSYM_SyntheticMethod *PSyntheticMethod();
    bool IsMethodDecl();
    class BCSYM_MethodDecl *PMethodDecl();
    bool IsEventDecl();
    class BCSYM_EventDecl *PEventDecl();
    bool IsDllDeclare();
    class BCSYM_DllDeclare *PDllDeclare();
    bool IsVariable();
    class BCSYM_Variable *PVariable();
    bool IsVariableWithValue();
    class BCSYM_VariableWithValue *PVariableWithValue();
    bool IsVariableWithArraySizes();
    class BCSYM_VariableWithArraySizes *PVariableWithArraySizes();
    bool IsCCConstant();
    class BCSYM_CCConstant *PCCConstant();
    bool IsStaticLocalBackingField();
    class BCSYM_StaticLocalBackingField *PStaticLocalBackingField();
    bool IsParam() const;
    class BCSYM_Param *PParam()const;
    bool IsParamWithValue();
    class BCSYM_ParamWithValue *PParamWithValue();
    bool IsExpression();
    class BCSYM_Expression *PExpression();
    bool IsImplements() const;
    class BCSYM_Implements *PImplements()const;
    bool IsInterface();
    class BCSYM_Interface *PInterface();
    bool IsHandlesList() const;
    class BCSYM_HandlesList *PHandlesList()const;
    bool IsImplementsList();
    class BCSYM_ImplementsList *PImplementsList();
    bool IsNamespace() const;
    class BCSYM_Namespace *PNamespace();
    bool IsNamespaceRing();
    class BCSYM_NamespaceRing *PNamespaceRing();
    bool IsXmlName();
    class BCSYM_XmlName *PXmlName();
    bool IsXmlNamespaceDeclaration();
    class BCSYM_XmlNamespaceDeclaration *PXmlNamespaceDeclaration();
    bool IsXmlNamespace();
    class BCSYM_XmlNamespace *PXmlNamespace();
    bool IsApplAttr() const;
    class BCSYM_ApplAttr *PApplAttr()const;
    bool IsUserDefinedOperator();
    class BCSYM_UserDefinedOperator *PUserDefinedOperator();
    bool IsGenericParam();
    class BCSYM_GenericParam *PGenericParam();
    bool IsGenericConstraint();
    class BCSYM_GenericConstraint *PGenericConstraint();
    bool IsGenericTypeConstraint();
    class BCSYM_GenericTypeConstraint *PGenericTypeConstraint();
    bool IsGenericNonTypeConstraint();
    class BCSYM_GenericNonTypeConstraint *PGenericNonTypeConstraint();
    bool IsGenericBinding() const;
    class BCSYM_GenericBinding *PGenericBinding() const;
    bool IsGenericTypeBinding() const;
    class BCSYM_GenericTypeBinding *PGenericTypeBinding() const;
    bool IsTypeForwarder() const;
    class BCSYM_TypeForwarder *PTypeForwarder();
    bool IsExtensionCallLookupResult();
    class BCSYM_ExtensionCallLookupResult * PExtensionCallLookupResult();
    bool IsLiftedOperatorMethod();
    class BCSYM_LiftedOperatorMethod * PLiftedOperatorMethod();

    //Checks to see if the symbols is a class, and if so if its synthetic
    bool IsTransientClass();

    //
    // AttrVals is used to collect information from well-known custom
    // attributes.  However, we only allocate space for this data
    // in BCSYM_NamedRoot and BCSYM_Param.  We present these *non*-virtual
    // methods off of BCSYM and redirect to the BCSYM_NamedRoot and
    // BCSYM_Param versions.
    //
    AttrVals *GetPAttrVals();
    WellKnownAttrVals *GetPWellKnownAttrVals();
    void FreeAttrVals();

    void AllocAttrVals(
        NorlsAllocator * pnra,
        CompilationState state,
        SourceFile * psourcefile,
        BCSYM_Container * pcontainercontext,
        bool AllocForWellKnownAttributesToo = false);

    // COM methods to help determine if this symbol came from tlbimp
    bool IsComImportClass();

    bool CanOverloadWithExtensionMethods();
    #if DEBUG
        const BilKind GetSkKind() const
        {
            return m_skKind;
        }
        void SetSkKind(BilKind value)
        {
            SymbolEntryFunction;
            VSASSERT(value >= SYM_Uninitialized && value < SYM_Max, "Illegal BilKind" );
            m_skKind = value;
        }
        BilKind m_skKind; // The kind of symbol that we have. (Defined this way in debug so expansion with autoexp.dat file will work)
    #else
        const unsigned __int8 GetSkKind() const
        {
            return m_skKind;
        }
        void SetSkKind(unsigned __int8 value)
        {
            VSASSERT(value >= SYM_Uninitialized && value < SYM_Max, "Illegal BilKind" );
            m_skKind = value;
        }
        unsigned __int8 m_skKind; // The kind of symbol that we have.
    #endif

#if NRLSTRACK
    void SetRecordedAllocator(NorlsAllocator *pAllocator)
    {
        // for debug, let's record the allocator that allocated this particular symbol, so we can Assert
        m_RecordedAllocator = pAllocator;
    }
    NorlsAllocator *GetRecordedAllocator()
    {
        return m_RecordedAllocator;
    }
#endif NRLSTRACK
    
    static const size_t GetSkKindOffset()
    {
        return offsetof(BCSYM, m_skKind);
    }

    //there is already IsLocationInherited acessor
    void SetIsLocationInherited(bool value)
    {
        SymbolEntryFunction;
        m_isLocationInherited = value;
    }

#if HOSTED
    void SetExternalSymbol(IUnknown* pExternalSymbol);
    IUnknown* GetExternalSymbol();
#endif

protected:
    static const BilkindInfo s_rgBilkindInfo[]; // The table that gives us dynamic_cast<> like functionality


#if TAGSYMBOLENTRYFUNCTION
    void SymbolEntry(char *szFile, int nLineNo, char *func) const;
    ULONG m_nBorn;
    ULONG m_nLastRef;
    ULONG m_nCallCount;
    ULONG m_dwThreadId;
#endif
#if NRLSTRACK
    NorlsAllocator *m_RecordedAllocator;
#endif NRLSTRACK

    // As with all symbol table nodes, it's important to pack fields into as small a
    // space as possible. Turning these into bitfields won't actually decrease space,
    // because at least one 32-bit word is effectively consumed.

    bool m_hasLocation : 1;

    bool m_isLocationInherited : 1;  // some symbols might get location information from the symbols they are synthesized from,
                                        // we need to know this in order to decide whether to put them in the location hash
                                        //

    //accessors are defined in BCSYM_GenericConstraint
    bool m_fIsBadConstraint : 1;     // This is necessary so that bad constraints are skipped during constraint checking.
                                        // This is supposed to be in BCSYM_GenericConstraint, but increases its size by a whole
                                        // word for just this one bit. So, since there is space for more bits here in BCSYM,
                                        // adding it here. This is NEVER to be accessed directly, but should only be used through
                                        // the appropriate accessors in BCSYM_GenericConstraint.


// <these 2 are from BCSYM_Member> They are refactored here to save memory by using unused bits
    bool m_isShared:1;
    bool m_isBracketed:1;
// </these 2 are from BCSYM_Member>


#if HOSTED
    IUnknown* m_pExternalSymbol;
#endif

#if FV_TRACK_MEMORY && IDE
    unsigned long m_totalSize;

public:
    const unsigned long GetTotalSize() const
    {
        return m_totalSize;
    }
#endif

}; // end of BCSYM

struct ExtensionCallAndPrecedenceLevel
{
public:
    ExtensionCallAndPrecedenceLevel() :
        m_pProc(NULL),
        m_precedenceLevel(0)
    {
    }

    ExtensionCallAndPrecedenceLevel(
        Procedure * pProc,
        unsigned long precedenceLevel) :
        m_pProc(pProc),
        m_precedenceLevel(precedenceLevel)
    {
    }
    Procedure * m_pProc;
    unsigned long m_precedenceLevel;
};

struct ExtensionCallInfo
{
    ExtensionCallInfo() :
        m_pProc(NULL),
        m_precedenceLevel(0),
        m_pPartialGenericBinding(NULL)
    {
    }

    ExtensionCallInfo(
        Procedure * pProc,
        unsigned long precedenceLevel,
        PartialGenericBinding * pPartialGenericBinding) :
        m_pProc(pProc),
        m_precedenceLevel(precedenceLevel),
        m_pPartialGenericBinding(pPartialGenericBinding)
    {
    }

    ExtensionCallInfo(const ExtensionCallInfo & src) :
        m_pProc(src.m_pProc),
        m_precedenceLevel(src.m_precedenceLevel),
        m_pPartialGenericBinding(src.m_pPartialGenericBinding)
    {
    }

    unsigned long GetFreeArgumentCount();

    Procedure * m_pProc;
    unsigned long m_precedenceLevel;
    PartialGenericBinding * m_pPartialGenericBinding;
};

struct PartialGenericBinding
{
    friend class GenericBindingInfo;

    PartialGenericBinding(
        Location * pTypeArgumentLocations,
        GenericBinding * pGenericBinding,
        IBitVector * pFixedTypeArgumentBitVector) :
        m_pTypeArgumentLocations(pTypeArgumentLocations),
        m_pGenericBinding(pGenericBinding),
        m_pFixedTypeArgumentBitVector(pFixedTypeArgumentBitVector)
    {
        ThrowIfNull(pTypeArgumentLocations);
        ThrowIfNull(pGenericBinding);
        ThrowIfNull(pFixedTypeArgumentBitVector);
    }

    unsigned long GetFreeArgumentCount();
    bool IsFixed(GenericParameter * pParam);

    Location * m_pTypeArgumentLocations;
    GenericBinding * m_pGenericBinding;
    IBitVector * m_pFixedTypeArgumentBitVector;
};

//Represents a pointer to either a BCYM_GenericBinding
//symbol, a BCSYM_GenericTypeBinding symbol,
//or a PartialGenericBinding structure.
//It is designed to allow all three types of
//generic bindings to be represented transparently
//and also contains utilities for converting partial bindings
//into full bindings.
//Keep in mind that this class is used like a pointer.
//This means that it is passed around by value all over the place.
//As a result, you should be very wary about adding new data members to it.
//In fact, JUST DON'T DO IT.
//If you absolutely need to add stuff to this class, then please go through and change the GenericBindingInfo instances into
//GenericBindingInfo * instances and GenericBindingInfo & instance into GenericBindingInfo ** instances.
class GenericBindingInfo
{
public:

    GenericBindingInfo(PartialGenericBinding * pPartialGenericBinding);

    GenericBindingInfo(GenericBinding * pGenericBinding);

    GenericBindingInfo();
    bool IsPartialBinding();

    GenericBindingInfo &operator =(PartialGenericBinding * pPartialGenericBinding);

    GenericBindingInfo &operator =(GenericBinding * pGenericBinding);

    PartialGenericBinding * PPartialGenericBinding(bool throwOnFailure = true);

    BCSYM_GenericBinding * PGenericBinding(bool throwOnFailure = true);

    BCSYM_GenericTypeBinding * PGenericTypeBinding(bool throwOnFailure = true);

    bool IsNull() const;
    bool IsGenericTypeBinding() const;
    unsigned long FreeTypeArgumentCount() const;

    void ApplyExplicitArgumentsToPartialBinding(
        Type * * ppTypeArguments,
        Location * pTypeArgumentLocations,
        unsigned long TypeArgumentCount,
        Semantics * pSemantics,
        Declaration * pGeneric);


    void ConvertToFullBindingIfNecessary(
        Semantics * pSemantics,
        Declaration * pGeneric);

    bool operator == (const GenericBindingInfo & src) const;
    bool operator != (const GenericBindingInfo & src) const;
    bool IsFullMethodBinding();

    BCSYM * GetCorrespondingArgument(GenericParameter * pParam);
    Location * GetTypeArgumentLocations();
    GenericBinding * GetGenericBindingForErrorText();
    IBitVector * GetFixedTypeArgumentBitVector();

    bool WasEverPartial()
    {
        return m_pPartialGenericBinding;
    }

    void SetTypeArgumentLocationsAndOldPartialBinding(
        Location * pTypeArgumentLocations,
        PartialGenericBinding * pOldGenericBinding);

private:
    //before you add data members to this class, please read the warnings up above.

    bool m_isPartialBinding;
    PartialGenericBinding * m_pPartialGenericBinding;
    GenericBinding * m_pGenericBinding;
    Location * m_pTypeArgumentLocations;

    static
    void MergeArguments(
        _In_count_(SourceArgumentCount) Type ** ppSourceTypeArguments,
        _In_count_(SourceArgumentCount) Location * pSourceArgumentLocations,
        unsigned long SourceArgumentCount,
        _Inout_count_(DestArgumentCount) Type ** ppDestTypeArguments,
        _Inout_count_(DestArgumentCount) Location * pDestArgumentLocations,
        _In_ IBitVector * pFixedArgumentBitVector,
        unsigned long DestArgumentCount);

    Type ** CloneTypeArguments(NorlsAllocator * pNorls);

    Location * CloneLocations(NorlsAllocator * pNorls);
};

class BCSYM_ExtensionCallLookupResult :
    public BCSYM
{
    DBGFRIENDS;

public:

    BCSYM_ExtensionCallLookupResult(NorlsAllocator * pAlloc);

    typedef ListValueIterator<ExtensionCallInfo, NorlsAllocWrapper> iterator_type;

    void AddProcedure(
        BCSYM_Proc * pProc,
        unsigned long precedenceLevel,
        PartialGenericBinding * pPartialGenericBinding);

    unsigned long ExtensionMethodCount();
    iterator_type GetExtensionMethods();
    unsigned long GetMaxPrecedenceLevel();
    STRING * GetErrorName(Compiler * pCompiler);
    BCSYM_Proc * GetFirstExtensionMethod();
    PartialGenericBinding * GetPartialGenericBindingForFirstExtensionMethod();
    void ClearExtensionMethods();
    bool CanApplyDefaultPropertyTransformation();
    void SetInstanceMethodResults(Declaration * pLookupResult, GenericBinding * pGenericBinding, Type * pAccessingInstanceType);
    Type * GetAccessingInstanceTypeOfInstanceMethodLookupResult();
    Declaration * GetInstanceMethodLookupResult();
    GenericBinding* GetInstanceMethodLookupGenericBinding();
    bool IsOverloads();

    List<ExtensionCallInfo, NorlsAllocWrapper> * GetList()
    {
        SymbolEntryFunction;
        return &m_list; 
    }

    void SetMaxPrecedenceLevel(unsigned long  value)
    {
        SymbolEntryFunction;
        m_maxPrecedenceLevel = value;
    }

    void SetInstanceMethodLookupResult(Declaration * value)
    {
        SymbolEntryFunction;
        m_pInstanceMethodLookupResult = value;
    }

    void SetInstanceMethodLookupGenericBinding(GenericBinding * value)
    {
        SymbolEntryFunction;
        m_pInstanceMethodLookupGenericBinding = value;
    }

    Type * GetInstanceMethodAccessingInstanceType() const
    {
        SymbolEntryFunction;
        return m_pInstanceMethodAccessingInstanceType;
    }
    void SetInstanceMethodAccessingInstanceType(Type * value)
    {
        SymbolEntryFunction;
        m_pInstanceMethodAccessingInstanceType = value;
    }

private:
    //The list of extension method overload candidates.
    List<ExtensionCallInfo, NorlsAllocWrapper> m_list;

    //The maximum precedence level of all extension methods in m_list
    unsigned long m_maxPrecedenceLevel;

    //The result of instance method lookup for the name used to yeild the extension
    //method candidates stored in m_list. It is stored as a Declaration because it
    //theoretically may be an alias. Sigh. I don't think there is any code that
    //actually does this, but instance method overload resolution currently deals with
    //potentally aliased procedures when finding overloads. To be compatable we
    //need to do the same thing.
    Declaration * m_pInstanceMethodLookupResult;

    //The generic binding context associated with m_pInstanceMethodLookupResult
    GenericBinding * m_pInstanceMethodLookupGenericBinding;

    //The type used to access the instance method lookup result.
    //This is necessary to do accessibility checks during overload resolution
    //when determining applicability (there may be applicable overloads that are
    //not accessible).
    Type * m_pInstanceMethodAccessingInstanceType;

};


/**************************************************************************************************
;Uninitialized

Uninitialized symbol - Apparently the only reason this exists is so you can detect via a Watch when
a symbol is pointing to non-allocated memory since the kind for an Uninitialized symbol is 0
I couldn't find anyone that actually built such a symbol or morphed an existing symbol to be uninitialized
***************************************************************************************************/
class BCSYM_Uninitialized : BCSYM
{
};

/**************************************************************************************************
;Expression

Constant Expression symbol
***************************************************************************************************/
class BCSYM_Expression : public BCSYM
{
    DBGFRIENDS;

public:

    // Returns TRUE if the value in this expression is not valid
    // because of an error during expression evalutation.
    //
    bool IsBadExpression()
    {
        SymbolEntryFunction;
        VSASSERT(IsEvaluated(), "We must be evaluated.");

        return GetValue().TypeCode == t_bad;
    }

    // Returns TRUE if this expression has been evaluated.
    bool IsEvaluated() const
    {
        SymbolEntryFunction;
        return m_fEvaled;
    }

    void SetIsEvaluated(bool fEvaluated)
    {
        SymbolEntryFunction;
        m_fEvaled = fEvaluated;
    }

    bool IsEvaluating() const
    {
        SymbolEntryFunction;
        return m_fEvaluating;
    }

    void SetIsEvaluating(bool fEvaluating)
    {
        SymbolEntryFunction;
        m_fEvaluating = fEvaluating;
    }

    ConstantValue GetValue() const
    {
        SymbolEntryFunction;
        return m_Value;
    }

    void SetValue(ConstantValue NewValue)
    {
        SymbolEntryFunction;
        m_Value = NewValue;
    }

    WCHAR *GetExpressionText()
    {
        SymbolEntryFunction;
        VSASSERT( !IsSyntheticExpressionForEnumMemberIncrement(),
                    "Why is anybody asking for this in this scenario ?");

        return GetExpressionTextRaw();
    }

    WCHAR *GetExpressionTextRaw()
    {
        SymbolEntryFunction;
        return m_wszExpression;
    }

    BCSYM *GetForcedType()
    {
        SymbolEntryFunction;
        return GetForcedTypeRaw()->DigThroughNamedType();
    }
    BCSYM *GetForcedTypeRaw() const
    {
        SymbolEntryFunction;
        return m_ptypForce;
    }

    void SetForcedType(BCSYM *ForcedType)
    {
        SymbolEntryFunction;
        m_ptypForce = ForcedType;
    }

    BCSYM_Expression *GetNext() const
    {
        SymbolEntryFunction;
        return m_pexprNext;
    }
    void SetNext(BCSYM_Expression * value)
    {
        SymbolEntryFunction;
        m_pexprNext = value;
    }

    BCSYM_NamedRoot *GetReferringDeclaration() const
    {
        SymbolEntryFunction;
        return m_pnamedContext;
    }
    void SetReferringDeclaration(BCSYM_NamedRoot * value)
    {
        SymbolEntryFunction;
        m_pnamedContext = value;
    }

    // Have this as separate function because in the future there could be
    // more synthetic contexts for all of which this will need to return true
    //
    bool IsSyntheticExpression() const
    {
        SymbolEntryFunction;
        return m_IsSyntheticExpression || IsSyntheticExpressionForEnumMemberIncrement();
    }

    void SetIsSyntheticExpression()
    {
        SymbolEntryFunction;
        m_IsSyntheticExpression = true;
    }

    bool IsSyntheticExpressionForEnumMemberIncrement() const
    {
        SymbolEntryFunction;
        return m_IsSyntheticExpressionForEnumMemberIncrement;
    }

    void SetPrevousEnumMember(BCSYM_VariableWithValue *member)
    {
        SymbolEntryFunction;
        m_IsSyntheticExpressionForEnumMemberIncrement = true;
        m_pPreviousEnumMemberSymbol = member;
    }

    BCSYM_VariableWithValue *GetPrevousEnumMember()
    {
        SymbolEntryFunction;
        VSASSERT( IsSyntheticExpressionForEnumMemberIncrement(),
                    "Why is anybody asking for this otherwise ?");

        // For Synthesized enum increment expressions, the previous enum member
        // variable is stored here so that it can be used to later synthesize an
        // appropriate expression parsetree.
        //
        // i.e. this can be set only when m_IsSyntheticExpressionForEnumMemberIncrement
        // is true
        return m_pPreviousEnumMemberSymbol;
    }

    WCHAR *GetUserDefinedExpressionText()
    {
        SymbolEntryFunction;
        if (IsSyntheticExpression())
        {
            return NULL;
        }
        else
        {
            return GetExpressionTextRaw();
        }
    }

private:

    // For the list of these guys.
    BCSYM_Expression *m_pexprNext;

    // The context to bind this expression in.  This will point to the member
    // that directly contains this expression (i.e. const x = 4, this will point
    // to the symbol for "x").
    //
    BCSYM_NamedRoot *m_pnamedContext;

    BCSYM             * m_ptypForce;      // force expression to this type if not NULL
                                  // (this type may be unbound or still an alias)

    ConstantValue       m_Value;       // value of the expression

    bool                m_fEvaluating : 1;  // Used to detect circular dependencies.
    bool                m_fEvaled : 1;      // set when it's done

    // set that this expression has been synthesized for an enum increment expression
    // and has special semantics
    //
    bool                m_IsSyntheticExpressionForEnumMemberIncrement : 1;
    bool                m_IsSyntheticExpression : 1;


    union
    {
        // actual text
        //
        // Points to the string for an expression.  This string is not owned
        // by the edit buffer.
        //
        //it seems that this never gets set
#pragma warning (suppress  : 4200)
        WCHAR m_wszExpression[0];

        // This is used when m_IsSyntheticExpressionForEnumMemberIncrement is true
        BCSYM_VariableWithValue *m_pPreviousEnumMemberSymbol;
    };
};


/**************************************************************************************************
;SimpleType

All simple type symbols (not including classes and UDTs) derive from BCSYM_SimpleType
***************************************************************************************************/
class BCSYM_SimpleType : public BCSYM
{
    DBGFRIENDS;

public:
  // The vtype classif  ication of this type.
    Vtypes GetVtype() const
    {
        SymbolEntryFunction;
        return m_vtype;
    }
    void SetVtype(Vtypes value)
    {
        SymbolEntryFunction;
        m_vtype = value;
    }


protected:
    Vtypes m_vtype;
}; // end BCSYM_SimpleType


/**************************************************************************************************
;VoidType

Void simple type
***************************************************************************************************/
class BCSYM_VoidType : public BCSYM_SimpleType
{
    DBGFRIENDS;
public:

    BCSYM_VoidType()
    {
        SetSkKind(SYM_VoidType);
        SetVtype(t_void);
    }
}; // end BCSYM_VoidType


/**************************************************************************************************
;NamedType

An alias with binding semantics.  One of these exists for each non-intrinsic type referenced by a
Basic module's source code.
***************************************************************************************************/

struct NameTypeArguments
{
    friend class Declared;

    Location *GetArgumentLocation(unsigned Index);

    Location *GetAllArgumentsLocation(Location *Loc);

    // The type arguments.
    BCSYM **m_Arguments;

    // The number of arguments.
    unsigned __int32 m_ArgumentCount : 31;

    // bool to indicates if the types for the type arguments
    // are indeed specified. The could be missing for types
    // in gettype expressions. Eg: GetType(C1(Of ,)).
    //
    unsigned __int32 m_TypesForTypeArgumentsSpecified : 1;

    // The location of the generic and its arguments.
    //
    Location m_BindingTextSpan;

    BCSYM_GenericBinding *m_BoundGenericBinding;


protected:

    // Need to keep locations of arguments for primitive types
    // and arrays because we don't have namedtypes for these
    // and so no "use" locations.
    //
    // Note that this is not a one-to-one mapping with m_Arguments
    // and the number of locations are <= m_ArgumentCount
    //
    // Go through access function above
    //
    Location *m_TextSpansForNonNamedTypeArguments;
};

class BCSYM_NamedType : public BCSYM_SimpleType
{
    DBGFRIENDS;
public:

  // The actual type this NamedType represents.  Returns NULL if not yet computed.
    BCSYM *GetSymbol() const
    {
        SymbolEntryFunction;
        return m_ptyp;
    }
    void SetSymbol(BCSYM * value)
    {
        SymbolEntryFunction;
        m_ptyp = value;
    }


  // Get the number of names, including the rightmost.
    unsigned GetNameCount() const
    {
        SymbolEntryFunction;
        return m_NumDotDelimitedNames;
    }
    void SetNameCount(unsigned value)
    {
        SymbolEntryFunction;
        m_NumDotDelimitedNames = value;
    }

  // Get the next named type symbol in its module's named type list.
    BCSYM_NamedType * GetNext() const
    {
        SymbolEntryFunction;
        return m_pnamtypNext;
    }
    void  SetNext(BCSYM_NamedType * value)
    {
        SymbolEntryFunction;
        m_pnamtypNext = value;
    }

    STRING **GetNameArray() const
    {
        SymbolEntryFunction;
        return m_DotDelimitedNames;
    }
    void SetNameArray(_In_ STRING ** value)
    {
        SymbolEntryFunction;
        m_DotDelimitedNames = value;
    }

    NameTypeArguments **GetArgumentsArray() const
    {
        SymbolEntryFunction;
        return m_ppArguments;
    }
    void SetArgumentsArray(NameTypeArguments ** value)
    {
        SymbolEntryFunction;
        m_ppArguments = value;
    }

  // Get the name used for hashing the type into a TypeHashTable
    STRING *GetHashingName()
    {
        SymbolEntryFunction;
        return GetNameArray()[GetNameCount() - 1];
    }

    BCSYM_NamedRoot *GetContext() const
    {
        SymbolEntryFunction;
        return m_pnamedContext;
    }
    void SetContext(BCSYM_NamedRoot * value)
    {
        SymbolEntryFunction;
        m_pnamedContext = value;
    }

    bool IsAttributeName() const
    {
        SymbolEntryFunction;
        return m_IsAttributeName;
    }

    void SetIsGlobalNameSpaceBased(bool IsGlobalNameSpaceBased)
    {
        SymbolEntryFunction;
        m_IsGlobalNameSpaceBased = IsGlobalNameSpaceBased;
    }

    bool IsGlobalNameSpaceBased() const
    {
        SymbolEntryFunction;
       return m_IsGlobalNameSpaceBased;
    }


    void SetIsAttributeName(bool IsAttributeName)
    {
        SymbolEntryFunction;
        m_IsAttributeName = IsAttributeName;
    }

    bool IsTHISType() const
    {
        SymbolEntryFunction;
        return m_IsTHISType;
    }
    void SetIsTHISType(bool value)
    {
        SymbolEntryFunction;
        m_IsTHISType = value;
    }

    bool IsTypeArgumentForGenericType() const
    {
        SymbolEntryFunction;
        return m_IsTypeArgumentForGenericType;
    }
    void SetIsTypeArgumentForGenericType(bool value)
    {
        SymbolEntryFunction;
        m_IsTypeArgumentForGenericType = value;
    }

    STRING ** GetFirstDotDelimitedNamesPtr() 
    {
        SymbolEntryFunction;
        return &m_FirstDotDelimitedNames;
    }
    void SetFirstDotDelimitedNames(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_FirstDotDelimitedNames = value;
    }

    bool IsUsedInAppliedAttributeContext() const
    {
        SymbolEntryFunction;
        return m_IsUsedInAppliedAttributeContext;
    }

    void SetIsUsedInAppliedAttributeContext(bool IsUsedInAppliedAttribute)
    {
        SymbolEntryFunction;
        m_IsUsedInAppliedAttributeContext = IsUsedInAppliedAttribute;
    }


protected:

  // The cached symbol.  NULL if not yet bound.
    BCSYM * m_ptyp;

  // The next named type in its module's list of named types.
    BCSYM_NamedType *m_pnamtypNext;

    // The number of names in the typed type, e.g. SomeNamespace.Type = 2
    unsigned __int32 m_NumDotDelimitedNames:26;

    // Is this the name of a custom attribute?
    unsigned __int32 m_IsAttributeName:1;

    // Is this type used in the context of an applied attribute ?
    // Eg: <MyAttr(GetType(Type1))> Class C1 'MyAttr and Type1 are both used in the application of the attribute MyAttr.
    unsigned __int32 m_IsUsedInAppliedAttributeContext:1;

    // Is this a GlobalNameSpace based symbol, i.e. "GlobalNameSpace.Bar.Foo"
    unsigned __int32 m_IsGlobalNameSpaceBased:1;

    // NamedType corresponding to the THIS type - it is the same as the ClassOrInterface for
    // non-generic types, but is the open binding corresponding to the ClassOrInterface
    // when the ClassOrInterface is either generic or is nested in a generic type.
    //
    // For non-generic and generic cases, for any kind of type that can be possibly be a partial
    // type (Class or Structure), this named type type is needed so that this can be bound to the
    // Main Type corresponding to the partial type during bindable.
    //
    // For generic cases, for any type that is enclosed in a possibly partial type (Class Or Structure),
    // this is needed.
    //
    unsigned __int32 m_IsTHISType: 1;

    // Is this a type argument to a generic type ?
    unsigned __int32 m_IsTypeArgumentForGenericType: 1;

    // Context to bind the type in.
    BCSYM_NamedRoot *m_pnamedContext;

    // The generic arguments for the names. Null if no name is qualified with generic arguments.
    NameTypeArguments ** m_ppArguments;

    // The named type identifiers.
    STRING ** m_DotDelimitedNames;
    STRING * m_FirstDotDelimitedNames;
}; // end of BCSYM_NameType

/**************************************************************************************************
;PointerType

Represents a pointer (e.g. the type of a byref param)
***************************************************************************************************/
class BCSYM_PointerType : public BCSYM_SimpleType
{
    DBGFRIENDS;
public:
    BCSYM *GetRawRoot() const
    {
        SymbolEntryFunction;
        return m_ptypRoot;
    }
    void SetRoot(BCSYM * value)
    {
        SymbolEntryFunction;
        m_ptypRoot = value;
    }

    BCSYM *GetRoot()
    {
        SymbolEntryFunction;
        return GetRawRoot()->DigThroughNamedType();
    }

    BCSYM *GetCompilerRoot();

    // Is this a pointer to a variant?

    // This constructor is only used when creating the static versions of this
    // type for the intrinsic types.
    BCSYM_PointerType(const BCSYM *ptyp)
    {
        SymbolEntryFunction;
        SetSkKind(SYM_PointerType);
        SetVtype(t_ptr);
        m_ptypRoot = (BCSYM *)ptyp;
    }

    BCSYM_PointerType()
    {
        SymbolEntryFunction;
    }

    STRING *GetErrorName(Compiler *pCompiler)
    {
        SymbolEntryFunction;
        return GetRawRoot()->GetErrorName(pCompiler);
    }

protected:

    BCSYM * m_ptypRoot;
}; // end BCSYM_PointerType


const unsigned ArrayRankLimit = 32; // COM+ limits array types to 32 dimensions.

/**************************************************************************************************
;ArrayType

The new zero-based array type in VB7.
***************************************************************************************************/
class BCSYM_ArrayType : public BCSYM_SimpleType
{
    DBGFRIENDS;
public:

    BCSYM *GetRoot()
    {
        SymbolEntryFunction;
        return GetRawRoot()->DigThroughNamedType();
    }

    BCSYM *GetRawRoot() const
    {
        SymbolEntryFunction;
        return m_ElementType;
    }

    void SetElementType(BCSYM * value)
    {
        SymbolEntryFunction;
        m_ElementType = value;
    }

    unsigned GetRank() const
    {
        SymbolEntryFunction;
        return m_Rank;
    }
    void SetRank(unsigned value)
    {
        SymbolEntryFunction;
        m_Rank = value;
    }

    STRING *GetErrorName(Compiler *pCompiler)
    {
        SymbolEntryFunction;
        return GetRawRoot()->GetErrorName(pCompiler);
    }

protected:

    BCSYM *m_ElementType;
    unsigned m_Rank; // the number of dimensions in the array
}; // end BCSYM_ArrayType

class BCSYM_ArrayLiteralType : public BCSYM_ArrayType
{
    DBGFRIENDS;
public:
    BCSYM_ArrayLiteralType(const NorlsAllocWrapper & alloc);
    ConstIterator<ILTree::Expression *> GetElements();
    void AddElement(ILTree::Expression * pElement);
    BCSYM_ArrayType * GetCorrespondingArrayType();
    void SetCorrespondingArrayType(BCSYM_ArrayType * pArrayType);    
    void SetLiteralLocation(const Location & loc);
    const Location & GetLiteralLocation();
    unsigned long GetElementCount()  const;
private:
    //A list of the element types of the original array literal.
    //For a multi-dimensional array this list will be flat, and will not reflect the
    //nesting structure of the literal.
    List<ILTree::Expression *, NorlsAllocWrapper> m_elements;
    
    BCSYM_ArrayType * m_pRealArrayType;
    Location m_literalLocation;
};

/**************************************************************************************************
;NamedRoot

All named symbols derive from this
***************************************************************************************************/
class BCSYM_NamedRoot : public BCSYM
{
    DBGFRIENDS;
public:

    // Get the containing compiler.
    Compiler *GetCompiler();

    // Get corresponding CompilerHost.
    CompilerHost *GetCompilerHost();

    // Get the module containing this symbol.
    BCSYM_Class *GetContainingClass();

    // Get the namespace containing this symbol.
    BCSYM_Namespace *GetContainingNamespace();

    // Get the project containing this symbol.  May be NULL.
    CompilerProject *GetContainingProject();

    // Get the class or interface that contains this symbol.
    BCSYM_Container *GetContainingClassOrInterface();

    // Gets the containing compiler file (basic or metadata).
    CompilerFile *GetContainingCompilerFile();

    // Same as GetContainingCompilerFile, but more consistent
    // naming with other parts of the compiler.
    //
    // Get the compilerfile that holds this symbol.
    CompilerFile *GetCompilerFile()
    {
        SymbolEntryFunction;
        return GetContainingCompilerFile();
    }

    // Gets the containing source file if this is a basic thing.
    SourceFile *GetContainingSourceFileIfBasic();

    // Same as GetContainingSourceFileIfBasic, but more consistent
    // naming with other parts of the compiler.
    //
    // Get the source file that holds this symbol.
    SourceFile *GetSourceFile()
    {
        SymbolEntryFunction;
        return GetContainingSourceFileIfBasic();
    }

    // Returns the event declaration that was responsible for synthetically creating this symbol
    BCSYM_EventDecl *CreatedByEventDecl() const
    {
        SymbolEntryFunction;
        return m_EventDeclThatCreatedSymbol;
    }

    // Keep track of the Event that created this symbol
    void SetEventThatCreatedSymbol( BCSYM_EventDecl *Event )
    {
        SymbolEntryFunction;
        m_EventDeclThatCreatedSymbol = Event;
    }

    BCSYM_Variable *CreatedByWithEventsDecl() const;

    void SetMemberThatCreatedSymbol( BCSYM_Member *pMember )
    {
        SymbolEntryFunction;
        m_memberThatCreatedSymbol = pMember;
    }

    BCSYM_Property *GetAutoPropertyThatCreatedSymbol();

    // Get the namespace for this container.
    STRING *GetNameSpace() const
    {
        SymbolEntryFunction;
        return m_pstrNameSpace;
    }
    void SetNameSpace(_In_z_ STRING * value)
    {
        m_pstrNameSpace = value;
    }

    // This symbol's name.
    STRING *GetName() const
    {
        SymbolEntryFunction;
        return m_pstrName;
    }

    STRING *GetIDEName(Compiler *pCompiler);

    void SetName(_In_z_ STRING *pstrName)
    {
        SymbolEntryFunction;
        m_pstrName = pstrName;
    }

    STRING *GetUntransformedName() const
    {
        SymbolEntryFunction;
        return m_pstrUntransformedName;
    }

    void SetUntransformedName(_In_z_ STRING *name)
    {
        SymbolEntryFunction;
        m_pstrUntransformedName = name;
    }

  private:

    void GetQualifiedNameHelper(bool fMakeSafeName,
                                BCSYM_Container *pContextContainer,
                                bool fUseCLRName,
                                bool fAppendGenericArguments,
                                BCSYM_GenericBinding *pGenericBindingContext,
                                _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames,
                                size_t cParameterNames,
                                bool fMakeTypeArgumentsGlobal,
                                bool fExpandNullable,
                                bool fForceQualificationOfIntrinsicTypes,
                                _Deref_opt_out_opt_z_ STRING **ppstr,          // One of ppstr and psb should be non-NULL
                                StringBuffer *psb,
                                bool fMinimallyQualify,
                                bool fUseUntransformedName = false);

  public:
    // The qualified name of a symbol
    STRING *GetQualifiedName(bool fMakeSafeName = true,
                             BCSYM_Container *pContextContainer = NULL,
                             bool fAppendGenericArguments = false,
                             BCSYM_GenericBinding *pGenericBindingContext = NULL,
                             bool fMinimallyQualify = false,
                             _In_opt_count_(cParameterNames)  STRING **pGenericMethodParameterNames = NULL,
                             size_t cParameterNames = 0,
                             bool fMakeTypeArgumentsGlobal = false,
                             bool fExpandNullable = false,
                             bool fForceQualificationOfIntrinsicTypes = false);

    void GetQualifiedName(StringBuffer *psb,
                          bool fMakeSafeName = true,
                          BCSYM_Container *pContextContainer = NULL,
                          bool fAppendGenericArguments = false,
                          BCSYM_GenericBinding *pGenericBindingContext = NULL,
                          _In_opt_count_(cParameterNames) STRING **pGenericMethodParameterNames = NULL,
                          size_t cParameterNames = 0,
                          bool fMakeTypeArgumentsGlobal = false);

    STRING *GetQualifiedEmittedName(BCSYM_Container *pContextContainer = NULL, bool fUseUntransformedName = false);

    void GetQualifiedEmittedName(StringBuffer *psb, BCSYM_Container *pContextContainer = NULL);

    static
    void AppendQualifiedTypeName(
        StringBuffer &Name,
        BCSYM * pType,
        bool fMakeSafeName,
        BCSYM_Container * pContextContainer,
        bool fUseCLRName,
        _In_opt_count_(cParameterNames)STRING * * pGenericMethodParameterNames,
        size_t cParameterNames,
        bool fMakeTypeArgumentsGlobal,
        bool fExpandNullable,
        bool fForceQualificationOfIntrinsicTypes,
        bool fMinimallyQualify);

#if IDE 
    STRING * GetSimpleName(
        bool fAppendGenericArguments = false,
        BCSYM_GenericBinding * pGenericBinding = NULL,
        bool fMakeSafeName = false,
        bool fAfterDot = false,
        IReadonlyBitVector * pGenericParamsToExclude = NULL);

    STRING * GetDisplayName(
        bool fAppendGenericArguments = false,
        BCSYM_GenericBinding * pGenericBinding = NULL,
        bool fMakeSafeName = false,
        bool fAfterDot = false,
        bool fQualified = false);
#endif

    // Get the signature that's needed to get the XML Doc String
    STRING * GetDocCommentSignature(
        Compiler * pCompiler,
        _In_opt_z_ STRING * pstrEventHandlerName = NULL,
        BCSYM_NamedRoot * pParent = NULL);

    // The name this symbol is emitted into IL as.
    STRING * GetEmittedName() const
    {
        SymbolEntryFunction;
        return m_pstrEmittedName;
    }
    void SetEmittedName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrEmittedName = value;
    }

    // This symbol's parent symbol.  Only NULL for projects and unnamed namespaces.
    // If the parent symbol is done with partials, then once partials have been
    // merged (so their MainType is known), GetParent() will always return the MainType.
    // (compare with GetPhysicalContainer(), which returns the partial that contains this symbol).
    BCSYM_NamedRoot *GetParent()
    {
        SymbolEntryFunction;
        if (GetImmediateParent())
        {
            BCSYM_NamedRoot *pParent = GetImmediateParent();

            while (pParent && (pParent->IsHash() || pParent->IsAlias()))
                pParent = pParent->GetImmediateParent();
            return pParent;
        }

        return NULL;
    }

    // The symbol that 'owns' this one
    BCSYM_NamedRoot *GetImmediateParent() const
    {
        SymbolEntryFunction;
        return m_pnamedParent;
    }
    void SetImmediateParent(BCSYM_NamedRoot * value)
    {
        SymbolEntryFunction;
        m_pnamedParent = value;
    }

    BCSYM_Container *GetPhysicalContainer();


#if IDE 
    BCSYM_NamedRoot * GetImmediateParentOrBuild();
#else
#define GetImmediateParentOrBuild GetImmediateParent
#endif

    // Get the nearest parent that is a BCSYM_Container symbol.
    BCSYM_Container *GetContainer()
    {
        SymbolEntryFunction;
        BCSYM_NamedRoot *pnamed = this->GetImmediateParentOrBuild();

        while (pnamed != NULL && !pnamed->IsContainer())
        {
            pnamed = pnamed->GetParent();
        }

        return pnamed->PContainer(); // yes, this works if pnamed is NULL.
    }

    BCSYM_NamedRoot *GetNextInSymbolList() const
    {
        SymbolEntryFunction;
        return m_psymNext;
    }
    void SetNextInSymbolList(BCSYM_NamedRoot * value)
    {
        SymbolEntryFunction;
        m_psymNext = value;
    }

    BCSYM_NamedRoot *GetContainerOrContainingMethod()
    {
        SymbolEntryFunction;
        BCSYM_NamedRoot *pnamed = this->GetImmediateParentOrBuild();

        while (pnamed != NULL && !pnamed->IsContainer() && !pnamed->IsProc())
        {
            pnamed = pnamed->GetParent();
        }

        return pnamed;
    }

    // Get the next symbol of the same name in the symbol list.
    BCSYM_NamedRoot *GetNextBound();

    // Get the next symbol in the overload list.
    // This is strictly for use by the binder to do overload resolution.
    // If anyone else uses this function for any reason, it's your fault
    // when I change this function and your code breaks.  So there.
    BCSYM_NamedRoot *GetNextOverload();

    // This is similar to GetNextOverload(), but it doesn't discriminate
    // based on binding space.  I needed a quick way to wind through a hash
    // looking for things of the same name, but not necessairly the same symbol type.
    BCSYM_NamedRoot *GetNextOfSameName();

    BCSYM_NamedRoot *GetNextOfSameName(unsigned BindspaceMask);

    // Returns true if pnamedMaybeContainer is an ancestor of this.
    bool IsContainedBy(BCSYM_NamedRoot *pnamedMaybeContainer);

    // Finds a member of this name.
    BCSYM_NamedRoot *SimpleBind(Symbols *psymbols, _In_z_ STRING *pstrName);

    // The visibility of the symbol (public, friend, protected, or private).
    // For partial types, this is the access of the main type.
    ACCESS GetAccess();

    // The visibility of the symbol (public, friend, protected, or private) irrespective of what
    // is on the other partial types in the partial type case.
    ACCESS GetRawAccess() const
    {
        SymbolEntryFunction;
        return (ACCESS)m_access;
    }

    void SetAccess(ACCESS access)
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                        "Partial type unexpected!!!");

        m_access = access;
    }

    ACCESS GetAccessConsideringAssemblyContext(BCSYM_Container * ContainerContext);

    // The contained visibility of the symbol. It's publicness might be
    // hidden by a container. The out param is the lowest non-public symbol
    // --which could be itself.
    // Takes into account potential Friendness
    bool IsPublicFromAssembly(
        bool ShouldConsiderFriendness,
        BCSYM_NamedRoot * * ppNonPublic);

    // Is this symbol private from within the project?
    bool IsPrivateInternal()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                        "Partial type unexpected!!!");

        return GetAccess() == ACCESS_Private;
    }

    // Be careful when using this function. With the introduction of FriendAssemblies, we will consider
    // the presence of any "InternalsVisibleTo" attribute as a reason to expose "Friend members".

    bool IsOrCouldBePublic();

    bool IsTruePublic()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                        "Partial type unexpected!!!");

        switch (GetAccess())
        {
            case ACCESS_Public:
            case ACCESS_Protected:
            case ACCESS_ProtectedFriend:
                return true;
        }

        return false;
    }

    // Should the IDE acknowldge this symbol?
    bool IsHiddenInEditor(
        BCSYM_NamedRoot * pParent = NULL,
        CompilerProject * pProject = NULL);

    // This is internal to the compiler
    bool IsHidden();

    // Should the IDE hide this "EditorBrowsable(advanced)" symbol?
    bool IsAdvanced();

    // Should the IDE hide this "EditorBrowsable(Never)" symbol?
    bool IsBrowsableNever();

    // Gets whether EditorBrowsable attribute has explicit Always specification.
    bool IsBrowsableAlways();

    // Should the IDE treat this symbol as obsolete?
    bool IsObsolete();

    // Change the hidden bit
    void SetIsHidden(bool IsHidden)
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        m_isHidden = IsHidden;
    }
    //Get IsHidden bit
    bool IsHiddenRaw() const
    {
        SymbolEntryFunction;
        return m_isHidden;
    }

    bool IsShadowsKeywordUsed() 
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType() || 
                    m_IsShadowsKeywordUsed == false,
                    "Partial type unexpected!!!");

        return m_IsShadowsKeywordUsed;
    }
    void SetShadowsKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");
        m_IsShadowsKeywordUsed = value;
    }

    // Some methods on interfaces are 'hidden' (such as _VtblGap8()) and shouldn't require implementation when interface is implemented
    bool DoesntRequireImplementation()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return m_DoesntRequireImplementation;
    }
    void SetDoesntRequireImplementation(bool value)
    {
        SymbolEntryFunction;
        m_DoesntRequireImplementation = value;
    }

    // Did the compiler determine that this symbol is shadowing something?
    bool IsShadowing() 
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return m_IsShadowing;
    }

    // Note that this symbol is shadowing something - the compiler figures this out
    void SetIsShadowing()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        m_IsShadowing = true;
    }

    void ClearIsShadowing()
    {
        SymbolEntryFunction;
        m_IsShadowing = false;
    }

    // Has a special Com+ name
    bool IsSpecialName() const
    {
        SymbolEntryFunction;
        return m_isSpecialName;
    }
    void SetIsSpecialName(bool value)
    {
        SymbolEntryFunction;
        m_isSpecialName = value;
    }

    // The binding namespaces (normal, type, or conditional comp) in which
    // this symbol can be found.
    // Was namepace until COM+ usurped the term
    BindspaceType GetBindingSpace()
    {
        SymbolEntryFunction;
        return (BindspaceType)m_BindingSpace;
    }

    void SetBindingSpace(BindspaceType bs)
    {
        SymbolEntryFunction;
        m_BindingSpace = bs;
    }

    //  Is this a local?
    bool IsLocal();

    // The metadata token for this item.
    mdToken GetToken() const
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        if(!IsTypeForwarder())
        {
            return m_tk_DoNotUseDirectly;
        }

        return mdTokenNil;
    }

    // The metadata token for the item on its related com class interface
    mdToken GetComClassToken() const
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        if(!IsTypeForwarder())
        {
            return m_tkComClass_DoNotUseDirectly;
        }

        return mdTokenNil;
    }

    AttrVals *GetPAttrVals() const
    {
        SymbolEntryFunction;
        if(!IsTypeForwarder())
        {
            return m_pattrvals_DoNotUseDirectly; // can be NULL
        }

        return NULL;
    }

    WellKnownAttrVals *GetPWellKnownAttrVals();

    bool IsMethodCodeTypeRuntime() const
    {
        SymbolEntryFunction;
        return m_IsMethodCodeTypeRuntime;
    }
    void SetIsMethodCodeTypeRuntime(bool value)
    {
        SymbolEntryFunction;
        m_IsMethodCodeTypeRuntime = value;
    }

    bool IsBadNamedRoot() const
    {
        SymbolEntryFunction;
        return m_isBad;
    }

    void SetIsBad(SourceFile *pSourceFile);
    void SetIsBad();

    void SetIsBadRaw(bool value)
    {
        SymbolEntryFunction;
        m_isBad = value;
    }

    ERRID GetErrid() 
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        return m_errid;
    }

    void SetErrid(ERRID errid)
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        m_errid = errid;
    }

    void SetBadName(_In_z_ STRING *pstrBadName)
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        VSASSERT(m_pstrBadName == NULL, "Already has a bad name!");
        m_pstrBadName = pstrBadName;
    }

    STRING *GetBadName()
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        return m_pstrBadName;
    }

    void SetBadNameSpace(_In_z_ STRING *pstrBadNameSpace)
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        VSASSERT(GetBadNameSpace() == NULL, "Already has a bad namespace!");
        m_pstrBadNameSpace = pstrBadNameSpace;
    }

    STRING *GetBadNameSpace() const
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        return m_pstrBadNameSpace;
    }

    void SetBadExtra(_In_z_ STRING *pstrBadExtra)
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        VSASSERT(m_pstrBadExtra == NULL, "Already has a bad extra!");
        m_pstrBadExtra = pstrBadExtra;
    }

    STRING *GetBadExtra() const
    {
        SymbolEntryFunction;
        VSASSERT(IsBad(), "must be bad!");
        return m_pstrBadExtra;
    }

    void SetXMLDocNode(XMLDocNode *pXMLDocNode)
    {
        SymbolEntryFunction;
        m_pXMLDocNode = pXMLDocNode;
    }

    XMLDocNode *GetXMLDocNode() const
    {
        SymbolEntryFunction;
        return m_pXMLDocNode;
    }

    void ReportError(
        Compiler * pCompiler,
        ErrorTable * perror,
        Location * ploc);

    void ResetBadness(CompilationState State)
    {
        SymbolEntryFunction;
        if (GetStateForBadness() >= State)
        {
            SetIsBadRaw(false);
            SetStateForBadness(CS_NoState);
        }
    }

    unsigned __int8 GetStateForBadness() const
    {
        SymbolEntryFunction;
        return m_StateForBadness;
    }
    void SetStateForBadness(unsigned __int8 value)
    {
        SymbolEntryFunction;
        m_StateForBadness = value;
    }

    BCSYM_Container* IsPartialTypeAndHasMainType() const;

    ErrorTable *GetErrorTableForContext();

    bool HasGenericParent();

    bool IsTransient() const
    {
        SymbolEntryFunction;
        return m_IsTransient;
    }

    void SetIsTransient()
    {
        SymbolEntryFunction;
        m_IsTransient = true;
    }

    // Nobody should use this directly unless they take partial types
    // into account. Always try to use the iterators or other accessors
    // like simplebind, getnext, etc.
    //
    // Next symbol in the hash bucket this symbol has been put in
    BCSYM_NamedRoot *GetNextInHash()
    {
        SymbolEntryFunction;
        return GetNextInSymbolList();
    }

    void SetToken(mdToken tk)
    {
        SymbolEntryFunction;
        AssertIfTrue(IsTypeForwarder());

        if(!IsTypeForwarder())
        {
            m_tk_DoNotUseDirectly = tk;
        }
    }

    void SetComClassToken(mdToken tkComClass)
    {
        SymbolEntryFunction;
        AssertIfTrue(IsTypeForwarder());

        if(!IsTypeForwarder())
        {
            m_tkComClass_DoNotUseDirectly = tkComClass;
        }
    }

    void SetPAttrVals(AttrVals * pattrvals)
    {
        SymbolEntryFunction;
        AssertIfTrue(IsTypeForwarder());

        if(!IsTypeForwarder())
        {
            m_pattrvals_DoNotUseDirectly = pattrvals;
        }
    }

    unsigned __int8 AreAttributesEmitted() const
    {
        SymbolEntryFunction;
        return m_AreAttributesEmitted;
    }

    void SetAttributesEmitted(unsigned __int8 value)
    {
        SymbolEntryFunction;
        m_AreAttributesEmitted = value;
    }

    STRING * GetCachedQualifiedName() const
    {
        SymbolEntryFunction;
        return m_pstrCachedQualifiedName;
    }
    void SetCachedQualifiedName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrCachedQualifiedName = value;
    }

    BCSYM_Container* GetCachedContainer() const
    {
        SymbolEntryFunction;
        return m_pCachedContainer;
    }
    void SetCachedContainer(BCSYM_Container* value)
    {
        SymbolEntryFunction;
        m_pCachedContainer = value;
    }

    bool IsCachedSafeName() const
    {
        SymbolEntryFunction;
        return m_fCachedSafeName;
    }
    void SetCachedSafeName(bool value)
    {
        SymbolEntryFunction;
        m_fCachedSafeName = value;
    }

    bool IsCachedCLRName() const
    {
        SymbolEntryFunction;
        return m_fCachedCLRName;
    }
    void SetCachedCLRName(bool value)
    {
        SymbolEntryFunction;
        m_fCachedCLRName = value;
    }

    bool IsCachedAppendGenericArguments() const
    {
        SymbolEntryFunction;
        return m_fCachedAppendGenericArguments;
    }
    void SetCachedAppendGenericArguments(bool value)
    {
        SymbolEntryFunction;
        m_fCachedAppendGenericArguments = value;
    }

    bool IsCachedQualifyIntrinsicTypes() const
    {
        SymbolEntryFunction;
        return m_fCachedQualifyIntrinsicTypes;
    }
    void SetCachedQualifyIntrinsicTypes(bool value)
    {
        SymbolEntryFunction;
        m_fCachedQualifyIntrinsicTypes = value;
    }

protected:

    // This symbol's name.
    STRING * m_pstrName;

    // The name of this symbol will be emitted with.
    STRING *m_pstrEmittedName;

    // The namespace that contains this.
    STRING * m_pstrNameSpace;

    // The untransformed name of a WinRT type;
    STRING * m_pstrUntransformedName;

    // This symbol's parent symbol.  Only NULL for projects and unnamed namespaces.
    BCSYM_NamedRoot *m_pnamedParent;

    union
    {
        // DO NOT ACCESS THIS FIELDS DIRECTLY !!!
        // USE ACCESSORS BELOW !!!
        struct
        {
            // The metadata token.
            mdToken m_tk_DoNotUseDirectly;

            // The metadata token for a com class member
            mdToken m_tkComClass_DoNotUseDirectly;

            // Any applied custom attributes (and their decoded values) get hung here
            AttrVals *m_pattrvals_DoNotUseDirectly;
        };

        // DO NOT ACCESS THIS FIELDS DIRECTLY !!!
        // USE ACCESSORS IN BCSYM_TypeForwarder !!!
        struct
        {
            // Fields used for BCSYM_TypeForwarder

            mdAssemblyRef m_tkAssemblyRefForwardedTo_DoNotUseDirectly;
            BCSYM *m_TypeForwardedTo_DoNotUseDirectly;

            unsigned __int8 m_IsTypeForwarderBeingResolved_DoNotUseDirectly : 1;
            unsigned __int8 m_IsTypeForwarderResolved_DoNotUseDirectly : 1;
        };
    };

    // For bad symbols
    ERRID m_errid;
    STRING * m_pstrBadName;
    STRING * m_pstrBadNameSpace;
    STRING * m_pstrBadExtra;

    // If this symbol was created for an Event, this is the Event declaration that created us
    BCSYM_EventDecl * m_EventDeclThatCreatedSymbol;

    // If this symbol was created for a WithEvents variable or automatic property, 
    // this is the property or variable that created us
    BCSYM_Member *m_memberThatCreatedSymbol;

    // 


    STRING * m_pstrCachedQualifiedName;
    BCSYM_Container* m_pCachedContainer;

    // XMLDocumentation associated with this NamedRoot symbol.
    XMLDocNode *m_pXMLDocNode;

    unsigned __int8 /* CompilationState */ m_StateForBadness;

    // The visibility of the symbol (public, friend, protected, private or compiler)
    unsigned __int8 /* ACCESS */ m_access : MAXACCESSBITS;

    // The binding namespaces (normal, type, or conditional comp) in which
    // this symbol can be found.
    // Was namepace until COM+ usurped the term
    unsigned __int8 /* BindspaceType */ m_BindingSpace : MAXBINDINGBITS;

    // Whether the UI should expose this member to the user.  This could
    // be set if this is a member defined in hidden code with a name
    // that starts with an underscore or if it's an alias that
    // only exists to extrude a member into another scope.
    unsigned __int8 m_isHidden : 1;

    // Whether this is a special com+ name
    unsigned __int8 m_isSpecialName : 1;

    // This symbol is bad
    unsigned __int8 m_isBad : 1;

    // Did the user put the Shadows keyword in front on this Does thi
    unsigned __int8 m_IsShadowsKeywordUsed : 1;

    // Does this symbol shadow something?
    unsigned __int8 m_IsShadowing : 1;

    bool m_fCachedSafeName : 1;
    bool m_fCachedCLRName : 1;
    bool m_fCachedAppendGenericArguments : 1;
    bool m_fCachedQualifyIntrinsicTypes : 1;

    // Some methods on interfaces don't require implementation
    unsigned __int8 m_DoesntRequireImplementation : 1;
    
    // Have the attributes for this symbol already been emitted ?
    unsigned __int8 m_AreAttributesEmitted : 1;

    // Is this a transient symbol generated during method body analysis ?
    unsigned __int8 m_IsTransient : 1;

    // This is to indigate that the generated method is managed by the runtime so doesn't need a body.
    unsigned __int8 m_IsMethodCodeTypeRuntime : 1;

    //  There are 3 free bits left. If you need more, pack one of the other
    // __int8 fields into a bitfield. We are currently using m_BindingSpace
    // and m_access.

private:
    // Next symbol in a list of symbols
    BCSYM_NamedRoot *m_psymNext;

}; // end of BCSYM_NamedRoot

// 


/**************************************************************************************************
;GenericBadNamedRoot

Represents a Bad Symbol
***************************************************************************************************/
class BCSYM_GenericBadNamedRoot : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:

    BCSYM_GenericBadNamedRoot()
    {
        SetSkKind(SYM_GenericBadNamedRoot);
        SetIsBadRaw(true);
        SetStateForBadness(CS_NoState);
        SetErrid(ERRID_None);
        SetName(NULL);
        SetBadName(NULL);
        SetBadNameSpace(NULL);
        SetBadExtra(NULL);
        SetAccess(ACCESS_Public);
    }
};

/**************************************************************************************************
;Hash

A symbol that refers to another symbol.
***************************************************************************************************/
class BCSYM_Hash : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:
    bool IsLocalsHash()
    {
        SymbolEntryFunction;
        
        return (GetImmediateParent() &&  
                    (GetImmediateParent()->IsHash() || GetImmediateParent()->IsMethodDecl())) || 
                m_IsLambdaHash;
    }

    // Returns the first BCSYM_NamedRoot with a name of pstrName
    BCSYM_NamedRoot *SimpleBind
    (
        _In_z_ const STRING *pstrName,              // [in] name of symbol to bind
        SimpleBindFlag flags = SIMPLEBIND_Default
    );

    // Don't use this directly unless you handle partial types. These need to be
    // kept public because a global function __CbBciterChildSafe need to access
    // them. Also lots of other classes will want to use these for locals hashes.
    //
    // The total number of symbols in this table and the tables it is linked to.
    //
    unsigned CSymbols() 
    { 
        SymbolEntryFunction;
        return m_cSymbols; 
    }
    void SetCSymbols(unsigned value) 
    { 
        SymbolEntryFunction;
        m_cSymbols = value; 
    }

    unsigned CBuckets() 
    { 
        SymbolEntryFunction;
        return m_cBuckets; 
    }
    void SetCBuckets(unsigned value) 
    { 
        SymbolEntryFunction;
        m_cBuckets = value; 
    }

    bool IsUnBindableChildrenHash() const
    {
        SymbolEntryFunction;
        return m_IsUnBindableChildrenHash;
    }

    void SetIsUnBindableChildrenHash() 
    {
        SymbolEntryFunction;
        m_IsUnBindableChildrenHash = true;
    }

    BCSYM_Hash* GetPrevHash() const
    {
        SymbolEntryFunction;
        return m_psymPrevHash;
    }

    void SetPrevHash(BCSYM_Hash *PrevHash)
    {
        SymbolEntryFunction;
        VSASSERT( !GetPrevHash() || GetPrevHash()->IsHash(),
                        "How can a hash be linked to anything else besides a hash ?");

        m_psymPrevHash = PrevHash;
    }

    BCSYM_Hash* GetNextHash()
    {
        SymbolEntryFunction;
        VSASSERT( !m_psymNextHash || m_psymNextHash->IsHash(),
                        "How can a hash be linked to anything else besides a hash ?");

        return m_psymNextHash->PHash();
    }
    BCSYM_Hash* GetNextHashRaw() const
    {
        SymbolEntryFunction;
        return m_psymNextHash;
    }

    void SetNextHash(BCSYM_Hash *NextHash)
    {
        SymbolEntryFunction;
        m_psymNextHash = NextHash;
    }

    // For partial types, this is the main component of the type that this
    // hash belongs to.
    //
    BCSYM_Container* GetLogicalContainer()
    {
        SymbolEntryFunction;
        return this->GetContainer();
    }

    void SetLogicalContainer(BCSYM_Container *Container)
    {
        SymbolEntryFunction;
        VSASSERT( !GetImmediateParent() ||
                  GetImmediateParent()->IsContainer(),
                        "Why is a BCSYM_Hash whose parent is not a Container being changed to be a Container ?");

        // Need the cast because the BCSYM_Container is not yet defined by this
        // time and we want to keep the definition for this accessor in the header
        // file for possible inlining.
        //
        SetImmediateParent((BCSYM_NamedRoot *)Container);
    }

    bool GetIsLambdaHash()
    {
        SymbolEntryFunction;
        return m_IsLambdaHash;
    }

    void SetIsLambdaHash(bool value)
    {
        SymbolEntryFunction;
        m_IsLambdaHash = value;
    }

#if HOSTED
    bool GetIsHostedDynamicHash()
    {
        SymbolEntryFunction;
        return m_IsHostedDynamicHash;
    }

    void SetIsHostedDynamicHash(bool value)
    {
        SymbolEntryFunction;
        m_IsHostedDynamicHash = value;
    }

    IVbHostedSymbolSource* GetExternalSymbolSource()
    {
        SymbolEntryFunction;
        return m_ExtSymbolSrc;
    }

    void SetExternalSymbolSource(IVbHostedSymbolSource* ExtSymbolSrc)
    {
        SymbolEntryFunction;
        m_ExtSymbolSrc = ExtSymbolSrc;
    }
#endif

    // For partial types, this is the actual partial component of the type that this
    // hash belongs to.
    //
    BCSYM_Container* GetPhysicalContainer()
    {
        SymbolEntryFunction;
        if (GetPhysicalContainerRaw())
        {
            return GetPhysicalContainerRaw();
        }

        BCSYM_NamedRoot *pCurrent = this;
        while(pCurrent = pCurrent->GetImmediateParent())
        {
            if (pCurrent->IsHash() && pCurrent->PHash()->GetPhysicalContainerRaw())
            {
                return pCurrent->PHash()->GetPhysicalContainerRaw();
            }

            if (pCurrent->IsContainer())
            {
                return pCurrent->PContainer();
            }
        }

        return NULL;
    }

    BCSYM_Container* GetPhysicalContainerRaw() const
    {
        SymbolEntryFunction;
        return m_PhysicalContainer;
    }

    void SetPhysicalContainer(BCSYM_Container *Container)
    {
        SymbolEntryFunction;
        m_PhysicalContainer = Container;
    }

    bool HasGenericParent();

    // BCSYM_NamespaceRing::CreateAndFillMergedHashTable() function delegates to this function
    // for merging each hash table from a BCSYM_Namespace object.
    //
    bool MergeHashTable(
        BCSYM_Hash * pMergedHash,
        Symbols * pSymbolCreator);

    BCSYM_NamedRoot **Rgpnamed() 
    {
        SymbolEntryFunction;
        return m_pHashTable; 
    }

protected:

    // Nobody should use this directly unless they take partial types
    // into account.
    //
    // Total number of symbols in this table.
    //
    unsigned m_cSymbols;

    // Nobody should use this directly unless they take partial types
    // into account.
    //
    // Size of this table.
    //
    unsigned m_cBuckets;

    // Used to store whether a hash contains bindable or unbindable members.
    // The default is Bindable.
    bool m_IsUnBindableChildrenHash : 1;

    bool m_IsLambdaHash : 1;

    // Used to cache whether a Hash has a generic parent or not. Useful because
    // otherwise each call to HasGenericParent has to iterate all the way up the
    // hierarchy which could potentially be expensive because of the high number
    // of calls to HasGenericParent.
    //
    bool m_HasGenericParent : 1;
    bool m_HasGenericParentBeenDetermined : 1;

#if HOSTED
    bool m_IsHostedDynamicHash : 1;
    IVbHostedSymbolSource* m_ExtSymbolSrc;
#endif

    // Used to store the actual parent container of the hash which could be a partial container
    BCSYM_Container *m_PhysicalContainer;

    // Used to store the previous hash in the chain of partial types' hashes
    BCSYM_Hash *m_psymPrevHash;
    BCSYM_Hash *m_psymNextHash;

    // Bucket array.
    #pragma warning (suppress  : 4200)
    BCSYM_NamedRoot * m_pHashTable[0];
}; // end of BCSYM_Hash

/**************************************************************************************************
;Alias

A symbol that refers to another symbol.  Because the existing design uses the Next pointers in
the symbols to conncect collisions when stored in a hash table, we have this problem where a symbol
can't be in more than one hash table at a time.  So this ---- wrapper was invented to encapsulate
a symbol that is stored in more than one hash table so we don't blow away the next pointer.
What should have happened is that a Hash table should never modify the symbol that is put in the
Hash - if we ever fix that, we could get rid of these things.
***************************************************************************************************/
class BCSYM_Alias : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:
    BCSYM *GetSymbol()
    {
        SymbolEntryFunction;
        return GetAliasedSymbol()->DigThroughNamedType();
    }

    BCSYM *GetAliasedSymbol() const
    {
        SymbolEntryFunction;
        return m_psym;
    }
    void SetAliasedSymbol(BCSYM * value)
    {
        SymbolEntryFunction;
        m_psym = value;
    }

protected:
    BCSYM * m_psym;

}; // end of BCSYM_Alias

struct SpellingInfo
{
    TrackedLocation m_Location;
    STRING * m_pSpelling;

    SpellingInfo * m_pNext;
};

class SpellingInfoIterator
{
public:
    SpellingInfoIterator(SpellingInfo * pRoot);
    SpellingInfo * Next();
private:
    SpellingInfo * m_pCurrent;
};

/**************************************************************************************************
;Container

Base for containers such as classes, interfaces, enums, namespaces, etc.  Represents symbols that
define a binding scope.
***************************************************************************************************/
class BCSYM_Container : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:

    // Get the hash table for this container.
    BCSYM_Hash *GetHash();

    BCSYM_Hash *GetHashRaw() const
    {
        SymbolEntryFunction;
        return m_phash;
    }

    BCSYM_Hash *GetUnBindableChildrenHash();

    BCSYM_Hash *GetUnBindableChildrenHashRaw() const
    {
        SymbolEntryFunction;
        return m_UnBindableChildrenHash;
    }

    // Is this a BASIC container?
    bool IsBasic();

    // Namespaces have a different symbol per file, so we need to handle them in a special way.
    bool IsSameContainer(BCSYM_NamedRoot *);

    // The module's first default property.
    BCSYM_Property *GetDefaultProperty()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        BCSYM_Property *DefaultProperty = m_DefaultProperty;
        BCSYM_Container *NextPartialContainer = this;

        while (!DefaultProperty &&
               (NextPartialContainer = NextPartialContainer->GetNextPartialType()))
        {
            DefaultProperty = NextPartialContainer->m_DefaultProperty;
        }

        return DefaultProperty;
    }
    void SetDefaultProperty(BCSYM_Property * value)
    {
        SymbolEntryFunction;
        m_DefaultProperty = value;
    }

    //
    // The following members are used to both handle demand loading
    // COM+ classes and generating metadata.
    //

    // Get the typedef token for this container.
    mdTypeDef GetTypeDef()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return (mdTypeDef)GetToken();
    }

    // Are the children loaded?
    bool AreChildrenLoaded()
    {
        SymbolEntryFunction;
        return IsBasic() || !GetChildrenNotLoaded();
    }

    // Are we loading the contained symbols?
    bool AreChildrenLoading() const
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return m_ChildrenLoading;
    }
    void SetChildrenLoading(bool value)
    {
        SymbolEntryFunction;
        m_ChildrenLoading = value;
    }

    // Make sure that the members of this symbol are fully
    // initialized.
    //
    void EnsureChildrenLoaded();

    // Are the base and implements loaded?
    bool AreBaseAndImplementsLoaded()
    {
        SymbolEntryFunction;
        return !m_BaseAndImplementsNotLoaded;
    }
    void SetBaseAndImplementsNotLoaded(bool value)
    {
        SymbolEntryFunction;
        m_BaseAndImplementsNotLoaded = value;
    }

    // Are we loading the base and implements?
    bool AreBaseAndImplementsLoading() const
    {
        SymbolEntryFunction;
        return m_BaseAndImplementsLoading;
    }
    void SetBaseAndImplementsLoading(bool value)
    {
        SymbolEntryFunction;
        m_BaseAndImplementsLoading = value;
    }

    // Get the compilerfile that holds this symbol.
    CompilerFile *GetCompilerFile() const
    {
        SymbolEntryFunction;
        return m_pfile;
    }
    void SetCompilerFile(CompilerFile * value)
    {
        SymbolEntryFunction;
        m_pfile = value;
    }

    // Get the source file that holds this symbol.
    SourceFile *GetSourceFile();

    // Get the metadatafile that holds this symbol.
    MetaDataFile *GetMetaDataFile();

    WellKnownAttrVals *GetPWellKnownAttrVals();

    bool IsClass() const;
    class BCSYM_Class *PClass() const;

    bool IsInterface();
    class BCSYM_Interface *PInterface();

    void
    SetHashes
    (
        BCSYM_Hash *BindableChildrenHash,
        BCSYM_Hash *UnBindableChildrenHash
    )
    {
        SymbolEntryFunction;
        // 




        SetHash(BindableChildrenHash);
        SetUnBindableChildrenHash(UnBindableChildrenHash);

        if (GetUnBindableChildrenHashRaw())
        {
            GetUnBindableChildrenHashRaw()->SetIsUnBindableChildrenHash();
        }
    }

    void
    SetHash(BCSYM_Hash * value)
    {
        SymbolEntryFunction;
        m_phash = value;
    }

    void SetUnBindableChildrenHash(BCSYM_Hash * value)
    {
        SymbolEntryFunction;
        m_UnBindableChildrenHash = value;
    }

    // Kind-a bogus but makes life easy.  Get the list of
    // nested types.
    //
    BCSYM_Container **GetNestedTypeList()
    {
        SymbolEntryFunction;
        VSASSERT(GetChildrenNotLoaded() || IsBadNamedRoot(), "Can't be loaded yet.");
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return &m_pNestedTypeList;
    }

    BCSYM_NamedType* GetNamedTypesList() const
    {
        SymbolEntryFunction;
        return m_HeadOfNamedTypesInContainer;
    }
    BCSYM_NamedType** GetNamedTypesListReference()
    {
        SymbolEntryFunction;
        return &m_HeadOfNamedTypesInContainer;
    }

    void AddToNestedTypesList(BCSYM_Container *NestedType)
    {
        SymbolEntryFunction;
        NestedType->SetNextNestedType(GetNestedTypes());
        SetNestedTypes(NestedType);
    }

    void RemoveFromNestedTypesList(BCSYM_Container *NestedType)
    {
        SymbolEntryFunction;
        if (GetNestedTypes() == NestedType && NestedType)
        {
            SetNestedTypes(NestedType->GetNextNestedType());
        }
        else
        {
           BCSYM_Container *PreviousType = GetNestedTypes();
           BCSYM_Container *CurrentType =
                GetNestedTypes() ?
                GetNestedTypes()->GetNextNestedType() :
            NULL;

            while (CurrentType)
            {
                if (CurrentType == NestedType)
                {
                    PreviousType->SetNextNestedType(CurrentType->GetNextNestedType());
                    return;
                }

                PreviousType = CurrentType;
                CurrentType = CurrentType->GetNextNestedType();
            }
        }
    }

    BCSYM_Container* GetNestedTypes() const
    {
        SymbolEntryFunction;
        return m_HeadOfNestedTypesInContainer;
    }
    void SetNestedTypes(BCSYM_Container* value)
    {
        SymbolEntryFunction;
        m_HeadOfNestedTypesInContainer = value;
    }

    BCSYM_Container* GetNextNestedType() const
    {
        SymbolEntryFunction;
        return m_NextNestedTypeInList;
    }
    void SetNextNestedType(BCSYM_Container* value)
    {
        SymbolEntryFunction;
        m_NextNestedTypeInList = value;
    }

    Bindable *GetBindableInstance();

    void DeleteBindableInstance();

    Bindable *GetBindableInstanceIfAlreadyExists() const
    {
        SymbolEntryFunction;
        return m_BindableInstance;
    };

    BCSYM* GetDeclaredAttrListHead() const
    {
        SymbolEntryFunction;
        return m_DeclaredAttrListHead;
    }

    void SetDeclaredAttrListHead(BCSYM *Symbol)
    {
        SymbolEntryFunction;
        m_DeclaredAttrListHead = Symbol;
    }

    BCSYM* GetBoundAttrListHead() const
    {
        SymbolEntryFunction;
        return m_BoundAttrListHead;
    }

    void SetBoundAttrListHead(BCSYM *Symbol)
    {
        SymbolEntryFunction;
        m_BoundAttrListHead = Symbol;
    }

    bool IsBindingDone() const
    {
        SymbolEntryFunction;
        return m_IsBindingDone;
    }

    bool HasBindingStarted()
    {
        SymbolEntryFunction;
        return IsBindingDone() || GetBindableInstanceIfAlreadyExists();
    }

    void SetBindingDone(bool value)
    {
        SymbolEntryFunction;
        m_IsBindingDone = value;
    }

    bool HasUserDefinedOperators()
    {
        SymbolEntryFunction;
        EnsureChildrenLoaded();

        // A container has user defined operators if it directly contains or
        // its partial types directly contain an operator.

        if (m_HasUserDefinedOperators)
        {
            return true;
        }

        // If this is a main type or a partial type, walk the list to see if
        // any of them contain operators.
        if (GetNextPartialType() || IsPartialType())
        {
            // Start the walk with the main type.
            BCSYM_Container *Current = GetMainType() ? GetMainType() : this;

            while (Current)
            {
                if (Current->m_HasUserDefinedOperators)
                {
                    return true;
                }

                Current = Current->GetNextPartialType();
            }
        }

        return false;
    }

    void SetHasUserDefinedOperators(bool value)
    {
        SymbolEntryFunction;
        m_HasUserDefinedOperators = value;
    }

    BCSYM_Expression *GetExpressionList() const
    {
        SymbolEntryFunction;
        if (!IsNamespace())
        {
            return m_ExpressionList;
        }
        else
        {
            return NULL;
        }
    }

    BCSYM_Expression ** GetExpressionListAddress()
    {
        SymbolEntryFunction;
        if (!IsNamespace())
        {
            return &m_ExpressionList;
        }
        else
        {
            return NULL;
        }
    }

    void SetExpressionList(BCSYM_Expression *ExpressionList)
    {
        SymbolEntryFunction;
        VSASSERT(!IsNamespace(), "SetExpressionList is not valid for namespace symbols. Namespaces may not use the m_ExpressionList member.");
        if (!IsNamespace())
        {
            m_ExpressionList = ExpressionList;
        }
    }

    bool IsPartialType() const
    {
        SymbolEntryFunction;
        return m_IsPartialType;
    }

    void SetIsPartialType(bool value)
    {
        SymbolEntryFunction;
        m_IsPartialType = value;
    }

    bool IsPartialKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_IsPartialKeywordUsed;
    }
    void SetPartialKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_IsPartialKeywordUsed = value;
    }

    BCSYM_Container* GetMainType() const;

    BCSYM_Container* GetNextPartialType();

    BCSYM_Container* IsPartialTypeAndHasMainType() const
    {
        SymbolEntryFunction;
        if (IsPartialType())
        {
            return GetMainType();
        }

        return NULL;
    }

#if IDE 
    SymbolList *GetBindableSymbolList()
    {
        SymbolEntryFunction;
        return &m_BindableSymbolList;
    }

    SymbolList *GetBindableDeletedSymbolList()
    {
        SymbolEntryFunction;
        return &m_BindableDeletedSymbolList;
    }

    bool IsNonMergedAnonymousType()
    {
        SymbolEntryFunction;
        return m_IsNonMergedAnonymousType;
    }

    void SetIsNonMergedAnonymousType(bool fIsNonMergedAnonmousType)
    {
        SymbolEntryFunction;
        m_IsNonMergedAnonymousType = fIsNonMergedAnonmousType;
    }
#endif

    bool IsCLSCompliant()
    {
        SymbolEntryFunction;
        DetermineIfCLSComplianceIsClaimed();

        return m_IsCLSCompliant;
    }

    bool HasCLSComplianceBeenDetermined() const
    {
        SymbolEntryFunction;
        return m_HasCLSComplianceBeenDetermined;
    }

    void SetIsCLSCompliant(bool fIsCLSCompliant)
    {
        SymbolEntryFunction;
        m_IsCLSCompliant = fIsCLSCompliant;
    }

    void SetHasCLSComplianceBeenDetermined(bool fHasCLSComplianceBeenDetermined)
    {
        SymbolEntryFunction;
        m_HasCLSComplianceBeenDetermined = fHasCLSComplianceBeenDetermined;
    }

    void DetermineIfCLSComplianceIsClaimed();

    bool IsDuplicateType() const
    {
        SymbolEntryFunction;
        return m_IsDuplicateType;
    }

    void SetIsDuplicateType(bool fIsDuplicateType)
    {
        SymbolEntryFunction;
        m_IsDuplicateType = fIsDuplicateType;
    }

    bool IsAnonymousType() const
    {
        SymbolEntryFunction;
        return m_IsAnonymous && !IsDelegate();
    }

    bool IsAnonymousDelegate() const
    {
        SymbolEntryFunction;
        return m_IsAnonymous && IsDelegate();
    }

    void SetIsAnonymous(bool value)
    {
        SymbolEntryFunction;
        m_IsAnonymous = value;
    }

    bool IsStateMachineClass() const
    {
        return m_IsStateMachineClass;
    }

    void SetIsStateMachineClass(bool value)
    {
        m_IsStateMachineClass = value;
    }

    BCSYM_Proc* GetOriginalResumableProc() const
    {
        return m_OriginalResumableProc;
    }

    void SetOriginalResumableProc(BCSYM_Proc* proc)
    {
        m_OriginalResumableProc = proc;
    }

#if IDE 

    // Dev10 #678696
    bool TreatAsEmbeddedLocalType() const;

#endif IDE

    BCSYM_Container * DigUpToModuleOrNamespace();

    bool GetChildrenNotLoaded() const
    {
        SymbolEntryFunction;
        return m_ChildrenNotLoaded;
    }
    void SetChildrenNotLoaded(bool value)
    {
        SymbolEntryFunction;
        m_ChildrenNotLoaded = value;
    }

    bool HasBeenAddedToDeclaredSymbolList() const
    {
        SymbolEntryFunction;
        return m_HasBeenAddedToDeclaredSymbolList;
    }
    void SetHasBeenAddedToDeclaredSymbolList(bool value)
    {
        SymbolEntryFunction;
        m_HasBeenAddedToDeclaredSymbolList = value;
    }

protected:


    // File that holds this container.
    CompilerFile *m_pfile;

    // If this container is loaded, this contains the
    // hash table for the container.  If it's not loaded
    // then it contains a linked list of types that are
    // nested inside of this one.
    //
    union
    {
        BCSYM_Hash *m_phash;
        BCSYM_Container *m_pNestedTypeList;
    };

    // Unbindable children like the get_foo, set_foo for property foo, etc.
    // go into this hash instead of the main hash
    BCSYM_Hash *m_UnBindableChildrenHash;

    // The default property of this container.
    BCSYM_Property *m_DefaultProperty;

    //
    // The following members are used to both handle demand loading
    // COM+ classes and generating metadata.
    //

    // TRUE if we're delay-loading the children of this symbol
    bool m_ChildrenNotLoaded : 1;

    // TRUE if we're currently loading the contained symbols.
    bool m_ChildrenLoading : 1;

    // TRUE if we haven't loaded in the base and implements yet.
    bool m_BaseAndImplementsNotLoaded : 1;

    // TRUE if we're currently loading the base and implements.
    bool m_BaseAndImplementsLoading : 1;

    // TRUE if the current container is either explicitly marked
    // as or inferred to be CLS Compliant
    bool m_IsCLSCompliant : 1;

    // TRUE if the CLS Compliance of the current container has
    // been determined.
    bool m_HasCLSComplianceBeenDetermined : 1;

    // TRUE if this container has been processed completely in bindable.
    bool m_IsBindingDone : 1;

    // Having the partial bits in BCSYM_Container instead of BCSYM_Class does not
    // take up any extra space and in the future makes it easier to support
    // partial interfaces, if ever needed.
    //
    // TRUE if this container is a partial type (i.e., has been declared with the 'Partial' specifier) or
    // has been determined to expand another type declared with a 'Partial' specifier.
    //
    bool m_IsPartialType : 1;

    // TRUE if this container is a partial type (i.e., has been declared with the 'Partial' specifier).
    bool m_IsPartialKeywordUsed : 1;

    // TRUE if this container has members which are user defined operators.  Only purpose for this flag is performance.
    bool m_HasUserDefinedOperators : 1;

    // TRUE if this container clashes with another container of the same full qualified name.
    bool m_IsDuplicateType : 1;

    // TRUE if this container has already been added to the global Symbol List and to its parent's nested type list in Declared
    bool m_HasBeenAddedToDeclaredSymbolList : 1;

    // TRUE if this container is anonymous
    bool m_IsAnonymous : 1;

    // TRUE if this container is a display class
    bool m_IsStateMachineClass : 1;

    // The following flags are used only for namespaces
    bool m_NamespaceContainsPublicType : 1;
    bool m_NamespaceContainsFriendType : 1;

#if IDE 
    // True if this container is an anonymous type which was created without
    // merging with other types. It requires special handling in AreTypesEqual.
    //
    bool m_IsNonMergedAnonymousType : 1;

#endif

    // used to store the BindableInstance that gets created temporarily during Bindable per container
    Bindable *m_BindableInstance;

    // list of all named types in a container (not including the bases)
    BCSYM_NamedType *m_HeadOfNamedTypesInContainer;

    // list of all types nested in a container
    BCSYM_Container *m_HeadOfNestedTypesInContainer;
    BCSYM_Container *m_NextNestedTypeInList;

    BCSYM *m_DeclaredAttrListHead;  // used to store symbols which are determined to have attributes in Declared
    BCSYM *m_BoundAttrListHead;     // used to store symbols which are determined to have attributes in bindable

    //Reuse symbol storage space for performance reasons
    //Namespace symbols defined in sourcefiles need to store a "spelling list" that tracks the
    //casings used and the positions of all occurances of that namespace decleration in the file.
    //Placing it in a union with m_pExressionList, which is not used for namespaces, allows it to be done
    //without increasing the storage requirements for meta data symbols.
    union
    {
        BCSYM_Expression *m_ExpressionList;  // used to store constant expressions that occur in declaration in a container. Only valid for non namespace containers
        SpellingInfo * m_pSpellingList;      // used to store namespace spelling information. This is only valid for namespace symbols. For meta data symbols this will always be null.
    };

#if IDE 
    // This is used to keep the list of symbols created by Bindable during binding
    // so that it knows to delete them from their hashes during decompilation.
    // this is 8 bytes in size
    //
    SymbolList m_BindableSymbolList;

    // This is used to keep the list of symbols deleted by Bindable during binding
    // so that it knows to undelete them from their hashes during decompilation.
    // this is 8 bytes in size
    //
    SymbolList m_BindableDeletedSymbolList;
#endif

    // If this container is a state machine class for resumable methods (async and
    // iterator methods) then we keep a pointer back to the original method.  This
    // is used to emit into the PDB for the special async method info.
    BCSYM_Proc *m_OriginalResumableProc;

}; // end of BCSYM_Container


/**************************************************************************************************
;CCContainer

Represents a Conditional Constant binding symbol.
***************************************************************************************************/
class BCSYM_CCContainer : public BCSYM_Container
{
    DBGFRIENDS;
};


/**************************************************************************************************
;NamespaceRing

There is exactly one NamespaceRing for each unique fully-qualified namespace name per compilation.
A NamespaceRing occurs only in the Namespaces table in a compiler, and is a root node for the ring
of instances of the namespace.
***************************************************************************************************/
class BCSYM_NamespaceRing : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:
    BCSYM_Namespace * GetFirstNamespace() const
    {
        SymbolEntryFunction;
        return m_pNamespaces;
    }
    void SetFirstNamespace(BCSYM_Namespace * value)
    {
        SymbolEntryFunction;
        m_pNamespaces = value;
    }

    bool GetNamesInAllModulesLoaded() const
    {
        SymbolEntryFunction;
        return m_NamesInAllModulesLoaded;
    }
    void SetNamesInAllModulesLoaded(bool value)
    {
        SymbolEntryFunction;
        m_NamesInAllModulesLoaded = value;
    }

#if HOSTED
    bool GetContainsHostedDynamicHash() const
    {
        SymbolEntryFunction;
        return m_ContainsHostedDynamicHash;
    }
    void SetContainsHostedDynamicHash(bool value)
    {
        SymbolEntryFunction;
        m_ContainsHostedDynamicHash = value;
    }
#endif

protected:

    BCSYM_Namespace *m_pNamespaces;
    bool m_NamesInAllModulesLoaded;        // Indicates whether the names in all the modules in all the namespaces in this ring are loaded.
#if HOSTED
    bool m_ContainsHostedDynamicHash;      // Indicates whether any of the BCSYM_Namespaces in the ring have IsHostedDynamicHash set.
#endif
};

/**************************************************************************************************
;Namespace

A symbol that refers to a namespace.
***************************************************************************************************/
class BCSYM_Namespace : public BCSYM_Container
{
    DBGFRIENDS;
public:
    bool IsSameNamespace(BCSYM_Namespace *pNamespace)
    {
        SymbolEntryFunction;
        return pNamespace && GetNamespaceRing() ==  pNamespace->GetNamespaceRing();
    }

    BCSYM_NamespaceRing * GetNamespaceRing() const
    {
        SymbolEntryFunction;
        return m_pRing;
    }
    void SetNamespaceRing(BCSYM_NamespaceRing * value)
    {
        SymbolEntryFunction;
        m_pRing = value;
    }

    BCSYM_Namespace *GetNextNamespace() const
    {
        SymbolEntryFunction;
        return m_pNextNamespace;
    }
    void SetNextNamespace(BCSYM_Namespace * value)
    {
        SymbolEntryFunction;
        m_pNextNamespace = value;
    }

    BCSYM_Namespace *GetPrevNamespace() const
    {
        SymbolEntryFunction;
        return m_pPrevNamespace;
    }
    void SetPrevNamespace(BCSYM_Namespace * value)
    {
        SymbolEntryFunction;
        m_pPrevNamespace = value;
    }

    BCSYM_Namespace *GetNextNamespaceInSameProject();

    void SetImports( ImportedTarget *ImportsList );

    inline ImportedTarget * GetImports() const
    {
        SymbolEntryFunction;
        return m_pImportsList;
    }

    // Return the head of the list of standard modules in the namespace.
    BCSYM_Class *GetFirstModule() const
    {
        SymbolEntryFunction;
        return m_pFirstModule;
    }
    void SetFirstModule(BCSYM_Class * value)
    {
        SymbolEntryFunction;
        m_pFirstModule = value;
    }
    
    // If the namespace is from metadata, make sure that the names of all members of all
    // standard modules in the namespace have been loaded.

    void EnsureMemberNamesInAllModulesLoaded();

    void ClearNamesInAllModulesLoaded()
    {
        SymbolEntryFunction;
        m_NamesInAllModulesLoaded = false;
        GetNamespaceRing()->SetNamesInAllModulesLoaded(false);
    }

    SpellingInfo * GetSpellings();
    void AddSpelling(SpellingInfo * pSpelling);


    // Dev10 #832844: Does namespace contain a public type in it or in one of its nested namespaces.
    bool ContainsPublicType() const
    {
        SymbolEntryFunction;
        return m_NamespaceContainsPublicType;
    }
    // Dev10 #832844: Mark/Unmark namespace as containing a public type in it or in one of its nested namespaces.
    // If namespace is to be marked, its parent namespace is marked as well.
    void SetContainsPublicType(bool value)
    {
        SymbolEntryFunction;
        if (value && !m_NamespaceContainsPublicType)
        {
            BCSYM_Namespace * pParentNamespace = GetImmediateParent()->GetContainingNamespace();

            if (pParentNamespace != NULL)
            {
                pParentNamespace->SetContainsPublicType(true);
            }
        }

        m_NamespaceContainsPublicType = value;
    }
    
    // Dev10 #832844: Does namespace contain a friend type in it or in one of its nested namespaces.
    bool ContainsFriendType() const
    {
        SymbolEntryFunction;
        return m_NamespaceContainsFriendType;
    }
    // Dev10 #832844: Mark/Unmark namespace as containing a friend type in it or in one of its nested namespaces.
    // If namespace is to be marked, its parent namespace is marked as well.
    void SetContainsFriendType(bool value)
    {
        SymbolEntryFunction;
        if (value && !m_NamespaceContainsFriendType)
        {
            BCSYM_Namespace * pParentNamespace = GetImmediateParent()->GetContainingNamespace();

            if (pParentNamespace != NULL)
            {
                pParentNamespace->SetContainsFriendType(true);
            }
        }
        
        m_NamespaceContainsFriendType = value;
    }


private:
    // All namespaces having the same fully qualified name are kept in a
    // doubly circular linked list.
    BCSYM_Namespace * m_pNextNamespace;
    BCSYM_Namespace * m_pPrevNamespace;
    ImportedTarget * m_pImportsList;
    BCSYM_NamespaceRing * m_pRing;

    // All the standard modules in a namespace are on a list so that
    // name lookup can traverse them quickly.
    BCSYM_Class * m_pFirstModule;

    bool m_NamesInAllModulesLoaded;

}; // end of BCSYM_Namespace

/**************************************************************************************************
;XmlName

Represents an Xml name
namespace value.
***************************************************************************************************/
class BCSYM_XmlName : public BCSYM_NamedRoot
{
    DBGFRIENDS;
}; // end of BCSYM_XmlName

/**************************************************************************************************
;XmlNamespaceDeclaration

Represents an Xml namespace declaration, where the declaration binds an Xml prefix to an Xml
namespace value.
***************************************************************************************************/
class BCSYM_XmlNamespaceDeclaration : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:
    bool IsImported() const
    {
        SymbolEntryFunction;
        return m_IsImported;
    }
    void SetIsImported(bool value) 
    {
        SymbolEntryFunction;
        m_IsImported = value;
    }

private:
    bool m_IsImported;
}; // end of BCSYM_XmlNamespaceDeclaration

/**************************************************************************************************
;XmlNamespace

Represents an Xml namespace container, where the Xml namespace holds all XmlNames in the namespace
***************************************************************************************************/
class BCSYM_XmlNamespace : public BCSYM_Container
{
    DBGFRIENDS;
}; // end of BCSYM_XmlNamespace

typedef unsigned __int64 DECLFLAGS;

/**************************************************************************************************
;Member

All class members derive from here.
***************************************************************************************************/
class BCSYM_Member : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:
    // The type of this member. For a function it is the return type. For an field
    // or property it is the type of that field/property. Note that the raw type
    // might not have been resolved yet. (Call GetType() to ensure it's bound.)
    BCSYM *GetRawType() const
    {
        SymbolEntryFunction;
        return m_ptyp;
    }

    BCSYM *GetType()
    {
        SymbolEntryFunction;
        return GetRawType()->DigThroughNamedType();
    }

    void SetType(BCSYM *ptyp)
    {
        SymbolEntryFunction;
        m_ptyp = ptyp;
    }

    // The type that the compiler uses for type checking and coercion
    // semantics.  This digs through aliases.
    BCSYM *GetCompilerType()
    {
        SymbolEntryFunction;
        return GetType()->DigThroughAlias();
    }

    bool IsShared() const
    {
        SymbolEntryFunction;
        return m_isShared;
    }

    // This only applies to VB code. If the member is for a MetaData file
    // the bracketed flag will not be set.
    bool IsBracketed() const
    {
        SymbolEntryFunction;
        return m_isBracketed;
    }

    void SetIsShared(bool IsShared)
    {
        SymbolEntryFunction;
        m_isShared = IsShared;
    }

    void SetIsBracketed(bool IsBracketed)
    {
        SymbolEntryFunction;
        m_isBracketed = IsBracketed;
    }

    DECLFLAGS GetDeclFlags();

protected:

    // The type of this symbol.
    BCSYM * m_ptyp;
}; // end of BCSYM_Member

/**************************************************************************************************
;HandlerList

Provides the information required to generate Add/Remove handler calls for the event handlers in
a class.  This information is typically tacked on to a WithEvents set synthetic method, but for
MyBase.Event handles, it is hooked on the constructor - See BCSYM_Proc::GetHandlerList()
***************************************************************************************************/
struct HandlerList
{
    BCSYM_Proc *HandlingProc; // the proc that handles the event
    BCSYM_EventDecl *Event; // the event that is handled
    BCSYM_GenericBinding *EventGenericBindingContext; // the GenericBinding for the event being handled
    BCSYM_Property *EventSourceProperty; // If this is an event on a subobject, this is the property that returns the subobject
    HandlerList *Next; // for chaining HandlerLists
};

/**************************************************************************************************
;Proc

Base class for a method symbol table entry
***************************************************************************************************/
class BCSYM_Proc : public BCSYM_Member
{
    DBGFRIENDS;
public:
    // Get the first parameter in the parameter list.
    BCSYM_Param* GetFirstParam() const
    {
        SymbolEntryFunction;
        return m_pparamFirst;
    }
    void SetFirstParam(BCSYM_Param * value)
    {
        SymbolEntryFunction;
        m_pparamFirst = value;
    }

    // Get the last parameter in the parameter list.
    BCSYM_Param *GetLastParam();

    // Get the param that represents the return value. This is only
    // set if there are attributes on the return type
    BCSYM_Param *GetReturnTypeParam() const
    {
        SymbolEntryFunction;
        return m_pparamReturn;
    }
    void SetReturnTypeParam(BCSYM_Param * value)
    {
        SymbolEntryFunction;
        m_pparamReturn = value;
    }

    // Find a parameter of the specified name.  If found, true is returned and
    // *ppprocParmOut is set to the parameter symbol and *piposOut is set to the
    // 0-based index of the parameter's position.  If not found, false is returned.
    // Used by named-argument handlers.
    bool GetNamedParam(_In_z_ STRING *pstrName, BCSYM_Param **ppparamOut, UINT *piposOut);

    // Get the next constructor.
    BCSYM_Proc *GetNextInstanceConstructor();

    // Is the Overrides keyword explicitly used in this method's declaration?
    bool IsOverridesKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isOverridesKeywordUsed;
    }
    void SetOverridesKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_isOverridesKeywordUsed = value;
    }

    // Is the Overloads keyword explicitly used in this method's declaration?
    bool IsOverloadsKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isOverloadsKeywordUsed;
    }
    void SetOverloadsKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_isOverloadsKeywordUsed = value;
    }

    // whether this symbol's mustoverride keyword if any should be ignored. This is set
    // for partial type members when some error scenarios are only detected in bindable.    bool IsIgnoreMustOverrideKeyword()
    bool IsIgnoreMustOverrideKeyword() const
    {
        SymbolEntryFunction;
        return m_IgnoreMustOverrideKeyword;
    }

    // Does this method actually override anybody, regardless
    // of whether or not the keyword was specified.
    bool IsOverrides()
    {
        SymbolEntryFunction;
        // Partial methods can never override a method.
        return ( !IsPartialMethodDeclaration() &&
                 !IsPartialMethodImplementation() &&
                 GetOverriddenProcRaw() != NULL
            );
    }

    // This is the introducing overridable method (that does not override any
    // other method) that this method overrrides
    BCSYM_Proc *OverriddenProc()
    {
        SymbolEntryFunction;
        // Partial methods can never override a method.
        if( IsPartialMethodDeclaration() || IsPartialMethodImplementation() )
        {
            return( NULL );
        }
        else
        {
            return GetOverriddenProcRaw();
        }
    }

    BCSYM_Proc *GetOverriddenProcRaw() const
    {
        SymbolEntryFunction;
        return m_OverriddenProc;
    }

    // The procedure we are actually overriding (this is just the next override
    // in the chain, not the introducing method necessarily)
    BCSYM_Proc *OverriddenProcLast()
    {
        SymbolEntryFunction;
        // Partial methods can never override a method.
        if( IsPartialMethodDeclaration() || IsPartialMethodImplementation() )
        {
            return( NULL );
        }
        else
        {
            return GetOverriddenProcLastRaw();
        }
    }
    BCSYM_Proc * GetOverriddenProcLastRaw() const
    {
        SymbolEntryFunction;
        return m_OverriddenProcLast;
    }
    void SetOverriddenProcLastRaw(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        m_OverriddenProcLast = value;
    }

    void SetOverriddenProc(BCSYM_Proc *OverriddenProc)
    {
        SymbolEntryFunction;
        //  m_OverriddenProc will end by pointing to the first overridable which doesn't override any other meth.
        //
        //  m_OverriddenProcLast creates a linked list of all the overrides chain with m_OverriddenProc at the end
        //  once is set to a non zero value it doesn't modify
        //

        if (OverriddenProc && OverriddenProc->OverriddenProc())
        {
            SetOverriddenProcRaw(OverriddenProc->OverriddenProc());
        }
        else
        {
            SetOverriddenProcRaw(OverriddenProc);
        }

        SetOverriddenProcLastRaw(OverriddenProc);
    }

    void SetOverriddenProcRaw(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        m_OverriddenProc = value;
    }

    void SetOverridesRequiresMethodImpl(bool fRequiresMethodImpl)
    {
        SymbolEntryFunction;
        m_OverridesRequiresMethodImpl = fRequiresMethodImpl;
    }

    // If a method is overriding another method and a virtual inaccessible shadowed
    // method intervenes, we have to emit a methodimpl for it (see comments where
    // this is called)
    bool OverridesRequiresMethodImpl() const
    {
        SymbolEntryFunction;
        return m_OverridesRequiresMethodImpl;
    }

    // Does this method actually overload anybody, regardless of whether or not the keyword was specified.
    bool IsOverloads() const
    {
        SymbolEntryFunction;
        return m_isOverloads;
    }
    void SetIsOverloads(bool value)
    {
        SymbolEntryFunction;
        m_isOverloads = value;
    }

    // Is this implementing something?
    bool IsImplementing() const
    {
        SymbolEntryFunction;
        return m_isImplementing;
    }
    void SetIsImplementing(bool value)
    {
        SymbolEntryFunction;
        m_isImplementing = value;
    }

    // Abstract method?
    bool IsMustOverrideKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isMustOverrideKeywordUsed && !IsIgnoreMustOverrideKeyword();
    }

    // Proc explicitly marked as not being overridable?
    bool IsNotOverridableKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isNotOverridableKeywordUsed;
    }
    void SetNotOverridableKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_isNotOverridableKeywordUsed = value;
    }

    // Proc explicitly marked as overridable?
    bool IsOverridableKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isOverridableKeywordUsed;
    }
    void SetOverridableKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_isOverridableKeywordUsed = value;
    }

    // Is this method a shared constructor?
    bool IsSharedConstructor()
    {
        SymbolEntryFunction;
        return IsAnyConstructor() && IsShared();
    }

    // Is this method a constructor that is NOT shared?
    bool IsInstanceConstructor()
    {
        SymbolEntryFunction;
        return IsAnyConstructor() && !IsShared();
    }

    // Is this method a constructor ( shared / notshared being irrelevant )?
    bool IsAnyConstructor() const
    {
        SymbolEntryFunction;
        return m_isConstructor;
    }
    void SetIsAnyConstructor(bool value)
    {
        SymbolEntryFunction;
        m_isConstructor = value;
    }

    // Is the procedure Overridable? - used in Dropdowns
    bool IsOverridable()
    {
        SymbolEntryFunction;
        return !IsNotOverridableKeywordUsed() && !IsInstanceConstructor();
    }

    // If this is a property get/set, return the property symbol that defines it
    inline BCSYM_Property *GetAssociatedPropertyDef() const
    {
        SymbolEntryFunction;
        VSASSERT((IsPropertyGet() || IsPropertySet()) && !IsUserDefinedOperatorMethod(),
                 "Why are you calling this for a method impl that isn't a property get/set?");
        return m_AssociatedPropertyDef;
    }

    // If this is a property get/set, keep track of the property symbol that defines it
    inline void SetAssociatedPropertyDef( BCSYM_Property *AssociatedPropertyDef )
    {
        SymbolEntryFunction;
        VSASSERT( (AssociatedPropertyDef == NULL) ||
                  ((IsPropertyGet() || IsPropertySet()) && !IsUserDefinedOperatorMethod()),
                 "Why are you setting this for a method impl that isn't a property get/set?");
        m_AssociatedPropertyDef = AssociatedPropertyDef;
    }

    // The number of parameters on this method.
    unsigned GetParameterCount();
    unsigned GetRequiredParameterCount();
    BCSYM_Param *GetParamArrayParameter();

    // Optimized version of the above that only walks through the
    // parameter list once.  The "maximum" number of parameters includes
    // all of the optional paramters and will be an extremely large
    // number if this method has a paramarray.
    //
    void GetAllParameterCounts(unsigned &Required, unsigned &Maximum, bool &HasParamArray);

    ILTree::ProcedureBlock *GetBoundTree()
    {
        SymbolEntryFunction;
        return m_pBoundTree;
    }

    void SetBoundTree(ILTree::ProcedureBlock *pBoundTree)
    {
        SymbolEntryFunction;
        m_pBoundTree = pBoundTree;
    }

    // Is this not a property?
    bool IsProcedure() const
    {
        SymbolEntryFunction;
        return m_isProcedure;
    }
    void SetIsProcedure(bool value)
    {
        SymbolEntryFunction;
        m_isProcedure = value;
    }

    // Is this a property get?
    bool IsPropertyGet() const
    {
        SymbolEntryFunction;
        return m_isPropertyGet;
    }
    void SetIsPropertyGet(bool value)
    {
        SymbolEntryFunction;
        m_isPropertyGet = value;
    }

    bool IsPropertySet() const
    {
        SymbolEntryFunction;
        return m_isPropertySet;
    }
    void SetIsPropertySet(bool value)
    {
        SymbolEntryFunction;
        m_isPropertySet = value;
    }

    bool IsEventAccessor() const
    {
        SymbolEntryFunction;
        return m_isEventAccessor;
    }
    void SetIsEventAccessor(bool value)
    {
        SymbolEntryFunction;
        m_isEventAccessor = value;
    }

    bool IsCallableAsExtensionMethod() const
    {
        SymbolEntryFunction;
        return m_isCallableAsExtensionMethod;
    }
    void SetCallableAsExtensionMethod(bool value)
    {
        SymbolEntryFunction;
        m_isCallableAsExtensionMethod = value;
    }

    bool IsMyGenerated() const
    {
        SymbolEntryFunction;
        return m_isMyGenerated;
    }
    void SetIsMyGenerated(bool value)
    {
        SymbolEntryFunction;
        m_isMyGenerated = value;
    }

    bool IsImplementingComClassInterface() const
    {
        SymbolEntryFunction;
        return m_isImplementingComClassInterface;
    }
    void SetIsImplementingComClassInterface(bool value)
    {
        SymbolEntryFunction;
        m_isImplementingComClassInterface = value;
    }

    bool IsBadParamTypeOrReturnType() const
    {
        SymbolEntryFunction;
        return m_isBadParamTypeOrReturnType;
    }
    void SetIsBadParamTypeOrReturnType(bool value)
    {
        SymbolEntryFunction;
        m_isBadParamTypeOrReturnType = value;
    }

    bool CheckAccessOnOverride() const
    {
        SymbolEntryFunction;
        return m_CheckAccessOnOverride;
    }
    void SetCheckAccessOnOverride(bool value)
    {
        SymbolEntryFunction;
        m_CheckAccessOnOverride = value;
    }

    bool ParticipatesInPartialMethod()
    {
        SymbolEntryFunction;
        return( IsPartialMethodDeclaration() || IsPartialMethodImplementation() );
    }

    bool IsPartialMethodDeclaration() const
    {
        SymbolEntryFunction;
        return m_isPartialMethodDeclaration;
    }
    void SetIsPartialMethodDeclaration(bool value)
    {
        SymbolEntryFunction;
        m_isPartialMethodDeclaration = value;
    }


    bool IsPartialMethodImplementation() const
    {
        SymbolEntryFunction;
        return m_isPartialMethodImplementation;
    }
    void SetIsPartialMethodImplementation(bool value)
    {
        SymbolEntryFunction;
        m_isPartialMethodImplementation = value;
    }


    bool JITOptimizationsMustBeDisabled() const
    {
        SymbolEntryFunction;
        return m_JITOptimizationsMustBeDisabled;
    }
    void SetJITOptimizationsMustBeDisabled(bool value)
    {
        SymbolEntryFunction;
        m_JITOptimizationsMustBeDisabled = value;
    }

    bool IsAsyncKeywordUsed() const
    {
        return m_isAsync;
    }
    void SetAsyncKeywordUsed(bool value)
    {
        m_isAsync = value;
    }

    bool IsIteratorKeywordUsed() const
    {
        return m_isIterator;
    }
    void SetIteratorKeywordUsed(bool value)
    {
        m_isIterator = value;
    }

    // Get the associated method. Use the IsPartialMethod[Impl/Decl] accessors
    // to determine what the associated method is.

    BCSYM_Proc* GetAssociatedMethod() const
    {
        SymbolEntryFunction;
        VSASSERT( IsPartialMethodDeclaration() || 
            IsPartialMethodImplementation() || 
            (m_AssociatedProcedure == NULL) ,
            "Must be a partial method somehow." );

        return m_AssociatedProcedure;
    }
    void SetAssociatedMethod(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        m_AssociatedProcedure = value;
    }

    // Sets the implementation method for this partial method.

    void SetImplementationForPartialMethod( BCSYM_Proc* impl )
    {
        SymbolEntryFunction;
        ThrowIfNull( impl );
        VSASSERT( IsMethodImpl(), "Why are we not a method impl?" );
        VSASSERT( IsPartialMethodDeclaration(), "Must be called on the partial method declaration." );

        VSASSERT( GetAssociatedMethod() == NULL, "Partial proc set; Decompilation problem?" );
        VSASSERT( impl->GetAssociatedMethod() == NULL, "Partial proc set; Decompilation problem?" );
        VSASSERT( !impl->IsPartialMethodDeclaration(), "Partial flag set; decompilation problem?" );

        SetAssociatedMethod(impl);
        impl->SetIsPartialMethodImplementation(true);
        impl->SetAssociatedMethod(this);
    }

    void ResetImplementationForPartialMethod()
    {
        SymbolEntryFunction;
        VSASSERT( IsMethodImpl(), "Why are we not a method impl?" );
        VSASSERT( IsPartialMethodDeclaration(), "Must be called on the partial method declaration." );

        // Unlink our procs, and unset our flag, but leave the declaration flag on, because
        // that gets built in declared, and bindable doesn't change it.

        BCSYM_Proc* impl = GetAssociatedMethod();

        if( impl != NULL )
        {
            VSASSERT( impl->IsPartialMethodImplementation(), "Huh? Why is the linked proc not an impl?" );

            SetAssociatedMethod(NULL);
            impl->SetAssociatedMethod(NULL);
            impl->SetIsPartialMethodDeclaration(false);
        }
    }

    // For adding to the list of events handled by the Synth_WithEvents_Set associated with a WithEvent var
    void AddToHandlerList( BCSYM_Proc *HandlingProc, BCSYM_EventDecl *Event, BCSYM_GenericBinding *EventGenericBindingContext, BCSYM_Property *EventSourceProperty, NorlsAllocator *Allocator );

    // Returns the list of events handled for the Synth_WithEvents_Set associated with a WithEvent var
    HandlerList *GetHandlerList() const
    {
        SymbolEntryFunction;
        return m_HandlerList;
    }

    void SetHandlerList(HandlerList * value)
    {
        SymbolEntryFunction;
        m_HandlerList = value;
    }

    // Initialize an iteration over this proc's parameters.
    void InitParamIterator(BCITER_Parameters *pbiparam);

    // Get the DocComment Signature for the Proc
    void GetDocCommentSignature(Compiler *pCompiler, _In_opt_z_ STRING *pstrName, BCSYM_NamedRoot *pParent, StringBuffer *pSignature);

    // Returns the XMLDoc signature for a generic proc.
    void GetDocCommentGenericSignature(StringBuffer *pBuffer);

    // Returns the XMLDoc signature for a proc paratms.
    void GetXMLDocCommentParamSignature(Compiler *pCompiler, StringBuffer *pSignature);

    // returns the list built from the IMPLEMENTS clause
    BCSYM_ImplementsList * GetImplementsList() const
    {
        SymbolEntryFunction;
        return m_pImplementsList;
    }

    void SetImplementsList( BCSYM_ImplementsList *ImplList )
    {
        SymbolEntryFunction;
        m_pImplementsList = ImplList;
    }

    GenericParams * GetGenericParams() const
    {
        SymbolEntryFunction;
        return m_GenericParams;
    }
    void SetGenericParams(GenericParams * value)
    {
        SymbolEntryFunction;
        m_GenericParams = value;
    }

    bool IsGeneric()
    {
        SymbolEntryFunction;
        return GetGenericParams() != NULL;
    }

    BCSYM_GenericParam *GetFirstGenericParam()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_FirstGenericParam : NULL;
    }

    BCSYM_Hash *GetGenericParamsHash()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_GenericParamsHash : NULL;
    }

    bool IsUserDefinedOperatorMethod() const
    {
        SymbolEntryFunction;
        VSASSERT(!m_isUserDefinedOperatorMethod || GetAssociatedOperatorDef(),
                 "an operator method must have associated operator def set");
        return m_isUserDefinedOperatorMethod;
    }

    BCSYM_UserDefinedOperator *
    GetAssociatedOperatorDef() const
    {
        SymbolEntryFunction;
        VSASSERT(m_isUserDefinedOperatorMethod && m_AssociatedOperatorDef &&
                 ((BCSYM_Proc*)m_AssociatedOperatorDef)->IsUserDefinedOperator(),
                 "unexpected state for associated operator def");
        return m_AssociatedOperatorDef;
    }

    void
    SetAssociatedOperatorDef
    (
        BCSYM_UserDefinedOperator *AssociatedOperatorDef
    )
    {
        SymbolEntryFunction;
        m_AssociatedOperatorDef = AssociatedOperatorDef;
        m_isUserDefinedOperatorMethod = true;
    }

#if IDE 
    bool IsENCSynthFunction();
#endif

    void
    SetBadMetadataProcOverload()
    {
        SymbolEntryFunction;
        m_isBadMetadataProcOverload = true;
    }

    bool
    IsBadMetadataProcOverload() const
    {
        SymbolEntryFunction;
        return m_isBadMetadataProcOverload;
    }

    void
    GenerateFixedArgumentBitVectorFromFirstParameter
    (
        IBitVector * pBitVector,
        Compiler * pCompiler
    );

    void SetIgnoreMustOverrideKeyword(bool IgnoreMustOverride)
    {
        SymbolEntryFunction;
        m_IgnoreMustOverrideKeyword = IgnoreMustOverride;
    }
    void SetMustOverrideKeywordUsed(bool IsMustOverrideKeywordUsed)
    {
        SymbolEntryFunction;
        m_isMustOverrideKeywordUsed = IsMustOverrideKeywordUsed;
    }

    bool IsVirtual()
    {
        SymbolEntryFunction;
        if (IsOverridesKeywordUsed() ||
            IsMustOverrideKeywordUsed() ||
            IsOverridableKeywordUsed() ||
            IsNotOverridableKeywordUsed() ||
            IsImplementing() ||
            IsImplementingComClassInterface())
        {
            return true;
        }

        return false;
    }

    bool IsPreserveSig(void) const
    {
        SymbolEntryFunction;
        return m_PreserveSig;
    }
    void SetPreserveSig(bool preserveSig)
    {
        SymbolEntryFunction;
        m_PreserveSig = preserveSig;
    }

    bool IsFakeWindowsRuntimeMember() const
    {
        SymbolEntryFunction;
        return m_isFakeWindowsRuntimeInterfaceMember;
    }

    void SetIsFakeWindowsRuntimeMember(bool value)
    {
        SymbolEntryFunction;
        m_isFakeWindowsRuntimeInterfaceMember = value;
    }

protected:


    BCSYM_Param *m_pparamFirst; // The first parameter in the parameter list.
    BCSYM_Param *m_pparamReturn; // The return parameter representation

    union
    {
        BCSYM_Property *m_AssociatedPropertyDef;    // the property block that defined this property get/set
        BCSYM_UserDefinedOperator *m_AssociatedOperatorDef;  // the defining operator symbol (see MakeOperator in declared)
    };

    HandlerList * m_HandlerList; // The list of events handled for the Synth_WithEvents_Set associated with a WithEvent var
    // the list from the IMPLEMENTS clause
    BCSYM_ImplementsList * m_pImplementsList;

    // Partial methods current will not allow overrides; thus, it is safe
    // to reuse these fields.

    union
    {
        BCSYM_Proc *m_AssociatedProcedure;
        BCSYM_Proc *m_OverriddenProc; // the proc this is overriding, if any (slot start)
    };

    BCSYM_Proc *m_OverriddenProcLast; // link to the last overriden if a chain of overrides

    GenericParams * m_GenericParams;

    ILTree::ProcedureBlock *m_pBoundTree; // The bound tree from interpreting this method body

    bool m_isProcedure:1; // Is this not a property?
    bool m_isPropertyGet:1; // Is this a property get?
    bool m_isPropertySet:1; // Is this a property set?
    bool m_isOverloadsKeywordUsed:1; // whether this symbol was explicitly marked with the 'Overloads' keyword
    bool m_isOverridesKeywordUsed:1; // whether this symbol was explicitly marked with the 'Overrides' keyword
    bool m_isMustOverrideKeywordUsed:1; // whether this symbol was explicitly marked with the 'MustOverride' keyword
    bool m_IgnoreMustOverrideKeyword:1; // whether this symbol's mustoverride keyword if any should be ignored. This is set
                                        // for partial type members when some error scenarios are only detected in bindable.
    bool m_isNotOverridableKeywordUsed:1; // whether this symbol was explicitly marked with the 'NotOverridable' keyword
    bool m_isOverridableKeywordUsed:1;  // whether this symbol was explicitly marked with the 'Overridable' keyword
    bool m_isOverloads:1; // whether this symbol is trying to overloade another ( this is regardless of whether the OVERLOADS keyword is used )
    bool m_isConstructor:1; // whether this symbol is a constructor
    bool m_isImplementing:1; // whether this symbol implements something
    bool m_isImplementingComClassInterface:1; // Is this part of a com interface?
    bool m_isBadParamTypeOrReturnType:1; // does this proc have a parameter with a bad type or does it return a bad type?
    bool m_OverridesRequiresMethodImpl:1;
    bool m_isUserDefinedOperatorMethod:1; // Is this the method for a user-defined operator?
    bool m_isMyGenerated:1; // Is this method generated by a MyGroup?
    bool m_isBadMetadataProcOverload:1; // Whether this metadata proc is marked as a bad symbol because of bad overloading against
                                        // other procs, other symbol kinds like fields, etc. VSWhidbey 553868.
    bool m_CheckAccessOnOverride:1; // For procs imported via MetaImport, this contains the value of
                                                    // the mdCheckAccessOnOverride flag, in case we need to export
                                                    // a local copy of the type later (for NoPIA).
    bool m_PreserveSig:1; // For procs imported via MetaImport, this contains the value of the miPreserveSig flag, in case
                          // we need to export a local copy of the type later (for NoPIA).

    //Whether this symbol is either an event AddHandler, and event RemoveHandler, or an event fire
    //This is introduced for performing validation on extension methods, where all we need to
    //know is whether or not the method is an event accessor. If you need more fine grained
    //information, then feel free to split this up into 3 separate flags and change
    //the implementation of IsEventAccessor accordingly.
    bool m_isEventAccessor: 1;

    bool m_isCallableAsExtensionMethod : 1; //Whether this symbol is callable as an extension method.
    bool m_isPartialMethodDeclaration : 1; // Is this symbol the partial method declaration?
    bool m_isPartialMethodImplementation : 1; // Is this symbol the implementation of a partial method?

    bool m_JITOptimizationsMustBeDisabled : 1; // Should compiler disable JIT optimizations for this proc?

    bool m_isAsync : 1; // Whether this symbol was marked with the 'Async' contextual keyword
    bool m_isIterator : 1; // Whether this symbol was marked with the 'Iterator' contextual keyword
    bool m_isFakeWindowsRuntimeInterfaceMember : 1; // This is a fake member on a WinRT type that implements certain interfaces.
}; // end of BCSYM_Proc

/**************************************************************************************************
;SyntheticMethod

Represents a method that was generated by the compilation process.
***************************************************************************************************/
class BCSYM_SyntheticMethod : public BCSYM_Proc
{
    DBGFRIENDS;
public:

    // What kind of code we need to generate for this.
    SyntheticKind GetSyntheticKind() const
    {
        SymbolEntryFunction;
        return (SyntheticKind) m_SynthKind;
    }
    void SetSyntheticKind(SyntheticKind  value)
    {
        SymbolEntryFunction;

        VSASSERT(value < 32, "enum too large");
        
        m_SynthKind = value;
    }

    ParseTree::StatementList *GetParsedCode() const
    {
        SymbolEntryFunction;
        return m_ParsedCode;
    }
    void SetParsedCode(ParseTree::StatementList * value)
    {
        SymbolEntryFunction;
        m_ParsedCode = value;
    }

    // Locals count of the statementList of GetParsedCode.
    unsigned GetLocalsCount() const
    {
        SymbolEntryFunction;
        return m_LocalsCount;
    }
    void SetLocalsCount(unsigned value)
    {
        SymbolEntryFunction;
        m_LocalsCount = value;
    }

    // The synthesized block of code for this method.
    const WCHAR *GetCode() const
    {
        SymbolEntryFunction;
        return m_CodeText;
    }
    void SetCode(const WCHAR * value)
    {
        SymbolEntryFunction;
        m_CodeText = value;
    }

    // The size of the block in characters.
    unsigned GetCodeSize() const
    {
        SymbolEntryFunction;
        return m_CodeSize;
    }
    void SetCodeSize(unsigned value)
    {
        SymbolEntryFunction;
        m_CodeSize = value;
    }

    // Whether the synthetic code should first call the base class
    bool IsCallBaseFirst() const
    {
        SymbolEntryFunction;
        return m_CallBaseFirst;
    }

    // Mark this synthetic method as requiring a call to the base class
    void SetCallBaseFirst()
    {
        SymbolEntryFunction;
        m_CallBaseFirst = true;
    }


    bool IsLambda() const
    {
        SymbolEntryFunction;
        return m_IsLambda;
    }

    void SetIsLambda()
    {
        SymbolEntryFunction;
        m_IsLambda = true;
    }

    bool IsResumable() const
    {
        return m_IsResumable;
    }

    void SetIsResumable()
    {
        VSASSERT(wcscmp(this->GetName(), L"MoveNext") == 0, "Only generated MoveNext method should be resumable");
        m_IsResumable = true;
    }

    CSingleList<MethodDebugInfo> *GetDebugInfoList() 
    {
        SymbolEntryFunction;
        return &m_MethodDebugInfoList;
    }

    void SetAsyncDebugInfo(AsyncMethodDebugInfo *pAsyncMethodDebugInfo)
    {
        SymbolEntryFunction;
        m_pAsyncMethodDebugInfo = pAsyncMethodDebugInfo;
    }

    AsyncMethodDebugInfo *GetAsyncDebugInfo()
    {
        SymbolEntryFunction;
        return m_pAsyncMethodDebugInfo;
    }

    void SetMethodScope(BlockScope * MethodScope)
    {
        SymbolEntryFunction;
        m_MethodScope = MethodScope;
    }

    BlockScope *GetMethodScope() const
    {
        SymbolEntryFunction;
        return m_MethodScope;
    }

    void SetSignatureToken(mdSignature SignatureToken)
    {
        SymbolEntryFunction;
        m_SignatureToken = SignatureToken;
    }

    mdSignature GetSignatureToken() const
    {
        SymbolEntryFunction;
        return m_SignatureToken;
    }

    bool GetIsRelaxedDelegateLambda()
    {
        SymbolEntryFunction;
        return m_IsRelaxedDelegateLambda;
    }

    void SetIsRelaxedDelegateLambda(bool value)
    {
        SymbolEntryFunction;
        m_IsRelaxedDelegateLambda = value;
    }

    void SetWholeSpan(const Location &WholeSpan)
    {
        SymbolEntryFunction;
        m_WholeSpan = WholeSpan;
    }

    void SetBodySpan(const Location &BodySpan)
    {
        SymbolEntryFunction;
        m_BodySpan = BodySpan;
    }

    const Location& GetWholeSpan()
    {
        SymbolEntryFunction;
        return m_WholeSpan;
    }

    const Location& GetBodySpan()
    {
        SymbolEntryFunction;
        return m_BodySpan;
    }


#if IDE 

    // These members are only used internally by the PE generation code.
    void GetImage(BYTE **ImageOut, unsigned *ImageSizeOut)
    {
        SymbolEntryFunction;
        *ImageOut = m_Image;
        *ImageSizeOut = m_ImageSize;
    }
    void SetImage(BYTE *Image, unsigned ImageSize)
    {
        SymbolEntryFunction;
        m_Image = Image;
        m_ImageSize = ImageSize;
    }

#endif IDE

protected:

    ParseTree::StatementList * m_ParsedCode; // The synthesized code already parsed as parsetree.
    unsigned m_LocalsCount; // Locals count of the statementList of GetParsedCode.
    const   WCHAR * m_CodeText; // The synthesized code.
    unsigned m_CodeSize; // The size of the code - in characters.

    unsigned __int32 /* SyntheticKind */ m_SynthKind:5; // What kind of code to produce.
    bool m_CallBaseFirst :1; // Whether the synthetic could should include a call to the base class
    bool m_IsLambda :1; // Whether the synthetic is created for a Lambda
    bool m_IsRelaxedDelegateLambda :1; // Whether the synthetic is created for a lambda used in a relaxed delegate.
    bool m_IsResumable :1; // Whether the synthetic is created for an Iterator/Async method.  Should only be true for "MoveNext" method

    Location m_WholeSpan; // only applies to the Invokes for multiline lambdas. WholeSpan goes from the start...
    Location m_BodySpan;  // ... of "Function()" to the end of "End Function". BodySpan goes from the end of the
                          // former to the start of the latter.

    CSingleList<MethodDebugInfo> m_MethodDebugInfoList;
    AsyncMethodDebugInfo        * m_pAsyncMethodDebugInfo;
    BlockScope                  * m_MethodScope;

    // Signature token for the procedure. This is required because the token for the method signature
    // is generated during code gen, and we need to keep track of it and re-use the token in
    // pdb emit.

    mdSignature m_SignatureToken;

#if IDE 

    // The compiled image.
    BYTE * m_Image;
    unsigned m_ImageSize;

#endif IDE

}; // end of BCSYM_SynthesizedMethod

/**************************************************************************************************
;MethodDecl

Represents a procedure declaration with no body.
***************************************************************************************************/
class BCSYM_MethodDecl : public BCSYM_Proc
{
    DBGFRIENDS;
}; // end of BCSYM_MethodDecl


class BCSYM_LiftedOperatorMethod :
    public BCSYM_MethodDecl
{
    DBGFRIENDS;
public:
    BCSYM_Proc * GetActualProc() const
    {
        SymbolEntryFunction;
        return m_pActualProc;
    }
    void SetActualProc(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        m_pActualProc = value;
    }
protected:
    BCSYM_Proc * m_pActualProc; // The implementation of the operator lifted by this symbol
};

/**************************************************************************************************
;MethodImpl

Represents a Basic function with a code body.
***************************************************************************************************/
class BCSYM_MethodImpl : public BCSYM_MethodDecl
{
    DBGFRIENDS;
public:

    // return the list built from the HANDLES clause
    BCSYM_HandlesList * GetHandlesList() const
    {
        SymbolEntryFunction;
        return m_pHandlesList;
    }
    void SetHandlesList(BCSYM_HandlesList * value)
    {
        SymbolEntryFunction;
        m_pHandlesList = value;
    }

    const CodeBlockLocation* GetCodeBlock() const
    {
        SymbolEntryFunction;
        return &m_codeblock;
    }

    const CodeBlockLocation* GetProcBlock() const
    {
        SymbolEntryFunction;
        return &m_procblock;
    }

    static size_t GetCodeBlockOffset() 
    {
        return offsetof(BCSYM_MethodImpl, m_codeblock);
    }

    static  size_t GetProcBlockOffset() 
    {
        return offsetof(BCSYM_MethodImpl, m_procblock);
    }

    void SetCodeBlock( const CodeBlockLocation* pblock )
    {
        SymbolEntryFunction;
        m_codeblock.m_oBegin = pblock->m_oBegin;
        m_codeblock.m_oEnd = pblock->m_oEnd;
        m_codeblock.m_lBegLine = pblock->m_lBegLine;
        m_codeblock.m_lBegColumn = pblock->m_lBegColumn;
        m_codeblock.m_lEndLine = pblock->m_lEndLine;
        m_codeblock.m_lEndColumn = pblock->m_lEndColumn;
    }

    void SetProcBlock( const CodeBlockLocation* pblock )
    {
        SymbolEntryFunction;
        m_procblock.m_oBegin = pblock->m_oBegin;
        m_procblock.m_oEnd = pblock->m_oEnd;
        m_procblock.m_lBegLine = pblock->m_lBegLine;
        m_procblock.m_lBegColumn = pblock->m_lBegColumn;
        m_procblock.m_lEndLine = pblock->m_lEndLine;
        m_procblock.m_lEndColumn = pblock->m_lEndColumn;
    }

    inline void SetHandledEvent( BCSYM_EventDecl *EventDecl )
    {
        SymbolEntryFunction;
        m_HandledEvent = EventDecl;
    }

    inline BCSYM_EventDecl * GetHandledEvent() const
    {
        SymbolEntryFunction;
        return m_HandledEvent;
    }

#if IDE 

    // These members are only used internally by the PE
    // generation code.
    //
    void GetImage(BYTE **ppbImage, unsigned *pcbImageSize)
    {
        SymbolEntryFunction;
        *ppbImage = m_Image;
        *pcbImageSize = m_ImageSize;
    }
    void SetImage(BYTE *Image, unsigned ImageSize)
    {
        SymbolEntryFunction;
        m_Image = Image;
        m_ImageSize = ImageSize;
    }

#endif IDE

    void SetMethodScope(BlockScope * MethodScope)
    {
        SymbolEntryFunction;
        m_MethodScope = MethodScope;
    }

    BlockScope *GetMethodScope() const
    {
        SymbolEntryFunction;
        return m_MethodScope;
    }


    CSingleList<MethodDebugInfo> *GetDebugInfoList() 
    {
        SymbolEntryFunction;
        return & m_MethodDebugInfoList;
    }

    void SetAsyncDebugInfo(AsyncMethodDebugInfo *pAsyncMethodDebugInfo)
    {
        SymbolEntryFunction;
        m_pAsyncMethodDebugInfo = pAsyncMethodDebugInfo;
    }

    AsyncMethodDebugInfo *GetAsyncDebugInfo()
    {
        SymbolEntryFunction;
        return m_pAsyncMethodDebugInfo;
    }

    void SetSignatureToken(mdSignature SignatureToken)
    {
        SymbolEntryFunction;
        m_SignatureToken = SignatureToken;
    }

    mdSignature GetSignatureToken() const
    {
        SymbolEntryFunction;
        return m_SignatureToken;
    }

protected:

    // Location of the code for this method.
    TrackedCodeBlock m_codeblock;
    TrackedCodeBlock m_procblock;

    // the list from the HANDLES clause
    BCSYM_HandlesList * m_pHandlesList;

    // the event that this method handles
    BCSYM_EventDecl * m_HandledEvent;

    // Signature token for the procedure. This is required because the token for the method signature
    // is generated during code gen, and we need to keep track of it and re-use the token in
    // pdb emit.

    mdSignature m_SignatureToken;

#if IDE 

    // The compiled image.
    BYTE * m_Image;
    unsigned m_ImageSize;

#endif IDE

    // Debug information.
    CSingleList<MethodDebugInfo>  m_MethodDebugInfoList;
    AsyncMethodDebugInfo        * m_pAsyncMethodDebugInfo;
    BlockScope                  * m_MethodScope;


}; // end of BCSYM_MethodImpl


/**************************************************************************************************
;UserDefinedOperator

Represents an overloaded operator declaration.
***************************************************************************************************/
class BCSYM_UserDefinedOperator : public BCSYM_MethodDecl
{
    DBGFRIENDS;
public:

    inline void SetOperator(UserDefinedOperators Operator)
    {
        SymbolEntryFunction;
        VSASSERT(Operator > OperatorUNDEF && Operator < OperatorMAXVALID,
                 "setting invalid operator");
        m_Operator = Operator;
    }

    inline UserDefinedOperators GetOperator() const
    {
        SymbolEntryFunction;
        return m_Operator;
    }

    BCSYM_MethodDecl * GetOperatorMethod() const
    {
        SymbolEntryFunction;
        return m_AssociatedMethod;
    }
    void SetOperatorMethod(BCSYM_MethodDecl * value)
    {
        SymbolEntryFunction;
        m_AssociatedMethod = value;
    }

private:

    // 




    BCSYM_Proc* GetAssociatedMethod() const;
    void SetAssociatedMethod(BCSYM_Proc * value);

protected:

    BCSYM_MethodDecl * m_AssociatedMethod;
    UserDefinedOperators m_Operator;
}; // end of BCSYM_UserDefinedOperator

/**************************************************************************************************
;Property

Represents a property block - the let/set
***************************************************************************************************/
class BCSYM_Property : public BCSYM_Proc
{
    DBGFRIENDS;
public:

    inline
    STRING * PropertyName() const
    {
        SymbolEntryFunction;
        return Name;
    }
    inline
    void SetPropertyName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        Name = value;
    }

    inline
    BCSYM_Proc * GetProperty() const
    {
        SymbolEntryFunction;
        return GetPropertySymbol;
    }
    inline
    void SetGetProperty(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        GetPropertySymbol = value;
    }

    inline
    BCSYM_Proc * SetProperty() const
    {
        SymbolEntryFunction;
        return SetPropertySymbol;
    }
    inline
    void SetSetProperty(BCSYM_Proc * value)
    {
        SymbolEntryFunction;
        SetPropertySymbol = value;
    }
    
    inline
    BCSYM_Proc * GetPropertySignature() const
    {
        SymbolEntryFunction;
        return (BCSYM_Proc*) this;
    }

    bool ReturnsEventSource(void); // i.e. the property is marked with an attribute that indicates it returns an object that sources events

    inline
    bool IsReadOnly() const // whether the property only contains a getter
    {
        SymbolEntryFunction;
        return ReadOnly;
    }
    inline
    void SetIsReadOnly(bool value) // whether the property only contains a getter
    {
        SymbolEntryFunction;
        ReadOnly = value;
    }

    inline
    bool IsWriteOnly() const // whether the property only contains a setter
    {
        SymbolEntryFunction;
        return WriteOnly;
    }
    inline
    void SetIsWriteOnly(bool value) // whether the property only contains a setter
    {
        SymbolEntryFunction;
        WriteOnly = value;
    }

    inline
    bool IsReadWrite() const // whether the property contains both a setter and a getter
    {
        SymbolEntryFunction;
        return !IsReadOnly() && !IsWriteOnly();
    }

    bool IsDefault() const
    {
        SymbolEntryFunction;
        return IsDefaultProperty;
    }
    void SetIsDefault(bool value)
    {
        SymbolEntryFunction;
        IsDefaultProperty = value;
    }

    bool IsExplicitlyNamed() const
    {
        SymbolEntryFunction;
        return ExplicitlyNamed;
    }
    void SetIsExplicitlyNamed(bool value)
    {
        SymbolEntryFunction;
        ExplicitlyNamed = value;
    }

    bool IsBangNamed() const
    {
        SymbolEntryFunction;
        return BangNamed;
    }
    void SetIsBangNamed(bool value)
    {
        SymbolEntryFunction;
        BangNamed = value;
    }

    bool IsFromAnonymousType() const
    {
        SymbolEntryFunction;
        return FromAnonymousType;
    }
    void SetIsFromAnonymousType(bool value)
    {
        SymbolEntryFunction;
        FromAnonymousType = value;
    }

    bool IsAutoProperty()
    {
        SymbolEntryFunction;
        return m_autoProperty;
    }

    void SetIsAutoProperty(bool value)
    {
        SymbolEntryFunction;
        m_autoProperty = value;
    }

    bool IsNewAutoProperty()
    {
        SymbolEntryFunction;
        return m_isNewAutoProperty;
    }

    void SetNewAutoProperty( bool value)
    {
        SymbolEntryFunction;
        m_isNewAutoProperty = value;
    }   

    inline
    void SetCreatedByHandlesClause(BCSYM_HandlesList * Handles)
    {
        SymbolEntryFunction;
        m_CreatedByHandlesClause = Handles;
    }

    inline
    BCSYM_HandlesList * CreatedByHandlesClause() const
    {
        SymbolEntryFunction;
        return m_CreatedByHandlesClause;
    }

#if IDE 
    // Only needed by IDE to track backing field from property symbol.
    
    void SetAutoPropertyBackingField( BCSYM_Variable* pValue)
    {
        SymbolEntryFunction;
        m_pAutoPropertyBackingField = pValue;
    }

    BCSYM_Variable*
    GetAutoPropertyBackingField()
    {
        SymbolEntryFunction;
        return m_pAutoPropertyBackingField;
    }
#endif

protected:

    STRING * Name; // Name of the property
    BCSYM_Proc * GetPropertySymbol; 
    BCSYM_Proc * SetPropertySymbol;
    BCSYM_HandlesList * m_CreatedByHandlesClause;    // this is set to the handles clause that caused the synthesis of this property because
                                                    // of handling an event of a base withevents variable
#if IDE 
    BCSYM_Variable *m_pAutoPropertyBackingField; // Only needed by IDE to track backing field from property symbol.
#endif

    bool ReadOnly :1; // Whether the property was marked as ReadOnly
    bool WriteOnly :1; // Whether the property was marked as WriteOnly
    bool IsDefaultProperty : 1;
    bool ExplicitlyNamed : 1; // Only used for properties created inside anonymous types in vb code.
    bool BangNamed       : 1; // Only used for properties created inside anonymous types in vb code.
    bool FromAnonymousType : 1; // Only used for properties created inside anonymous types.
    bool m_autoProperty : 1; // Only used for auto properties.
    bool m_isNewAutoProperty : 1; // Only used for auto properties, set if the auto property is declared with "As New" or "= New".
}; // end of BCSYM_Property

/**************************************************************************************************
;EventDecl

Symbol for an Event Declaration
***************************************************************************************************/
class BCSYM_EventDecl : public BCSYM_MethodDecl
{
    DBGFRIENDS;
public:

    // GetDelegate returns either a BCSYM_Class or a generic binding that binds a BCSYM_Class.
    BCSYM * GetDelegate()
    {
        SymbolEntryFunction;
        // Dev10#489103: Orcas used to report !IsBad() for a NamedType which pointed to a bad type,
        // and so it checked IsBad() || DigThroughNamedType()->IsBad().
        // But (1) we've changed IsBad() to return true for such a type, and (2) the first IsBad()
        // is in any case redundant, as shown by the VSASSERT:
#if DEBUG
        if ( GetRawDelegate() )
        {
            VSASSERT(!(GetRawDelegate()->IsBad() && !GetRawDelegate()->DigThroughNamedType()->IsBad()), "internal logic error: IsBad(), then DigThroughNamedType()->IsBad() must also be true.");
        }
#endif

        // Setting that aside, and so as to be scrupulous about not changing behavior, we've changed the order.
        // That's because DigThroughNamedType has side effects when given a named type, but IsBad does not.
        return !(GetRawDelegate() && ( GetRawDelegate()->DigThroughNamedType()->IsBad() || GetRawDelegate()->IsBad())) ? GetRawDelegate()->DigThroughNamedType() : NULL;
    }

    inline
    BCSYM * GetRawDelegate() const
    {
        SymbolEntryFunction;
        return m_pDelegate;
    }

    inline
    void SetDelegate(BCSYM * pDelegate)
    {
        SymbolEntryFunction;
        m_pDelegate = pDelegate;
    }

#if IDE
    inline SetDeclaredDelegate(BCSYM * pDelegate)
    {
        SymbolEntryFunction;
        m_pDeclaredDelegate = pDelegate;
    }

    inline SetDeclaredDelegateVariableType(BCSYM * pDelegateVariableType)
    {
        SymbolEntryFunction;
        ThrowIfNull(pDelegateVariableType);
        if (!m_pDeclaredDelegateVariableType)
        {
            // There's two codepaths that can call this method, but whoever called it first
            // had the type that was actually valid at CS_Declared.
            m_pDeclaredDelegateVariableType = pDelegateVariableType;
        }
    }

    void ReplaceDelegateByDeDeclaredDelegate();
#endif

    BCSYM_Proc * GetProcAdd() const
    {
        SymbolEntryFunction;
        return m_pprocAdd;
    }

    inline
    void SetProcAdd(BCSYM_Proc * pprocAdd)
    {
        SymbolEntryFunction;
        m_pprocAdd = pprocAdd;
    }

    inline
    BCSYM_Proc * GetProcRemove() const
    {
        SymbolEntryFunction;
        return m_pprocRemove;
    }

    inline
    void SetProcRemove(BCSYM_Proc * pprocRemove)
    {
        SymbolEntryFunction;
        m_pprocRemove = pprocRemove;
    }

    inline
    BCSYM_Proc * GetProcFire() const
    {
        SymbolEntryFunction;
        return m_pprocFire;
    }

    inline
    void SetProcFire(BCSYM_Proc * pprocFire)
    {
        SymbolEntryFunction;
        m_pprocFire = pprocFire;
    }

    inline
    BCSYM_Variable * GetDelegateVariable() const
    {
        SymbolEntryFunction;
        return m_DelegateVariable;
    }

    inline
    void SetDelegateVariable(BCSYM_Variable * DelegateVariable)
    {
        SymbolEntryFunction;
        m_DelegateVariable = DelegateVariable;
    }

    inline
    void SetParametersObtainedFromDelegate(bool AreParametersObtainedFromDelegate)
    {
        SymbolEntryFunction;
        m_AreParametersObtainedFromDelegate = AreParametersObtainedFromDelegate;
    }

    inline
    bool AreParametersObtainedFromDelegate() const
    {
        SymbolEntryFunction;
        return m_AreParametersObtainedFromDelegate;
    }

    inline
    bool IsDelegateFromImplements() const
    {
        SymbolEntryFunction;
        return m_IsDelegateFromImplements;
    }

    inline
    void SetIsDelegateFromImplements(bool fFromImplements)
    {
        SymbolEntryFunction;
        m_IsDelegateFromImplements = fFromImplements;
    }

    inline
    bool IsBlockEvent() const
    {
        SymbolEntryFunction;
        return m_IsBlockEvent;
    }

    inline
    void SetIsBlockEvent()
    {
        SymbolEntryFunction;
        m_IsBlockEvent = true;
    }

    inline
    bool IsWindowsRuntimeEvent() const
    {
        return m_IsWindowsRuntimeEvent;
    }

    inline
    void SetIsWindowsRuntimeEvent(bool IsWindowsRuntimeEvent)
    {
        m_IsWindowsRuntimeEvent = IsWindowsRuntimeEvent;
    }

    inline
    void SetParamList(BCSYM_Param *pParam)
    {
        SetFirstParam(pParam);
        m_IsParamListFromBuildEvents = true;
    }

    inline
    void ClearParamList()
    {
        // Only clear the parameters if they were created in Bindable::BuildEvents
        if (m_IsParamListFromBuildEvents)
        {
            SetFirstParam(NULL);
            m_IsParamListFromBuildEvents = false;
        }
    }

    // Get the DocComment Signature for the Event Declaration.
    void GetDocCommentSignature(
        Compiler * pCompiler,
        BCSYM_NamedRoot * pParent,
        StringBuffer * pSignature);

    void ConvertToWindowsRuntimeEvent(_In_ CompilerHost *pCompilerHost, _In_ Symbols *pSymbolCreator);

#if IDE
    void RestoreDeclaredDelegateVariableForWinRT();
#endif

protected:

#if IDE
    BCSYM * m_pDeclaredDelegate;
    BCSYM * m_pDeclaredDelegateVariableType;
#endif 

    BCSYM * m_pDelegate;
    BCSYM_Proc *m_pprocAdd;
    BCSYM_Proc *m_pprocRemove;
    BCSYM_Proc *m_pprocFire;
    BCSYM_Variable *m_DelegateVariable;

    bool m_AreParametersObtainedFromDelegate : 1;   // Do the list of parameters for the Event come from a delegate?
    bool m_IsDelegateFromImplements : 1;            // Does the delegate for the event come from an event that it is implementing ?
    bool m_IsBlockEvent : 1;                        // Is this a block event i.e. an event with custom add, remove and raise method ?
    bool m_IsWindowsRuntimeEvent : 1;               // Is this a Windows Runtime Event
    bool m_IsParamListFromBuildEvents : 1;          // The parameter list came from Bindable::BuildEvents

}; // end of BCSYM_EventDecl

enum DECLAREKIND
{
    DECLARE_Ansi,
    DECLARE_Unicode,
    DECLARE_Auto,
};

/**************************************************************************************************
;DllDeclare

Represents a VB Declare statement
***************************************************************************************************/
class BCSYM_DllDeclare : public BCSYM_Proc
{
    DBGFRIENDS;
public:
    STRING * GetLibName() const
    {
        SymbolEntryFunction;
        return m_pstrLibName;
    }
    void SetLibName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrLibName = value;
    }

    STRING * GetAliasName() const
    {
        SymbolEntryFunction;
        return m_pstrAliasName;
    }
    void SetAliasName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrAliasName = value;
    }

    DECLAREKIND GetDeclareType() const
    {
        SymbolEntryFunction;
        return m_ndType;
    }
    void SetDeclareType(DECLAREKIND value)
    {
        SymbolEntryFunction;
        m_ndType = value;
    }

protected:
    STRING     *m_pstrLibName;
    STRING     *m_pstrAliasName;

    DECLAREKIND m_ndType;

}; // end of BCSYM_DllDeclare


/**************************************************************************************************
;Variable

Represents a variable
***************************************************************************************************/
class BCSYM_Variable : public BCSYM_Member
{
    DBGFRIENDS;
public:

    bool IsStatic() const
    {
        SymbolEntryFunction;
        return m_isStatic;
    }
    void SetStatic(bool value)
    {
        SymbolEntryFunction;
        m_isStatic = value;
    }

    bool IsMe() const
    {
        SymbolEntryFunction;
        return (bool)m_IsMe;
    }

    void SetIsMe(bool value)
    {
        SymbolEntryFunction;
        m_IsMe = value;
    }

    VARIABLEKIND GetVarkind() const
    {
        SymbolEntryFunction;
        return (VARIABLEKIND)m_varkind;
    }

    void SetVarkind(VARIABLEKIND VarKind)
    {
        SymbolEntryFunction;
        m_varkind = VarKind;
    }

    // Is New variable
    bool IsNew() const
    {
        SymbolEntryFunction;
        return m_isNew;
    }
    void SetNew(bool value)
    {
        SymbolEntryFunction;
        m_isNew = value;
    }

    // Was this variable declared with a specific type?
    bool IsExplicitlyTyped() const
    {
        SymbolEntryFunction;
        return m_isExplicitlyTyped;
    }

    void SetExplicitlyTyped(bool isExplicitlyTyped)
    {
        SymbolEntryFunction;
        m_isExplicitlyTyped = isExplicitlyTyped;
    }

    // Was this variable implicitly declared (ie no decl; just used)
    bool IsImplicitDecl() const
    {
        SymbolEntryFunction;
        return m_isImplicitDecl;
    }

    void SetImplicitDecl(bool isImplicitDecl)
    {
        SymbolEntryFunction;
        m_isImplicitDecl = isImplicitDecl;
    }

    bool IsConstant() const
    {
        SymbolEntryFunction;
        return GetVarkind() == VAR_Const;
    }

    bool IsReadOnly() const
    {
        SymbolEntryFunction;
        return m_isReadOnly;
    }
    void SetReadOnly(bool value)
    {
        SymbolEntryFunction;
        m_isReadOnly = value;
    }
    
    bool IsWithEvents() const
    {
        SymbolEntryFunction;
        return m_isWithEvents;
    }
    void SetIsWithEvents(bool value)
    {
        SymbolEntryFunction;
        m_isWithEvents = value;
    }

    bool IsAutoPropertyBackingField()
    {
        SymbolEntryFunction;
        return m_isAutoPropertyBackingField;
    }

    bool IsBadVariableType()
    {
        SymbolEntryFunction;
        return m_isBadVariableType;
    }
    void SetIsBadVariableType(bool value)
    {
        SymbolEntryFunction;
        m_isBadVariableType = value;
    }

    ParseTree::AttributeSpecifierList * GetAttributesTree() const
    {
        SymbolEntryFunction;
        return m_AttributesTree;
    }

    void StowAttributeTree(ParseTree::AttributeSpecifierList * AttributeTree)
    {
        SymbolEntryFunction;
        ThrowIfTrue(IsTemporary());
        m_AttributesTree = AttributeTree;
    }

    bool IsParameterLocal() const
    {
        SymbolEntryFunction;
        return GetVarkind() == VAR_Param;
    }

    bool IsFunctionResultLocal() const
    {
        SymbolEntryFunction;
        return GetVarkind() == VAR_FunctionResult;
    }
    bool IsTemporary() const
    {
        SymbolEntryFunction;
        return (bool)m_isTemporary;
    }

    void SetIsTemporary()
    {
        SymbolEntryFunction;
        m_isTemporary = true;
    }

    void SetTemporaryManager(TemporaryManager *tempmgr);

    TemporaryManager *GetTemporaryManager()
    {
        SymbolEntryFunction;
        ThrowIfFalse(IsTemporary());
        return m_temporaryMgr;
    }

    Temporary *GetTemporaryInfo();

    bool IsMyGenerated() const
    {
        SymbolEntryFunction;
        return m_isMyGenerated;
    }
    void SetIsMyGenerated(bool value)
    {
        SymbolEntryFunction;
        m_isMyGenerated = value;
    }

    bool IsLambdaMember() const
    {
        SymbolEntryFunction;
        return (bool)m_IsLambdaMember;
    }

    void SetIsLambdaMember(bool isLambdaParam)
    {
        SymbolEntryFunction;
        m_IsLambdaMember = isLambdaParam;
    }

    bool IsQueryRecord() const
    {
        SymbolEntryFunction;
        return (bool)m_IsQueryRecord;
    }

    void SetIsQueryRecord(bool isQueryRecord)
    {
        SymbolEntryFunction;
        m_IsQueryRecord = isQueryRecord;
    }

    bool FieldCausesStructCycle() const
    {
        SymbolEntryFunction;
        return (bool)m_FieldCausesStructCycle;
    }

    void SetFieldCausesStructCycle(bool FieldCausesStructCycle)
    {
        SymbolEntryFunction;
        m_FieldCausesStructCycle = FieldCausesStructCycle;
    }

    void SetIsUsed()
    {
        SymbolEntryFunction;
        m_isUsed = 1;
    }

    void CleanIsUsed()
    {
        SymbolEntryFunction;
        m_isUsed = 0;
    }


    bool IsUsed() const
    {
        SymbolEntryFunction;
        return (bool)m_isUsed;
    }

    //


#if IDE 
    void SetIsENCTrackingList()
    {
        SymbolEntryFunction;
        m_IsENCTrackingList = true;
    }

    bool IsENCTrackingList() const
    {
        SymbolEntryFunction;
        return m_IsENCTrackingList;
    }
#endif IDE

    // used by code generator
    unsigned GetLocalSlot()
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        return m_LocalSlot;
    }

    void SetLocalSlot(unsigned LocalSlot)
    {
        SymbolEntryFunction;
        VSASSERT( !IsPartialTypeAndHasMainType(),
                    "Partial type unexpected!!!");

        m_LocalSlot = LocalSlot;
    }

    unsigned GetDefAsgSlot() const
    {
        SymbolEntryFunction;
        return m_DefAsgSlot;
    }
    void SetDefAsgSlot(unsigned value)
    {
        SymbolEntryFunction;
        m_DefAsgSlot = value;
    }

    unsigned GetDefAsgSize() const
    {
        SymbolEntryFunction;
        return m_DefAsgSize;
    }
    void SetDefAsgSize(unsigned value)
    {
        SymbolEntryFunction;
        m_DefAsgSize = value;
    }

    bool HasNullableDecoration() const
    {
        SymbolEntryFunction;
        return m_hasNullableDecoration;
    }
    void SetHasNullableDecoration(bool value)
    {
        SymbolEntryFunction;
        m_hasNullableDecoration = value;
    }

    void SetSuppressAsClauseMessages()
    {
        SymbolEntryFunction;
        m_suppressAsClauseMessages = true;
    }

    bool SuppressAsClauseMessages() const
    {
        SymbolEntryFunction;
        return m_suppressAsClauseMessages;
    }

    void SetIsAutoPropertyBackingField(bool value)
    {
        SymbolEntryFunction;
        m_isAutoPropertyBackingField = value;
    }

    STRING *GetRewrittenName() const
    {
        SymbolEntryFunction;
        return m_pstrRewrittenName;
    }

    void SetRewrittenName(_In_z_ STRING *name)
    {
        SymbolEntryFunction;
        m_pstrRewrittenName = name;
    }

#if HOSTED
    void SetIsFromScriptScope(bool value)
    {
        SymbolEntryFunction;
        m_IsFromScriptScope = value;
    }

    bool IsFromScriptScope() const
    {
        SymbolEntryFunction;
        return m_IsFromScriptScope;
    }
#endif


protected:
    unsigned m_DefAsgSlot;
    unsigned m_DefAsgSize;

    // The kind of this variable.
    unsigned __int32 /* VARIABLEKIND */ m_varkind:3;

    // Was this declared with "new".
    unsigned __int32 m_isNew:1;
    unsigned __int32 m_isExplicitlyTyped:1;
    unsigned __int32 m_isImplicitDecl:1;       // there was no Dim; it was just used
    unsigned __int32 m_isReadOnly:1;
    unsigned __int32 m_isStatic:1;
    unsigned __int32 m_IsMe:1; // whether this is named 'me' - true, vs. '[me]' or anything else - false
    unsigned __int32 m_isWithEvents:1; // whether this is a withevents created field
    unsigned __int32 m_isAutoPropertyBackingField:1; //whether this is an auto property backing field.
    unsigned __int32 m_isBadVariableType:1; // is this variable declared of a bad type?
    unsigned __int32 m_isTemporary:1;   //temporary variable created by temp manager
    unsigned __int32 m_isMyGenerated:1;   //synthetic field created by My Group
    unsigned __int32 m_FieldCausesStructCycle:1;    // Does this field cause a struct member cycle i.e. circularity in struct layout ?

    unsigned __int32 m_IsQueryRecord:1;               // This variable is created to represent iteration variable - record for a Query
    unsigned __int32 m_IsLambdaMember:1;             // This variable is created to represent Lambda's parameter

#if IDE 
    unsigned __int32 m_IsENCTrackingList:1; // Field created for ENC object tracking purposes
#endif IDE

    unsigned __int32 m_isUsed:1;            // tracks unused local variables
    unsigned __int32 m_hasNullableDecoration:1; //indicates wether or not the variable was declared with a "?" type modifier.
                                                //This is specifically not named "m_isNullable" to avoid confusion about is purpose.
                                                //In particular this is not useful for determining wether or not the variable
                                                //is of a nullable type. To do this call IsNullableType on the variable's type.
                                                //Instead this field just indicates wether or not the variable declaration was decorated with a
                                                //"?" or not.
    unsigned __int32 m_suppressAsClauseMessages : 1; //when true indicates that error and warning messages about missing as clauses should not be specified

#if HOSTED
    unsigned __int32 m_IsFromScriptScope:1; // Field created for variable from Script Scope
#endif

    BCSYM_Expression *m_pexpr; // Only accessible from VariableWithValue
    STRING *m_pstrRewrittenName;

    union
    {
        ParseTree::AttributeSpecifierList * m_AttributesTree; // we need to hang on to the tree in declared when generating withevent synthetic code
        TemporaryManager * m_temporaryMgr;   // Only valid on temporary variables
    };

    // used by the code generator
    unsigned m_LocalSlot;
}; // end of BCSYM_Variable

/**************************************************************************************************
;VariableWithValue

Represents a variable with an expression.
***************************************************************************************************/
class BCSYM_VariableWithValue : public BCSYM_Variable
{
    DBGFRIENDS;
public:
    // The default value expression.
    BCSYM_Expression *GetExpression() const
    {
        SymbolEntryFunction;
        return m_pexpr;
    }
    void SetExpression(BCSYM_Expression * value)
    {
        SymbolEntryFunction;
        m_pexpr = value;
    }

protected:
    // It is imperative that there be NO fields in VariableWithValue. The problem
    // is that when importing metadata, there is no way to know before we
    // create the symbol whether the Variable will have a value or not (because
    // decimal values are stored as attributes). So we have to be able to
    // morph a Variable into a VariableWithValue without reallocating.

    // 

};

/**************************************************************************************************
;VariableWithArraySizes

A variable with explicit array size information in its declarator.
***************************************************************************************************/
class BCSYM_VariableWithArraySizes : public BCSYM_Variable
{
    DBGFRIENDS;
public:
    // The default value expression.
    BCSYM_Expression *GetArraySize( unsigned Dimension );

    void SetArraySize( unsigned Dimension, BCSYM_Expression *Expr )
    {
        SymbolEntryFunction;
        m_ArraySizes[Dimension] = Expr;
    }

protected:
    #pragma warning (suppress  : 4200)
    BCSYM_Expression *m_ArraySizes[0];
};

/**************************************************************************************************
;CCConstant

Represents a conditional compilation constant
***************************************************************************************************/
class BCSYM_CCConstant : public BCSYM_VariableWithValue
{
    DBGFRIENDS;
public:

    BCSYM_CCConstant *GetNextCCConstant() const
    {
        SymbolEntryFunction;
        return m_NextCCConstant;
    }

    void SetNextCCConstant(BCSYM_CCConstant *NextCCConstant)
    {
        SymbolEntryFunction;
        m_NextCCConstant = NextCCConstant;
    }

protected:
    BCSYM_CCConstant *m_NextCCConstant;
};

/**************************************************************************************************
;StaticLocalBackingField

Represents a backing class field for a static local variable, or the
associated init flag. (Both of these are treated as backing fields in order
to get their metadata names encoded properly.)
***************************************************************************************************/
class BCSYM_StaticLocalBackingField : public BCSYM_Variable
{
    DBGFRIENDS;
public:

    void SetProcDefiningStatic( BCSYM_Proc *ProcDefiningStatic )
    {
        SymbolEntryFunction;
        m_ProcDefiningStatic = ProcDefiningStatic;
    }

    BCSYM_Proc *GetProcDefiningStatic() const
    {
        SymbolEntryFunction;
        return m_ProcDefiningStatic;
    }

    void SetInitFlag( BCSYM_StaticLocalBackingField * InitFlag )
    {
        SymbolEntryFunction;
        m_InitFlag = InitFlag;
        InitFlag->SetIsInitFlag(true);
    }

    BCSYM_StaticLocalBackingField * GetInitFlag() const
    {
        SymbolEntryFunction;
        return m_InitFlag;
    }

    bool IsInitFlag() const
    {
        SymbolEntryFunction;
        return m_IsInitFlag;
    }
    void SetIsInitFlag(bool value)
    {
        SymbolEntryFunction;
        m_IsInitFlag = value;
    }

protected:

    BCSYM_Proc *m_ProcDefiningStatic;

    // The Init Flag represents a field associated with the static local that
    // holds the state of the static local's initialization, with the following
    // possible values:
    //
    //      0           Initialization has not begun.
    //      2           Initialization is in progress.
    //      1           Initialization is complete.

    BCSYM_StaticLocalBackingField * m_InitFlag;
    bool m_IsInitFlag;
};

/**************************************************************************************************
;Param

Represents a parameter to a function.

Note: If you add any new fields to this class, you may need to make changes to Declared::CloneParameters()
since we are doing a shalow copy of BCSYM_Params in there.
***************************************************************************************************/
class BCSYM_Param : public BCSYM
{
    DBGFRIENDS;
public:
    // The next parameter in the parameter list.
    BCSYM_Param *GetNext() const
    {
        SymbolEntryFunction;
        return m_pparamNext;
    }
    void SetNext(BCSYM_Param *next)
    {
        SymbolEntryFunction;
        m_pparamNext = next;
    }

    // The default value expression.
    BCSYM_Expression *GetExpression() const
    {
        SymbolEntryFunction;
        return m_pexpr;
    }
    void SetExpression(BCSYM_Expression * value)
    {
        SymbolEntryFunction;
        m_pexpr = value;
    }


    // a paramarray parameter
    bool IsParamArray();

    // Is this an optional parameter?
    bool IsOptional() const
    {
        SymbolEntryFunction;
        return m_isOptional;
    }
    void SetIsOptional(bool value)
    {
        SymbolEntryFunction;
        m_isOptional = value;
    }

    // Is this a parameter which will be marshaled to a VB6 object?
    bool IsMarshaledAsObject() const
    {
        SymbolEntryFunction;
        return m_isMarshaledAsObject;
    }
    void SetIsMarshaledAsObject(bool value)
    {
        SymbolEntryFunction;
        m_isMarshaledAsObject = value;
    }

    bool IsOptionCompare() const
    {
        return m_isOptionCompare;
    }
    void SetIsOptionCompare(bool value)
    {
        SymbolEntryFunction;
        m_isOptionCompare = value;
    }

    bool IsByRefKeywordUsed() const
    {
        SymbolEntryFunction;
        return m_isByRefKeywordUsed;
    }
    void SetIsByRefKeywordUsed(bool value)
    {
        SymbolEntryFunction;
        m_isByRefKeywordUsed = value;
    }

    bool IsReturnType() const
    {
        SymbolEntryFunction;
        return m_isReturnType;
    }
    void SetIsReturnType(bool value)
    {
        SymbolEntryFunction;
        m_isReturnType = value;
    }

    // The metadata token for this item.
    mdToken GetToken() const
    {
        SymbolEntryFunction;
        return m_tk;
    }
    void SetToken(mdToken value)
    {
        SymbolEntryFunction;
        m_tk = value;
    }

    // Partial methods should never emit COM attributes.
    mdToken GetComClassToken()
    {
        SymbolEntryFunction;
        if( IsPartialMethodParam() )
        {
            return 0;
        }
        else
        {
            return m_tkComClass;
        }
    }
    void SetComClassToken(mdToken value)
    {
        SymbolEntryFunction;
        m_tkComClass = value;
    }

    STRING *GetName() const
    {
        SymbolEntryFunction;
        return m_pstrName;
    }

    void SetName(_In_z_ STRING *name)
    {
        SymbolEntryFunction;
        m_pstrName = name;
    }

    BCSYM *GetRawType() const
    {
        SymbolEntryFunction;
        return m_ptyp;
    }

    BCSYM *GetType()
    {
        SymbolEntryFunction;
        return m_ptyp->DigThroughNamedType();
    }

    void SetType( BCSYM* type )
    {
        SymbolEntryFunction;
        m_ptyp = type;
    }

    BCSYM *GetCompilerType();

    AttrVals *GetPAttrVals() const
    {
        SymbolEntryFunction;
        return m_pattrvals;    // Can return NULL
    }
    void SetPAttrVals(AttrVals * value)
    {
        SymbolEntryFunction;
        m_pattrvals = value;    // Can return NULL
    }

    WellKnownAttrVals *GetPWellKnownAttrVals();

    bool IsBadParam() const
    {
        SymbolEntryFunction;
        return m_isBad;
    }

    void SetIsBad()
    {
        SymbolEntryFunction;
        m_isBad = true;
    }

    void ClearIsBad()
    {
        SymbolEntryFunction;
        m_isBad = false;
    }

    bool AreAttributesEmitted() const
    {
        SymbolEntryFunction;
        return m_AreAttributesEmitted;
    }
    void SetAreAttributesEmitted(bool value)
    {
        SymbolEntryFunction;
        m_AreAttributesEmitted = value;
    }

    bool IsQueryIterationVariable() const
    {
        SymbolEntryFunction;
        return m_IsQueryIterationVariable;
    }

    void SetIsQueryIterationVariable(bool b)
    {
        SymbolEntryFunction;
        m_IsQueryIterationVariable = b;
    }

    bool IsRelaxedDelegateVariable() const
    {
        SymbolEntryFunction;
        return m_IsRelaxedDelegateVariable;
    }

    void SetIsRelaxedDelegateVariable(bool b)
    {
        SymbolEntryFunction;
        m_IsRelaxedDelegateVariable = b;
    }

    bool IsQueryRecord() const
    {
        SymbolEntryFunction;
        return m_IsQueryRecord;
    }

    void SetIsQueryRecord(bool b)
    {
        SymbolEntryFunction;
        m_IsQueryRecord = b;
    }

    bool IsPartialMethodParam()
    {
        SymbolEntryFunction;
        return m_IsPartialMethodDeclaration || m_IsPartialMethodImplementation;
    }

    bool IsPartialMethodParamDeclaration() const
    {
        SymbolEntryFunction;
        return m_IsPartialMethodDeclaration;
    }
    void SetIsPartialMethodDeclaration(bool value)
    {
        SymbolEntryFunction;
        m_IsPartialMethodDeclaration = value;
    }

    bool IsPartialMethodParamImplementation() const
    {
        SymbolEntryFunction;
        return m_IsPartialMethodImplementation;
    }
    void SetIsPartialMethodParamImplementation(bool value)
    {
        SymbolEntryFunction;
        m_IsPartialMethodImplementation = value;
    }

    BCSYM_Param* GetAssociatedParam()
    {
        SymbolEntryFunction;
        VSASSERT( IsPartialMethodParam(), "Must be partial method param!" );
        return( m_CorrespondingParam );
    }
    BCSYM_Param* GetAssociatedParamRaw()
    {
        SymbolEntryFunction;
        return( m_CorrespondingParam );
    }
    void SetAssociatedParam(BCSYM_Param* value)
    {
        SymbolEntryFunction;
        m_CorrespondingParam = value;
    }

    bool IsParamArrayRaw() const
    {
        SymbolEntryFunction;
        return m_isParamArray;
    }
    void SetIsParamArray(bool value)
    {
        SymbolEntryFunction;
        m_isParamArray = value;
    }


    // Bindable will call this to link the parameters up.

    void SetImplementationForPartialParam(BCSYM_Param * par)
    {
        SymbolEntryFunction;
        VSASSERT( m_IsPartialMethodDeclaration == false, "Decompilation error on params?" );
        VSASSERT( m_IsPartialMethodImplementation == false, "Decompilation error on params?" );
        VSASSERT( m_CorrespondingParam == NULL, "Decompilation error on params?" );
        VSASSERT( par->IsPartialMethodParamDeclaration() == false, "Decompilation error on params?" );
        VSASSERT( par->IsPartialMethodParamImplementation() == false, "Decompilation error on params?" );
        VSASSERT( par->m_CorrespondingParam == NULL, "Decompilation error on params?" );

        SetIsPartialMethodDeclaration(true);
        SetAssociatedParam(par);
        par->SetIsPartialMethodParamImplementation(true);
        par->SetAssociatedParam(this);
    }

    // Bindable decompilation will call this to unlink the parameters that we setup.

    void ResetImplementationForPartialParam()
    {
        SymbolEntryFunction;
        SetIsPartialMethodDeclaration(false);

        if( GetAssociatedParamRaw() != NULL )
        {
            GetAssociatedParamRaw()->SetAssociatedParam(NULL);
            GetAssociatedParamRaw()->SetIsPartialMethodParamImplementation(false);
            SetAssociatedParam(NULL);
        }
    }

protected:
    // The next parameter in the parameter list.
    BCSYM_Param *m_pparamNext;

    STRING *m_pstrName;
    BCSYM *m_ptyp;

    // Any applied custom attributes (and their decoded values) get hung here
    AttrVals *m_pattrvals;

    // The default value expression, or NULL, only accessible from ParamWithValue
    BCSYM_Expression *m_pexpr;

    // The metadata token
    mdToken m_tk;

    // NOTE: for now, this is safe, because COM class members MUST be public,
    // and partial methods MUST be private. Thus, we can reuse the storage here
    // to avoid allocating an extra 4 bytes of data.

    union
    {
        // The com class metadata token
        mdToken m_tkComClass;

        // The parameter that corresponds with this parameter, in a partial method.
        BCSYM_Param* m_CorrespondingParam;
    };

    bool m_isParamArray : 1;
    bool m_isOptional : 1;
    bool m_isMarshaledAsObject : 1;
    bool m_isOptionCompare : 1;
    bool m_isByRefKeywordUsed : 1;
    bool m_isReturnType : 1;   // Is this param really encoding a function's return type?
    bool m_isBad : 1;          // Is this parameter bad?
    bool m_AreAttributesEmitted : 1; // Have the attributes for this symbol already been emitted ?

    unsigned __int32 m_IsQueryIterationVariable:1; // This parameter is created to represent iteration variable for a Query
    unsigned __int32 m_IsQueryRecord:1;               // This parameter is created to represent iteration variable - record for a Query (meaningless if m_IsQueryIterationVariable isn't set)
    unsigned __int32 m_IsRelaxedDelegateVariable:1; // This parmaeter is created to represent relaxed delegate variables such that we don't report name errors on these.
    unsigned __int32 m_IsPartialMethodDeclaration : 1;   // This parameter belongs to a partial method declaration.
    unsigned __int32 m_IsPartialMethodImplementation : 1; // This parameter belongs to a partial method implementation.
}; // end of BCSYM_Param

/**************************************************************************************************
;ParamWithValue

Represents a parameter to a function with an expression.

Note: If you add any new fields to this class, you may need to make changes to Declared::CloneParameters()
since we are doing a shalow copy of BCSYM_Params in there.
***************************************************************************************************/
class BCSYM_ParamWithValue : public BCSYM_Param
{
    DBGFRIENDS;
public:

protected:
    // It is imperative that there be NO fields in ParamWithValue. The problem
    // is that when importing metadata, there is no way to know before we
    // create the symbol whether the param will have a value or not (because
    // decimal values are stored as attributes). So we have to be able to
    // morph a Param into a ParamWithValue without reallocating.
};

/**************************************************************************************************
;GenericConstraint

Describes the constraints on a generic parameter.
***************************************************************************************************/
class BCSYM_GenericConstraint : public BCSYM
{
    DBGFRIENDS;
public:

    BCSYM_GenericConstraint *Next() const
    {
        SymbolEntryFunction;
        return m_pNext;
    }
    void SetNext(BCSYM_GenericConstraint * value)
    {
        SymbolEntryFunction;
        m_pNext = value;
    }

    BCSYM_GenericConstraint **GetNextConstraintTarget() 
    {
        SymbolEntryFunction;
        return &m_pNext;
    }

    bool IsNewConstraint();

    bool IsReferenceConstraint();

    bool IsValueConstraint();

     // 
    bool IsBadConstraint() const
    {
        SymbolEntryFunction;
        return m_fIsBadConstraint;
    }
    void SetIsBadConstraint(bool value)
    {
        SymbolEntryFunction;
        m_fIsBadConstraint = value;
    }

protected:

    BCSYM_GenericConstraint *m_pNext;
};

/**************************************************************************************************
;GenericTypeConstraint

Describes the type constraints on a generic parameter.
***************************************************************************************************/
class BCSYM_GenericTypeConstraint : public BCSYM_GenericConstraint
{
    DBGFRIENDS;
public:

    BCSYM *GetRawType()
    {
        SymbolEntryFunction;
        return m_pType;
    }

    BCSYM *GetType()
    {
        SymbolEntryFunction;
        return GetRawType()->DigThroughNamedType();
    }
    void SetType(BCSYM * value)
    {
        m_pType= value;
    }

    BCSYM_GenericTypeConstraint *Next()
    {
        SymbolEntryFunction;
        for(BCSYM_GenericConstraint *Constraint = this->PGenericConstraint()->Next();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsGenericTypeConstraint())
            {
                return Constraint->PGenericTypeConstraint();
            }
        }

        return NULL;
    }

protected:

    BCSYM * m_pType; // The type that is the constraint.
};

/**************************************************************************************************
;GenericNonTypeConstraint

Describes the non-type constraints on a generic parameter.
Currently the non-type constraints allowed are New, Class (any reference type) and Structure (any value type).

***************************************************************************************************/
class BCSYM_GenericNonTypeConstraint : public BCSYM_GenericConstraint
{
    DBGFRIENDS;
public:

    enum ConstraintKind
    {
        ConstraintKind_New,     // New constraint
        ConstraintKind_Ref,     // Class i.e. any reference type
        ConstraintKind_Value    // Structure i.e. any value type like structures or enums
    };

    bool IsNewConstraint() const
    {
        SymbolEntryFunction;
        return GetConstraintKind() == ConstraintKind_New;
    }

    bool IsReferenceConstraint() const
    {
        SymbolEntryFunction;
        return GetConstraintKind() == ConstraintKind_Ref;
    }

    bool IsValueConstraint() const
    {
        SymbolEntryFunction;
        return GetConstraintKind() == ConstraintKind_Value;
    }

    ConstraintKind GetConstraintKind() const
    {
        SymbolEntryFunction;
        return m_ConstraintKind;
    }
    void SetConstraintKind(ConstraintKind value)
    {
        SymbolEntryFunction;
        m_ConstraintKind = value;
    }
protected:

    ConstraintKind m_ConstraintKind;
};

/**************************************************************************************************
;GenericParam

Represents a parameter to a generic type or method.

***************************************************************************************************/
enum Variance_Kind : unsigned char
{
    Variance_None,  // invariant
    Variance_Out,   // "Out" - covariant
    Variance_In     // "In" - contravariant
    // nb. we pack it into a 2-bit bitfield, so don't add more to this enum.
};

class BCSYM_GenericParam : public BCSYM_NamedRoot
{
    DBGFRIENDS;
 public:


    // The next parameter in the parameter list.
    BCSYM_GenericParam *GetNextParam() const
    {
        SymbolEntryFunction;
        return m_pNextParam;
    }
    void SetNextParam(BCSYM_GenericParam * value)
    {
        SymbolEntryFunction;
        m_pNextParam = value;
    }

    BCSYM_GenericParam **GetNextParamTarget() 
    {
        SymbolEntryFunction;
        return &m_pNextParam;
    }

    BCSYM_GenericConstraint *GetConstraints()
    {
        SymbolEntryFunction;
        return m_pConstraints;
    }
    BCSYM_GenericConstraint **GetConstraintsTarget() 
    {
        SymbolEntryFunction;
        return &m_pConstraints;
    }

    void SetConstraints(BCSYM_GenericConstraint *Constraints)
    {
        SymbolEntryFunction;
        m_pConstraints = Constraints;
    }

    BCSYM_GenericTypeConstraint *GetTypeConstraints()
    {
        SymbolEntryFunction;
        for(BCSYM_GenericConstraint *Constraint = GetConstraints();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsGenericTypeConstraint())
            {
                return Constraint->PGenericTypeConstraint();
            }
        }

        return NULL;
    }

    // The lexical position of this parameter, counted from zero.
    unsigned GetPosition() const
    {
        SymbolEntryFunction;
        return m_uPosition;
    }
    void SetPosition(unsigned value)
    {
        SymbolEntryFunction;
        m_uPosition = value;
    }

    // The metadata position is the lexical position of this parameter, counted from zero
    // and beginning after the position of the last parameter of the parent of the symbol
    // owning this symbol.
    unsigned GetMetaDataPosition() const
    {
        SymbolEntryFunction;
        return m_uMetaDataPosition;
    }

    void SetMetaDataPosition(unsigned Position)
    {
        SymbolEntryFunction;
        m_uMetaDataPosition = Position;
    }

    Variance_Kind GetVariance() const
    {
        SymbolEntryFunction;
        return m_Variance;
    }

    void SetVariance(Variance_Kind value)
    {
        SymbolEntryFunction;
        m_Variance = value;
    }

    bool IsGenericMethodParam() const
    {
        SymbolEntryFunction;
        return m_fIsGenericMethodParam;
    }
    void SetIsGenericMethodParam(bool value)
    {
        SymbolEntryFunction;
        m_fIsGenericMethodParam = value;
    }

    bool HasNewConstraint()
    {
        SymbolEntryFunction;
        for(BCSYM_GenericConstraint *Constraint = GetConstraints();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (Constraint->IsNewConstraint())
            {
                return true;
            }
        }

        return false;
    }

    // Indicates whether type parameter T can used in a "new" expression as "new T".
    //
    bool CanBeInstantiated();

    bool HasReferenceConstraint()
    {
        SymbolEntryFunction;
        for(BCSYM_GenericConstraint *Constraint = GetConstraints();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (Constraint->IsReferenceConstraint())
            {
                return true;
            }
        }

        return false;
    }

    bool IsReferenceType();

private:
    static
    bool IsReferenceTypeHelper(
        GenericParameter * GenericParam,
        Compiler * Compiler,
        CompilerHost * CompilerHost);

public:

    bool HasValueConstraint()
    {
        SymbolEntryFunction;
        for(BCSYM_GenericConstraint *Constraint = GetConstraints();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            if (Constraint->IsValueConstraint())
            {
                return true;
            }
        }

        return false;
    }

    bool IsValueType();

    // The following functions do consider the constraints on any type parameters used
    // as constraint too. These should only be used after constraint validation for the
    // for this type param has been completed during the transition from CS_Declared to
    // CS_Bound.
    //
    bool HasClassConstraint(bool IgnoreNonReferenceTypes);

    BCSYM * GetClassConstraint(
        CompilerHost * CompilerHost,
        NorlsAllocator * Allocator,
        bool DigThroughToClassType,
        bool DigThroughToReferenceType = false);

#if DEBUG
    bool IsConstraintCycleCheckingDone() const
    {
        SymbolEntryFunction;
        return m_fIsConstraintCycleCheckingDone;
    }
    void SetConstraintCycleCheckingDone(bool CycleCheckingDone)
    {
        SymbolEntryFunction;
        m_fIsConstraintCycleCheckingDone = CycleCheckingDone;
    }
#endif

    void SetIsBadVariance(){ m_IsBadVariance = true;}
    bool IsBadVariance(){ return m_IsBadVariance;}

protected:

    // The next parameter in the parameter list.
    BCSYM_GenericParam *m_pNextParam;

    BCSYM_GenericConstraint *m_pConstraints;

    // The lexical position of this parameter, counted from zero.
    unsigned m_uPosition;

    // The metadata position is the lexical position of this parameter, counted from zero
    // and beginning after the position of the last parameter of the parent of the symbol
    // owning this symbol.
    unsigned m_uMetaDataPosition;

    // True for type parameters of generic methods; false for type parameters of generic classes.
    // This field seems ----, but turns out to be necessary for metadata generation.
    bool m_fIsGenericMethodParam : 1;

    bool m_IsBadVariance : 1;

    // The variance that this generic parameter was declared with
    Variance_Kind m_Variance;

#if DEBUG
    // True if constraint cycle checking for this generic param has been completed.
    bool m_fIsConstraintCycleCheckingDone : 1;
#endif

}; // end of BCSYM_GenericParam

/**************************************************************************************************
;GenericBinding

Represents a binding of a generic type or method with a list of arguments.

// 



*/
class BCSYM_GenericBinding : public BCSYM_NamedRoot
{
    DBGFRIENDS;
public:

    BCSYM_NamedRoot *GetGeneric() const
    {
        SymbolEntryFunction;
        return GetGenericRaw()->DigThroughNamedType()->PNamedRoot();
    }

    BCSYM *GetGenericRaw() const
    {
        SymbolEntryFunction;
        return m_pGeneric;
    }
    void SetGeneric(BCSYM * value) 
    {
        SymbolEntryFunction;
        m_pGeneric = value;
    }

    unsigned GetArgumentCount() const
    {
        SymbolEntryFunction;
        return m_ArgumentCount;
    }
    void SetArgumentCount(unsigned value)
    {
          SymbolEntryFunction;
      m_ArgumentCount = value;
    }

    BCSYM_GenericTypeBinding *GetParentBinding() const
    {
        SymbolEntryFunction;
        return m_pParentBinding;
    }
    void SetParentBinding(BCSYM_GenericTypeBinding * value)
    {
        SymbolEntryFunction;
        m_pParentBinding = value;
    }

    bool IsBadGenericBinding() const
    {
        SymbolEntryFunction;
        return m_isBad;
    }

    BCSYM **GetArguments() const
    {
        SymbolEntryFunction;
        return m_Arguments;
    }
    void SetArguments(BCSYM ** value)
    {
        SymbolEntryFunction;
        m_Arguments = value;
    }

    BCSYM *GetArgument(unsigned ArgumentIndex)
    {
        SymbolEntryFunction;
        VSASSERT(ArgumentIndex < GetArgumentCount(), "Attempt to access an out of range generic argument.");

        return GetArguments()[ArgumentIndex]->DigThroughNamedType();
    }

    BCSYM *GetCorrespondingArgument(BCSYM_GenericParam *Param);

    static
    bool CompareBindingArguments(
        BCSYM_GenericBinding * pLeft,
        BCSYM_GenericBinding * pRight);

    static
    bool AreBindingsEqual(
        BCSYM_GenericBinding * pLeft,
        BCSYM_GenericBinding * pRight);

protected:

    // The generic.
    BCSYM *m_pGeneric;

    // The arguments to the generic.
    BCSYM **m_Arguments;

    // The number of arguments.
    unsigned m_ArgumentCount;

    // The binding of the generic whose definition lexically encloses the definition of the unbound generic.
    // (For example, if this is a binding of a generic method in a generic class, the parent binding is the
    // binding of the generic class. If this is a binding of a generic class nested within another generic
    // class, the parent binding is the binding of the enclosing generic class.
    BCSYM_GenericTypeBinding *m_pParentBinding;
};

/**************************************************************************************************
;GenericTypeBinding

Represents a binding of a generic type with a list of arguments.

***************************************************************************************************/
class BCSYM_GenericTypeBinding : public BCSYM_GenericBinding
{
    DBGFRIENDS;
public:

    BCSYM_NamedRoot *GetGenericType()
    {
        SymbolEntryFunction;
        return GetGenericRaw()->DigThroughNamedType()->PNamedRoot();
    }

    static bool AreTypeBindingsEqual
    (
        BCSYM_GenericTypeBinding *pBinding1,
        BCSYM_GenericTypeBinding *pBinding2
    );

    // VS2011 bug 308452. IsEnumerableProxyType is true when 
    // wrapping a Linq result into SystemCore_EnumerableDebugView.
    // This bit tells debugger that don't show the actual type(SystemCore_EnumerableDebugView) 
    // during quick watch.
    bool IsEnumerableProxyType;
protected:

}; // end of BCSYM_GenericTypeBinding

/**************************************************************************************************
;Interface

Represents an interface
***************************************************************************************************/
class BCSYM_Interface : public BCSYM_Container
{
    DBGFRIENDS;
public:

    BCSYM_Implements *GetFirstImplements();

    // Does this class implement pinterface?
    bool DerivesFrom(
        Compiler * pCompiler,
        BCSYM_Interface * pinterfaceMaybeBase);

    // A Dispinterface is one to which all calls are late-bound.
    bool IsDispinterface();

    // If member not found:generate a late-bound call to that name.
    bool IsExtensible();

    GenericParams * GetGenericParams() const
    {
        SymbolEntryFunction;
        return m_GenericParams;
    }
    void SetGenericParams(GenericParams * value)
    {
        SymbolEntryFunction;
        m_GenericParams = value;
    }

    bool IsGeneric() const
    {
        SymbolEntryFunction;
        return GetGenericParams() != NULL;
    }

    BCSYM_GenericParam *GetFirstGenericParam()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_FirstGenericParam : NULL;
    }

    BCSYM_Hash *GetGenericParamsHash()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_GenericParamsHash : NULL;
    }

    BCSYM_Implements * GetImpList() const
    {
        SymbolEntryFunction;
        return m_----lList;
    }
    void SetImpList(BCSYM_Implements * value)
    {
        SymbolEntryFunction;
        m_----lList = value;
    }

protected:
    BCSYM_Implements    *m_----lList;

    GenericParams * m_GenericParams;
};

/**************************************************************************************************
;Implements

Holds an entry in an 'Implements' statement in a class.  Also used to hold an entry in an 'Inherits'
statement for an interface.  Also used for an 'Implements' statement on a method.  Confusing - yes?
Should investigate whether we just need a BCSYM_Inherits symbol - which I could use with classes as well.
***************************************************************************************************/
class BCSYM_Implements : public BCSYM
{
    DBGFRIENDS;
public:

    STRING *GetName() const
    {
        SymbolEntryFunction;
        return m_pstrName;
    }
    void SetName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrName = value;
    }

    BCSYM *GetRawRoot() const
    {
        SymbolEntryFunction;
        return m_psym;
    }
    void SetRawRoot(BCSYM * value)
    {
        SymbolEntryFunction;
        m_psym = value;
    }

    BCSYM *GetRoot()
    {
        SymbolEntryFunction;
        return GetRawRoot()->DigThroughNamedType();
    }

    BCSYM *GetCompilerRoot()
    {
        SymbolEntryFunction;
        return GetRawRoot()->DigThroughAlias();
    }

    BCSYM *GetInterfaceIfExists()
    {
        SymbolEntryFunction;
        return GetRawRoot()->DigThroughAliasToleratingNullResult();
    }

    BCSYM_Implements *GetNext() const
    {
        SymbolEntryFunction;
        return m_----lNext;
    }
    void SetNext(BCSYM_Implements * value)
    {
        SymbolEntryFunction;
        m_----lNext = value;
    }

    bool IsBadImplements() const
    {
        SymbolEntryFunction;
        return m_isBad;
    }

    void SetIsBad()
    {
        SymbolEntryFunction;
        m_isBad = true;
    }

    void ClearIsBad()
    {
        SymbolEntryFunction;
        m_isBad = false;
    }

    void SetIsRedundantImplements(bool IsRedundant)
    {
        SymbolEntryFunction;
        m_IsRedundantImplements = IsRedundant;
    }

    bool IsRedundantImplements() const
    {
        SymbolEntryFunction;
        return m_IsRedundantImplements;
    }

    void SetIsReimplementingInterface(bool IsReimplementing)
    {
        SymbolEntryFunction;
        m_IsReimplementingInterface = IsReimplementing;
    }

    bool IsReimplementingInterface() const
    {
        SymbolEntryFunction;
        return m_IsReimplementingInterface;
    }

    void SetAreAllMembersReimplemented(bool AreReimplemented)
    {
        SymbolEntryFunction;
        m_AreAllMembersReimplemented = AreReimplemented;
    }

    bool AreAllMembersReimplemented() const
    {
        SymbolEntryFunction;
        return m_AreAllMembersReimplemented;
    }

protected:
    STRING *m_pstrName;
    BCSYM *m_psym;

    BCSYM_Implements * m_----lNext;

    bool m_isBad : 1;

    // Indicates that the implements is redundant. This can happen when the same interface
    // is specified in the implements clauses of two partial types which is valid.
    //
    bool m_IsRedundantImplements : 1;

    // Indicates that the interface in this clause is re-implementing an interface already
    // implemented in a base class.
    bool m_IsReimplementingInterface : 1,
         m_AreAllMembersReimplemented:1;
};

/**************************************************************************************************
;HandlesList

Represents an event that a procedure "Handles"
***************************************************************************************************/
class BCSYM_HandlesList : public BCSYM
{
    DBGFRIENDS;
public:

    STRING * GetEventName()  const// just the name of the Event being handled
    {
        SymbolEntryFunction;
        return m_EventName;
    }

    void SetEventName(_In_z_ STRING *Name) // just the name of the Event being handled
    {
        SymbolEntryFunction;
        m_EventName = Name;
    }

    STRING * GetEventSourcePropertyName() const
    {
        SymbolEntryFunction;
        return m_EventSourcePropertyName;
    }

    void SetEventSourcePropertyName(_In_z_ STRING *Name)
    {
        SymbolEntryFunction;
        m_EventSourcePropertyName = Name;
    }

    void SetEvent(BCSYM_EventDecl *EventDecl)
    {
        SymbolEntryFunction;
        m_ActualEvent = EventDecl;
    }

    BCSYM_EventDecl *GetEvent() const
    {
        SymbolEntryFunction;
        return m_ActualEvent;
    }

    STRING * GetWithEventsVarName() const
    {
        SymbolEntryFunction;
        return m_WithEventsVarName;
    }
    void SetWithEventsVarName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_WithEventsVarName = value;
    }

    bool IsBadHandlesList() const
    {
        SymbolEntryFunction;
        return m_isBad;
    }

    void SetIsBad()
    {
        SymbolEntryFunction;
        m_isBad = true;
    }

    void ClearIsBad()
    {
        SymbolEntryFunction;
        m_isBad = false;
    }

    // Is this: Handles MyBase.*?
    bool IsMyBase() const
    {
        SymbolEntryFunction;
        return m_IsMyBase;
    }

    void SetIsMyBase()
    {
        SymbolEntryFunction;
        m_IsMyBase = true;
    }

    bool IsMyClass() const
    {
        SymbolEntryFunction;
        return m_IsMyClass;
    }

    void SetIsMyClass()
    {
        SymbolEntryFunction;
        m_IsMyClass = true;
    }

    bool IsEventFromMe() const
    {
        SymbolEntryFunction;
        return m_IsEventFromMe;
    }

    void SetIsEventFromMe()
    {
        SymbolEntryFunction;
        m_IsEventFromMe = true;
    }

    bool IsEventFromMeOrMyClass() const
    {
        SymbolEntryFunction;
        return IsEventFromMe() || IsMyClass();
    }

    // Return the location of the Event in the Handles declaration, e.g.
    // sub foo() Handles WithEventVar.Event  <-- location of Event
    Location *GetLocationOfEvent() 
    {
        SymbolEntryFunction;
        return & m_LocOfEvent;
    }

    // Return the location of the WithEvents variable in the Handles declaration, e.g.
    // sub foo() Handles WithEventVar.Event  <-- location of WithEventVar
    TrackedLocation *GetLocationOfWithEventsVar() 
    {
        SymbolEntryFunction;
        return & m_LocOfWithEventVar;
    }

    void SetWithEventsProperty(BCSYM_Property *WithEventsProperty)
    {
        SymbolEntryFunction;
        m_WithEventsProperty = WithEventsProperty;
    }

    BCSYM_Property* GetWithEventsProperty() const
    {
        SymbolEntryFunction;
        return m_WithEventsProperty;
    }

    BCSYM_Property* GetEventSourceProperty() const
    {
        SymbolEntryFunction;
        return m_EventSourceProperty;
    }

    void SetEventSourceProperty(BCSYM_Property *EventSourceProperty)
    {
        SymbolEntryFunction;
        m_EventSourceProperty = EventSourceProperty;
    }

    BCSYM_HandlesList *GetNextHandles() const
    {
        SymbolEntryFunction;
        return m_NextHandles;
    }
    BCSYM_HandlesList **GetNextHandlesTarget() 
    {
        SymbolEntryFunction;
        return &m_NextHandles;
    }
    void SetNextHandles(BCSYM_HandlesList * value)
    {
        SymbolEntryFunction;
        m_NextHandles = value;
    }

    BCSYM_MethodImpl *GetHandlingMethod() const
    {
        SymbolEntryFunction;
        return m_HandlingMethod;
    }
    void SetHandlingMethod(BCSYM_MethodImpl * value)
    {
        SymbolEntryFunction;
        m_HandlingMethod = value;
    }

protected:

    // the next in line;
    BCSYM_HandlesList * m_NextHandles;
    STRING * m_EventName; // the name of the Event handled
    STRING * m_EventSourcePropertyName; // the name of the EventSourceProperty in WithEventsVar.EventSourceProperty.EventName
    STRING * m_WithEventsVarName; // the name of the withEvents variable
    BCSYM_EventDecl *m_ActualEvent; // The actual event this handles
    BCSYM_Property *m_WithEventsProperty;
    BCSYM_Property *m_EventSourceProperty;
    Location m_LocOfEvent; // location of the event in the Handles declaration
    TrackedLocation m_LocOfWithEventVar; // location in the Handles declaration of the WithEvent variable

    BCSYM_MethodImpl * m_HandlingMethod; // the method on which this handles is specified

    bool m_IsMyBase : 1; // whether this handles an event on MyBase, e.g. ... Handles MyBase.SomeEvent
    bool m_IsMyClass : 1;// whether this handles an event on MyClass, e.g. ... Handles MyClass.SomeEvent
    bool m_IsEventFromMe : 1; // whether this handles an event on Me, e.g. ... Handles Me.SomeEvent

    bool m_isBad : 1; // TRUE if something is wrong with the symbol - typically means that an event in a Handles clause was specified twice.
};

/**************************************************************************************************
;ImplementsList

A list of procedures that are marked for implementing an interface
***************************************************************************************************/
class BCSYM_ImplementsList : public BCSYM
{
    DBGFRIENDS;
public:
    BCSYM * GetType()
    {
        SymbolEntryFunction;
        return GetTypeRaw()->DigThroughNamedType();
    }
    BCSYM * GetTypeRaw() const
    {
        SymbolEntryFunction;
        return m_ptyp;
    }
    void SetType(BCSYM * value)
    {
        SymbolEntryFunction;
        m_ptyp = value;
    }

    // will return the name of the method that we want to implement
    STRING * GetName() const
    {
        SymbolEntryFunction;
        return m_pstrProcName;
    }
    void SetName(_In_z_ STRING * value)
    {
        SymbolEntryFunction;
        m_pstrProcName = value;
    }

    BCSYM_NamedRoot * GetImplementedMember()
    {
        SymbolEntryFunction;
        return m_pImplementedMember;
    }

    BCSYM_GenericBinding * GetGenericBindingContext() const
    {
        SymbolEntryFunction;
        return m_pGenericBindingContext;
    }

    void SetImplementedMember(
        BCSYM_NamedRoot * pMember,
        BCSYM_GenericBinding * pGenericBindingContext)
    {
        SymbolEntryFunction;
        m_pImplementedMember = pMember;
        m_pGenericBindingContext = pGenericBindingContext;
    }

    BCSYM_ImplementsList * GetNext() const
    {
        SymbolEntryFunction;
        return m_pnext;
    }
    void SetNext(BCSYM_ImplementsList * value)
    {
        SymbolEntryFunction;
        m_pnext = value;
    }

protected:
    // the next in line;
    BCSYM_ImplementsList * m_pnext;

    // the name of the procedure that we aim to implement
    STRING * m_pstrProcName;

    // the interface that we target for implementation
    // this pointer will become an interface symbol after ResolveAllNamedTypes
    // is called
    BCSYM * m_ptyp;

    // Cached member being implemented.
    BCSYM_NamedRoot *m_pImplementedMember;

    // Generic binding used for the implemented member (which may be for the implemented interface).
    BCSYM_GenericBinding *m_pGenericBindingContext;
};

#if IDE 
// Used to back up class info that will be modified during bindable
// when electing a type to be the main type of a bunch of partial types.
//
// NOTE that if any other info in the class is changed in bindable, then
// it will also need to be backed up and then restored during decompilation.
//
struct MainClassBackupInfo
{
    unsigned __int8 m_Access;
    unsigned __int8 m_IsShadowsKeywordUsed : 1;
    unsigned __int8 m_IsNotInheritable : 1;
    unsigned __int8 m_IsMustInherit : 1;

    BCSYM * m_RawBase;
};
#endif IDE

/**************************************************************************************************
;Class

Represents a VB Class or Structure
***************************************************************************************************/
class BCSYM_Class : public BCSYM_Container

{
    DBGFRIENDS;
public:

    bool IsMeSet() const
    {
        SymbolEntryFunction;
        return (m_pvarMe != NULL);
    }

    // Get the instance of "me".
    BCSYM_Variable *GetMe() const
    {
        SymbolEntryFunction;
        VSASSERT(m_pvarMe, "Fetching a NULL Me!");
        return m_pvarMe;
    }
    void SetMe(BCSYM_Variable * value)
    {
        SymbolEntryFunction;
        m_pvarMe = value;
    }

    BCSYM *GetRawBase() const
    {
        SymbolEntryFunction;
        return m_ptypBaseClass;
    }
    void SetRawBase(BCSYM * value)
    {
        SymbolEntryFunction;
        m_ptypBaseClass = value;
    }

    // The base class. Does some work to make sure that it isn't bad and that is it actually a class.  Really ---- name...
    BCSYM_Class *GetCompilerBaseClass();

    // Returns the raw base class (will dig through named types, at least)
    BCSYM *GetBaseClass();

    // The base class token (for metadata)
    mdTypeRef GetBaseClassToken() const
    {
        SymbolEntryFunction;
        return m_tkBase;
    }
    void SetBaseClassToken(mdTypeRef value)
    {
        SymbolEntryFunction;
        m_tkBase = value;
    }


    // Get the first instance constructor.
    BCSYM_Proc * GetFirstInstanceConstructor(Compiler * pCompiler);

    // Get the first constructor no required parameters
    BCSYM_Proc * GetFirstInstanceConstructorNoRequiredParameters(Compiler * pCompiler);

    // Get the class constructor
    BCSYM_Proc * GetSharedConstructor(Compiler * pCompiler);

    // Is this symbol an intrinsic type that maps on to a COM+ type?
    bool IsIntrinsicType() const
    {
        SymbolEntryFunction;
        return m_IsIntrinsicType;
    }
    void SetIsIntrinsicType()
    {
        m_IsIntrinsicType = true;
    }


    // The Vtype of mapped intrinsic types.
    inline Vtypes GetVtype();
    
    unsigned __int32 /* Vtypes */ GetRawVtype() const
    {
        SymbolEntryFunction;
        return m_vtype;
    }
    void SetVtype(unsigned __int32 /* Vtypes */ value)
    {
        SymbolEntryFunction;
        m_vtype = value;
    }

    // Does this class derive from PossibleBaseClass?
    bool DerivesFrom(BCSYM_Class *PossibleBaseClass);

    // Are ALL the MustOverrides for this class and all its base classes overridden?
    bool AreMustOverridesSatisfied() const
    {
        SymbolEntryFunction;
        return m_AllMustOverridesSatisfied;
    }

    // Set TRUE if this class and all its bases have ALL their MustOverrides taken care of
    void SetMustOverridesSatisifed(bool allOverridden)
    {
        SymbolEntryFunction;
        m_AllMustOverridesSatisfied = allOverridden;
    }

    bool DerivesFromMarshalByRef()
    {
        SymbolEntryFunction;
        return false;
    }

    //  not used
    /*    
    void SetInheritsFromMarshalByRefObject(bool value)
    {
        m_DerivesFromMarshalByRef = value;
    }*/

    // Is the class marked as 'MustInherit', i.e. regardless of the MustOverrides situation, this class is to be considered Abstract
    bool IsMustInherit() const
    {
        SymbolEntryFunction;
        return m_isMustInherit;
    }
    void SetMustInherit(bool value)
    {
        SymbolEntryFunction;
        m_isMustInherit = value;
    }

    // True if nobody can use this class as a base class.  Either the class was imported with the Sealed attribute or it was marked NotInheritable in VB.
    bool IsNotInheritable() const
    {
        SymbolEntryFunction;
        return m_isNotInheritable;
    }
    void SetNotInheritable(bool value)
    {
        SymbolEntryFunction;
        m_isNotInheritable = value;
    }

    bool IsStdModule() const
    {
        SymbolEntryFunction;
        return m_isStdModule;
    }
    void SetIsStdModule(bool value)
    {
        SymbolEntryFunction;
        m_isStdModule = value;
    }

    // True if it is illegal to create an instance of this class for whatever reason.  The class may be shared ( i.e. it is a std module ),
    // there may be MustOverride funcs that haven't been overridden, or the class may be marked as Abstract via the 'MustInhert' keyword, etc.
    bool IsCantNew()
    {
        SymbolEntryFunction;
        return IsStdModule() || IsMustInherit() || !AreMustOverridesSatisfied() || IsNotCreateable();
    }

    // Get the location of the inherits statement.
    Location *GetInheritsLocation();

    // Set the location of the inherits statement.  Use this when inheriting from a type that doesn't store its own location information for use in the Inherits line
    void SetInheritsLocation(
        Location * Loc,
        NorlsAllocator * Allocator);

    // True if we're not allowed to create an instance of this class.
    bool IsNotCreateable();

    // Whether this is a class or a struct
    inline
    bool IsStruct() const
    {
        SymbolEntryFunction;
        return m_isStruct;
    }
    inline void SetIsStruct(bool value)
    {
        SymbolEntryFunction;
        m_isStruct = value;
    }

    // Whether this is a delegate
    bool IsDelegate() const
    {
        SymbolEntryFunction;
        return m_isDelegate;
    }
    inline void SetIsDelegate(bool value)
    {
        SymbolEntryFunction;
        m_isDelegate = value;
    }

    // Whether this is an enum
    bool IsEnum() const
    {
        SymbolEntryFunction;
        return m_isEnum;
    }
    void SetIsEnum(bool value)
    {
        SymbolEntryFunction;
        m_isEnum = value;
        if (value)
        {
            SetIsStruct(true);
        }
    }
    
    unsigned __int32 /* AttributeValidity */ GetAttributeValidity() const
    {
        SymbolEntryFunction;
        return m_AttributeValidity;
    }
    void SetAttributeValidity(unsigned __int32 /* AttributeValidity */ value)
    {
        SymbolEntryFunction;
        m_AttributeValidity = value;
    }

    GenericParams * GetGenericParams() const
    {
        SymbolEntryFunction;
        return m_GenericParams;
    }
    void SetGenericParams(GenericParams * value)
    {
        SymbolEntryFunction;
        m_GenericParams = value;
    }

    bool IsGeneric()
    {
        SymbolEntryFunction;
        return GetGenericParams() != NULL;
    }

    BCSYM_GenericParam * GetFirstGenericParam()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_FirstGenericParam : NULL;
    }

    // Keep track of whether this base class leads to circular inheritance
    inline bool IsBaseInvolvedInCycle() const
    {
        SymbolEntryFunction;
        return m_BaseInvolvedInCycle;
    }
    inline void SetBaseInvolvedInCycle(bool value)
    {
        SymbolEntryFunction;
        m_BaseInvolvedInCycle = value;
    }

    // Is this the intrinsic type symbol for "System.Object"?
    inline bool IsObject() const 
    {
        SymbolEntryFunction;
        return m_isObject;
    }

    // Mark this symbol as being System.Object
    inline
    void SetIsObject(bool IsObject)
    {
        SymbolEntryFunction;
        m_isObject = IsObject;
    }

    bool IsCOMValueType() const
    {
        SymbolEntryFunction;
        return m_isCOMValueType;
    }
    void SetIsCOMValueType(bool IsCOMValueType)
    {
        SymbolEntryFunction;
        m_isCOMValueType = IsCOMValueType;
    }

    // Clear out the list of backing fields for static variables found in the methods of this class
    void BCSYM_Class::ClearBackingFieldsList()
    {
        SymbolEntryFunction;
        m_BackingFieldsForStaticLocals = NULL;
    }

    // These setters just mark this class as being an attribute or a security attribute.  They're grouped together like this because
    // these properties are all determined exactly the same way: via inheritance from a particular class.
    bool IsAttribute() const
    { 
        SymbolEntryFunction;
        return m_fAttribute; 
    }
    void SetIsAttribute(bool fIsAttribute) 
    { 
        SymbolEntryFunction;
        m_fAttribute = fIsAttribute; 
    }
    
    bool IsSecurityAttribute() const
    { 
        SymbolEntryFunction;
        return m_fSecurityAttribute; 
    }
    void SetIsSecurityAttribute(bool fIsSecurityAttribute) 
    { 
        SymbolEntryFunction;
        m_fSecurityAttribute = fIsSecurityAttribute; 
    }

    // AttributeValidity is used by BCSYM_Class::IsValidAttributeClass to optimize checking attribute classes only once
    enum AttributeValidity
    {
        ValidityUnknown = 0,
        ValidAsAttribute,
        InvalidAsAttribute
    };

    void ResetAttributeValidity() 
    { 
        SymbolEntryFunction;
        SetAttributeValidity(ValidityUnknown) ; 
    }

    bool IsValidAttributeClass(ErrorTable *pErrorTable, BCSYM_ApplAttr *psymReportError);

    BCSYM_Implements *GetFirstImplements();

    // Return the next in the list of standard modules for the containing namespace.
    BCSYM_Class *GetNextModule() const
    {
        SymbolEntryFunction;
        // Since the field m_pNextModule is unioned with m_pUnderlyingTypeForEnum for
        // enums, this assert is important
        //
        VSASSERT( !IsEnum(), "Unexpected call for enum!!!");

        return m_pNextModule;
    }
    void SetNextModule(BCSYM_Class * value)
    {
        SymbolEntryFunction;
        m_pNextModule = value;
    }

    // Attaches a list of backing fields for local static vars to the class
    void StowBackingFieldsForStatics( SymbolList *BackingFieldsList, Symbols *SymbolFactory );

    // Returns the list of backing fields that were created to store the values for any static locals found in the class methods
    BCSYM_Hash *GetBackingFieldsForStatics();

    BCSYM_Hash *GetBackingFieldsForStaticsRaw() const
    {
        SymbolEntryFunction;
        return m_BackingFieldsForStaticLocals;
    }

    // Sets the hash of backing fields that were created to store the values for any static locals found in the class methods
    void SetBackingFieldsForStatics(BCSYM_Hash * BackingFieldsHash)
    {
        SymbolEntryFunction;
        m_BackingFieldsForStaticLocals = BackingFieldsHash;
    }

    // Return the name of the nested interface that we will generate
    // when the user add <ComClass()> to a class.
    STRING* GetComClassInterfaceName();

    // Return the name of the nested event interface that we will generate
    // when the user add <ComClass()> to a class.
    STRING* GetComClassEventsInterfaceName();

    // Get/Set com class events interface token
    mdToken GetComClassEventsToken() const
    {
        SymbolEntryFunction;
        return m_tkComClassEvents;
    }
    void SetComClassEventsToken(mdToken value)
    {
        SymbolEntryFunction;
        m_tkComClassEvents = value;
    }

    BCSYM *GetUnderlyingTypeForEnum() const
    {
        SymbolEntryFunction;
        // Since the field m_pUnderlyingTypeForEnum is unioned with m_pNextModule for
        // modules, this assert is important
        //
        VSASSERT( IsEnum(), "Unexpected call for non-enum!!!");

        return m_pUnderlyingTypeForEnum;
    }

    void SetUnderlyingTypeForEnum(BCSYM *pUnderlyingTypeForEnum)
    {
        SymbolEntryFunction;
        // Since the field m_pUnderlyingTypeForEnum is unioned with m_pNextModule for
        // modules, this assert is important
        //
        VSASSERT( IsEnum(), "Unexpected call for non-enum!!!");

        m_pUnderlyingTypeForEnum = pUnderlyingTypeForEnum;
    }

    BCSYM_Hash *GetGenericParamsHash()
    {
        SymbolEntryFunction;
        return GetGenericParams() ? GetGenericParams()->m_GenericParamsHash : NULL;
    }

    BCSYM_Class* GetMainType() const
    {
        SymbolEntryFunction;
        return m_MainType;
    }

    BCSYM_Class* GetNextPartialType() const
    {
        SymbolEntryFunction;
        return m_NextPartialType;
    }

    bool ContainsExtensionMethods()
    {
        SymbolEntryFunction;
        return m_ContainsExtensionMethods;
    }

    void SetContainsExtensionMethods(bool value = true)
    {
        SymbolEntryFunction;
        m_ContainsExtensionMethods = value;
    }

#if IDE 
    // Devdiv Bug [26424] Anonymous Type is to support assembly level merging.
    BCSYM_Proc *GetAnonymousTypeProc() const
    {
        SymbolEntryFunction;
        VSASSERT(IsAnonymousType(), "Anonymous Type Proc is meaningless for a non-anonymous type.");
        return m_AnonymousTypeProc;
    }

    void SetAnonymousTypeProc(BCSYM_Proc *Proc)
    {
        SymbolEntryFunction;
        VSASSERT(IsAnonymousType() || Proc == NULL, "Anonymous Type Proc is meaningless for a non-anonymous type.");
        m_AnonymousTypeProc = Proc;
    }
#endif

    void SetMainType(BCSYM_Class *MainClass)
    {
        SymbolEntryFunction;
        m_MainType = MainClass;
    }

    void SetNextPartialType(BCSYM_Class *NextPartialClass)
    {
        SymbolEntryFunction;
        m_NextPartialType = NextPartialClass;
    }

    StructCycleNode *GetStructCycleInfo() const
    {
        SymbolEntryFunction;
        VSASSERT(IsStruct(), "Unexpected access of member for non-struct!!!");

        return m_StructCycleInfo;
    }
    void SetStructCycleInfo(StructCycleNode *StructCycleInfo)
    {
        SymbolEntryFunction;
        VSASSERT(IsStruct(), "Unexpected access of member for non-struct!!!");
        VSASSERT(!IsPartialTypeAndHasMainType(), "Unexpected access of member for partial type!!!");

        m_StructCycleInfo = StructCycleInfo;
    }

    void SetTypeAccessExplictlySpecified(bool AccessExplicitlySpecified)
    {
        SymbolEntryFunction;
        m_IsTypeAccessNotExplictlySpecified = !AccessExplicitlySpecified;
    }

    bool IsTypeAccessExplictlySpecified()
    {
        SymbolEntryFunction;
        return !m_IsTypeAccessNotExplictlySpecified;
    }

    void SetBaseExplicitlySpecified(bool BaseExplicitSpecified)
    {
        SymbolEntryFunction;
        m_IsBaseNotExplicitlySpecified = !BaseExplicitSpecified;
    }

    bool IsBaseExplicitlySpecified()
    {
        SymbolEntryFunction;
        return !m_IsBaseNotExplicitlySpecified;
    }

    void SetIsBaseBad(bool IsBaseBad);

    bool IsBaseBad() const
    {
        SymbolEntryFunction;
        return m_IsBaseBad;
    }

    void SetIsStructCycleCheckingDone(bool IsStructCycleCheckingDone)
    {
        SymbolEntryFunction;
        m_IsStructCycleCheckingDone = IsStructCycleCheckingDone;
    }

    bool IsStructCycleCheckingDone() const
    {
        SymbolEntryFunction;
        return m_IsStructCycleCheckingDone;
    }

    BCSYM_Implements * GetImpList() const
    {
        SymbolEntryFunction;
        return m_----lList;
    }
    void SetImpList(BCSYM_Implements * value)
    {
        SymbolEntryFunction;
        m_----lList = value;
    }

    Location * GetInheritsLocationForIntrinsic() const
    {
        SymbolEntryFunction;
        return m_InheritsLocationForIntrinsic;
    }
    void SetInheritsLocationForIntrinsic(Location * value)
    {
        SymbolEntryFunction;
        m_InheritsLocationForIntrinsic = value;
    }
  

#if IDE 

    MainClassBackupInfo *GetMainClassBackupInfo()
    {
        SymbolEntryFunction;
        return m_MainClassBackupInfo;
    }

    void SetMainClassBackupInfo(MainClassBackupInfo *BackupClassInfo)
    {
        SymbolEntryFunction;
        m_MainClassBackupInfo = BackupClassInfo;
    }

    // Return the original base specified for partial types and not the logical base.
    //
    BCSYM *GetRawBaseForPartialType()
    {
        SymbolEntryFunction;
        if (m_MainClassBackupInfo)
        {
            return m_MainClassBackupInfo->m_RawBase;
        }

        return GetRawBase();
    }

#endif IDE

protected:

    // The instance of Me.
    BCSYM_Variable * m_pvarMe;

    // The base class - never access this directly.  Use GetBaseClass(), GetCompilerBaseClass() because there is cycle detection logic incorporated there
    BCSYM *m_ptypBaseClass;

    // the base class token for metadata
    mdTypeRef m_tkBase;

    // The Vtype of mapped intrinsic types and enums
    unsigned __int32 /* Vtypes */ m_vtype : 5;

    unsigned __int32 /* AttributeValidity */ m_AttributeValidity:2;

    // Is this symbol an intrinsic type that maps on to a COM+ type?
    unsigned __int32 m_IsIntrinsicType : 1;

    // Are all the items marked as MustOverride for this class overridden?
    unsigned __int32 m_AllMustOverridesSatisfied:1;

    // Is the class marked as 'MustInherit', i.e. there may or not be any MustOverrides on the class, yet it is still to be considered Abstract
    unsigned __int32 m_isMustInherit:1;

    // True if this is a 'final' class, i.e. it is marked with 'NotInheritable' in VB or Sealed from com+ metadata
    unsigned __int32 m_isNotInheritable:1;

    // True if this is a STRUCTURE
    unsigned __int32 m_isStruct:1;

    // True if this is a DELEGATE
    unsigned __int32 m_isDelegate:1;

    // True if this is an ENUM
    unsigned __int32 m_isEnum:1;

    unsigned __int32 m_fAttribute:1;        // True iff this class represents a custom attribute
    unsigned __int32 m_fSecurityAttribute:1;     // True iff this class represents a security attribute

    // Is this the Object type?
    unsigned __int32 m_isObject:1;

    // Is this the System.ValueType type ?
    unsigned __int32 m_isCOMValueType:1;

    // Is this a Standard Module?
    unsigned __int32 m_isStdModule:1;

    // Whether this class derives directly or indirectly from System.MarshallByRefObject
    //  not used
    // unsigned __int32 m_DerivesFromMarshalByRef:1;

    // Whether the base class leads us into an inheritance cycle
    unsigned __int32 m_BaseInvolvedInCycle:1;

    // Whether the access is implied and not explicitly specified
    unsigned __int32 m_IsTypeAccessNotExplictlySpecified:1;

    // Whether the base is implied and not explicitly specified
    unsigned __int32 m_IsBaseNotExplicitlySpecified:1;

    // Whether the base is bad. This should only be set in bindable.
    unsigned __int32 m_IsBaseBad:1;

    // Is struct member cycle (i.e. struct layout circularity) checking done. This is
    // only applicable for structures, although during decompilation this is always
    // reset no matter whether it is a structure or not.
    //
    unsigned __int32 m_IsStructCycleCheckingDone:1;

    unsigned __int32 m_ContainsExtensionMethods : 1;

    // Used to indicate if a class is a synthetic class. A class can be synthetic if
    // it is compiler generated in order to support closures, etc.
    //
    // 




    BCSYM_Implements * m_----lList;

    union
    {
        // All the modules in a namespace are on a list so that name lookup can traverse them quickly.
        BCSYM_Class * m_pNextModule;     // used if the class is marked as a standard module

        // If the class is an Enum, this is the underlying type of the enum
        BCSYM * m_pUnderlyingTypeForEnum;

        // Used only for structures to detect embedded member cycles in structures.
        //
        StructCycleNode * m_StructCycleInfo;
    };

    // The backing fields for local static vars in this class
    BCSYM_Hash * m_BackingFieldsForStaticLocals;

    GenericParams * m_GenericParams;

    // When we inherit from an intrinsic (integer, short, etc.) we need to hang on to the location information for the Inherits statement
    Location * m_InheritsLocationForIntrinsic;

    // When the user adds <ComClass()> to a class, we need to keep some additional state
    // around during _PromoteToTypesEmitted. This data should not be accessed except
    // during that state and only by the background thread.
    // These such state is kept in BCSYM_NamedRoot::m_tkComClass and
    // BCSYM_Param::m_tkComClass and BCSYM_Class::m_tkComClassEvents.
    mdToken m_tkComClassEvents;

    // Used to hold partial class related information
    //
    BCSYM_Class * m_MainType;
    
    BCSYM_Class * m_NextPartialType;
#if IDE 
    BCSYM_Proc * m_AnonymousTypeProc;    // used to store proc that created the anonymous type for method-level merging.
                                        // Devdiv Bug [27239] don't union this field with m_NextPartialType because even though
                                        // Anonymous Type cannot be partial, m_NextPartialType can still be accessed by other code
                                        // for NULL checks.
#endif

#if IDE 
    // This is used by bindable when collapsing partial types in order to back up the
    // class information for the partial types elected as the main type. The backed
    // up information is useful during decompilation.
    //
    MainClassBackupInfo * m_MainClassBackupInfo;
#endif IDE

}; // end of BCSYM_Class

struct AttrIndex;

/**************************************************************************************************
;ApplAttr

Represents an applied attribute.  For example,
in this code, 'AA1' and 'AA2' are the applied attributes and T is
the target to which they are being applied:

    class <AA1(5, "arg"), AA2(namedprop := 1.2)> T
    end class

The BCSYM_ApplAttr needs to store all of the information
about one applied attribute so it can be parsed and examined.

We also chain BCSYM_ApplAttrs into a list; all of the
attributes applied to the same target live in the same list.
***************************************************************************************************/

class BCSYM_ApplAttr : public BCSYM
{
    DBGFRIENDS;
public:

    
    BCSYM_ApplAttr *GetNext(void) const
    {
        SymbolEntryFunction;
        return m_psymApplAttrNext;
    }
    void SetNext(BCSYM_ApplAttr * value)
    {
        SymbolEntryFunction;
        m_psymApplAttrNext = value;
    }

    BCSYM *GetAttributeSymbol(void) const
    {
        SymbolEntryFunction;
        return GetAttrClass()->DigThroughNamedType();
    }

    BCSYM_Expression * GetExpression(void) const
    {
        SymbolEntryFunction;
        return m_psymDeferredExpr;
    }
    void SetExpression(BCSYM_Expression * value)
    {
        SymbolEntryFunction;
        m_psymDeferredExpr = value;
    }

    bool IsAssembly() const
    {
        SymbolEntryFunction;
        return m_fAssembly;
    }
    void SetIsAssembly(bool value)
    {
        SymbolEntryFunction;
        m_fAssembly = value;
    }

    bool IsModule() const
    {
        SymbolEntryFunction;
        return m_fModule;
    }
    void SetIsModule(bool value)
    {
        SymbolEntryFunction;
        m_fModule = value;
    }

    ILTree::AttributeApplicationExpression *GetBoundTree()
    {
        SymbolEntryFunction;
        return m_pBoundTree;
    }

    void SetBoundTree(ILTree::AttributeApplicationExpression *pBoundTree)
    {
        SymbolEntryFunction;
        m_pBoundTree = pBoundTree;
    }

    bool IsBadApplAttr() const
    {
        SymbolEntryFunction;
        return m_fIsBad;
    }

    void SetIsBad()
    {
        SymbolEntryFunction;
        m_fIsBad = true;
    }

    void ClearIsBad()
    {
        SymbolEntryFunction;
        m_fIsBad = false;
    }

    BCSYM_NamedType * GetAttrClass() const
    {
        SymbolEntryFunction;
        return m_psymAttrClass;
    }
    void SetAttrClass(BCSYM_NamedType * value)
    {
        SymbolEntryFunction;
        m_psymAttrClass = value;
    }

    const AttrIndex * GetAttrIndex() const
    {
        SymbolEntryFunction;
        return m_pAttributeIndex;
    }
    void SetAttrIndex(const AttrIndex * value)
    {
        SymbolEntryFunction;
        m_pAttributeIndex = value;
    }

private:

    // The attribute class that is being used.
    BCSYM_NamedType * m_psymAttrClass;

    // Deferred expression that will be (re)parsed later
    BCSYM_Expression *m_psymDeferredExpr;

    // Link next applied attribute here
    BCSYM_ApplAttr *m_psymApplAttrNext;

    // Is this an assembly-level attribute?
    bool m_fAssembly : 1;

    // Is this a module-level attribute?
    bool m_fModule : 1;

    bool m_fIsBad : 1;

    // The bound tree got after interpreting this attribute
    ILTree::AttributeApplicationExpression *m_pBoundTree;

    // Pointer to AttrIndex structure for well-known Attributes
    const AttrIndex * m_pAttributeIndex;
    
}; // end of BCSYM_ApplAttr


/**************************************************************************************************
;TypeForwarder

Represents a Type forwarder symbol placed in the hash.
Could have used BCSYM_GenericBadNamedRoot itself, but this is
cleaner and will also be more useful if in the future type
forwarders can forwards to types of different names.

Inheriting from BCSYM_GenericBadNamedRoot is useful because this
symbol needs to be marked bad and always have an error in it. Gets
filtered out automatically whereever GenericBadNamedRoots are
filtered out.

***************************************************************************************************/
class BCSYM_TypeForwarder : public BCSYM_GenericBadNamedRoot
{
    DBGFRIENDS;
public:
    mdAssemblyRef GetAssemblyRefForwardedTo() const
    {
        SymbolEntryFunction;
        return m_tkAssemblyRefForwardedTo_DoNotUseDirectly;
    }

    void SetAssemblyRefForwardedTo(mdAssemblyRef tkAssemblyRefForwardedTo)
    {
        SymbolEntryFunction;
        m_tkAssemblyRefForwardedTo_DoNotUseDirectly = tkAssemblyRefForwardedTo;
    }

    bool IsTypeForwarderBeingResolved() const
    {
        SymbolEntryFunction;
        return m_IsTypeForwarderBeingResolved_DoNotUseDirectly;
    }

    void SetTypeForwarderBeingResolved(bool IsBeingResolved)
    {
        SymbolEntryFunction;
        m_IsTypeForwarderBeingResolved_DoNotUseDirectly = IsBeingResolved;
    }

    bool IsTypeForwarderResolved() const
    {
        SymbolEntryFunction;
        return m_IsTypeForwarderResolved_DoNotUseDirectly;
    }

    void SetTypeForwarderResolved(bool IsResolved)
    {
        SymbolEntryFunction;
        m_IsTypeForwarderResolved_DoNotUseDirectly = IsResolved;
    }

    BCSYM *GetTypeForwardedTo() const
    {
        SymbolEntryFunction;
        VSASSERT(IsTypeForwarderResolved(), "Unresolved type forwarder query unexpected!!!");
        return m_TypeForwardedTo_DoNotUseDirectly;
    }

    void SetTypeForwardedTo(BCSYM *Type)
    {
        SymbolEntryFunction;
        m_TypeForwardedTo_DoNotUseDirectly = Type;
    }


};


//****************************************************************************
// Accessor functions.
//****************************************************************************

//
// BCSYM inline methods
//

#define KindSupports(type) \
  VSASSERT(s_rgBilkindInfo[GetKind()].m_kind == GetKind(), "s_rgBilkindInfo table is inconsistent with Bilkind values."); \
  return s_rgBilkindInfo[GetKind()].m_is##type
#define KindEquals(type) \
  VSASSERT(s_rgBilkindInfo[SYM_##type].m_kind == SYM_##type, "s_rgBilkindInfo table is inconsistent with Bilkind values."); \
  VSASSERT(!s_rgBilkindInfo[SYM_##type].m_hasDerivers, "Not a leaf kind!  Use DoesKindSupport instead of KindEquals"); \
  return (GetKind() == SYM_##type)
#define CastSymbolKind(type) \
  VSASSERT(this == NULL || Is##type(), "bad kind"); \
  return ((BCSYM_##type *)this)

inline bool BCSYM::IsSimpleType()
{
    SymbolEntryFunction;
    KindSupports(SimpleType);
}

inline BCSYM_SimpleType *BCSYM::PSimpleType()
{
    SymbolEntryFunction;
    CastSymbolKind(SimpleType);
}

inline bool BCSYM::IsVoidType()
{
    SymbolEntryFunction;
    KindEquals(VoidType);
}

inline BCSYM_VoidType *BCSYM::PVoidType()
{
    SymbolEntryFunction;
    CastSymbolKind(VoidType);
}

inline bool BCSYM::IsArrayType() const
{
    SymbolEntryFunction;
    KindSupports(ArrayType);
}

inline BCSYM_ArrayType *BCSYM::PArrayType() const
{
    SymbolEntryFunction;
    CastSymbolKind(ArrayType);
}

inline bool BCSYM::IsArrayLiteralType() const
{
    SymbolEntryFunction;
    KindEquals(ArrayLiteralType);
}

inline BCSYM_ArrayLiteralType * BCSYM::PArrayLiteralType() const
{
    SymbolEntryFunction;
    CastSymbolKind(ArrayLiteralType);
}

inline bool BCSYM::IsNamedRoot() const
{
    SymbolEntryFunction;
    KindSupports(NamedRoot);
}

inline BCSYM_NamedRoot *BCSYM::PNamedRoot() const
{
    SymbolEntryFunction;
    if (this && IsGenericBinding())
    {
        return PGenericBinding()->GetGeneric()->PNamedRoot();
    }

    CastSymbolKind(NamedRoot);
}

inline bool BCSYM::IsGenericBadNamedRoot()
{
    SymbolEntryFunction;
    KindSupports(GenericBadNamedRoot);
}

inline BCSYM_GenericBadNamedRoot *BCSYM::PGenericBadNamedRoot()
{
    SymbolEntryFunction;
    CastSymbolKind(GenericBadNamedRoot);
}

inline bool BCSYM::IsContainer() const
{
    SymbolEntryFunction;
    KindSupports(Container);
}

inline BCSYM_Container *BCSYM::PContainer() const
{
    SymbolEntryFunction;
    if (this && IsGenericBinding())
    {
        return PGenericBinding()->GetGeneric()->PContainer();
    }

    CastSymbolKind(Container);
}

inline bool BCSYM::IsCCContainer()
{
    SymbolEntryFunction;
    KindEquals(CCContainer);
}

inline BCSYM_CCContainer *BCSYM::PCCContainer()
{
    SymbolEntryFunction;
    CastSymbolKind(CCContainer);
}

inline bool BCSYM::IsMember()
{
    SymbolEntryFunction;
    KindSupports(Member);
}

inline BCSYM_Member *BCSYM::PMember()
{
    SymbolEntryFunction;
    CastSymbolKind(Member);
}

inline bool BCSYM::IsProc()
{
    SymbolEntryFunction;
    KindSupports(Proc);
}

inline BCSYM_Proc *BCSYM::PProc()
{
    SymbolEntryFunction;
    CastSymbolKind(Proc);
}

inline bool BCSYM::IsProperty()
{
    SymbolEntryFunction;
    KindEquals(Property);
}

inline BCSYM_Property *BCSYM::PProperty()
{
    SymbolEntryFunction;
    CastSymbolKind(Property);
}

inline bool BCSYM::IsVariable()
{
    SymbolEntryFunction;
    KindSupports(Variable);
}

inline BCSYM_Variable *BCSYM::PVariable()
{
    SymbolEntryFunction;
    CastSymbolKind(Variable);
}

inline bool BCSYM::IsVariableWithValue()
{
    SymbolEntryFunction;
    KindSupports(VariableWithValue);
}

inline BCSYM_VariableWithValue *BCSYM::PVariableWithValue()
{
    SymbolEntryFunction;
    CastSymbolKind(VariableWithValue);
}

inline bool BCSYM::IsVariableWithArraySizes()
{
    SymbolEntryFunction;
    KindEquals(VariableWithArraySizes);
}

inline BCSYM_VariableWithArraySizes *BCSYM::PVariableWithArraySizes()
{
    SymbolEntryFunction;
    CastSymbolKind(VariableWithArraySizes);
}

inline bool BCSYM::IsCCConstant()
{
    SymbolEntryFunction;
    KindEquals(CCConstant);
}

inline BCSYM_CCConstant *BCSYM::PCCConstant()
{
    SymbolEntryFunction;
    CastSymbolKind(CCConstant);
}

inline bool BCSYM::IsStaticLocalBackingField()
{
    SymbolEntryFunction;
    KindEquals(StaticLocalBackingField);
}

inline BCSYM_StaticLocalBackingField *BCSYM::PStaticLocalBackingField()
{
    SymbolEntryFunction;
    CastSymbolKind(StaticLocalBackingField);
}

inline bool BCSYM::IsParam() const
{
    SymbolEntryFunction;
    KindSupports(Param);
}

inline BCSYM_Param *BCSYM::PParam() const
{
    SymbolEntryFunction;
    CastSymbolKind(Param);
}

inline bool BCSYM::IsParamWithValue()
{
    SymbolEntryFunction;
    KindEquals(ParamWithValue);
}

inline BCSYM_ParamWithValue *BCSYM::PParamWithValue()
{
    SymbolEntryFunction;
    CastSymbolKind(ParamWithValue);
}

inline bool BCSYM::IsPointerType()
{
    SymbolEntryFunction;
    KindEquals(PointerType);
}

inline BCSYM_PointerType *BCSYM::PPointerType()
{
    SymbolEntryFunction;
    CastSymbolKind(PointerType);
}

inline bool BCSYM::IsNamedType() const
{
    SymbolEntryFunction;
    KindEquals(NamedType);
}

inline BCSYM_NamedType *BCSYM::PNamedType() const
{
    SymbolEntryFunction;
    CastSymbolKind(NamedType);
}

inline bool BCSYM::IsHash()
{
    SymbolEntryFunction;
    KindEquals(Hash);
}

inline BCSYM_Hash *BCSYM::PHash()
{
    SymbolEntryFunction;
    CastSymbolKind(Hash);
}

inline bool BCSYM::IsAlias()
{
    SymbolEntryFunction;
    KindEquals(Alias);
}

inline BCSYM_Alias *BCSYM::PAlias()
{
    SymbolEntryFunction;
    CastSymbolKind(Alias);
}

inline bool BCSYM::IsClass() const
{
    SymbolEntryFunction;
    if (this && IsGenericTypeBinding())
    {
        return PGenericTypeBinding()->GetGenericType()->IsClass();
    }

    KindSupports(Class);
}

inline bool BCSYM_Container::IsClass() const
{
    SymbolEntryFunction;
    KindSupports(Class);
}

inline BCSYM_Class *BCSYM::PClass() const
{
    SymbolEntryFunction;
    if (this && IsGenericTypeBinding())
    {
        return PGenericTypeBinding()->GetGenericType()->PClass();
    }

    CastSymbolKind(Class);
}

inline BCSYM_Class *BCSYM_Container::PClass() const
{
    CastSymbolKind(Class);
}


inline bool BCSYM::IsMethodImpl()
{
    SymbolEntryFunction;
    KindEquals(MethodImpl);
}

inline BCSYM_MethodImpl *BCSYM::PMethodImpl()
{
    SymbolEntryFunction;
    CastSymbolKind(MethodImpl);
}

inline bool BCSYM::IsSyntheticMethod()
{
    SymbolEntryFunction;
    KindEquals(SyntheticMethod);
}

inline BCSYM_SyntheticMethod *BCSYM::PSyntheticMethod()
{
    SymbolEntryFunction;
    CastSymbolKind(SyntheticMethod);
}

inline bool BCSYM::IsMethodDecl()
{
    SymbolEntryFunction;
    KindSupports(MethodDecl);
}

inline BCSYM_MethodDecl *BCSYM::PMethodDecl()
{
    SymbolEntryFunction;
    CastSymbolKind(MethodDecl);
}

inline bool BCSYM::IsEventDecl()
{
    SymbolEntryFunction;
    KindEquals(EventDecl);
}

inline BCSYM_EventDecl *BCSYM::PEventDecl()
{
    SymbolEntryFunction;
    CastSymbolKind(EventDecl);
}

inline bool BCSYM::IsDllDeclare()
{
    SymbolEntryFunction;
    KindEquals(DllDeclare);
}

inline BCSYM_DllDeclare *BCSYM::PDllDeclare()
{
    SymbolEntryFunction;
    CastSymbolKind(DllDeclare);
}

inline bool BCSYM::IsExpression()
{
    SymbolEntryFunction;
    KindSupports(Expression);
}

inline BCSYM_Expression *BCSYM::PExpression()
{
    SymbolEntryFunction;
    CastSymbolKind(Expression);
}

inline bool BCSYM::IsImplements() const
{
    SymbolEntryFunction;
    KindEquals(Implements);
}

inline BCSYM_Implements *BCSYM::PImplements()const
{
    SymbolEntryFunction;
    CastSymbolKind(Implements);
}

inline bool BCSYM::IsInterface()
{
    SymbolEntryFunction;
    if (this && IsGenericTypeBinding())
    {
        return PGenericTypeBinding()->GetGenericType()->IsInterface();
    }

    KindEquals(Interface);
}

inline bool BCSYM_Container::IsInterface()
{
    SymbolEntryFunction;
    KindEquals(Interface);
}

inline BCSYM_Interface *BCSYM::PInterface()
{
    SymbolEntryFunction;
    if (this && IsGenericTypeBinding())
    {
        return PGenericTypeBinding()->GetGenericType()->PInterface();
    }

    CastSymbolKind(Interface);
}

inline BCSYM_Interface *BCSYM_Container::PInterface()
{
    SymbolEntryFunction;
    CastSymbolKind(Interface);
}


inline bool BCSYM::IsHandlesList()const
{
    SymbolEntryFunction;
    KindEquals(HandlesList);
}

inline BCSYM_HandlesList *BCSYM::PHandlesList() const
{
    SymbolEntryFunction;
    CastSymbolKind(HandlesList);
}

inline bool BCSYM::IsImplementsList()
{
    SymbolEntryFunction;
    KindEquals(ImplementsList);
}

inline BCSYM_ImplementsList *BCSYM::PImplementsList()
{
    SymbolEntryFunction;
    CastSymbolKind(ImplementsList);
}

inline bool BCSYM::IsNamespace() const
{
    SymbolEntryFunction;
    KindEquals(Namespace);
}

inline BCSYM_Namespace *BCSYM::PNamespace()
{
    SymbolEntryFunction;
    CastSymbolKind(Namespace);
}

inline bool BCSYM::IsNamespaceRing()
{
    SymbolEntryFunction;
    KindEquals(NamespaceRing);
}

inline BCSYM_NamespaceRing *BCSYM::PNamespaceRing()
{
    SymbolEntryFunction;
    CastSymbolKind(NamespaceRing);
}

inline bool BCSYM::IsXmlName()
{
    SymbolEntryFunction;
    KindEquals(XmlName);
}

inline BCSYM_XmlName *BCSYM::PXmlName()
{
    SymbolEntryFunction;
    CastSymbolKind(XmlName);
}

inline bool BCSYM::IsXmlNamespaceDeclaration()
{
    SymbolEntryFunction;
    KindEquals(XmlNamespaceDeclaration);
}

inline BCSYM_XmlNamespaceDeclaration *BCSYM::PXmlNamespaceDeclaration()
{
    SymbolEntryFunction;
    CastSymbolKind(XmlNamespaceDeclaration);
}

inline bool BCSYM::IsXmlNamespace()
{
    SymbolEntryFunction;
    KindEquals(XmlNamespace);
}

inline BCSYM_XmlNamespace *BCSYM::PXmlNamespace()
{
    SymbolEntryFunction;
    CastSymbolKind(XmlNamespace);
}

inline bool BCSYM::IsApplAttr() const
{
    SymbolEntryFunction;
    KindEquals(ApplAttr);
}

inline BCSYM_ApplAttr *BCSYM::PApplAttr() const
{
    SymbolEntryFunction;
    CastSymbolKind(ApplAttr);
}

inline bool BCSYM::IsUserDefinedOperator()
{
    SymbolEntryFunction;
    KindEquals(UserDefinedOperator);
}

inline bool BCSYM::IsGenericParam()
{
    SymbolEntryFunction;
    KindEquals(GenericParam);
}

inline BCSYM_UserDefinedOperator *BCSYM::PUserDefinedOperator()
{
    SymbolEntryFunction;
    CastSymbolKind(UserDefinedOperator);
}

inline BCSYM_GenericParam *BCSYM::PGenericParam()
{
    SymbolEntryFunction;
    CastSymbolKind(GenericParam);
}

inline bool BCSYM::IsGenericConstraint()
{
    SymbolEntryFunction;
    KindSupports(GenericConstraint);
}

inline BCSYM_GenericConstraint *BCSYM::PGenericConstraint()
{
    SymbolEntryFunction;
    CastSymbolKind(GenericConstraint);
}

inline bool BCSYM::IsGenericTypeConstraint()
{
    SymbolEntryFunction;
    KindEquals(GenericTypeConstraint);
}

inline BCSYM_GenericTypeConstraint *BCSYM::PGenericTypeConstraint()
{
    SymbolEntryFunction;
    CastSymbolKind(GenericTypeConstraint);
}

inline bool BCSYM::IsGenericNonTypeConstraint()
{
    SymbolEntryFunction;
    KindEquals(GenericNonTypeConstraint);
}

inline BCSYM_GenericNonTypeConstraint *BCSYM::PGenericNonTypeConstraint()
{
    SymbolEntryFunction;
    CastSymbolKind(GenericNonTypeConstraint);
}

inline bool BCSYM::IsGenericTypeBinding() const
{
    SymbolEntryFunction;
    KindEquals(GenericTypeBinding);
}

inline BCSYM_GenericTypeBinding *BCSYM::PGenericTypeBinding() const
{
    SymbolEntryFunction;
    CastSymbolKind(GenericTypeBinding);
}

inline bool BCSYM::IsGenericBinding() const
{
    SymbolEntryFunction;
    KindSupports(GenericBinding);
}

inline BCSYM_GenericBinding *BCSYM::PGenericBinding() const
{
    SymbolEntryFunction;
    CastSymbolKind(GenericBinding);
}

inline bool BCSYM::IsTypeForwarder() const
{
    SymbolEntryFunction;
    KindEquals(TypeForwarder);
}

inline BCSYM_TypeForwarder *BCSYM::PTypeForwarder()
{
    SymbolEntryFunction;
    CastSymbolKind(TypeForwarder);
}

inline bool BCSYM::IsExtensionCallLookupResult()
{
    SymbolEntryFunction;
    KindEquals(ExtensionCallLookupResult);
}

inline BCSYM_ExtensionCallLookupResult * BCSYM::PExtensionCallLookupResult()
{
    SymbolEntryFunction;
    CastSymbolKind(ExtensionCallLookupResult);
}

inline bool BCSYM::IsLiftedOperatorMethod()
{
    SymbolEntryFunction;
    KindEquals(LiftedOperatorMethod);
}

inline BCSYM_LiftedOperatorMethod * BCSYM::PLiftedOperatorMethod()
{
    SymbolEntryFunction;
    CastSymbolKind(LiftedOperatorMethod);
}


//****************************************************************************
// Iterators.
//****************************************************************************


/**************************************************************************************************
;BCITER_HASH_CHILD

Iterate over all of the symbols in a BCSYM_Hash.
***************************************************************************************************/
struct BCITER_HASH_CHILD
{
    // Initialize the iterator.
    void Init(BCSYM_Hash *pHash);

    BCITER_HASH_CHILD() {}
    BCITER_HASH_CHILD(BCSYM_Hash *pHash)
    {
        Init(pHash);
    }

    // Get the next symbol.
    BCSYM_NamedRoot *GetNext();

protected:

    BCSYM_Hash      *m_phash;
    UINT             m_ihash;
    BCSYM_NamedRoot *m_pnamedNext;

   // Used when iterating over a SymbolList rather than a hash table
    BCSYM_NamedRoot *GetNextFromList(void);

};

/**************************************************************************************************
;BCITER_CHILD

Iterate over all of the children of a symbol.
***************************************************************************************************/
struct BCITER_CHILD
{
    // Initialize the iterator.
    void Init(
        BCSYM_NamedRoot * pnamed,
        bool fBindToUnBindableMembers = false,
        bool fLookOnlyInCurrentPartialType = false,
        bool fIncludePartialContainers = false);
   
    void InitWithFile(
        CompilerFile * pfile,
        bool fIncludePartialContainers = false);

    BCITER_CHILD() :
        m_pcontainer(NULL),
        m_pnamedNext(NULL)
    {}

    BCITER_CHILD(
        BCSYM_NamedRoot * pnamed,
        bool fBindToUnBindableMembers = false,
        bool fLookOnlyInCurrentPartialType = false,
        bool fIncludePartialContainers = false)
    {
        Init(
            pnamed,
            fBindToUnBindableMembers,
            fLookOnlyInCurrentPartialType,
            fIncludePartialContainers);
    }

    // Get the next class symbol.
    BCSYM_NamedRoot *GetNext();


protected:

    BCSYM_Container  *m_pcontainer;
    bool m_fBindToUnBindableMembers;
    bool m_fLookOnlyInCurrentPartialType;
    bool m_fIncludePartialContainers;

    BCSYM_Hash *m_CurrentHash;

    BCITER_HASH_CHILD m_phashIterator;

    BCSYM_NamedRoot *m_pnamedNext;

   // Used when iterating over a SymbolList rather than a hash table
    BCSYM_NamedRoot *GetNextFromList(void);

    void InitHashIterator();
};


// Iterates over both Bindable and UnBindable members
// Only very few folks like Bindable, MetaEmit should
// ever use this
struct BCITER_CHILD_ALL
{
    BCITER_CHILD_ALL(
        BCSYM_Container * Container,
        bool fLookOnlyInCurrentPartialType = false,
        bool fIncludePartialContainers = false)
    {
        m_Container = Container;
        m_fLookOnlyInCurrentPartialType = fLookOnlyInCurrentPartialType;
        m_fIncludePartialContainers = fIncludePartialContainers;

        Reset();
    }

    // Get the next class symbol.
    BCSYM_NamedRoot *GetNext();

    // Reset the iterator
    void Reset();

    // Get Count of Symbols
    unsigned GetCount();

protected:

    BCSYM_Container *m_Container;

    BCITER_CHILD m_MembersIter;
    bool m_CurrentlyBindingToBindableMembers;
    bool m_fLookOnlyInCurrentPartialType;
    bool m_fIncludePartialContainers;
};


/**************************************************************************************************
;BCITER_ImplInterfaces

Iterate over all the handles of a method, and handle partial methods as well,
by iterating over the handles for the associated method.
***************************************************************************************************/

struct BCITER_Handles
{
    BCITER_Handles(
        BCSYM_MethodImpl * pMethod,
        bool fLookOnlyInCurrentMethod = false) :
        m_pMethod(pMethod),
        m_pCurrentMethod(pMethod),
        m_pCurrentHandles(NULL),
        m_fLookOnlyInCurrentMethod(fLookOnlyInCurrentMethod)
    {

    }

    void Reset();

    BCSYM_HandlesList * GetNext();

private:

    BCSYM_MethodImpl* m_pMethod;
    BCSYM_MethodImpl* m_pCurrentMethod;
    BCSYM_HandlesList* m_pCurrentHandles;
    bool m_fLookOnlyInCurrentMethod;
};

/**************************************************************************************************
;BCITER_ImplInterfaces

Iterate over all of the implemented interfaces of a class.
***************************************************************************************************/
struct BCITER_ImplInterfaces
{
    // Initialize the iterator.
    void Init(
        BCSYM_Container * pClassOrInterface,
        bool fLookOnlyInCurrentPartialType = false);

    BCITER_ImplInterfaces(
        BCSYM_Container * pClassOrInterface,
        bool fLookOnlyInCurrentPartialType = false)
    {
        Init(pClassOrInterface, fLookOnlyInCurrentPartialType);
    }

    BCITER_ImplInterfaces()
    {
        Init(NULL);
    }

    void Reset();

    // Get the next interface MemberVar symbol.
    BCSYM_Implements *GetNext();

    static bool HasImplInterfaces(BCSYM_Container *ClassOrInterface);

private:

    BCSYM_Implements *m_----lCur;
    BCSYM_Container *m_pInitialClassOrInterface;
    BCSYM_Container *m_pCurrentClassOrInterface;
    bool m_fLookOnlyInCurrentPartialType;
};

/**************************************************************************************************
;BCITER_Parameters

Iterates over all parameter symbols of the specified proc symbol, in order from left to right.
Me is not included.
***************************************************************************************************/
struct BCITER_Parameters
{
    public:

        // Constructors
        BCITER_Parameters() {}
        BCITER_Parameters(BCSYM_Proc * psymProc)
        {
            Init( psymProc );
        }

        // Initialize the iterator.
        void Init(BCSYM_Proc *psymProc);

        // Get the next parameter.
        BCSYM_Param* PparamNext();

        // Get a specific argument.  Note the argument is zero based.
        BCSYM_Param* operator[]( size_t );

    private:

        BCSYM_Param* m_pparamCur;
        BCSYM_Param* m_pparamFirst;
};

/**************************************************************************************************
;BCITER_ApplAttrs

Iterate over all of the Attributes applied on a symbol.
***************************************************************************************************/
struct BCITER_ApplAttrs
{
    // Initialize the iterator.
    void Init(
        BCSYM * pSymbolHavingAttrs,
        bool fLookOnlyInImmediateSymbol = true);

    BCITER_ApplAttrs(
        BCSYM * pSymbolHavingAttrs,
        bool fLookOnlyInImmediateSymbol = true)
    {
        Init(pSymbolHavingAttrs, fLookOnlyInImmediateSymbol);
    }

    // Get the next applied attribute
    BCSYM_ApplAttr *GetNext();

    // Get the previously returned applied attribute's container context, i.e. the physical
    // container in which it has been applied.
    //
    BCSYM_NamedRoot* GetNamedContextForCurrentApplAttr();

protected:

    BCSYM *m_pOriginalSymbol;
    BCSYM *m_pSymbolHavingAttrs;
    BCSYM_ApplAttr *m_pNextApplAttr;
    bool m_fLookOnlyInImmediateSymbol;
};

//
// Iterate over all of the children of a symbol.  This version is
// safe to use even if the list of children changes while
// iterating.
//

class  BCITER_CHILD_SAFE
{
public:
    BCITER_CHILD_SAFE(BCSYM_NamedRoot *psym, bool ShouldInit = true);
    ~BCITER_CHILD_SAFE();

    // Get the next symbol.
    BCSYM_NamedRoot *GetNext();

    // Resets the iteration back to the beginning.
    void Reset();

protected:
        // Initialize the iterator.
    void Init(BCSYM_NamedRoot *psym);

private:
    unsigned CountSymbols(BCSYM_NamedRoot *);

protected:
    unsigned m_iSym;
    unsigned m_cSyms;
    BCSYM_NamedRoot **m_rgpnamed;
    
    //Microsoft avoid allocation if possible.
    #define CHILD_ITER_BUF_COUNT 8
    BCSYM_NamedRoot* m_Syms[CHILD_ITER_BUF_COUNT];
    bool m_fAllocated;
};

//
// Iterate over all of the children of a symbol in name order.
//

class BCITER_CHILD_SORTED : public BCITER_CHILD_SAFE
{
public:
    BCITER_CHILD_SORTED(BCSYM_NamedRoot *psym);

protected:
    // Initialize the iterator.
    void Init(BCSYM_NamedRoot *psym);
};

// Iterate over the inheritance heirarchy of a Class OR Interface

struct InheritanceWalker
{
    InheritanceWalker(
        Compiler * CompilerInstance,
        BCSYM_Container * ContainerToWalk);

    BCSYM_Container *GetMostDerived()  { m_CurrentIdx = 0; return GetNextBase(); } // The first guy in the list is the container Init was called on
    BCSYM_Container *GetMostBase()     { m_CurrentIdx = m_InheritanceArray.Count() > 0 ? m_InheritanceArray.Count() - 1 : 0; return GetPreviousBase();  }
    BCSYM_Container *GetNextBase()     { return m_CurrentIdx < (int)m_InheritanceArray.Count() ? m_InheritanceArray.Element( m_CurrentIdx++ ) : NULL; } // walk from most derived to most base
    BCSYM_Container *GetPreviousBase() { return ( m_CurrentIdx >= 0 && m_CurrentIdx < (int)m_InheritanceArray.Count()) ? m_InheritanceArray.Element( m_CurrentIdx-- ) : NULL; } // walk from most base to most derived

private:

    void AddInterfaceBases( BCSYM_Container *Interface );

    unsigned m_CurrentIdx;
    DynamicArray<BCSYM_Container*> m_InheritanceArray;
};

//
// Iterate over all of the children of a symbol sorted by
// file declaration order.
//

/**************************************************************************************************
;BCITER_Classes

Iterate over all the classes in a file
***************************************************************************************************/
struct BCITER_Classes
{
    BCITER_Classes() : m_pFile(NULL), m_pNext(NULL) {}

    // Initialize the iterator.
    void Init(
        CompilerFile * pFile,
        bool m_fIncludePartialContainers = false);

    // Get the next interface MemberVar symbol.
    BCSYM_Class *Next();

private:
    CompilerFile *m_pFile;
    BCSYM_NamedRoot * m_pNext;
    bool m_fIncludePartialContainers;
};

/**************************************************************************************************
;BCITER_Constraints

Iterate over all the constraint of a generic param
***************************************************************************************************/
struct BCITER_Constraints
{
private:

    // Initialize the iterator to start iterating over the constraints of the given
    // generic parameter. Note that the generic parameter provided can be NULL
    //
    void Init(
        BCSYM_GenericParam * pGenericParam,
        bool fDigThroughTypeParamTypeConstraints,
        Compiler * pCompiler);
    
    // Add the given constraint to the end of the constraints in progress list
    void PushIntoConstraintsInProgressList(BCSYM_GenericConstraint *pConstraint);

    // Removes the last constraint from the constraints in progress list and returns it
    BCSYM_GenericConstraint* PopFromConstraintsInProgressList();

    // Free the constraints in progress list if allocated on the heap
    void FreeConstraintsInProgressList();

    bool ConstraintsInProgress()
    {
        return m_CountOfConstraintsInProgress > 0;
    }

public:

    BCITER_Constraints(
        BCSYM_GenericParam * GenericParam,
        bool DigThroughTypeParamTypeConstraints,
        Compiler * CompilerInstance)
    {
        Init(GenericParam, DigThroughTypeParamTypeConstraints, CompilerInstance);
    }

    BCITER_Constraints
    (
        Compiler *pCompiler
    )
    {
        Init(NULL, false, pCompiler);
    }

    ~BCITER_Constraints()
    {
        FreeConstraintsInProgressList();
    }

    // Get the next interface MemberVar symbol.
    BCSYM_GenericConstraint* Next();

    // Get the first constraint from the list of constraints in progress.
    BCSYM_GenericConstraint *FirstGenericParamConstraintContext()
    {
        if (m_CountOfConstraintsInProgress > 0)
        {
            return m_ppConstraintsInProgress[0];
        }

        return NULL;
    }

    // Get the last constraint from the list of constraints in progress.
    BCSYM_GenericConstraint *CurrentGenericParamConstraintContext()
    {
        if (m_CountOfConstraintsInProgress > 0)
        {
            return m_ppConstraintsInProgress[m_CountOfConstraintsInProgress - 1];
        }

        return NULL;
    }

    // Get the generic param type of the first constraint from the list of constraints in progress.
    BCSYM_GenericParam *FirstGenericParamContext()
    {
        if (m_CountOfConstraintsInProgress > 0)
        {
            return m_ppConstraintsInProgress[0]->PGenericTypeConstraint()->GetType()->PGenericParam();
        }

        return m_pGenericParam;
    }

    // Get the generic param type of the last constraint from the list of constraints in progress.
    BCSYM_GenericParam *CurrentGenericParamContext()
    {
        if (m_CountOfConstraintsInProgress > 0)
        {
            return m_ppConstraintsInProgress[m_CountOfConstraintsInProgress - 1]->
                        PGenericTypeConstraint()->GetType()->PGenericParam();
        }

        return m_pGenericParam;
    }

    void Reset()
    {
        m_pNextConstraint = m_pGenericParam->GetConstraints();
        m_CountOfConstraintsInProgress = 0;
    }

private:

    BCSYM_GenericParam *m_pGenericParam;
    bool m_fDigThroughTypeParamTypeConstraints;
    Compiler *m_pCompiler;

    BCSYM_GenericConstraint *m_pNextConstraint;
    BCSYM_GenericConstraint **m_ppConstraintsInProgress;            // Constraints for which digging is in progress
    BCSYM_GenericConstraint *m_ppConstraintsInProgressBuffer[12];   // Buffer to use for the constraints

    unsigned int m_CountOfConstraintsInProgress;
    unsigned int m_MaxAllowedConstraintsInProgress;
};


//****************************************************************************
// Random inlined implementation.
//****************************************************************************

inline bool BCSYM::IsTransientClass()
{
    SymbolEntryFunction;
    return this && IsClass() && PClass()->IsTransient();
}

// Is this the intrinsic type symbol for "Object"?
inline bool BCSYM::IsObject()
{
    SymbolEntryFunction;
    return IsClass() && PClass()->IsObject();
}

// Is this the intrinsic type symbol for "System.ValueType"?
inline bool BCSYM::IsCOMValueType()
{
    SymbolEntryFunction;
    return IsClass() && PClass()->IsCOMValueType();
}

//============================================================================
// Determine if the given symbol corresponds to a Class Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class VB Construct
//    - False: otherwise
//============================================================================
inline bool
IsClassOnly
(
    BCSYM_NamedRoot *pSymbol
)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsClass() &&
        !pSymbol->PClass()->IsStruct() &&        // Not a Structure
        !pSymbol->PClass()->IsEnum() &&          // Not an Enum
        !pSymbol->PClass()->IsDelegate() &&      // Not a Delegate
        !pSymbol->PClass()->IsStdModule();       // Not a Module
}


//============================================================================
// Determine if the given symbol corresponds to a Structure Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Structure VB Construct
//    - False: otherwise
//============================================================================
inline bool
IsStructOnly
(
    BCSYM_NamedRoot *pSymbol
)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsClass() &&
        pSymbol->PClass()->IsStruct() &&         // Not a Structure
        !pSymbol->PClass()->IsEnum() &&          // Not an Enum
        !pSymbol->PClass()->IsIntrinsicType();   // Not an Intrinsic
}

//============================================================================
// Determine if the given container corresponds to a Class or Structure Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class or Structure VB Construct
//    - False: otherwise
//============================================================================
inline bool
IsClassStructOrModuleOnly
(
    BCSYM_NamedRoot *pSymbol
)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsClass() &&
        !pSymbol->PClass()->IsEnum() &&          // Not an Enum
        !pSymbol->PClass()->IsDelegate();        // Not a Delegate
}

//============================================================================
// Determine if the given container corresponds to a Class or Structure Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class or Structure VB Construct
//    - False: otherwise
//============================================================================
inline
bool IsClassOrStructOnly(BCSYM_NamedRoot * pSymbol)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsClass() &&
        !pSymbol->PClass()->IsEnum() &&          // Not an Enum
        !pSymbol->PClass()->IsDelegate() &&      // Not a Delegate
        !pSymbol->PClass()->IsStdModule();       // Not a Module
}

//============================================================================
// Determine if the given container corresponds to a Class or Structure Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class or Structure VB Construct
//    - False: otherwise
//============================================================================
inline
bool IsClassStructOrEnumOnly(BCSYM_NamedRoot * pSymbol)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsClass() &&
        !pSymbol->PClass()->IsDelegate() &&      // Not a Delegate
        !pSymbol->PClass()->IsStdModule();       // Not a Module
}


//============================================================================
// Determine if the given symbol corresponds to a Class or Interface Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class or InterfaceVB Construct
//    - False: otherwise
//============================================================================
inline
bool IsClassOrInterfaceOnly(BCSYM_NamedRoot * pSymbol)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsInterface() ||
        IsClassOnly(pSymbol);
}

//============================================================================
// Determine if the given symbol corresponds to a Class, Structure or Interface Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class, Structure or Interface VB Construct
//    - False: otherwise
//============================================================================
inline
bool IsClassStructOrInterfaceOnly(BCSYM_NamedRoot * pSymbol)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsInterface() ||
        IsClassOrStructOnly(pSymbol);
}

//============================================================================
// Determine if the given symbol corresponds to a Class, Structure, Enum or Interface Language construct.
//
// Returns:
//    - True: if the symbols corresponds to a Class, Structure, Enum or Interface VB Construct
//    - False: otherwise
//============================================================================
inline
bool IsClassStructEnumOrInterfaceOnly(BCSYM_NamedRoot * pSymbol)
{
    VSASSERT(pSymbol != NULL, "Invalid Symbol");

    return pSymbol->IsInterface() ||
        IsClassStructOrEnumOnly(pSymbol);
}

inline
bool IsAnonymousTypeProperty(BCSYM * pSym)
{
    return pSym &&
        pSym->IsProperty() &&
        pSym->PProperty()->GetContainer() &&
        pSym->PProperty()->GetContainer()->IsAnonymousType();

}
// Helper to check if a type is a valid attribute type
bool IsValidAttributeType(
    BCSYM * pType,
    CompilerHost * pCompilerHost);

