//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Interface to VB semantic analysis.
//
//  Semantic Analysis Model
//  
//  Semantic analysis is a process of traversing unbound (parse) trees as input,
//  determining their semantic validity (issuing diagnostic messages as
//  appropriate), and producing bound trees as output. In this compiler,
//  this mutates the unbound trees, but this is not an intrinsic property
//  of the process.
//  
//  Semantic analysis is implemented as recursive top-down interpretation with
//  context. If determining the meaning of a given construct requires information
//  about the context in which the construct occurs, that context information is
//  supplied to the interpretation. (This is the purpose of the expression flags
//  and the name flags.) A goal of this model is to minimize the number of places
//  where a specific semantic action occurs. Another goal is to make it possible to
//  fully understand the interpretation of a construct by examining a localized
//  body of code.
//  
//  Semantic analysis observes the Principles of Badness, which minimize the
//  production of cascading extraneous semantic errors and avoid compiler crashes.
//  Briefly stated, these are:
//    1) A construct with any Bad operands is Bad and generates no error messages.
//    2) A construct with no Bad operands is Bad and generates an error message if
//       and only one of its semantic checks fails.
//  Badness is encoded in trees using the SF_ERROR flag.
//  
//  Here, then, is the general model for interpreting any semantic construct:
//    1) Interpret all operands, supplying context information as appropriate.
//    2) If any interpreted operand is bad, return a bad result.
//    3) Check semantic constraints. If any fail, produce an error message
//       and return a bad result.
//    4) Perform tree transformations and return the bound tree for the
//       construct.
//  
//  This model is simple, robust, and adequate for the analysis of complex modern
//  programming languages.
//  
//  However, there are semantic issues with VB that prevent performing semantic
//  analysis in a purely top-down fashion. Specifically, assignments to properties
//  (including late-bound properties and indexed variants) require the
//  interpretation of the right operand of the assignment to be available at the
//  point where the property assignment is to be generated, and pushing the
//  processing of assignment statements into the expression interpreter would be
//  messy. Fetching or addressing properties or indexed variants have some other
//  irregularities that are impossible to capture with simple context information.
//  
//  Therefore, semantic analysis of references to properties, late-bound calls, and
//  indexed properties involves deferring some choices until full context information
//  is available. The mechanisms used for this are SX_PROPERTY_REFERENCE and
//  SX_LATE_REFERENCE trees, which represent property references or late-bound
//  references, respectively, which have been incompletely processed. Passing in
//  the ExprPropagatePropertyReference flag allows the expression interpreter to
//  build such a tree instead of fully interpreting a property or late reference.
//  Cases that pass in this flag must be prepared to deal with an
//  SX_PROPERTY_REFERENCE or SX_LATE_REFERENCE tree as the result of an
//  expression interpretation.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#if HOSTED
#include "VBHostedInterop.h"
#endif

#define StackAllocTypeArgumentsIfPossible_EX(TypeArguments, ArgumentCount, Allocator, SymbolCreator, TypeArgumentsBuffer, TypeArgumentsStackAllocated)    \
                                                                                                        \
    TypeArgumentsStackAllocated = false;                                                                \
                                                                                                        \
    if (ArgumentCount == 0)                                                                             \
    {                                                                                                   \
        TypeArguments = NULL;                                                                           \
    }                                                                                                   \
    else if (SymbolCreator.AreBindingsCached() &&                                                       \
            (ArgumentCount) <= _countof((TypeArgumentsBuffer)))                                         \
    {                                                                                                   \
        memset(TypeArgumentsBuffer, 0, (size_t)sizeof((TypeArgumentsBuffer)));                            \
        TypeArguments = TypeArgumentsBuffer;                                                            \
        TypeArgumentsStackAllocated = true;                                                             \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        TypeArguments = new(*(Allocator)) BCSYM *[(ArgumentCount)];                                     \
    }


#define StackAllocTypeArgumentsIfPossible(TypeArguments, ArgumentCount, Allocator, SymbolCreator)       \
    Type *_TypeArgumentsBuffer[12];                                                                     \
    bool TypeArguments##StackAllocated = false;                                                         \
    StackAllocTypeArgumentsIfPossible_EX(TypeArguments, ArgumentCount, Allocator, SymbolCreator, _TypeArgumentsBuffer, TypeArguments##StackAllocated);

class Cycles;
class CallGraph;
class SourceFile;

struct MemberInfoList;

// Flags for expression interpretation.

#define ExprMustBeConstant 0x1
#define ExprArgumentsMustBeConstant 0x2
#define ExprIsOperandOfConditionalBranch 0x4
#define ExprAccessDefaultProperty 0x8
#define ExprForceRValue 0x10
#define ExprIsExplicitCallTarget 0x20
#define ExprIsAssignmentTarget 0x40
#define ExprCreateImplicitDeclarations 0x80
// #define ExprConditionalCompilation 0x80
#define ExprAllowTypeReference 0x100
#define ExprTreatQualifiedNamesEnMasse 0x200
#define ExprAllowNamespaceReference 0x400
#define ExprSuppressMeSynthesis 0x800
#define ExprResultNotNeeded 0x1000
// #define ExprCallIsDelegateInvocation 0x2000
#define ExprPropagatePropertyReference 0x4000
#define ExprIsExplicitCast 0x8000
#define ExprIsConstructorCall 0x10000
#define ExprIsPropertyAssignment 0x20000
#define ExprHasExplicitCastSemantics 0x40000
#define ExprSuppressLateBinding 0x80000
#define ExprSuppressImplicitVariableDeclaration 0x100000
#define ExprIsOperandOfConcatenate 0x200000
#define ExprTypeReferenceOnly 0x400000     // Map to NameSearchTypeReferenceOnly which maps to BINDSPACE_Type
#define ExprIsInitializationCall 0x800000
#define ExprLeadingQualifiedName 0x1000000 // Identifies the qualifiers in a qualified name (W, X, Y in qualified name W.X.Y.Z).

//





#define ExprSuppressDefaultInstanceSynthesis 0x2000000
#define ExprSuppressTypeArgumentsChecking 0x4000000 // This flag is for internal use within semantics only and should not be set
                                                    // by anybody outside of semantics.
                                                    // It is used to suppress type arguments checking for generic types, i.e.
                                                    // don't check whether a generic type is being referred to without any type
                                                    // arguments.
#define ExprTypeInferenceOnly 0x8000000 // The purpose of intepretation is for type infereince only.  Set this flag skips certain operations.
#define ExprHasDirectCastSemantics 0x10000000 // Identifies a cast made with DirectCast
#define ExprHasTryCastSemantics 0x20000000 // Identifies a cast made with TryCast
#define ExprForceConstructorCall 0x40000000  // if ExprIsConstructorCall is set, treat CallOrIndex as constructor only
#define ExprSkipOverloadResolution 0x80000000
#define ExprIsLHSOfObjectInitializer 0x100000000  // Identifies that the expression being analyzed in the LHS of an initializer
                                                  // in an object initializer. Eg: ".Prop" in New SomeType With {.Prop = 1}
#define ExprInferLoopControlVariableExplicit 0x200000000
#define ExprDontInferResultType 0x400000000   // This flag will infer the ResultType of the expression if it isn't directly computed by
                                                  // interpretation. An example of this is lambda's. If we have a LHS type we don't want to infer
                                                  // it because it will generate an anonymous delegate type. But if we don't we will need to.
#define ExprInferResultTypeExplicit 0x800000000   // This flag will infer the ResultType of the expression if it isn't directly computed by

//This is used when interpreting DotQualifiedExpressions that result in extension method bindings.
//In particular, if the call to InterpretQualifiedExpression results in either a call expression
//that bound to an extension method with a byref first argument or an SX_ExtensionCall node with
//at least one candidate that has a byref first argument, we will reinterpret both the qualified expression
//and it's base reference by passing in ExprPropagatePropertyReference. This is done by a recursive call
//to InterpretExpression with this flag set.
#define ExprForceBaseReferenceToPropigatePropertyReference 0x1000000000

#define ExprIsQueryOperator 0x2000000000 // Set while interpreting Qualified expression, which represents a reference to a query operator, causes NameSearchMethodsOnly flag to be passed to the name lookup

#define ExprGetLambdaReturnTypeFromDelegate 0x4000000000 // We can get the lambda's return type from the delegate. 

//Indicates that casts that SX_WIDECOERCE expressions should not be used, but instead an explicit cast expression should be used.
//This is needed when processing IF expressions involving
//interfaces because of some limitations in the CLR verifier.
#define ExprSuppressWideCoerce  0x8000000000 
#define ExprCreateNestedArrayLiteral 0x10000000000
//Tells InterpretExpression to create a ColInitElement expression instead of a 
//call expression. Interpreting a call or index expression without this flag will 
//yield a giant SX_SEQ in the event that copy-back expressions are required for by-ref 
//arguments. We don't want to deal with digging through those expressions when
//we are generating expression trees. As a result, passing in the flag will yield an 
//ColInitElementExpression, which seperates the main call expression from the copy-back
//sequence into two seperate fields. That way expression tree generation only has to deal
//with the call part, and can ignore the copy-backs, which are implemented inside the
//expression tree compiler. 
#define ExprCreateColInitElement 0x20000000000
#define ExprCreateDelegateInstance 0x40000000000
#define ExprSpeculativeBind 0x80000000000   // Used to avoid mutating state when calling InterpretAwait

#define ExprNoFlags ((ExpressionFlags)0)
const int ExprScalarValueFlags = (ExprForceRValue);

typedef unsigned __int64 ExpressionFlags;

// Flags for overload resolution.

#define OvrldExactArgCount 0x01  // reject candidates that can accept more parametrs than amount of supplied arguments
#define OvrldIgnoreParamArray 0x02  // do not expand ParamArray
#define OvrldDisableTypeArgumentInference 0x04 // do not try to infer Type arguments
#define OvrldIgnoreEvents 0x08 // do not include events into overload resolution
#define OvrldIgnoreSharedMembers 0x10 // do not include static members into candidate list
#define OvrldIgnoreSubs 0x20 // do not include Subs into candidate list
#define OvrldIgnoreProperties 0x40 // do not include properties into candidate list
#define OvrldIgnoreLateBound 0x80 // do not allow overload resolution to revert to latebound call in case of multiple ambigious arguments all narrowing to object.
#define OvrldPreferSubOverFunction 0x100 // in case ambiguity exists, prefer subs over functions
#define OvrldPreferFunctionOverSub 0x200 // in case ambiguity exists, prefer function over subs
#define OvrldDontReportSingletonErrors 0x400 // used by delegate overload resolution to ensure that overload resolution won't report specific errors about calling the method.
#define OvrldDontReportAddTypeParameterInTypeInferErrors 0x800 // used by query overload resolution to not ask the user to specify the type parameters explicitly in case of type inference failure.
#define OvrldForceOverloadResolution 0x1000 //go through overload resolution even in the case of a single method
#define OvrldSomeCandidatesAreExtensionMethods 0x2000 //Indicates that at least one candidate method is an extension method

//Tells overload resolution to return a method that is uncallable as if it had passed overload resolution if type inference succeeded for it, and it was the only available candidate.
//This allows extension method overload resolution to support the special case error messages reported by delegates without needed a seperate
//version of overload resolution like Instance method delegates do.
#define OvrldReturnUncallableSingletons 0x4000 
#define OvrldReportErrorsForAddressOf 0x8000 //Indicates that overload resolution errors for addressof should be used.

#define OvrldSkipTargetResolution 0x10000 // the Target has already had overload-resolution done in it

typedef unsigned OverloadResolutionFlags;
#define OvrldNoFlags ((OverloadResolutionFlags)0)


// Flags for name interpretation.

#define NameSearchImmediateOnly 0x1     // Don't look beyond the given scope.
#define NameSearchIgnoreParent 0x2      // Look in base clases but not in enclosing scopes
                                        // and imported namespaces.
                                        // However, if the given scope is a namespace, then
                                        // do a lookup in all namespaces of the same name.
// #define NameSearchAllowTypeReference 0x4
#define NameSearchIgnoreImports 0x8    // Don't look in any imported namespaces.
#define NameSearchTypeReferenceOnly 0x10
#define NameSearchConditionalCompilation 0x20
#define NameSearchDonotResolveImportsAlias 0x40
#define NameSearchAttributeReference 0x100
#define NameSearchUnnamedNamespace 0x200
#define NameSearchLeadingQualifiedName 0x400  // Identifies the qualifiers in a qualified name (W, X, Y in qualified name W.X.Y.Z).
#define NameSearchFirstQualifiedName 0x800    // Identifies the first name in a qualified name (W in qualified name W.X.Y.Z).
#define NameSearchEventReference 0x1000
#define NameSearchCoClassContext 0x2000
#define NameSearchIgnoreModule 0x4000
#define NameSearchIgnoreImmediateBases 0x8000   // Don't look in immediate context's bases.
                                                // Useful when resolving bases for interfaces
                                                // in order to avoid looking at the interface's
                                                // bound bases when resolving other bases of the
                                                // same interface.

#define NameSearchIgnoreBases 0x10000           // Don't look in any entities' bases. Useful when
                                                // resolving imports.

#define NameSearchDoNotBindTypeArguments 0x20000
#define NameSearchIgnoreExtensionMethods 0x40000   //Do not consider extension methods during name lookup.
#define NameSearchIgnoreObjectMethodsOnInterfaces 0x80000   //Do not consider Object methods during name lookup on interfaces.

#define NameSearchLocalsOnly 0x100000 //We only want to search within local scopes
#define NameSearchBindingAnonymousTypeFieldInSyntheticMethod 0x200000 // A hack to let name binding know that we are binding an anonymous type field.
#define NameSearchDoNotSubstituteAnonymousTypeName 0x400000 // A hack to prevent interpret name from doing anonymous type name substutition.
#define NameSearchDoNotMergeNamespaceHashes 0x800000 // A hack to prevent interpret name from merging hashes yet since that is supposed to be done by bindable.

#define NameSearchMethodsOnly 0x1000000 //We are looking for methods, want to ignore properties, events, fields, nested types, like they are not there

#define NameSearchCanonicalInteropType 0x2000000 // #546428 We are looking for canonical interop type
#define NameSearchIgnoreAccessibility 0x4000000 // We are resolving symbol descriptor back to symbol and want to ignore accessibility checks
#define NameSearchForceUnspecifiedBaseArity 0x8000000 // Xml Doc comments allow any arity for the bases (See Dev11 405893)
#define NameSearchGlobalName (NameSearchIgnoreModule | NameSearchIgnoreParent | NameSearchIgnoreImports)
#define NameSearchGlobalNameButLookIntoModule (NameSearchIgnoreParent | NameSearchIgnoreImports)


#define NameNoFlags ((NameFlags)0)

typedef unsigned __int64 NameFlags;

// Flags for type resolution.

#define TypeResolveAllowAllTypes 0x1      // Allow use of module & void as a type
#define TypeResolvePerformObsoleteChecks 0x2  // Check resolved type for obsoleteness
#define TypeResolveDontMarkContainingProcAsBad 0x4  // For resolving types for lambda expressions, where we won't mark the context method (e.g. "Sub Main") as bad if there's something wrong in a lambda inside it

#define TypeResolveNoFlags ((TypeResolutionFlags)0)

typedef unsigned __int64 TypeResolutionFlags;

// MatchGenericArgumentParameter:
// This is used in type inference, when matching an argument e.g. Arg(Of String) against a parameter Parm(Of T).
// In covariant contexts e.g. Action(Of _), the two match if Arg <= Parm (i.e. Arg inherits/implements Parm).
// In contravariant contexts e.g. IEnumerable(Of _), the two match if Parm <= Arg (i.e. Parm inherits/implements Arg).
// In invariant contexts e.g. List(Of _), the two match only if Arg and Parm are identical.
// Note: remember that rank-1 arrays T() implement IEnumerable(Of T), IList(Of T) and ICollection(Of T).
enum MatchGenericArgumentToParameterEnum
{
    MatchBaseOfGenericArgumentToParameter,
    MatchArgumentToBaseOfGenericParameter,
    MatchGenericArgumentToParameterExactly
};


// Flags for query interpretation
enum QueryExpressionFlagValues
{
    // bit mask
    QueryNoFlags               =0x00,
    ControlVarIsRecord      =0x01,
    ControlVarIsNested      =0x02,
    ControlVarIsHidden      =0x04,
    AppliedAtLeastOneOperator=0x08,
};

typedef unsigned QueryExpressionFlags;

// Enum for emitting a warning that you've apssed Async Sub when maybe it should have been Async Function
typedef unsigned AsyncSubAmbiguityFlags;
enum AsyncSubAmbiguityFlagValues
{
    FoundNoAsyncOverload         = 0x00,
    FoundAsyncOverloadWithAction = 0x01,
    FoundAsyncOverloadWithFunc   = 0x02
};

typedef DynamicHashTable<ILTree::Expression*, AsyncSubAmbiguityFlags, VBAllocWrapper> AsyncSubAmbiguityFlagCollection;


// Type aliases to remove ugly grot.

typedef ILTree::ExpressionWithChildren UnaryExpression;
typedef ILTree::Expression ExpressionList;
typedef ILTree::PropertyReferenceExpression LateReferenceExpression;
typedef ILTree::StatementWithExpression CloseStatement;
typedef ILTree::StatementWithExpression EraseStatement;
typedef ILTree::StatementWithExpression AssertStatement;

typedef LABEL_ENTRY_BIL LabelDeclaration;

typedef __int64 Quadword;

const int MAX_SINGLE_BITS = 0x7f7fffff;
const float MAX_SINGLE = *(const float *)&MAX_SINGLE_BITS;
const int MIN_SINGLE_BITS = 0xff7fffff;
const float MIN_SINGLE = *(const float *)&MIN_SINGLE_BITS;

// This Resume index represents a statement which has no Resume behavior.  The value 0xffff was
// rejected because an off-by-one bug could mistakenly cause an invalid index to become this value.
const unsigned short NoResume = 0xfffe;

// Variable count buffer for for loops.
const unsigned short ForLoopVariableCount = 0x2;

struct OverloadList;
class TypeSet;
class Semantics;

//==============================================================================
// This class represents all of the state in semantics regarding the procedure
// context that semantics is operating in.  This struct allows us to save and
// replace the ProcedureState of an instance of Semantics.
//
// This is especially valuable to closures which must call back into semancis
// in different states when it is updating several types of items
//==============================================================================
struct ProcedureState
{
    Procedure *Proc;
    ILTree::ProcedureBlock *Body;
    TemporaryManager *TemporaryMgr;

    ProcedureState() :
        Proc(NULL),
        Body(NULL),
        TemporaryMgr(NULL)
    {

    }

    ProcedureState(Procedure *proc, ILTree::ProcedureBlock *body, TemporaryManager *tempManager) :
        Proc(proc),
        Body(body),
        TemporaryMgr(tempManager)
    {

    }

    ProcedureState(_In_ ILTree::ProcedureBlock *body) :
        Proc(body->pproc),
        Body(body),
        TemporaryMgr(body->ptempmgr)
    {

    }
};

struct TrackedDelegateCtorCallList
{
    ILTree::DelegateConstructorCallExpression *DelegateCtorCall;
    TrackedDelegateCtorCallList *Next;
};

typedef void
(Semantics::*ArgumentDetector)
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    GenericBindingInfo GenericBindingContext,
    ExpressionList *Arguments,
    Type *DelegateReturnType,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethods,
    OverloadResolutionFlags OvrldFlags
);

typedef bool
(*CandidateProperty)
(
    OverloadList *Candidate
);

typedef void
(Semantics::*ParameterCompare)
(
    ILTree::Expression *Argument,
    Parameter *LeftParameter,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    bool ExpandLeftParamArray,
    Parameter *RightParameter,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool ExpandRightParamArray,
    bool &LeftIsBetter,
    bool &RightIsBetter,
    bool &BothLose,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
);

typedef void
(Semantics::*ParameterTypeCompare)
(
    Type *LeftParameterType,
    Procedure *LeftProcedure,
    GenericBindingInfo LeftBinding,
    Type *RightParameterType,
    Procedure *RightProcedure,
    GenericBindingInfo RightBinding,
    bool &LeftIsBetter,
    bool &RightIsBetter,
    bool &BothLose,
    bool LeftIsExtensionMethod,
    bool RightIsExtensionMethod
);

typedef ErrorTable*
(*AlternateErrorTableGetter)
(
    SourceFile *File
);


DECLARE_ENUM(ConversionSemantics)

    // Logical conversion flags:
    //
    // These say which conversions should be considered. They're useful only to implementers of
    // ClassifyPredefinedCLRConversion, and are documented above that function. No one else should
    // use these logical flags.
    // Note that, implicitly, every conversion function assumes it can do variance conversions, array covariance,
    // interface inheritance &c. The flags here are to indicate which additional conversions are allowed.
    //
    FLAG_VALUE(AllowBoxingConversions),           // e.g. GenericParamX->Interface if X isn't constrained to be a reference
    FLAG_VALUE(AllowIntegralConversions),         // e.g. Enum->Int -- conversions that keep the same bitsize
    FLAG_VALUE(AllowArrayIntegralConversions),    // e.g. Enum[] -> UnderlyingType[]
    FLAG_VALUE(OverestimateNarrowingConversions), // e.g. S[]->T[], i.e. ALL conversions that MIGHT CONCEIVABLY suceed at runtime.
    //
    // * We don't need a flag AllowArrayBoxingConversions e.g. Integer[] -> Object[].
    //   It's not allowed by the CLR and so is never useful.
    // * Do we ever encounter AllowArrayValueConversions but not AllowValueConversions? Yes, all the time...
    //   eg. in variance, IReadOnly(T) -> IReadOnly(U) depends on T->U, and this T->U conversion isn't allowed to use
    //   value conversions, but it is allowed to use ArrayValue conversions.
    // * Do we ever encounter AllowValueConversions but not AllowArrayValueConversions? No, not that I can see.
    // * Do we ever encounter AllowIntegralConversions with OverestimateNarrowingConversions? It's a conceivable concept,
    //   but the only time when OverestimateNarrowing is used is within variance, and that doesn't allow integral conversions,
    //   so the flags never end up being used together in practice.

    
    // Semantic conversion flags: (classifying conversions according to their semantic intent)
    // All users of ClassifyPredefinedCLRConversion should use this flags instead of the logical ones.
    // That way, if additional conversion types are added, we'll know where and how and why they were added.
    Default             = AllowBoxingConversions | AllowIntegralConversions | AllowArrayIntegralConversions,
    ForConstraints      = AllowBoxingConversions,
    ForExtensionMethods = AllowBoxingConversions,
    ReferenceConversions= AllowArrayIntegralConversions
    //
END_ENUM(ConversionSemantics);
DECLARE_ENUM_OPERATORS(ConversionSemantics);


enum ConversionClass
{
    ConversionIdentity,   // The types are exactly equivalent, no conversion needed
    ConversionWidening,   // The conversion is widening and can not fail
    ConversionNarrowing,  // The conversion is narrowing and can fail
    ConversionError,      // There is no conversion
};

inline ConversionClass BestConversion(ConversionClass conversion1, ConversionClass conversion2)
{
    return (conversion1<conversion2) ? conversion1 : conversion2;
}

inline ConversionClass WorstConversion(ConversionClass conversion1, ConversionClass conversion2)
{
    return (conversion1>conversion2) ? conversion1 : conversion2;
}



typedef unsigned MethodConversionClass;
enum MethodConversionClassEnum
{
    MethodConversionIdentity                            = 0x00000000,
    MethodConversionOneArgumentIsVbOrBoxWidening        = 0x00000001,
    MethodConversionOneArgumentIsClrWidening            = 0x00000002,
    MethodConversionOneArgumentIsNarrowing              = 0x00000004,
    MethodConversionReturnIsWidening                    = 0x00000008,
    MethodConversionReturnIsClrNarrowing                = 0x00000010,
    MethodConversionReturnIsIsVbOrBoxNarrowing          = 0x00000020,
    MethodConversionReturnValueIsDropped                = 0x00000040,
    MethodConversionAllArgumentsIgnored                 = 0x00000080,
    MethodConversionExcessOptionalArgumentsOnTarget     = 0x00000100,
    MethodConversionExcessParamsArgumentOnDelegate      = 0x00000200,
    MethodConversionLateBoundCall                       = 0x00000400,
    MethodConversionByRefByValMismatch                  = 0x00000800,
    MethodConversionNotAMethod                          = 0x00001000,
    MethodConversionError                               = 0x00002000,
    MethodConversionExtensionMethod                     = 0x00004000

};

DECLARE_ENUM(ConversionRequired)
    // When we do type inference, we have to unify the types supplied and infer generic parameters.
    // e.g. if we have Sub f(ByVal x as T(), ByVal y as T) and invoke it with x=AnimalArray, y=Mammal,
    // then we have to figure out that T should be an Animal. The way that's done:
    // (1) All the requirements on T are gathered together, e.g.
    //     T:{Mammal+vb, Animal+arr} means
    //     (+vb)  "T is something such that argument Mammal can be supplied to parameter T"
    //     (+arr) "T is something such that argument Animal() can be supplied to parameter T()"
    // (2) We'll go through each candidate type to see if they work. First T=Mammal. Does it work for each requirement?
    //     (+vb)  Yes,     argument Mammal can be supplied to parameter Mammal through identity
    //     (+arr) Sort-of, argument Animal() can be supplied to parameter Mammal() only through narrowing
    // (3) Now try the next candidate, T=Animal. Does it work for each requirement?
    //     (+vb)  Yes,     argument Mammal can be supplied to parameter Animal through widening
    //     (+arr) Yes,     argumetn Animal() can be supplied to parameter Animal() through identity
    // (4) At the end, we pick out the one that worked "best". In this case T=Animal worked best.
    // The criteria for "best" are documented and implemented in ConversionResolution.cpp/FindDominantType.

    // This enumeration contains the different kinds of requirements...
    // Each requirement is that some X->Y be a conversion. Inside FindDominantType we will grade each candidate
    // on whether it could satisfy that requirement with an Identity, a Widening, a Narrowing, or not at all.

    // Identity:
    // This restriction requires that req->candidate be an identity conversion according to the CLR.
    // e.g. supplying "New List(Of Mammal)" to parameter "List(Of T)", we require that Mammal->T be identity
    // e.g. supplying "New List(Of Mammal)" to a parameter "List(Of T)" we require that Mammal->T be identity
    // e.g. supplying "Dim ml as ICovariant(Of Mammal) = Nothing" to a parameter "*ByRef* ICovariant(Of T)" we require that Mammal->T be identity
    // (but for non-ByRef covariance see "ReferenceConversion" below.)
    // Note that CLR does not include lambda->delegate, and doesn't include user-defined conversions.
    Identity,

    // Any:
    // This restriction requires that req->candidate be a conversion according to VB.
    // e.g. supplying "New Mammal" to parameter "T", we require that Mammal->T be a VB conversion
    // It includes user-defined conversions and all the VB-specific conversions.
    Any,

    // AnyReverse:
    // This restriction requires that candidate->req be a conversion according to VB.
    // It might hypothetically be used for "out" parameters if VB ever gets them:
    // e.g. supplying "Dim m as Mammal" to parameter "Out T" we require that T->Mammal be a VB conversion.
    // But the actual reason it's included now is as be a symmetric form of AnyConversion:
    // this simplifies the implementation of InvertConversionRequirement and CombineConversionRequirements
    AnyReverse,

    // AnyAndReverse:
    // This restriction requires that req->candidate and candidate->hint be conversions according to VB.
    // e.g. supplying "Dim m as New Mammal" to "ByRef T", we require that Mammal->T be a conversion, and also T->Mammal for the copyback.
    // Again, each direction includes user-defined conversions and all the VB-specific conversions.
    AnyAndReverse,

    // ArrayElement:
    // This restriction requires that req->candidate be a array element conversion.
    // e.g. supplying "new Mammal(){}" to "ByVal T()", we require that Mammal->T be an array-element-conversion.
    // It consists of the subset of CLR-array-element-conversions that are also allowed by VB.
    // Note: ArrayElementConversion gives us array covariance, and also by enum()->underlying_integral().
    ArrayElement,

    // Reference:
    // This restriction requires that req->candidate be a reference conversion.
    // e.g. supplying "Dim x as ICovariant(Of Mammal)" to "ICovariant(Of T)", we require that Mammal->T be a reference conversion.
    // It consists of the subset of CLR-reference-conversions that are also allowed by VB.
    Reference,

    // ReverseReference:
    // This restriction requires that candidate->req be a reference conversion.
    // e.g. supplying "Dim x as IContravariant(Of Animal)" to "IContravariant(Of T)", we require that T->Animal be a reference conversion.
    // Note that just because T->U is a widening reference conversion, it doesn't mean that U->T is narrowing, nor vice versa.
    // Again it consists of the subset of CLR-reference-conversions that are also allowed by VB.
    ReverseReference,

    // None:
    // This is not a restriction. It allows for the candidate to have any relation, even be completely unrelated,
    // to the hint type. It is used as a way of feeding in candidate suggestions into the algorithm, but leaving
    // them purely as suggestions, without any requirement to be satisfied. (e.g. you might add the restriction
    // that there be a conversion from some literal "1L", and add the type hint "Long", so that Long can be used
    // as a candidate but it's not required. This is used in computing the dominant type of an array literal.)
    None,

    // These restrictions form a partial order composed of three chains: from less strict to more strict, we have:
    //    [reverse chain] [None] < AnyReverse < ReverseReference < Identity
    //    [middle  chain] None < [Any,AnyReverse] < AnyConversionAndReverse < Identity
    //    [forward chain] [None] < Any < ArrayElement < Reference < Identity
    //
    //            =           KEY:
    //         /  |  \           =     Identity
    //        /   |   \         +r     Reference
    //      -r    |    +r       -r     ReverseReference
    //       |  +-any  |       +-any   AnyConversionAndReverse
    //       |   /|\   +arr     +arr   ArrayElement
    //       |  / | \  |        +any   Any
    //      -any  |  +any       -any   AnyReverse
    //         \  |  /           none  None
    //          \ | /
    //           none
    //
    // The routine "CombineConversionRequirements" finds the least upper bound of two elements.
    // The routine "StrengthenConversionRequirementToReference" walks up the current chain to a reference conversion.
    // The routine "InvertConversionRequirement" switches from reverse chain to forwards chain or vice versa,
    // and asserts if given ArrayElementConversion since this has no counterparts in the reverse chain.
    // These three routines are called by InferTypeArgumentsFromArgumentDirectly, as it matches an
    // argument type against a parameter.
    // The routine "CheckHintSatisfaction" is what actually implements the satisfaction-of-restriction check.
    // Note: for CheckHintSatifaction, there in't a direct connection between "how well the restriction is satisfied"
    // (e.g. Argument->Candidate might be a narrowing conversion, and if we were checking how well +vb
    // was satisfied with it then we'd say SatisfactionLevelNarrowing, but if we're checking how well +r was
    // satisfied with it then we'd say SatisfactionLevelError. See Dev10#595234.)
    //
    // If you make any changes to this enum or the partial order, you'll have to change all the above functions.
    // They do "VSASSERT(Count==8)" to help remind you to change them, should you make any additions to this enum.
    Count
END_ENUM(ConversionRequired)


typedef unsigned InferenceErrorReasons;
enum InferenceErrorReasonsEnum
{
    InferenceErrorReasonsOther                          = 0x00000000,
    InferenceErrorReasonsAmbiguous                      = 0x00000001,
    InferenceErrorReasonsNoBest                         = 0x00000002
};


enum DelegateRelaxationLevel
{
    DelegateRelaxationLevelNone                         = 0,  // Identity / Whidbey
    DelegateRelaxationLevelWidening                     = 1,
    DelegateRelaxationLevelWideningDropReturnOrArgs     = 2,
    DelegateRelaxationLevelWideningToNonLambda          = 3,
    DelegateRelaxationLevelNarrowing                    = 4,  // OrcasStrictOff
    // Keep invalid the biggest number
    DelegateRelaxationLevelInvalid                      = 254
};

enum TypeInferenceLevel
{
    TypeInferenceLevelNone                              = 0,
    // None is used to indicate uninitialized, but semantically it should not matter if there is a whidbey delegate
    // or no delegate in the overload resolution, hence both have value 0 such that comparisson in Semantics::MostSpecificProcedure
    // will not prefferer a non inferred method over an inferred one.
    TypeInferenceLevelWhidbey                           = 0,
    TypeInferenceLevelOrcas                             = 1,

    // Keep invalid the biggest number
    TypeInferenceLevelInvalid                           = 254
};

struct ProcedureDescriptor
{
    Procedure *Proc;
    GenericBindingInfo Binding;
    unsigned long FreeTypeArgumentCount();
    long DelegateRelaxationLevel;
    long TypeInferenceLevel;
};

enum RingIteratorType
{
    ritExtensionMethods,
    ritModules,
};

enum LambdaKind
{
    NotALambda,
    SingleLineLambda,
    MultiLineSubLambda,
    MultiLineFunctionLambda
};

// 
extern const ConversionClass ConversionClassTable[t_ref - t_bad + 1][t_ref - t_bad + 1];


class Attribute;
class Semantics;
class Closure;
class ClosureRoot;
class OptimizedClosure;
class ClosureGotoIterator;
class ParserHelper;

typedef HashTable<0x100> FromItemsHashTable;

#define NamedNodeType       1
#define TypedNodeType       2

typedef ArrayList<Type*, NorlsAllocWrapper>                     TypeList;
typedef ArrayListIterator<Type *, NorlsAllocWrapper>            TypeListIterator;

typedef ArrayList<ILTree::Expression*, NorlsAllocWrapper>         DominantExpressionList;
typedef ArrayListIterator<ILTree::Expression*, NorlsAllocWrapper> DominantExpressionListIterator;

struct DominantTypeData
{
    Type*                    ResultType;
    ConversionRequiredEnum   InferenceRestrictions;
    DominantExpressionList   SourceExpressions;  // DominantTypeOfExpressions uses this to track where the candidate came from

    bool IsStrictCandidate;        // did this candidate satisfy all hints solely through widening/identity?
    bool IsUnstrictCandidate;      // did this candidate satisfy all hints solely through narrowing/widening/identity?

    DominantTypeData(NorlsAllocWrapper* allocatorWrapper) :
        ResultType(NULL),
        InferenceRestrictions(ConversionRequired::Any),
        SourceExpressions(*allocatorWrapper),
        IsStrictCandidate(false),
        IsUnstrictCandidate(false)
    {
    };
};

struct DominantTypeDataTypeInference : DominantTypeData
{

    // Fields needed for error reporting
    bool                                ByAssumption; // was ResultType chosen by assumption or intention?
    Parameter*                          Parameter;
    bool                                InferredFromObject;
    GenericParameter*                   TypeParameter;
    const Location*                     ArgumentLocation;

    DominantTypeDataTypeInference(NorlsAllocWrapper* allocatorWrapper) :
        DominantTypeData(allocatorWrapper),
        ByAssumption(false),
        TypeParameter(NULL),
        Parameter(NULL),
        InferredFromObject(false)
    {
    };
};

typedef ArrayList<DominantTypeData*, NorlsAllocWrapper>         DominantTypeDataList;
typedef ArrayListIterator<DominantTypeData*, NorlsAllocWrapper> DominantTypeDataListIterator;

// This table can be used in two modes; the first mode is in resolving
// self-referencing fields. In this mode, temporaries are created for any
// field that is referenced later in the anonymous type.
// In the second mode, we bind and do replacement of properties with
// the temporaries.

struct AnonymousTypeBindingTable
{
    // The entry contains the real property, the dummy property, and the
    // temporary. The dummy property is used in the anonymous types binder to
    // resolve self-referencing fields.

    struct AnonymousTypeBindingTableEntry
    {
        BCSYM_Property* RealProperty;
        BCSYM_Property* DummyProperty;
        ILTree::Expression* Temp;
    };

    enum BindingMode
    {
        Unknown = -1,
        Resolving = 0,
        Replacing = 1
    };

    DynamicArray< AnonymousTypeBindingTableEntry > m_List;
    BindingMode m_Mode;

    // We will also store the base reference here (this changes while binding anonymous
    // types since we first do a bind on a temporary that we through away to do type
    // inference and validation, and then we bind for real).

    Variable* m_BaseReference;

    TemporaryManager* m_TemporaryManager;

    AnonymousTypeBindingTable(TemporaryManager* tempMgr ) :
        m_Mode( Unknown ),
        m_BaseReference( NULL ),
        m_TemporaryManager( tempMgr )
    {
    }

    // Get the temporary manager that is associated with this anonymous type. This is necessary because
    // if a field is initialized with a lambda, it's possible that the lambda will have it's own
    // temporary manager.

    TemporaryManager* GetTemporaryManager()
    {
        VSASSERT( m_TemporaryManager != NULL, "No temp manager set!" );
        return( m_TemporaryManager );
    }

    // We always know the number of anonymous type fields, so pre-calculate it.

    void SetInitialSize( _In_ unsigned long size )
    {
        if( size > 0 )
        {
            m_List.SizeArray( size );
            m_List.Grow( size );
        }
    }

    // Add a real property to the binding table. The real property is used as a key to look
    // up the temporary associated with a property.

    void AddRealProperty( unsigned long i, BCSYM_Property* Prop )
    {
        VSASSERT( i >= 0 && i < m_List.Count(), "Index out of range!" );
        VSASSERT( m_List.Element( i ).DummyProperty == NULL, "Dummy property added before real?" );
        m_List.Element( i ).RealProperty = Prop;
    }

    // Retrieve the real property.

    BCSYM_Property* GetRealProperty( unsigned long i )
    {
        VSASSERT( i >= 0 && i < m_List.Count(), "Index out of range!" );
        VSASSERT( m_List.Element( i ).RealProperty != NULL, "No real property?" );
        return( m_List.Element( i ).RealProperty );
    }

    // The dummy property is used to perform speculative binding. If we bind to a dummy
    // property, we will create a temp and associated with with the real/dummy property pair.

    void AddDummyProperty( unsigned long i, BCSYM_Property* Prop )
    {
        VSASSERT( i >= 0 && i < m_List.Count(), "Index out of range!" );
        VSASSERT( m_List.Element( i ).RealProperty != NULL, "Must add real property before dummy!" );
        m_List.Element( i ).DummyProperty = Prop;
    }

    bool DummyExists( BCSYM_Property* Dummy )
    {
        for( unsigned long i = 0; i < m_List.Count(); i += 1 )
        {
            if( m_List.Element( i ).DummyProperty == Dummy )
            {
                return( true );
            }
        }

        return( false );
    }

    // Check whether there is a temp associated with the dummy property.

    bool HasTemp( BCSYM_Property* Dummy )
    {
        VSASSERT( DummyExists( Dummy ), "Why does the dummy property not exist?" );

        for( unsigned long i = 0; i < m_List.Count(); i += 1 )
        {
            if( m_List.Element( i ).DummyProperty == Dummy )
            {
                if( m_List.Element( i ).Temp == NULL )
                {
                    return( false );
                }
                else
                {
                    return( true );
                }
            }
        }

        VSASSERT( false, "Shouldn't reach here." );
        return( false );
    }

    // Add a temporary and associated it with the corresponding dummy property.

    void AddTemp( BCSYM_Property* Dummy, ILTree::Expression* Temp )
    {
        VSASSERT( DummyExists( Dummy ), "Why does the dummy property not exist?" );

        for( unsigned long i = 0; i < m_List.Count(); i += 1 )
        {
            if( m_List.Element( i ).DummyProperty == Dummy )
            {
                VSASSERT( m_List.Element( i ).Temp == NULL, "Temp already exists!" );
                m_List.Element( i ).Temp = Temp;
                break;
            }
        }
    }

    // When we replace properties with temp, we are replacing a real property, not
    // a dummy property. Thus, to get the temp, we need to give it the real property.

    ILTree::Expression* GetTemp( BCSYM_Property* Real )
    {
        ULONG i = 0;
        for( i = 0; i < m_List.Count(); i += 1 )
        {
            if( m_List.Element( i ).RealProperty == Real )
            {
                break;
            }
        }

        VSASSERT( i >= 0 && i < m_List.Count(), "Dummy does not exist!" );
        return( GetTemp( i ) );
    }

    // Get the temp based on index.

    ILTree::Expression* GetTemp( ULONG i )
    {
        VSASSERT( i >= 0 && i < m_List.Count(), "Index out of range." );
        return( m_List.Element( i ).Temp );
    }
};

namespace Debugger
{
    class ExpressionAnalyzer;
    class ParseTreeBinder;
    class EvaluationEngine;
}

unsigned
CountArguments
(
    ExpressionList *Arguments
);


class Semantics
{

public:

    // forward Decl
    class TypeInference;

    friend class Attribute;
    friend class SourceFile;
    friend class PEBuilder;
    friend class Closure;
    friend class ClosureRoot;
    friend class OptimizedClosure;
    friend class ClosureGotoVisitor;
    friend class ResumableMethodLowerer;
    friend class MyBaseMyClassStubContext;
    friend class CIntelliSense;
    friend class LowerBoundTreeVisitor;
    friend class XmlSemanticsSymbols;
    friend class XmlSemantics;
    friend class XmlLocalSymbolMgr;
    friend class DeferredTempIterator;
    friend class MultilineLambdaReturnTypeVisitor;
    friend struct VBEEImplicitVariables;
    friend Debugger::ExpressionAnalyzer;
    friend Debugger::ParseTreeBinder;
    friend Debugger::EvaluationEngine;
    template<class TREE_TYPE> friend class ExpressionTreeSemantics;
    friend class ILTreeETGenerator;

#if HOSTED
    friend class VBHostedExpressionTreeSemantics;
#endif

#if IDE 
    friend class CProcLocalSymbolLookup;
#endif


    Semantics
    (   _In_ NorlsAllocator *TreeStorage,
        ErrorTable *Errors,
        Compiler *TheCompiler,
        CompilerHost *CompilerHost,
        SourceFile *ContainingFile,
        _In_ TransientSymbolStore *CreatedDuringInterpretation,
        bool IsGeneratingXML,
        bool fIncludeBadExpressions = false,
        bool createdByParseTreeService = false
    );

    bool
    ShouldLiftType
    (
        Type * pType,
        GenericTypeBinding * pGenericBindingContext,
        CompilerHost * pCompilerHost
    );

    bool
    ShouldLiftParameter
    (
        Parameter * pParam,
        GenericTypeBinding * pGenericBindingContext,        
        CompilerHost * pCompilerHost
    );

    bool
    ShouldLiftOperator
    (
        Operator * pOperator,
        GenericTypeBinding * pGenericBindingContext
    );


    bool
    DoesReceiverMatchInstance
    (
        Type * InstanceType,
        Type * ReceiverType
    ); // check if this receiver applies to the instance

    static bool
    AreSameNamespace
    (
        Namespace *Left,
        Namespace *Right
    );

    static Declaration *
    GetExtensionAttributeClass
    (
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        SourceFile * pSourceFile,
        _In_ NorlsAllocator * pTreeStorage,
        Type * pAccessingInstanceType
    );

    static Declaration *
    GetExtensionAttributeCtor
    (
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        SourceFile * pSourceFile,
        _In_opt_ NorlsAllocator * pTreeStorage,
        Type * pAccessingInstanceType
    );

    NorlsAllocator * GetTreeStorage()
    {
        return &m_TreeStorage;
    }

    Symbols * GetSymbols()
    {
        return &m_SymbolCreator;
    }

    Symbols * GetTransientSymbols()
    {
        return &m_TransientSymbolCreator;
    }

    Compiler * GetCompiler()
    {
        return m_Compiler;
    }

    // Perform semantic analysis of an expression, returning the resulting tree. The result
    // tree has the SF_ERROR flag set if semantic analysis fails.

    CompilerHost * GetCompilerHost()
    {
        return m_CompilerHost;
    }

    FX::FXSymbolProvider * GetFXSymbolProvider()
    {
        return m_FXSymbolProvider;
    }

    ILTree::Expression *
    InterpretExpression
    (
        ParseTree::Expression *Input,
        Scope *Lookup,
        ILTree::Expression *WithStatementContext,
        ExpressionFlags flags = ExprNoFlags,
        Type *TargetType = NULL
    );

    ILTree::Expression *
    InterpretExpressionAndLower
    (
        ParseTree::Expression *Input,
        Scope *Lookup,
        ILTree::Expression *WithStatementContext,
        ExpressionFlags flags = ExprNoFlags,
        Type *TargetType = NULL
    );

#if HOSTED
    ILTree::ExecutableBlock *
    InterpretStatementsForHostedCompiler
    (
        ParseTree::ExecutableBlockStatement *Input,
        Scope *Lookup
    );

    ILTree::Expression *
    InterpretExpressionForHostedCompiler
    (
        ParseTree::Expression *Input,
        Scope *Lookup,
        Type *TargetType
    );
#endif

#if IDE 
    void
    GetQueryGroupTypeAndElement
    (
        ParseTree::Expression *ExpressionWithGroup,
        Scope *Lookup,
        ILTree::Expression *WithStatementContext,
        Type **GroupType,
        Type **GroupElementType
    );

    void SetAllowMergeAnonymousTypes(bool MergeAnonymousTypes)
    {
       m_MergeAnonymousTypeTemplates = MergeAnonymousTypes;
    }

    void SetIDECompilationCaches(CompilationCaches *pCompilationCaches);
#endif

    // Perform semantic analysis of an expression that is required to be constant. If
    // interpetation succeeds, *ResultIsBad is false and *pExpressionResult contains the value
    // of the expression. If interpretations fails, *ResultIsBad is true.
    ConstantValue
    InterpretConstantExpression
    (
        ParseTree::Expression *Input,
        Scope *Lookup,
        Type *TargetType,
        bool ConditionalCompilationExpression,
        _Out_ bool *ResultIsBad,
        BCSYM_NamedRoot *ContextOfSymbolUsage = NULL,
        bool IsSyntheticExpression = false,
        _Out_opt_ ILTree::Expression **pExpressionResult = NULL
    );

    // Given an already-interpreted constant expression, convert it to another constant type
    ConstantValue
    ConvertConstantExpressionToConstantWithErrorChecking
    (
        ILTree::Expression *Input,
        Type *TargetType,
        _Out_ bool &ResultIsBad
    );


    // Perform semantic analysis of an initializer, returning the resulting tree. The result
    // tree has the SF_ERROR flag set if semantic analysis fails.

    ILTree::Expression *
    InterpretInitializer
    (
        _In_ ParseTree::Initializer *Input,
        Scope *Lookup,
        ILTree::Expression *WithStatementContext,
        Type *TargetType,
        bool MergeAnonymousTypes,
        ExpressionFlags flags = 0
    );

    bool
    OptionStrictOn();

    bool
    OptionInferOn();

    bool
    ShouldInfer(ILTree::Expression *Target);

    bool
    ErroneousTypeInference(ILTree::Expression * expression);

    void
    CoerceType
    (
        _Inout_ ILTree::Expression *Target,
        _In_ BCSYM *ResultType
    );

    ILTree::Expression *
    InferVariableType
    (
        ILTree::Expression *Target,
        ParseTree::Initializer *Init
    );

    BCSYM*
    InferVariableType
    (
        _Inout_ ILTree::Expression *Target,
        ILTree::Expression *InitValue
    );

    BCSYM *
    InferVariableType
    (
        _Inout_ ILTree::Expression * Target, 
        BCSYM * ResultType
    );

    Type*
    InferLambdaReturnTypeFromArguments
    (
        _In_opt_ Parameter* parameters,
        _In_ ILTree::UnboundLambdaExpression *unboundLambda
    );

    Type*
    InferMultilineLambdaReturnTypeFromReturnStatements
    (
        _In_ ILTree::UnboundLambdaExpression *unboundLambda
    );

    BCSYM*
    InferLambdaType
    (
        _Inout_ ILTree::UnboundLambdaExpression *LambdaType,
        _In_ Location &Location,
        _Inout_ bool *pRanDominantTypeAlgorithm=NULL
    );

    Type *
    InjectNullableIntoArray
    (
        Type * pType
    );

    // Perform semantic analysis of a method body.
    //
    // If any statement within the method body is syntactically or semantically
    // invalid, the returned procedure body is marked bad.
    ILTree::ProcedureBlock *
    InterpretMethodBody
    (
        Procedure *Method,
        _In_ ParseTree::MethodBodyStatement *MethodBody,
        Cycles *CycleDetection,
        _Inout_opt_ CallGraph *CallTracking,
        // If the client of the interpretation is the debugger evaluation
        // engine, the locals scope is supplied. For normal interpretation,
        // the locals scope is synthesized within the interpreter and LocalsAndParameters
        // should be NULL.
        Scope *LocalsAndParameters,
        // should only be passed in by the background compiler during the main compilation
        //
        AlternateErrorTableGetter AltErrTablesForConstructor,
        DynamicArray<BCSYM_Variable *> *ENCMembersToRefresh,
        bool MergeAnonymousTypes
    );

    Type *
    InterpretType
    (
        ParseTree::Type *TypeName,
        Scope *Lookup,
        _Out_ bool &TypeIsBad,
        TypeResolutionFlags Flags
    );

    // Method to find a symbol given a partially or fully qualified name by looking in the current scope
    // only
    static Symbol *
    InterpretQualifiedName
    (
        _In_count_(NumberOfIdentifiers) Identifier *Names[],
        unsigned int NumberOfIdentifiers,
        _In_opt_ NameTypeArguments *Arguments[],
        _In_opt_ NorlsAllocator *GenericBindingStorage,
        // Lookup is the scope in which to perform the name lookup.
        Scope *Lookup,
        NameFlags Flags,
        // SourceLocation is used for error reporting.
        const Location &SourceLocation,
        ErrorTable *Errors,
        Compiler *TheCompiler,
        CompilerHost *TheCompilerHost,
        CompilationCaches *IDECompilationCaches,
        SourceFile *ContainingFile,
        bool PerformObsoleteChecks,
        // NameIsBad is set true if the name interpretation fails and issues
        // appropriate diagnostics.
        _Out_ bool &NameIsBad,
        //
        // This can be used to suppress declaration caching by passing in true.
        // Note that "false" here does NOT mean EnableDeclarationCaching, but only
        // leaves it unchanged to the default which is different for IDE and vbc).
        //
        bool DisableDeclarationCaching = false,
        //
        // This can be used if the lookup scope and the context scope are different
        // as in the case of resolvenamedtypes for "Global" qualified names.
        // If NULL, then the context is assumed to be the same as the lookup.
        //
        Scope *ContextOfQualifiedName = NULL,
        //
        // This is used to pass the named context for the named types used in
        // applied attribute contexts.
        //
        Declaration *AppliedAttributeContext = NULL,
        Type * targetEmbeddedTypeIdentity = NULL // #546428
    );

    // Method to find a symbol given a qualified or unqualified name.
    static Declaration *
    InterpretName
    (
        _In_ ParseTree::Name *Name,
        // Lookup is the scope in which to perform the name lookup.
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        NameFlags Flags,
        Type *AccessingInstanceType,
        ErrorTable *Errors,
        Compiler *TheCompiler,
        CompilerHost *TheCompilerHost,
        CompilationCaches *IDECompilationCaches,
        SourceFile *File,
        // NameIsBad is set true if the name interpretation fails and issues
        // appropriate diagnostics.
        _Out_ bool &NameIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext,
        _In_opt_ NorlsAllocator *GenericBindingStorage,
        int GenericTypeArity

        #if IDE 
        , bool * pwasBoundThroughProjectLevelImport = NULL
        #endif
    );

    // Similar to above, but useful for things like RaiseEvent where there is no ParseTree:: Name object.
    static Symbol *
    Semantics::InterpretName
    (
        _In_z_ Identifier *Name,           // Name to interpret
        Location TextSpan,          // Location to use if an error is logged while looking up the name
        Scope *Lookup,              // Current hash scope
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        NameFlags Flags,            // Flags for name interpretation
        ErrorTable *Errors,         // Error table to put any errors in
        Compiler *TheCompiler,      // Current compiler
        CompilerHost *TheCompilerHost,      // Current CompilerHost
        SourceFile *File,           // Current File
        _Out_ bool &NameIsBad,            // [out] Did we find the name?
        _Out_opt_ GenericBinding **GenericBindingContext,
        _In_ NorlsAllocator *GenericBindingStorage,
        int GenericTypeArity
    );

    static ConstantValue
    Semantics::InterpretConstantExpression
    (
        ParseTree::Expression *Input,
        SourceFile *File,
        BCSYM_Hash *Lookup,
        BCSYM *TargetType,
        NorlsAllocator *TreesStorage,
        ErrorTable *Errors,
        bool ConditionalCompilationExpression,
        Compiler *TheCompiler,
        CompilerHost *TheCompilerHost,
        bool *ResultIsBad,
        bool IsSyntheticExpression = false,
        bool disableCaching = true,
        Declaration *ContextOfSymbolUsage = NULL
    );

    // Slightly different from the other overloads,
    // - useful for Bindable which needs to pass names and not ParseTrees
    // - More importantly note that this takes
    //      Scope - indicates the scope to look in
    //      AccessingInstanceType - the type through which the Name is being accessed
    //      CurrentContextScope   - the current context in which this access is occuring
    static Symbol *
    InterpretName
    (
        _In_z_ Identifier *Name,                   // Name to interpret
        Location TextSpan,                  // TextSpan to look in
        Scope *Lookup,                      // Current hash scope
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        NameFlags Flags,                    // Flags for name interpretation
        Type *AccessingInstanceType,        // The type through which the member is being accessed
        Scope *CurrentContextScope,         //  The current context in which this access is occuring
        ErrorTable *Errors,                 // Error table to put any errors in
        Compiler *TheCompiler,              // Current compiler
        CompilerHost *TheCompilerHost,      // Current compilerHost
        CompilationCaches *IDECompilationCaches,
        SourceFile *File,                   // Current File
        bool PerformObsoleteChecks,         // To perform obsolete check or not
        _Out_ bool &NameIsBad,                    // [out] Did we find the name?
        _Out_opt_ GenericBinding **GenericBindingContext,
        _In_ NorlsAllocator *GenericBindingStorage,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces = NULL
    );

    // Method to find a symbol given an Xml prefix.
    static Declaration *
    InterpretXmlPrefix
    (
        BCSYM_Hash *Lookup,         // Hash table to map prefix to alias
        _In_z_ Identifier *Prefix,         // Prefix to interpret
        NameFlags Flags,            // Flags for name interpretation
        SourceFile *File            // Current File
    );

    // Method to perform semantic analysis for an attribute and return the bound tree.
    ILTree::Expression *
    InterpretAttribute
    (
        ParseTree::Expression *ExpressionTree,
        ParseTree::ArgumentList *NamedArguments,
        Scope *Lookup,
        Declaration *NamedContextOfAppliedAttribute,
        ClassOrRecordType *AttributeClass,
        Location *Location
    );


    bool
    IsAccessible
    (
        Declaration * pDeclToCheck,
        Container * pContext
    );

    // Determine whether a declaration is accessible from a particular context.

    static bool
    IsAccessible
    (
        Declaration *Result,
        GenericBinding *ResultGenericBindingContext,
        Container *Context,
        Type *AccessingInstanceType,
        _In_ Symbols &SymbolCreator,
        CompilerHost *CompilerHost
    );

#if IDE 
    // It's a nuisance to have to manufacture Symbols objects in order to perform queries. From within the
    // compiler, it's necessary in order to make possible the caching of generic bindings. From the IDE,
    // it's not necessary.

    static bool
    IsAccessible
    (
        Declaration *Result,
        GenericBinding *ResultGenericBindingContext,
        Container *Context,
        Type *AccessingInstanceType,
        Compiler *Compiler,
        CompilerHost *CompilerHost
    );
#endif

    static bool
    CanAccessDefaultPropertyThroughType
    (
        Type *TypeToCheck,
        CompilerHost *TheCompilerHost
    );

    static Declaration *
    LookupDefaultProperty
    (
        Type *LookupClassInterfaceOrGenericParam,
        Compiler *TheCompiler,
        CompilerHost *TheCompilerHost,
        CompilationCaches *IDECompilationCaches,
        _Out_opt_ GenericBinding **GenericBindingContext,
        NorlsAllocator *GenericBindingStorage,
        bool MergeAnonymousTypes
    );

    static ConversionClass
    ClassifyPredefinedConversion
    (
        Type *TargetType,
        Type *SourceType,
        CompilerHost *CompilerHost
    );

    MethodConversionClass
    ClassifyDelegateInvokeConversion
    (
        Type* TargetDelegate,
        Type* SourceDelegate
    );

    static MethodConversionClass
    ClassifyMethodConversion
    (
        BCSYM_Proc *TargetMethod,
        GenericBindingInfo BindingForMethod,
        BCSYM_Proc *InvokeMethod,
        BCSYM_GenericBinding *BindingForInvoke,
        bool IgnoreReturnValueErrorsForInference,
        Symbols *SymbolFactory,
        CompilerHost *CompilerHost
    );

    MethodConversionClass
    ClassifyMethodConversion
    (
        BCSYM_Proc *TargetMethod,
        GenericBindingInfo BindingForMethod,
        BCSYM_Proc *InvokeMethod,
        BCSYM_GenericBinding *BindingForInvoke,
        bool IgnoreReturnValueErrorsForInference,
        Symbols *SymbolFactory,
        bool TargetIsExtensionMethod
    );

    MethodConversionClass
    ClassifyMethodConversion
    (
        Type *TargetRetType,
        BCSYM_Param *TargetParameters,
        bool TargetMethodIsDllDeclare,
        GenericBindingInfo BindingForTarget,
        BCSYM_Proc *InvokeMethod,
        BCSYM_GenericBinding *BindingForInvoke,
        bool IgnoreReturnValueErrorsForInference,
        Symbols *SymbolFactory
    );

    void
    Semantics::ClassifyReturnTypeForMethodConversion
    (
        Type *TargetReturnType,
        Type *DelegateReturnType,
        MethodConversionClass &MethodConversion
    );

    void
    Semantics::ClassifyArgumentForMethodConversion
    (
        Type *TargetType,
        Type *DelegateType,
        bool TargetIsParamArray,
        GenericBindingInfo BindingForTarget,
        Symbols *SymbolFactory,
        Type *&FixedTargetType,
        MethodConversionClass &MethodConversion
    );

    static bool
    IsStubRequiredForMethodConversion
    (
        MethodConversionClass Conversion
    );

    static bool
    IsSupportedMethodConversion
    (
        bool OptionStrictOnFile,
        MethodConversionClass Conversion,
        bool* WouldHaveSucceededWithOptionStrictOff = NULL,
        bool *pRequiresNarrowingConversion = NULL,
        bool ConversionIsForAddressOf = false
    );

    static bool
    IsNarrowingMethodConversion
    (
        MethodConversionClass Conversion,
        bool ConversionIsForAddressOf
    );

    static bool
    IsProcAllowedAsDelegate
    (
        BCSYM_Proc *TargetMethod
    );

    static DelegateRelaxationLevel
    Semantics::DetermineDelegateRelaxationLevel
    (
        MethodConversionClass Conversion
    );

    bool
    IsInterpretingForAttributeExpression
    (
    );

    void
    CheckFlow
    (
        ILTree::ProcedureBlock *,
        bool OnlyCheckReturns
    );

    void
    DefAsgEvalBlock
    (
        ILTree::PILNode ptreeBlock,
        unsigned int ClosureDepth
    );

    void
    DefAsgEvalStatementList
    (
       ILTree::PILNode ptreeStmt,
       unsigned int ClosureDepth
    );

    void IterativelyCheckRightSkewedTree(
        ILTree::PILNode ptree,
        unsigned int ClosureDepth);

    void
    DefAsgCheckUse
    (
        ILTree::PILNode ptree,
        unsigned int ClosureDepth
    );

    void
    UpdateLabelOrBlockAcrossTryCatch
    (
        ILTree::PILNode gotoNode,
        bool updateFinallyFixups,
        unsigned int ClosureDepth
    );

    ILTree::PILNode
    DefAsgUpdateParentGetContinuation
    (
        ILTree::PILNode currentReEvalStmt,
        unsigned int ClosureDepth
    );

    ILTree::TryBlock*
    DefAsgUpdateTryGroupOutBitSet
    (
        ILTree::PILNode ptree,
        DWORD_PTR* changed
    );

    void
    ReportDefAsgNoRetVal(
        Procedure *procedure,
        Type *pRetVarType,
        Location loc,
        _In_z_ STRING *pName
    );

    static bool
    IsDesignerControlClass
    (
        _In_ ClassOrRecordType *PossibleDesignerControl,
        bool SearchThisSymbolOnly = false
    );

    static bool
    IsInitializeComponent
    (
        Compiler *CompilerToUse,
        Declaration *PossibleInitializeComponent
    );

    static bool
    IsInSyntheticTryGroup
    (
        ILTree::PILNode ptree
    );

    static bool
    ContainsStatement(
        ILTree::ExecutableBlock*  blockNode,
        ILTree::PILNode                    targetNode
    );

    static ILTree::PILNode
    GetFinallyBlockInTryGroup
    (
        ILTree::PILNode ptree
    );

    static ILTree::PILNode
    GetTryBlockInTryGroup
    (
        ILTree::PILNode ptree
    );

    static bool
    IsLastNonOverlapedCatchInTheGroup
    (
        ILTree::PILNode tree
    );

    bool
    IsFieldAccessInLocalOfStructType
    (
        ILTree::PILNode ptreeStmtCur
    );

    void
    DefAsgSetOrCheckForUse
    (
        ILTree::PILNode ptree,
        unsigned int ClosureDepth
    )
    {
        DefAsgSetOrCheckForUse(ptree, m_DefAsgCurrentBitset, false/*onlySet*/, ClosureDepth);
    };

    void
    DefAsgSet
    (
        ILTree::PILNode ptree,
        unsigned int ClosureDepth
    )
    {
        DefAsgSetOrCheckForUse(ptree, m_DefAsgCurrentBitset, true/*onlySet*/, ClosureDepth);
    };

    void
    DefAsgSet
    (
        ILTree::PILNode ptree,
        BITSET  *&bitSet,
        unsigned int ClosureDepth
    )
    {
        DefAsgSetOrCheckForUse(ptree, bitSet, true/*onlySet*/, ClosureDepth);
    };

    void
    DefAsgSetOrCheckForUse
    (
        ILTree::PILNode ptree,
        BITSET  *&bitSet,
        bool onlySet,
        unsigned int ClosureDepth
    );

    void
    DefAsgProcessSlots
    (
        ILTree::ExecutableBlock* block,
        bool IncludeLocals,
        bool IncludeReturns
    );

    void
    CheckUnusedLocals
    (
        ILTree::ExecutableBlock* block
    );

    unsigned
    DefAsgTypeSize
    (
        BCSYM* type
    );

    unsigned
    DefAsgTypeSize
    (
        ILTree::PILNode tree
    );

    unsigned
    DefAsgGetSlotOfFieldStructExpr
    (
        ILTree::PILNode ptree
    );

    bool
    DefAsgIsValidMember
    (
        BCSYM_NamedRoot *pnamed
    );

    void
    SetLocalIsUsed
    (
        BCSYM_Variable * pvar
    );

    Type*
    TypeInGenericContext
    (
        Type *T,
        GenericBinding *GenericContext
    );

    // Perform lookup in a particular scope, visiting nowhere except the
    // supplied scope object.

    static Declaration *
    ImmediateLookup
    (
        Scope *Lookup,
        _In_z_ Identifier *Name,
        unsigned BindspaceMask,
        CompilerProject *pCurrentProject = NULL
    );

    static Declaration *
    ImmediateLookupForMergedHash
    (
        Scope *Lookup,
        _In_z_ Identifier *Name,
        unsigned BindspaceMask,
        CompilerProject *pCurrentProject = NULL
    );

        // Compare two parameters of two procedures to see if one is more applicable than the other.

    void
    CompareParameterApplicability
    (
        _In_opt_ ILTree::Expression *Argument,
        Parameter *LeftParameter,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        bool ExpandLeftParamArray,
        Parameter *RightParameter,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        bool ExpandRightParamArray,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins,
        _Inout_ bool &BothLose,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    void
    CompareParameterTypeApplicability
    (
        _In_opt_ ILTree::Expression *Argument,
        Type *LeftType,
        Procedure *LeftProcedure,
        Type *RightType,
        Procedure *RightProcedure,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins,
        _Inout_ bool &BothLose
    );

    void
    CompareParameterGenericDepth
    (
        _In_opt_ ILTree::Expression *Argument,
        Parameter *LeftParameter,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        bool ExpandLeftParamArray,
        Parameter *RightParameter,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        bool ExpandRightParamArray,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins,
        _Inout_ bool &BothLose,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    void
    CompareParameterTypeGenericDepth
    (
        Type *LeftType,
        Type *RightType,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins
    );


    // Determine whether or not Result is accessible from the current context.
    bool
    IsAccessible
    (
        Symbol *Result,
        GenericBinding *ResultGenericBindingContext,
        Type *AccessingInstanceType
    );

    Declaration *
    GetNextOverloadForProcedureConsideringBaseClasses
    (
        Declaration * OverloadedProcedure,
        Type * AccessingInstanceType
    );

    bool
    Semantics::HasAccessibleSharedOverload
    (
        BCSYM_NamedRoot * pDecl,
        GenericBinding * pBinding,
        Type * pAccessingInstanceType
    );

    // Given an overloaded procedure, find the next set of overloaded procedures
    // (in a different scope) that contribute to the overloading. For example,
    // if the procedure is a class method, search the class's base classes
    // for procedures of the same name.

    Declaration *
    FindMoreOverloadedProcedures
    (
        Declaration *OverloadedProcedure,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Inout_ bool &SomeCandidatesBad
    );

    bool
    IsOrInheritsFromOrImplements
    (
        Type *Derived,
        Type *Base
    );

    bool ApplySuperTypeFilter
    (
        Type * ReceiverType,
        Procedure * Procedure,
        PartialGenericBinding * pPartialGenericBinding
    );

    static Declaration * EnsureNamedRoot(Symbol * pSymbol)
    {
        ThrowIfFalse(!pSymbol || pSymbol->IsNamedRoot());
        return (BCSYM_NamedRoot *)pSymbol;
    }

    static ParseTree::IdentifierDescriptor *
    ExtractName
    (
        ParseTree::Expression *Input,
        bool &IsNameBangQualified
    );

    static ParseTree::IdentifierDescriptor *
    ExtractAnonTypeMemberName
    (
        ParseTree::Expression *Input,
        bool &IsNameBangQualified,
        bool &IsXMLNameRejectedAsBadVBIdentifier,
        ParseTree::XmlNameExpression *&XMLNameInferredFrom
    );

    static STRING*
    GenerateUniqueName
    (
        Compiler *compiler,
        const WCHAR *rootName,
        unsigned counter
    );

    bool
    CheckXmlFeaturesAllowed
    (
        const Location & TextSpan,
        ExpressionFlags Flags
    );

    bool ExtractXmlNamespace
    (
        ParseTree::Expression * Expr,
        ParseTree::IdentifierDescriptor & Prefix,
        ILTree::StringConstantExpression * & NamespaceExpr
    );

    // Given a SX_IIFCoalesce node, build the elements for the equivalent IIF. IIF(Condition, TrueExpr, FalseExpr)
    void LoadIIfElementsFromIIfCoalesce
    (
        ILTree::Expression *expr,       // in: SX_IIFCoalesce node IF(X,Y) .
        ILTree::Expression *&Condition, // out: Condition - based on first argument. Can be be temp(=X), 'temp IsNot Nothing', 'temp.hasValue'
        _Out_ ILTree::Expression *&TrueExpr,  // out: TrueExpr - can be Conv(temp), Conv(temp->GetValueOrDefault)
        _Out_ ILTree::Expression *&FalseExpr, // out: FalseExpr - is Conv(Y)
        bool fPreserveConditon  // in:
                                // false - the first operand in expr is evaluated once possibly via a temp. The temp is used in Condition and TrueExpr.
                                //         Used in lowering for preparing the tree for code gen.
                                // true  - the first operand in expr is returned as the Condition. Also, it is shared in TrueExpr. Necesary conversions
                                //          are built on top of X and Y. Used for Expression Trees.
    );

    ILTree::Expression* LowerCType(
        ILTree::Expression* expr
        );

    ILTree::Expression* CreateNullableHasValueCall
    (
        Type *OperandType,
        ILTree::Expression *Operand
    );

    ILTree::Expression* CreateNullableValueCall
    (
        Type *OperandType,
        ILTree::Expression *Operand
    );

    ILTree::Expression* CreateNullableExplicitOperatorCall
    (
        Type *OperandType,
        ILTree::Expression *Operand
    );

    ILTree::Expression* CreateNullValue
    (
        Type *ResultType,
        Location &Location
    );

    ILTree::Expression* WrapInNullable
    (
        Type *ResultType,
        ILTree::Expression *Value
    );

    ILTree::Expression* WrapInNullable
    (
        bool Value,
        Location &Location
    );

    void CaptureNullableOperand
    (
        _In_ ILTree::Expression *Operand,
        _Out_ ILTree::Expression *&Capture,
        _Out_ ILTree::Expression *&VariableReference,
        _Deref_out_opt_ Variable **Variable = NULL
    );

    void ProcessNullableOperand
    (
        _In_ ILTree::Expression *Operand,
        _Inout_ ILTree::Expression *&Condition,
        _Out_ ILTree::Expression *&Value,
        bool returnOperandAsValue = false
    );

    // This is post interpretation step in InterpretMethodBody. This method walks the
    // bound tree and checks to see if any anonymous delegates are used and if so
    // registers them with the transient symbol store. This registering is done so
    // late during interpretation and not as soon as the anonymous delegate is created
    // in order to ensure that only the anonymous delegates that are used in the final
    // bound tree are emitted into the assembly.
    // For an exmaple, see bug Devdiv 78381.
    //
    void RegisterRequiredAnonymousDelegates(ILTree::ProcedureBlock *OuterMostProcedureBoundTree);

#if IDE 
    ILTree::Expression *
    InterpretFromItemWithContext
    (
        ParseTree::FromItem * FromClause,
        ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        ILTree::Expression *WithStatementContext = NULL
    );
#endif

protected:

    ILTree::ColInitExpression *
    AllocateColInitExpression
    (
        ILTree::Expression * pNewExpression, 
        ILTree::ExpressionWithChildren * pElements, 
        BCSYM_Variable * pTmpVar,
        const Location & loc
    );

    ILTree::ColInitElementExpression *
    AllocateColInitElement
    (
        ILTree::Expression * pCallExpression, 
        ILTree::Expression * pCopyOutArguments, 
        ExpressionFlags flags,
        const Location & callLocation
    );    

    ILTree::Expression *
    InterpretCollectionInitializerElement
    (
        ParserHelper & ph,
        BCSYM_Variable * pCollectionTemporary,
        ParseTree::ArgumentList * pArgs,
        ExpressionFlags flags
    );  


    ILTree::Expression *
    InterpretCollectionInitializer
    (
        ParseTree::CollectionInitializerExpression * pExpr, 
        ExpressionFlags flags
    );

    ParseTree::ArgumentList *
    TranslateCollectionInitializerElement
    (
        ParserHelper & ph, 
        ParseTree::Expression * pValue
    );
    
    ILTree::ExpressionWithChildren *
    CreateDimList    
    (
        unsigned rank,
        unsigned * pDims,
        const Location & loc
    );


    void
    Semantics::CheckLambdaParameterShadowing
    (
        ILTree::UnboundLambdaExpression * pLambda
    );

    ILTree::Expression * 
    InterpretArrayLiteral
    (
        ParseTree::ArrayInitializerExpression * pInput, 
        ExpressionFlags flags
    );

    bool ValidateShape
    (
        ParseTree::ArrayInitializerExpression * pInput, 
        ArrayList<unsigned> & lengthList
    );

    bool ValidateShape
    (
        ParseTree::ArrayInitializerExpression * pInput,
        ArrayList<unsigned> & lengthList,
        unsigned dimIndex,
        bool first
    );

    bool ValidateElementCount
    (
        unsigned expectedCount,
        unsigned count,
        const Location & location
    );

    ParseTree::Expression * 
    GetInitializerValue
    (
        ParseTree::Initializer * pInitializer
    );    

    unsigned GetElementCount
    (
        ParseTree::ArrayInitializerExpression * pExpr
    );
        
    bool
    ShouldRebindExtensionCall
    (
        ILTree::Expression * pQualifiedExpression,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretArrayInitializerElement
    (
        ParseTree::Initializer * pInitializer,
        //NOTE: this location is only valid for reporting "internal compiler error" messages only.
        const Location & loc,
        ExpressionFlags flags
    );
    
    bool
    CanUseExtensionMethodCache();

    ExtensionCallLookupResult *
    Semantics::ExpandCacheEntryAndApplyFilters
    (
        ExtensionMethodLookupCacheEntry * pEntry,
        Container * pContextOfCall
    );


    void DoUnfilteredExtensionMethodLookupInImportedTargets
    (
        _In_z_ Identifier * pName ,
        ImportedTarget * pTargets,
        ImportedTarget * pExtraTargets,
        CompilerProject * pProject,
        _Inout_ ExtensionMethodLookupCacheEntry * & pCurrentItem,
        _Inout_ ExtensionMethodLookupCacheEntry * & pLastItem,
        _Inout_ ExtensionMethodLookupCacheEntry * & pRet,
        _Inout_ unsigned long & precedenceLevel,
        _Inout_ unsigned long & lastPrecedenceLevel
    );

    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInSourceFileImports
    (
        _In_z_ Identifier * pName ,
        SourceFile * pSourceFile
    );


    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInClassImport
    (
        _In_z_ Identifier * pName,
        BCSYM_Class * pClass
    );

    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInNamespaceImport
    (
        _In_z_ Identifier * pName,
        BCSYM_NamespaceRing * pRing,
        CompilerProject * pReferencingProject
    );

    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInNamespace
    (
        _In_z_ Identifier * pName,
        BCSYM_NamespaceRing * pRing,
        CompilerProject * pReferencingProject
    );

    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInClass
    (
        _In_z_ Identifier * pName,
        BCSYM_Class * pClass
    );

    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookupInContainer
    (
        _In_z_ Identifier * pName,
        Container * pContainer
    );


    ExtensionMethodLookupCacheEntry *
    DoUnfilteredExtensionMethodLookup
    (
        _In_z_ Identifier * pName,
        Container * pContainer
    );


    GenericBinding *
    GetGenericTypeBindingContextForContainingClass();


    // Interpret all the statements in a statement list.

    // returns FALSE if aborting
    bool
    TryInterpretStatementSequence
    (
        _Inout_opt_ ParseTree::StatementList *FirstStatement
    );

    // Interpret the statements that are children of the block, then
    // terminate the block structure in the the bound trees that
    // corresponds to the block.

    void
    InterpretAndPopBlock
    (
        _In_ ParseTree::ExecutableBlockStatement *Block
    );

    // returns FALSE if aborting
    bool
    TryInterpretBlock
    (
        _In_ ParseTree::ExecutableBlockStatement *Block
    );

    void
    InterpretStatement
    (
        _In_ ParseTree::Statement *Input,
        _Inout_ ILTree::TryBlock *&TryPendingCompletionInCurrentBlock
    );

    void
    MarkContainingLambdaOrMethodBodyBad
    (
    );
    
    void
    MarkLambdaBodyBad
    (
    );

    ILTree::Expression *
    InitializeAnonymousType
    (
        ParseTree::BracedInitializerList *BracedInitializerList,
        bool NoWithScope,
        bool QueryErrorMode,
        BCSYM_Namespace *ContainingNamespace,
        BCSYM_Class *ContainingClass,
        Location &CreatingLocation,
        Location &TextSpan,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InitializeAnonymousType
    (
        GenericBinding *AnonymousTypeBinding,
        MemberInfoList *MemberInfos,
        AnonymousTypeBindingTable* BindingTable,
        ULONG FieldCount,
        bool NoWithScope,
        Location &CreatingLocation,
        Location &TextSpan,
        ExpressionFlags Flags
    );

    MemberInfoList *
    InitializeMemberInfoList
    (
        NorlsAllocator &rnra,
        ParseTree::BracedInitializerList *BracedInitializerList,
        bool QueryErrorMode,
        ULONG *MemberInfoCount
    );

    void
    BuildBindingTableFromClass
    (
        MemberInfoList *MemberInfos,
        AnonymousTypeBindingTable* BindingTable,
        ULONG FieldCount,
        BCSYM_Class* Class
    );

    GenericBinding *
    GetAnonymousTypeBinding
    (
        Declaration *AnonymousTypeTemplate,
        Location &TextSpan,
        MemberInfoList *MemberInfos,
        AnonymousTypeBindingTable* BindingTable,
        ULONG FieldCount,
        bool NoWithScope,
        bool QueryErrorMode
    );

    ClassOrRecordType *
    GetAnonymousTypeTemplate
    (
        BCSYM_Namespace *ContainingNamespace,
        BCSYM_Class *ContainingClass,
        MemberInfoList *MemberInfos,
        AnonymousTypeBindingTable* BindingTable,
        unsigned FieldCount,
        bool QueryErrorMode
    );

    ClassOrRecordType *
    GenerateAnonymousType
    (
        BCSYM_Namespace *ContainingNamespace,
        BCSYM_Class *ContainingClass,
        MemberInfoList *MemberInfos,
        AnonymousTypeBindingTable* BindingTable,
        DWORD AnonymousTypeHash,
        unsigned FieldCount,
        bool QueryErrorMode
    );

    void
    BuildAnonymousTypeConstructorTemplate
    (
        BCSYM **GenericArguments,
        MemberInfoList *MemberInfos,
        ULONG FieldCount,
        SymbolList *OwningSymbolList // [in] symbol list to add to
    );

    ClassOrRecordType *
    GenerateUnregisteredAnonymousType
    (
        _In_z_ STRING *ClassName,
        _In_z_ STRING *ContainingNamespaceName,
        DWORD AnonymousTypeHash,
        MemberInfoList *MemberInfos,
        unsigned FieldCount,
        bool HasAtleastOneKeyField
    );

    void
    GenerateAnonymousTypeImplements
    (
        _In_z_ STRING *ClassName,
        BCSYM **GenericArguments,
        ULONG FieldCount,
        ClassOrRecordType *AnonymousClass,
        BCSYM_Implements **ImplementsSymbolList
    );

    void
    GenerateAnonymousTypeChildren
    (
        _In_z_ STRING *ClassName,
        BCSYM **GenericArguments,
        DWORD AnonymousTypeHash,
        ULONG FieldCount,
        bool HasAtleastOneKeyField,
        ClassOrRecordType *AnonymousClass,
        SymbolList *ClassChildren
    );

    void
    GenerateAnonymousTypeAttributes
    (
        ClassOrRecordType *AnonymousClass,
        MemberInfoList *MemberInfos
    );

    STRING*
    GenerateAnonymousTypeDebuggerDisplayProperty
    (
        ClassOrRecordType *AnonymousClass,
        MemberInfoList *MemberInfos
    );

    void
    Semantics::GenerateDebuggerDisplayAttribute
    (
        ClassOrRecordType *ClassToAdd,
        _In_z_ STRING *DebuggerDisplayString
    );

    BCSYM_Property*
    GenerateAnonymousTypeField
    (
        Type *Target,
        _In_z_ STRING *ValueName,
        Location &TextSpan,
        Location &NameTextSpan,
        bool IsExplicitlyNamed,
        bool IsBangNamed,
        bool QueryErrorMode,
        bool FieldIsKey,
        BCSYM *FieldType
    );

    STRING *
    AnonymousTypeCreateFieldNameForDeclaration(_In_z_ STRING *PropertyName);

    void
    AnonymousTypeAppendFieldNameIntoCode(_In_z_ STRING *PropertyName, StringBuffer & Code);

    STRING *
    AnonymousTypeCreateFieldNameForCode(_In_z_ STRING *PropertyName);

    STRING *
    AnonymousTypeCreateFieldNameFromCode(_In_z_ STRING *NameInCode);

    STRING *
    MapAnonymousTypeConstructorNameForBody(_In_z_ STRING *PropertyName);

    STRING *
    UnMapAnonymousTypeConstructorNameForBody(_In_z_ STRING *PropertyName);

    void
    UpdateAnonymousTypeChildren
    (
        BCSYM_Hash *Members,
        MemberInfoList *MemberInfos,
        ULONG FieldCount
    );

    ILTree::Expression *
    SetupAnonymousTypeWithEnclosure
    (
        Variable *Temporary,
        ILTree::Expression *Value,
        bool NoWithScope
    );

    ILTree::ProcedureBlock *
    CreateEqualsOverridesBody
    (
        Procedure *EqualsOverrides,
        Procedure *EqualsOverloads,
        ClassOrRecordType *AnonymousClass,
        ULONG FieldCount
    );

    void
    GenerateDummyProperties
    (
        ClassOrRecordType *AnonymousType,
        MemberInfoList *MemberInfos,
        ULONG FieldCount
    );

    BCSYM_NamedType *
    CreateNamedType
    (
        _In_ Symbols &SymbolCreator,
        Declaration *Context,
        ULONG NameCount,
        ...
    );

    BCSYM_NamedType *
    CreateNamedType
    (
        _In_ Symbols &SymbolCreator,
        Declaration *Context,
        _Inout_opt_ BCSYM_NamedType **TypeList,
        bool IsGlobalNameSpaceBased,
        bool IsUsedInAppliedAttributeContext,
        bool IsTypeArgumentForGenericType,
        ULONG NameCount,
        ...
    );

    BCSYM_NamedType *
    CreateNamedType
    (
        _In_ Symbols &SymbolCreator,
        Declaration *Context,
        _Inout_opt_ BCSYM_NamedType **TypeList,
        bool IsGlobalNameSpaceBased,
        bool IsUsedInAppliedAttributeContext,
        bool IsTypeArgumentForGenericType,
        ULONG NameCount,
        va_list ArgumentList
    );


    NameTypeArguments *
    CreateNameTypeArguments
    (
        Symbols &SymbolCreator,
        bool TypesForTypeArgumentsSpecified,
        ULONG ArgumentCount,
        ...
    );

    NameTypeArguments *
    CreateNameTypeArguments
    (
        Symbols &SymbolCreator,
        BCSYM **Arguments,
        ULONG ArgumentCount
    );

    ClassOrRecordType *
    ConstructAnonymousDelegateClass
    (
        BCSYM_Param * Parameters,
        Type * LambdaResultType,
        bool IsFunctionLambda,
        Location CreatingLocation
    );

    ClassOrRecordType *
    RetreiveAnonymousDelegate
    (
        BCSYM_Class *ContainingClass,
        BCSYM_Param * Parameters,
        Type * LambdaResultType,
        bool IsFunctionLambda,
        Location CreatingLocation
    );

    Type*
    GetInstantiatedAnonymousDelegate
    (
        _In_ ILTree::LambdaExpression *Lambda
    );

    Type*
    GetInstantiatedAnonymousDelegate
    (
        BCSYM_Param * LambdaParameters,
        Type * LambdaResultType,
        bool IsFunctionLambda,
        Location CreatingLocation
    );

    DWORD
    GetAnonymousDelegateHashCode
    (
        BCSYM_Param * Parameters,
        bool IsFunctionLambda
    );

    void
    GenerateAnonymousDelegateAttributes
    (
        ClassOrRecordType *delegate
    );

    BCSYM_Proc*
    PopulateDelegateClassForLambda
    (
        ClassOrRecordType *DelegateClass,
        _Inout_opt_ SymbolList *DelegateChildren,
        Location &TextSpan,
        BCSYM_Param *Parameters,
        BCSYM_GenericParam *GenericParams,
        BCSYM *ReturnType
    );

    BCSYM_Param*
    BuildGenericParametersForLambda
    (
        BCSYM_Param*        Parameter,
        BCSYM_GenericParam* GenericParameter,
        BCSYM_Param**       LastParameterSymbol,
        Symbols*            Symbols
    );

    void
    RegisterTransientSymbol(Declaration *Symbol);

    void
    RegisterAnonymousDelegateTransientSymbolIfNotAlreadyRegistered
    (
        _In_ BCSYM_Class *pSymbol
    );

    bool
    IsLocalParamOrMe
    (
        Declaration *Symbol
    );

    bool
    IsLocalOrParamFromOutsideProcedure
    (
        Declaration *Symbol,
        Procedure *InnerProc
    );

    void
    TransformDeferredTemps
    (
    );

    ILTree::ProcedureBlock*
    TransformMethod
    (
        ILTree::ProcedureBlock *OuterMostProcedureBoundTree
    );

    virtual void
    InitClosures(Procedure *, ILTree::ProcedureBlock *);

    void
    CleanupClosures();

    // Lowering semantic step for the procedure 'BoundBody' and any transient method produced during 'BoundBody' interpretation up to this point.
    // Assumes the (TransientSymbolStore::) StartMethod() and EndMethod() are used correctly to set the TransientSymbolStore::InMethodSymbolIterator.
    void
    LowerBoundTreesForMethod(
        ILTree::ProcedureBlock *BoundBody,
        TransientSymbolStore *CreatedDuringInterpretation
    );

    void
    LowerBoundTree(
        _In_opt_ ILTree::ProcedureBlock *BoundBody
    );

    ILTree::ProcedureBlock*
    LowerResumableMethod(
        _In_opt_ ILTree::ProcedureBlock *BoundBody
    );



    ParseTree::IdentifierDescriptor
    ItIdentifierDescriptor
    (
    );

    ParseTree::IdentifierDescriptor
    Join1IdentifierDescriptor
    (
    );

    ParseTree::IdentifierDescriptor
    Join2IdentifierDescriptor
    (
    );

    ParseTree::IdentifierDescriptor
    ItIdentifierDescriptor
    (
        _In_z_ STRING * id
    );

    bool
    CheckControlVariableDeclaration
    (
        _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration
    );

    bool
    CheckControlVariableIdentifier
    (
        _In_ ParseTree::IdentifierDescriptor & Name
    );

    bool
    CheckControlVariableName
    (
        _In_opt_z_ Identifier * name,
        Location  & location,
        bool CheckForObjectMemberNameCollision = true
    );

    bool
    CheckNameForShadowingOfLocals
    (
        _In_ Identifier* Name,
        Location&   Location,
        ERRID       ErrorId,
        bool        DeferForImplicitDeclarations
    );

    Type *
    InferControlVariableType
    (
        Declaration * selectMethodDecl,
        _In_opt_ GenericBindingInfo *TargetBinding
    );

    Type *
    InferControlVariableType
    (
        _In_ OverloadList *Candidates,
        unsigned CandidateCount,
        bool & failedDueToAnAmbiguity
    );

    Type *
    InferControlVariableTypeFromMembers
    (
        ILTree::Expression *BaseReference,
        ExpressionList *BoundArguments,
        Location SourceLoc,
        bool & failedDueToAnAmbiguity
    );

    Type *
    InferControlVariableType
    (
        ILTree::Expression * Source
    );

    ILTree::Expression *
    ToQueryableSource
    (
        ILTree::Expression * Source,
        _Out_opt_ Type ** ControlVariableType,
        _Inout_opt_ QueryExpressionFlags * pQueryFlags = NULL
    );

    ILTree::Expression *
    InterpretQueryOperatorCall
    (
        _In_ ParseTree::CallOrIndexExpression * operatorCall,
        ExpressionFlags Flags
    );


    ILTree::Expression *
    InterpretFromItem
    (
        _In_ ParseTree::FromItem * FromClause,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL
    );

    Type *
    GetControlVariableType
    (
        _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration,
        _In_ Location & TextSpan,
        bool & TypeIsBad
    );

    ILTree::Expression *
    InterpretQueryControlVariableDeclaration
    (
        _In_ ParseTree::VariableDeclaration *ControlVariableDeclaration,
        _In_ ParseTree::Expression * Source,
        bool IsLetControlVar,
        _In_ Location & TextSpan,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags
    );

    class LambdaBodyInterpreter
    {
    public:
        ILTree::ILNode * InterpretBody(Symbols * pSymbols)
        {
            ILTree::ILNode* pRet = DoInterpretBody();
            if (pRet && pRet->IsExprNode())
            {
                ILTree::Expression * pExpr = &pRet->AsExpression();
                pExpr->ResultType = pExpr->ResultType->DigThroughArrayLiteralType(pSymbols);
            }
   
            return pRet;
        }

        virtual ILTree::ResumableKind GetResumableKind()=0;
        virtual Type* GetResumableGenericType()=0;

    protected:        
        virtual ILTree::ILNode * DoInterpretBody()=0;
    };

    ILTree::LambdaExpression *
    CreateLambdaExpression
    (
        _In_opt_ Parameter * FirstParameter,
        Location & LambdaTextSpan,
        Location BodySpan,
        Type * LambdaResultType,
        LambdaBodyInterpreter * bodyInterpreter,
        bool IsFunctionLambda,
        bool IsAsyncKeywordUsed,
        bool IsIteratorKeywordUsed,
        LambdaKind lambdaKind,
        _In_opt_ Type *pTargetTypeOfMultilineLambdaBody=NULL,
        bool inferringLambda = false
    );

    class LambdaBodyInterpretJoinedFromItem : public LambdaBodyInterpreter
    {
    private:
        Semantics * m_Semantics;
        ParseTree::FromItem *m_FromItem;
        ParseTree::NewObjectInitializerExpression * m_ItInitializer;
        ParseTree::AlreadyBoundType * m_ControlVariableType;
        ParseTree::IdentifierDescriptor m_AddedName;

    public:
        LambdaBodyInterpretJoinedFromItem
        (
            _In_ Semantics * semantics,
            _In_ ParseTree::FromItem *FromItem,
            ParseTree::NewObjectInitializerExpression * ItInitializer
        );

        ILTree::ResumableKind GetResumableKind() {return ILTree::NotResumable;}
        Type* GetResumableGenericType() {return NULL;}

        ParseTree::AlreadyBoundType * GetControlVariableType();
        ParseTree::IdentifierDescriptor GetAddedName();
        QueryExpressionFlags GetQueryFlags()
        {
            return QueryNoFlags;
        }
    protected:
        ILTree::ILNode * DoInterpretBody();
    };

    class LambdaBodyInterpretJoinedLetItem : public LambdaBodyInterpreter
    {
    private:
        Semantics * m_Semantics;
        ParseTree::FromItem *m_FromItem;
        ParseTree::NewObjectInitializerExpression * m_ItInitializer;
        ParseTree::IdentifierDescriptor m_AddedName;
        ParseTree::InitializerList * m_AddedInitializer;

    public:
        LambdaBodyInterpretJoinedLetItem
        (
            _In_ Semantics * semantics,
            _In_ ParseTree::FromItem *FromItem,
            _In_ ParseTree::NewObjectInitializerExpression * ItInitializer
        );

        ILTree::ResumableKind GetResumableKind() {return ILTree::NotResumable;}
        Type* GetResumableGenericType() {return NULL;}
        
        ParseTree::InitializerList * GetAddedInitializer();
        ParseTree::IdentifierDescriptor GetAddedName();
    protected:
        ILTree::ILNode * DoInterpretBody();
    };

    ILTree::Expression *
    BuildJoinedFromClauses
    (
        _In_opt_ ParseTree::FromList *FromItems,
        ParseTree::LinqExpression * CrossJoinTo,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _Out_opt_ ParseTree::IdentifierDescriptor *pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        Location TextSpan
    );

    ILTree::Expression *
    InterpretJoinedFromItem
    (
        _In_ ParseTree::FromItem *FromItem,
        ParseTree::NewObjectInitializerExpression * ItInitializer,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType
    );

    ParseTree::FromList *
    Semantics::GetNextItemInFromList
    (
        _In_opt_ ParseTree::FromList *FromItems,
        _Inout_ ParseTree::LinqExpression ** CrossJoinTo,
        _Out_ Location & CommaOrFromOrErrorTextSpan,
        _Out_ bool & AnError
    );

    ILTree::Expression *
    InterpretCrossJoin
    (
        ILTree::Expression * Source,
        ParseTree::IdentifierDescriptor ControlVariableName,
        ParseTree::AlreadyBoundType * ControlVariableType,
        QueryExpressionFlags QueryFlags,
        Location CommaOrFromTextSpan,
        _In_opt_ ParseTree::FromList *nextFromItems,
        ParseTree::LinqExpression * CrossJoinTo,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _Out_opt_ ParseTree::IdentifierDescriptor *pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        Location TextSpan
    );

    ILTree::Expression *
    InterpretCrossJoin
    (
        ILTree::Expression * Source,
        _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
        _Inout_ QueryExpressionFlags &QueryFlags,
        Location CommaOrFromTextSpan,
        _In_ ParseTree::FromItem *FromItem,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _In_opt_ ParseTree::FromItem *FollowingLet,
        bool lastItemToJoin,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        Location joinTextSpan
    );

    ILTree::Expression *
    Semantics::InterpretJoinSelector
    (
        _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
        _Inout_ QueryExpressionFlags &QueryFlags,
        _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName2,
        ParseTree::AlreadyBoundType * ControlVariableType2,
        QueryExpressionFlags QueryFlags2,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _In_opt_ ParseTree::FromItem *FollowingLet,
        bool lastItemToJoin,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        _Inout_ Location &joinTextSpan // can be modified !!!
    );

    ILTree::Expression *
    InterpretLetJoin
    (
        ILTree::Expression * Source,
        _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
        _Inout_ QueryExpressionFlags &QueryFlags,
        Location CommaOrFromTextSpan,
        _In_ ParseTree::FromItem *FromItem,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        bool lastItemToJoin,
        _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        Location joinTextSpan
    );

    bool
    ShouldFlattenLetJoinSelector
    (
        bool lastItemToJoin,
        _In_ ParseTree::LinqOperatorExpression **pFollowingOperator
    );

    ILTree::Expression *
    InterpretLetJoinSelector
    (
        _Inout_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Inout_ ParseTree::AlreadyBoundType * &ControlVariableType,
        _Inout_ QueryExpressionFlags &QueryFlags,
        _In_ ParseTree::FromItem *FromItem,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        bool lastItemToJoin,
        _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        Location joinTextSpan,
        _Inout_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2 = NULL,
        ParseTree::AlreadyBoundType * pControlVariableType2 = NULL,
        QueryExpressionFlags QueryFlags2 = QueryNoFlags
    );

    ILTree::Expression *
    InterpretCrossJoinExpression
    (
        _In_ ParseTree::CrossJoinExpression *CrossJoin,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL,
        _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator = NULL
    );

    ILTree::Expression *
    InterpretInnerJoinExpression
    (
        _In_ ParseTree::InnerJoinExpression *InnerJoin,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator = NULL
    );

    ILTree::Expression *
    InterpretInnerJoinExpression
    (
        _In_ ParseTree::InnerJoinExpression *InnerJoin,
        ExpressionFlags Flags,
        _In_opt_ ParseTree::FromItem *FollowingLet,
        bool lastItemToJoin,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Deref_out_ ParseTree::AlreadyBoundType *& pControlVariableType,
        _Out_ QueryExpressionFlags & QueryFlags,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator
    );

    void
    InterpretJoinKeys
    (
        _In_ ParseTree::IdentifierDescriptor & ControlVariableName1,
        ParseTree::AlreadyBoundType *ControlVariableType1,
        QueryExpressionFlags QueryFlags1,
        _In_ ParseTree::InitializerList * InitializerList1,
        _In_ ParseTree::IdentifierDescriptor & ControlVariableName2,
        ParseTree::AlreadyBoundType *ControlVariableType2,
        QueryExpressionFlags QueryFlags2,
        _In_ ParseTree::InitializerList * InitializerList2,
        _In_opt_ ParseTree::Expression *Predicate,
        ExpressionFlags Flags,
        _Out_ ILTree::LambdaExpression * &boundKey1,
        _Out_ ILTree::LambdaExpression * &boundKey2
    );

    ILTree::Expression *
    InterpretJoinOperatorSource
    (
        _In_ ParseTree::LinqExpression *Source,
        ExpressionFlags Flags,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Deref_out_ ParseTree::AlreadyBoundType *& pControlVariableType,
        _Out_ QueryExpressionFlags & QueryFlags,
        _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator
    );


    void
    GetNameInfoFromInitializer
    (
        _In_ ParseTree::InitializerList * nextRightInitializer,
        _Out_ ParseTree::IdentifierDescriptor * &RightIdentifierDescriptor,
        _Out_ Location * &rightNameLocation
    );

    bool
    JoinCheckDuplicateControlVars
    (
        _In_opt_ ParseTree::InitializerList * firstLeftInitializer,
        ParseTree::InitializerList * stopInitializer,
        _In_opt_ ParseTree::IdentifierDescriptor * RightIdentifierDescriptor,
        Location * rightNameLocation,
        _In_opt_ ParseTree::InitializerList * nextRightInitializer
    );


    ILTree::Expression *
    InterpretGroupJoinExpression
    (
        _In_ ParseTree::GroupJoinExpression *GroupJoin,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags,
        _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
        _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType = NULL
    );

    ILTree::Expression *
    InterpretGroupJoinExpression
    (
        _In_ ParseTree::GroupJoinExpression *GroupJoin,
        ExpressionFlags Flags,
        ParseTree::FromItem *FollowingLet,
        bool lastItemToJoin,
        _Inout_ ParseTree::NewObjectInitializerExpression * &ItInitializer,
        _Out_ ParseTree::IdentifierDescriptor &ControlVariableName,
        _Deref_out_ ParseTree::AlreadyBoundType *& pControlVariableType,
        _Out_ QueryExpressionFlags & QueryFlags,
        _In_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType = NULL
    );

    ILTree::Expression *
    BindGroupJoinOperator
    (
        ILTree::Expression * Source1,
        ILTree::Expression * Source2,
        _In_ Location & JoinTextSpan,
        _In_ Location & JoinNameLocation,
        ILTree::LambdaExpression * boundKey1,
        ILTree::LambdaExpression * boundKey2,
        _In_ ParseTree::IdentifierDescriptor &ControlVariableName1,
        ParseTree::AlreadyBoundType *ControlVariableType1,
        QueryExpressionFlags QueryFlags1,
        _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
        _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
    );

    ILTree::Expression *
    Semantics::BindGroupJoinOperator
    (
        ILTree::Expression * Source1,
        ILTree::Expression * Source2,
        _In_ Location & JoinTextSpan,
        _In_ Location & JoinNameLocation,
        ILTree::LambdaExpression * boundKey1,
        ILTree::LambdaExpression * boundKey2,
        ILTree::Expression *ProjectionLambda
    );

    class LambdaBodyBuildKeyExpressions : public LambdaBodyInterpreter
    {
    private:
        Semantics * m_Semantics;
        ParseTree::Expression * m_Predicate;
        ExpressionFlags m_Flags;

        Identifier *m_ControlVarName1;
        Identifier *m_ControlVarName2;
        ParseTree::InitializerList * m_InitializerList1;
        ParseTree::InitializerList * m_InitializerList2;
        StringBuffer m_ErrorArg1;
        StringBuffer m_ErrorArg2;

        Declaration * m_Var1;
        Declaration * m_Var2;

        Identifier *m_NameBoundToControlVar;

        enum BoundState
        {
            NotBound,
            BoundToLeft,
            BoundToRight,
        };

        BoundState m_BoundTo;
        bool m_ThereWasANameReference;
        bool m_ThereWasABadNameReference;

        unsigned m_KeySegmentCount;

        LambdaBodyBuildKeyExpressions * m_Prev;

        ErrorTable * m_Errors;

    public:
        ParseTree::Expression * m_Key1;
        ParseTree::Expression * m_Key2;
        bool m_KeysAreInvalid;

        LambdaBodyBuildKeyExpressions
        (
            _In_ Semantics * semantics,
            _In_ ParseTree::Expression * Predicate,
            ExpressionFlags Flags,
            _In_z_ Identifier *ControlVarName1,
            _In_z_ Identifier *ControlVarName2,
            _In_ ParseTree::InitializerList * InitializerList1,
            _In_ ParseTree::InitializerList * InitializerList2
        );

        ILTree::ResumableKind GetResumableKind() {return ILTree::NotResumable;}
        Type* GetResumableGenericType() {return NULL;}

        void
        BuildKey
        (
            _In_ ParseTree::Expression * Predicate
        );

        void
        CheckName
        (
            Declaration * var,
            _In_z_ Identifier * Name,
            const Location & nameTextSpan
        );

        WCHAR* GetErrorArg1();
        WCHAR* GetErrorArg2();
    protected:
        ILTree::ILNode * DoInterpretBody();
    };


    ILTree::Expression *
    InterpretFromExpression
    (
        _In_ ParseTree::FromExpression *From,
        ParseTree::LinqExpression * CrossJoinTo,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL,
        _Inout_opt_ ParseTree::LinqOperatorExpression **pFollowingOperator = NULL
    );

    ILTree::Expression *
    InterpretFilterExpression
    (
        _In_ ParseTree::FilterExpression *Filter,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );

    ILTree::Expression *
    Semantics::InterpretSkipTakeExpression
    (
        _In_ ParseTree::SkipTakeExpression *SkipTake,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );

    ILTree::Expression *
    InterpretGroupByClause
    (
        _In_ ParseTree::GroupByExpression *Group,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pGroupElementType = NULL
    );

    class LambdaBodyInterpretAggregateProjection : public LambdaBodyInterpreter
    {
    private:
        Semantics * m_Semantics;
        ParseTree::Expression * m_Group;
        ParseTree::IdentifierDescriptor m_ElementControlVariableName;
        ParseTree::AlreadyBoundType *m_ElementControlVariableType;
        QueryExpressionFlags m_ElementQueryFlags;
        ParseTree::NewObjectInitializerExpression * m_ItInitializer;
        ParseTree::InitializerList * m_Projection;
        ExpressionFlags m_Flags;

        bool m_ProjectionIsGood;
        ParseTree::InitializerList * m_FirstAddedInitializer;

    public:
        LambdaBodyInterpretAggregateProjection
        (
            _In_ Semantics * semantics,
            _In_ ParseTree::Expression * Group,
            _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
            ParseTree::AlreadyBoundType *ElementControlVariableType,
            QueryExpressionFlags ElementQueryFlags,
            _In_opt_ ParseTree::NewObjectInitializerExpression * ItInitializer,
            ParseTree::InitializerList * Projection,
            ExpressionFlags Flags
        );

        ILTree::ResumableKind GetResumableKind() {return ILTree::NotResumable;}
        Type* GetResumableGenericType() {return NULL;}

        bool ProjectionIsGood()
        {
            return m_ProjectionIsGood;
        }

        ParseTree::InitializerList * FirstAddedInitializer()
        {
            return m_FirstAddedInitializer;
        }
    protected:
        ILTree::ILNode * DoInterpretBody();
    };

    ILTree::Expression *
    InterpretAggregateExpression
    (
        _In_ ParseTree::AggregateExpression *Aggregate,
        ExpressionFlags Flags,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableTypeContainingGroup = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pGroupElementType = NULL
    );

    ILTree::Expression *
    InterpretAggregateExpression
    (
        _In_ ParseTree::AggregateExpression *Aggregate,
        ExpressionFlags Flags,
        _Out_opt_ Type **pGroupType = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType **pGroupElementType = NULL
    );

    ILTree::Expression *
    InterpretGroupForAggregateExpression
    (
        _Inout_ ParseTree::QueryAggregateGroupExpression * AggGroup,
        ExpressionFlags Flags
    );

    bool
    CheckIntoProjectionForShadowing
    (
        ParseTree::InitializerList * FirstAddedInitializer
    );

    void
    ConvertIntoProjectionToExpressions
    (
        ParseTree::InitializerList * FirstAddedInitializer
    );

    bool
    AppendIntoProjection
    (
        _In_opt_ ParseTree::NewObjectInitializerExpression *ProjectionInitializer,
        ParseTree::InitializerList * Projection,
        _In_ ParseTree::Expression * GroupReference,
        _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
        ParseTree::AlreadyBoundType *ElementControlVariableType,
        QueryExpressionFlags ElementQueryFlags,
        _Out_opt_ ParseTree::InitializerList ** pFirstAddedInitializer
    );

    bool
    GetItemFromIntoProjection
    (
        ParseTree::InitializerList * Initializers,
        _Out_ ParseTree::Expression *& expr,
        _Out_ ParseTree::AggregationExpression * & agg,
        _Out_ ParseTree::IdentifierDescriptor * &Name,
        _Out_ Location & initializerTextSpan
    );

    ParseTree::Expression *
    Semantics::GetGroupReferenceFromIntoProjection
    (
        ParseTree::InitializerList * Initializer
    );

    Type *
    GetTypeOfTheGroup
    (
        ILTree::Expression * GroupByCall,
        _In_z_ STRING * memberName,
        Location & memberNameTextSpan
    );

    Type *
    GetTypeOfTheGroup
    (
        Type * ProjectionReturnType,
        _In_z_ STRING * memberName,
        Location & memberNameTextSpan
    );

    Type *
    GetControlVariableTypeAfterGroupBy
    (
        ILTree::Expression * GroupByCall
    );

    bool
    IsOKToNotFlattenBeforeGroupBy
    (
        _In_ ParseTree::GroupByExpression *Group
    );

    bool
    Semantics::IsOKToNotFlattenBeforeInto
    (
        ParseTree::InitializerList *Projection,
        bool IsGroupBy
    );

    ParseTree::Expression *
    BuildAggregateFunctionCall
    (
        _In_opt_ ParseTree::Expression * GroupReference,
        _In_ ParseTree::IdentifierDescriptor &ElementControlVariableName,
        ParseTree::AlreadyBoundType *ElementControlVariableType,
        QueryExpressionFlags ElementQueryFlags,
        _In_ ParseTree::AggregationExpression * agg
    );

    ILTree::UnboundLambdaExpression *
    BuildUnboundLambdaForIntoProjection
    (
        _In_ ParseTree::IdentifierDescriptor &keyControlVariableName,
        ParseTree::AlreadyBoundType *keyControlVariableType,
        QueryExpressionFlags keyQueryFlags,
        _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
        _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
    );

    ILTree::Expression *
    BindGroupByOperator
    (
        ILTree::Expression * Source,
        _In_ Location & GroupByTextSpan,
        _In_ Location & GroupByNameLocation,
        ILTree::LambdaExpression * boundElementSelector,
        ILTree::LambdaExpression * boundKeySelector,
        _In_ ParseTree::IdentifierDescriptor &keyControlVariableName,
        ParseTree::AlreadyBoundType *keyControlVariableType,
        QueryExpressionFlags keyQueryFlags,
        _In_ ParseTree::IdentifierDescriptor &groupControlVariableName,
        _In_opt_ ParseTree::NewObjectInitializerExpression *Projection
    );

    ILTree::Expression *
    BindGroupByOperator
    (
        ILTree::Expression * Source,
        _In_ Location & GroupByTextSpan,
        _In_ Location & GroupByNameLocation,
        ILTree::LambdaExpression * boundElementSelector,
        ILTree::LambdaExpression * boundKeySelector,
        ILTree::Expression *ProjectionLambda
    );


    ILTree::Expression *
    InterpretOrderByExpression
    (
        _In_ ParseTree::OrderByExpression *Order,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );

    ILTree::Expression *
    InterpretLinqQuery
    (
        _In_ ParseTree::Expression *Source,
        ExpressionFlags Flags
    );


    ILTree::Expression *
    InterpretLinqOperatorSource
    (
        _In_ ParseTree::Expression *Source,
        ExpressionFlags Flags,
        _Inout_ ParseTree::LinqOperatorExpression **pFollowingOperator, //when *pFollowingOperator is changed to NULL, the following operator should be skiped because it was replaced with its equivalent
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );

    ILTree::Expression *
    BindSelectOperator
    (
        ILTree::Expression * Source,
        _In_ Location & selectTextSpan,
        _In_ Location & selectNameLocation,
        ILTree::LambdaExpression * boundSelector
    );

    ILTree::LambdaExpression *
    BindSelectSelector
    (
        _In_ ParseTree::Expression * SelectExpression,
        _In_ ParseTree::IdentifierDescriptor &ControlVariableName,
        ParseTree::AlreadyBoundType *ControlVariableType,
        QueryExpressionFlags QueryFlags,
        Type * TargetType = NULL
    );

    ILTree::LambdaExpression *
    BindSelectSelector
    (
        _In_ ParseTree::Expression * SelectExpression,
        ParseTree::ParameterList * Parameters,
        Type * TargetType /*= NULL*/
    );

    ILTree::LambdaExpression *
    BindSelectSelector
    (
        _In_ ParseTree::Expression * SelectExpression,
        _In_ ParseTree::IdentifierDescriptor &ControlVariableName1,
        ParseTree::AlreadyBoundType *ControlVariableType1,
        QueryExpressionFlags QueryFlags1,
        _In_ ParseTree::IdentifierDescriptor &ControlVariableName2,
        ParseTree::AlreadyBoundType *ControlVariableType2,
        QueryExpressionFlags QueryFlags2,
        Type * TargetType = NULL
    );

    ILTree::Expression *
    InterpretDistinctExpression
    (
        ParseTree::DistinctExpression *Distinct,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );


    ILTree::Expression *
    InterpretSelectExpression
    (
        _In_ ParseTree::SelectExpression *Select,
        ExpressionFlags Flags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName = NULL,
        _Out_opt_ ParseTree::AlreadyBoundType ** pControlVariableType = NULL,
        _Out_opt_ QueryExpressionFlags * pQueryFlags = NULL,
        _Out_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer = NULL
    );

    ILTree::Expression *
    InterpretSelectSelector
    (
        _In_ ParseTree::SelectExpression *Select,
        ParseTree::IdentifierDescriptor ControlVariableName,
        ParseTree::AlreadyBoundType *ControlVariableType,
        QueryExpressionFlags QueryFlags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags,
        _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
        _In_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2 = NULL,
        ParseTree::AlreadyBoundType *ControlVariableType2 = NULL,
        QueryExpressionFlags QueryFlags2 = QueryNoFlags
    );

    ILTree::Expression *
    InterpretInitializerSelector
    (
        _In_ ParseTree::InitializerList * Projection,
        bool ForceNameInferenceForSingleElement,
        ParseTree::IdentifierDescriptor ControlVariableName,
        ParseTree::AlreadyBoundType *ControlVariableType,
        QueryExpressionFlags QueryFlags,
        _Out_opt_ ParseTree::IdentifierDescriptor * pControlVariableName,
        _Out_opt_ ParseTree::AlreadyBoundType **  pControlVariableType,
        _Out_opt_ QueryExpressionFlags * pQueryFlags,
        _Out_opt_ ParseTree::NewObjectInitializerExpression ** pRecordInitializer,
        _In_opt_ ParseTree::IdentifierDescriptor *pControlVariableName2 = NULL,
        ParseTree::AlreadyBoundType *ControlVariableType2 = NULL,
        QueryExpressionFlags QueryFlags2 = QueryNoFlags
    );


    struct QueryMemberLookupVariable : public CSingleLink<QueryMemberLookupVariable>
    {
        Variable * IterationVariable;
    };

    ILTree::Expression *
    CreateXNamespaceExpression
    (
        _In_ STRING *NamespaceString, 
        const Location &loc
    );

    ILTree::Expression *
    GetDeferredTempForXNamespace
    (
        _In_ STRING *NamespaceString
    );

    ILTree::Expression *
    GetXNamespaceReference
    (
        _In_ STRING *NamespaceString,
        const Location &TextSpan
    );

    ILTree::Expression *
    CreateXNameExpression
    (
        _In_ STRING *LocalName, 
        _In_ STRING *Namespace, 
        const Location &loc
    );

    ILTree::Expression *
    GetDeferredTempForXName
    (
        _In_ STRING *LocalName,
        _In_ STRING *Namespace
    );

    ILTree::Expression *
    GetXNameReference
    (
        _In_ STRING *LocalName, 
        _In_ STRING *Namespace, 
        const Location &loc
    );

    ULONG
    AddInScopeXmlNamespaces
    (
        ParseTree::ExpressionList * Attributes
    );

    ULONG
    AddInScopeXmlNamespaces
    (
        ParseTree::XmlElementExpression * Expr
    );

    void AddInScopePrefixAndNamespace
    (
        BCSYM_Alias *prefixAlias,
        ArrayList<STRING *, NorlsAllocWrapper> *inScopePrefixes, 
        ArrayList<STRING*, NorlsAllocWrapper> *inScopeNamespaces
    );

    void GetInScopePrefixesAndNamespaces
    (
        ILTree::Expression **prefixesExpr, 
        ILTree::Expression **namespacesExpr
    );

    ILTree::Expression*
    AllocateRootNamspaceAttributes
    (
    );

    ILTree::Expression *
    InterpretXmlExpression
    (
        ParseTree::Expression * Expr,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretXmlExpression
    (
        XmlSemantics *XmlAnalyzer,
        ParseTree::Expression * Expr
    );

    ILTree::Expression*
    InterpretXmlExpression
    (
    ParseTree::Expression * Expr,
    ParseTree::Expression::Opcodes Opcode
    );

    ILTree::Expression *
    InterpretXmlDocument
    (
        ParseTree::XmlDocumentExpression * Xml
    );

    ParseTree::Expression *
    InterpretXmlDeclaration
    (
        ParseTree::XmlDocumentExpression * Xml
    );

    ILTree::Expression *
    InterpretXmlContent
    (
        ParseTree::XmlExpression *XmlElement,
        ILTree::SymbolReferenceExpression *VariableReference,
        ILTree::Expression **LastContent
    );

    ILTree::Expression *
    InterpretXmlComment
    (
        ParseTree::XmlExpression *Comment
    );

    ILTree::Expression *
    InterpretXmlCData
    (
        ParseTree::XmlExpression *CData
    );

    ILTree::Expression *
    InterpretXmlPI
    (
        ParseTree::XmlPIExpression *PI
    );

    ILTree::Expression *
    InterpretXmlOther
    (
        ParseTree::XmlExpression *Expr,
        Type *XObjectType,
        ParseTree::ArgumentList *ArgList
    );

    ILTree::Expression *
    InterpretXmlCharData
    (
        ParseTree::XmlCharDataExpression *XmlCharData,
        ParseTree::Expression::Opcodes Opcode
    );

    ILTree::Expression *
    ConcatenateAdjacentText
    (
        ParseTree::ExpressionList *&Content,
        ParseTree::Expression::Opcodes Opcode
    );

    ILTree::Expression *
    InterpretXmlName
    (
        ParseTree::Expression * Expr,
        BCSYM_Alias **ResolvedPrefix
     );

    void ValidateEntityReference(ParseTree::XmlReferenceExpression *Reference);

    void ValidateXmlAttributeValue(ParseTree::Expression *Value, _In_z_ STRING *Text);

    void ValidateXmlAttributeValue2(ParseTree::Expression *Value, _In_z_ STRING *Text1, _In_z_ STRING *Text2);

    ILTree::Expression *
    InterpretXmlAttribute(
        ParseTree::XmlAttributeExpression * Attribute
    );

    ILTree::Expression *
    InterpretXmlAttributes
    (
        ParseTree::ExpressionList * Attributes,
        ILTree::SymbolReferenceExpression *VariableReference,
        ILTree::Expression **LastExpression
    );

    ILTree::Expression *
    InterpretXmlElement
    (
        ParseTree::XmlElementExpression * XmlElement
    );

    ClassOrRecordType *
    EnsureXmlHelperClass
    (
    );

    Procedure *
    GetXmlHelperMethod
    (
        _In_z_ STRING *MethodName
    );

    ILTree::Statement *
    InterpretAssignment
    (
        _In_ ParseTree::AssignmentStatement *Assignment,
        const bool link = true
    );

    ILTree::Statement *
    InterpretAssignment
    (
        ParseTree::Expression *UnboundTarget,
        ParseTree::Expression *UnboundSource,
        _In_ const Location &TextSpan,
        const unsigned short ResumeIndex,
        const bool link = true
    );

    ILTree::Statement *
    InterpretAssignment
    (
        ILTree::Expression *Target,
        ParseTree::Expression *UnboundSource,
        _In_ const Location &TextSpan,
        const unsigned short ResumeIndex,
        const bool link = true
    );

    void
    InterpretOperatorAssignment
    (
        _In_ ParseTree::AssignmentStatement *UnboundAssignment
    );

    // Interprets an assignment to a property reference. The property reference
    // can be indexed or non indexed.

    ILTree::Expression *
    InterpretPropertyAssignment
    (
        const Location &AssignmentLocation,
        ILTree::Expression *Target,
        ILTree::Expression *BoundSource,
        bool IsAggrInitAssignment = false   // indicates that this is an assignment in an aggregate initializer
    );


    ILTree::Expression *
    InterpretPropertyAssignmentSource
    (
        ParseTree::Expression *pUnboundSource,
        Type *pPropertyType
    );

    ILTree::Expression *
    InterpretPropertyAssignment
    (
        const Location &AssignmentLocation,
        ILTree::Expression *pTarget,
        ParseTree::Expression *pUnboundSource,
        bool IsAggrInitAssignment = false   // indicates that this is an assignment in an aggregate initializer
    );


    // Interpret a property assignment and generate a statement to
    // perform the assignment.

    ILTree::Statement *
    AssignToProperty
    (
        _In_ const Location &AssignmentLocation,
        const unsigned short ResumeIndex,
        ILTree::Expression *Target,
        ILTree::Expression *BoundSource,
        const bool link = true
    );

    ILTree::Statement *
    AssignToProperty
    (
        _In_ const Location &AssignmentLocation,
        const unsigned short ResumeIndex,
        ILTree::Expression *pTarget,
        ParseTree::Expression *pUnboundSource,
        const bool link = true
    );


    ILTree::Statement *
    AssignToPropertyInternal
    (
        _In_ const Location &AssignmentLocation,
        const unsigned short ResumeIndex,
        ILTree::Expression *Target,
        ILTree::Expression *SetCall,
        const bool link = true
    );


    ILTree::Expression *
    FetchFromProperty
    (
        ILTree::Expression *Source
    );

    // Generate an assignment to an LValue.

    ILTree::Expression *
    Semantics::GenerateNonPropertyAssignment
    (
        const Location &AssignmentLocation,
        ILTree::Expression *Target,
        ILTree::Expression *Source
    );

    // Generate an assignment to any kind of assignable reference, whether
    // a property reference or an LValue.

    ILTree::Expression *
    GenerateAssignment
    (
        const Location &AssignmentLocation,
        ILTree::Expression *Target,
        ILTree::Expression *Source,
        bool IsByRefCopyOut,
        bool IsAggrInitAssignment = false   // indicates that this is an assignment in an aggregate initializer
    );

    // Take care of setting flags and other cleanup work in processing a
    // normal (non-property) assignment.

    ILTree::Statement *
    FinishNonPropertyAssignment
    (
        _In_ const Location &AssignmentLocation,
        const unsigned short ResumeIndex,
        ILTree::Expression *Target,
        ILTree::Expression *Source,
        const bool link = true
    );

    void
    ReportAssignmentToRValue
    (
        ILTree::Expression *RValue
    );

    void
    InterpretCall
    (
        _In_ ParseTree::CallStatement *UnboundCall
    );

    class ExpressionListHelper
    {
    public:
        ExpressionListHelper(Semantics * pSemantics, ILTree::Expression * pList = NULL);
        void Init(ExpressionList * pList);
        void Add(ILTree::Expression * pExpr, const Location & location);
        void Splice(ExpressionList * pList);
        ILTree::ExpressionWithChildren * Start();
        unsigned long Count();
    private:
        void AdvanceToEnd();
        Semantics * m_pSemantics;
        ExpressionList ** m_ppListEnd;
        ExpressionList * m_pListStart;
        unsigned long m_count;
    };

    ILTree::Expression *
    InterpretInitializer
    (
        _In_ ParseTree::Initializer *Init,
        _Inout_ ILTree::Expression *Target
    );

    ILTree::Expression *
    InterpretInitializer
    (
        ParseTree::Initializer *Init,
        Type *TargetType
    );

    ILTree::Expression *
    InterpretInitializer
    (
        ParseTree::Initializer *Init,
        Type *TargetType,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InitializeConstructedVariable
    (
        _Inout_ ILTree::Expression *VariableReference,
        _In_opt_ ParseTree::ArgumentList *Arguments,
        const Location *TypeLoc           // Can be NULL
    );

    ILTree::Expression *
    InitializeConstructedVariable
    (
        _Inout_ ILTree::Expression *VariableReference,
        _In_opt_ ParseTree::ArgumentList *Arguments,
        const Location *TypeLoc,           // Can be NULL
        _In_opt_ ParseTree::ObjectInitializerList *ObjectInitializer, // Can be NULL - Initializers for members of the constructed object.
        _In_opt_ ParseTree::BracedInitializerList * CollectionInitializer, //can be null - the collectio
        const ParseTree::PunctuatorLocation * FromLocation,
        const Location * Loc
    );

    bool
    ArgumentsAllowedAsDelegateConstructorArguments
    (
        _In_ ParseTree::ArgumentList *UnboundArguments
    );

    // Generate code to initialize shared/non-shared fields within
    // a shared/instance constructor.
    void
    InitializeFields
    (
    );

    // Generate code to hook up an event handler for a method declared with
    // "Handles MyBase.<EventName>".

    void
    InterpretMyBaseHandler
    (
        Procedure *Method,
        HandlesList *Handles
    );

#if IDE 
    // Generate code in the the ENC refresh function that will access
    // all the new members emitted during ENC so that the clr will
    // create real entities corresponding to these members.
    //
    void
    RefreshNewENCMembers
    (
        DynamicArray<BCSYM_Variable *> *ENCMembersToRefresh
    );

    void
    RefreshNewENCMember
    (
        BCSYM_Variable *MemberVar,
        BCSYM_Variable *&ObjectTemporary
    );

    void
    GenerateENCTrackingList
    (
        ClassOrRecordType *Class
    );
#endif IDE

    void
    InterpretGotoStatement
    (
        _In_ ParseTree::LabelReferenceStatement *Goto
    );

    void
    InterpretOnErrorStatement
    (
        _In_ ParseTree::OnErrorStatement *OnError
    );

    void
    InterpretResumeStatement
    (
        _Inout_ ParseTree::ResumeStatement *OnError
    );

    void
    InterpretLabelStatement
    (
        _Inout_ ParseTree::LabelReferenceStatement *Statement
    );

    LabelDeclaration *
    GetLabel
    (
        _In_ ParseTree::LabelReferenceStatement *LabelReference,
        bool IsDefinition,
        _Out_ bool *ExitsTry,
        _Out_ bool *ExitsCatch,
        _Out_ bool *ExitsCatchToCorrespondingTry
    );

    LabelDeclaration *
    ReferToLabel
    (
        _In_ ParseTree::LabelReferenceStatement *LabelReference,
        _Out_ bool *ExitsTry,
        _Out_ bool *ExitsCatch,
        _Out_ bool *ExitsCatchToCorrespondingTry
    )
    {
        *ExitsTry = false;
        *ExitsCatch = false;
        *ExitsCatchToCorrespondingTry = false;
        return GetLabel(LabelReference, false, ExitsTry, ExitsCatch, ExitsCatchToCorrespondingTry);
    }

    LabelDeclaration *
    GetLabelForDefinition
    (
        _In_ ParseTree::LabelReferenceStatement *LabelReference
    )
    {
        return GetLabel(LabelReference, true, NULL, NULL, NULL);
    }

    void
    InterpretIfStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *If
    );

    void
    InterpretElseIfStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *If
    );

    void
    InterpretLoopStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *Loop
    );

    void
    InterpretSelectStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *Select
    );

    void
    InterpretCaseStatement
    (
        _In_ ParseTree::CaseStatement *Case
    );

    void
    InterpretCaseElseStatement
    (
        _In_ ParseTree::ExecutableBlockStatement *CaseElse
    );

    bool
    RecommendSwitchTable
    (
        _Out_ ILTree::SelectBlock *Select
    );

    void
    OptimizeSelectStatement
    (
        _Out_ ILTree::SelectBlock *Select
    );

    void
    InterpretForStatement
    (
        _In_ ParseTree::ForStatement *For
    );

    ILTree::Expression *
    InterpretForLoopOperator
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        Type *ControlVariableType,
        _In_ ILTree::Expression *Left,
        _In_ ILTree::Expression *Right
    );

    void
    GenerateWarningIfNoExplicitFieldReference
    (
        _In_ ParseTree::ForStatement *For,
        ILTree::Expression* ControlVariableReference
    );

    bool
    TryInferForFromToType
    (
        _In_ ParseTree::ForFromToStatement *For,
        _In_ ILTree::Expression*& ControlVariableReference,
        _Out_ ILTree::Expression*& InitialValue,
        _Out_ ILTree::Expression*& Limit,
        _Inout_ ILTree::Expression*& Step
    );

    bool
    TypeMaySupportFor
    (
        Type * pType
    );

    void
    InterpretForFromToStatement
    (
        _In_ ParseTree::ForFromToStatement *For,
        ILTree::Expression *ControlVariableReference,
        Scope *ForLocals
    );

    void
    InterpretForEachStringStatement
    (
        _In_ ParseTree::ForEachInStatement *ForEach,
        ILTree::Expression *String,
        ILTree::Expression *ControlVariableReference,
        Scope *ForLocals,
        ILTree::Expression *ControlVariableTempInitializer
    );

    ILTree::Expression *
    InterpretForEachStatementCollection
    (
        _In_ ILTree::Expression *Collection,
        _Out_ bool *IsEnumerable,
        _Out_ Type **ElementType
    );

    void
    InterpretForEachStatement
    (
        _In_ ParseTree::ForEachInStatement *ForEach,
        _Out_ ILTree::Expression *ControlVariableReference,
        Scope *ForLocals
    );

    void
    GenerateDisposeCallBasedOnWellKnownPattern
    (
        Variable *ObjectToDispose,
        const Location &ErrorLoc,
        bool ProtectAgainstInvalidCast
    );

    // Returns true iff the collection is of a type that supports
    // the design pattern specified for use with collections in
    // For Each statements.
    bool
    MatchesForEachCollectionDesignPattern
    (
        _In_ ILTree::Expression *Collection,
        _Out_opt_ Type **ElementType
    );

    bool
    MatchesForEachCollectionDesignPattern
    (
        Type *CollectionType,
        ILTree::Expression *Collection,
        _Out_opt_ Type **ElementType
    );

    void
    InterpretForEachArrayStatement
    (
        _In_ ParseTree::ForEachInStatement *ForEach,
        ILTree::Expression *Array,
        ILTree::Expression *ControlVariableReference,
        Scope *ForLocals,
        ILTree::Expression *ControlVariableTempInitializer
    );

    typedef bool
    TypeTester
    (
        Type *T
    );

    typedef bool
    DeclarationTester
    (
        Declaration *D
    );

    Declaration *
    Semantics::ExtensionMethodMatchesForEachRequirements
    (
        ExtensionCallLookupResult * LookupResult,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        DeclarationTester *CandidateTester,
        _Out_opt_ GenericBinding ** GenericBindingContext
    );

    // A helper for MatchesForEachCollectionDesignPattern.

    Declaration *
    Semantics::MatchesForEachRequirements
    (
        Symbol * LookupResult,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        DeclarationTester *CandidateTester,
        _Inout_ bool &IsBad,
        _Out_opt_ GenericBinding **GenericBindingContext = NULL
    );

    // Performs error checking common to control variable references for
    // For and For Each loops.
    // Returns true if the loop control reference is OK for processing
    // the For(Each) loop, and false otherwise.
    bool
    VerifyForLoopControlReference
    (
        ILTree::Expression *ControlVariableReference,
        _In_ ParseTree::ForStatement *For
    );

    ILTree::WithBlock *
    InterpretWithStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *With
    );

    void
    InterpretWithStatement
    (
        ILTree::Expression *WithValue,
        _Inout_ ILTree::WithBlock *Result
    );

    ILTree::TryBlock *
    InterpretTryStatement
    (
        _In_ ParseTree::ExecutableBlockStatement *Try
    );

    void
    InterpretCatchStatement
    (
        _In_ ParseTree::CatchStatement *Catch
    );

    Variable *
    InterpretSyncLockStatement
    (
        _In_ ParseTree::ExpressionBlockStatement *SyncLock,
        _Out_ ILTree::TryBlock *&TryStatementForSynclock,
        _Out_ Variable * & pLockTakenVar
    );

    // Caller should create and manage the try statement
    // Dev10 #578762 If pLockTakenVar!=NULL on exit, the caller is also responsible to 
    // call Monitor.Enter(retVar, pLockTakenVar) as the first statement in the try block
    Variable *
    InterpretSyncLockStatement
    (
        ILTree::Expression *LockedObject,
        _In_ const Location &TreeLocation,
        const Location &OperandLocation,
        _Out_ Variable * & pLockTakenVar
    );

    // Dev10 #578762 
    Variable *
    InitSyncLockTakenVar
    (
        _In_ const Location & syncLockLocation,
        const unsigned short resumeIndex
    );

    // Dev10 #578762 
    void
    CallMonitorEnter_ObjectByRefBoolean
    (
        const Location &OperandLocation,
        Variable *pLockedObject,
        GenericBinding *pGenericBindingContextForVar,
        Variable * pLockTakenVar
    );
    
    void
    InterpretUsingStatement
    (
        _In_ ParseTree::UsingStatement *Using
    );

    void
    MakeInnerTryConstructForVariableDeclaration
    (
        _In_ ParseTree::UsingStatement *Using,
        _In_ ParseTree::VariableDeclarationList *Declarations
    );

    void MakeFinallyPartOfUsing
    (
        ParseTree::UsingStatement *Using,
        Variable *UsingVariable,
        _Out_opt_ ILTree::TryBlock *TryStatementForUsing
    );

    void
    TerminateSyncLockStatement
    (
        Variable *CapturedObject,
        const Location &EndConstructLocation,
        _Out_opt_ ILTree::TryBlock *TryStatementForSynclock,
        _In_ Variable * pLockTakenVar
    );

    // Call "System.Threading.Monitor.<XXX>", passing a reference
    // to LockedObject as an argument. (This is a helper for the
    // interpretation of SyncLock.)
    void
    CallMonitorMethod
    (
        const Location &OperandLocation,
        const unsigned short ResumeIndex,
        Variable *LockedObject,
        GenericBinding *GenericBindingContextForVar,
        _In_z_ Identifier *MethodName
    );

    void
    InterpretExitStatement
    (
        _In_ ParseTree::Statement *Exit
    );

    void
    InterpretReturn
    (
        _Inout_ ParseTree::ExpressionStatement *UnboundCall
    );

    void
    InterpretAwaitStatement
    (
        _Inout_ ParseTree::ExpressionStatement *Await
    );

    void
    InterpretYieldStatement
    (
        _Inout_ ParseTree::ExpressionStatement *Yield
    );

    void
    Semantics::InterpretContinueStatement
    (
        _In_ ParseTree::Statement *Continue
    );

    void
    InterpretVariableDeclarationStatement
    (
        _In_ ParseTree::VariableDeclarationStatement *VariableDeclaration
    );

    void
    InterpretErrorStatement
    (
        _In_ ParseTree::ExpressionStatement *Error
    );

    void
    InterpretThrowStatement
    (
        _In_ ParseTree::ExpressionStatement *Error
    );

    void
    InterpretRedimStatement
    (
        _In_ ParseTree::RedimStatement *Redim
    );

    void
    InterpretEraseStatement
    (
        _In_ ParseTree::EraseStatement *Erase
    );

    void
    InterpretAssignMidStatement
    (
        _In_ ParseTree::AssignMidStatement *AssignMid
    );

    void
    InterpretRaiseEventStatement
    (
        _In_ ParseTree::RaiseEventStatement *RaiseEvent
    );

    void
    InterpretHandlerStatement
    (
        _In_ ParseTree::HandlerStatement *AddOrRemove
    );

    ILTree::Expression*
    ApplyComEventInterfaceBinder
    (
        Declaration *Event,
        ParseTree::IdentifierDescriptor &EventName,
        ParseTree::HandlerStatement *AddOrRemove,
        GenericBinding *EventGenericBindingContext,
        ILTree::Expression *Base,
        ILTree::Expression *DelegateBinding
    );

    void
    CheckForBlockLevelShadowing
    (
        _In_ ILTree::ExecutableBlock *Child
    );

    void
    CheckChildrenForBlockLevelShadowing
    (
        _In_ ILTree::ExecutableBlock *Parent
    );

    // Methods to manage block contexts.

    void
    LinkStatement
    (
        ILTree::Statement *Statement
    );

    void
    EstablishBlockContext
    (
        _In_ ILTree::ExecutableBlock *NewContext
    );

    void
    PopBlockContext
    (
    );

    // Create a temporary with the same type as Value, assign Value to it, and
    // return the assignment. Temporary is set to the created temporary.
    ILTree::ExpressionWithChildren *
    CaptureInTemporaryImpl
    (
        ILTree::Expression *Value,
        Variable *Temporary
    );

    inline ILTree::ExpressionWithChildren *
    CaptureInLongLivedTemporary
    (
        _In_ ILTree::Expression *Value,
        _Out_ Variable *&Temporary,
        ILTree::ExecutableBlock *Block
    )
    {
        ThrowIfNull(Block); // LongLived temporaries must be associated with a Block
        Temporary = AllocateLongLivedTemporary(Value->ResultType, &Value->Loc, Block);
        return CaptureInTemporaryImpl(Value, Temporary);
    }

    inline ILTree::ExpressionWithChildren *
    CaptureInShortLivedTemporary
    (
        _In_opt_ ILTree::Expression *Value,
        _Out_ Variable *&Temporary
    )
    {
        ThrowIfNull(Value);
        Temporary = AllocateShortLivedTemporary(Value->ResultType, &Value->Loc);
        return CaptureInTemporaryImpl(Value, Temporary);
    }

    // Returns the count of the accessible base class constructors that can be
    // invoked with no parameters. If base class is bad, then it returns -1
    //

    int
    BaseClassConstructorAcceptingNoArgumentsCount
    (
        ClassOrRecordType *DerivedClass,
        _Out_ Procedure *&MatchingConstructor,
        _Out_ bool &HasAccessibleConstructor
    );

    // Create a temporary with the same type as Value, assign Value to it, and
    // return a sequence expresssion that refers to the temporary.  Don't use
    // this method directly, instead use the ShortLived and LongLived versions
    ILTree::Expression *
    CaptureInTemporaryAsSequenceImpl
    (
        ILTree::Expression *Value,
        Variable *Temporary,
        ILTree::ExpressionWithChildren *Assign
    );

    ILTree::Expression *
    CaptureInShortLivedTemporaryAsSequence
    (
        _In_opt_ ILTree::Expression *Value
    )
    {
        Variable *Temporary = NULL;
        ILTree::ExpressionWithChildren *Assign = CaptureInShortLivedTemporary(Value, Temporary);
        return CaptureInTemporaryAsSequenceImpl(Value, Temporary, Assign);
    }

    ILTree::Expression *
    CaptureInLongLivedTemporaryAsSequence
    (
        _In_ ILTree::Expression *Value,
        ILTree::ExecutableBlock *block
    )
    {
        Variable *Temporary = NULL;
        ILTree::ExpressionWithChildren *Assign = CaptureInLongLivedTemporary(Value, Temporary, block);
        return CaptureInTemporaryAsSequenceImpl(Value, Temporary, Assign);
    }

    // Create a temporary with the same type as Argument, assign Argument to it,
    // and return an expression representing the address of the temporary.
    // TemporaryType is explicit (rather than implicitly being the type of Argument)
    // because a fixed string RValue has type String, but should be captured to a
    // temporary with a fixed string type.
    // Temporary is set to the created temporary.

    ILTree::Expression *
    CaptureInAddressedTemporary
    (
        ILTree::Expression *Argument,
        Type *TemporaryType,
        Variable *&Temporary
    );

    ILTree::Expression *
    CaptureInAddressedTemporary
    (
        ILTree::Expression *Argument,
        Type *TemporaryType
    )
    {
        Variable *Temporary;
        return CaptureInAddressedTemporary(Argument, TemporaryType, Temporary);
    }

    // Create trees to produce the same value twice, producing side effects
    // only once.
    // If FirstResultUsedAsValue is true, then FirstResult is set to compute
    // the value of Value, and to capture all RValues in Value in temporaries;
    // otherwise, FirstResult is set to capture the RValues, but not to produce
    // a result. SecondResult is always set compute the value of Value, using
    // the temporary values captured by FirstResult.
    void
    UseTwiceImpl
    (
        ILTree::Expression *Value,
        ILTree::Expression *&FirstResult,
        ILTree::Expression *&SecondResult,
        bool UseLongLivedTemporaries,
        bool FirstResultUsedAsValue,
        ILTree::ExecutableBlock *block
    );

    void
    UseTwiceShortLived
    (
        ILTree::Expression *Value,
        ILTree::Expression *&FirstResult,
        ILTree::Expression *&SecondResult
    )
    {
        UseTwiceImpl(Value, FirstResult, SecondResult, false, true, NULL);
    }

    void
    UseTwiceLongLived
    (
        ILTree::Expression *Value,
        ILTree::Expression *&FirstResult,
        ILTree::Expression *&SecondResult,
        ILTree::ExecutableBlock *block
    )
    {
        ThrowIfNull(block);
        UseTwiceImpl(Value, FirstResult, SecondResult, true, false, block);
    }

    inline Variable *
    AllocateLongLivedTemporary
    (
        Type *ResultType,
        const Location *TextSpan,
        ILTree::ExecutableBlock *Block
    )
    {
        ThrowIfNull(Block);

        // DevDiv 91052
        // The call to AllocateLongLivedTemporary should never use a lambda temporary manager.
        // Long lived temporaries are associated with a block.  Yet lambdas have no block.  So
        // we should always prefer the procedure temporary manager when allocating a long lived
        // temporary.
        //
        // However we can still be askde to create one in the context of an expression so we
        // must fall back to the current if no proc is available
        TemporaryManager *manager = m_TemporaryManager;
        if ( m_ProcedureTree && m_ProcedureTree->ptempmgr && (m_InLambda != MultiLineSubLambda) && (m_InLambda != MultiLineFunctionLambda))
        {
            manager = m_ProcedureTree->ptempmgr;
        }

        return manager->AllocateLongLivedTemporary(ResultType, TextSpan, Block);
    }

    inline Variable *
    AllocateShortLivedTemporary
    (
        Type *ResultType,
        const Location *TextSpan
    )
    {
        return m_TemporaryManager->AllocateShortLivedTemporary(ResultType, TextSpan);
    }

    inline Variable *
    AllocateShortLivedTemporary
    (
        Type *ResultType
    )
    {
        return AllocateShortLivedTemporary(ResultType, NULL);
    }

    inline Variable *
    AllocateLifetimeNoneTemporary
    (
        Type *ResultType,
        const Location *TextSpan
    )
    {
        return m_TemporaryManager->AllocateLifetimeNoneTemporary(ResultType, TextSpan);
    }

    inline Variable *
    AllocateDefaultValueTemporary
    (
        Type *ResultType,
        const Location *TextSpan
    )
    {
        return m_TemporaryManager->AllocateDefaultValueTemporary(ResultType, TextSpan);
    }

    Variable *
    AllocateResultTemporary
    (
        Type *ResultType
    );

    ILTree::Expression *
    InterpretConditionalOperand
    (
        ParseTree::Expression *Conditional,
        ExpressionFlags Flags = ExprNoFlags
    );

    ILTree::Expression *
    NegateBooleanExpression
    (
        ILTree::Expression *Input
    );

    // Perform semantic analysis of an expression, returning the resulting tree. The result
    // tree has the SF_ERROR flag set if semantic analysis fails. Flags are as indicated
    // above.

    ILTree::Expression *
    InterpretExpression
    (
        ParseTree::Expression *Input,
        ExpressionFlags Flags,
        int GenericTypeArity = 0,
        Location *GenericTypeArgsLoc = NULL,
        Type *TargetType = NULL   // Some things e.g. array literals can only be interpreted in a context where the target type is known
    );

    ILTree::Expression *
    InterpretExpressionWithTargetType
    (
        ParseTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType,
        Type **OriginalType = NULL
    );

    //If you are looking at a windiff trying to figure out what's going on
    //with this file so that you can do an integration, here's the jist of it:
    //
    //InterpretGenericQualifiedExpression was renamed to
    //InterpretGenericQualifiedSymbolExpression, and  then a new
    //procedure named InterpretGenericQualifiedExpression was added.
    //The new InterpretGenericQualifiedExpression contains contents that used to be under
    //the Generic Qualified expression branch of InterpretExpression.
    ILTree::Expression *
    InterpretGenericQualifiedExpression
    (
        ParseTree::GenericQualifiedExpression *GenericQualified,
        ILTree::Expression * BaseReference,
        unsigned ArgumentCount,
        ExpressionFlags Flags,
        bool & CallerShouldReturnImmedietly
    );

    //If you are looking at a windiff trying to figure out what's going on
    //with this file so that you can do an integration, here's the jist of it:
    //
    //InterpretGenericQualifiedExpression was renamed to
    //InterpretGenericQualifiedSymbolExpression, and  then a new
    //procedure named InterpretGenericQualifiedExpression was added.
    //The new InterpretGenericQualifiedExpression contains contents that used to be under
    //the Generic Qualified expression branch of InterpretExpression.
    ILTree::Expression *
    InterpretGenericQualifiedSymbolExpression
    (
        ParseTree::GenericQualifiedExpression * GenericQualified,
        ILTree::SymbolReferenceExpression * BaseReference,
        Type **BoundArguments,
        Location * TypeArgumentLocations,
        unsigned int ArgumentCount,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    Semantics::AttemptQueryNameLookup
    (
        Location & location,
        _In_z_ Identifier *Name,
        typeChars TypeCharacter,
        NameFlags NameLookupFlags,
        ExpressionFlags Flags,
        int GenericTypeArity,
        _Out_opt_ Symbol ** pNameBinding,
        bool CheckUseOfLocalBeforeDeclaration  = true
    );

    ILTree::Expression *
    AttemptInterpretMemberReference
    (
        Location & location,
        _In_z_ Identifier * Name,
        typeChars TypeCharacter,
        NameFlags NameLookupFlags,
        ExpressionFlags ExprFlags,
        int GenericTypeArity,
        BCSYM_NamedRoot * cutOffParent
    );


    struct AttemptInterpretMemberReferenceInfo : public CSingleLink<AttemptInterpretMemberReferenceInfo>
    {
        GenericBinding * candidateGenericBindingContext;
        Symbol * candidateNameBinding;
    };

    AttemptInterpretMemberReferenceInfo *
    AttemptInterpretMemberReference
    (
        Type * lookInType,
        Location & location,
        _In_z_ Identifier *Name,
        NameFlags NameLookupFlags,
        int GenericTypeArity,
        _Inout_ NorlsAllocator * Scratch,
        _Inout_ CSingleList<AttemptInterpretMemberReferenceInfo> &MemberReferenceList
    );

    bool
    AttemptInterpretNestedMemberReference
    (
        Type * lookInType,
        Location & location,
        _In_z_ Identifier *Name,
        NameFlags NameLookupFlags,
        int GenericTypeArity,
        _Inout_ NorlsAllocator * Scratch,
        _Inout_ CSingleList<AttemptInterpretMemberReferenceInfo> &MemberReferenceList
    );

    bool
    CanAccessMemberUnqualified
    (
        Symbol *Referenced
    );

    ILTree::Expression *
    InterpretNameExpression
    (
        ParseTree::Expression *Input,
        ExpressionFlags Flags,
        int GenericTypeArity,
        Location *GenericTypeArgsLoc
    );

    struct InitializerInferInfo
        : BackupValue<InitializerInferInfo*>
    {
        InitializerInferInfo(InitializerInferInfo** parent);

        InitializerInferInfo* Parent;
        bool CircularReferenceDetected;
        BCSYM_Variable* Variable;
    };

    InitializerInferInfo*
    CircularReferenceInInitializer
    (
        _In_ Symbol* pNameBinding
    );

    Type *
    ExtractReceiverType
    (
        ILTree::Expression * pExpr
    );

    ILTree::Expression *
    InterpretCallOrIndex
    (
        ParseTree::CallOrIndexExpression * CallOrIndex,
        ExpressionFlags Flags,
        typeChars TypeCharacter
    );

    ILTree::Expression *
    InterpretIIF
    (
        ParseTree::IIfExpression *ift,
        ExpressionFlags Flags
    );

    ILTree::Expression*
    CreateTernaryIIF
    (
        ILTree::Expression* Condition,
        ILTree::Expression* TrueExpr,
        ILTree::Expression* FalseExpr,
        const Location &TreeLocation,
        ExpressionFlags Flags
    );

    ILTree::Expression*
    CreateCoalesceIIF
    (
        ILTree::Expression* Operand1,
        ILTree::Expression* Operand2,
        const Location &TreeLocation,
        ExpressionFlags Flags
    );


    ILTree::Expression*
    ForceLiftToEmptyString
    (
        ILTree::Expression *pExpr,
        Type    *ResultType
    );

    Symbol *
    AttemptInterpretLocalReference
    (
        Location & location,
        _In_z_ Identifier * Name,
        NameFlags NameLookupFlags,
        ExpressionFlags ExprFlags,
        int GenericTypeArity,
        ILTree::Expression ** ppResult, //if we detect an error then we will place a bad expression here
        bool CheckUseOfLocalBeforeDeclaration = true
    );

    // Interpret an expression that occurs as the operand of an expression.
    ILTree::Expression *
    InterpretStatementOperand
    (
        ParseTree::Expression *Input,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretStatementOperand
    (
        ParseTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType,
        Type **OriginalType
    );

    // Semantic processing that applies to all expressions after the
    // semantics of the specific expression.

    ILTree::Expression *
    ApplyContextSpecificSemantics
    (
        ILTree::Expression *Result,
        ExpressionFlags Flags,
        Type *TargetType
    );

    // Interpret a dot/bang qualified reference.

    ILTree::Expression *
    InterpretQualifiedExpression
    (
        ILTree::Expression *BaseReference,
        _In_z_ Identifier *Name,
        ParseTree::Expression::Opcodes Opcode,
        const Location &TextSpan,
        ExpressionFlags Flags,
        int GenericTypeArity = 0
    );

    ILTree::Expression *
    SetupLookupEnviornmentForQualifiedExpressionInterpretation
    (
        ILTree::Expression * & BaseReference,
        Scope * & MemberLookup,
        bool & BaseReferenceIsNamespace,
        Type * & BaseReferenceType,
        GenericParameter * & TypeParamToLookupInForMember,
        Type * & TypeForGenericBinding,
        ParseTree::Expression::Opcodes Opcode,
        const Location & TextSpan
    );

    ILTree::Expression *
    InterpretQualifiedExpression
    (
        ILTree::Expression *BaseReference,
        ParseTree::Expression *Name,
        ParseTree::Expression::Opcodes Opcode,
        const Location &TextSpan,
        ExpressionFlags Flags,
        int GenericTypeArity = 0
    );

    // Check semantic constraints on creating an instance of TypeOfInstance
    // with the indicated Arguments, returning a tree that represents the
    // allocation/construction.

    ILTree::Expression *
    CreateConstructedInstance
    (
        Type *TypeOfInstance,
        const Location &TypeTextSpan,
        const Location &TextSpan,
        ParseTree::ArgumentList *UnboundArguments,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    CreateConstructedInstance
    (
        Type *TypeOfInstance,
        const Location &TypeTextSpan,
        const Location &TextSpan,
        ExpressionList *BoundArguments,
        bool SomeArgumentsBad,
        ExpressionFlags Flags
    );

    ILTree::Expression * 
    CreateInstanceComInteropNoPIA
    (
        Type *TypeOfInstance,
        Type *ConstructedType,
        const Location &loc
    );

public:
    static Type * 
    GetCanonicalTypeFromLocalCopy
    (
        Type *pType,
        bool & outFoundCanonicalType
    );
protected:

#if IDE  
    // Compute a single identifier from a qualified name expression.
    // GlobalQualified: caller should initialize to false
    Identifier *
    SynthesizeQualifiedName
    (
        ParseTree::QualifiedExpression *Input,
        bool &GlobalQualified   // caller should initialize to false
    );
#endif

    ExpressionList *
    InterpretArraySizeList
    (
        ParseTree::ArrayDimList *Dimensions,
        ExpressionFlags Flags,
        bool &SomeDimensionsBad
    );

    ExpressionList *
    InterpretArrayInitializerList
    (
        ParseTree::BracedInitializerList *Input,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InitializeArray
    (
        ExpressionList *Initializer,
        ArrayType *ResultType,
        ExpressionList *DimensionSizes,
        const Location &TextSpan
    );

    ILTree::Expression *
    InitializeArray
    (
        ExpressionList *Initializer,
        ArrayType *ResultType,
        ExpressionList *DimensionSizes,
        const Location &TextSpan,
        unsigned StorageIndices[]
    );

    ILTree::Expression *
    Semantics::InitializeArray
    (
        ExpressionList *Initializer,
        ArrayType *ResultType,
        ExpressionList *DimensionSizes,
        const Location &TextSpan,
        unsigned StorageIndices[],
        Variable *&ArrayTemporary
    );

    // Generates a bound tree representing the statements required to
    // initialize the object followed by a reference to the object.
    //
    ILTree::Expression *
    CreateInitializedObject
    (
        ParseTree::BracedInitializerList *BracedInitializerList,
        ILTree::Expression *ObjectToInitialize,
        const Location &TextSpanOfObjectInit,
        const Location &TextSpanOfWithClause,
        ExpressionFlags Flags
    );

    // Generates a bound tree representing the statements required to
    // initialize the object.
    //
    ILTree::Expression *
    InitializeObject
    (
        ParseTree::BracedInitializerList *BracedInitializerList,
        ILTree::Expression *ObjectToInitialize,
        const Location &TextSpanOfWithClause,
        ExpressionFlags Flags
    );

    // Convert all elements of an array initializer to the element type of the array.
    // This is not used for the mainline compiler logic, but is used in creating
    // trees for XML generation.

    void
    ConvertAllArrayElements
    (
        ExpressionList *Initializer,
        Type *ElementType
    );

    typedef struct DimCounts
    {
        unsigned DimensionCount;
        bool isNotConstant;
    } DIMCOUNTS;

    ILTree::Expression *
    InitializeArrayElements
    (
        Variable *ArrayTemporary,
        ArrayType *InitializedArrayType,
        ExpressionList *Initializer,
        unsigned Dimension,
        _In_count_(IndicesCount) unsigned Indices[],
        unsigned IndicesCount,
        DIMCOUNTS DimensionCounts[]
    );

    ILTree::Expression *
    InitializeArrayElements
    (
        Variable *ArrayTemporary,
        ArrayType *InitializedArrayType,
        ExpressionList *Initializer,
        unsigned Dimension,
        _In_count_(IndicesCount) unsigned Indices[],
        unsigned IndicesCount,
        DIMCOUNTS DimensionCounts[],
        unsigned StorageIndices[]
    );

    ILTree::Expression *
    InitializeArrayElementsAsBlob
    (
        ArrayType *InitializedArrayType,
        ExpressionList *Initializer,
        unsigned Dimension,
        DIMCOUNTS DimensionCounts[]
    );

    // return true if valid initializer, else returns false
    bool
    ValidateArrayInitializer
    (
        ExpressionList *Initializer,
        unsigned Dimension,
        unsigned DimensionCount,
        DIMCOUNTS DimensionCounts[],
        unsigned &InitializerCount
    );

    void
    CheckRecursiveOperatorCall
    (
        Procedure *CallTarget,
        const Location &CallLocation
    );

    ILTree::Expression *
    InterpretUserDefinedOperator
    (
        BILOP Opcode,
        Procedure *OperatorMethod,
        GenericBinding *OperatorMethodGenericContext,
        const Location &ExpressionLocation,
        ILTree::Expression *Operand,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretUserDefinedOperator
    (
        BILOP Opcode,
        Procedure *OperatorMethod,
        GenericBinding *OperatorMethodGenericContext,
        const Location &ExpressionLocation,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretUserDefinedShortCircuitOperator
    (
        BILOP Opcode,
        Procedure *OperatorMethod,
        GenericBinding *OperatorMethodGenericContext,
        const Location &ExpressionLocation,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    Semantics::InterpretGetType
    (
        ParseTree::GetTypeExpression * UnboundType,
        ExpressionFlags Flags
    );


    ILTree::Expression *
    Semantics::InterpretGetType
    (
        Type * SourceType,
        bool DisallowOpenTypes,
        const Location &ExpressionLocation,
        const Location &TypeLocation
    );

    ILTree::Expression *
    Semantics::InterpretGetXmlNamespace
    (
        ParseTree::GetXmlNamespaceExpression * UnboundPrefix,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    Semantics::InterpretUnaryOperation
    (
        ParseTree::Expression::Opcodes Opcode,
        const Location &ExpressionLocation,
        ILTree::Expression *Operand,
        ExpressionFlags Flags
    );

    virtual 
    ILTree::Expression *
    Semantics::InterpretAwaitExpression
    (
        const Location &ExpressionLocation,
        ParseTree::Expression *OperandTree,
        ExpressionFlags Flags
    );

    static bool
    Semantics::IsNothingOrConversionFromNothing
    (
        ILTree::Expression *pExp
    );

    // Given a tree for a binary operator, whose operands have been interpreted
    // and are known not to be bad, apply the semantics of the binary operation
    // and return the result.

    ILTree::Expression *
    InterpretBinaryOperation
    (
        ParseTree::Expression::Opcodes Opcode,
        const Location &ExpressionLocation,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    Semantics::InterpretBinaryOperation
    (
        BILOP BoundOpcode,
        const Location &ExpressionLocation,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        ExpressionFlags Flags,
        bool fSelectGenerated // this is an expression generated for Select/Case statement
    );

    BILOP
    MapOperator
    (
        ParseTree::Expression::Opcodes Opcode
    );

    ILTree::Expression *
    InterpretDelegateBinding
    (
        ILTree::Expression *Input,
        Type *DelegateType,
        const Location &AddressOfTextSpan,
        bool SuppressMethodNameInErrorMessages,
        ExpressionFlags Flags,
        DelegateRelaxationLevel &RelaxationLevel,
        bool *pRequiresNarrowingConversion
    );

    ILTree::Expression *
    InterpretDelegateBinding
    (
        ILTree::Expression *MethodOperand,
        ILTree::Expression * Input,
        Type *DelegateType,
        const Location &AddressOfTextSpan,
        bool SuppressMethodNameInErrorMessages,
        ExpressionFlags Flags,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        OverloadResolutionFlags OverloadFlags,
        bool *pRequiresNarrowingConversion
    );


    void
    GetAccessibleSignatureMismatchErrorAndLocation
    (
        unsigned long numberOfFormalTypeParameters,
        unsigned long numberOfActualTypeArguments,
        bool suppressMethodNameInErrorMessages,
        bool candidatesIsExtensionMethod,
        Location * pMethodOperandLocation,
        Location * pActualTypeArgumentsSpan,
        RESID * pErrorIDOut,
        Location ** ppErrorLocationOut
    );

    ILTree::Expression *
    Semantics::CreateDelegateConstructorCall
    (
        Type *DelegateType,
        ILTree::Expression *ObjectArgument,
        ILTree::Expression *MethodOperand,
        unsigned DelegateCreateFlags,
        const Location &AddressOfTextSpan
    );

    Procedure *
    ResolveMethodForDelegateInvoke
    (
        ILTree::Expression *MethodOperand,
        unsigned Flags,
        Procedure *InvokeMethod,
        Type *DelegateType,
        Type *OriginalDelegateTypeForErrors,
        GenericBinding *&GenericBindingContext,
        bool SuppressMethodNameInErrorMessages,
        bool IgnoreReturnValueErrorsForInference,
        MethodConversionClass &MethodConversion,
        OverloadResolutionFlags OverloadFlags,
        bool & ResultIsExtensionMethod,
        bool *pRequiresNarrowingConversion
    );

    Procedure *
    Semantics::ResolveExtensionMethodForDelegateInvokeTryingFullAndRelaxedArgs
    (
        ILTree::ExtensionCallExpression * MethodOperand,
        unsigned uFlags,
        Procedure * InvokeMethod,
        Type * DelegateType,
        Type* OriginalDelegateTypeForErrors,
        bool IgnoreReturnValueErrorsForInference,
        GenericBindingInfo & Binding,
        MethodConversionClass &MethodConversion,
        bool & ResultIsExtensionMethod,
        bool *pRequiresNarrowingConversion
    );

    ILTree::Expression *
    CreateRelaxedDelegateLambda
    (
        ILTree::Expression* MethodOperand,
        ILTree::Expression* ObjectArgument,
        Procedure *TargetMethod,
        Procedure *InvokeMethod,
        Type *DelegateType,
        BCSYM_GenericBinding *GenericBindingContext,
        _In_ const Location &Location,
        bool CallIsExtensionMethod
    );

    ILTree::Expression*
    ConvertAnonymousDelegateToOtherDelegate  // WARNING! This function is badly named. It does something quite different. Read the comments in its implementation.
    (
        _In_ ILTree::Expression*     Input,
        Type*           TargetType
    );

    ILTree::UnboundLambdaExpression *
    ShallowCloneUnboundLambdaExpression
    (
        _In_ ILTree::UnboundLambdaExpression * Input
    );

    BCSYM_Param*
    CopyParameterList
    (
        Procedure *InvokeMethod,
        BCSYM_GenericBinding *GenericBindingContext,
        bool IsRelaxedDelegateParameterList
    );

    ParseTree::ArgumentList *
    Semantics::CreateArgumentList
    (
        BCSYM_Param *InvokeParam,
        BCSYM_Param *TargetParam,
        bool ForceCopyInvokeArguments,
        const Location &Location
    );

    bool
    ProcessExplicitTypeArgumentsForExtensionMethod
    (
        ILTree::ExtensionCallExpression * pExtensionCall,
        GenericBindingInfo & binding,
        ExtensionCallInfo * pCandidate,
        DynamicArray<ProcedureDescriptor> * pAccessibleSignatureMismatches,
        Location * & pTypeArgumentLocations
    );

    void InsertIntoProcDescriptorArray
    (
        ExtensionCallInfo * pCandidate,
        GenericBindingInfo binding,
        DynamicArray<ProcedureDescriptor> * pDescriptorArray
    );

    bool
    InferTypeArgumentsForExtensionMethodDelegate
    (
        ILTree::ExtensionCallExpression * pExtensionCall,
        GenericBindingInfo & binding,
        ExtensionCallInfo * pCandidate,
        DynamicArray<ProcedureDescriptor> * pAccessibleTypeInferenceFailures,
        Location * & pTypeArgumentLocations,
        Procedure * pDelegateInvokeMethod,
        GenericBinding * pDelegateBinding,
        TypeInferenceLevel & TypeInferenceLevel
    );

    Procedure *
    ResolveCandidateInstanceMethodForDelegateInvokeAndReturnBindingContext
    (
        ILTree::Expression *MethodOperand,
        unsigned uFlags,
        Procedure *InvokeMethod,
        Type *DelegateType,
        Type* OriginalDelegateTypeForErrors,
        Procedure *&InaccessibleMatchingMethod,
        bool &SomeCandidatesBad,
        Location *&MatchingMethodTypeArgumentLocations,
        GenericBinding * &GenericBindingContext,
        bool SuppressMethodNameInErrorMessages,
        bool IgnoreReturnValueErrorsForInference,
        MethodConversionClass &MethodConversion,
        OverloadResolutionFlags OverloadFlags,
        bool *pRequiresNarrowingConversion,
        bool *pWouldHaveSucceededWithStrictOff,
        bool *pCouldTryZeroArgumentRelaxation,
        bool attemptZeroArgumentRelaxation
    );

    Procedure *
    ResolveInstanceMethodForDelegateInvokeTryingFullAndRelaxedArgs
    (
        ILTree::Expression *MethodOperand,
        unsigned uFlags,
        Procedure *InvokeMethod,
        Type *DelegateType,
        Type* OriginalDelegateTypeForErrors,
        Procedure *&InaccessibleMatchingMethod,
        bool &SomeCandidatesBad,
        Location *&MatchingMethodTypeArgumentLocations,
        GenericBinding * &GenericBindingContext,
        bool SuppressMethodNameInErrorMessages,
        bool IgnoreReturnValueErrorsForInference,
        MethodConversionClass &MethodConversion,
        OverloadResolutionFlags OverloadFlags,
        bool *pRequiresNarrowingConversion
    );

    bool
    IsMagicDelegateConstructor
    (
        Procedure *DelegateConstructor
    );

    ExpressionList *
    InterpretArgumentList
    (
        ParseTree::ArgumentList *UnboundArguments,
        bool &SomeArgumentsBad,
        ExpressionFlags ArgumentFlags
    );

    ILTree::Expression *
    Semantics::MakeCallLateBound
    (
        ILTree::Expression *Target,
        Procedure *TargetProcedure,
        unsigned TypeArgumentCount,
        Type **TypeArguments,
        Location *TypeArgumentLocations,
        GenericBinding *TargetBinding,
        ExpressionList *BoundArguments,
        const Location &CallLocation,
        ExpressionFlags Flags
    );

    // Interpret a procedure call. At input, Target is bound and the
    // UnboundArguments are unbound. The method with no CopyOutArguments
    // parameter incorporates any post-call assignments necessary for
    // ByRef parameters into the result expression--the other returns
    // the assignments for later processing.
    //
    // We have four InterpretCallExpression functions. Here is their callgraph.
    //
    // BindArgsAndInterpretCallExpressionWithNoCopyOut   <--  InterpretCall, InterpretCallOrIndex, InterpretQualifiedExpression,
    //          |                                             InterpretQueryOperator, InterpretXMLContent
    //     (supply null copyout)
    //          |
    //         \|/
    // BindArgsAndInterpretCallExpression   <--  InitializeConstructedVariable
    //          |                            --> InterpretArgumentList
    //      (bind args)
    //          |                  
    //         \|/                
    // InterpretCallExpression   <--> InterpretExtensionCallExpression
    //         /|\               <--  Convert, CreateConstructedInstance
    //          |
    //      (supply null copyout)
    //          |
    // InterpretCallExpressionWithNoCopyOut   <--  ConstructLateBoundArgumentList, CreateXNameExpression,
    //                                             InterpretCallOrIndex, InterpretGenericQualifiedSymbol, 
    //                                             InterpretXMLContent, ReferToSymbol

    ILTree::Expression *
    InterpretExtensionCallExpression
    (
        const Location &CallLocation,
        ILTree::ExtensionCallExpression * extCall,
        typeChars TypeCharacter,
        ExpressionList *BoundArguments,
        ExpressionList *&CopyOutArguments,
        bool SomeArgumentsBad,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        Declaration *RepresentTargetInMessages
    );


    ILTree::Expression *
    BindArgsAndInterpretCallExpressionWithNoCopyOut
    (
        const Location &CallLocation,
        ILTree::Expression *Target,
        typeChars TypeCharacter,
        ParseTree::ArgumentList *UnboundArguments,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        Declaration *RepresentTargetInMessages
    );

    ILTree::Expression *
    BindArgsAndInterpretCallExpression
    (
        const Location &CallLocation,
        ILTree::Expression *Target,
        typeChars TypeCharacter,
        ParseTree::ArgumentList *UnboundArguments,
        ExpressionList *&CopyOutArguments,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        Declaration *RepresentTargetInMessages
    );

    // Interpret a call with bound arguments.

    ILTree::Expression *
    InterpretCallExpressionWithNoCopyout
    (
        const Location &CallLocation,
        ILTree::Expression *Target,
        typeChars TypeCharacter,
        ExpressionList *Arguments,
        bool SomeArgumentsBad,
        ExpressionFlags Flags,
        Declaration *RepresentTargetInMessages
    );

    ILTree::Expression *
    InterpretCallExpression
    (
        const Location &CallLocation,
        ILTree::Expression *Target,
        typeChars TypeCharacter,
        ExpressionList *Arguments,
        ExpressionList *&CopyOutArguments,
        bool SomeArgumentsBad,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        Declaration *RepresentTargetInMessages
    );

    ILTree::Expression *
    OptimizeLibraryCall
    (
        ILTree::CallExpression &LibraryCall,
        ExpressionFlags Flags
    );

    ExpressionList *
    ConstructLateBoundArgumentList
    (
        ExpressionList *InterpretedArguments,
        const Location &CallLocation,
        bool LateBoundAssignment,
        bool NeedAssignmentInfo,
        ExpressionList *&CopyOutArguments,
        ILTree::Expression *&AssignmentInfoArrayParam
    );

    ILTree::Expression *
    InterpretLateBoundExpression
    (
        const Location &ExpressionLocation,
        LateReferenceExpression &Target,
        ExpressionList *Arguments,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretLateBoundExpression
    (
        const Location &ExpressionLocation,
        LateReferenceExpression &Target,
        ParseTree::ArgumentList *Arguments,
        ExpressionFlags Flags
    );

    void
    Semantics::SetLateCallInvocationProperties
    (
        ILTree::BinaryExpression &Target,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    AppendCopyOutArguments
    (
        ILTree::Expression *Result,
        ExpressionList *CopyOutArguments,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    InterpretObjectIndexReference
    (
        const Location &ExpressionLocation,
        ILTree::Expression *ArrayRef,
        ParseTree::ArgumentList *UnboundIndices
    );

    // Interpret an array indexing. At input, ArrayRef is bound and the
    // UnboundIndices are unbound.

    ILTree::Expression *
    InterpretArrayIndexReference
    (
        const Location &ExpressionLocation,
        ILTree::Expression *ArrayRef,
        const ParseTree::ParenthesizedArgumentList &UnboundIndices
    );

    ExpressionList *
    InterpretArrayIndices
    (
        ParseTree::ArgumentList *UnboundIndices,
        bool ForRedim,
        unsigned &IndexCount,
        bool &SomeOperandsBad
    );

    // Returns true iff the argument can be passed as by itself as a paramarray
    // parameter of the indicated type. In this case the argument will be passed
    // as the paramarray, not as an element of the paramarray.

    bool
    CanPassToParamArray
    (
        ILTree::Expression *Argument,
        Type *ParamArrayType
    );

    // MatchArguments creates a list of arguments, matched to appropriate
    // parameters, that can be used to call TargetProcedure. If it is not
    // valid to produce such a binding, SomeArgumentsBad is set true.
    //
    // The input Arguments have been interpreted. CAUTION: MatchArguments can
    // alter the input Arguments. If you need to keep MatchArguments "pure"
    // (i.e. without side-effects on the tree) then call SaveArguments+MakeScratchCopiesOfArguments
    // before, and RestoreArguments afterwards. If you also want MatchArguments
    // to be without side-effects on the error-table, then use
    // BackupValue<bool> backup_report_errors(&m_ReportErrors);
    //
    // We have four MatchArguments functions. Here is their callgraph:
    //
    //   MatchArguments2  <--- ResolveCandidateInstanceMethodForDelegateInvoke
    //       |
    //      \|/
    //   MatchArguments1  <--- InterpretCallExpression, ResolveOverloading
    //       |
    //      \|/
    //   MatchArguments4  <--- RejectUncallableProcedure
    //      /|\
    //       |
    //   MatchArguments3  <--- DetectArgumentErrors, DetectArgumentNarrowing

    ExpressionList *
    MatchArguments1
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        Declaration *RepresentTargetInMessages,
        GenericBinding * & GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        ExpressionFlags CallFlags,
        OverloadResolutionFlags OvrldFlags,
        ExpressionList *&CopyOutArguments,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool DisallowParamArrayExpansion,
        bool DisallowParamArrayExactMatch,
        bool &SomeArgumentsBad,
        bool &ArgumentArityBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool &InferenceFailed,
        bool &AllFailedInferenceIsDueToObject,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        IReadonlyBitVector * FixedTypeArgumentBitVector,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        TypeInferenceLevel &TypeInferenceLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequiresInstanceMethodBinding,
        _In_opt_ Location *pCallerInfoLineNumber = NULL
    );

    ExpressionList *
    MatchArguments2
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        Declaration *RepresentTargetInMessages,
        GenericBinding * & GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        ExpressionFlags CallFlags,
        OverloadResolutionFlags OvrldFlags,
        ExpressionList *&CopyOutArguments,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool DisallowParamArrayExpansion,
        bool DisallowParamArrayExactMatch,
        bool &SomeArgumentsBad,
        bool &ArgumentArityBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool &InferenceFailed,
        bool &AllFailedInferenceIsDueToObject,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        TypeInferenceLevel &TypeInferenceLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequiresInstanceMethodBinding
    );

    ExpressionList *
    MatchArguments3
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        Declaration *RepresentTargetInMessages,
        GenericBindingInfo & GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        ExpressionFlags CallFlags,
        OverloadResolutionFlags OvrldFlags,
        ExpressionList *&CopyOutArguments,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool DisallowParamArrayExpansion,
        bool DisallowParamArrayExactMatch,
        bool &SomeArgumentsBad,
        bool &ArgumentArityBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool &InferenceFailed,
        bool &AllFailedInferenceIsDueToObject,
        bool SuppressMethodNameInErrorMessages,
        bool CandidatesIsExtensionMethod,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        TypeInferenceLevel &TypeInferenceLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequiresInstanceMethodBinding
    );

    ExpressionList *
    MatchArguments4
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        Declaration *RepresentTargetInMessages,
        GenericBindingInfo & GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        ExpressionFlags CallFlags,
        OverloadResolutionFlags OvrldFlags,
        ExpressionList *&CopyOutArguments,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool DisallowParamArrayExpansion,
        bool DisallowParamArrayExactMatch,
        bool &SomeArgumentsBad,
        bool &ArgumentArityBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool &InferenceFailed,
        bool &AllFailedInferenceIsDueToObject,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        IReadonlyBitVector * FixedTypeArgumentBitVector,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        TypeInferenceLevel &TypeInferenceLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequiresInstanceMethodBinding,
        bool & UsedDefaultForAnOptionalParameter,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL,
        _In_opt_ Location *pCallerInfoLineNumber = NULL
    );

    bool
    InferTypeArgumentsFromLambdaArgument
    (
        ILTree::Expression *Argument,
        Type *ParameterType,
        Parameter *Param,_In_opt_
        TypeInference* TypeInference,
        _Inout_cap_(TypeArgumentCount) Type **TypeInferenceArguments,
        _Out_opt_cap_(TypeArgumentCount) bool *TypeInferenceArgumentsByAssumption,
        _Out_opt_cap_(TypeArgumentCount) Location *InferredTypeArgumentLocations,
        _Out_opt_ GenericParameter**  InferredTypeParameter, // Needed for error reporting
        unsigned TypeArgumentCount,
        Procedure *TargetProcedure,
        GenericTypeBinding *GenericBindingContext,
        _Out_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity
    );

    // If the target procedure is generic, attempt to infer the type arguments to it from the
    // supplied arguments. The returned value is true iff inference succeeds. If inference occurs,
    // the generic binding context is set to the inferred binding.

    // Inference can occur for either a call or a delegate binding. Either a set of arguments or
    // a list of delegate parameters can be supplied.

    bool
    InferTypeArguments_Unused
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        ILTree::Expression **BoundArguments,
        ILTree::Expression *FirstParamArrayArgument,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags,
        _Inout_ GenericBinding *& GenericBindingContext,
        _Out_ Location *&InferredTypeArgumentLocations,
        _Out_ TypeInferenceLevel &TypeInferenceLevel,
        _Out_ bool &AllFailedInferenceIsDueToObject,
        bool ignoreFirstParameter = false,
        bool SuppressMethodNameInErrorMessages = false,
        bool CandidateIsExtensionMethod = false
    );

    bool
    InferTypeArguments
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        ILTree::Expression **BoundArguments,
        ILTree::Expression *FirstParamArrayArgument,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags,
        GenericBindingInfo & GenericBindingContext,
        _Out_ Location *&InferredTypeArgumentLocations,
        _Out_ TypeInferenceLevel &TypeInferenceLevel,
        _Out_ bool &AllFailedInferenceIsDueToObject,
        bool ignoreFirstParameter = false,
        bool SuppressMethodNameInErrorMessages = false,
        bool CandidateIsExtensionMethod = false,
        bool reportLambdaInferredToBeObject = false,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
    );

    void
    ComputeErrorsForInference
    (
        InferenceErrorReasons InferenceErrorReasons,
        bool NoExplicitTrySpecify,
        _Out_opt_ ERRID *ResultError1,
        _Out_opt_ ERRID *ResultError2,
        _Out_opt_ ERRID *ResultError3
    );

    static bool
    ArgumentTypePossiblyMatchesParamarrayArrayShape
    (
        Type *ArgumentType,
        Type *ParamarrayParameterType
    );

    GenericBinding *
    ValidateGenericArguments
    (
        const Location &TextSpan,
        Declaration *Generic,
        _In_count_(ArgumentCount) Type **BoundArguments,
        Location TypeArgumentLocations[],
        unsigned ArgumentCount,
        GenericTypeBinding *ParentBinding,
        _Out_ bool &ResultIsBad
    );

    ILTree::Expression *
    PassArgumentByref
    (
        ILTree::Expression *Argument,
        Parameter *Param,
        Type *TargetType,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool TargetIsDllDeclare,
        ExpressionList *&CopyOutArguments,
        bool &SomeArgumentsBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool SuppressMethodNameInErrorMessages,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequireInstanceMethodBinding
    );

    ILTree::Expression *
    PassArgumentByval
    (
        ILTree::Expression *Argument,
        Parameter *Param,
        Type *TargetType,
        ExpressionFlags CallFlags,
        bool CheckValidityOnly,
        bool RejectNarrowingConversions,
        bool &SomeArgumentsBad,
        bool &RequiresNarrowingConversion,
        bool &RequiresSomeConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral,
        bool SuppressMethodNameInErrorMessages,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        bool & RequiresUnwrappingNullable,
        bool & RequireInstanceMethodBinding,
        _Inout_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity = NULL
    );

    ILTree::Expression *
    MakeRValueArgument
    (
        ILTree::Expression *Argument,
        BCSYM *TargetType,
        bool &RequiresNarrowingConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral
    );

    void
    EnforceArgumentNarrowing
    (
        ILTree::Expression *Argument,
        Type *OriginalArgumentType,
        ILTree::Expression *OriginalArgument,
        Parameter *Param,
        Type *TargetType,
        bool RejectNarrowingConversions,
        bool NarrowingIsInCopyBack,
        bool NarrowingFromNumericLiteral,
        bool &SomeArgumentsBad,
        bool &RequiresNarrowingConversion,
        bool &AllNarrowingIsFromObject,
        bool &AllNarrowingIsFromNumericLiteral
    );

    void
    DetectArgumentErrors
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        GenericBindingInfo GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        OverloadResolutionFlags OvrldFlags
    );

    void
    DetectArgumentArityErrors
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        GenericBindingInfo GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        OverloadResolutionFlags OvrldFlags,
        ExpressionFlags CallFlags,
        bool &ArgumentArityBad
    );



    void
    DetectArgumentNarrowing
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        GenericBindingInfo GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        OverloadResolutionFlags OvrldFlags
    );

    void
    DetectUnspecificity
    (
        const Location &CallLocation,
        Procedure *TargetProcedure,
        GenericBindingInfo GenericBindingContext,
        ExpressionList *Arguments,
        Type *DelegateReturnType,
        bool SuppressMethodNameInErrorMessages,
        bool CandidatesIsExtensionMethod,
        OverloadResolutionFlags OvrldFlags
    );

    Declaration *
    Semantics::ResolveOverloadedCall
    (
        _In_ const Location &CallLocation,
        _In_ Declaration *OverloadedProcedure,
        _In_ ExpressionList *Arguments,
        _In_opt_ Type *DelegateType,
        _In_opt_ Type *DelegateReturnType,
        _Inout_ GenericBinding *&GenericBindingContext,
        _In_count_(TypeArgumentCount) Type **TypeArguments,
        unsigned TypeArgumentCount,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        _In_ Type *AccessingInstanceType,
        _Out_ bool &ResolutionFailed,
        _Out_ bool &ResolutionIsLateBound,
        _Out_ bool &ResolutionIsAmbiguous
    );


    // *** Begin helper procedures for overload resolution ***

    enum ShadowState
    {
        ShadowStateNone = 0,
        ShadowStateLeft = 1,
        ShadowStateRight = 2
    };

    ShadowState
    ShadowBasedOnParamArrayUsage
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags,
        unsigned LeftParamArrayCount,
        unsigned RightParamArrayCount
    );

    ShadowState
    ShadowBasedOnReceiverTypeSpecificity
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    bool
    IsMethodInGenericClass
    (
        OverloadList * Candidate
    );

    ShadowState
    ShadowBasedOnGenericity
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    ShadowState
    ShadowBasedOnInstanceMethodVsExtensionMethodStatus
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    ShadowState
    ShadowBasedOnTypeInferenceLevel
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    ShadowState
    ShadowBasedOnExtensionMethodScope
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    ShadowState
    ShadowBasedOnSubOfFunction
    (
        OverloadList * Left,
        OverloadList * Right,
        ExpressionList * Arguments,
        OverloadResolutionFlags OvrldFlags
    );

    OverloadList *
    ApplyShadowingSemantics
    (
        OverloadList * Candidates,
        unsigned & CandidateCount,
        ExpressionList *Arguments,
        OverloadResolutionFlags OvrldFlags
    );


    bool
    ContainsNonNarrowingCallableInstanceMethods
    (
        OverloadList * Candidates
    );

    Declaration *
    ResolveOverloading
    (
        _In_ const Location &CallLocation,
        _In_ OverloadList * Candidates,
        unsigned CandidateCount,
        _In_ ExpressionList *Arguments,
        unsigned ArgumentCount,
        _In_opt_ Type *DelegateType,
        _In_opt_ Type *DelegateReturnType,
        _Out_ bool &ResolutionFailed,
        OverloadResolutionFlags OvrldFlags,
        _Out_ bool &ResolutionIsLateBound,
        _Out_ bool &ResolutionIsAmbiguous,
        _Inout_ GenericBinding *&GenericBindingContext,
        _In_opt_ OutputParameter<OverloadList *> MatchingCandidate = OutputParameter<OverloadList *>(NULL),
        bool CandidatesAreOperators = false,
        _In_opt_ OverloadList * InstanceMethodOnlyCandidates = NULL,
        unsigned InstanceMethodOnlyCandidateCount = 0,
        _Out_opt_ bool * pIsBadSingleton  = NULL,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity=NULL
    );

    void
    ReportNoExtensionCallCandidatesError
    (
        unsigned RejectedForArgumentCount,
        unsigned RejectedForTypeArgumentCount,
        _In_opt_ ILTree::ExtensionCallExpression * pExtensionCall,
        ExpressionList *pBoundArguments,
        unsigned boundArgumentsCount,
        OverloadResolutionFlags OvrldFlags,
        const Location & callLocation
    );

    Declaration *
    CollectCandidatesAndResolveOverloading
    (
        _In_ const Location &CallLocation,
        _In_ Declaration *OverloadedProcedure,
        _In_ ExpressionList *Arguments,
        _In_opt_ Type *DelegateType,
        _In_opt_ Type *DelegateReturnType,
        _Inout_ GenericBinding *&GenericBindingContext,
        _In_count_(TypeArgumentCount) Type **TypeArguments,
        unsigned TypeArgumentCount,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        _In_ Type *AccessingInstanceType,
        _Out_ bool &ResolutionFailed,
        _Out_ bool &ResolutionIsLateBound,
        _Out_ bool &ResolutionIsAmbiguous,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    Procedure *
    ResolveExtensionCallOverloading
    (
        _In_opt_ ILTree::ExtensionCallExpression * ExtCall,
        ExpressionList * BoundArguments,
        unsigned BoundArgumentsCount,
        GenericBindingInfo  &GenericBindingContext,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        const Location & CallLocation,
        Type * DelegateType,
        Type * InvokeMethodReturnType,
        bool & SomeCandidatesBad,
        bool & ResultIsExtensionMethod,
        bool & IsBadSingleton,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    ILTree::Expression *
    ResolveExtensionCallOverloadingAndReferToResult
    (
        _In_opt_ ILTree::ExtensionCallExpression * ExtCall,
        ExpressionList *BoundArguments,
        unsigned BoundArgumentsCount,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        const Location &CallLocation,
        bool & ResultIsExtensionMethod
    );

    // Given a set of overloaded procedures, reject those that can't be
    // called with the given arguments. Return one of the callable procedures,
    // or NULL if none are callable.

    OverloadList *
    RejectUncallableProcedures
    (
        const Location &CallLocation,
        _Inout_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags,
        _Inout_ unsigned &CandidateCount,
        _Out_ bool &SomeCandidatesAreGeneric,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    // Given an overloaded procedure, reject it if it can not be called with
    // the given arguments. During this process, also infer the type arguments
    // for generic methods if required to do so.

    void
    RejectUncallableProcedure
    (
        _In_ const Location &CallLocation,
        _In_ OverloadList *Candidate,
        _In_ ExpressionList *Arguments,
        _In_ ILTree::Expression **SavedArguments,
        _In_opt_ Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    OverloadList *
    RejectBetterTypeInferenceLevelOfLaterReleases
    (
        _Inout_ OverloadList *Candidates,
        _Inout_ unsigned &CandidateCount
    );

    OverloadList *
    RejectLessSpecificDelegateRelaxationLevels
    (
        _Inout_ OverloadList *Candidates,
        _Inout_ unsigned &CandidateCount
    );

    // Given a set of overloaded procedures, for those already determined to be
    // uncallable with the given Arguments, report diagnostics describing the reasons
    // why each procedure is not callable.

    static bool
    Semantics::CandidateIsLiftedOperator
    (
        _In_opt_ OverloadList * Candidate
    );


    bool
    IsSingleton
    (
        OverloadList * Candidates
    );

    void
    ReportOverloadResolutionFailure
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        unsigned ErrorId,
        ArgumentDetector Detector,
        CandidateProperty CandidateFilter,
        OverloadResolutionFlags OvrldFlags
        );

    void
    ReportUncallableProcedures
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags
    );

    void
    ReportNarrowingProcedures
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags
    );

    void
    ReportLateBoundConstructors
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags
    );

    void
    ReportUnspecificProcedures
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags
    );

    void
    ReportInferenceFailureProcedures
    (
        const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_opt_ OverloadList *Candidates,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        OverloadResolutionFlags OvrldFlags
    );

    void
    ReportPropertyMismatch
    (
        Declaration *AllegedProperty,
        ExpressionFlags Flags,
        const Location &ErrorLocation
    );

    void
    ReportBadAwaitInNonAsync
    (
        const Location &loc
    );

    // Given a set of overloaded procedures, identify the "most specific" one,
    // and return it. Return NULL if no procedure is most specific.

    OverloadList *
    MostSpecificProcedure
    (
        _Inout_ OverloadList *Candidates,
        _Inout_ unsigned &CandidateCount,
        ExpressionList *Arguments,
        bool LookAtSafeConversions,
        OverloadResolutionFlags OvrldFlags
    );

    bool
    AreProceduresEquallySpecific
    (
        OverloadList *Left,
        OverloadList *Right,
        ExpressionList *Arguments,
        OverloadResolutionFlags OvrldFlags,
        unsigned & LeftParamArrayCount,
        unsigned & RightParamArrayCount
    );

    // Given two procedures, return the one that is "most specific" between the
    // two, or NULL if neither is more specific than the other. The set of parameters
    // examined is determined by the supplied Arguments, and both procedures
    // are assumed callable with the Arguments.

    // The supplied Compare method determines the type of parameter comparison
    // performed--the comparison can be for "Most Specific" or "Least Generic".

    OverloadList *
    MostSpecificProcedureOfTwo
    (
        _In_ OverloadList *Left,
        _In_ OverloadList *Right,
        ExpressionList *Arguments,
        ParameterCompare Compare,
        _Out_ bool &BothLose,
        bool ContinueWhenBothLose = false,
        bool LeftIsExtensionMethod = false,
        bool RightIsExtensionMethod = false,
        OverloadResolutionFlags OvrldFlags = OvrldNoFlags

    );

    Procedure *
    LeastGenericProcedure
    (
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        bool ConsiderReturnTypesToo,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    Procedure *
    LeastGenericProcedure
    (
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        ParameterTypeCompare Compare,
        bool ConsiderReturnTypesToo,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    bool
    CompareGenericityIsSignatureMismatch
    (
        ILTree::Expression *Argument,
        Type *LeftParameterType,
        GenericBindingInfo LeftBinding,
        Type *RightParameterType,
        GenericBindingInfo RightBinding
    );

    // Compare two parameters of two procedures to see if one is less generic than the other.
    // If the parameters do not have identical signatures, then neither is less generic. If
    // the signatures match and one parameter refers to a type parameter of its procedure and
    // the other does not, the parameter that does not is less generic.

    void
    CompareGenericityBasedOnMethodGenericParams
    (
        ILTree::Expression *Argument,
        Parameter *LeftParameter,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        bool ExpandLeftParamArray,
        Parameter *RightParameter,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        bool ExpandRightParamArray,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    void
    CompareTypeGenericityBasedOnMethodGenericParams
    (
        ILTree::Expression *Argument,
        Type *LeftParameterType,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Type *RightParameterType,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    void
    CompareTypeGenericityBasedOnMethodGenericParams
    (
        Type *LeftParameterType,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Type *RightParameterType,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    // Compare two parameters of two procedures to see if one is less generic than the other.
    // If the parameters do not have identical signatures, then neither is less generic. If
    // the signatures match and one parameter refers to a type parameter of its method's parent
    // type and the other does not, the parameter that does not is less generic.

    void
    CompareGenericityBasedOnTypeGenericParams
    (
        ILTree::Expression *Argument,
        Parameter *LeftParameter,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        bool ExpandLeftParamArray,
        Parameter *RightParameter,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        bool ExpandRightParamArray,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    bool
    RefersToGenericTypeParameter
    (
        bool ProcedureIsExtensionMethod,
        Procedure * Procedure,
        Type * Type,
        GenericBindingInfo Binding
    );

    void
    CompareTypeGenericityBasedOnTypeGenericParams
    (
        ILTree::Expression *Argument,
        Type *LeftParameterType,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Type *RightParameterType,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    void
    CompareTypeGenericityBasedOnTypeGenericParams
    (
        Type *LeftParameterType,
        Procedure *LeftProcedure,
        GenericBindingInfo LeftBinding,
        Type *RightParameterType,
        Procedure *RightProcedure,
        GenericBindingInfo RightBinding,
        _Out_ bool &LeftIsLessGeneric,
        _Out_ bool &RightIsLessGeneric,
        _Out_ bool &SignatureMismatch,
        bool LeftIsExtensionMethod,
        bool RightIsExtensionMethod
    );

    // Compare two parameters of two procedures to see if one is more integral specific than the other.

    void
    CompareParameterIntegralSpecificity
    (
        ILTree::Expression *Argument,
        Parameter *LeftParameter,
        Procedure *LeftProcedure,
        GenericBinding *LeftBinding,
        bool ExpandLeftParamArray,
        Parameter *RightParameter,
        Procedure *RightProcedure,
        GenericBinding *RightBinding,
        bool ExpandRightParamArray,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins,
        _Inout_ bool &BothLose
    );

    OverloadList *
    InsertIfMethodAvailable
    (
        Declaration *NewCandidate,
        GenericBindingInfo GenericBindingContext,
        bool ExpandNewCandidateParamArray,
        ExpressionList *ScratchArguments,
        _In_opt_count_(ArgumentCount) ILTree::Expression **ArgumentValues,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        unsigned TypeArgumentCount,
        OverloadResolutionFlags OvrldFlags,
        _Inout_ OverloadList *Candidates,
        _Inout_ unsigned &CandidateCount,
        _Inout_ NorlsAllocator &ListStorage,
        const Location &SourceLocation,
        bool CandidateIsExtensionMethod = false,
        unsigned long PrecedenceLevel = 0,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
    );

    void
    CompareParamarraySpecificity
    (
        bool LeftParamarrayExpanded,
        unsigned LeftParamarrayExpansionCount,
        bool RightParamarrayExpanded,
        unsigned RightParamarrayExpansionCount,
        _Inout_ bool &LeftWins,
        _Inout_ bool &RightWins
    );

    void
    CompareNumericTypeSpecificity
    (
        Type *LeftType,
        Type *RightType,
        _Out_ bool &LeftWins,
        _Out_ bool &RightWins
    );

    Type * Semantics::ExtractReceiverType
    (
        Procedure * pProc,
        GenericBindingInfo binding,
        bool isExtensionMethod
    );

    void
    CompareReceiverTypeSpecificity
    (
        Procedure * pLeftProc,
        GenericBindingInfo leftBinding,
        Procedure * pRightProc,
        GenericBindingInfo rightBinding,
        _Inout_ bool & leftWins,
        _Inout_ bool & rightWins,
        bool leftIsExtensionMethod,
        bool rightIsExtensionMethod
    );

    // Given an overloaded procedure, find the complete set of overloaded
    // procedures (in the same or difference scopes) that contribute to the
    // overloading.

    OverloadList *
    CollectOverloadCandidates
    (
        OverloadList * Candidates,  //The candidate list to attach the results to(can be null)
        Declaration *OverloadedProcedure,
        GenericBinding *GenericBindingContext,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        Type *DelegateReturnType,
        _In_count_(TypeArgumentCount) Type **TypeArguments,
        unsigned TypeArgumentCount,
        ExpressionFlags Flags,
        OverloadResolutionFlags OvrldFlags,
        Type *AccessingInstanceType,
        _Inout_ NorlsAllocator &ListStorage,
        _Out_ unsigned &CandidateCount,
        _Out_ unsigned &RejectedForArgumentCount,
        _Out_ unsigned &RejectedForTypeArgumentCount,
        const Location &SourceLocation,
        _Inout_ bool &SomeCandidatesBad,
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    bool
    VerifyParameterCounts
    (
        unsigned int freeTypeParameterCount,
        Procedure * pProc,
        unsigned int typeArgumentCount,
        unsigned int argumentCount,
        _Inout_ unsigned &RejectedForArgumentCount,
        _Inout_ unsigned &RejectedForTypeArgumentCount,
        _Out_ bool & hasParamArray,
        _Out_ unsigned int & maximumParameterCount
    );

    void
    CollectExtensionMethodOverloadCandidates
    (
        ExtensionCallLookupResult * ExtensionCall,
        ExpressionList *Arguments,
        unsigned ArgumentCount,
        _In_opt_count_(TypeArgumentCount) Type **TypeArguments,
        Location * TypeArgumentLocations,
        unsigned TypeArgumentCount,
        _Inout_ NorlsAllocator &ListStorage,
        _Out_ unsigned &CandidateCount,
        _Out_ unsigned &RejectedForArgumentCount,
        _Out_ unsigned &RejectedForTypeArgumentCount,
        _Out_ unsigned &RejectedBySuperTypeFilterCount,
        const Location &SourceLocation,
        OverloadResolutionFlags OvrldFlags,
        Type * InvokeMethodReturnType,
        ExpressionFlags Flags,
        Type * AccessingInstanceType,
        bool & SomeCandidatesBad,
        OverloadList * & Candidates,
        OverloadList * & InstanceMethodOnlyCandidates,
        unsigned & InstanceMethodOnlyCandidateCount,
        _Inout_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
    );

    Declaration *
    VerifyLateboundCallConditions
    (
        _In_ const Location &CallLocation,
        _In_z_ Identifier *OverloadedProcedureName,
        _In_opt_ Type *DelegateType,
        _In_ OverloadList *Candidates,
        _In_count_(ArgumentCount) ExpressionList *Arguments,
        unsigned ArgumentCount,
        _In_opt_ Type *DelegateReturnType,
        _Out_ bool &ResolutionFailed,
        _Out_ bool &ResolutionIsLateBound,
        OverloadResolutionFlags OvrldFlags
    );


    // Here are some handy debugging functions for diagnostics
#if DEBUG
    wchar_t *DelegateRelaxationLevelToDebugString(DelegateRelaxationLevel level);
    wchar_t *TypeInferenceLevelToDebugString(TypeInferenceLevel level);
    wchar_t *CandidateToDebugString(_In_ Declaration *Candidate, _In_ bool showDetail=false);
    wchar_t *CandidateToDebugString(_In_ OverloadList *Candidate, _In_ bool showDetail=false);
    wchar_t *TypeToDebugString(_In_ Type *Type);
#endif


    // Return tree iff Property, which may be an overloading consisting of Get and
    // assignment methods, has exactly one visible Get method, and that method has no
    // parameters.

    bool
    IsSimplePropertyGet
    (
        Declaration *Property,
        GenericBinding *PropertyGenericBindingContext,
        Type *AccessingInstanceType,
        bool PropertyIsTargetOfAssignment,
        const Location &SourceLocation
    );

    static void AddAsyncSubArgumentAmbiguity(
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity,
        _In_ ILTree::Expression *Argument,
        AsyncSubAmbiguityFlags AsyncSubArgumentAmbiguity
    );

    void ReportAndDeleteAsyncSubArgumentListAmbiguity(
        _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity,
        bool MakeReport
    );


    // *** End helper procedures for overload resolution ***

    Symbol *
    InterpretName
    (
        _In_z_ Identifier *Name,
        // Lookup is the scope in which to perform the name lookup.
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        NameFlags Flags,
        Type *AccessingInstanceType,
        // SourceLocation is used for error reporting.
        const Location &SourceLocation,
        // NameIsBad is set true if the name interpretation fails and issues
        // appropriate diagnostics.
        _Out_ bool &NameIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_ ImportTrackerEntry *pImportTrackerEntry = NULL      // this should only be non-null when this function calls itself
    );

    bool
    AllowsLateBinding
    (
        ILTree::Expression *BaseReference
    );

    Symbol *
    InterpretName
    (
        _In_ ParseTree::Name *Name,
        // Lookup is the scope in which to perform the name lookup.
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        NameFlags Flags,
        Type *AccessingInstanceType,
        // NameIsBad is set true if the name interpretation fails and issues
        // appropriate diagnostics.
        _Out_ bool &NameIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext,
        int GenericTypeArity
    );

    Type *
    InterpretTypeName
    (
        ParseTree::Type *Type,
        _Out_ bool &TypeIsBad
    );

    Type *
    InterpretTypeName
    (
        ParseTree::Type *Type,
        _Out_ bool &TypeIsBad,
        TypeResolutionFlags Flags
    );

    virtual 
    void
    CreateNameLookupGenericBindingContext
    (
        Declaration *Member,
        Symbol *TypeOrMethodFoundIn,
        _Out_opt_ GenericBinding **GenericBindingContext,
        _In_opt_ Type *AccessingInstanceType = NULL
    );

    virtual 
    GenericBinding* CreateMeGenericBinding( _In_ ClassOrRecordType* pMeType );

    virtual
    Type* ResolveTypeNameGenericBinding( _In_ Type* pType );

    void
    UpdateNameLookupGenericBindingContext
    (
        Symbol * NameLookupResult,
        Type  * PossiblyGenericType,
        GenericBinding ** GenericBindingContext
    );

    void
    UpdateNameLookupGenericBindingContext
    (
        Type *PossiblyGenericType,
        _Inout_opt_ GenericBinding **GenericBindingContext
    );


    inline static void
    InitializeNameLookupGenericBindingContext
    (
        _Out_opt_ GenericBinding **GenericBindingContext
    )
    {
        if (GenericBindingContext)
        {
            *GenericBindingContext = NULL;
        }
    }

    Declaration *
    ApplyIdenticalNamesRule
    (
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        Member *MemberBinding,
        NameFlags Flags,
        Type *AccessingInstanceType,
        const Location &SourceLocation
    );

/*    Namespace *
    LookupImportedNamespace
    (
        STRING *NamespaceName[];
        int NumberofNames;
    )
*/

    Declaration *
    LookupDefaultProperty
    (
        Type *LookupClassInterfaceOrGenericParam,
        const Location &SourceLocation,
        _Inout_ bool &PropertyIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext
    );

    Declaration *
    LookupDefaultPropertyInClass
    (
        Type *LookupClass,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Out_ bool &PropertyIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext
    );

    Declaration *
    LookupDefaultPropertyInInterface
    (
        Type *LookupInterface,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Out_ bool &TypeIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext
    );

    void
    LookupDefaultPropertyInBaseInterface
    (
        Type *LookupBaseInterface,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Out_ bool &PropertyIsBad,
        _Inout_ Declaration *&Result,
        _Inout_ GenericBinding *&ResultGenericBindingContext
    );

    Declaration *
    LookupDefaultPropertyInGenericTypeParameter
    (
        GenericParameter *TypeParameter,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Inout_ bool &PropertyIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext
    );

    // If within a With statement, produce an expression suitable for use
    // as a base reference in .name or !name references. Otherwise, produce
    // a diagnostic and return a bad expression.

    ILTree::Expression *
    EnclosingWithValue
    (
        const Location &ReferringLocation,
        ExpressionFlags Flags = ExprNoFlags
    );

    bool
    CanMakeRValue
    (
        ILTree::Expression * Input
    );

    ILTree::Expression *
    MakeRValue
    (
        ILTree::Expression *Input,
        BCSYM *TargetType = NULL
    );

    ILTree::Expression *
    MakeAddress
    (
        ILTree::Expression *Input,
        bool SuppressReadonlyLValueCapture
    );

    ILTree::Expression *
    MakeMissingArgument
    (
        const Location &CallLocation
    );

    virtual 
    bool IsVBEESemantics()
    {
        return false;
    }

    virtual 
    bool BypassConstructorLogic()
    {
        return false;
    }

    virtual
    bool IgnoreConditionalAttribute()
    {
        return false;
    }
	
    virtual
    bool IgnoreCallerInfoAttribute()
    {

        if (m_Procedure != NULL && m_Procedure->IsSyntheticMethod())
        {
             SyntheticKind SyntheticKind = m_Procedure->PSyntheticMethod()->GetSyntheticKind();

             switch (SyntheticKind)
             {
                case SYNTH_New:
                case SYNTH_SharedNew:
                    // Field initializations are inserted to constructor,
                    // but we don't want to skip caller info for field initialization.
                    if (m_FieldInitializerContext != NULL)
                    {
                        return false;
                    }
                    __fallthrough;
                default:
                    return true;
             }
        }

        if ( m_SynthesizingBaseCtorCall || m_IsCreatingRelaxedDelegateLambda || (m_Procedure && m_Procedure->IsMyGenerated()))
        {
            return true;
        }
                
        return false;
    }

    // If it is valid to use Me to refer to a member of ReferencedClass,
    // synthesize a Me expression. Otherwise, produce a diagnostic and return a
    // Bad expression.

    virtual
    ILTree::Expression *
    SynthesizeMeReference
    (
        const Location &ReferringLocation,
        Type *ReferencedClassOrInterface,
        bool SuppressMeSynthesis,
        bool ReportError = true,
        unsigned * pErrorID = NULL //should be non null if ReportError is false
    );

    // synthesize the call to the default instance property of a class if the class has one
    ILTree::Expression*
    CheckForDefaultInstanceProperty(
        const Location &ReferringLocation,
        ILTree::Expression *BaseReference,
        _In_z_ STRING* MyBaseName,
        ExpressionFlags Flags,
        bool MangleName
    );

    // return the string with the fully qualified name of the My* default instace of a class if any
    STRING *
    GetDefaultInstanceBaseNameForMyGroupMember
    (
        BCSYM_Class *Class, //in
        bool    *MangleName //out
    );

    // Turn an expression of a value type into a valid base reference for use in
    // referring to a member of the value type (or a base class of the value type).

    ILTree::Expression *
    MakeValueTypeOrTypeParamBaseReferenceToProcedure
    (
        Declaration *ReferencedMember,
        ILTree::Expression *BaseReference,
        bool SuppressReadonlyLValueCapture,
        bool ConstrainValueTypeReference
    );

    ILTree::Expression *
    MakeValueTypeOrTypeParamBaseReferenceToField
    (
        Declaration *ReferencedMember,
        ILTree::Expression *BaseReference,
        GenericBinding * GenericBindingContext,
        bool SuppressReadonlyLValueCapture,
        bool ConstrainValueTypeReference
    );

    ILTree::Expression *
    MakeValueTypeOrTypeParamBaseReferenceInternal
    (
        Declaration *ReferencedMember,
        ILTree::Expression *BaseReference,
        GenericBinding * GenericBindingContext,
        bool SuppressReadonlyLValueCapture,
        bool ConstrainValueTypeReference
    );

    // Synthesize an expression that refers to Referenced. Depending on the
    // properties of Referenced, the resulting expression can be a normal
    // symbol reference, a constant value, a function call, Bad, or
    // who knows what.

    ILTree::Expression *
    ReferToSymbol
    (
        const Location &ReferringLocation,
        Symbol *Referenced,
        typeChars TypeCharacter,
        ILTree::Expression *BaseReference,
        GenericBinding *GenericBindingContext,
        ExpressionFlags Flags
    );

    //This function does not free memory, but simply
    //invokes a destructor on the pointer it's given. This is
    //because this function is designed to run on data
    //that has been allocated on the NorlsAllocator. It primary purpose
    //is to be used as a "destruction hook" that will be run when
    //a norls allocator region containing pv is either rolled back or destroyed.
    static void DestroyErrorTable(void * pv);

    ILTree::Expression *
    ReferToExtensionMethod
    (
        const Location &ReferringLocation,
        ExtensionCallLookupResult * UnfilteredCall,
        ILTree::Expression * BaseReference,
        ExpressionFlags Flags,
        typeChars TypeCharacter
    );

    ILTree::Expression *
    ReferToExtensionMethod
    (
        const Location &ReferringLocation,
        ILTree::ExtensionCallExpression * ExtensionCall,
        ExpressionFlags Flags,
        typeChars TypeCharacter
    );

    bool
    HasAccessibleZeroArgumentInstanceMethods
    (
        ExtensionCallLookupResult * LookupResult
    );

    ILTree::Expression *
    ReferToProcByName
    (
        const Location &ReferringLocation,
        Container *Container,
        _In_ STRING *ProcName,
        ILTree::Expression *BaseReference,
        ExpressionFlags Flags
    );

    ConstantValue
    GetConstantValue
    (
        const Location &ReferringLocation,
        SymbolicValue *Value
    );

    ILTree::Expression *
    ReferToConstant
    (
        const Location &ReferringLocation,
        Constant *Referenced,
        GenericBinding *GenericBindingContext
    );

    // Determine whether a result type is compatible with a type character,
    // and issue a diagnostic if not so.
    void
    VerifyTypeCharacterConsistency
    (
        const Location &ErrorLocation,
        Type *ResultType,
        typeChars TypeCharacter
    );

    // For an expression used as the base of a reference, determine the type to be
    // used for checking the accessibility of the reference.

    Type *
    InstanceTypeOfReference
    (
        ILTree::Expression *Instance
    );

    // Determine whether or not Result is accessible from the current context, or, if
    // Result is an overloaded procedure or property, if any one of the overloaded
    // members is accessible.

    bool
    CanBeAccessible
    (
        Symbol *Result,
        GenericBinding *ResultGenericBindingContext,
        Type *AccessingInstanceType
    );

    // If Result is Bad, quietly set ResultIsBad. Otherwise, issue a
    // diagnostic and set ResultIsBad if Result is not accessible from
    // the current context. Return the result of chasing aliases through Result.
    //
    // If Result is an overloaded procedure, or an alias to one, do not check
    // accessibility, and return Result without chasing through aliases.
    // (If overloading is involved, accessibility can't be checked until the
    // overloading is resolved.)

    Symbol *
    CheckAccessibility
    (
        Symbol * Result,
        GenericBinding *ResultGenericBindingContext,
        const Location &SourceLocation,
        NameFlags Flags,
        Type *AccessingInstanceType,
        _Inout_ bool &ResultIsBad
    );

    Declaration *
    CheckNamedRootAccessibility
    (
        Declaration *Result,
        GenericBinding *ResultGenericBindingContext,
        const Location &SourceLocation,
        NameFlags Flags,
        Type *AccessingInstanceType,
        _Inout_ bool &ResultIsBad
    );

    void
    CheckNamedRootAccessibilityCore
    (
        Declaration *Result,
        GenericBinding *ResultGenericBindingContext,
        const Location &SourceLocation,
        Type *AccessingInstanceType,
        _Inout_ bool &ResultIsBad
    );

    ILTree::ResumableKind GetLocalResumableKind(); // gets the resumable info of the lambda/method we're currently in
    void SetLocalResumableInfo(ILTree::ResumableKind kind, Type* resumableGenericType);
    Type* GetLocalReturnType();              // the return type of the current lambda/procedure we're in
    Type* GetTypeForLocalReturnStatements(); // the type that "Return" statements are expected to return from the current lambda/procedure we're in (will be different from the above in case of resumable methods)
    Type* GetTypeForLocalYieldStatements();  // the type that "Yield" statements are expected to return

    void IndicateLocalResumable();           // says that the lambda/method we're currently in is a resumable, and reports any errors associated with that

    void SetLocalSeenAwait();                // says that the lambda/method we're in has seen an await expression

    static Location GetLocationForErrorReporting(_In_ ILTree::Expression *pExpr); // Normally the same as pExpr->Loc, but for lambdas it's smaller

public:

    static void
    Semantics::CheckResultTypeAccessible
    (
        Member           *Result,
        Location         *SourceLocation,
        Container        *Context,
        Symbols          &SymbolCreator,
        CompilerHost     *CompilerHost,
        ErrorTable       *Errors
    );

protected:

    static bool
    IsResultTypeAccessible
    (
          Type            *ResultType,
          Container       *Context,
          Symbols         &SymbolCreator,
          CompilerHost    *CompilerHost
    );

    bool
    IsOrInheritsFrom
    (
        Type *Derived,
        Type *Base
    );

    Type *
    GetBaseClass
    (
        Type *Derived
    );


    // Perform lookup in a particular scope, including visiting base classes
    // or merged namespaces as appropriate.

    Symbol *
    LookupInScope
    (
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        _In_z_ Identifier *Name,
        NameFlags Flags,
        Type *AccessingInstanceType,
        unsigned BindspaceMask,
        bool IgnoreImports,
        bool IgnoreModules,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );

    // Look up a name in a class, examining base classes as necessary. If a name
    // is declared inaccessibly in a derived class, but accessibly in a base class,
    // the declaration in the base class is returned.

    Symbol *
    LookupInClass
    (
        ClassOrRecordType *Lookup,
        _In_z_ Identifier *Name,
        Type *AccessingInstanceType,
        unsigned BindspaceMask,
        _Out_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        NameFlags Flags
    );

protected:
    bool
    IsExtensionMethodExistsCacheValid()
    {
        Container *pContainer = ContainingContainer();

        if (pContainer != NULL)
        {
            SourceFile * pSourceFile = m_SourceFile ? m_SourceFile : pContainer->GetSourceFile();
        
            return pSourceFile && pSourceFile->IsImportedExtensionMethodMetaDataLoaded();
        }
        
        return false;
    }

    bool 
    ExtensionMethodExists(CompilerProject *pProject, _In_z_ STRING *pName);

    Symbol *
    LookForExtensionMethods
    (
        _In_z_ Identifier *Name,
        Declaration * Result,
        Type * AccessingInstanceType,
        BCSYM_GenericBinding **GenericBindingContext,
        NameFlags Flags
    );

    void
    ReportMethodCallError
    (
        bool SuppressMethodNameInErrorMessages,
        bool CandidateIsExtensionMethod,
        bool ErrorIsForDelegateBinding,
        RESID SuppressMethodNameErrorID,
        RESID RegularMethodCallErrorID,
        RESID ExtensionCallErrorID,
        RESID DelegateBindingErrorID,
        const Location & Location,
        _In_opt_z_ STRING * Substitution1,
        Declaration * TargetToUseForSubstitutions2And3,
        IReadonlyBitVector * FixedTypeArgumentBitVector,
        GenericBindingInfo GenericBindingContext
    );


    bool
    IsInterfaceExtension
    (
        Procedure * pProc,
        _In_opt_ PartialGenericBinding * pPartialGenericBinding
    );

    Type *
    GetTypeBeingExtended
    (
        Procedure * pProc,
        _In_opt_ PartialGenericBinding * pPartialGenericBinding
    );


    ExtensionCallLookupResult *
    LookForExtensionMethods
    (
        _In_z_ Identifier * pName,
        Container * pContextOfCall
    );

    ExtensionMethodLookupCacheEntry *
    CreateExtensionMethodCacheEntry();


    Declaration *
    ImmediateExtensionMethodLookup
    (
        Container * pContainerToLookIn,
        _In_z_ Identifier * pNameToLookFor
    );

    PartialGenericBinding *
    CreatePartialGenericBinding
    (
        Procedure * pProc
    );

    inline bool FlagsAllowExtensionMethods(NameFlags Flags)
    {
        return
            !
            (
                Flags &
                (
                    NameSearchIgnoreExtensionMethods |
                    NameSearchTypeReferenceOnly |
                    NameSearchAttributeReference |
                    NameSearchConditionalCompilation |
                    NameSearchEventReference
                )
            );
    }

    inline bool FlagsAllowObjectMethodsOnInterfaces(NameFlags Flags)
    {
        return
            !
            (
                Flags &
                (
                    NameSearchIgnoreObjectMethodsOnInterfaces |
                    NameSearchTypeReferenceOnly |
                    NameSearchAttributeReference |
                    NameSearchConditionalCompilation |
                    NameSearchEventReference
                )
            );
    }


    // Look up a name in the base classes of a class without applying any
    // accessibility checks.

    static Declaration *
    LookupInBaseClasses
    (
        ClassOrRecordType *Lookup,
        _In_z_ Identifier *Name,
        unsigned BindspaceMask
    );

    Symbol *
    LookupInBaseInterfaces
    (
        InterfaceType *Lookup,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
        Declaration * Result
    );

    void
    LookupInBaseInterface
    (
        Type *PossibleBaseInterface,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Inout_ Declaration *&Result,
        _Inout_ GenericBinding *&ResultGenericBindingContext,
        int GenericTypeArity,
        _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces
    );

    Symbol *
    LookupInGenericTypeParameter
    (
        GenericParameter *TypeParameter,
        _In_z_ Identifier *Name,
        Type *AccessingInstanceType,
        unsigned BindspaceMask,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
        NameFlags Flags
    );

#if HOSTED
public:
    Declaration *
    LookupInNamespace
    (
        Namespace *Lookup,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        _Inout_ bool &NameIsBad,
        _Out_opt_ GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Out_opt_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );
protected:
#endif

    Declaration *
    LookupInNamespace
    (
        Namespace *Lookup,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        bool IgnoreImports,
        bool IgnoreModules,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Out_opt_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );

#if 0
    Declaration *
    LookupInNamespaceNoCaching
    (
        Namespace *Lookup,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        bool IgnoreImports,
        bool IgnoreModules,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );
#endif

    Declaration *
    LookupInNamespaceNoCaching_WithMergedHash
    (
        Namespace *Lookup,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        bool IgnoreImports,
        bool IgnoreModules,
        const Location &SourceLocation,
        bool &NameIsBad,
        BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        ImportTrackerEntry *pImportTrackerEntry = NULL
    );

    static bool EnsureNamespaceRingHashTableLoaded();

#if HOSTED
public:
#endif

    static bool
    IsMemberSuitableBasedOnGenericTypeArity
    (
        Declaration *Member,
        int GenericTypeArity
    );

#if HOSTED
protected:
#endif

    static bool
    ContinueMemberLookupBasedOnGenericTypeArity
    (
        Declaration *Member,
        int GenericTypeArity
    )
    {
        return (GenericTypeArity == -1 && Member->IsType());
    }

    static bool
    ResolveGenericTypesByArity
    (
        int GenericTypeArity,
        Declaration *Challenger,
        GenericBinding *ChallengerGenericBindingContext,
        _Inout_ Declaration *&Current,
        _Out_opt_ GenericBinding **CurrentGenericBindingContext
    );

    Declaration *
    LookupInModulesInNamespace
    (
        Namespace *Lookup,
        _In_z_ Identifier *Name,
        unsigned BindspaceMask,
        const Location &SourceLocation,
        _Out_ bool &NameIsBad,
        int GenericTypeArity
    );

    void LookupInModulesInImportedTarget
    (
        _In_ ImportedTarget *Import,
        _In_z_ Identifier     *Name,
        unsigned        BindspaceMask,
        const Location &SourceLocation,
        _Out_ bool           &NameIsBad,
        _Inout_ bool           &AmbiguityExists,
        StringBuffer   *AmbiguityErrorsText,
        int             GenericTypeArity,
        _Inout_ Declaration   **Result
    );

    bool LookupInImportedTarget
    (
        _In_ ImportedTarget  *Import,
        _In_z_ Identifier      *Name,
        NameFlags        Flags,
        unsigned         BindspaceMask,
        const Location  &SourceLocation,
        int              GenericTypeArity,
        _Inout_ bool            &NameIsBad,
        _Inout_ bool            &AmbiguityExists,
        StringBuffer    *AmbiguityErrorsText,
        _Out_ bool            &BoundToAlias,
        _Inout_ bool            *NameAmbiguousAcrossBaseInterfaces,
        _Inout_ GenericBinding **ResultGenericBindingContext,
        _Inout_ Declaration    **Result
    );

    Declaration *
    LookupInImports
    (
        _In_z_ ImportedTarget *ImportsList,
        _In_z_ Identifier *Name,
        NameFlags Flags,
        unsigned BindspaceMask,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );

    Symbol *
    LookupName
    (
        Scope *Lookup,
        GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
        _In_z_ Identifier *Name,
        NameFlags Flags,
        Type *AccessingInstanceType,
        const Location &SourceLocation,
        _Inout_ bool &NameIsBad,
        _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
        int GenericTypeArity,
        _Inout_ bool *NameAmbiguousAcrossBaseInterfaces = NULL,
        _Out_opt_ ImportTrackerEntry *pImportTrackerEntry = NULL
    );

    // Test to see if a namespace is from a project that is or is referenced
    // by the project being compiled.
    bool
    NamespaceIsAvailableToCurrentProject
    (
        Namespace *NamespaceToTest
    );

    // Test to see if a namespace is from a project that is or is referenced
    // by the project being compiled.
    bool
    DeclarationIsAvailableToCurrentProject
    (
        Declaration *DeclarationToTest
    );

    Variable *
    CreateImplicitDeclaration
    (
        _In_z_ Identifier *Name,
        typeChars TypeCharacter,
        Location *loc,
        ExpressionFlags Flags,
        bool lambdaMember=false
    );

    Type *
    ResolveBinaryOperatorResultType
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Left,
        _In_ ILTree::Expression *Right,
        _Out_ bool &ResolutionFailed,
        _Out_ Procedure *&OperatorMethod,
        _Out_ GenericBinding *&OperatorMethodGenericContext,
        _Out_ bool &LiftedNullable
    );

    Type *
    ResolveUnaryOperatorResultType
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Operand,
        _Out_ bool &ResolutionFailed,
        _Out_ Procedure *&OperatorMethod,
        _Out_ GenericBinding *&OperatorMethodGenericContext,
        _Out_ bool &LiftedNullable
    );

    void
    ReportUndefinedOperatorError
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Left,
        _In_ ILTree::Expression *Right,
        _Out_ bool &ResolutionFailed
    );

    void
    ReportUndefinedOperatorError
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Operand,
        _Out_ bool &ResolutionFailed
    );

    void
    ReportUndefinedOperatorError
    (
        UserDefinedOperators Operator,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Left,
        _In_opt_ ILTree::Expression *Right,
        _Out_ bool &ResolutionFailed
    );

    struct OperatorInfo : CDoubleLink<OperatorInfo>
    {
        Operator *Symbol;
        GenericTypeBinding *GenericBindingContext;
    };

    void
    ScanMembers
    (
        UserDefinedOperators Operator,
        Type *Class,
        _Out_ bool &KeepClimbing,
        _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
        _Inout_ NorlsAllocator &Scratch
    );

    void
    CollectOperators
    (
        UserDefinedOperators Operator,
        Type *Type1,
        Type *Type2,
        _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
        _Inout_ NorlsAllocator &Scratch
    );

    /* bool
    IsConvertible
    (
        Type *TargetType,
        Type *SourceType
    );

    void
    RejectInapplicableOperators
    (
        UserDefinedOperators Operator,
        Type *LeftType,
        Type *RightType,
        CDoubleList<OperatorInfo> &OperatorSet
    ); */

    UserDefinedOperators
    MapToUserDefinedOperator
    (
        BILOP Opcode
    );

    Type *
    ResolveUserDefinedOperator
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Left,
        _In_ ILTree::Expression *Right,
        _Out_ bool &ResolutionFailed,
        _Out_ bool &ResolutionIsLateBound,
        _Out_ Procedure *&OperatorMethod,
        _Out_ GenericBinding *&OperatorMethodGenericContext
    );

    Type *
    ResolveUserDefinedOperator
    (
        BILOP Opcode,
        const Location &ExpressionLocation,
        _In_ ILTree::Expression *Operand,
        _Out_ bool &ResolutionFailed,
        _Out_ bool &ResolutionIsLateBound,
        _Out_ Procedure *&OperatorMethod,
        _Out_ GenericBinding *&OperatorMethodGenericContext
    );

    ILTree::Expression *
    AccessDefaultProperty
    (
        const Location &TextSpan,
        ILTree::Expression *Input,
        typeChars TypeCharacter,
        ExpressionFlags Flags
    );

    // Convert assumes the validity of a conversion
    // from the type of Input to ResultType.

    ILTree::Expression *
    ConvertUsingConversionOperator
    (
        ILTree::Expression *Source,
        Type *TargetType,
        Procedure *OperatorMethod,
        GenericBinding *OperatorMethodGenericContext,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    ConvertUsingConversionOperatorWithNullableTypes
    (
        ILTree::Expression *Source,
        Type *TargetType,
        Procedure *OperatorMethod,
        GenericBinding *OperatorMethodGenericContext,
        ExpressionFlags Flags
    );

    ILTree::Expression *
    Convert
    (
        ILTree::Expression *Input,
        Type *TargetType,
        ExpressionFlags Flags,
        ConversionClass ConversionClassification
    );

    ILTree::Expression *
    ConvertFloatingValue
    (
        double SourceValue,
        Type *TargetType,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ConvertDecimalValue
    (
        DECIMAL SourceValue,
        Type *TargetType,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ConvertIntegralValue
    (
        Quadword SourceValue,
        Type *SourceType,
        Type *TargetType,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ConvertStringValue
    (
        _In_z_ WCHAR *Spelling,
        Type *TargetType,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ConvertWithErrorChecking
    (
        ILTree::Expression *Input,
        Type *TargetType,
        ExpressionFlags Flags,
        bool SuppressMethodNameInErrorMessages = false,
        bool * pRequiresUnwrappingNullable = NULL,
        _In_opt_ bool IgnoreOperatorMethod = false
    );

    void AssertLanguageFeature(unsigned feature,_Inout_ Location const &Loc);
    void ReportSyntaxErrorForLanguageFeature(
        unsigned Errid,
        _In_ const Location &ErrLocation,
        unsigned Feature,
        _In_opt_z_ const WCHAR *wszVersion
    );

    ILTree::Expression *
    ConvertArrayLiteral
    (
        ILTree::ArrayLiteralExpression * pLiteral,
        BCSYM * pTargetType
    );

    ILTree::Expression *
    ConvertArrayLiteral
    (
        ILTree::ArrayLiteralExpression * pLiteral,
        BCSYM * pTargetType,
        bool &RequiresNarrowingConversion,
        bool &NarrowingFromNumericLiteral
    );
    
    ILTree::ExpressionWithChildren *
    ConvertArrayLiteralElements
    (
        ILTree::Expression * pLiteral,
        BCSYM * pTargetType,
        bool &RequiresNarrowingConversion,
        bool &NarrowingFromNumericLiteral
    );    

    ILTree::ExpressionWithChildren *
    ConvertArrayLiteralElementsHelper
    (
        ILTree::Expression * pLiteral,
        BCSYM * pTargetType,
        int &RequiresNarrowingConversion,
        int &NarrowingFromNumericLiteral
    );    


    ILTree::Expression *
    ConvertWithErrorChecking
    (
        ILTree::Expression *Input,
        Type *TargetType,
        ExpressionFlags Flags,
        Parameter *CopyBackConversionParam,
        bool &RequiresNarrowingConversion,
        bool &NarrowingFromNumericLiteral,
        bool SuppressMethodNameInErrorMessages,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        bool & RequiresUnwrappingNullable,
        _Inout_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity = NULL,
        _In_opt_ bool IgnoreOperatorMethod = false
    );

    bool
    MakeVarianceConversionSuggestion
    (
        ILTree::Expression* &Input,
        Type *TargetType,
        ConversionClass ConversionClassification
    );

    ILTree::UnboundLambdaExpression *
    InterpretLambdaExpression
    (
        _In_ ParseTree::LambdaExpression *Lambda,
        ExpressionFlags InterpretBodyFlags
    );

    Parameter *
    InterpretLambdaParameters
    (
        ParseTree::ParameterList    *LambdaParameters,
        DECLFLAGS    MethodFlags = DECLF_NoFlags
    );

    ILTree::Expression *
    Semantics::InterpretUnboundLambdaBinding
    (
        _Inout_ ILTree::UnboundLambdaExpression *UnboundLambda,
        Type *DelegateType,
        bool ConvertToDelegateReturnType,
        _Inout_ DelegateRelaxationLevel &DelegateRelaxationLevel,
        bool InferringReturnType = false,
        _Out_opt_ bool *pRequiresNarrowingConversion = NULL,
        _Out_opt_ bool *pNarrowingFromNumericLiteral = NULL,
        bool GetLambdaTargetTypeFromDelegate = false,
        bool inferringLambda = false,
        _Out_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity = NULL,
        _Out_opt_ bool *pDroppedAsyncReturnTask = NULL
    );

    ILTree::LambdaExpression *
    InterpretUnboundLambda
    (
        _In_ ILTree::UnboundLambdaExpression *UnboundLambda,
        Type * ReturnTypeOfLambda = NULL,
        _Inout_opt_ DelegateRelaxationLevel *RelaxationLevel = NULL,
        bool inferringLambda = false,
        _Out_opt_ bool * pAsyncLacksAwaits = NULL
    );

    bool
    Semantics::IsOrCouldBeConvertedIntoLambda
    (
        ILTree::Expression * Expr
    );

    void
    Semantics::FixLambdaExpressionForClosures
    (
        ILTree::LambdaExpression * Expr
    );

    class LambdaBodyInterpretNode : public LambdaBodyInterpreter
    {
    protected: 
        Semantics * m_Semantics;
        ExpressionFlags m_Flags;
        Type * m_TargetBodyType;

        // WARNING: this m_OriginalResultType records the original return type of a single-line expression
        // lambda. For multi-line lambdas it is meaningless.
        // The point of it is to help calculate delegate relaxation levels (see Dev10#524269).
        // For multi-line lambdas, they store the same thing in the public field m_ReturnRelaxationLevel.
        // It has to be public because IntrepretReturnStatement is the person who sets it.
        // Consider: we should scrap m_OriginalResultType entirely, so that both single and multi-line
        // lambdas use m_ReturnRelaxationLevel.
        Type * m_OriginalResultType;

    public:
        Type* GetOriginalResultType() {return m_OriginalResultType;}
        DelegateRelaxationLevel m_ReturnRelaxationLevel; // WARNING: see above. Only used for multi-line lambdas.
        bool m_SeenAwait;
        bool m_SeenReturnWithOperand;
    };

    class LambdaBodyInterpretExpression : public LambdaBodyInterpretNode
    {
    private: 
        ParseTree::Expression * m_Body;

    public:
        LambdaBodyInterpretExpression
        (
            _In_opt_ Semantics * semantics,
            ParseTree::Expression * body,
            bool IsAsyncKeywordUsed,
            ExpressionFlags flags,
            Type * TargetBodyType = NULL
        );
        bool m_IsAsyncKeywordUsed;

        ILTree::ResumableKind GetResumableKind() {return ILTree::NotResumable;}
        Type* GetResumableGenericType() {return NULL;}
    protected:
        ILTree::ILNode * DoInterpretBody();
    };

    class LambdaBodyInterpretStatement : public LambdaBodyInterpretNode
    {
    private: 
        ParseTree::LambdaBodyStatement * m_Body;
        bool m_IsSingleLine;

    public:
        ILTree::StatementLambdaBlock *m_Tree; // created in DoInterpretBody
        bool m_IsAsyncKeywordUsed;
        bool m_IsIteratorKeywordUsed;
        bool m_FunctionLambda;

        // Following two fields are populated inside DoInterpretBody by IndicateLocalResumable, based on the
        // return-type that we've been given and whether we had Async/Iterator keywords used
        ILTree::ResumableKind m_ResumableKind;   // for whether this lambda was iterator/async
        Type *m_ResumableGenericType;    // for Task<T>, IEnumerable<T>, IEnumerator<T>, this is that T.

        LambdaBodyInterpretStatement
        (
            _In_opt_ Semantics * semantics,
            ParseTree::LambdaBodyStatement * body,
            ExpressionFlags flags,
            bool functionLambda,
            bool isSingleLine,
            bool isAsyncKeywordUsed,
            bool isIteratorKeywordUsed,
            Type * TargetBodyType = NULL
        );  

        ILTree::ResumableKind GetResumableKind() {return m_ResumableKind;}
        Type* GetResumableGenericType() {return m_ResumableGenericType;}
        bool IsSingleLine(){ return m_IsSingleLine; }
    protected:    
        ILTree::ILNode * DoInterpretBody();

    };
    
    ILTree::Expression *
    ConvertToDelegateType
    (
        _In_ ILTree::LambdaExpression *Lambda,
        Type *DelegateType,
        bool ConvertToDelegateReturnType,
        DelegateRelaxationLevel &DelegateRelaxationLevel,
        _Out_opt_ bool *pRequiresNarrowingConversionForReturnValue = NULL,
        _Out_opt_ bool *pNarrowingFromNumericLiteral = NULL
    );

    void
    ReportLambdaBindingMismatch
    (
        Type *DelegateType,
        const Location &Loc,
        bool isFunctionLambda,
        bool WouldHaveMatchedIfOptionStrictWasOff = false
    );

    // ----------------------------------------------------------------------------
    // Check if the type is a generic binding for an anonymous type.
    // ----------------------------------------------------------------------------

    static bool
    IsAnonymousType
    (
        Type* candidate
    );

    // ----------------------------------------------------------------------------
    // Conversions to expression trees.
    // ----------------------------------------------------------------------------

    ILTree::Expression *
    ConvertLambdaToExpressionTree
    (
        ILTree::Expression *Input,
        ExpressionFlags Flags,
        Type *TargetType
    );

    // ----------------------------------------------------------------------------
    // Check whether we should convert this lhs/rhs pair to an expression tree.
    // ----------------------------------------------------------------------------

    bool
    IsConvertibleToExpressionTree
    (
        Type* TargetType,
        ILTree::Expression* Input,
        ILTree::LambdaExpression **LambdaExpr = NULL
    );

    // ----------------------------------------------------------------------------
    // Check whether the target is a lambda expression.
    // ----------------------------------------------------------------------------

    bool
    IsLambdaExpressionTree
    (
        Type* TargetType
    );

    // ----------------------------------------------------------------------------
    // End Conversions to expression trees.
    // ----------------------------------------------------------------------------

    bool
    IsValidAttributeArrayConstant
    (
        ILTree::Expression *PossibleConstant
    );

    bool
    IsValidAttributeConstant
    (
        ILTree::Expression *PossibleConstant
    );

    void
    ReportArrayCovarianceMismatch
    (
        ArrayType *SourceArray,
        ArrayType *TargetArray,
        const Location &ErrorLocation
    );

    void
    CollectConversionOperators
    (
        Type *TargetType,
        Type *SourceType,
        CDoubleList<OperatorInfo> &OperatorSet,
        NorlsAllocator &Scratch
    );

    ConversionClass
    Semantics::ClassifyConversionFromExpression
    (
        Type * pTargetType, 
        ILTree::Expression * pExpression
    );

    bool 
    IsEmptyArrayLiteralType
    (
        Type * pType
    );

    ConversionClass
    ClassifyArrayLiteralConversion
    (
        Type *TargetType,
        BCSYM_ArrayLiteralType *SourceType
    );

    ConversionClass
    ClassifyConversion
    (
        Type *TargetType,
        Type *SourceType,
        Procedure *&OperatorMethod,
        GenericBinding *&OperatorMethodGenericContext,
        bool &OperatorMethodIsLifted,
        bool considerConversionsOnNullableBool = true,    //Controls whether or not we consider
                                                          //Boolean->Boolean? or Boolean?->Boolean conversions. If this is false, we will still consider all other nullable
                                                          //conversions. It is named considerConversionsOnNullableBool because the conversions from T?->T are actually implemented
                                                          //as userdefined conversions on the Nullable(of T) type, even though we treat them like predefined conversions.
        bool * pConversionRequiresUnliftedAccessToNullableValue = NULL, // [out] - Returns true if the classified conversion requires
                                                                        //a T?->T conversion.
        bool *pConversionIsNarrowingDueToAmbiguity = NULL,// [out] did we return "Narrowing" for reason of ambiguity,
                                                          // e.g. multiple variance-conversions?
        DelegateRelaxationLevel *pConversionRelaxationLevel = NULL,  // [out] If converting a VB$AnonymousDelegate to a delegate type, how did it relax?
        bool IgnoreOperatorMethod = false
    );
    
    ConversionClass
    ClassifyPredefinedCLRConversion
    (
        Type *TargetType,
        Type *SourceType,
        ConversionSemanticsEnum ConversionSemantics,    // allow boxing? value-conversions? non-VB conversions?
        bool IgnoreProjectEquivalence = false,          // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForTargetType = NULL,  // The project containing a component of type TargetType that was considered
                                                        // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForSourceType = NULL,  // The project containing a component of type SourceType that was considered
                                                        // for a successful match ignoring project equivalence.
        bool *pConversionIsNarrowingDueToAmbiguity = NULL,  // [out] did we return "Narrowing" for reason of ambiguity,
                                                           // e.g. multiple variance-conversions?
        bool *pNarrowingCausedByContraVariance = NULL                                                          
    );

    ConversionClass
    ClassifyPredefinedConversion
    (
        Type *TargetType,
        Type *SourceType,
        bool IgnoreProjectEquivalence = false,          // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForTargetType = NULL,  // The project containing a component of type TargetType that was considered
                                                        // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForSourceType = NULL,   // The project containing a component of type SourceType that was considered
                                                        // for a successful match ignoring project equivalence.
        bool ignoreNullablePredefined = false,           // Nullable conversions apply only no other regular conversion exists.
                                                        // Use this flag to avoid infinite recursion.
        bool considerConversionsOnNullableBool = true,          // Note: This flag is different than ignoreNullablePredefined. In particular, it controls wether or not we consider
                                                //Booealn->Boolean? or Boolean?->Boolean conversions. If this is false, we will still consider all other nullable
                                                //conversions. It is named considerConversionsOnNullableBool because the conversions from T?->T are actually implemented
                                                //as userdefined conversions on the Nullable(of T) type, even though we treat them like predefined conversions.
        bool * pConversionRequiresUnliftedAccessToNullableValue = NULL, // [out] - Returns true if the classified conversion requires
                                                                        //a T?->T conversion.
        bool *pConversionIsNarrowingDueToAmbiguity = NULL,  // [out] did we return "Narrowing" for reason of ambiguity,
                                                            // e.g. multiple variance-conversions?
        DelegateRelaxationLevel *pConversionRelaxationLevel = NULL  // [out] If converting a VB$AnonymousDelegate to a delegate type, how did it relax?
    );

    ConversionClass
    ClassifyTryCastConversion
    (
        Type *TargetType,
        Type *SourceType,
        bool IgnoreProjectEquivalence = false,          // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForTargetType = NULL,  // The project containing a component of type TargetType that was considered
                                                        // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForSourceType = NULL   // The project containing a component of type SourceType that was considered
                                                        // for a successful match ignoring project equivalence.
    );

    ConversionClass
    ClassifyTryCastForGenericParams
    (
        Type *TargetType,
        Type *SourceType,
        bool IgnoreProjectEquivalence = false,          // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForTargetType = NULL,  // The project containing a component of type TargetType that was considered
                                                        // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForSourceType = NULL   // The project containing a component of type SourceType that was considered
                                                        // for a successful match ignoring project equivalence.
    );

    bool
    CanClassOrBasesSatisfyConstraints
    (
        Type *Class,
        GenericParameter *GenericParam,
        bool IgnoreProjectEquivalence = false,          // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForClass = NULL,       // The project containing a component of type "Class" that was considered
                                                        // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForGenericParam = NULL // The project containing a component of type "GenericParam" that was considered
                                                        // for a successful match ignoring project equivalence.
    );

    ConversionClass
    ResolveConversion
    (
        Type *TargetType,
        Type *SourceType,
        CDoubleList<OperatorInfo> &OperatorSet,
        CDoubleList<OperatorInfo> &OperatorChoices,
        bool WideningOnly,
        NorlsAllocator &Scratch,
        bool &ResolutionIsAmbiguous,
        bool considerConversionsOnNullableBool = true,
        bool * pConversionRequiresUnliftedAccessToNullableValue= NULL
    );

    bool ConversionMaybeLifted
    (
        Type * SourceType,
        Type * TargetType,
        Type * InputType,
        Type * ResultType
    );


    ConversionClass
    ClassifyUserDefinedConversion
    (
        Type *TargetType,
        Type *SourceType,
        Procedure *&OperatorMethod,
        GenericBinding *&OperatorMethodGenericContext,
        bool *OperatorMethodIsLifted,
        bool considerConversionsOnNullableBool = true,
        bool * pConversionRequiresUnliftedAccessToNullableValue = NULL
    );

    void
    FindBestMatch
    (
        Type *SourceType,
        Type *TargetType,
        CDoubleList<OperatorInfo> &SearchList,
        CDoubleList<OperatorInfo> &ResultList,
        bool &GenericOperatorsExistInResultList
    );

    void
    InsertInOperatorListIfLessGenericThanExisting
    (
        OperatorInfo *Operator,
        CDoubleList<OperatorInfo> &OperatorList,
        bool &GenericTypeMembersSeen
    );

    bool
    Encompasses
    (
        Type *Larger,
        Type *Smaller,
        bool considerConversionsOnNullableBool,
        bool IgnoreIdentity = false
    );

    bool
    NotEncompasses
    (
        Type *Larger,
        Type *Smaller,
        bool considerConversionsOnNullableBool
    );

    Type *
    MostEncompassing
    (
        TypeSet &Types,
        TypeSet &Results
    );

    Type *
    MostEncompassed
    (
        TypeSet &Types,
        TypeSet &Results
    );

    ConversionClass
    CheckNullableConversion
    (
        bool &IsNullableInvolved,
        Type *TargetType,
        Type *SourceType,
        bool IgnoreProjectEquivalence,
        CompilerProject **ProjectForTargetType,
        CompilerProject **ProjectForSourceType,
        bool considerConversionsOnNullableBool = true,
        bool * pConversionRequiresUnliftedAccessToNullableValue = NULL
    );

    // From a ConstantValue holding a compile-time computed value, produce
    // an expression tree representing the value contained in the ConstantValue,
    // with a result type equivalent to the type contained in the ConstantValue.
    ILTree::Expression *
    ProduceConstantExpression
    (
        ConstantValue Value,
        const Location &ExpressionLocation,
        Type *ResultType
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ProduceConstantExpression
    (
        Quadword Value,
        const Location &ExpressionLocation,
        Type *ResultType
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ProduceStringConstantExpression
    (
        _In_opt_count_(LengthInCharacters) const WCHAR *Spelling,
        size_t LengthInCharacters,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    ProduceFloatingConstantExpression
    (
        double Value,
        const Location &ExpressionLocation,
        Type *ResultType
        IDE_ARG(unsigned Flags)
    );

    ILTree::Expression *
    Semantics::ProduceDecimalConstantExpression
    (
        DECIMAL Value,
        const Location &ExpressionLocation
        IDE_ARG(unsigned Flags)
    );

    // Given an expression representing a constant value, produce a
    // ConstantValue containing the same value and an appropriate type.
    ConstantValue
    ExtractConstantValue
    (
        ILTree::Expression *Input
    );

    // Given an expression that is a unary or binary operator with constant
    // operand(s), evalate the expression at compile time and produce a
    // constant expression as a result.

    ILTree::Expression *
    PerformCompileTimeBinaryOperation
    (
        BILOP Opcode,
        Type *ResultType,
        const Location &ExpressionLocation,
        ILTree::Expression *Left,
        ILTree::Expression *Right
    );

    ILTree::Expression *
    PerformCompileTimeUnaryOperation
    (
        BILOP Opcode,
        Type *ResultType,
        const Location &ExpressionLocation,
        ILTree::Expression *Operand
    );

    // Return true if the current interpretation occurs within the body
    // of a method, and false otherwise.
    bool
    WithinProcedure
    (
    );

    bool
    WithinSharedProcedure
    (
    );

    bool
    WithinInstanceProcedure
    (
    );

    bool
    WithinSyntheticCode
    (
    );

    bool
    PerformObsoleteChecks
    (
    );

    Container *
    ContainerContextForObsoleteSymbolUsage
    (
    );

    // Return true if an assignment can be made to the property reference, and false
    // otherwise. If the property has no assignment methods that are accessible and
    // accept appropriate arguments, the result will be false.

    bool
    AssignmentPossible
    (
        ILTree::PropertyReferenceExpression &Reference
    );

    // To record a dependency on a declaration, use RecordDependency. The reason why
    // both LogDependency and RecordDependency exist is to achieve zero compile-time
    // cost for dependency collection in a non-IDE build.

    void
    LogDependency
    (
        Declaration *DependedOn
    );

    void
    LogNameDependency
    (
        _In_z_ Identifier *DependedOn
    );

    void
    RecordDependency
    (
        Declaration *DependedOn
    )
    {
#if IDE 
        LogDependency(DependedOn);
#endif
    }

    void
    RecordDependency
    (
        Declaration *DependedOn,
        bool LeadingQualifiedSymbol
    );

    bool
    RecordDependencyForSymbolCaching
    (
        Declaration *DependedOn
    );

    bool
    DoesRingContainExtensionMethods
    (
        BCSYM_NamespaceRing *NamespaceRing
    );

    bool
    DoesRingContainModules
    (
        BCSYM_NamespaceRing *NamespaceRing
    );

    bool
    IterateNamespaceRing
    (
        BCSYM_NamespaceRing *NamespaceRing,
        RingIteratorType
    );

    bool
    IterateNamespaceRingForModules
    (
        BCSYM_NamespaceRing *NamespaceRing
    );

    bool
    IterateNamespaceRingForEM
    (
        BCSYM_NamespaceRing *NamespaceRing,
        bool & skippedUnavailableNamespaces
    );

    Scope *
    GetMergedHashTable
    (
        BCSYM_NamespaceRing *NamespaceRing
    );

    Scope *
    CreateMergedHashTable
    (
        BCSYM_NamespaceRing *NamespaceRing,
        NorlsAllocator *pNorlsAllocator
    );

    void
    CreateLabelDefinitions(_In_ ParseTree::ExecutableBlockStatement *MethodBody);

    // Initialize the per-interpretation state of a Semantics object.
    void
    InitializeInterpretationState
    (
        _In_ ILTree::ProcedureBlock *MethodBody,
        Scope *Lookup,
        Cycles *CycleDetection,
        CallGraph *CallTracking,
        _In_ TransientSymbolStore *CreatedDuringInterpretation,
        bool PreserveExtraSemanticInformation,
        bool PerformObsoleteChecks,
        bool CanInterpretStatements,
        bool CanInterpretExpressions,
        bool MergeAnonymousTypeTemplates,
        Declaration *AppliedAttributeContext = NULL,
        bool ResetClosuresInfo = true,
        Declaration *NamedContextForConstantUsage = NULL
#if IDE 
        , FromItemsHashTable *FromItemsMap = NULL
#endif
    );

    ClassOrRecordType *
    ContainingClass
    (
    )
    {
        // If we're under the Expression Evaluator we may need to build m_ContainingClass
        return m_ContainingClass ? m_ContainingClass : m_ContainingClass = ContainingContainer()->GetContainingClass();
    }

    Container *
    ContainingContainer
    (
    )
    {
        // If we're under the Expression Evaluator we may need to build m_ContainingContainer
        return m_ContainingContainer ?
                    m_ContainingContainer :
                    (m_Lookup ? m_ContainingContainer = m_Lookup->GetContainer() : NULL);
    }

    bool IsAppliedAttributeContext()
    {
        return (m_NamedContextForAppliedAttribute != NULL);
    }

    // Returns true if the current context is within a module and not within
    // a class defined with Class ... End Class.
    bool
    WithinModule
    (
    );

    // Find the scope that encloses a given scope.

    static Scope *
    GetEnclosingScope
    (
        Scope *Lookup,
        NameFlags flags = NameNoFlags
    );

    // Find the scope that encloses the current scope, skipping over
    // all locals scopes.
    Scope *
    GetEnclosingNonLocalScope
    (
    );

    Type *
    GetPointerType
    (
        Type *PointedToType
    );

    TemporaryManager *GetTemporaryManager()
    {
        return m_TemporaryManager;
    }

    // Find a helper method in the specified type.
    // If the method is not available, produce a diagnostic and return NULL.
    Procedure *
    FindHelperMethod
    (
        _In_z_ Identifier *MethodName,
        ClassOrRecordType *ContainingType,
        const Location &TextSpan,
        bool SuppressErrorReporting = false
    );

    enum HelperNamespace
    {
        SystemNamespace,
        MicrosoftVisualBasicNamespace,
        MicrosoftVisualBasicCompilerServicesNamespace
    };

    // Find a helper class in the specified namespace.
    // If the class is not available, produce a diagnostic and return NULL.
    ClassOrRecordType *
    FindHelperClass
    (
        _In_z_ Identifier *ClassName,
        HelperNamespace ContainingNamespace,
        const Location &TextSpan
    );

    // Methods for producing diagnostic messages.

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        ParseTree::Statement::Opcodes Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        ParseTree::Expression::Opcodes Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        BILOP Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ILTree::ILNode *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location *ErrorLocation,
        Symbol *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        StringBuffer &Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        ACCESS Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ILTree::ILNode *Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1,
        _In_ ParseTree::Expression *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        StringBuffer &Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1,
        StringBuffer &Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1,
        _In_opt_z_ const WCHAR *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ILTree::ILNode *Substitution1,
        StringBuffer &Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        StringBuffer &Substitution1,
        _In_ CompileError *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        _In_opt_z_ const WCHAR *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        _In_opt_z_ const WCHAR* Extra,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        const Quadword Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        _In_opt_z_ const WCHAR *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        BILOP Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_ ParseTree::Expression *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        _In_ ILTree::Expression *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorID,
        const Location & ErrorLocation,
        _In_opt_z_ STRING * Substitution1,
        _In_opt_z_ STRING * Substitution2,
        _In_opt_z_ STRING * Substitution3,
        _In_opt_z_ STRING * Substitution4,
        _In_opt_z_ STRING * Substitution5,
        _In_opt_z_ STRING * Substitution6,
        _In_opt_z_ STRING * Substitution7
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        ParseTree::Expression::Opcodes Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        BILOP Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        BILOP Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3,
        Symbol *Substitution4
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3,
        Symbol *Substitution4
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        ACCESS Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        ACCESS Substitution1
    );


    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        Symbol *Substitution2,
        StringBuffer &Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        _In_z_ WCHAR *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        _In_z_ WCHAR *Substitution3,
        _In_z_ WCHAR *Substitution4,
        _In_z_ WCHAR *Substitution5
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        _In_z_ WCHAR *Substitution3,
        _In_z_ WCHAR *Substitution4,
        _In_z_ WCHAR *Substitution5,
        _In_z_ WCHAR *Substitution6
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        _In_z_ WCHAR *Substitution2,
        _In_z_ WCHAR *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        CompilerProject *Substitution2,
        CompilerProject *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        _In_opt_z_ const WCHAR *wszExtra,
        const Location &ErrorLocation,
        Symbol *Substitution1
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        _In_opt_z_ const WCHAR *wszExtra,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3,
        Symbol *Substitution4
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        Symbol *Substitution3,
        _In_opt_z_ const WCHAR *Substitution4
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        Symbol *Substitution1,
        Symbol *Substitution2,
        _In_opt_z_ const WCHAR *Substitution3,
        _In_opt_z_ const WCHAR *Substitution4
    );

    void
    ReportSemanticError
    (
        unsigned ErrorId,
        const Location &ErrorLocation,
        _In_opt_z_ const WCHAR *Substitution1,
        _In_opt_z_ const WCHAR *Substitution2,
        _In_opt_z_ const WCHAR *Substitution3
    );

    void
    ReportSemanticError
    (
        unsigned ErrorID,
        const Location & ErrorLocation,
        Symbol * Substitution1,
        _In_opt_z_ const WCHAR * Substitution2,
        _In_opt_z_ const WCHAR * Substitution3,
        _In_opt_z_ const WCHAR * Substitution4
    );

    void
    ReportMissingType
    (
        FX::TypeName type,
        const Location & ErrorLocation
    );

    void 
    ReportLambdaParameterInferredToBeObject
    (
        Parameter* pLambdaParam
    );    

    void
    ReportuntimeHelperNotFoundError
    (
        const Location & ErrorLocation,
        _In_opt_z_ const WCHAR *NameOfMissingHelper
    );

    // Errors related to bad types are not necessarily reported when the
    // type is marked bad. (The associated symbol may be part of a
    // referenced assembly, in which case the error can't be reported until
    // the symbol is referenced.) Therefore, when semantic analysis comes
    // upon a bad type, it must report this.

    void
    ReportBadType
    (
        Type *BadType,
        const Location &ErrorLocation
    );

public:
    static void
    ReportBadType
    (
        Type *BadType,
        const Location &ErrorLocation,
        Compiler * pCompiler, 
        ErrorTable * pErrorTable
    );

protected:
    void
    ReportBadDeclaration
    (
        Declaration *Bad,
        const Location &ErrorLocation
    );

public:
    static void
    ReportBadDeclaration
    (
        Declaration *Bad,
        const Location &ErrorLocation,
        Compiler * pCompiler, 
        ErrorTable * pErrorTable
    );

protected:

    WCHAR *
    ExtractErrorName
    (
        Symbol *Substitution,
        StringBuffer &TextBuffer,
        bool FormatAsExtensionMethod = false,
        IReadonlyBitVector * FixedTypeArgumentBitVector = NULL,
        GenericBinding * GenericBindingContext = NULL
    );

    static Location
    ExtractLambdaErrorSpan
    (
        _In_ ILTree::UnboundLambdaExpression *lambda
    );

    BCSYM_NamedRoot* GetContextOfSymbolUsage();

    void
    CheckObsolete
    (
        Symbol *SymbolToCheck,
        _In_ const Location &ErrorLocation
    );

    const WCHAR *
    MessageSubstitution
    (
        _In_ ParseTree::Expression *TreeToRepresent,
        StringBuffer &TextBuffer
    );

    const WCHAR *
    MessageSubstitution
    (
        ParseTree::Expression::Opcodes Opcode
    );

    const WCHAR *
    MessageSubstitution
    (
        ParseTree::Statement::Opcodes Opcode
    );

    const WCHAR *
    MessageSubstitution
    (
        _In_ ILTree::ILNode *TreeToRepresent,
        StringBuffer &TextBuffer
    );

    const WCHAR *
    MessageSubstitution
    (
        BILOP Opcode
    );

    const WCHAR *
    MessageSubstitution
    (
        ACCESS Access
    );

    // Methods for tree management.

    ILTree::ExtensionCallExpression *
    AllocateExtensionCall
    (
        ILTree::Expression * BaseReference,
        ExtensionCallLookupResult * ExtensionCallLookupResult,
        const Location & TreeLocation,
        unsigned ImplicitMeErrorID,
        bool SynthesizedMeReference
    );

    ILTree::DeferredTempExpression * 
    Semantics::AllocateDeferredTemp
    (
        ParseTree::Expression *InitialValue,
        Type *ResultType,
        ExpressionFlags ExprFlags,
        const Location & TreeLocation
    );

    ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        const Location &StartLocation,
        const Location &EndLocation
    );

    ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        const Location &TreeLocation
    );

    ILTree::Expression *
    AllocateUserDefinedOperatorExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Left,
        ILTree::Expression *Right,
        const Location &TreeLocation
    );

    ILTree::Expression *
    AllocateIIfExpression
    (
        Type *ResultType,
        ILTree::Expression *Condition,
        ILTree::Expression *TruePart,
        ILTree::Expression *FalsePart,
        const Location &TreeLocation
    );

    ILTree::Expression *
    AllocateBadExpression
    (
        const Location &TreeLocation
    );

    ILTree::Expression *
    AllocateBadExpression
    (
        Type *ResultType,
        const Location &TreeLocation
    );

    ILTree::DelegateConstructorCallExpression *
    AllocateDelegateConstructorCall
    (
        Type *ResultType,
        ILTree::Expression *Constructor,
        ILTree::Expression *BaseReference,
        ILTree::Expression *Method,
        const Location &TreeLocation
    );

    ILTree::SymbolReferenceExpression *
    AllocateSymbolReference
    (
        Declaration *Symbol,
        Type *ResultType,
        ILTree::Expression *BaseReference,
        const Location &TreeLocation,
        GenericBinding *GenericBindingContext = NULL
    );

    inline ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Operand,
        const Location &StartLocation,
        const Location &EndLocation
    )
    {
        return AllocateExpression(Opcode, ResultType, Operand, NULL, StartLocation, EndLocation);
    }

    inline ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        ILTree::Expression *Operand,
        const Location &TreeLocation
    )
    {
        return AllocateExpression(Opcode, ResultType, Operand, NULL, TreeLocation);
    }

    inline ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        const Location &StartLocation,
        const Location &EndLocation
    )
    {
        return AllocateExpression(Opcode, ResultType, NULL, NULL, StartLocation, EndLocation);
    }

    inline ILTree::Expression *
    AllocateExpression
    (
        BILOP Opcode,
        Type *ResultType,
        const Location &TreeLocation
    )
    {
        return AllocateExpression(Opcode, ResultType, NULL, NULL, TreeLocation);
    }

    ILTree::ArrayLiteralExpression* 
    AllocateArrayLiteralExpression
    (
        ILTree::ExpressionWithChildren * pElementList,
        const ArrayList<unsigned> & lengthList,
        const Location & loc
    );

    ILTree::ArrayLiteralExpression* 
    AllocateArrayLiteralExpression
    (
        ILTree::ExpressionWithChildren * pElementList,
        unsigned rank,
        unsigned * pDims,
        const Location & loc
    );    

    ILTree::NestedArrayLiteralExpression *
    AllocateNestedArrayLiteralExpression
    (
        ILTree::ExpressionWithChildren * pElementList,
        const Location & loc
    );

    ILTree::Statement *
    AllocateStatement
    (
        BILOP Opcode,
        _In_ const Location &TreeLocation,
        const unsigned short ResumeIndex,
        bool Link = true
    );

    // Create a bound statement and append the comments from the parse tree
    // statement to it.
    ILTree::Statement *
    AllocateStatement
    (
        BILOP Opcode,
        _In_ ParseTree::Statement *UnboundStatement,
        bool link = true
    );

    ILTree::ExecutableBlock *
    AllocateBlock
    (
        BILOP Opcode,
        _In_ ParseTree::ExecutableBlockStatement *UnboundBlock,
        bool link = true
    );

    ILTree::ExecutableBlock *
    AllocateHiddenBlock
    (
        BILOP Opcode,
        unsigned LocalsCount
    );

    ILTree::FinallyBlock *
    AllocateFinallyStatement
    (
        _Out_opt_ ILTree::TryBlock *OwningTry,
        _In_ const Location &TreeLocation,
        const unsigned short ResumeIndex
    );

    ILTree::FinallyBlock *
    AllocateFinallyBlock
    (
        _Out_opt_ ILTree::TryBlock *OwningTry,
        _In_ ParseTree::ExecutableBlockStatement *UnboundBlock
    );

    // Returns true if a name represents an obsolete VB6 keyword that could
    // be used as a standalone expression.

    bool
    IsObsoleteStandaloneExpressionKeyword
    (
        _In_z_ Identifier *Name
    );

    ExpressionList *
    BuildConcatList
    (
        ILTree::Expression *CurrentNode,
        ExpressionList *BuiltTree,
        unsigned &ElementCount
    );

    void
    ReduceConcatList
    (
        ExpressionList *ConcatList,
        unsigned &ElementCount
    );

    ILTree::Expression *
    OptimizeConcatenate
    (
        ILTree::Expression *ConcatTree,
        const Location &ConcatLocation
    );

    ILTree::Expression*
    AlterForMyGroup
    (
        ILTree::Expression *Operand,
        Location location
    );

    void
    CheckRestrictedArraysInLocals
    (
        BCSYM_Hash *Locals
    );

    void
    CheckSecurityCriticalAttributeOnParentOfResumable
    (
        BCSYM_Proc* pResumableProc
    );

    Location&
    GetSpan
    (
        _Out_ Location &Target,
        Location &StartLocation,
        Location &EndLocation
    );

    bool
    WarnOptionStrict
    (
    )
    {
        return (!m_EvaluatingConditionalCompilationConstants && m_SourceFile && !m_SourceFile->OptionStrictOffSeenOnFile());
    };

    class ArgumentParameterIterator
    {
    public:

        ArgumentParameterIterator(
            Parameter*      parameters,
            ILTree::Expression**    boundArguments,
            ILTree::Expression*     firstParamArrayArgument,
            bool            ignoreFirstParameter);

        bool MoveNext();

        Parameter*      CurrentParameter();
        Type*           ParameterType();
        ILTree::Expression*     CurrentArgument();
        Type*           ArgumentType();
        ILTree::Expression*     CurrentArgumentHolder();
        bool            IsExpandedParamArray();

    private:
        Parameter   *m_parameter;
        Parameter   *m_nextParameter;

        Type        *m_parameterType;
        bool        m_parameterIsExpandedParamArray;

        ILTree::Expression**    m_boundArguments;
        ILTree::Expression*     m_currentParamArrayArgument;
        ILTree::Expression*     m_firstParamArrayArgument;
        int             m_argumentIndex;
        ILTree::Expression*     m_argument;
        Type*           m_argumentType;
        ILTree::Expression* m_argumentHolder;

        bool        m_ignoreFirstParameter;

    };

#if DEBUG
    static void
    DumpStronglyConnectedGraph(GraphNodeList* sscGraph)
    {
        if (VSFSWITCH(fDumpInference))
        {
            GraphNodeListIterator iter(sscGraph);
            while (iter.MoveNext())
            {
                StronglyConnectedComponent* sccNode = (StronglyConnectedComponent*)iter.Current();
                DebPrintf("    - StronglyConnectedComponent\n");

                GraphNodeListIterator childIter(sccNode->GetChildNodes());
                while (childIter.MoveNext())
                {
                    InferenceNode* current = (InferenceNode*)childIter.Current();

                    DebPrintf("        - %S (", current->DbgPrint());
                    GraphNodeArrayListIterator dumpOutEdgesIter(&current->GetOutgoingEdges());
                    while (dumpOutEdgesIter.MoveNext())
                    {
                        DebPrintf("%S, ", dumpOutEdgesIter.Current()->DbgPrint());
                    }
                    DebPrintf(")\n", current->DbgPrint());
                }
            }
        }
    }
#endif

    // InferDominantTypeOfExpressions: see this function's body for a definition of what what is the
    // dominant type of a set of expressions.
    BCSYM *
    InferDominantTypeOfExpressions
    (
        _Out_ unsigned &NumCandidates,
        _Out_ ILTree::Expression* &Winner,
        _In_ ConstIterator<ILTree::Expression *> &expressions
    );

    // Another way to call it, if you don't want to create an iterator:
    BCSYM *
    InferDominantTypeOfExpressions
    (
        _Out_ unsigned &NumCandidates,
        _Out_ ILTree::Expression* &Winner,
        _In_opt_ ILTree::Expression *expression1,
        _In_opt_ ILTree::Expression *expression2,
        _In_opt_ ILTree::Expression *expression3 = NULL
    );

    // ConvertExpressionToDominantType: use this instead of ConvertWithErrorChecking to get
    // "object assumed" warnings/errors correct
    ILTree::Expression *
    ConvertExpressionToDominantType(
        _In_ ILTree::Expression *Expression,
        _In_ BCSYM *ResultType,
        _In_ ILTree::Expression *DominantWinnerExpression,
        _In_ ExpressionFlags Flags
    );

    class TypeInferenceCollection
    {
    private:

        NorlsAllocator*         m_allocator;
        NorlsAllocWrapper*      m_allocatorWrapper;
        Semantics*              m_pSemantics;
        Compiler*               m_pCompiler;

        DominantTypeDataList    m_dominantTypeDataList;

        // Note: Using an array for now because we need nested iteration, and it is
        // faster with contiguous memory, and our hash tables don't support this.

    public:
        TypeInferenceCollection( _In_ Semantics* pSemantics, NorlsAllocator* allocator, NorlsAllocWrapper* allocatorWrapper) :
            m_pCompiler( pSemantics->GetCompiler() ),
            m_pSemantics( pSemantics ),
            m_allocator(allocator),
            m_allocatorWrapper(allocatorWrapper),
            m_dominantTypeDataList(*m_allocatorWrapper)
        {
        }

        virtual ~TypeInferenceCollection()
        {
        }

        void AddType(Type* pSym, ConversionRequiredEnum ConversionRequired, _In_opt_ ILTree::Expression *SourceExpression);

        unsigned long TypeCount()
        {
            return m_dominantTypeDataList.Count();
        }

        DominantTypeDataList* GetTypeDataList();
        void FindDominantType(
            DominantTypeDataList& resultList,
            InferenceErrorReasons *errorReasons,
            bool ignoreOptionStrict = false);

        DECLARE_ENUM(HintSatisfaction)
            ThroughIdentity,
            ThroughWidening,
            ThroughNarrowing,
            Unsatisfied,
            // count: number of elements in this enum, used to construct an array
            Count
        END_ENUM(HintSatisfaction)

        HintSatisfactionEnum CheckHintSatisfaction(
            DominantTypeData* candidate,
            DominantTypeData* hint,
            ConversionRequiredEnum hintRestrictions);
    };



    bool Semantics::IsValidInLiftedSignature
    (
        Type * pType,
        GenericTypeBinding * pGenericBindingContext = NULL
    );

    bool
    ShouldInsertLiftedOperator
    (
        Operator * pLiftedOperator,
        Operator * pSourceOperator,
        GenericTypeBinding * pGenericBindingContext,
        _In_ CDoubleList<OperatorInfo> & OperatorSet
    );

    void LiftOperator
    (
        _Inout_ CDoubleList<OperatorInfo> & OperatorSet,
        GenericTypeBinding * pGenericBindingContext,
        _In_ Operator * pSourceOperator,
        _Inout_ NorlsAllocator * pNorls
    );

    void
    LiftUserDefinedOperatorsForNullable
    (
        _Inout_ CDoubleList<OperatorInfo>  & OperatorSet,
        Type *  pLeftType,
        Type * pRightType,
        _Inout_ NorlsAllocator  * pNorls
    );

    Procedure*
    FindRealIsTrueOperator
    (
        ILTree::Expression* Input
    );

    ILTree::Expression*
    FindOperandForIsTrueOperator
    (
        ILTree::Expression* Input
    );

    bool
    CanLiftIntrinsicOperator
    (
        BILOP Opcode
    );

    // Forward decls for cyclic datastructure.
    class InferenceGraph;
    class InferenceNode;
    class InferenceNamedNode;
    class InferenceTypeNode;

    class InferenceGraph : public Graph
    {
    public:
        InferenceGraph
        (
            _In_ TypeInference*  m_typeInference
        );

        bool
        InferTypeParameters
        (
            Parameter*              firstParameter,
            ILTree::Expression**    boundArguments,
            ILTree::Expression*     firstParamArrayArgument,
            Type*                   delegateReturnType, // Can be Null if none.
            const Location &        callLocation,
            Semantics*              pSemantics = NULL, 
            bool                    reportInferenceAssumptions = false,
            _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
        );

        void
        PopulateGraph
        (
            Parameter*              parameters,
            ILTree::Expression**    boundArguments,
            ILTree::Expression*     firstParamArrayArgument,
            Type*                   delegateReturnType, // Can be Null if none.
            const Location &        callLocation
        );

        InferenceTypeNode*
        FindTypeNode
        (
            GenericParameter* parameter
        );

        TypeInference*
        GetTypeInference
        (
        );

        bool
        IsVerifyingAssertions
        (
        );

        void
        RegisterTypeParameterHint
        (
            GenericParameter*        genericParameter,
            Type*                    inferredType,
            bool                     inferredTypeByAssumption,
            const Location*          argumentLocation,
            Parameter*               parameter,
            bool                     inferedFromObject,
            ConversionRequiredEnum   inferenceRestrictions
        );

    private:
        void
        AddLambdaToGraph
        (
            Type*               parameterType,
            Parameter*          parameter,
            ILTree::Expression*         lambdaExpression,
            _Out_ InferenceNamedNode* nameNode
        );

        void
        AddAddressOfToGraph
        (
            Type*               parameterType,
            Parameter*          parameter,
            ILTree::Expression*         lambdaExpression,
            _Out_ InferenceNamedNode* nameNode
        );

        void
        AddTypeToGraph
        (
            Type*               parameterType,
            Parameter*          parameter,
            _Out_ InferenceNamedNode* nameNode,
            bool                isOutgoingEdge
        );

        void
        AddDelegateReturnTypeToGraph
        (
            Type*               delegateReturnType,
            const Location &    callLocation            
        );

        // members
        TypeInference*                  m_typeInference;
        unsigned                        m_numberOfTypeNodes;
        InferenceTypeNode**             m_typeNodes;    // Array of pointers to the type nodes
        bool                            m_VerifyingAssertions;
    };

    class InferenceNode : public GraphNode
    {

    protected:
        InferenceNode(_In_ Graph *graph);
        virtual ~InferenceNode() {};

    public:
        virtual bool InferTypeAndPropagateHints
        (
            bool reportInferenceAssumptions = false, 
            Semantics *pSemantics = NULL,
            _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
        ) = 0;
        
        InferenceGraph* GetInferenceGraph();
        TypeInference*  GetTypeInference();

#if DEBUG
        void
        VerifyIncomingInferenceComplete
        (
            unsigned nodeType
        );
#endif

        Parameter*      m_Parameter;
        Type*           m_ParameterType;
        bool            m_InferenceComplete;

    };

    class InferenceTypeNode : public InferenceNode
    {
    public:
        InferenceTypeNode(_In_ InferenceGraph *graph);

    #if DEBUG
        virtual void DbgPrint(StringBuffer *buffer)
        {
            buffer->AppendString(m_DeclaredTypeParam->PNamedRoot()->GetName());
        }
    #endif

        virtual bool
        InferTypeAndPropagateHints
        (
            bool reportInferenceAssumptions = false,
            Semantics *pSemantics = NULL,
            _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
        );

        void
        VerifyTypeAssertions
        (
        );

        void
        Semantics::InferenceTypeNode::AddTypeHint
        (
            Type*                               type,
            bool                                typeByAssumption,
            const Location*                     argumentLocation,
            Parameter*                          parameter,
            bool                                inferedFromObject,
            ConversionRequiredEnum              inferenceRestrictions
        );

        GenericParameter*           m_DeclaredTypeParam;
        TypeInferenceCollection     m_inferenceTypeCollection;
        Type*                       m_InferredType;
        bool                        m_InferredTypeByAssumption;
        GraphNodeArrayList          m_TypeAssertions;
    };


    class InferenceNamedNode : public InferenceNode
    {
    public:
        InferenceNamedNode(_In_ InferenceGraph *graph);

    #if DEBUG
        virtual void DbgPrint(StringBuffer *buffer)
        {
            buffer->AppendString(L"[");
            buffer->AppendString(m_Parameter ? m_Parameter->GetName() : L"?");
            buffer->AppendString(L"]");
        }
    #endif

        virtual bool
        InferTypeAndPropagateHints
        (
            bool reportInferenceAssumptions = false,
            Semantics *pSemantics = NULL,
            _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
        );

        void
        SetExpression
        (
            _In_opt_ ILTree::Expression* expression
        );

        ILTree::Expression*
        GetExpression
        (
        );

        void SetArgumentHolder(_In_opt_ ILTree::Expression* expression);


        bool
        GetInferringFromObject
        (
        );

    private:
        ILTree::Expression* m_expression;
        ILTree::Expression* m_argumentHolder;
    };

    class TypeComparer
    {
    public:
        TypeComparer()
        {
        };

        static int
        Compare
        (
            Type* left,
            Type* right
        )
        {
            return TypeHelpers::EquivalentTypes(left, right) ? 0 : 1;
        };
    };

public:
    class TypeInference
    {
    public:
        TypeInference
        (
            Semantics*          semantics,
            Compiler*           compiler,
            CompilerHost*       compilerHost,
            Symbols*            symbolCreator,
            Procedure*          targetProcedure,
            unsigned            typeArgumentCount,
            _In_count_(typeArgumentCount) Type**              typeArguments,
            _In_count_(typeArgumentCount) bool *              typeArgumentsByAssumption,
            _In_opt_count_(typeArgumentCount) Location *      typeArgumentLocations,
            IReadonlyBitVector* fixedParameterBitVector,
            _In_count_(typeArgumentCount) Parameter**         paramsInferredFrom,
            GenericBindingInfo* genericBindingContext
        );

        void SetErrorReportingOptions
        (
            bool                suppressMethodNameInErrorMessages,
            bool                reportErrors,
            bool                candidateIsExtensionMethod
        );

        bool
        InferTypeParameters
        (
            Parameter*              firstParameter,
            ILTree::Expression**    boundArguments,
            ILTree::Expression*     firstParamArrayArgument,
            Type*                   delegateReturnType, // Can be Null if none.
            const Location &        callLocation,
            bool                    reportInferenceAssumptions = false,
            _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity = NULL
        );

        bool
        RegisterInferredType
        (
            GenericParameter*    genericParameter,
            Type*                inferredType,
            bool                 inferredTypeByAssumption,
            _In_ const Location* argumentLocation,
            Parameter*           parameter
        );

        static bool
        RegisterInferredType
        (
            GenericParameter*                 GenericParam,
            Type*                             ArgumentType,
            bool                              ArgumentTypeByAssumption,
            _In_ const Location*              ArgumentLocation,
            Parameter*                        Param,

            _Out_opt_ Type**                  InferredTypeArgument,
            _Out_opt_ GenericParameter**      InferredTypeParameter,

            _Inout_ Type**                    TypeInferenceArguments,
            _Out_opt_ bool*                   TypeInferenceArgumentsByAssumption,
            _Out_opt_ Location*               InferredTypeArgumentLocations,
            _Out_ Parameter**                 ParamsInferredFrom,
            Symbols *                         SymbolCreator
        );

        GenericTypeBinding*
        ConstructPartialGenericTypeBinding
        (
        );

        bool
        SomeInferenceHasFailed
        (
        );

        TypeInferenceLevel
        GetTypeInferenceLevel
        (
        );

        bool
        AllFailedInferenceIsDueToObject
        (
        );

        void
        MarkInferenceFailure
        (
        );

        void
        MarkInferenceLevel
        (
            TypeInferenceLevel  typeInferenceLevel
        );

        bool
        InferTypeArgumentsFromArgument
        (
            _In_ Location*      argumentLocation,
            Type*               argumentType,
            bool                argumentTypeByAssumption,
            Parameter*          parameter,
            Type*               parameterType,
            MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
            _Out_opt_ GenericParameter**  inferredTypeParameter, // Needed for error reporting
            ConversionRequiredEnum InferenceRestrictions
        );


        bool
        InferTypeArgumentsFromLambdaArgument
        (
            ILTree::Expression*         argument,
            Parameter*          parameter,
            Type*               parameterType,
            _Out_opt_ GenericParameter**  inferredTypeParameter, // Needed for error reporting
            _Out_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity
        );

        bool
        InferTypeArgumentsFromAddressOfArgument
        (
            ILTree::Expression*         argument,
            Parameter*          parameter,
            Type*               parameterType,
            _Out_opt_ GenericParameter**  inferredTypeParameter // Needed for error reporting
        );

        void
        SetTypeArgumentLocations
        (
            DominantTypeDataTypeInference* dominantTypeData
        );

        void
        ReportNotFailedInferenceDueToObject
        (
        );

        void
        ReportIncompatibleInferenceError
        (
            DominantTypeDataList *typeInfos,
            TypeInferenceCollection* typeCollection
        );

        void
        RegisterErrorReasons
        (
            InferenceErrorReasons inferenceErrorReasons
        );

        InferenceErrorReasons
        GetErrorReasons
        (
        );

        void
        ReportAmbiguousInferenceError
        (
            DominantTypeDataList &typeInfos
        );

        Semantics*      GetSemantics();
        Compiler*       GetCompiler();
        CompilerHost*   GetCompilerHost();
        Symbols*        GetSymbolCreator();


        NorlsAllocator* GetAllocator()
        {
            return &m_allocator;
        };

        NorlsAllocWrapper* GetAllocatorWrapper()
        {
            return &m_allocatorWrapper;
        };

        InferenceGraph* GetInferenceGraph()
        {
            return &m_inferenceGraph;
        };

        Procedure*
        GetTargetProcedure
        (
        );

        GenericBindingInfo*
        GetGenericBindingContext
        (
        );

    private:

        // Not always there, need to clean up callers first to unify this.
        Semantics*      m_semantics;
        Compiler*       m_compiler;
        CompilerHost*   m_compilerHost;
        Symbols*        m_symbolCreator;

        // Used to thread through.
        unsigned                m_typeArgumentCount;
        Type**                  m_typeArguments;
        bool*                   m_typeArgumentsByAssumption;
        Location *              m_typeArgumentLocations;
        Procedure*              m_targetProcedure;
        IReadonlyBitVector*     m_fixedParameterBitVector;
        Parameter**             m_paramsInferredFrom;
        GenericBindingInfo*     m_genericBindingContext;

        // used for errormessage reporting
        bool m_reportErrors;
        bool m_suppressMethodNameInErrorMessages;
        bool m_candidateIsExtensionMethod;
        bool m_someInferenceFailed;
        InferenceErrorReasons m_inferenceErrorReasons;

        // result information
        bool m_allFailedInferenceIsDueToObject;
        TypeInferenceLevel m_typeInferenceLevel;

        // Needs to be at the bottom and this order for proper order of initialization.
        // c++ compiler doesn't warn and will use order of member declaration, not
        // order of initialization in constructor.
        NorlsAllocator      m_allocator;
        NorlsAllocWrapper   m_allocatorWrapper;
        InferenceGraph      m_inferenceGraph;
    };

protected:

    void
    GetProcedureState(_Out_ ProcedureState *);

    void
    SetProcedureState(_In_ ProcedureState *);

    ILTree::Expression *
    CreateExtensionMethodValueTypeDelegateLambda
    (
        Type * DelegateType,
        ILTree::Expression * ObjectArgument,
        ILTree::Expression * MethodOperator,
        BCSYM_Proc * InvokeMethod,
        const Location & AddressOfTextSpan
    );

    void RewriteResumableMethod(BCSYM_Proc* proc);

    // Data members.
    //
    // m_pReceiverType and m_pReceiverLocation store the
    // type of and location of the receiver of an extension method call.
    Type * m_pReceiverType;
    Location * m_pReceiverLocation;

    Compiler *m_Compiler;
    CompilerHost *m_CompilerHost;

    NorlsAllocator &m_TreeStorage;
    NorlsAllocator &m_DefAsgAllocator;

    ErrorTable *m_Errors;
    AlternateErrorTableGetter m_AltErrTablesForConstructor; // for constructors in partial types, different lines of code could
                                                            // be from different files. So the requirement for alternate error
                                                            // tables so that the errors if any could be reported in the appropriate
                                                            // error tables.

    BILALLOC m_TreeAllocator;
    Symbols m_SymbolCreator;
    SourceFile *m_SourceFile;
    LANGVERSION m_CompilingLanguageVersion; // the version we are compiling against, can be set by /LangVersion switch
    OPTION_FLAGS m_SourceFileOptions; // SourceFile Option flags

    bool m_ReportErrors;

    // All data members below this point must be initialized by each public interpreter.

    // The scope where name lookup originates.
    Scope *m_Lookup;

    bool m_CreateImplicitDeclarations;
    bool m_EvaluatingConditionalCompilationConstants;

    // Used to help prevent circular references in declarations.
    // For eg: dim x = foo(x).

    // We need a stack of the initializer variables in order to
    // keep track of nested initializations in multiline lambda expressions.
    InitializerInferInfo* m_InitializerInferStack;

    // To catch the use of collection initializers in ways that weren't available to previous
    // versions of vb, we need to know if the target type of an initialization expression is
    // an array or not, e.g. to distinguish things like dim a = {1,2,3} (introduced in v10) from 
    // dim a() = {1,2,3} (legal in v9 and before)
    bool m_InitializerTargetIsArray;

    // Used to create explicit loop variables when option infer is
    // on even if the variable use is implicit.

    bool m_ExplicitLoopVariableCreated;
    unsigned m_CreateExplicitScopeForLoop;

    TemporaryManager *m_TemporaryManager;
    // State for Xml Literal Interpretation
    XmlSymbols m_XmlSymbols;
    // XmlSemantics has state which lives for the life of one xml expression
    XmlSemantics *m_XmlSemantics;
    // XmlNameVars live for the life of a method
    DynamicHashTable<STRING *,ILTree::Expression *,NorlsAllocWrapper> *m_XmlNameVars;
    // Temporaries that need to be added to the method body before processing closures
    unsigned m_methodDeferredTempCount;

    // Data members used for statement interpretation.

    bool m_CompilingConstructorDefinition;
    bool m_ExpectingConstructorCall;
    bool m_ProcedureContainsTry;
    bool m_ProcedureContainsOnError;
    bool m_SynthesizingBaseCtorCall;

    bool m_UsingOptionTypeStrict;
    // References to Me are not allowed in the arguments of a constructor call
    // if that call is the first statement in another constructor.

    bool m_DisallowMeReferenceInConstructorCall;

    // Some clients of semantic analysis (e.g. IntelliSense) require
    // that some information be preserved in the trees that is not required
    // (or interferes) with normal code generation.
    // The PreserveExtraSemanticInformation flag controls this.

    bool m_PreserveExtraSemanticInformation;

    // Sometimes it is important to control when obsolete checks are performed
    // (for example, do not perform the checks before we have finished importing and marking
    // symbols obsolete).
    // The PerformObsoleteChecks flag controls this.

    bool m_PerformObsoleteChecks;

    // Some tree constructs are created differently for XML generation.
    // For example, XMLGen doesn't like SX_SEQ_OP2s, so don't generate
    // them when the client is XMLGen.
    bool m_IsGeneratingXML;

    //XMLGen needs to know about SX_Bad nodes. VSW 547873
    //Without this flag expressions with bad LHS and RHS will not be included
    //but other SX_Bad nodes will.
    bool m_fIncludeBadExpressions;

    // Declaration caching is permitted only for compilation proper, not
    // for XML generation or individual construct interpretation. (This is
    // so because such interpretations could create cachings that would
    // not be invalidated as necessary by demotions.)
    bool m_PermitDeclarationCaching;

    // The enclosing bound block context.
    ILTree::ExecutableBlock *m_BlockContext;

    // The nearest class type that is or encloses the current context.
    ClassOrRecordType *m_ContainingClass;

    // The nearest container (Class/Interface/Namespace) that is or
    // encloses the current context.
    Container *m_ContainingContainer;

    // The actual Unnamed Namespace to consider when looking for
    // the file level imports in which to look up a name. This is
    // needed explicitly because it could be different from the
    // enclosing logical Unnamed Namespace for partial types.
    //
    Namespace *m_UnnamedNamespaceForNameLookupInImports;

    // This is used to suppress generic binding synthesis when
    // interpreting raw generic names. This is needed so that
    // in these cases bindings are not synthesized and thus
    // finding the raw generic through two different bindings
    // is not treated as ambiguous.
    //
    // Eg: C1(Of , ) in GetType(C1(Of , ))
    //
    bool m_SuppressNameLookupGenericBindingSynthesis;

    // Used to indicate if the currently interpretConstantExpression
    // is evaluating a Synthetic constant expression. This is need
    // so that circular constant evaluation error can only be given
    // on Non-Synthetic expressions.
    //
    bool m_IsEvaluatingSyntheticConstantExpression;

    // The type to be used for the context for obsolete checks/
    // named types, etc. in an applied attribute context.
    //
    // The context in which an obsolete symbol was used.
    //      - If the attribute was applied on a container,
    //        then the the context is the container itself
    //      - If the attribute was applied on a namedroot,
    //        the the context is the container of the named
    //        root.
    //      - If the attribute was applied on a parameter,
    //        then the context is the method the parameter
    //        belongs to
    // Why do we need this ? - This is needed because bindable
    // processes the attributes on a container as part of
    // the container itself unlike other named roots for which
    // the attributes are processed as part of their container.
    //
    // Bindable does this because of the ----ymmetry between
    // attributes on namespaces where the lookup and the use
    // context are the same vs. on other types where the lookup
    // is the parent of the use context.
    //
    // This needs to be passed to the obsolete checker
    // which in turn can attach any delayed obsolete checks
    // to the bindable instance of this container.
    //
    // This should only be set when processing attributes
    // applied on containers because the symbol lookup is different
    // (it is the parent container) than the actual container
    // context in which the attribute was used. In all other
    // cases it is NULL because the ContainingClass is itself the
    // context.
    //
    // Also required to indicate to semantics whether the processing
    // is in the context of an applied attribute, i.e. attribute
    // arguments, etc.
    //
    Declaration *m_NamedContextForAppliedAttribute;

    // In general, m_NamedContextForConstantUsage refers to
    // a variable or procedure, to which the constant is assigned to or used respectively.

    // If the constant is used as an optional parameter, then the procedure
    // is referred.
    // If the constant is assigned to a constant field, then the variable is referred.

    Declaration *m_NamedContextForConstantUsage;

    // Where to link in the next bound statement at the same block level.
    ILTree::Statement **m_StatementTarget;

    // The last statement linked in the current context.
    ILTree::Statement *m_LastStatementInContext;

    // A hash table of label declarations.
    struct LabelDeclarationList : public LabelDeclaration
    {
        LabelDeclarationList * Next;
    };

    unsigned m_LabelHashBucketCount;
    LabelDeclarationList **m_LabelHashBuckets;

    // If a method body is being interpreted, the bound tree for the method body,
    // otherwise NULL.
    ILTree::ProcedureBlock *m_ProcedureTree;

    // If a statement/expr lambda is being interpreted, the interpreter for it,
    // otherwise NULL.
    LambdaBodyInterpretStatement *m_StatementLambdaInterpreter;
    LambdaBodyInterpretExpression *m_ExpressionLambdaInterpreter;
    // The rule is:
    //    1. if m_ExpressionLambdaInterpreter is non-NULL, then we're in an expression lambda (and m_StatementLambdaInterpreter might be out enclosing multiline lambda, and m_Procedure is our enclosing method)
    //    2. Otherwise if m_StatementLambdaInterpreter is non-NULL, then we're in a statement lambda (and m_Procedure is our enclosing method)
    //    3. Otherwise go for m_Procedure since we're not in any lambdas.


    // The outer most multiline lambda.
    ILTree::StatementLambdaBlock *m_OuterStatementLambdaTree;

    // Used to report lambda return type inference errors even when
    // m_ReportErrors is false.
    // 



    TriState<bool> m_ReportMultilineLambdaReturnTypeInferenceErrors;

    // If the current interpretation is within the context of a method body, the
    // declaration of the method, otherwise NULL. Note that, for IntelliSense or
    // debugging clients, an interpretation that is not of an entire method body
    // can be within the context of a method body.
    Procedure *m_Procedure;

    // This is used to track the correct owner of transient symbols (the outer containing procedure) when
    // m_Procedure may be changing while lowering resumable methods.
    Procedure *m_TransientSymbolOwnerProc;

    bool m_NoIntChecks;    // If the project option forces no integer overflow check or if we are compiling a special synthetic method that does not want integer overflow check.

    // If the body of a With statement is being interpreted, the
    // immediately enclosing With. Otherwise NULL.
    ILTree::WithBlock *m_EnclosingWith;

    // A mechanism for detecting cycles in constructor calls.
    Cycles *m_ConstructorCycles;

    // A mechanism to build the call graph for the given method.
    CallGraph *m_CallGraph;

    LookupTree *m_LookupCache;
    ExtensionMethodNameLookupCache * m_ExtensionMethodLookupCache;
    LiftedUserDefinedOperatorCache * m_LiftedOperatorCache;
    NamespaceRingTree *m_MergedNamespaceCache;
    bool m_DoNotMergeNamespaceCaches;
    CompilationCaches *m_CompilationCaches;

    CompilerProject *m_Project;

    // Data flow definite assignment members
    unsigned m_DefAsgCount;
    BITSET * m_DefAsgCurrentBitset;
    BITSET * m_DefAsgTempBitset;    // work set for temporary use
    BITSET * m_DefAsgErrorDisplayedBitset;
    DefAsgReEvalStmtList m_DefAsgReEvalStmtList;
    bool    m_DefAsgIsReachableCode;
    bool    m_DefAsgOnErrorSeen;
#if DEBUG
    DynamicArray<BCSYM_Variable*> m_DefAsgSlotAssignments;
#endif

    // List to store new symbols created during interpretation
    TransientSymbolStore *m_SymbolsCreatedDuringInterpretation;

    // Flag to determine whether we are converting nested lambdas for expression
    // trees. We need to handle these with special care.
    bool m_QuoteLambda;

    // If these two are set, it means that ConvertInternalToExpressionTree will replace
    // the expression with a reference to the variable.

    Variable* m_CoalesceLambdaParameter;
    ILTree::Expression* m_CoalesceArgumentToBeReplaced;

    Symbols m_TransientSymbolCreator;
    bool m_MergeAnonymousTypeTemplates;

    // Members that track closures during semantic analysis.
    ClosureRoot *m_closureRoot;

    // Whether or not we're currently in a lambda expression
    // WARNING!!! This is a delicate field. In its boolean sense (m_InLambda==0 means "not in a lambda"
    // while m_InLambda>0 means "in a lambda") it's robust enough. But its specific >0 values,
    // SingleLineLambda vs MultiLineSubLambda vs MultiLineFunctionLambda, have to do with where
    // long-lived temporaries are allocated during lambda body interpretation. For this reason, in some cases
    // the caller to CreateExpression will set this to MultiLineSubLambda even when a single-line
    // expression comes out at the end! The m---- is: only use this field in its boolean sense,
    // since it's other sense combines too many different side-effects.
    LambdaKind m_InLambda;

    // This instance of Semantics was created from ParseTreeService.
    bool m_CreatedInParseTreeService;

    // Whether or not we're currently in context of a Query Expression
    bool m_InQuery;

    // Whether or not a lambda was allocated during InterpretMethodBoy().  Allows
    // us to short circuit closue work if there aren't any lambda's
    bool m_methodHasLambda;

    // GroupId for Statements.  Used to track temporary flow between statements.  This
    // starts off at 1 and is incremented for every parsetree statement analyzed.
    unsigned m_statementGroupId;

    CSingleList<QueryMemberLookupVariable> m_QueryMemberLookupList;
    bool m_UseQueryNameLookup;
    LambdaBodyBuildKeyExpressions * m_JoinKeyBuilderList;

    // Temporary fix for Bug 36881 - DevDiv Bugs
    // We are disallowing some new features like Query Expressions and
    // Aggregate Initializers in context of a constant expression because
    // their interpretation allocates new symbols, which in some scenarios
    // may cause a crash later on. So, we need to track whether we are
    // in context of a constant expression and that is what this flag is used
    // for. Other non-constant expressions are still going to be allowed when
    // this flag is set to TRUE, but we are going to make it a warning (separate
    // bug is opened) for this release and convert it to an error for the next release.
    bool m_InConstantExpressionContext;

    FromItemsHashTable *m_FromItemsMap;

    // Indicates that a method body is being analyzed via a call to ::InterpretMethodBody
    //
    bool m_InterpretingMethodBody;

    // Used to help with anonymous type bindings, because we need to track properties
    // of anonymous types.

    AnonymousTypeBindingTable* m_AnonymousTypeBindingTable;

    BCSYM_NamedRoot* m_FieldInitializerContext;

    bool m_IsCreatingRelaxedDelegateLambda;


#if IDE 
protected:
    bool m_nameFoundInProjectImports;
public:
    bool WasNameFoundInProjectLevelImport()
    {
        return m_nameFoundInProjectImports;
    }
#endif

    FX::FXSymbolProvider* m_FXSymbolProvider;
    Type * m_EmbeddedTypeIdentity; // #546428 Used to lookup canonical interop type
};

class DefaultPartialGenericBindingFactory
{
public:
    DefaultPartialGenericBindingFactory
    (
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        _In_ Symbols * pSymbols
    );

    PartialGenericBinding * Execute
    (
        Type * pReceiverType,
        _In_ Location * pReceiverLocation,
        Procedure * pProc
    );

private:
    Compiler * m_pCompiler;
    CompilerHost * m_pCompilerHost;
    Symbols * m_pSymbols;
};

bool
IsRestrictedType
(
    BCSYM *Type,
    CompilerHost *CompilerHost
);

void
CheckRestrictedType
(
    ERRID errid,
    BCSYM *Type,
    Location *ErrorLocation,
    CompilerHost *CompilerHost,
    ErrorTable *Errors
);

void
CheckRestrictedArrayType
(
    BCSYM *Type,
    Location *ErrorLocation,
    CompilerHost *CompilerHost,
    ErrorTable *Errors
);

bool
IsRestrictedArrayType
(
    BCSYM *Type,
    CompilerHost *CompilerHost
);

inline bool
HasFlag32
(
    unsigned FlagsToSearchThrough,
    unsigned FlagsToSearchFor
)
{
    return (FlagsToSearchThrough & FlagsToSearchFor) != 0;
}

inline bool
HasFlag32
(
    ILTree::ILNode *TreeToTest,
    unsigned FlagToTest
)
{
    return (TreeToTest->uFlags & FlagToTest) != 0;
}

inline bool
HasFlag32
(
    ILTree::ILNode &TreeToTest,
    unsigned FlagToTest
)
{
    return (TreeToTest.uFlags & FlagToTest) != 0;
}

inline void
SetFlag32
(
    _Inout_ ILTree::ILNode *TreeToSet,
    unsigned FlagToSet
)
{
    TreeToSet->uFlags |= FlagToSet;
}

inline void
SetFlag32
(
    ILTree::ILNode &TreeToSet,
    unsigned FlagToSet
)
{
    TreeToSet.uFlags |= FlagToSet;
}

inline void
SetFlag
(
    unsigned __int64 &FlagWordToSet,
    unsigned __int64 FlagToSet
)
{
    FlagWordToSet |= FlagToSet;
}

inline void
SetFlag32
(
    _Inout_ unsigned &FlagWordToSet,
    unsigned FlagToSet
)
{
    FlagWordToSet |= FlagToSet;
}

inline void
ClearFlag32
(
    _Inout_ ILTree::ILNode *TreeToSet,
    unsigned FlagToSet
)
{
    TreeToSet->uFlags &= ~FlagToSet;
}

inline void
ClearFlag32
(
    ILTree::ILNode &TreeToSet,
    unsigned FlagToSet
)
{
    TreeToSet.uFlags &= ~FlagToSet;
}

inline void
ClearFlag32
(
    _Inout_ unsigned &FlagWordToSet,
    unsigned FlagToSet
)
{
    FlagWordToSet &= ~FlagToSet;
}

inline void
ClearFlag
(
    unsigned __int64 &FlagWordToSet,
    unsigned __int64 FlagToSet
)
{
    FlagWordToSet &= ~FlagToSet;
}

inline bool
HasFlag
(
    unsigned __int64 FlagWordToTest,
    unsigned __int64 FlagToTest
)
{
    return (FlagWordToTest & FlagToTest) != 0;
}

inline bool
IsConstant
(
    ILTree::Expression *TreeToTest
)
{
    return ILTree::BilopIsConst(TreeToTest->bilop);
}

// Returns true for any reference which pertains to Me (including MyBase and MyClass)

inline bool
IsMeReference
(
    ILTree::Expression *BaseReference
)
{
    // Check if the BaseReference symbol is marked as Me.
    // The symbol may be buried under an address node if Me is a reference to a value type.
    // See Semantics::MakeValueTypeOrTypeParamBaseReference.
    //
    return
        BaseReference &&
           ((BaseReference->bilop == SX_SYM &&
                BaseReference->AsSymbolReferenceExpression().Symbol->IsVariable() &&
                BaseReference->AsSymbolReferenceExpression().Symbol->PVariable()->IsMe()) ||
            (BaseReference->bilop == SX_ADR &&
                BaseReference->AsExpressionWithChildren().Left->bilop == SX_SYM &&
                BaseReference->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol->IsVariable() &&
                BaseReference->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol->PVariable()->IsMe()));
}

bool IsMyBaseMyClassSymbol(ILTree::Expression *expr);

// Determine if an interpreted statement or expression is bad.

inline bool
IsBad
(
    ILTree::ILNode *TreeToTest
)
{
    return (TreeToTest->uFlags & SF_ERROR) != 0;
}

inline bool
IsBad
(
    ILTree::ILNode &TreeToTest
)
{
    return (TreeToTest.uFlags & SF_ERROR) != 0;
}

// Determine if a declaration is bad.

inline bool
IsBad
(
    Symbol *DeclarationToTest
)
{
    return DeclarationToTest->IsBad();
}

inline ILTree::ILNode *
MakeBad
(
    _Inout_ ILTree::ILNode *TreeToSet
)
{
    TreeToSet->uFlags |= SF_ERROR;
    return TreeToSet;
}

inline void
MakeBad
(
    ILTree::ILNode &TreeToSet
)
{
    TreeToSet.uFlags |= SF_ERROR;
}

inline ILTree::Expression *
MakeBad
(
    ILTree::Expression *TreeToSet
)
{
    TreeToSet->uFlags |= SF_ERROR;
    return TreeToSet;
}

inline ILTree::Expression *
MakeBad
(
    _In_ ILTree::Expression &TreeToSet
)
{
    TreeToSet.uFlags |= SF_ERROR;
    return &TreeToSet;
}

inline void
SetResultType
(
    _Out_ ILTree::Expression *TreeToSet,
    Type *ResultType
)
{
    TreeToSet->ResultType = ResultType;
    TreeToSet->vtype = ResultType->GetVtype();
}

inline void
SetResultType
(
    ILTree::Expression &TreeToSet,
    Type *ResultType
)
{
    TreeToSet.ResultType = ResultType;
    TreeToSet.vtype = ResultType->GetVtype();
}

inline void
Semantics::MarkContainingLambdaOrMethodBodyBad
(
)
{
    // If we have a lambda block, mark it bad before we mark the 
    // procedure bad. This in turn will mark the lambda expression bad.
    // If the caller still wants to mark the method bad, they can call 
    // MarkContainingLambdaOrMethodBodyBad after we have completed interpretation of the lambda.

    if (m_StatementLambdaInterpreter)
    {
        // INVARIANT: if m_StatementLambdaInterpreter is non-null then m_Tree will be non-null.
        // This is established in LambdaBodyInterpretStatement::DoInterpretBody.
        if (m_StatementLambdaInterpreter->m_Tree)
        {
            MakeBad(m_StatementLambdaInterpreter->m_Tree);
        }
        else
        {
            VSFAIL("Unexpected: if m_StatementLambdaInterpreter!=NULL, then we should have its m_Tree that was set in LambdaBodyInterpretStatement::DoInterpretBody");
            // this codepath will never be taken thanks to the invariant. But, if it is, then it'd be nice
            // to at least mark something as bad.
            if (m_ProcedureTree)
            {
                MakeBad(m_ProcedureTree);
            }
        }
    }
    else if (m_ProcedureTree)
    {
        MakeBad(m_ProcedureTree);
    }
}

inline void
Semantics::MarkLambdaBodyBad
(
)
{
    if (m_StatementLambdaInterpreter)
    {
        if (m_StatementLambdaInterpreter->m_Tree)
        {
            MakeBad(m_StatementLambdaInterpreter->m_Tree);
        }
        else
        {
            VSFAIL("Unexpected: if m_StatementLambdaInterpreter!=NULL, then we should have its m_Tree that was set in LambdaBodyInterpretStatement::DoInterpretBody");
        }
    }
}

inline ILTree::Expression *
Semantics::InterpretStatementOperand
(
    ParseTree::Expression *Input,
    ExpressionFlags Flags
)
{
    ILTree::Expression *Result = InterpretExpression(Input, Flags);
    if (IsBad(Result))
    {
        MarkContainingLambdaOrMethodBodyBad();
    }

    return Result;
}

inline ILTree::Expression *
Semantics::InterpretStatementOperand
(
    ParseTree::Expression *Input,
    ExpressionFlags Flags,
    Type *TargetType,
    Type **OriginalType
)
{
    ILTree::Expression *Result = InterpretExpressionWithTargetType(Input, Flags | ExprForceRValue, TargetType, OriginalType);
    if (IsBad(Result))
    {
        MarkContainingLambdaOrMethodBodyBad();
    }

    return Result;
}


inline Type *
GetDataType
(
    Variable *Symbol
)
{
    return Symbol->GetType()->DigThroughAlias();
}

inline Type *
GetDataType
(
    Parameter *Symbol
)
{
    return Symbol->GetType()->DigThroughAlias();
}

inline Type *
GetReturnType
(
    Procedure *Symbol
)
{
    return Symbol->GetType()->DigThroughAlias();
}

bool
AllowsCompileTimeConversions
(
    Type *TargetType
);

bool
AllowsCompileTimeOperations
(
    Type *TargetType
);

inline bool
IsShiftOperator
(
    BILOP Opcode
)
{
    return Opcode == SX_SHIFT_LEFT || Opcode == SX_SHIFT_RIGHT;
}


ConversionClass
ClassifyPredefinedCLRConversion
(
    Type *TargetType,
    Type *SourceType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,    // allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                             // How many times have we recursively called ourself?
    bool IgnoreProjectEquivalence,                  // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,         // The project containing a component of type TargetType that was considered
                                                    // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType,         // The project containing a component of type SourceType that was considered
                                                    // for a successful match ignoring project equivalence.
    bool *pConversionIsNarrowingDueToAmbiguity,     // [out] did we return "Narrowing" for reason of ambiguity,
                                                    // e.g. multiple variance-conversions?
    bool* pNarrowingCausedByContraVariance = NULL
);

ConversionClass
ClassifyCLRReferenceConversion
(
    Type *TargetType,
    Type *SourceType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    bool IgnoreProjectEquivalence,
    CompilerProject **ProjectForTargetType,
    CompilerProject **ProjectForSourceType,
    bool* pNarrowingCausedByContraVariance = NULL
);

ConversionClass
ClassifyAnyVarianceConversionToInterface
(
    Type *TargetType,
    Type *SourceType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    bool IgnoreProjectEquivalence,
    CompilerProject **ProjectForTargetType,
    CompilerProject **ProjectForSourceType
);

bool
HasVarianceConversionToDelegate
(
    Type *TargetType,
    Type *SourceType,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost
);

ConversionClass
ClassifyVarianceConversionToDelegate
(
    Type *TargetType,
    Type *SourceType,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    bool* bNarrowingCausedByContraVariance = NULL
);


// VarianceParameterCompatibility: This structure is output from ClassifyImmediateVarianceCompatibility,
// which is used to judge whether one binding G(Of S1,...) is variance-convertible to another binding G(Of D1,...),
// for some generic type G(Of T1,...). One of these structures it output for each "Si/Di/Ti", plus a note
// of whether the variance-convertibility on Si/Di worked or not.
struct VarianceParameterCompatibility
{
    bool Compatible;            // Was SourceArgument "Si" compatible with TargetArgument "Ti"?
    BCSYM *SourceArgument;      // "Si"     
    BCSYM *TargetArgument;      // "Di"
    BCSYM_GenericParam *Param;  // "Ti"
    BCSYM_NamedRoot *Generic;   // This is the particular generic Gx(Of ...) that contains Ti.
                                // NB. an arbitrary binding has the general form GX(Of ...).GY(Of ...).GZ(Of ...)
};

ConversionClass
ClassifyImmediateVarianceCompatibility
(
    Type *TargetType,
    Type *SourceType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    bool IgnoreProjectEquivalence = NULL,
    CompilerProject **ProjectForTargetType = NULL,
    CompilerProject **ProjectForSourceType = NULL,
    DynamicArray<VarianceParameterCompatibility> *ParameterDetails = NULL,
    bool *bNorrawingCausedByContraVariance = NULL
);

ConversionClass
ClassifyCLRConversionForArrayElementTypes
(
    Type *TargetElementType,
    Type *SourceElementType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                         // How many times have we recursively called ourselves?
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType      // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.                                    
);

bool
AreAssembliesEqualByName
(
    CompilerProject *Project1,
    CompilerProject *Project2
);

STRING *
GetAssemblyName
(
    CompilerProject *Project
);

STRING *
GetErrorProjectName
(
    CompilerProject *Project
);

void __cdecl
ReportSmartReferenceError
(
    unsigned ErrorId,
    _In_ CompilerProject *ReferencingProject,
    CompilerProject *RequiredReferenceProject,
    Compiler *Compiler,
    ErrorTable *ErrorTable,
    _In_z_ WCHAR *Extra,
    const Location *ErrorLocation,
    ...
);

// Methods related to generics.

// Calls the other overload of RefersToGenericParameter but ensures enough
// memory is allocated for the list of generic params found.
bool
RefersToGenericParameter
(
    Type                *PossiblyGenericType,
    _Inout_ NorlsAllocator      *allocator,
    _Out_ BCSYM_GenericParam  **&ListOfGenericParamsFound,
    _Out_ unsigned            *NumberOfGenericParamsFound
);

// Determine whether a given type refers to a generic parameter.
// If GenericParam is specified,
//    the result is true only if the type refers to that generic parameter
// Otherwise
//    If Generic is specified,
//        the result is true only if the type refers to a parameter of Generic.
// Otherwise
//    the result is true if the type refers to any generic parameters at all.
//
bool
RefersToGenericParameter
(
    Type *PossiblyGenericType,
    Declaration *Generic = NULL,
    GenericParameter *GenericParam = NULL,
    _Out_opt_cap_(ListSize) BCSYM_GenericParam **ListOfGenericParamsFound = NULL,
    unsigned ListSize = 0,
    _Inout_ unsigned *NumberOfGenericParamsFound = NULL,
    bool IgnoreMethodTypeParams = false,
    IReadonlyBitVector * pFixedParameterBitVector = NULL
);

bool
RefersToGenericParameter
(
    Type *PossiblyGenericType,
    Declaration *Generic,
    IReadonlyBitVector * pFixedParameterBitVector
);


bool
IsGenericOrHasGenericParent
(
    Declaration *Decl
);

inline bool
IsGeneric
(
    Symbol * Sym
)
{
    return Sym->IsNamedRoot() && Sym->PNamedRoot()->IsGeneric();
}

// For a type that might make reference to one or more generic parameters, return the
// equivalent type with the generic parameters replaced with generic arguments.
Type *
ReplaceGenericParametersWithArguments
(
    Type *PossiblyGenericType,
    GenericBindingInfo GenericBindingContext,
    _In_ Symbols &SymbolCreator
);

inline Type*
Semantics::TypeInGenericContext
(
    Type *T,
    GenericBinding *GenericContext
)
{
    return ReplaceGenericParametersWithArguments(T, GenericContext, m_SymbolCreator);
}

// Return the generic type binding applicable to referring to a declaration that is a member of
// a possibly generic type (or a base type of that type).
// A non-null result occurs if the type owning the declaration is a binding of a generic type or
// is declared within a generic type and is referred to through a binding of that type.
GenericTypeBinding *
DeriveGenericBindingForMemberReference
(
    Type *PossiblyGenericType,
    Declaration *Member,
    _In_ Symbols &SymbolCreator,
    CompilerHost *CompilerHost
);

// Given an argument type, a parameter type, and a set of (possibly unbound) type arguments
// to a generic method, infer type arguments corresponding to type parameters that occur
// in the parameter type.
//
// A return value of false indicates that inference fails.
bool
InferTypeArgumentsFromArgument
(
    const Location *ArgumentLocation,
    Type *ArgumentType,
    bool ArgumentTypeByAssumption,
    Type *ParameterType,
    Parameter *Param,
    _In_opt_ Semantics::TypeInference* TypeInference,
    _Inout_ Type **TypeInferenceArguments,
    _Out_opt_ bool *TypeInferenceArgumentsByAssumption,   // can be NULL
    _Out_opt_ Location *InferredTypeArgumentLocations,    // can be NULL
    _Out_opt_ Parameter **ParamsInferredFrom,                 // can be NULL
    _Out_opt_ GenericParameter **InferenceFailedTypeParam,  // can be NULL
    _Out_opt_ Type **InferredTypeArgument,                // can be NULL
    Procedure *TargetProcedure,
    Compiler *CompilerInstance,
    CompilerHost *CompilerHostInstance,
    Symbols &SymbolCreator,
    MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
    ConversionRequiredEnum InferenceRestrictions,
    IReadonlyBitVector * pFixedParameterBitVector = NULL
);

bool
InferBaseSearchTypeFromArray
(
    _In_ Type* BaseSearchType,
    _In_ Type* FixedType,
    _In_ CompilerHost *CompilerHostInstance,
    _Inout_ DynamicArray<GenericTypeBinding *> &MatchingGenericBindings,
    Symbols &SymbolCreator,
    FX::TypeName InterfaceName
);

// Given an argument type, a parameter type, and a set of (possibly unbound) type arguments
// to a generic method, infer type arguments corresponding to type parameters that occur
// in the parameter type.
//
// A return value of false indicates that inference fails.
bool
InferTypeArgumentsFromArgumentDirectly
(
    const Location *ArgumentLocation,
    Type *ArgumentType,
    bool ArgumentTypeByAssumption,
    Type *ParameterType,
    Parameter *Param,
    _In_opt_ Semantics::TypeInference* TypeInference,
    _Inout_ Type **TypeInferenceArguments,
    _Out_opt_ bool *TypeInferenceArgumentsByAssumption,
    _Out_opt_ Location *InferredTypeArgumentLocations,
    _Out_ Parameter **ParamsInferredFrom,
    _Out_opt_ GenericParameter **TypeParamWithInferenceConflict,
    _Out_opt_ Type **InferredTypeArgument,
    Procedure *TargetProcedure,
    Compiler *CompilerInstance,
    CompilerHost *CompilerHostInstance,
    Symbols &SymbolCreator,
    MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
    ConversionRequiredEnum InferenceRestrictions,
    IReadonlyBitVector * pFixedParameterBitVector = NULL
);

// Combining inference restrictions: the least upper bound of the two restrictions
ConversionRequiredEnum
CombineConversionRequirements
(
    ConversionRequiredEnum restriction1,
    ConversionRequiredEnum restriction2
);

// Strengthens the restriction to at least ReferenceRestriction or ReverseReferenceRestriction
// Note: AnyConversionAndReverse strengthens to Identity
ConversionRequiredEnum
StrengthenConversionRequirementToReference
(
    ConversionRequiredEnum restriction
);

// Invert the sense of the restriction, e.g. from "ReverseReference" to "Reference".
// Note: ArrayElementConversion has no inverse, and fails with a VSASSERT.
// Don't call it with this.
ConversionRequiredEnum
InvertConversionRequirement
(
    ConversionRequiredEnum restriction
);

// Given a generic, synthesize a binding that uses the parameters as arguments.
GenericBinding *
SynthesizeOpenGenericBinding
(
    Declaration *Generic,
    _In_ Symbols &SymbolCreator
);

// Given a generic parameter, test if it has a constraint that is a class type.
inline bool
HasClassConstraint
(
    GenericParameter *P
)
{
    return P->HasClassConstraint(false);
}

inline Type*
GetClassConstraint
(
    GenericParameter *P,
    CompilerHost *CompilerHost,
    NorlsAllocator *Allocator,
    bool ReturnArraysAsSystemArray,
    bool ReturnValuesAsSystemValueTypeOrEnum
)
{
    return
        P->GetClassConstraint(
            CompilerHost,
            Allocator,
            ReturnArraysAsSystemArray,
            ReturnValuesAsSystemValueTypeOrEnum);
}

inline bool
IsMethodGenericParamHash
(
    Scope *Lookup
)
{
    return
        (Lookup &&
         Lookup->GetImmediateParent() &&
         Lookup->GetImmediateParent()->IsProc() &&
         Lookup->GetImmediateParent()->PProc()->GetGenericParamsHash() == Lookup);
}

// Narrow a quadword result to a specific integral type, setting Overflow true
// if the result value cannot be represented in the result type.
Quadword
NarrowIntegralResult
(
    Quadword SourceValue,
    Type *SourceType,
    Type *ResultType,
    bool &Overflow
);

Quadword
NarrowIntegralResult
(
    Quadword SourceValue,
    Vtypes vtSourceType,
    Vtypes vtResultType,
    bool &Overflow
);

// Narrow a double result to a specific floating type, setting Overflow true
// if the result value cannot be represented in the result type.
double
NarrowFloatingResult
(
    double Result,
    Vtypes vtResultType,
    bool &Overflow
);

double
NarrowFloatingResult
(
    double Result,
    Type *ResultType,
    bool &Overflow
);

// If Input is any sort of alias, deferred or not, chase through to a non-alias.
// If Input is not an alias, return Input.

Declaration *
ChaseThroughAlias
(
    Symbol *Input
);

inline bool
IsType
(
    Declaration *Input
)
{
    Input = ChaseThroughAlias(Input);
    return Input->IsClass() || Input->IsInterface();
}

inline bool
IsConstant
(
    Declaration *Input
)
{
    Input = ChaseThroughAlias(Input);
    return Input->IsVariable() && Input->PVariable()->IsConstant();
}

inline bool
IsVariable
(
    Declaration *Input
)
{
    return ChaseThroughAlias(Input)->IsVariable();
}

inline bool
IsProcedure
(
    Symbol *Input
)
{
    return ChaseThroughAlias(Input)->IsProc();
}

inline bool
IsProcedureDefinition
(
    Declaration *Input
)
{
    return ChaseThroughAlias(Input)->IsMethodImpl();
}

inline Procedure *
ViewAsProcedure
(
    Symbol *Input
)
{
    return ChaseThroughAlias(Input)->PProc();
}

inline Procedure *
ViewAsProcedureHandlingNulls
(
    Symbol * Input
)
{
    if (! Input)
    {
        return NULL;
    }
    else
    {
        return ViewAsProcedure(Input);
    }
}

inline ProcedureDefinition *
ViewAsProcedureDefinition
(
    Declaration *Input
)
{
    return ChaseThroughAlias(Input)->PMethodImpl();
}

// 

inline bool
HasUserDefinedOperators
(
    Type *T
)
{
    return TypeHelpers::IsClassOrRecordType(T) && T->PClass()->HasUserDefinedOperators();
}

inline bool
CanTypeContainUserDefinedOperators
(
    Type *T
)
{
    return
        TypeHelpers::IsClassOrRecordType(T) ||
        (TypeHelpers::IsGenericParameter(T) && HasClassConstraint(T->PGenericParam()));
}

inline bool
IsUserDefinedOperator
(
    Declaration *Input
)
{
    Input = ChaseThroughAlias(Input);
    return Input->IsUserDefinedOperator();
}

inline bool
IsConversionOperator
(
    Declaration *Input
)
{
    Input = ChaseThroughAlias(Input);
    return Input->IsUserDefinedOperator() && IsConversionOperator(Input->PUserDefinedOperator()->GetOperator());
}

inline bool
IsProperty
(
    Symbol *Input
)
{
    return ChaseThroughAlias(Input)->IsProperty();
}

inline bool
IsReadableProperty
(
    Declaration *Input
)
{
    return
        IsProperty(Input) &&
        !ChaseThroughAlias(Input)->PProperty()->IsWriteOnly();
}

inline bool
IsPropertyMethod
(
    Procedure *P
)
{
    return !P->IsProcedure();
}

inline bool
IsPropertySet
(
    Procedure *P
)
{
    return P->IsPropertySet();
}

inline bool
IsPropertyGet
(
    Procedure *P
)
{
    return P->IsPropertyGet();
}

// Return the relevant property method if Property has a method that
// matches the conditions given by the flags
// (e.g. if Property has a Set and Flags includes ExprIsPropertyAssignment),
// and return NULL otherwise.

Procedure *
MatchesPropertyRequirements
(
    Procedure *Property,
    ExpressionFlags Flags
);

Procedure *
ResolveOverriddenProperty
(
    Procedure *Target,
    ExpressionFlags Flags,
    bool &ResolvedToDifferentTarget
);

inline bool
IsPropertyReference
(
    ILTree::Expression *Result
)
{
    return
        Result->bilop == SX_PROPERTY_REFERENCE ||
        Result->bilop == SX_LATE_REFERENCE;
}

inline bool
IsLateReference
(
    ILTree::Expression *Result
)
{
    return Result->bilop == SX_LATE_REFERENCE;
}

// Return true if an Expression computes a result that will be stored in a
// temporary location.

inline bool
CreatesIntermediateValue
(
    ILTree::Expression *Result
)
{
    return Result->bilop != SX_SYM && Result->bilop != SX_INDEX;
}

// Return true if and only if an expression is a literal Nothing. A value
// of Nothing converted to a specific type does not qualify.
inline bool
IsNothingLiteral
(
    _In_ ILTree::Expression *Value
)
{
    return Value->bilop == SX_NOTHING && TypeHelpers::IsRootObjectType(Value->ResultType);
}


// Return true if and only if an expression is a integral literal with a value of zero.
inline bool
IsIntegerZeroLiteral
(
    _In_ ILTree::Expression *Value
)
{
    return
        TypeHelpers::IsIntegerType(Value->ResultType) &&
        Value->bilop == SX_CNS_INT &&
        Value->AsIntegralConstantExpression().Value == 0 &&
        HasFlag32(Value, SXF_ICON_LITERAL);
}

inline bool
IsDecimalZeroValue
(
    ILTree::DecimalConstantExpression &Tree
)
{
    return
        Tree.Value.scale == 0 &&
        Tree.Value.Hi32 == 0 &&
        Tree.Value.Mid32 == 0 &&
        Tree.Value.Lo32 == 0;
}

inline bool
IsDecimalZeroOrOneValue
(
    ILTree::DecimalConstantExpression &Tree
)
{
    // This returns true for -1, 0, 1
    return
        Tree.Value.scale == 0 &&
        Tree.Value.Hi32 == 0 &&
        Tree.Value.Mid32 == 0 &&
        (Tree.Value.Lo32 == 0 || Tree.Value.Lo32 == 1);
}

inline bool
IsEvent
(
    Declaration *D
)
{
    return D->IsEventDecl();
}

inline bool
IsFunction
(
    Procedure *P
)
{
    return P->GetType() != NULL && !IsProperty(P) && !IsPropertyMethod(P) && !IsEvent(P);
}

inline bool
IsFunction
(
    Declaration *D
)
{
    return IsProcedure(D) && IsFunction(ViewAsProcedure(D));
}

inline bool
IsSub
(
    Procedure *P
)
{
    return P->GetType() == NULL && !IsProperty(P) && !IsPropertyMethod(P) && !IsEvent(P);
}

inline bool
IsInstanceParamlessSub
(
    Declaration *D
)
{
    return
        IsProcedure(D) &&
        !ViewAsProcedure(D)->IsShared() &&
        IsSub(ViewAsProcedure(D)) &&
        ViewAsProcedure(D)->GetFirstParam() == NULL;
}

inline Scope *
ViewAsScope
(
    Container *Lookup
)
{
    return Lookup->GetHash();
}

inline bool
IsClassType
(
    Scope *Lookup
)
{
    Symbol *Parent = Lookup->GetImmediateParent();
    return Parent->IsClass() && Lookup != Parent->PClass()->GetGenericParamsHash();
}

inline bool
IsInterface
(
    Scope *Lookup
)
{
    Symbol *Parent = Lookup->GetImmediateParent();
    return Parent->IsInterface() && Lookup != Parent->PInterface()->GetGenericParamsHash();
}

inline bool
IsMethodBody
(
    Scope *Lookup
)
{
    Symbol *Parent = Lookup->GetImmediateParent();
    return Parent->IsProc() && Lookup != Parent->PProc()->GetGenericParamsHash();
}

inline bool
IsNamespace
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent()->IsNamespace();
}

inline bool
IsNamespace
(
    Symbol *SymbolToTest
)
{
    return SymbolToTest->IsNamespace();
}

inline ClassOrRecordType *
ViewAsClass
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent()->PClass();
}

inline InterfaceType *
ViewAsInterface
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent()->PInterface();
}

inline Type *
ViewAsType
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent();
}

inline Procedure *
ViewAsMethodBody
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent()->PProc();
}

inline Namespace *
ViewAsNamespace
(
    Scope *Lookup
)
{
    return Lookup->GetImmediateParent()->PNamespace();
}

inline bool
Semantics::WithinProcedure
(
)
{
    return m_ProcedureTree != NULL;
}

inline bool
Semantics::WithinSharedProcedure
(
)
{
    return m_Procedure != NULL && m_Procedure->IsShared();
}

inline bool
Semantics::WithinInstanceProcedure
(
)
{
    return m_Procedure != NULL && !m_Procedure->IsShared();
}

inline bool
Semantics::WithinSyntheticCode
(
)
{
    // Don't treat synthetic constructors as being synthetic code
    // because except for a few scenarios, it is usually user specified
    // initializer code that is put in them. The few scenarios where
    // synthetic code is generated in the constructor are handled
    // specially.
    //
    return m_Procedure != NULL &&
           m_Procedure->IsSyntheticMethod() &&
           !m_Procedure->IsAnyConstructor();
}

inline bool
Semantics::PerformObsoleteChecks
(
)
{
    return m_PerformObsoleteChecks && m_ReportErrors;
}

inline Container *
Semantics::ContainerContextForObsoleteSymbolUsage
(
)
{
    if (m_NamedContextForAppliedAttribute)
    {
        return
            m_NamedContextForAppliedAttribute->IsContainer() ?
                m_NamedContextForAppliedAttribute->PContainer() :
                m_NamedContextForAppliedAttribute->GetContainer();
    }

    return ContainingContainer();
}

// If an expression refers to a symbol, return the referenced symbol. Otherwise,
// return NULL.
Declaration *
ReferencedSymbol
(
    ILTree::Expression *Input,
    bool AssertIfReturnNULL = true
);

// Determine whether or not a namespace is available for use by a particular
// project. (A namespace is available if its in the same project or in a
// referenced project.)
bool
NamespaceIsAvailable
(
    CompilerProject *LookupProject,
    Namespace *NamespaceToTest
);

bool
DeclarationIsAvailable
(
    CompilerProject *LookupProject,
    Declaration *DeclarationToTest
);

inline bool
Semantics::NamespaceIsAvailableToCurrentProject
(
    Namespace *NamespaceToTest
)
{
    return NamespaceIsAvailable(m_Project, NamespaceToTest);
}

inline bool
Semantics::DeclarationIsAvailableToCurrentProject
(
    Declaration *DeclarationToTest
)
{
    return DeclarationIsAvailable(m_Project, DeclarationToTest);
}

int
GetShiftSizeMask
(
    Vtypes Type
);

// If Input is an expression with a type character, return the type character.
// Otherwise, return chType_NONE.

typeChars
ExtractTypeCharacter
(
    ParseTree::Expression *Input
);

// Returns the number of elements in a list.
unsigned
ExpressionListLength
(
    ExpressionList *List
);

// For an expression that represents a string constant, return the length
// of the string. (The input expression can be SX_CNS_STR or SX_NOTHING.)

size_t
GetStringLength
(
    ILTree::Expression *String
);

// For an expression that represents a string constant, return the spelling
// of the string. (The input expression can be SX_CNS_STR or SX_NOTHING
// (which produces L"").)

WCHAR *
GetStringSpelling
(
    ILTree::Expression *String
);

bool
DetectFloatingToIntegralOverflow
(
    double SourceValue,
    bool IsUnsigned
);

Quadword
ConvertFloatingToUI64
(
    double SourceValue
);

// Determine if an instance of a type can be created with New. Returns zero
// for success, and an error id for failure.
unsigned
CheckConstraintsOnNew
(
    _Inout_ Type *&TypeOfInstance
);


bool
IsInvalidDoubleValue
(
    double Value
);

struct MemberInfoList
{
    ParseTree::IdentifierDescriptor *Name;
    ULONG Index;
    ParseTree::Initializer *Initializer;
    Location TextSpan;
    ILTree::Expression *InferredInitializer;
    bool IsBadInitializer;
    bool IsNameBangQualified;
    bool FieldIsKey;
    MemberInfoList *Next;

    MemberInfoList
    (
        _In_ ParseTree::IdentifierDescriptor *MemberName,
        ULONG MemberIndex,
        ParseTree::Initializer *MemberInitializer,
        bool BangQualified,
        bool Key,
        _In_ const Location &MemberLoc
    ) :
        Name(MemberName),
        Index(MemberIndex),
        Initializer(MemberInitializer),
        TextSpan(MemberLoc),
        InferredInitializer(NULL),
        IsBadInitializer(false),
        IsNameBangQualified(BangQualified),
        FieldIsKey(Key)
    {
    }
};

bool
MatchAnonymousType
(
    ClassOrRecordType *AnonymousClass,
    MemberInfoList *MemberInfos,
    SourceFile *SourceFile,
    unsigned FieldCount,
    bool CheckLocation
);

#if IDE 
bool
MatchAnonymousType
(
    Container *AnonymousType1,
    Container *AnonymousType2
);
#endif

DWORD
GetAnonymousTypeHashCode
(
    MemberInfoList *MemberInfos,
    ULONG FieldCount,
    bool CheckLocation
);

bool
MatchAnonymousDelegate
(
    BCSYM_Param*    InvokeParameters,
    Type*           InvokeResultType,
    SourceFile*     InvokeSourceFile,
    Location*       InvokeLocation,

    BCSYM_Param*    LambdaParameters,
    bool            LamdbdaIsFunction,
    SourceFile*     LambdaSourceFile,
    Location*       LambdaLocation,

    bool            CheckLocation
);

#if IDE 
bool
MatchAnonymousDelegate
(
    Container *AnonymousDelegate1,
    Container *AnonymousDelegate2
);
#endif

bool
IsCircularTypeReference
(
    Type *AnonymousType,
    Type *MemberType
);

struct OverloadList
{
    Declaration *Candidate;
    GenericBindingInfo Binding;
    OverloadList *Next;
    bool ParamArrayExpanded;
    bool RequiresNarrowingConversion;
    bool AllNarrowingIsFromObject;
    bool AllNarrowingIsFromNumericLiteral;
    bool AllFailedInferenceIsDueToObject;
    DelegateRelaxationLevel DelegateRelaxationLevel;
    TypeInferenceLevel TypeInferenceLevel;
    bool InferenceFailed;
    bool NotCallable;
    bool LessSpecific;
    bool ArgumentMatchingDone;
    unsigned long PrecedenceLevel;
    bool RequiresUnwrappingNullable;
    //Indicates wether or not the candidate is an extension method.
    //This is currently used when reporting overload resolution
    //candidate error messages to select between extension
    //method candidate errors and instance method candidate errors.
    bool IsExtensionMethod;
    //Indicates wether or not a not callable canidate allows
    //extension methods to be bound to. By default the value is true
    //and will not be set to false if an error is encountered in the candidate signature
    //that allows extension methods to be bound against.
    //This value only makes sense when IsExtensionMethod is false and NotCallable is true.
    //do not use it otherwise.
    bool RequiresInstanceMethodBinding;
    bool UsedDefaultForAnOptionalParameter;

    OverloadList
    (
        Declaration *Candidate,
        GenericBindingInfo Binding,
        _In_ OverloadList *Next,
        bool ParamArrayExpanded,
        unsigned long Precedence = 0
    ) :
        Candidate(Candidate),
        Binding(Binding),
        Next(Next),
        ParamArrayExpanded(ParamArrayExpanded),
        RequiresNarrowingConversion(false),
        AllNarrowingIsFromObject(true),
        AllNarrowingIsFromNumericLiteral(true),
        AllFailedInferenceIsDueToObject(true),
        DelegateRelaxationLevel(DelegateRelaxationLevelNone),
        TypeInferenceLevel(TypeInferenceLevelNone),
        InferenceFailed(false),
        NotCallable(false),
        LessSpecific(false),
        ArgumentMatchingDone(false),
        PrecedenceLevel(Precedence),
        RequiresUnwrappingNullable(false),
        IsExtensionMethod(false),
        RequiresInstanceMethodBinding(true),
        UsedDefaultForAnOptionalParameter(false)
    {
    }
};

struct OverloadListPair
{
    OverloadList * InstanceMethodCandidates;
    OverloadList * MixedCandidates;
};

class CallGraph :
    public SparseGraphT<BCSYM_Proc *>
{
public:
    CallGraph(Compiler *pCompiler):
        SparseGraphT<BCSYM_Proc *>(),
        m_iSourceIndex(0)
    {
    }

    void SetSource(BCSYM_Proc *pSourceNode)
    {
        AssertIfNull(pSourceNode);

        m_iSourceIndex = GetNodeIndex(&pSourceNode);
    }

    void AddCall(BCSYM_Proc *pTargetNode)
    {
        AssertIfNull(pTargetNode);

        InsertEdge(m_iSourceIndex, GetNodeIndex(&pTargetNode));
    }

private:
    ULONG m_iSourceIndex;
};

class TypeSet
{
public:

    TypeSet
    (
        unsigned long Size,
        _Inout_ NorlsAllocator &Allocator
    ) :
        // m_Allocator(Allocator),
        m_Count(0),
        m_Size(Size)
    {
        VSASSERT(m_Size > 0, "Creating a typeset with size zero?");
        if (!VBMath::TryMultiply(Size, sizeof(Type*)))
        {
            VSASSERT(m_Size > 0, "Typeset size is too large");
            return;
        }

        if (Size > INITIAL_SIZE)
        {
            m_Buffer = (Type**)Allocator.Alloc(Size * sizeof(Type*));
        }
        else
        {
            // memset here to clear the buffer?
            m_Buffer = m_BufferScratch;
        }
    };

    void
    Add
    (
        Type *Item
    )
    {
        if (m_Count < m_Size)
        {
            for (unsigned long Current = 0; Current < m_Count; Current++)
            {
                if (Item == m_Buffer[Current])
                {
                    return;
                }
            }

            m_Buffer[m_Count] = Item;
            m_Count++;
        }
        else
        {
            VSFAIL("TypeSet: Adding more items than size allows!");
        }
    }

    /* Type *
    Item
    (
        unsigned long Index
    )
    {
        return (Index < m_Size) ? m_Buffer[Index] : NULL;
    } */

    unsigned long
    Count()
    {
        return m_Count;
    }

    void
    Clear()
    {
        m_Count = 0;
    }

private:

    static const unsigned long INITIAL_SIZE = 4;  // 

    // NorlsAllocator &m_Allocator;
    unsigned long m_Size;
    unsigned long m_Count;
    Type **m_Buffer;
    Type *m_BufferScratch[INITIAL_SIZE];

    friend class TypeSetIter;
};

class TypeSetIter
{
public:

    TypeSetIter
    (
        _In_ TypeSet *Set
    )
    {
        m_Buffer = Set->m_Buffer;
        m_Cursor = Set->m_Count;
    }

    Type *
    Next()
    {
        if (m_Cursor == 0)
        {
            return NULL;
        }
        else
        {
            m_Cursor--;
            return m_Buffer[m_Cursor];
        }
    }

private:
    Type **m_Buffer;
    unsigned long m_Cursor;
};

// Don't give a warning on the zero-sized array within the structure.
#pragma warning( disable : 4200 )
class BITSET {

public:
    // Use DWORD_PTRs here so everything is the same size
    class BITSETIMP {
    public:
        DWORD_PTR * arrayEnd;  // pointer to the next dword after the array ends...
        DWORD_PTR bitArray[0]; // actually longer...
    };
    DWORD_PTR bits;
    BITSET() {};

public:

    static void swap(BITSET *** bs1, BITSET *** bs2);

    unsigned __fastcall getSize();

    BITSET * __fastcall setBit(unsigned bit);
    BITSET * __fastcall setBits(unsigned start, unsigned size);
    unsigned __fastcall numberSet();
    DWORD_PTR __fastcall testBit(unsigned bit);
    DWORD_PTR __fastcall testBits(unsigned start, unsigned size);
    bool __fastcall isZero();
    BITSET * __fastcall orInto(BITSET * source);
    BITSET * __fastcall orInto(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall andInto(BITSET * source);
    BITSET * __fastcall andInto(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall setInto(BITSET * source);
    BITSET * __fastcall setInto(BITSET * source, NorlsAllocator * alloc, DWORD_PTR * changed);
    BITSET * __fastcall setInto(BITSET * source, NorlsAllocator * alloc);
    BITSET * __fastcall setInto(int value);
    static BITSET * __fastcall create(unsigned size, NorlsAllocator * alloc, int init);
    static BITSET * __fastcall createCopy(BITSET * source, NorlsAllocator * alloc);
    static BITSET * __fastcall createCopy(BITSET * source, void * space);
    bool __fastcall isEqual(BITSET * other);
    BITSET * __fastcall andIntoOrCopy(BITSET * source, DWORD_PTR * changed, NorlsAllocator * alloc);
    BITSET * __fastcall andIntoOrCopy(BITSET * source, NorlsAllocator * alloc);
    BITSET * __fastcall orIntoOrCopy(BITSET * source, DWORD_PTR * changed, NorlsAllocator * alloc);

    // used for serialization
    unsigned __fastcall sizeInBytes();
    static BYTE *   __fastcall rawBytes(BITSET **bitset);
    static unsigned __fastcall rawSizeInBytes(unsigned numberOfBits);

private:

    static unsigned __fastcall numberSet(DWORD_PTR dw);
    BITSET * __fastcall setBitLarge(unsigned bit);
    BITSET * __fastcall setBitsLarge(unsigned start, unsigned size);
    DWORD_PTR __fastcall testBitLarge(unsigned bit);
    DWORD_PTR __fastcall testBitsLarge(unsigned start, unsigned size);
    bool __fastcall isZeroLarge();
    BITSET * __fastcall orIntoLarge(BITSET * source);
    BITSET * __fastcall orIntoLarge(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall andIntoLarge(BITSET * source);
    BITSET * __fastcall andIntoLarge(BITSET * source, DWORD_PTR * changed);
    BITSET * __fastcall setIntoLarge(BITSET * source, NorlsAllocator * alloc, DWORD_PTR * changed);
    BITSET * __fastcall setIntoLarge(BITSET * source);
    BITSET * __fastcall setIntoLarge(BITSET * source, NorlsAllocator * alloc);
    BITSET * __fastcall setIntoLarge(int value);
    static BITSET * __fastcall createLarge(unsigned size, NorlsAllocator * alloc, int init);
    static BITSET * __fastcall createCopyLarge(BITSET * source, NorlsAllocator * alloc);
    static BITSET * __fastcall createCopyLarge(BITSET * source, void * space);
    bool __fastcall isEqualLarge(BITSET * other);
};


//////////////////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN64)

C_ASSERT(sizeof(void*) == 8);

#define BITSET_BIT_SHIFT        6
#define BITSET_BIT_MASK         63
#define BITSET_MAX_SIZE         64


#else

C_ASSERT(sizeof(void*) == 4);

#define BITSET_BIT_SHIFT        5   // Shift by this much
#define BITSET_BIT_MASK         31  // This is how many bits can go in a 'slot'
#define BITSET_MAX_SIZE         32  // This is the size of a 'slot'

#endif

#define BITSET_ALL              -1

#define createBitsetOnStack(ref, source) \
{ \
    if (((DWORD_PTR)(source)) & 1) {  \
        (ref) = (source);  \
    } else { \
        unsigned size = (source)->getSize(); \
        void * space = _alloca(size); \
        (ref) = BITSET::createCopy((source), space); \
    } \
}

//////////////////////////////////////////////////////////////////////////////////////////////

#define createNewBitsetOnStack(name,size,init) \
BITSET * name; \
{ \
    if ((size) <= BITSET_BIT_MASK) { \
        name = (BITSET *) ((DWORD_PTR)(init) | 1); \
    }  else { \
        unsigned rawSize = BITSET::rawSizeInBytes((unsigned) size); \
        BITSET::BITSETIMP * rval = (BITSET::BITSETIMP*) _alloca(sizeof(BITSET::BITSETIMP) + rawSize); \
        ASSERT(!(((DWORD_PTR)rval) & 1)); \
        memset(rval->bitArray, init,  rawSize); \
        rval->arrayEnd = rval->bitArray + (rawSize / sizeof (DWORD_PTR)); \
        name = (BITSET*) rval; \
    } \
}

#pragma warning( default : 4200 )

class BackupAllocator : public BackupState<Symbols *, NorlsAllocator *>
{
public:
    BackupAllocator(Symbols * pSymbols) :
      BackupState(pSymbols, pSymbols ? pSymbols->GetNorlsAllocator() : NULL)
    {
    };

    ~BackupAllocator()
    {
        DoRestore();
    }
protected:
    void DoRestore()
    {
        if (m_savedValue)
        {
            m_constructorArg->SetNorlsAllocator(m_savedValue);
        }
    }
};


// SaveArguments, MakeScratchCopiesOfArguments, RestoreArguments: these helper functions
// are for people who call MatchArguments (which modifies the "IsBad" flag on the argument nodes.)
// So if we want a non-mutating test of MatchArguments, we have to save arguments
// and then restore them.
// The idea: first call SaveArguments to allocate space. Then call MakeScratchCopiesOfArguments
// each time you need to copy arguments into that space. And call Restore at the end.

void SaveArguments(
    _Inout_ NorlsAllocator &TreeStorage,
    ILTree::Expression *ArgumentsScratch[],
    unsigned ScratchCount,
    _Out_ _Deref_post_cap_(ArgumentCount) ILTree::Expression **&SavedArguments,
    ExpressionList *Arguments,
    unsigned ArgumentCount);

void
MakeScratchCopiesOfArguments
(
    BILALLOC &TreeAllocator,
    _In_opt_ ILTree::Expression *SavedArguments[],
    ExpressionList *Arguments
);

void RestoreOriginalArguments(
    _In_ ILTree::Expression *SavedArguments[],
    ExpressionList *Arguments);


ILTree::Statement *
NearestEnclosing
(
    _In_ ILTree::Statement *Enclosed,
    BILOP OpcodeToMatch,
    bool ExitWhile,
    _Out_ bool &VisitsTry,
    _Out_ bool &VisitsCatch,
    _Out_ bool &VisitsFinally,
    _Out_opt_ bool *VisitsSynclock = NULL
);


// StatementListBuilder: this is a specialized class for building up the "ILTree::Statement.Next" fields
// in a chain of statements. It doesn't modify Prev or Parent pointers, nor fix anything else.
// It should only be used late, during bound-tree-lowering, when Prev/Parent generally aren't needed.
// (although in some cases like If and Try, you'll have to patch them up yourself anyway).
struct StatementListBuilder
{
    ILTree::Statement **m_ppLastStatement;
    StatementListBuilder(ILTree::Statement **ppInitialStatement) : m_ppLastStatement(ppInitialStatement) {}
    void Add(_In_ ILTree::Statement *statement); // note: statement->Next must be NULL
    void AddList(_In_opt_ ILTree::Statement *statements); // statement may be NULL itself. And statement->Next need not be NULL.
};




