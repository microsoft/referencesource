//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB conversion resolution semantics.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void Semantics::CollectConversionOperators(
    Type *TargetType,
    Type *SourceType,
    CDoubleList<OperatorInfo> &OperatorSet,
    NorlsAllocator &Scratch)
{
    CollectOperators(
        OperatorWiden,  // Though we specify widening here, this collects all conversion operators.
        TargetType,
        SourceType,
        OperatorSet,
        Scratch);
}

#define Err     ConversionError
#define Same    ConversionIdentity
#define Wide    ConversionWidening
#define Narr    ConversionNarrowing

//                                         Target             Source
const ConversionClass ConversionClassTable[t_ref - t_bad + 1][t_ref - t_bad + 1] =
{
// From->   bad  bool  i1    ui1   i2    ui2   i4    ui4   i8    ui8   dec   r4    r8    date  char  str   ref
/* bad  */ {Err, Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err },
/* bool */ {Err, Same, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* i1   */ {Err, Narr, Same, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* ui1  */ {Err, Narr, Narr, Same, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* i2   */ {Err, Narr, Wide, Wide, Same, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* ui2  */ {Err, Narr, Narr, Wide, Narr, Same, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* i4   */ {Err, Narr, Wide, Wide, Wide, Wide, Same, Narr, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* ui4  */ {Err, Narr, Narr, Wide, Narr, Wide, Narr, Same, Narr, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* i8   */ {Err, Narr, Wide, Wide, Wide, Wide, Wide, Wide, Same, Narr, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* ui8  */ {Err, Narr, Narr, Wide, Narr, Wide, Narr, Wide, Narr, Same, Narr, Narr, Narr, Err,  Err,  Narr, Err },
/* dec  */ {Err, Narr, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Same, Narr, Narr, Err,  Err,  Narr, Err },
/* r4   */ {Err, Narr, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Same, Narr, Err,  Err,  Narr, Err },
/* r8   */ {Err, Narr, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Wide, Same, Err,  Err,  Narr, Err },
/* date */ {Err, Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Same, Err,  Narr, Err },
/* char */ {Err, Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Same, Narr, Err },
/* str  */ {Err, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Narr, Wide, Same, Err },
/* ref  */ {Err, Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err,  Err },
};

#undef Err
#undef Same
#undef Wide
#undef Narr

static ConversionClass
ClassifyCLRArrayToInterfaceConversion
(
    Type *TargetInterface,
    ArrayType *SourceArray,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                     // How many times have we called ourself recursively?
    bool IgnoreProjectEquivalence,          // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTarget,     // The project containing a component of type "TargetInterface" that was considered
                                            // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSource      // The project containing a component of type "SourceArray" that was considered
                                            // for a successful match ignoring project equivalence.
);

ConversionClass
Semantics::ClassifyConversionFromExpression
(
    Type * pTargetType, 
    ILTree::Expression * pExpression
)
{
    Procedure * pMethod = NULL;    
    GenericBinding * pBinding = NULL;
    bool opIsLifted = false;
    
    if (IsBad(pExpression))
    {
        return ConversionError;
    }


    // CASE 1: conversion from an AddressOf expression
    if (pExpression->bilop == SX_ADDRESSOF)
    {
        
        BackupValue<bool> backup_strict_on(&m_UsingOptionTypeStrict);
        BackupValue<bool> backup_report_errors(&m_ReportErrors);
        m_UsingOptionTypeStrict = false;
        m_ReportErrors = false;
        DelegateRelaxationLevel relaxationLevel = DelegateRelaxationLevelNone;
        
        ILTree::Expression * pDelegateBinding = 
            InterpretDelegateBinding
            (
                pExpression,
                pTargetType,
                pExpression->Loc,
                false,
                ExprNoFlags,
                relaxationLevel,
                NULL
            );

        backup_strict_on.Restore();
        backup_report_errors.Restore();

        if (pDelegateBinding && ! IsBad(pDelegateBinding))
        {
            if (relaxationLevel == DelegateRelaxationLevelNone)
            {
                return ConversionIdentity;
            }
            else if (relaxationLevel == DelegateRelaxationLevelWidening ||
                     relaxationLevel == DelegateRelaxationLevelWideningDropReturnOrArgs ||
                     relaxationLevel == DelegateRelaxationLevelWideningToNonLambda)
            {
                return ConversionWidening;
            }
            else if (relaxationLevel == DelegateRelaxationLevelNarrowing)
            {
                return ConversionNarrowing;
            }
        }

        return ConversionError;
    }

    // CASE 2: conversion from an unbound lambda
    else if (pExpression->bilop == SX_UNBOUND_LAMBDA)
    {
        BackupValue<bool> backup_strict_on(&m_UsingOptionTypeStrict);
        BackupValue<bool> backup_report_errors(&m_ReportErrors);
        
        m_UsingOptionTypeStrict = false;
        m_ReportErrors = false;

        ILTree::UnboundLambdaExpression *  pCopiedLambda = 
            (ILTree::UnboundLambdaExpression *)
            m_TreeAllocator.xShallowCopyUnboundLambdaTreeForScratch(&pExpression->AsUnboundLambdaExpression());
        // Dev10#470292: InterpretUnboundLambdaBinding will mutate the types of its parameters;
        // that's why we need a deep copy.

        DelegateRelaxationLevel relaxationLevel = DelegateRelaxationLevelNone;

        if (pTargetType->IsDelegate())
        {
            ILTree::Expression * pBoundLambda =
                InterpretUnboundLambdaBinding
                (
                    pCopiedLambda,
                    pTargetType,
                    true,
                    relaxationLevel,
                    false,
                    NULL,
                    NULL,
                    false,
                    true // We are inferring the lambda's type.
                );

            if (! IsBad(pBoundLambda))
            {
                if (relaxationLevel == DelegateRelaxationLevelNone)
                {
                    return ConversionIdentity;
                }
                else if (relaxationLevel == DelegateRelaxationLevelWidening ||
                        relaxationLevel == DelegateRelaxationLevelWideningDropReturnOrArgs ||
                        relaxationLevel == DelegateRelaxationLevelWideningToNonLambda)
                {
                    return ConversionWidening;
                }
                else if (relaxationLevel == DelegateRelaxationLevelNarrowing)
                {
                    return ConversionNarrowing;
                }                    
            }
        }

        return ConversionError;
    }

    // CASE 3: the "Nothing" literal widens to everything
    else if (IsNothingLiteral(pExpression))
    {
        return ConversionWidening;
    }

    // CASE 4: an array literal
    else if (pExpression->ResultType->IsArrayLiteralType())
    {
        if (pExpression->bilop != SX_ARRAYLITERAL)
        {
            VSFAIL("unexpected: an expression which claimed to be ArrayLiteralType, but didn't have SX_ARRAYLITERAL");
            // Note: do NOT disable this assertion. If you have ArrayLiteralTypes which aren't SX_ARRAYLITERALS
            // then the Convert* routines will fail in mysterious ways, as will InterpretExpresion, and ClassifyConversion.
            return ConversionError;
        }
        // Note: an unbound array literal is indicated with SX_ArrayLiteral / ResultType->IsArrayLiteralType().
        // A bound array literal is indicated with SX_ArrayLiteral / !ResultType->IsArrayLiteralType().
        // We can only classify array literal conversions if it hasn't yet been bound.
        return ClassifyArrayLiteralConversion(pTargetType, pExpression->AsArrayLiteralExpression().ResultType->PArrayLiteralType());
    }

    // CASE 5: a nested array literal.
    else if (pExpression->bilop == SX_NESTEDARRAYLITERAL && (pExpression->ResultType->IsArrayLiteralType() || pExpression->ResultType->IsVoidType()))
    {
        VSFAIL("Shouldn't occur. When PopulateTypeListForArrayLiteral fills out the BCSYM_ArrayLiteralType with the flattened list of all elements, there should be no unbound array literal elements in it.");
        // The design is that it's the outer SX_ArrayLiteral that contains all the useful
        // information. And it's BCSYM ResultType contains a flattened list of all children
        // (including ones inside SX_NestedArrayLiterals). So here we just skip, on the
        // assumption that our caller is already looking after convertibility of all our children.
        // Note that for a jagged array, it might contain bound SX_NestedArray literals.
        return ConversionIdentity;
    }        

    // CASE 6: otherwise, base it on the literal's computed type
    else
    {
        return ClassifyConversion(pTargetType, pExpression->ResultType, pMethod, pBinding, opIsLifted, true, NULL);
    }
};


/*=======================================================================================
ClassifyConversion

This function classifies the nature of the conversion from the source type to the target
type. If such a conversion requires a user-defined conversion, it will be supplied as an
out parameter.
=======================================================================================*/
ConversionClass
Semantics::ClassifyConversion
(
    Type *TargetType,
    Type *SourceType,
    Procedure *&OperatorMethod,
    GenericBinding *&OperatorMethodGenericContext,
    bool &OperatorMethodIsLifted,
    bool considerConversionsOnNullableBool,
    bool * pConversionRequiresUnliftedAccessToNullableValue,
    bool * pConversionIsNarrowingDueToAmbiguity,
    DelegateRelaxationLevel * pConversionRelaxationLevel,
    bool IgnoreOperatorMethod
)
{

    ConversionClass Result =
        ClassifyPredefinedConversion
        (
            TargetType,
            SourceType,
            false,
            NULL,
            NULL,
            false,
            considerConversionsOnNullableBool,
            pConversionRequiresUnliftedAccessToNullableValue,
            pConversionIsNarrowingDueToAmbiguity,
            pConversionRelaxationLevel
        );

    OperatorMethodIsLifted = false;
    // Try the conversion considering user-defined conversion operators.
    // Dev10 #564475 Dig into Nullable types to make sure we don't look for user defined
    //                          operators between two intrinsic types. For example, Decimal has
    //                          user defined operators, which should be shadowed by intrinsic conversions
    //                          even if intrinsic conversion results in compilation error.
    if (!IgnoreOperatorMethod && 
        Result == ConversionError &&
        !TypeHelpers::IsInterfaceType(SourceType) &&
        !TypeHelpers::IsInterfaceType(TargetType) &&
        (CanTypeContainUserDefinedOperators(SourceType) || CanTypeContainUserDefinedOperators(TargetType)) &&
        !(TypeHelpers::IsIntrinsicType(TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost)) && 
            TypeHelpers::IsIntrinsicType(TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost))))
    {
        if (pConversionRequiresUnliftedAccessToNullableValue)
        {
            *pConversionRequiresUnliftedAccessToNullableValue = false;
        }

        Result =
            ClassifyUserDefinedConversion
            (
                TargetType,
                SourceType,
                OperatorMethod,
                OperatorMethodGenericContext,
                &OperatorMethodIsLifted,
                considerConversionsOnNullableBool,
                pConversionRequiresUnliftedAccessToNullableValue
            );
    }

    return Result;
}


/*=======================================================================================
ClassifyCLRReferenceConversion
Says whether source and target can be converted between using only CLR reference-conversion
rules or identity.

It returns ConversionIdentity/Narrowing/Widening/Error according to what that CLR
reference conversion is.

* If the function returns Identity/Widening, it means there is the CERTAINLY that a
  CLR reference conversion will succeed through the subset of CLR rules that are
  allowed by VB.
* You could pass the flag OverestimateNarrowingConversions and check for the return
  value being non-Error. That will tell you that there is the POSSIBILITY of a CLR
  reference conversion succeeding at runtime (maybe through an appropriate
  instantiation of generics, or maybe through a CLR conversion like int()->uint()
  that the CLR always allows but VB counts as error, or maybe through normal
  inheritance like the cast Animal->Mammal may succeed at runtime if you happen
  to give it a Mammal.)
* There's no way to tell whether a function is guaranteed to succeed according to
  the full CLR rules (e.g. int()->uint() is guaranteed to succeed by the CLR
  rules, but it's counted as Error by the VB semantics). That's a decision decision:
  it's a rare thing to need, and it felt nicer that to use the word "Widening"
  solely for VB-acceptable widening.

In the case (Of Source as {Ref,Target}, Target) where we're considering Source->Target,
then it will return Widening if you include OverestimateNarrowingConversions since Source might
be instantiated with a class that inherits from Target and this would allow a reference
conversion between them. Without the flag it still returns Widening, since that's what the VB
semantics say is possible in this case. (even though !Target->IsReferenceType(), we can look
at it and realize that Target must be a reference type, and the VB spec classifies this
Source->Target as a reference conversion.)

In the case (Of Source as Target, Target as IFoo) then it will return Narrowing if you include
the flag since Source might be instantiated with a class that implements Target.
But without the flag then the function will return Error. That's because Source might
be instantiated with a *structure* that implements Target, and so the conversion
Source->Target would require boxing (which isn't a reference conversion), and so
we can't guarantee a reference conversion Source->Target.

Note: this function returns Identity given two identical value-types.
=======================================================================================*/
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
    bool* pNarrowingCausedByContraVariance
)
{
    VSASSERT(TargetType && SourceType, L"Expected TargetType & SourceType");
    
    ConversionSemantics &= ~ConversionSemantics::AllowBoxingConversions;
    ConversionSemantics &= ~ConversionSemantics::AllowIntegralConversions;

    // Note: this function is so simple that you might think there's no need for it.
    // True, but it's main role is to be an appropriate place to write the comments above!

    ConversionClass result = ::ClassifyPredefinedCLRConversion(
                                        TargetType,
                                        SourceType,
                                        SymbolCreator,
                                        CompilerHost,
                                        ConversionSemantics,
                                        RecursionCount, // we're just a trivial wrapper around the function, so don't increment RecursionCount
                                        IgnoreProjectEquivalence,
                                        ProjectForTargetType,
                                        ProjectForSourceType,
                                        NULL,
                                        pNarrowingCausedByContraVariance
                                        );

    return result;
}



// ==============================================================================================================================
// ClassifyPredefinedCLRConversion:
//
// Here we have a complete list of all conversions performed implicitly by the CLR
// i.e. this corresponds to the CLR notion of "assignment-compatibility" (ECMA I $8.7)
// It's sort of a cross between dynamic_cast<> and reinterpret_cast<>, in the sense
// that it looks up run-time type information to determine whether the conversion
// can be accomplished without altering the raw bits. If ClassifyPredefinedCLRConversion
// returns "Identity" or "Widening" then this conversion can take place for sure.
// If it returns "Narrowing" or "Error" then maybe it can't.
//
// The CLR plays fast-and-loose as regards "typesafety" of integral types. It thinks
// that e.g. treating a given 8-bit value as "Byte" or "SByte" counts as typesafe.
//
// ==============================================================================================================================
//
// In specifying the conversions, we use these flags:
//
//  * AllowBoxingConversions -- if true, then e.g. an integer inherits from Object.
//    If false then it doesn't. You'd assume implicit boxing when doing most
//    conversions, but not e.g. when checking array element types or variance,
//    e.g. Integer() doesn't convert to Object()
//
//  * AllowIntegralConversions -- if true, then e.g. an enum can convert to its
//    underlying type. VB allows integral conversions in many contexts, but not in
//    all e.g. it doesn't use them when determining whether an extension method matches.
//    This flag solely governs integral->integral *reinterpretations*. Integral->integral *identities*
//    are still allowed if the flag is unset. And other value->reference conversions
//    depend on AllowBoxingConversions instead of this flag. And
//    the integral->integral conversions that change bitsize (e.g. int->long) are
//    completely disallowed.
//
//  * AllowArrayIntegralConversions -- if true, then e.g. enum() -> underlying().
//    This is false when testing if extension methods match. It's true in
//    most other situations.
//
//  * OverestimateNarrowingConversions -- the normal VB semantics say that
//    most of the conversions that will certainly succeed at runtime are widening/identity, and
//    many of the conversions that might succeed at runtime are narrowing, and the
//    rest are errors. But when we use this flag, we expand the narrowing to encompass
//    EVERY SINGLE conversion that might conceivably succeed at runtime that's not
//    already counted as widening/identity. Some of its effect is so that
//    CLR conversions that will definitely succeed get counted at least as narrowing
//    even though normally VB counts them as error, e.g. underlying()->enum().
//    The rest of its effect is so that conversions with generic parameter types,
//    which MIGHT succeed at runtime if the generic parameters are instantiated
//    the right way, will now count as narrowing under this flag.
//    e.g. given (Of S,T), then the conversion S->T might succeed at runtime if S and T
//    are instantiated with convertible types.
//    The flag is called "OverestimateNarrowingConversions". We might conceivably
//    just class EVERYTHING as narrowing if the flag is set. But that wouldn't be
//    very helpful. So we make a best effort to still class as ConversionError those
//    things that we can guarantee will fail. Maybe in future versions of the compiler
//    we can improve upon this effort, if there's need.
//    This flag is used when judging variance-ambiguity (since the CLR will throw an
//    AmbiguousCastException if at runtime it ends up with multiple conversion candidates
//    according to CLR rules).
//
// ==============================================================================================================================
//
// The correctness of this conversion-classification function depends crucially on
//   (1) having a complete list of types that something may be -- see IsKnownType
//   (2) knowing which ones satisfy IsValueType and which ones satisfy IsReferenceType -- see their implementations
//   (3) knowing which types satisfy neither -- see comments at top of TypeTables.h
//
// If you make changes to either of these things, then ClassifyPredefinedCLRConversion
// will likely have to change as well. If you have doubts, please talk with Microsoft (31/May/2008)
//
// ==============================================================================================================================
//
// This is the complete list of CLR conversions:
//
// (0) Transitivity: if X converts to Y, and Y converts to Z, then X converts to Z
//
// (1) Identity: on known-to-be-values, on known-to-be-references, and on generic-params-not-known-to-be-either
//
// (2) Variance conversion: given IFoo<Of Out T>, there's a conversion from IFoo<Of X> to IFoo<Of Y> if
//     there's a conversion (no value conversions, no boxing) from X to Y. And the reverse if given IFoo<Of In T>.
//     And given IFoo<Of T> then there's a conversion from IFoo<Of X> to IFoo<Of Y> only so long as X=Y.
//     Similarly for delegates.
//     * NB. that this rule is recursive, i.e. it checks for convertibility of X and Y.
//
// (3) Array covariance: X[] is convertible to Y[] if X and Y are known to be references and
//     there's a conversion (no value conversions, no boxing) from X to Y
//     * NB. that this rule is recursive, i.e. it checks for convertibility of X and Y.
//
// (3b)Array covariance on generics: X[] is convertible to Y[] if X is a generic parameter known to be a reference
//     and Y is not known to be a reference or value (i.e. it's a generic parameter with no useful constraints)
//     and X is constrained, directly or indirectly, to be Y
//     This arises e.g. <Of X As {Class,Y}, Y>
//
// (4) Array generic interface: X[] is convertible to IList<Of Y> if X and Y are known to be references
//     and X is Y or is inherited from Y or implements Y or is constrained to be Y, directly or indirectly.
//     * NB. this rule isn't recursive. It doesn't ask that X be convertible to Y. It just asks for inherits/implements/constrained/is.
//       Boxing is implicitly not considered from X to Y since X is known to be a reference
//     * NB. thanks to rules (3) and (0), this rule (4) might as well be rewritten as recursive.
//       After all, if X can be converted to Y (no value-conversions, no boxing) then X[] can be converted to Y[],
//       which can in turn be converted to IList<Of Y>.
//
// (4b)Array generic interface on generics: X() is convertible to IList(Of Y) if X is a generic parameter known to be
//     a reference and Y is not known to be a reference or value (i.e. it's a generic parameter with no useful constraints)
//     and X is constrained, directly or indirectly, to be Y. This arises e.g. (Of X as {Class,Y}, Y)
//
// (5) Enum to underlying: an enum is convertible to its underlying type
//     * NB. Only if AllowValueConversions
//
// (5) Int32 converts to Boolean
//     * NB. We only include this in AllowValueConversions & OverestimateNarrowingConversions.
//
// (6) Inheritance/Implements: X is convertible to Y if X declares that it inherits from Y, or that it implements Y
//     * NB. If X is a value-type then it will have no inheritance/implements conversions unless AllowBoxingConversions.
//       e.g. Integer->Object is true if AllowBoxingConversions, and false otherwise. We'd use AllowBoxingConversions for
//       normal conversions, and !AllowBoxingConversions e.g. when doing the recursive check for variance-conversion
//
// (6b)Constraints: X is convertible to Y if generic parameter X is constrained to Y and X is known to be a reference.
//
// (6c)Constraints on generics: X is convertible to Y if X is a generic parameter not known to be a reference which
//     is constrained to generic parameter Y and Y is not known to be a reference or value (i.e. it's a generic
//     parameter with no useful constraints).
//     * NB. Only if AllowBoxingConversions. Note incidentally that there are no value->value conversions even
//       possible from one generic parameter to another. If a generic parameter is known to be a reference then
//       we already know for sure that it admits inheritance &c. But if it's not known to be a reference, then
//       the only way we can know for sure that it converts to Y is if we know we're allowed to box it if necessary.
//
// (7a)Bitsize: signed and unsigned integral types and bools can be converted so long as they have the same bitsize.
//     Bool is taken to be the same size as int8.
//     * NB. Only if AllowValueConversions and OverestimateNarrowingConversions
//
// (7b)Array bitsize: arrays of integral types/bools can be converted back and forth so long as the integral types have the same
//     bitsize. Again bool is taken to be the same size as int8.
//     * NB. Only if AllowArrayValueConversions and OverestimateNarrowingConversions
//
// (7c)Enum array: arrays of enums can be converted to arrays of their underlying integral type
//     * NB. Only of AllowArrayValueConversions.
//
// (7d)Enum array bitsize: arrays of enums can be converted to arrays of integral types of the same
//     bitsize as their underlying integral type but not identical to their underlying integral type,
//     This doesn't include converting enum() to bool() in the case that the enum's underlying type is int8/uint8.
//     * NB. Only if OverestimateNarrowingConversions and AllowArrayValueConversions.
//
// (7e)Enum array bitsize reverse: arrays of integral types can be converted to enums
//     of the same bitsize, and again this doesn't include bools
//     * NB. Only if OverestimateNarrowingConversions and AllowArrayValueConversions.
//
// ==============================================================================================================================
ConversionClass
Semantics::ClassifyPredefinedCLRConversion
(
    Type *TargetType,
    Type *SourceType,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType,     // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
    bool *pConversionIsNarrowingDueToAmbiguity, // [out] did we return "Narrowing" for reason of ambiguity,
                                                // e.g. multiple variance-conversions?
    bool *pNarrowingCausedByContraVariance                                            
)
{
    return
        ::ClassifyPredefinedCLRConversion(
            TargetType,
            SourceType,
            m_SymbolCreator,
            m_CompilerHost,
            ConversionSemantics,
            0,  // we're calling from outside, so RecursionCount=0
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType,
            pConversionIsNarrowingDueToAmbiguity,
            pNarrowingCausedByContraVariance
            );
}

//=======================================================================================
// ::ClassifyPredefinedCLRConversion. (See comments above).
//=======================================================================================
ConversionClass
ClassifyPredefinedCLRConversion
(
    Type *TargetType,
    Type *SourceType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                         // How many times have we recursively called ourselves?
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType,     // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
    bool *pConversionIsNarrowingDueToAmbiguity, // [out] did we return "Narrowing" for reason of ambiguity,
                                                // e.g. multiple variance-conversions?
    bool* pNarrowingCausedByContraVariance                                                
)
{
    VSASSERT(SourceType!=NULL && TargetType!=NULL, "Please don't invoke ClassifyPredefinedCLRConversion with null types");
    
    // Dev10 #647268: The following Assert is very expensive. Commenting out.
    //VPASSERT(TypeHelpers::IsKnownType(SourceType) && TypeHelpers::IsKnownType(TargetType), "ClassifyPredefinedCLRConversion has been given a type that it doesn't know about. Please fix it, and TypeTables.h, and TypeHelpers::IsKnownType/IsReferenceType/IsValueType, and everything else.");

    VSASSERT(HASFLAG(ConversionSemantics, ConversionSemantics::AllowArrayIntegralConversions) || !HASFLAG(ConversionSemantics, ConversionSemantics::AllowIntegralConversions), "It's unusual to disallow array-integral-conversions like enum()->int(), but to allow integral-conversions like enum->int. Are you sure you intend this?");
    VSASSERT(!HASFLAG(ConversionSemantics, ConversionSemantics::AllowIntegralConversions) || !HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions), "It's unusual to need to OverestimateNarrowingConversions (which is done for variance reference conversions), and also AllowIntegralConversions like int->enum. So unusual that we haven't even implemented it yet. Are you sure you intend this? It's more usual to AllowArrayIntegralConversions like int()->enum() instead.");


    // Conceptually, this function is just a large truth-table for the different combinations
    // of source types and target types. Alas, the full table is far too big to draw or to implement in a simplistic fashion.
    // So instead we implement it with a confusing series of checks one by one (e.g. identity, inheritance, variance-conversion, ...)_
    // Sometimes a check always returns an answer because it knows it's the only check that can ever apply to a given Source/Target.
    // Sometimes a check will see that it doesn't apply, and will fall-through to further processing.
    // There's not much rhyme nor reason. This function would benefit from cleanup by someone who can dream up a better architecture for it.


    // *****************
    // (*) STACK OVERFLOW
    // There are some computations for variance that will result in an infinite recursion
    // e.g. the conversion C -> N(Of C) given "Interface N(Of In X)" and "Class C : Implements N(Of N(Of C))"
    // The CLR spec is recursive on this topic, hence ambiguous, so it's not known whether there
    // is a conversion. Theoretically we should detect such cases by maintaining a set of all
    // conversion-pairs (SourceType,TargetType) that we've visited so far in our quest to tell whether
    // a given conversion is possible: if we revisit a given pair, then we've encountered an ambiguity.
    // But maintaining such a set is too onerous. So instead we keep a simple "RecursionCount".
    // Once that reaches above a certain arbitrary limit, we'll return "ConversionNarrowing". That's
    // our way of saying that the conversion might be allowed by the CLR, or it might not, but we're not
    // sure. Note that even though we use an arbitrary limit, we're still typesafe.

    if (RecursionCount > 20)
    {
        return ConversionNarrowing;
    }

    // So under what circumstances do we increment RecursionCount?
    // In essence, we increment it only for those recursive calls that we don't know are making
    // the problem simpler...
    //   * So, if we're trying to check whether the array-conversion "S() -> T()" is possible, and we
    //     reduce it to a test of "S -> T", then we've syntactically reduced the complexity of
    //     the task and therefore we don't need to increment RecursionCount.
    //   * And if we're trying to check whether "S -> T" for some generic parameter S, and we reduce
    //     it to a test of "S's constraint -> T", then we've again made the problem simpler so we
    //     don't need to increment RecursionCount. That's because the number of constraints-to-check
    //     is bounded.
    //   * And likewise if we reduce it to "S's base class -> T" or "S's implemented interface -> T",
    //     again the number of base classes and implemented interfaces is finite, so we don't
    //     increment RecursionCount.
    //   * Actually, the only case where we do increment RecursionCount is in variance, as in the
    //     example above. It wanted to check whether "C -> N(Of C)", so it looked saw that C
    //     inherits from N(Of N(Of C)), so it needed to check whether "N(Of N(Of C)) -> N(Of C)".
    //     This is a variance check: since N is contravariant, it requires a check that "N(Of C) <- C".
    //     Which hasn't simplified the problem. Variance is the only place where we have to
    //     make a sub-check and we can't guarantee that the problem has been made simpler or bounded.
    //     And that's the only place where we increment RecursionCount.
    


    // *****************
    // (1) IDENTITY
    // on two known-to-be-references, or two known-to-be-values, or two generic-params-not-known-to-be-either

    if (TypeHelpers::EquivalentTypes(
            TargetType,
            SourceType,
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType))
    {
        return ConversionIdentity;
    }


    // The VB compiler uses "void" as the type of typeless expression, e.g. "AddressOf f" or "function () r".
    // So the CLR has nothing to say about conversions of these things.
    if (SourceType->IsVoidType() || TargetType->IsVoidType() || SourceType->IsBad() || TargetType->IsBad())
    {
        return ConversionError;
    }


    // See the comments above for all the conversions that we consider.
    // NB. that list only says whether the forward conversion would be identity/widening;
    // this function also needs to determine whether it's narrowing.

    
    // ****************
    // (*) NO-BOXING
    // Lots of the rules assume boxing. We pull out most of the boxing issues here for convenience.
    // This means that, for the rest of the function, we won't bother checking that boxing conversions are allowed;
    // we'll just assume that either they are allowed, or it's already been settled that the conversions
    // we're going to test for are those that don't require boxing. (or "don't necessarily require boxing"
    // in the case of OverestimateNarrowingConversions). There's just one exception: boxing issues
    // might mean that whatever was classified as Widening has to actually be degraded to just Narrowing...
    bool DegradeWideningToNarrowing = false;

    if (!HASFLAG(ConversionSemantics, ConversionSemantics::AllowBoxingConversions))
    {
        // A type is either a known-ref (IsReferenceType), or a known-val (IsValueType),
        // or unknown whether it's a ref or a val. The only type that's unknown is a
        // generic parameter where (1) it has no ref/struct constraint-flags on it,
        // (2) no other generic parameter that it's constrained to be has a struct flag on it,
        // (3) neither it nor any other generic parameter that it's constrained to be
        // has a type constraint.
        //
        // We're going to classify all conversions according to whether they require, or might
        // require, boxing. "Might require" is a concept that arises in situations like this:
        // Sub f(Of S As T, T As Intf)(ByVal x as S()) Dim y as T() = x.
        // Here f(Of Mystruct, InterfaceThatMyStructImplements) would require boxing the array elements of MyStruct()
        // into Interface(), which isn't supported by the CLR and would throw TypeCastException. But
        // other invocations of f, e.g. f(Of MyStruct,MyStruct), would never require boxing and would succeed.
        //
        // It's up to the discretion of VB whether such conversions-that-might-succeed should be counted
        // as ConversionNarrowing or ConversionError. And VB makes the following decision:
        //
        // If boxing is disallowed, then we classify a conversion S->T as follows:
        // (1) If S and T are both known values, then continue on with the normal classification since no boxing will occur
        // (2) Otherwise, if S and T are both known references, then continue on with the normal classification;
        //     the only conversion rules that will end up applying are all reference->reference ones anyway.
        // (3) Otherwise, if S is a known reference and constrained to T, then the only way to instantiate it
        //     will be with T a reference too, so just go on with the normal classification.
        // (4) Otherwise, if one is a known ref and the other a known value, then any conversion would require boxing, so return error
        // (5) Otherwise at least one of them it's not known whether it's a reference or a value. The conversion
        //     might require boxing or it might not. So we'll go through normal classification, but if anything
        //     was going to return Widening, we degrade it to Narrowing (since we can't guarantee it).

        if (TypeHelpers::IsValueType(SourceType) && TypeHelpers::IsValueType(TargetType))
        {
            // VAL->VAL: doesn't need boxing
        }
        else if (TypeHelpers::IsReferenceType(SourceType) && TypeHelpers::IsReferenceType(TargetType))
        {
            // REF->REF: doesn't need boxing
        }
        else if (TypeHelpers::IsReferenceType(SourceType) && SourceType->IsGenericParam() && TargetType->IsGenericParam() &&
                 TypeHelpers::IsOrInheritsFrom(SourceType,TargetType))
        {
            // REF->REF: doesn't need boxing
            // Note: the only cases of rule (3) that haven't already been covered by rule (2) are ones where
            // S is a known reference type, and T isn't, and they're both generic parameters.
            // Note: the function IsOrInheritsFrom is badly named. What it does when given two generic parameters
            // is it determines whether there's a constraint relationship between them.
        }
        else if (TypeHelpers::IsReferenceType(TargetType) && SourceType->IsGenericParam() && TargetType->IsGenericParam() &&
                 TypeHelpers::IsOrInheritsFrom(TargetType,SourceType))
        {
            // REF->REF: doesn't need boxing
        }

        // All the above are to fall-through in cases where we might conceivably guarantee a succesful conversion
        // Now we can think about ruling some out.

        else if (TypeHelpers::IsReferenceType(SourceType) && TypeHelpers::IsValueType(TargetType))
        {
            // REF->VAL is impossible to succeed ever
            return ConversionError;
        }
        else if (TypeHelpers::IsValueType(SourceType) && TypeHelpers::IsReferenceType(TargetType))
        {
            // VAL->REF will always require boxing
            return ConversionError;
        }
        else
        {
            VPASSERT((SourceType->IsGenericParam() && !TypeHelpers::IsValueType(SourceType) && !TypeHelpers::IsReferenceType(SourceType)) ||
                     (TargetType->IsGenericParam() && !TypeHelpers::IsValueType(TargetType) && !TypeHelpers::IsReferenceType(TargetType)),
                     "After ruling out all ref/val conversions, you'd kind of expect that what's left must involve at least one neither-ref-nor-val type, which necessarily must be a generic parameter");
            // The conversions that are left are ones that involve at least one generic parameter who is not known
            // whether he's ref or val, i.e. S->T or S->REF or S->VAL or REF->T or VAL->T.
            // If S/T are instantiated one way then maybe the conversion will require boxing and then succeed.
            // If they're instantiated another way then maybe the conversion won't succeed even with boxing.
            // What we'll do is allow the regular conversion classficiation -- BUT... if it was to have return Widening,
            // then we downgrade it to Narrowing because we can't guarantee it.
            DegradeWideningToNarrowing = true;
        }
    }


    // *****************
    // (0) TRANSITIVITY.
    // Not implemented directly. Instead, transitivity is rolled into all the other conversions.




    if (TypeHelpers::IsGenericParameter(TargetType) != TypeHelpers::IsGenericParameter(SourceType))
    {
        // Handle covariance and backcasting with respect to the class constraint.

        // 


        // *****************
        // (*) NARROWING.
        // Here we look for the inverse of every GenericParameter --> NonGenericParameter conversion
        // via constraints/variance/inheritance/arrays.

        if (TypeHelpers::IsGenericParameter(TargetType))
        {
            Type *TargetClassConstraint =
                GetClassConstraint(
                    TargetType->PGenericParam(),
                    CompilerHost,
                    SymbolCreator.GetNorlsAllocator(),
                    false, // don't ReturnArraysAs"System.Array"
                    false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                    );

            if (TargetClassConstraint)
            {
                ConversionClass Conversion =
                    ClassifyPredefinedCLRConversion(
                        SourceType,
                        TargetClassConstraint,
                        SymbolCreator,
                        CompilerHost,
                        ConversionSemantics,
                        RecursionCount,  // We've simplified the problem (looking for TargetClassConstraint), so don't increment RecursionCount
                        IgnoreProjectEquivalence,
                        ProjectForSourceType,
                        ProjectForTargetType,
                        NULL
                        );

                if (Conversion == ConversionIdentity ||
                    Conversion == ConversionWidening)
                {
                    return ConversionNarrowing;
                }
            }
        }

        // *****************
        // (6b,6c) CONSTRAINTS ON GENERICS, plus transitivity of this and everything thereafter
        // GenericParam X is convertible to Y if X is constrained to Y and X is known to be a reference
        // GenericParam X is convertible to Y if X is constrained to Y and X isn't known to be a reference or value,

        else if (TypeHelpers::IsGenericParameter(SourceType))
        {
            Type *SourceClassConstraint =
                GetClassConstraint(
                    SourceType->PGenericParam(),
                    CompilerHost,
                    SymbolCreator.GetNorlsAllocator(),
                    false, // don't ReturnArraysAs"System.Array"
                    false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                    );

            if (SourceClassConstraint)
            {
                ConversionClass Conversion =
                    ClassifyPredefinedCLRConversion(
                        TargetType,
                        SourceClassConstraint,
                        SymbolCreator,
                        CompilerHost,
                        ConversionSemantics,
                        RecursionCount, // We've simplified the problem (to SourceClassConstraint), so don't increment RecursionCount
                        IgnoreProjectEquivalence,
                        ProjectForTargetType,
                        ProjectForSourceType,
                        NULL
                        );

                if (Conversion == ConversionIdentity ||
                    Conversion == ConversionWidening)
                {
                    return DegradeWideningToNarrowing ? ConversionNarrowing : ConversionWidening;
                }
            }
        }
    }

    /* INHERITANCE */

    if (TypeHelpers::IsRootObjectType(TargetType) ||
        TypeHelpers::IsOrInheritsFrom(
            SourceType,
            TargetType,
            SymbolCreator,
            false,
            NULL,
            IgnoreProjectEquivalence,
            ProjectForSourceType,
            ProjectForTargetType))
    {  
        return DegradeWideningToNarrowing ? ConversionNarrowing : ConversionWidening;
    }
    if (TypeHelpers::IsArrayType(SourceType) && TypeHelpers::IsRootArrayType(TargetType, CompilerHost))
    {
        VSASSERT(!DegradeWideningToNarrowing, "internal logic error: how can we have figured a downgrade from an array to System.Array?");
        return ConversionWidening;
    }

    if (TypeHelpers::IsRootObjectType(SourceType) ||
        TypeHelpers::IsOrInheritsFrom(
            TargetType,
            SourceType,
            SymbolCreator,
            false,
            NULL,
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType))
    {
        return ConversionNarrowing;
    }
    if (TypeHelpers::IsRootArrayType(SourceType, CompilerHost) && TypeHelpers::IsArrayType(TargetType))
    {
        return ConversionNarrowing;
    }


    /* VARIANCE */

    if (TargetType->IsInterface() && TargetType->IsGenericTypeBinding() // this subfunction assumes a variant target interface
        && TypeHelpers::HasVariance(TargetType)
        && !SourceType->IsArrayType()) // this subfunction doesn't work on arrays: we handle them separately
    {
        ConversionClass result = 
            ClassifyAnyVarianceConversionToInterface(
                TargetType,
                SourceType,
                SymbolCreator,
                CompilerHost,
                ConversionSemantics & ~ConversionSemantics::AllowBoxingConversions & ~ConversionSemantics::AllowIntegralConversions,
                // don't allow boxing/integrals when judging variance-convertibility
                RecursionCount, // The function internally will increment RecursionCount when it tries to call us back
                false,
                NULL,
                NULL);

        // The above function explains that it returns "Narrowing" solely in case of ambiguity...
        if (result==ConversionNarrowing && pConversionIsNarrowingDueToAmbiguity!=NULL)
        {
            *pConversionIsNarrowingDueToAmbiguity = true;
        }
        
        if (result!=ConversionError)
        {
            return (result == ConversionWidening && DegradeWideningToNarrowing) ? ConversionNarrowing : result;
        }

        // Note: this call also catches the case where SourceType is a generic parameter,
        // with either class or interface or other-generic-parameter-constraints that entail
        // a variance conversion.

        // Note: the above Inheritance tests have already returned "Narrowing" in the case
        // Ttarget -inh-> Tsource. Do we need worry that this will cover up a Widening that
        // variance would allow? No, because Tsource-inh-> T -var-> Ttarget -inh-> Tsource
        // would be an inheritance circle, which isn't allowed.

        // Note: we don't need to look for ConversionNarrowing here. That's because
        // the narrowing introduced by variance is Something -> VariantInterface,
        // and Anything->Interface is already counted as narrowing by the tests below.

    }
    else
    {
        if (TargetType->IsDelegate() // this subfunction assumes that it's given a delegate target
            && !SourceType->IsGenericParam()) // subfunction doesn't handle generic-params; but we've already dealt with them above
        {
            ConversionClass conversion = 
                ClassifyVarianceConversionToDelegate(
                    TargetType, 
                    SourceType, 
                    ConversionSemantics, 
                    RecursionCount, 
                    SymbolCreator, 
                    CompilerHost,
                    pNarrowingCausedByContraVariance
                    );

            switch (conversion)
            {
                case ConversionIdentity:
                case ConversionWidening:
                    VSASSERT(!DegradeWideningToNarrowing, "internal logic error: if target is a delegate, and source isn't a generic param, then how did we infer degrading due to no-boxing?");
                    return ConversionWidening;
                    
                case ConversionNarrowing:
                    return ConversionNarrowing;
                    
                case ConversionError:
                    // continue classification
                    break;
                    
                default:
                    VSFAIL("unexpected conversion");
                    break;
            }
        }
        
        if (SourceType->IsDelegate() && !TargetType->IsGenericParam())
        {
            ConversionClass conversion = ClassifyVarianceConversionToDelegate(SourceType, TargetType, ConversionSemantics, RecursionCount, SymbolCreator, CompilerHost);

            switch (conversion)
            {
                case ConversionIdentity:
                case ConversionWidening:
                case ConversionNarrowing:
                    return ConversionNarrowing;
                    // We do have to look for ConversionNarrowing here, unlike the Something->VariantInterface
                    // case above. That's because Delegate->Delegate isn't counted as narrowing by anything else.
                    
                case ConversionError:
                    // continue classification
                    break;
                    
                default:
                    VSFAIL("unexpected conversion");
                    break;
            }
        }
    }


    /* INTERFACE IMPLEMENTATION */

    if (TypeHelpers::IsInterfaceType(SourceType))
    {
        if (TypeHelpers::IsClassType(TargetType) || TypeHelpers::IsGenericParameter(TargetType))
        {
            // Even if a class is marked NotInheritable, it can still be a COM class and implement
            // any interface dynamically at runtime, so we must allow a narrowing conversion.
            return ConversionNarrowing;
        }

        else if (TypeHelpers::IsArrayType(TargetType))
        {
            // Arrays types are essentially NotInheritable, and can't contain COM classes, so
            // we'll return ConversionError if the conversion's impossible.
            ConversionClass Conversion = ClassifyCLRArrayToInterfaceConversion(
                                                SourceType,
                                                TargetType->PArrayType(),
                                                SymbolCreator,
                                                CompilerHost,
                                                ConversionSemantics,
                                                RecursionCount, // The subroutine can increment RecursionCount if it wants
                                                IgnoreProjectEquivalence,
                                                ProjectForSourceType,
                                                ProjectForTargetType
                                                );
            if (Conversion == ConversionError)
            {
                return ConversionError;
            }
            else
            {
                return ConversionNarrowing;
            }
        }

        else if (TypeHelpers::IsInterfaceType(TargetType))
        {
            VPASSERT(!TypeHelpers::IsOrInheritsFrom(
                            SourceType,
                            TargetType,
                            SymbolCreator,
                            false,
                            NULL,
                            IgnoreProjectEquivalence,
                            ProjectForSourceType,
                            ProjectForTargetType) &&
                     !TypeHelpers::IsOrInheritsFrom(
                            TargetType,
                            SourceType,
                            SymbolCreator,
                            false,
                            NULL,
                            IgnoreProjectEquivalence,
                            ProjectForTargetType,
                            ProjectForSourceType),
                        "interface inheritance relation should have been discovered by now");

            return ConversionNarrowing;
        }

        else if (TypeHelpers::IsValueType(TargetType))
        {
            if (TypeHelpers::Implements(
                TargetType,
                SourceType,
                SymbolCreator,
                false,
                NULL,
                CompilerHost,
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType))
            {
                return ConversionNarrowing;
            }
            // At this stage SourceType is an Interface (and not a generic parameter)
            // TargetType is a concrete value type and not a generic parameter and we haven't
            // been able to prove that TargetType implements SourceType. So there's no
            // possible relation.
            return ConversionError;
        }

        else
        {
            VSFAIL("All conversions from interface should have been handled by now");
            return ConversionError;
        }
    }

    VSASSERT(!TypeHelpers::IsInterfaceType(SourceType), "we should have handled all conversions from interface by now");

    if (TypeHelpers::IsInterfaceType(TargetType))
    {
        if (TypeHelpers::IsArrayType(SourceType))
        {
            VSASSERT(!DegradeWideningToNarrowing, "internal logic error: how can we have figured on a degrade from Array to Interface?");
            return
                ClassifyCLRArrayToInterfaceConversion(
                    TargetType,
                    SourceType->PArrayType(),
                    SymbolCreator,
                    CompilerHost,
                    ConversionSemantics,
                    RecursionCount, // The subroutine can increment RecursionCount if it wants
                    IgnoreProjectEquivalence,
                    ProjectForTargetType,
                    ProjectForSourceType
                    );
        }

        if (TypeHelpers::IsClassType(SourceType))
        {
            VSASSERT(!DegradeWideningToNarrowing, "internal logic error: how can we have figured a degrade from a class to anything?");
            return
                TypeHelpers::Implements(
                    SourceType,
                    TargetType,
                    SymbolCreator,
                    false,
                    NULL,
                    CompilerHost,
                    IgnoreProjectEquivalence,
                    ProjectForSourceType,
                    ProjectForTargetType) ?
                ConversionWidening :
                ConversionNarrowing;
        }

        if (TypeHelpers::IsGenericParameter(SourceType))
        {
            // Note that the interface inheritance and class inheritance for the constraints are already
            // in the IsOrInheritsFrom check made earlier

            if (SourceType->PGenericParam()->HasValueConstraint())
            {
                // Since the type param has a "Structure" constraint, the type param will always widen to
                // System.ValueType. So check if System.ValueType implements the target Interface type.
                //
                if (TypeHelpers::Implements(
                        CompilerHost->GetFXSymbolProvider()->GetType(FX::ValueTypeType),
                        TargetType,
                        SymbolCreator,
                        false,
                        NULL,
                        CompilerHost,
                        IgnoreProjectEquivalence,
                        ProjectForSourceType,
                        ProjectForTargetType))
                {
                    return DegradeWideningToNarrowing ? ConversionNarrowing : ConversionWidening;
                }
            }
            else
            {
                // Check if the class type constraint implements the target interface type

                BCSYM *ClassConstraintType =
                    GetClassConstraint(
                        SourceType->PGenericParam(),
                        CompilerHost,
                        SymbolCreator.GetNorlsAllocator(),
                        true,  // ReturnArraysAs"System.Array"
                        false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                        ); 

                if (ClassConstraintType &&
                    TypeHelpers::Implements(
                        ClassConstraintType,
                        TargetType,
                        SymbolCreator,
                        false,
                        NULL,
                        CompilerHost,
                        IgnoreProjectEquivalence,
                        ProjectForSourceType,
                        ProjectForTargetType))
                {
                    return DegradeWideningToNarrowing ? ConversionNarrowing : ConversionWidening;
                }
            }

            return ConversionNarrowing;
        }

        VSASSERT(!TypeHelpers::IsGenericParameter(SourceType), "Generic parameters need to be handled earlier!!!");

        if (TypeHelpers::IsValueType(SourceType))
        {
            if (TypeHelpers::Implements(
                    SourceType,
                    TargetType,
                    SymbolCreator,
                    false,
                    NULL,
                    CompilerHost,
                    IgnoreProjectEquivalence,
                    ProjectForSourceType,
                    ProjectForTargetType))
            {
                return DegradeWideningToNarrowing ? ConversionNarrowing : ConversionWidening;
            }
            // At this point we know TargetType is an interface (not a generic parameter)
            // and SourceType is a value-type (maybe a generic parameter with a "structure" constraint-flag, or other constraint)
            // and SourceType is not constrained to implement TargetType.
            // But if it's a generic parameter, then conceivably, at runtime, if it's instantiated right, it might...
            else if (HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions) && SourceType->IsGenericParam())
            {
                return ConversionNarrowing;
                // note: unreachable. This generic-param-as-structure -> interface conversoin will only occur when
                // (1) we allow boxing, and (2) we're overestimating narrowing conversions. But the current codepaths
                // only ever disallow boxing when overestimating, so we can't reach here.
            }
            else
            {
                return ConversionError;
            }
        }
    }

    VSASSERT(!TypeHelpers::IsInterfaceType(TargetType), "we should have handled all conversions to interface by now");


    /* INTEGRALS */

    //
    // It's also made simpler because we've already checked for identity.
    //
    if (HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions))
    {
        // This code is made simpler because no one will ever call us with both AllowIntegralConversions and OverestimateNarrowingConversions:
        VSASSERT(!HASFLAG(ConversionSemantics, ConversionSemantics::AllowIntegralConversions), "We haven't implemented the case where both AllowValueConversions OverestimateNarrowingConversions because it never seemed needed.");
        // Since we're overestimating narrowing conversions, the question to ask here is:
        // "The fact that integral conversions are disallowed: for what SourceType/TargetType pairs does
        // is fact allow us to rule out any possible conversion between them succeeding?
        // Answer: if SourceType and TargetType are both integrals (and by assumption they're not equal),
        // then the only conversion possible between them is a value conversion, so it's impossible
        if (TypeHelpers::IsIntegralType(SourceType) && TypeHelpers::IsIntegralType(TargetType))
        {
            return ConversionError;
        }
        // All other cases, e.g. integral->GenericParameter, enum->reference System.Enum ..., well, they might succeed.
        // at the very least we can't rule them out now. That's why we fall through...
    }
    else if (TypeHelpers::IsIntegralType(SourceType) || TypeHelpers::IsIntegralType(TargetType))
    {
        // nb. that the CLI spec says "enums shall have a built-in integer type" (ECMA I $8.5.2) and gives
        // a list of built-in types (ECMA I $8.2.2). The only built-in integer types are i8,ui8,i16,ui16,i32,ui32,i64,ui64
        // (not decimal). These are precisely the same types as satisfy ->IsIntegral().

        if (HASFLAG(ConversionSemantics, ConversionSemantics::AllowIntegralConversions))
        {
            if (TypeHelpers::IsEnumType(SourceType) && !TypeHelpers::IsEnumType(TargetType) && 
                SourceType->GetVtype()==TargetType->GetVtype())
            {
                VSASSERT(!DegradeWideningToNarrowing, "internal logic error: how can we have inferred a degrade when they have the same VType and one's an enum?");
                return ConversionWidening;
                // enum->underlying is a widening
            }
            else if (TypeHelpers::IsEnumType(TargetType) && SourceType->GetVtype()==TargetType->GetVtype())
            {
                return ConversionNarrowing;
                // underlying->enum and enum->enum is a narrowing
            }
        }
        // At this point we know that one of the parties is an integral. And we're looking for whether or not
        // we can guarantee a conversion. And we've already covered the case of e.g. integral inheriting from System.Enum
        // or System.ValueType. So the only conversions that we can possibly guarantee here are the integral ones.
        // But we've already looked at those, and found they didn't apply. So the conversion can't be guaranteed to succeed:
        return ConversionError;
    }


    /* GENERIC PARAMS */

    if (TypeHelpers::IsGenericParameter(SourceType) || TypeHelpers::IsGenericParameter(TargetType))
    {
        // All the conversions involving the type parameter's constraints are already handled
        // in the IsOrInheritsFrom and Implements checks made earlier.
        //
        if (HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions))
        {
            return ConversionNarrowing;
        }
        else
        {
            return ConversionError;
        }
    }

    /* ARRAY COVARIANCE */

    if (TypeHelpers::IsArrayType(SourceType) && TypeHelpers::IsArrayType(TargetType))
    {
        VSASSERT(!DegradeWideningToNarrowing, "internal logic error: how can we have figured on a degrade with source and target are arrays?");
        ArrayType *SourceArray = SourceType->PArrayType();
        ArrayType *TargetArray = TargetType->PArrayType();

        // The element types must either be the same or
        // the source element type must extend or implement the
        // target element type. (VB implements array covariance.)

        if (SourceArray->GetRank() == TargetArray->GetRank())
        {
            return
                ClassifyCLRConversionForArrayElementTypes(
                    TypeHelpers::GetElementType(TargetArray),
                    TypeHelpers::GetElementType(SourceArray),
                    SymbolCreator,
                    CompilerHost,
                    ConversionSemantics, // the function does its own thing with ConversionSemantics
                    RecursionCount, // Simplified problem to elements, so don't increment RecursionCount
                    IgnoreProjectEquivalence,
                    ProjectForTargetType,
                    ProjectForSourceType);
        }

        return ConversionError;
    }

    
    return ConversionError;
}





/*=======================================================================================
ClassifyImmediateVarianceCompatibility
Says whether a single step of variance-conversion can convert from SourceType to TargetType
The function doesn't impose expectations on what its input arguments are. So even if
it's given SourceType/TargetType which are impossible to have immediate variance-conversion
steps (e.g. class or array), it just returns ConversionError rather than asserting.

Note that, following CLI notation, an identity conversion between two generics is
considered an identity variance step. And identity between two non-generic interfaces
or delegates is still considered an identity variance step.
=======================================================================================*/

ConversionClass
ClassifyImmediateVarianceCompatibility
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
    DynamicArray<VarianceParameterCompatibility> *ParameterDetails, // optional; output.
    bool* bNarrowingCausedByContraVariance
)
{
    VSASSERT(SourceType!=NULL && TargetType!=NULL, "unexpected NULL types to compare");

    // ParameterDetails is an optional place for the caller to collection detailed diagnostics on how each parameter fared
    if (ParameterDetails!=NULL)
    {
        ParameterDetails->Reset();
    }


    if (SourceType->IsBad() || TargetType->IsBad())
    {
        return ConversionError;
    }

    // CLI I $8.7
    // Given an arbitrary generic type signature X(Of X1,...,Xn), a value of type
    // X(Of U1,...,Un) can be stored in a location of type X(Of T1,...,Tn) when all
    // the following hold:
    //  * For each "in Xi" where Ti is not a value-type or generic parameter, we
    //    require that a value of type Ti can be stored in a location Ui. [Note: since
    //    Ti is not a value-type or generic parameter, it must be a reference type; and
    //    references can only be stored in locations that hold references; so we know that
    //    Ui is also a reference type.]
    //  * For each "out Xi" where Ti is not a value-type or generic parameter, we require
    //    that a value of type Ui can be stored in a location of type Ti. [Note: reference
    //    locations can only ever hold values that are references, so we know that Ui is
    //    also a reference type.]
    //  * For each "Xi" neither in nor out where Ti is not a value-type or generic parameter,
    //    we require that Ti must be the exact same type as Ui. [Note: therefore Ui is
    //    also a reference type.]
    //  * For each "Xi" where Ti is a value-type or generic parameter, we require that
    //    Ti must be the exact same type as Ui.
    //
    // e.g. a class that implements IReadOnly(Of Mammal) can be stored in a location
    // that holds IReadOnly(Of Animal), by the second bullet point, given IReadOnly(Of Out T),
    // since a Mammal can be stored in an Animal location.
    // but IReadOnly(Of Car) cannot be stored, since a Car cannot be stored in an Animal location.


    // CLI I $8
    // "variance is only permitted on generic interfaces and generic delegates"
    // So, if we're given a pair that aren't these, then there isn't a variance-conversion-step
    // between them: (this function only checks for variance-conversion-steps)
    if (!(SourceType->IsInterface() && TargetType->IsInterface()) &&
        !(SourceType->IsDelegate() && TargetType->IsDelegate()))
    {
        return ConversionError;
    }

    // In the eyes of the spec CLI I $8.7, a non-generic-type-binding is really a
    // generic-type-binding with zero parameters, and it admits the identity
    // variance-conversion:
    if (!SourceType->IsGenericTypeBinding() && !TargetType->IsGenericTypeBinding())
    {
        return BCSYM::AreTypesEqual(SourceType, TargetType) ? ConversionIdentity : ConversionError;
    }

    // Now check that we have the same "X(Of X1,...,Xn)" for both SourceType and TargetType:
    if (!SourceType->IsGenericTypeBinding() || !TargetType->IsGenericTypeBinding())
    {
        return ConversionError;
    }

    bool is_identity = true;       // here we'll keep track of whether we managed to avoid any widening/narrowing in the arguments
    bool avoided_narrowing = true; // and whether we avoided narrowing
    bool allNarrowingCausedByContraVariance = true; // whether all narrowing conversions that we run into are caused by narrowing conversions or error conversions between delegate's contravariance type arguments.
    bool is_error = false;         // whether we encountered any error conversions
    int i=0;

    
    // A generic binding is constructed in layers, e.g. C(Of Z).I(Of X,Y) is a type which IsGenericTypeBinding(),
    // with generic type "I", parameters "T1,T2", arguments "X,Y", and its GetParentBinding is...
    // ... generic type "C", parameter "T", argument "Z". And a null parent binding.
    // So we have to iterate over those layers:
    //
    BCSYM_GenericTypeBinding *SourceBinding = SourceType->PGenericTypeBinding();
    BCSYM_GenericTypeBinding *TargetBinding = TargetType->PGenericTypeBinding();
    for (;
         SourceBinding!=NULL && TargetBinding!=NULL;
         SourceBinding = SourceBinding->GetParentBinding(),  TargetBinding = TargetBinding->GetParentBinding())
    {
        if (!BCSYM::AreTypesEqual(SourceBinding->GetGenericType(), TargetBinding->GetGenericType()))
        {
            if (ParameterDetails!=NULL)
            {
                ParameterDetails->Reset(); // if the generic bindings don't match, we won't even report any parameter information.
            }
            return ConversionError;
        }
        BCSYM_NamedRoot *GenericType = SourceBinding->GetGenericType();
        VSASSERT(SourceBinding->GetArgumentCount()==GenericType->GetGenericParamCount() &&
                 TargetBinding->GetArgumentCount()==GenericType->GetGenericParamCount(),
                 "unexpected two bindings of same type but different size");

        // Within each layer, we have to check the four bullet-points for every generic parameter.
        int ArgumentIndex = 0;
        for (BCSYM_GenericParam *gParam = GenericType->GetFirstGenericParam(); gParam; gParam=gParam->GetNextParam(), ArgumentIndex++)
        {
            BCSYM *SourceArgument = SourceBinding->GetArgument(ArgumentIndex);
            BCSYM *TargetArgument = TargetBinding->GetArgument(ArgumentIndex);

            if (ParameterDetails!=NULL)
            {
                ParameterDetails->Grow();
                ParameterDetails->End().Generic = GenericType;
                ParameterDetails->End().Compatible = true;  // we'll assume true, and change to false if it isn't
                ParameterDetails->End().Param = gParam;
                ParameterDetails->End().SourceArgument = SourceArgument;
                ParameterDetails->End().TargetArgument = TargetArgument;
            }

            // Maybe the arguments are identical?
            if (BCSYM::AreTypesEqual(SourceArgument, TargetArgument))
            {
                continue;
            }

            // So they're not identical. At best we'll have had to use widening, and at worst we might have failed or
            // needed widening...
            is_identity = false;

            ConversionClass result;
            bool isNarrowingCausedByContraVariance = false;
            switch (gParam->GetVariance())
            {
                case Variance_Out:
                {
                    bool narrowingCausedByContraVariance = false;
                   
                    result = ClassifyCLRReferenceConversion(
                                    TargetArgument,
                                    SourceArgument,
                                    SymbolCreator,
                                    CompilerHost,
                                    ConversionSemantics,
                                    RecursionCount + 1,  // Increment RecursionCount to stop infinite recursion
                                    false,
                                    NULL,
                                    NULL,
                                    &narrowingCausedByContraVariance);
                    
                    if (result == ConversionNarrowing && narrowingCausedByContraVariance)
                    {
                        isNarrowingCausedByContraVariance = true;
                    }    
                    
                    break;
                }    
                case Variance_In:
                    result = ClassifyCLRReferenceConversion(
                                    SourceArgument,
                                    TargetArgument,
                                    SymbolCreator,
                                    CompilerHost,
                                    ConversionSemantics,
                                    RecursionCount + 1, // Increment RecursionCount to stop infinite recursion
                                    false, 
                                    NULL,
                                    NULL);

                    if (result != ConversionWidening && result != ConversionIdentity)
                    {
                        // Dev10 #820752 For delegates, treat conversion as narrowing if both type arguments are reference types.
                        if (TargetType->IsDelegate() && TypeHelpers::IsReferenceType(SourceArgument) && TypeHelpers::IsReferenceType(TargetArgument))
                        {
                            result = ConversionNarrowing;
                            isNarrowingCausedByContraVariance = true;
                        }
                    }
                    break;
                case Variance_None:
                    result = ConversionError;
                    break;
                default:
                    VSFAIL("unexpected variance");
            }

            if (ParameterDetails!=NULL)
            {
                ParameterDetails->End().Compatible = (result == ConversionWidening || result == ConversionIdentity);
            }

            if (result == ConversionError)
            {
                is_error = true;
                if (ParameterDetails == NULL)
                {
                    return ConversionError;
                    // this is a short-circuit optimisation: if an error, and the caller doesn't need full details,
                    // then we can abort right now
                }
            }
            else if (result == ConversionNarrowing)
            {
                avoided_narrowing = false;

                if (!isNarrowingCausedByContraVariance)
                {
                    allNarrowingCausedByContraVariance = false;
                }
            }
            else
            {
                VSASSERT(result==ConversionWidening, "we already said that types aren't equal, so they shouldn't have an identity conversion");
            }
        }
    }
    
    if (SourceBinding!=NULL || TargetBinding!=NULL)
    {
        if (ParameterDetails!=NULL)
        {
            ParameterDetails->Reset();  // if the generic bindings don't match, we won't even report any parameter information.
        }
        return ConversionError;
    }
    else if (is_error)
    {
        return ConversionError;
    }
    else if (is_identity)
    {   
        return ConversionIdentity;
    }
    else if (avoided_narrowing)
    {
        return ConversionWidening;
    }
    else if (HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions) ||
        allNarrowingCausedByContraVariance)
    {
        // If the flag "OverestimateNarrowingConversions" is set, then the meaning of "narrowing"
        // becomes "this conversion might potentially succeed at runtime". Consider the conversion
        // Function f(ByVal x as IEnumerable(Of Animal)) return CType(x, IEnumerable(Of Mammal))
        // might potentially succeed at runtime, if you happen to give it an x which implements IEnumerable(Of Mammal)
        // So in this case, if any of the interior parts involved narrowing, then we have
        // to return narrowing as well.
        if (bNarrowingCausedByContraVariance != NULL)
        {
            *bNarrowingCausedByContraVariance = allNarrowingCausedByContraVariance;
        }    
        
        return ConversionNarrowing;

        // But if the flag was unset, then the meaning of Narrowing becomes "VB will allow this
        // conversion with Option Strict Off although it might throw at runtime". And our design
        // decision for variance was that it just won't be allowed as a narrowing conversion.
    }
    else
    {
        return ConversionError;
    }
}


/*=======================================================================================
ClassifyAnyVarianceConversionToInterface(to interface TargetType, from SourceType)

This function
 * checks whether there is a conversion from SourceType to the interface TargetType
 * the target type is asserted to be an variance generic interface (there's no sense looking if it isn't)
 * the conversion must include a variance-conversion step
 * it may also include inherits, implements, is-constrained-by steps
 * if SourceType is a value-type then we judge whether its boxed form has the variance conversion
Note:
 * This function doesn't handle SourceType as arrays, and we assert to that effect

Result:
 * If there's a unique variance conversion, it returns ConversionWidening
 * If there's are multiple ambiguous variance conversions, it returns ConversionNarrowing
 * Otherwise it returns ConversionError
 * Note that it does *NOT* look for reverse variance conversions and call them Narrowing.
   It returns narrowing solely in cases of ambiguity. That's because reverse variance
   conversions are just a special case of conversions-to-interfaces, and rarely need
   any special treatment, so there's no sense wasting code+execution-time on them.

The following observation is crucial:
* Every chain of conversions can be rewritten into a chain which has a single variance-
  conversion as its final step. (because two successive variance-conversions can be
  rewritten as a single variance-conversion, and because variance-then-inheritance
  can be rewritten as inheritance-then-variance).
Because of this, if we're looking for a variance-conversion to some variance generic
interface X(Of Ti...), all we need do is look through the entire inheritance/implements
hierarchy of SourceType to look for X(Of Ui...) in it. We might find several, e.g.
X(Of Ai...) and X(Of Bi...) and X(Of Ci...). If any one of them is an exact match, then
variance-conversion will succeed. Otherwise, if a single one can be variance-converted
to TargetType, then variance-conversion will succeed. Otherwise, if several can be
variance-converted to TargetType, it's an ambiguity error.

Note: in this function we don't worry about any of intrinsic types.
That's because the only intrinsic type that's inheritance+variance-convertible is an array,
and this function specifically says that it doesn't work on arrays

The function is implemented inside a self-recursive helper function...
=======================================================================================*/
void
Helper_VarianceToInterface
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
    Type*&FoundPossibleCandidate,      // [out] - this is set for the first candidate that's a non-ConversionError match, allowing OverestimateNarrowingConversions.
    bool &FoundTwoPossibleCandidates,  // [out] - this is set if we find two or more such candidates
    bool &FoundWideningCandidate,      // [out] - this is set when we find a candidate that's an Widening match
    bool &FoundIdentityCandidate       // [out] - this is set when we find a candidate that's an Identity match
)
{
    VSASSERT(TargetType!=NULL && SourceType!=NULL, "unexpected NULL SourceType/TargetType");

    VSASSERT(!SourceType->IsArrayType(), "variance helper didn't expect to be asked to work on an array");

    if (SourceType->IsBad() || TargetType->IsBad() ||
         SourceType->IsArrayType() )  // Dev10 #839275 Prevent crash on an invalid input.
    {
        return;
    }

    VSASSERT(TargetType->IsInterface() && TargetType->IsGenericTypeBinding() && TypeHelpers::HasVariance(TargetType),
             "should only be looking for variance-interface conversions when target is a variance type");

    VSASSERT(SourceType->IsClass() || SourceType->IsInterface() || SourceType->IsGenericParam(),
             "should only be looking for variance-interface conversions from class, interface or generic pram");

    // We implement this function as a recursive search through the inherits+implements hierarchy.
    // We keep track of three variables:
    // * "FoundPossibleCandidate" is the first X(Of Ai...) candidate we've found
    //   that could possibly be converted to TargetType using full CLR rules, including narrowing/OverestimateNarrowConversions.
    //   (If we find a second candidate that might possible be widened, then it counts as an ambiguity).
    // * FoundTwoPossibleCandidates is set when we first find a second candidate. Note: even if we find
    //   ambiguity, we still can't abort the search and report "ambiguity error", in case
    //   later on we find an identity candidate.
    // * FoundIdentityCandidate is set when we find an exact match. At this point we can abort the search.
    // * FoundWideningCandidate is set when we first find a candidate that can be converted to TargetType
    //   using only VB's rules for widening.
    //
    // The "OverestimateNarrowingConversions" means that instead of using VB's narrowing criteria,
    // we expand it so we label as Narrowing *ANYTHING* that might potentially succeed at runtime
    // (if it's not alreayd identity/widening).

    ConversionClass Conversion = ClassifyImmediateVarianceCompatibility(
                                    TargetType,
                                    SourceType,
                                    SymbolCreator,
                                    CompilerHost,
                                    ConversionSemantics | ConversionSemantics::OverestimateNarrowingConversions, // see comment above
                                    RecursionCount, // no need to increment here
                                    IgnoreProjectEquivalence,
                                    ProjectForTargetType,
                                    ProjectForSourceType);

    // An exact match means that the conversion is guaranteed to succeed
    if (Conversion==ConversionIdentity)
    {
        FoundIdentityCandidate = true;
        return;
    }

    if (Conversion==ConversionWidening)
    {
        FoundWideningCandidate = true;
    }

    // If the conversion might possibly succeed, then we have to watch out for ambiguity
    if (Conversion!=ConversionError)
    {
        if (FoundPossibleCandidate==NULL)
        {
            // in this case this is the first candidate we've found: no ambiguity yet.
            FoundPossibleCandidate = SourceType; 
        }
        else if (BCSYM::AreTypesEqual(SourceType, FoundPossibleCandidate))
        {
            // this case isn't a problem: this isn't a geniune ambiguity
        }
        else
        {
            FoundTwoPossibleCandidates = true;
        }
    }

    // And also we look through all inherited/implemented/constrained-as types.
    
    if (SourceType->IsGenericParam())
    {
        BCSYM_GenericParam *SourceParam = SourceType->PGenericParam();
        for (GenericTypeConstraint *Constraint = SourceParam->GetTypeConstraints(); Constraint; Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }
            BCSYM *ConstraintType = Constraint->GetType();
            VSASSERT(ConstraintType, "unexpected null ConstraintType");

            // Dev10 #839275 Skip array types, Helper_VarianceToInterface can't handle them.
            if (ConstraintType->IsArrayType())
            {
                continue;
            }

            // recursive call to self:
            Helper_VarianceToInterface(
                    TargetType,
                    ConstraintType,
                    SymbolCreator,
                    CompilerHost,
                    ConversionSemantics,
                    RecursionCount, // Walking up the constraint tree, so simplifying problem, so don't increment RecursionCount
                    IgnoreProjectEquivalence,
                    ProjectForTargetType,
                    ProjectForSourceType,
                    FoundPossibleCandidate,
                    FoundTwoPossibleCandidates,
                    FoundWideningCandidate,
                    FoundIdentityCandidate);

            if (FoundIdentityCandidate)
            {
                return; // we can short-circuit the search
                // Actually, this path never happens in practice, because ClassifyPredefinedCLRConversion
                // (1) first checks for ClassifyPredefinedCLRConversion from a genericparam that has
                // a class constraint, and (2) then checks for inheritance, and the genericparam will
                // be counted as inheriting from all its interfaces. So any exact match reachable
                // through a generic param will have been caught by ClassifyPredefinedCLRConversion
                // before it even starts looking for variance. Nevertheless, we leave this branch
                // here for sake of completeness.
            }
        }
    }

    else
    {
        VSASSERT(SourceType->IsClass() || SourceType->IsInterface(), "we've already checked that SourceType is a class or interface");
        VSASSERT(SourceType->IsContainer(), "unexpected source type isn't a container");

        // 1. check base class
        if (SourceType->IsClass())
        {
            BCSYM *SourceBase = SourceType->PClass()->GetBaseClass();
            if (SourceBase!=NULL)
            {
                if (SourceType->IsGenericTypeBinding())
                {
                    SourceBase = ReplaceGenericParametersWithArguments(SourceBase, SourceType->PGenericTypeBinding(), SymbolCreator);
                }
                
                // recursive call to self:
                Helper_VarianceToInterface(
                        TargetType,
                        SourceBase,
                        SymbolCreator,
                        CompilerHost,
                        ConversionSemantics,
                        RecursionCount, // walking up inherits tree, so simplifying problem, so don't increment RecursionCount
                        IgnoreProjectEquivalence,
                        ProjectForTargetType,
                        ProjectForSourceType,
                        FoundPossibleCandidate,
                        FoundTwoPossibleCandidates,
                        FoundWideningCandidate,
                        FoundIdentityCandidate);

                if (FoundIdentityCandidate)
                {
                    return;
                }
            }
        }

        // 2. check interfaces
        BCITER_ImplInterfaces ImplInterfacesIter(SourceType->PContainer());
        while (BCSYM_Implements *Implements = ImplInterfacesIter.GetNext())
        {
            BCSYM *SourceImplements = Implements->GetCompilerRoot();
            VSASSERT(SourceImplements!=NULL, "unexpected null implements");
            if (SourceType->IsGenericTypeBinding())
            {
                SourceImplements = ReplaceGenericParametersWithArguments(SourceImplements, SourceType->PGenericTypeBinding(), SymbolCreator);
            }

            // recursive call to self:
            Helper_VarianceToInterface(
                    TargetType,
                    SourceImplements,
                    SymbolCreator,
                    CompilerHost,
                    ConversionSemantics,
                    RecursionCount, // walking up implements hierarchy, so simplifying problem, so don't increment RecursionCount
                    IgnoreProjectEquivalence,
                    ProjectForTargetType,
                    ProjectForSourceType,
                    FoundPossibleCandidate,
                    FoundTwoPossibleCandidates,
                    FoundWideningCandidate,
                    FoundIdentityCandidate);

            if (FoundIdentityCandidate)
            {
                return;
            }
        }
    }
   
}



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
)
{
    VSASSERT(TargetType!=NULL && SourceType!=NULL, "unexpected NULL SourceType/TargetType");

    if (TargetType->IsBad() || SourceType->IsBad())
    {
        return ConversionError;
    }

    VSASSERT(TargetType->IsInterface() && TargetType->IsGenericTypeBinding() && TypeHelpers::HasVariance(TargetType),
             "please only pass variant generic interfaces to HasVarianceConversionToInterface");

    VSASSERT(!SourceType->IsArrayType(), "please don't pass arrays to 'ClassifyAnyVarianceConversionToInterface': it doesn't handle them");

    VPASSERT(!BCSYM::AreTypesEqual(SourceType, TargetType), "Please catch the case SourceType==TargetType before calling ClassifyAnyVarianceConversion");


    if (SourceType->IsClass() && !SourceType->PClass()->IsEnum() && !SourceType->PClass()->IsStdModule()
        && !SourceType->PClass()->IsDelegate())
    {
        // a class can implement a variance interface
    }
    else if (SourceType->IsInterface())
    {
        // so can an interface
    }
    else if (SourceType->IsGenericParam())
    {
        // a generic param can implement a variance interface, if it has the right constraints
    }
    else
    {
        // but nothing else can implement a variance interface
        // (except for arrays, but we don't deal with them)
        return ConversionError;
    }

    if (!TypeHelpers::HasVariance(TargetType))
    {
        return ConversionError;
    }


    // We'll call a recursive helper function to look for candidate implemented interfaces,
    // and to detect ambiguities. These things are [out] parameters from the helper:
    Type *FoundPossibleCandidate = NULL;      // a candidate that might succeed at runtime using full CLR rules
    bool FoundTwoPossibleCandidates = false;  // set if we find a second such candidate
    bool FoundIdentityCandidate = false;      // did we find an exact match?
    bool FoundWideningCandidate = false;      // or one that's guaranteed to succeed by VB rules?

    // this is the recursive helper:
    Helper_VarianceToInterface(
            TargetType,
            SourceType,
            SymbolCreator,
            CompilerHost,
            ConversionSemantics,
            RecursionCount,  // We're just a wrapper for that function, so don't increment RecursionCount
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType, 
            FoundPossibleCandidate,
            FoundTwoPossibleCandidates,
            FoundWideningCandidate,
            FoundIdentityCandidate);

    if (FoundIdentityCandidate)
    {
        VPASSERT(!BCSYM::AreTypesEqual(SourceType, TargetType), "Please catch the case SourceType==TargetType before calling ClassifyAnyVarianceConversion");
        return ConversionWidening;
    }
    else if (FoundWideningCandidate && !FoundTwoPossibleCandidates)
    {
        return ConversionWidening;
    }
    else if (FoundTwoPossibleCandidates)
    {
        return ConversionNarrowing; // ambiguity is the SOLE reason that we'll return Narrowing
    }
    else
    {
        return ConversionError;
    }
}

/*=======================================================================================
ClassifyVarianceConversionToDelegate
Checks whether a given SourceType has a variance conversion to a given delegate TargetType and return its classification.

This function
 * asserts that TargetType is a delegate
 * DOES NOT WORK when given a SourceType that's a generic param. It has an assert to that effect.

Note that delegates inherit from System.MultiCastDelegate, which inherits from System.Delegate.
Also they're sealed.
So the only variance conversion possible for delegates is a single-step variance conversion
from one binding of a delegate to another binding of the same delegate.
(Or from a generic constrained to be a delegate, but as said we don't check for that case here.)
=======================================================================================*/
ConversionClass
ClassifyVarianceConversionToDelegate
(
    Type *TargetType,
    Type *SourceType,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    bool *bNarrowingCausedByContraVariance
)
{
    VSASSERT(TargetType!=NULL && SourceType!=NULL, "please don't call ClassifyVarianceConversionToDelegate with null types");
    if (TargetType->IsBad() || SourceType->IsBad())
    {
        return ConversionError;
    }
    VSASSERT(TargetType->IsDelegate(), "Please only call ClassifyVarianceConversionToDelegate with a delegate target");
    VSASSERT(!SourceType->IsGenericParam(), "Please don't call ClassifyVarianceConversionToDelegate with a generic-parameter source");

    if (!SourceType->IsDelegate())
    {
        return ConversionError; // shortcut: no non-generic non-delegate can have a variance conversion to a delegate
    }

    ConversionClass Conversion = 
        ClassifyImmediateVarianceCompatibility(
            TargetType,
            SourceType,
            SymbolCreator,
            CompilerHost, 
            ConversionSemantics, 
            RecursionCount,
            NULL,
            NULL,
            NULL,
            NULL,
            bNarrowingCausedByContraVariance
            );

    return Conversion; 
}


/*=======================================================================================
HasVarianceConversionToDelegate
Checks whether a given SourceType is guaranteed to have a variance conversion to a given delegate TargetType

This function
 * asserts that TargetType is a delegate
 * DOES NOT WORK when given a SourceType that's a generic param. It has an assert to that effect.

Note that delegates inherit from System.MultiCastDelegate, which inherits from System.Delegate.
Also they're sealed.
So the only variance conversion possible for delegates is a single-step variance conversion
from one binding of a delegate to another binding of the same delegate.
(Or from a generic constrained to be a delegate, but as said we don't check for that case here.)
=======================================================================================*/
bool
HasVarianceConversionToDelegate
(
    Type *TargetType,
    Type *SourceType,
    ConversionSemanticsEnum ConversionSemantics,
    int RecursionCount,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost
)
{
    ConversionClass Conversion = ClassifyVarianceConversionToDelegate(TargetType, SourceType, ConversionSemantics, RecursionCount, SymbolCreator, CompilerHost);

    switch (Conversion)
    {
        case ConversionIdentity:
        case ConversionWidening:
            return true;
        case ConversionError:
        case ConversionNarrowing:
            return false;
        default:
            VSFAIL("unexpected conversion");
            return false;
    }
}



static ConversionClass
ClassifyCLRArrayToInterfaceConversion
(
    Type *TargetInterface,
    ArrayType *SourceArray,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                         // How many times have we called ourselves recursively?
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTarget,         // The project containing a component of type "TargetInterface" that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSource          // The project containing a component of type "SourceArray" that was considered
                                                // for a successful match ignoring project equivalence.
)
{
    VSASSERT(TargetInterface->IsInterface(), "Non-Interface type unexpected!!!");
    VSASSERT(SourceArray->IsArrayType(), "Non-Array type unexpected!!!");

    // Arrays are effectively non-inheritable.
    // Going by $8.7 of the CLI spec "assignment compatibility", if you have a location of type T(),
    // the value stored in that location must be one of these:
    //   * T()
    //   * S() where S has a reference conversion to T ("array covariance")
    //   * or a host of possibilities when T is an integral or enum type, but these don't concern us here.
    //
    // Here are all the rules from $8.7 that govern what an array S() can convert to:
    //  * S() if S is an integral or enum, has an integral-array-conversions that don't concern us here.
    //  * S() has an inheritance conversion to System.Array, (this is implicit; it is not listed in array->GetFirstImplements)
    //         - System.Array has implements conversions to ICloneable, IList, ICollection, IEnumerable
    //  * S() has an array-covariance conversion to U() if S is reference-convertible to U
    //  * S() has an array-generic conversion to IList(Of U) if S=U, or S is reference-convertible to U
    //        - IList(Of U) has inheritance conversions to ICollection(Of U), IEnumerable(Of U), IEnumerable
    //        - IEnumerable(Of U) has further covariance conversions to IEnumerable(Of V) if U is reference-convertible to V
    //
    // In this function we are given the type T(), and an interface I.
    // The question is whether all possible values in T() -- i.e. either T() or S() -- can be converted to I.
    // If they all can be converted then we return Widening. If some might be converted but not necessarily
    // all then we return Narrowing. Otherwise we return Error.
    //
    // Note a concern about ambiguity through variance. If T implements IReadOnly(Of Mammal) and also 
    // implements IReadOnly(Of Fish), then the conversion from T() to IEnumerable(IReadOnly(Of Animal)) should
    // be counted as ambiguous too (i.e. narrowing).
    //
    // The reason that integral-array-conversions don't concern us is that they only convert from an
    // integral array to another integral array. And all integral arrays support exactly the same interfaces.
    // Note that array-covariance and generic-variance both work on reference conversions only, not on
    // value-types, so they're not affected by integral-array-conversions.
    //
    // e.g. [1] if the location has type Animal(), and the value in it has type Shrew(), and we wish to convert
    // to IList(Of Mammal), then it will succeed at runtime. But if the same location has in it a value of type
    // Animal() then it will fail at runtime. So Animal() -> IList(Of Mammal) is a narrowing conversion.
    //
    // e.g. [2] if the location has type Mammal(), and we wish to convert to IList(Of Animal), then every possible
    // value in the location is guaranteed to succeed. So Mammal() -> IList(Of Animal) is widening.
    //
    // e.g. [3] if the location has type Mammal(), and we wish to convert to IList(Of Car), then the conversion
    // is bound to fail since Mammal->Car is an error. So Mammal() -> IList(Of Car) is error.
    //
    // e.g. [4] if the location has type ChimeraFM() where ChimeraFM implements IEnumerable(Of Fish) and
    // IEnumerable(Of Mammal), and we wish to convert to IList(Of IEnumerable(Of Animal)) or to
    // IEnumerable(Of IEnumerable(Of Animal)), then there's a variance-ambiguity so we return Narrowing.
    //
    // e.g. [5] if the location has type Mammal(), and we wish to convert to IFred, then the conversion is 
    // bound to fail since we've listed above every possible interface that every array can support, and
    // IFred isn't in the list. So Mammal() -> IFred is error.
    // 
    //
    // OLD CODE: Up to Dev9 the code had listed this conversion:
    //      5. Some interface that inherits from IList(Of T) - Narrowing
    // But that was wrong. An array is guaranteed not to implement any interface except for those
    // we've already listed. So it should have said Error.
    //
    // Similarly, it said
    //      Multi-dimensional arrays do not support IList<T>
    //      if (SourceArray->GetRank() > 1) return ConversionNarrowing
    // And that too should have returned ConversionError.
    //
    // Also, it used to look for conversions via System.Array, then for conversions
    // to IList(Of U)/ICollection(Of U)/IEnumerable(Of U), and then for conversions
    // to non-generic interfaces implemented by IList(Of U). But actually there are
    // no interfaces (generic or non-generic) that are implemented by IList(Of U)
    // which we haven't already dealt considered. If the final check was an attempt
    // to future-proof the code against further things being implemented by IList
    // but not System.Array then I think it was a bad attempt, since you have to
    // look at conversions holistically (i.e. considering all possible transitive
    // closures of conversions). In this case it wasn't future-proofing against
    // future generic interfaces being implemented by IList(Of U), and it can't do
    // that anyway because we have to allow for array-variance first, and we can
    // be sure that the BCL will never implement a non-generic in IList(Of U) that
    // it doesn't also implement in IList.
    //
    


    // IMPLEMENTATION:
    // 1. Look for any conversions that start with inheritance to System.Array
    //    (this covers ICloneable, IList, ICollection, IEnumerable)
    // 2. If the target interface is IList(Of U) or ICollection(Of U) or IEnumerable(Of U),
    //    look for any conversions that start with array covariance T()->U()
    //    and then have a single array-generic conversion step U()->IList/ICollection/IEnumerable(Of U)
    //    Note: there's no need to look for further conversion steps after that --
    //    we've already covered all the interfaces that these things implement, and we've
    //    already covered covariance of IEnumerable(Of U).
    // 3. Otherwise return error.
    //



    // 1. Look for any conversions that start with array-inheritance to System.Array:
    //
    ConversionClass Conversion = ClassifyPredefinedCLRConversion(
            TargetInterface,
            CompilerHost->GetFXSymbolProvider()->GetType(FX::ArrayType),
            SymbolCreator,
            CompilerHost,
            ConversionSemantics::Default,
            RecursionCount,  // We've simplified the problem, so don't increment RecursionCount
            IgnoreProjectEquivalence,
            ProjectForTarget,
            ProjectForSource,
            NULL
            );
    
    VSASSERT(Conversion!=ConversionIdentity, "System.Array is not an interface, so shouldn't have been identity");
    // nb. if result is ConversionNarrowing, we'll discount it, because the only reason it might have
    // returned ConversionNarrowing is to allow for some class that inherits from System.Array and also ITargetInterface.
    // But here we know we're not dealing with any such class; we're dealing with an array.
    if (Conversion == ConversionWidening)
    {
        return ConversionWidening;
    }


    // 2. If the target interface is IList(Of U) or IReadOnlyList(Of U) or ICollection(Of U) or IEnumerable(Of U) or IReadOnlyCollection(Of U),
    //    look for any conversions that start with array covariance T()->U()
    //    and then have a single array-generic conversion step U()->IList/ICollection/IEnumerable(Of U)
    
    if (SourceArray->GetRank() == 1 &&   // Multi-dimensional arrays do not support IList(Of T)
        TargetInterface->IsGenericTypeBinding() &&
        ((CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIListType) &&  // some frameworks don't have it
          (TargetInterface->PGenericTypeBinding()->GetGeneric() == CompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIListType) ||
           TargetInterface->PGenericTypeBinding()->GetGeneric() == CompilerHost->GetFXSymbolProvider()->GetType(FX::GenericICollectionType) ||
           TargetInterface->PGenericTypeBinding()->GetGeneric() == CompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType))) ||
         (CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIReadOnlyListType) &&  // some frameworks don't have it
          (TargetInterface->PGenericTypeBinding()->GetGeneric() == CompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyListType) ||
           TargetInterface->PGenericTypeBinding()->GetGeneric() == CompilerHost->GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyCollectionType))))) 
    {
        Conversion = ClassifyCLRConversionForArrayElementTypes(
                        TargetInterface->PGenericTypeBinding()->GetArgument(0),
                        TypeHelpers::GetElementType(SourceArray),
                        SymbolCreator,
                        CompilerHost,
                        ConversionSemantics & ~ConversionSemantics::AllowBoxingConversions & ~ConversionSemantics::AllowIntegralConversions,
                        RecursionCount, // We've simplified the problem, so don't increment RecursionCount
                        IgnoreProjectEquivalence,
                        ProjectForTarget,
                        ProjectForSource);
        // Note: this call already does the right thing for array element types that are generic parameters,
        // and for target interfaces like IEnumerable(Of generic-param).
        // It also does the right thing for variance-ambiguity, i.e. returning Narrowing

        if (Conversion == ConversionIdentity || Conversion == ConversionWidening)
        {
            // Identity gets promoted to Widening here, because e.g. T()->IList(Of T) is a widening even if T->T is an identity
            return ConversionWidening;
        }
        else if (Conversion == ConversionNarrowing)
        {
            return ConversionNarrowing;
        }

        return ConversionNarrowing; // Dev10 #831390 Closely match Orcas behavior of this function to never return ConversionError.
    }

    return ConversionError;
}


ConversionClass
ClassifyCLRConversionForArrayElementTypes
(
    Type *TargetElementType,
    Type *SourceElementType,
    Symbols &SymbolCreator,
    CompilerHost *CompilerHost,
    ConversionSemanticsEnum ConversionSemantics,// allow boxing? value-conversions? non-VB conversions?
    int RecursionCount,                         // How many times have we called ourself recursively?
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType      // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
    
)
{

    // First, we'll see if the elements can be converted using the standard conversions (but no boxing or integral conversions)

    ConversionClass result =
            ClassifyCLRReferenceConversion(
                TargetElementType,
                SourceElementType,
                SymbolCreator,
                CompilerHost,
                ConversionSemantics & ~ConversionSemantics::AllowIntegralConversions & ~ConversionSemantics::AllowBoxingConversions,
                RecursionCount,  // This is a simplified problem, so don't increment RecursionCount
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType
                );

    // If that failed, then maybe the AllowIntegralConversions flag will allow something more...
    if (result==ConversionError && HASFLAG(ConversionSemantics, ConversionSemantics::AllowArrayIntegralConversions))
    {

        // Bug Dev10#463550
        // In doing this check for integral conversions, also consider the case where SourceElementType
        // is a generic parameter constrained to be something integral-like. You can't write these directly.
        // But you can through a sneaky trick involving inheritance...
        //    Enum E : a : End Enum 
        //    Interface IConstraintInjector(Of C) : Sub f(Of T As C)() : End Interface
        //    Class HasConstraint : Implements IConstraintInjector(Of E)
        //        Private Sub fe(Of T As E)() Implements IConstraintInjector(Of E).f
        //          Dim xx As T() = {}
        //          Dim yy As Integer() = xx ' uses T()->Integer()
        // NB. Notionally, if we discovered any ConstraintType->TargetElementType conversion that
        // was an identity, we should rewrite it as Widening since the extra step
        // T->ConstraintType->TargetElementType would make it a widening. But on the other hand, putting on
        // our wizard hats, we can see that any T satisfying ConstraintType here must be identical to
        // ConstraintType, so maybe it should be left as Identity!
        // In any case, the point is irrelevant, since none of the conversions here count as identity.
        Type *EffectiveSourceElementType = SourceElementType;
        if (TypeHelpers::IsGenericParameter(SourceElementType))
        {
            EffectiveSourceElementType =
                GetClassConstraint(
                    SourceElementType->PGenericParam(),
                    CompilerHost,
                    SymbolCreator.GetNorlsAllocator(),
                    false, // don't ReturnArraysAs"System.Array"
                    false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                    );
            if (EffectiveSourceElementType==NULL)
            {
                EffectiveSourceElementType = SourceElementType;
            }
        }

        // Also the case where TargetElementType is constrained to be something integral-like. 
        // This time, if we get identity/widening, it will have to be degraded to just narrowing.
        // After all, the fact that SourceElementType widens to TargetElementConstraint doesn't
        // necessarily mean that it widens to TargetElementType.
        // This check is done a little bit down...
        Type *EffectiveTargetElementType = TargetElementType;
        if (TypeHelpers::IsGenericParameter(TargetElementType))
        {
            EffectiveTargetElementType =
                GetClassConstraint(
                    TargetElementType->PGenericParam(),
                    CompilerHost,
                    SymbolCreator.GetNorlsAllocator(),
                    false, // don't ReturnArraysAs"System.Array"
                    false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                    );
            if (EffectiveTargetElementType==NULL)
            {
                EffectiveTargetElementType = TargetElementType;
            }
        }


        if (HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions))
        {
            if (TypeHelpers::IsIntegralType(EffectiveSourceElementType) && TypeHelpers::IsIntegralType(EffectiveTargetElementType))
            {
                if (TypeHelpers::IsEnumType(EffectiveSourceElementType) && !TypeHelpers::IsEnumType(EffectiveTargetElementType) &&
                    EffectiveSourceElementType->GetVtype() == EffectiveTargetElementType->GetVtype())
                {
                    result = ConversionWidening;
                    // VB says that enum()->underlying() is widening
                }
                else if (CbOfVtype(EffectiveSourceElementType->GetVtype()) == CbOfVtype(EffectiveTargetElementType->GetVtype()))
                {
                    result = ConversionNarrowing;
                    // CLR spec $8.7 says that integral()->integral() is possible so long as they have the same bitsize.
                    // It claims that bool is to be taken as the same size as int8/uint8, so allowing e.g. bool()->uint8().
                    // That isn't allowed in practice by the current CLR runtime, but since it's in the spec,
                    // we'll return "ConversionNarrowing" to mean that it might potentially possibly occur.
                    // Remember that we're in the business here of "OverestimateNarrowingConversions" after all.
                }
                else
                {
                    result = ConversionError;
                    // Even if we know we're trying to overestimate narrowing conversions, well, in this case
                    // we know that we have integral()->integral() of different bitsize, so we can authoritatively
                    // say that the conversion will never be possible under any circumstance.
                }
            }
            else if (TypeHelpers::IsIntegralType(EffectiveSourceElementType) && EffectiveTargetElementType->IsGenericParam() && !TypeHelpers::IsReferenceType(EffectiveTargetElementType))
            {
                result = ConversionNarrowing;
                // given a generic type (Of T), then the conversion int()->T() might possibly succeed, e.g. if T gets instantiated
                // with int or uint. That's why we return Narrowing here.
                // Actually, this code has already been dealt with by ClassifyPredefinedCLRConversion, since int->T counts as narrowing
            }
            else if (TypeHelpers::IsIntegralType(EffectiveTargetElementType) && EffectiveSourceElementType->IsGenericParam() && !TypeHelpers::IsReferenceType(EffectiveSourceElementType))
            {
                result = ConversionNarrowing;
                // Likewise, T()->int() might possibly succeed.
                // And likewise, it has already been dealt with by ClassifyPredefinedCLRConversion, since T->int counts as narrowing
            }
        }
        else
        {
            if (TypeHelpers::IsIntegralType(EffectiveSourceElementType) && TypeHelpers::IsIntegralType(EffectiveTargetElementType))
            {
                if (TypeHelpers::IsEnumType(EffectiveSourceElementType) && !TypeHelpers::IsEnumType(EffectiveTargetElementType) &&
                    EffectiveSourceElementType->GetVtype() == EffectiveTargetElementType->GetVtype())
                {
                    result = ConversionWidening;
                    // VB says that enum()->underlying() is widening
                }
                else if (TypeHelpers::IsEnumType(EffectiveTargetElementType) &&
                    EffectiveSourceElementType->GetVtype() == EffectiveTargetElementType->GetVtype())
                {
                    result = ConversionNarrowing;
                    // VB says that underlying()->enum() or enum()->enum() is narrowing, so long as they have the same bitsize.
                }
                else
                {
                    result = ConversionError;
                    // otherwise we know there's no conversion possible between these two integral() types
                }
            }
            else if (TypeHelpers::IsIntegralType(EffectiveSourceElementType) || TypeHelpers::IsIntegralType(EffectiveTargetElementType))
            {
                result = ConversionError;
                // Here we know that either source or target is an integral type, but we don't have any further
                // information. And the cases of inheritance/identity have already been dealt with. Therefore
                // there are no further grounds to justify a conversion between them.
            }
        }

        // Now we possible degrade the result, if it had depended on TargetElementType's constraint
        // rather than on TargetElementType itself
        if (TargetElementType != EffectiveTargetElementType)
        {
            if (result == ConversionIdentity || result == ConversionWidening)
            {
                result = ConversionNarrowing;
            }
        }

        // Well, that's all we can deduce from the possibility of array-element-value-conversions.
        // Maybe thanks to our additional array-integral checks, some things that ClassifyPredefinedCLRConversion
        // had classed as "error" can instead be classed as narrowing or even widening.
    }


#if 0
    // This function has two implementations, an old and a new
    // For the moment we keep both of them in place.
    // Once we're more confident that the new implementation gives the same results as the old, then we'll
    // remove the old.
    // The old one should be removed prior to shipping Dev10. I think that's enough time. (Microsoft, 31/May/2008)
    //
    // The old implementation doesn't implement the OverestimateNarrowingConversions flag.
    // Also, it only works when AllowArrayIntegralConversions==AllowIntegralConversions.
    // And the only integral conversion it considered was "Enum < UnderlyingType",
    // and the only array integral conversion it considered was "Enum() < UnderlyingType()".
    // By that trick, it combined the two things into a single flag, and didn't handle
    // the array integral functions in here at all.
    // (it's not a good trick, because the CLR really does use different conversions
    // in the two cases, e.g. int32->bool as an integral conversion, but int8()->bool()
    // as an array integral conversion; and so on.)
    //

    ConversionClass oldresult;

    if (TypeHelpers::IsReferenceType(SourceElementType) &&
        TypeHelpers::IsReferenceType(TargetElementType))
    {
        oldresult =
            ClassifyPredefinedCLRConversion(
                TargetElementType,
                SourceElementType,
                SymbolCreator,
                CompilerHost,
                ConversionSemantics, // since they're both references, it doesn't matter if this includes value-conversions or not, since no value-conversions will be used
                RecursionCount, // Simplified problem, so don't increment RecursionCount
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType,
                NULL
                );
    }

    else if (TypeHelpers::IsValueType(SourceElementType) &&
             TypeHelpers::IsValueType(TargetElementType))
    {
        oldresult =
            ClassifyPredefinedCLRConversion(
                TargetElementType,
                SourceElementType,
                SymbolCreator,
                CompilerHost,
                ConversionSemantics,
                RecursionCount, // Simplified problem, so don't increment RecursionCount
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType,
                NULL
                );
    }

    // Bug VSWhidbey 369131.
    // Array co-variance and back-casting special case for generic parameters.
    //
    else if (TypeHelpers::IsGenericParameter(SourceElementType) &&
             TypeHelpers::IsGenericParameter(TargetElementType))
    {
        if (TypeHelpers::EquivalentTypes(SourceElementType, TargetElementType))
        {
            oldresult = ConversionIdentity;
        }

        else if (TypeHelpers::IsReferenceType(SourceElementType) &&
                 TypeHelpers::IsOrInheritsFrom(
                    SourceElementType,
                    TargetElementType,
                    SymbolCreator,
                    false,
                    NULL,
                    IgnoreProjectEquivalence,
                    ProjectForSourceType,
                    ProjectForTargetType))
        {
            oldresult = ConversionWidening;
        }

        else if (TypeHelpers::IsReferenceType(TargetElementType) &&
                 TypeHelpers::IsOrInheritsFrom(
                    TargetElementType,
                    SourceElementType,
                    SymbolCreator,
                    false,
                    NULL,
                    IgnoreProjectEquivalence,
                    ProjectForTargetType,
                    ProjectForSourceType))
        {
            oldresult = ConversionNarrowing;
        }
        
        else
        {
            oldresult = ConversionError;
        }
    }

    else
    {
        oldresult = ConversionError;
    }

    if (!HASFLAG(ConversionSemantics, ConversionSemantics::OverestimateNarrowingConversions) &&
        HASFLAG(ConversionSemantics, ConversionSemantics::AllowIntegralConversions) == HASFLAG(ConversionSemantics, ConversionSemantics::AllowArrayIntegralConversions) &&
        !TypeHelpers::IsGenericParameter(SourceElementType) && !TypeHelpers::IsGenericParameter(TargetElementType))
    {
        VSASSERT(oldresult == result, "Microsoft's conversion reimplementation (2008.June.28) disagree with the old ForArrayElementConversion");
    }
#endif

    return result;
}

ConversionClass
Semantics::ClassifyArrayLiteralConversion
(
    Type *TargetType,
    BCSYM_ArrayLiteralType *SourceType
)
{
    VSASSERT(SourceType!=NULL && TargetType!=NULL, "please don't call ClassifyArrayLiteralConversion with null arguments");

    // An array literal {e0,e1} can be converted to...
    //   * an array T() so long as each element can be converted to T
    //   * IEnumerable(Of T) / ICollection(Of T) / IList(Of T) so long as each element can be converted to T
    //   * IEnumerable / ICollection / IList
    //   * System.Array / System.Object
    // Note that array literal conversions count as "predefined" conversions, and so user-defined conversions
    // have already been taken into account previously before the predefined conversions are exercised.
    //
    // Incidentally, it might be that all elements can each convert to T even though
    // the array literal doesn't have a dominant type -- e.g. {lambda} or {} have no dominant type.
    // And if there are two elements {e0,e1} such that e0 narrows to e1 and e1 narrows to e0 then
    // again there's no dominant type.
    //
    // Observe that if every element has a reference conversion to T, then the conversions
    // from {e0,e1} to T()/IEnumerable(Of T)/ICollection(Of T)/IList(Of T) are all predefined CLR conversions.
    // Observe that the conversions to IEnumerable/ICollection/IList/System.Array/System.Object
    // are always predefined CLR conversions.
    
    if (TargetType->IsObject() || TargetType==GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ArrayType) ||
        ((SourceType->GetRank()==1 || IsEmptyArrayLiteralType(SourceType)) &&
         (TargetType==GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::IEnumerableType) ||
          TargetType==GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ICollectionType) ||
          TargetType==GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::IListType))))
    {
        return ConversionWidening;
    }

    // For the others, we have to make a check that the elements can be converted to T, i.e. TargetElementType
    // Suppose each element->T is an identity. Well, {e0,e1}->T() would then be an identity.
    // But {e0,e1}->IEnumerable(Of T) would merely be widening. In other words, we might have to weaken the conversion.
    Type *TargetElementType = NULL;
    bool MustWeakenConversion;


    if (TargetType->IsArrayType() &&
        (SourceType->GetRank() == TargetType->PArrayType()->GetRank() || IsEmptyArrayLiteralType(SourceType)))
    {
        TargetElementType = TargetType->PArrayType()->GetRoot();
        MustWeakenConversion = false;
    }

    else if ((SourceType->GetRank()==1 || IsEmptyArrayLiteralType(SourceType)) &&
             TargetType->IsGenericBinding() && 
             ((GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIListType) &&
               (TargetType->PGenericBinding()->GetGeneric() == GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType) ||
                TargetType->PGenericBinding()->GetGeneric() == GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericICollectionType) ||
                TargetType->PGenericBinding()->GetGeneric() == GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericIListType))) ||
              (GetCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIReadOnlyListType) &&
               (TargetType->PGenericBinding()->GetGeneric() == GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyListType) ||
                TargetType->PGenericBinding()->GetGeneric() == GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyCollectionType)))))
               
    {
        TargetElementType = TargetType->PGenericBinding()->GetArgument(0);
        MustWeakenConversion = true;
    }

    else
    {
        return ConversionError;
    }


    // Now we have to check that every element converts to TargetElementType.
    // It's tempting to say "if the dominant type converts to TargetElementType, then it must be true that
    // every element converts." But this isn't true for several reasons. First, the dominant type might
    // be the unique NARROWING candidate among the elements in the case that no widest elements existed.
    // Second, even assuming that every element did id/widen to the dominant type, then we still wouldn't
    // know whether the result of every conversion would be identity or widening.
    // So we don't bother with the shortcut.
    
    // Here we keep running track of the best we can do for converting each element to T.
    // Incidentally, if the array literal is empty, then we'll be left with ConversionIdentity as desired.
    ConversionClass result = ConversionIdentity; 

    ConstIterator<ILTree::Expression *> elementIterator = SourceType->PArrayLiteralType()->GetElements();
    while (elementIterator.MoveNext() && result != ConversionError) // "!= ConversionError" is a shortcut out of the loop
    {
        ConversionClass elementConversion = ClassifyConversionFromExpression(TargetElementType, elementIterator.Current());
        result = WorstConversion(result, elementConversion);
    }

    if (MustWeakenConversion)
    {
        result = WorstConversion(result, ConversionWidening);
    }

    return result;
}


bool 
Semantics::IsEmptyArrayLiteralType
(
    Type * pType
)
{
	VSASSERT(!pType->IsArrayLiteralType() || pType->PArrayLiteralType()->GetRoot()!=NULL, "unexpected: an array literal with null element type");

	return
        pType->IsArrayLiteralType() && 
		pType->PArrayLiteralType()->GetRoot()!=NULL &&
        pType->PArrayLiteralType()->GetRoot()->IsObject() && 
        pType->PArrayLiteralType()->GetElementCount() == 0 &&
        pType->PArrayLiteralType()->GetRank() == 1;
}


/*=======================================================================================
ClassifyPredefinedConversion

This function classifies all intrinsic language conversions, such as inheritance,
implementation, array covariance, and conversions between intrinsic types.
=======================================================================================*/
ConversionClass
Semantics::ClassifyPredefinedConversion
(
    Type *TargetType,
    Type *SourceType,
    bool IgnoreProjectEquivalence,              // Consider types as equivalent based on their project equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType,     // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
    bool ignoreNullablePredefined,              // Nullable conversions apply only no other regular conversion exists.
                                                // Use this flag to avoid infinite recursion.
    bool considerConversionsOnNullableBool,     // Note: This flag is different than ignoreNullablePredefined. In particular, it controls wether or not we consider
                                                //Booealn->Boolean? or Boolean?->Boolean conversions. If this is false, we will still consider all other nullable
                                                //conversions. It is named considerConversionsOnNullableBool because the conversions from T?->T are actually implemented
                                                //as userdefined conversions on the Nullable(of T) type, even though we treat them like predefined conversions.
    bool * pConversionRequiresUnliftedAccessToNullableValue, // [out] - Returns true if the classified conversion requires a T?->T conversion.
    bool *pConversionIsNarrowingDueToAmbiguity, // [out] did we return "Narrowing" for reason of ambiguity, e.g. multiple variance-conversions?
    DelegateRelaxationLevel *pConversionRelaxationLevel // [out] If converting a VB$AnonymousDelegate to a delegate, how much relaxing went on?
)
{

    if (pConversionRequiresUnliftedAccessToNullableValue)
    {
        *pConversionRequiresUnliftedAccessToNullableValue = false;
    }
    if (pConversionIsNarrowingDueToAmbiguity)
    {
        *pConversionIsNarrowingDueToAmbiguity = false;
    }
    if (pConversionRelaxationLevel)
    {
        *pConversionRelaxationLevel = DelegateRelaxationLevelNone;
    }

    // Make an easy pointer comparison for a common case.  More complicated type comparisons will happen later.
    if (TargetType == SourceType)
    {
        return ConversionIdentity;
    }

    if (!TargetType || !SourceType)
    {
        return ConversionError;
    }

    if (SourceType->IsArrayLiteralType())
    {
        ConversionClass result = ClassifyArrayLiteralConversion(TargetType, SourceType->PArrayLiteralType());
        if (result != ConversionError)
        {
            return result;
        }
    }

    if ( !ignoreNullablePredefined)
    {
        bool IsNullableInvolved = false;
        ConversionClass result = CheckNullableConversion
        (
            IsNullableInvolved,
            TargetType,
            SourceType,
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType,
            considerConversionsOnNullableBool,
            pConversionRequiresUnliftedAccessToNullableValue
        );

        if (IsNullableInvolved)
        {
            return result;
        }
    }

    Vtypes TargetVtype = TargetType->GetVtype();
    Vtypes SourceVtype = SourceType->GetVtype();

    if (TargetVtype < t_ref && SourceVtype < t_ref &&
        TargetVtype >= t_bad && SourceVtype >= t_bad)
    {
        if (TypeHelpers::IsEnumType(TargetType))
        {
            // Need to check equivalent types because enums could be
            // generic bindings.
            //
            if (TypeHelpers::IsEnumType(SourceType) &&
                TypeHelpers::EquivalentTypes(
                    SourceType,
                    TargetType,
                    IgnoreProjectEquivalence,
                    ProjectForSourceType,
                    ProjectForTargetType))
            {
                return ConversionIdentity;
            }

            if (TypeHelpers::IsIntegralType(SourceType) && TypeHelpers::IsIntegralType(TargetType))
            {
                // Conversion from an integral type (including an Enum type)
                // to an Enum type (that has an integral underlying type)
                // is narrowing. Enums that come from metadata do not necessarily
                // have integral underlying types.

                return ConversionNarrowing;
            }
        }

        if (SourceVtype == TargetVtype && TypeHelpers::IsEnumType(SourceType))
        {
            // Conversion from an Enum to it's underlying type is widening.
            // If we used the table, this kind of conversion would be classified
            // as identity, and that would not be good. Catch this case here.

            return ConversionWidening;
        }

        return ConversionClassTable[TargetVtype - t_bad][SourceVtype - t_bad];
    }

    // Try VB specific conversion: String --> Char()
    if (IsArrayType(SourceVtype))
    {
        if (TypeHelpers::IsStringType(TargetType) && TypeHelpers::IsCharArrayRankOne(SourceType))
        {
            // Array of Char widens to String.
            return ConversionWidening;
        }
    }
    // Try VB specific conversion: Char() --> String
    else if (IsArrayType(TargetVtype))
    {
        if (TypeHelpers::IsStringType(SourceType) && TypeHelpers::IsCharArrayRankOne(TargetType))
        {
            // String requires a narrowing conversion to Array of Char
            return ConversionNarrowing;
        }
    }

    // Try anonymous delegate conversion a anonymous delegate is convertable to any other delegate
    // if the signature is matches
    if (SourceType->IsAnonymousDelegate() &&
        TypeHelpers::IsDelegateType(TargetType) &&
        TargetType != GetFXSymbolProvider()->GetMultiCastDelegateType())
    {
        MethodConversionClass methodConversion = ClassifyDelegateInvokeConversion(
            TargetType,
            SourceType);
        if (IsSupportedMethodConversion(
                false, // Option Strict off since we want to check for both to mark narrowing or widening.
                methodConversion))
        {
            DelegateRelaxationLevel ConversionRelaxationLevel = DetermineDelegateRelaxationLevel(methodConversion);

            if (pConversionRelaxationLevel)
            {
                *pConversionRelaxationLevel = ConversionRelaxationLevel;
            }

            switch (ConversionRelaxationLevel)
            {
            case DelegateRelaxationLevelNone:
                return (TargetType->IsAnonymousDelegate()) ? ConversionIdentity : ConversionWidening;
            case DelegateRelaxationLevelWidening:
            case DelegateRelaxationLevelWideningDropReturnOrArgs:
            case DelegateRelaxationLevelWideningToNonLambda:
                return ConversionWidening;
            case DelegateRelaxationLevelNarrowing:
                return ConversionNarrowing;
            }
        }
    }

    ConversionClass result = ClassifyPredefinedCLRConversion(
                TargetType,
                SourceType,
                ConversionSemantics::Default,
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType,
                pConversionIsNarrowingDueToAmbiguity);

    // Dev10#703313: for e.g. Func(Of String)->Func(Of Object) we have to record it as DelegateRelaxationLevelWidening.
    // for e.g. Func(Of String)->Object we have to record it as DelegateRelaxationLevelWideningToNonLambda
    if (pConversionRelaxationLevel && TypeHelpers::IsDelegateType(SourceType))
    {
        if (TargetType == NULL ||
            !TypeHelpers::IsDelegateType(TargetType) || 
            TypeHelpers::IsStrictSupertypeOfConcreteDelegate(TargetType, GetFXSymbolProvider())) // covers Object, System.Delegate, System.MulticastDelegate
        {
            *pConversionRelaxationLevel = DelegateRelaxationLevelWideningToNonLambda;
        }
        else if (result == ConversionIdentity)
        {
            *pConversionRelaxationLevel = DelegateRelaxationLevelNone;
        }
        else if (result == ConversionWidening)
        {
            *pConversionRelaxationLevel = DelegateRelaxationLevelWidening;
        }
        else if (result == ConversionNarrowing)
        {
            *pConversionRelaxationLevel = DelegateRelaxationLevelNarrowing;
        }
        else if (result == ConversionError)
        {
            *pConversionRelaxationLevel = DelegateRelaxationLevelInvalid;
        }
    }

    return result;
}

ConversionClass
Semantics::ClassifyPredefinedConversion
(
    Type *TargetType,
    Type *SourceType,
    CompilerHost *CompilerHost
)
{
    NorlsAllocator Storage(NORLSLOC);
    Semantics Analyzer(&Storage, NULL, CompilerHost->GetCompiler(), CompilerHost, NULL, NULL, NULL, false);

    return Analyzer.ClassifyPredefinedConversion(TargetType, SourceType);
}
/*=======================================================================================
ClassifyMethodConversion

This function classifies conversions involving two methods where the first Proc is
typically the method being assigned to a delegate and the second proc is the invoke of
the delegate class.
=======================================================================================*/
MethodConversionClass
Semantics::ClassifyDelegateInvokeConversion
(
    Type* TargetDelegate,
    Type* SourceDelegate
)
{
    // Terminology switch for regular conversion / assignment the naming convention is:
    //      TargetDelegate = SourceSource
    // For MethodConversion the naming convention is:
    //      InvokeMethod = AddressOf TargetDelegate.Invoke

    Declaration *TargetMethod = GetInvokeFromDelegate(SourceDelegate, m_Compiler);
    if (TargetMethod == NULL || IsBad(TargetMethod) || !TargetMethod->IsProc())
    {
        return MethodConversionError;
    }
    BCSYM_GenericBinding* BindingForTarget = TypeHelpers::IsGenericTypeBinding(SourceDelegate) ? SourceDelegate->PGenericTypeBinding() : NULL;

    Declaration *InvokeMethod = GetInvokeFromDelegate(TargetDelegate, m_Compiler);
    if (InvokeMethod == NULL || IsBad(InvokeMethod) || !InvokeMethod->IsProc())
    {
        return MethodConversionError;
    }
    BCSYM_GenericBinding* BindingForInvoke = TypeHelpers::IsGenericTypeBinding(TargetDelegate) ? TargetDelegate->PGenericTypeBinding() : NULL;

    return ClassifyMethodConversion(
        TargetMethod->PProc(),
        BindingForTarget,
        InvokeMethod->PProc(),
        BindingForInvoke,
        false, // IgnoreReturnValueErrorsForInference
        &m_SymbolCreator,
        m_CompilerHost);

}

/*=======================================================================================
ClassifyMethodConversion

This function classifies conversions involving two methods where the first Proc is
typically the method being assigned to a delegate and the second proc is the invoke of
the delegate class.
=======================================================================================*/
MethodConversionClass
Semantics::ClassifyMethodConversion
(
    BCSYM_Proc *TargetMethod,
    GenericBindingInfo BindingForTarget,
    BCSYM_Proc *InvokeMethod,
    BCSYM_GenericBinding *BindingForInvoke,
    bool IgnoreReturnValueErrorsForInference,
    Symbols *SymbolFactory,
    CompilerHost *CompilerHost
)
{
    NorlsAllocator Storage(NORLSLOC);
    Semantics Analyzer(&Storage, NULL, CompilerHost->GetCompiler(), CompilerHost, NULL, NULL, NULL, false);

    return Analyzer.ClassifyMethodConversion(TargetMethod, BindingForTarget, InvokeMethod, BindingForInvoke, IgnoreReturnValueErrorsForInference, SymbolFactory, false);
}

/*=======================================================================================
ClassifyMethodConversion

This function classifies conversions involving two methods where the first Proc is
typically the method being assigned to a delegate and the second proc is the invoke of
the delegate class.
=======================================================================================*/
MethodConversionClass
Semantics::ClassifyMethodConversion
(
    BCSYM_Proc *TargetMethod,
    GenericBindingInfo BindingForTarget,
    BCSYM_Proc *InvokeMethod,
    BCSYM_GenericBinding *BindingForInvoke,
    bool IgnoreReturnValueErrorsForInference,
    Symbols *SymbolFactory,
    bool TargetIsExtensionMethod
)
{
    if (IsProcAllowedAsDelegate(TargetMethod))
    {

        Parameter * pParam = TargetMethod->GetFirstParam();

        if (TargetIsExtensionMethod)
        {
            ThrowIfNull(pParam);
            pParam = pParam->GetNext();
        }

        return ClassifyMethodConversion
        (
            GetReturnType(TargetMethod),
            pParam,
            TargetMethod->IsDllDeclare(),
            BindingForTarget,
            InvokeMethod,
            BindingForInvoke,
            IgnoreReturnValueErrorsForInference,
            SymbolFactory
        );
    }
    else
    {
        return MethodConversionNotAMethod;
    }
}

/*=======================================================================================
ClassifyMethodConversion

This function classifies conversions involving two methods where the first Proc is
typically the method being assigned to a delegate and the second proc is the invoke of
the delegate class.
=======================================================================================*/
MethodConversionClass
Semantics::ClassifyMethodConversion
(
    Type *TargetRetType,
    BCSYM_Param *TargetParameters,
    bool TargetMethodIsDllDeclare,
    GenericBindingInfo BindingForTarget,
    BCSYM_Proc *InvokeMethod,
    BCSYM_GenericBinding *BindingForInvoke,
    bool IgnoreReturnValueErrorsForInference,
    Symbols *SymbolFactory
)
{
    MethodConversionClass MethodConversion = MethodConversionIdentity;

    DBG_SWITCH_PRINTF(fDumpRelaxedDel, "MethConv: Classifyint method conversion\n");

    if (!IgnoreReturnValueErrorsForInference && (!TargetRetType || !TargetRetType->IsBad()))
    {
        // Get the real return types of the target method & delegate
        Type *DelegateRetType = GetReturnType(InvokeMethod);
        DelegateRetType = ReplaceGenericParametersWithArguments(DelegateRetType, BindingForInvoke, *SymbolFactory);
        TargetRetType = ReplaceGenericParametersWithArguments(TargetRetType, BindingForTarget, *SymbolFactory);

        ClassifyReturnTypeForMethodConversion(TargetRetType, DelegateRetType, MethodConversion);
    }

    Type *FixedTargetType = NULL;

    // Run through the parameters to see how they compare between the delegate & target method
    BCSYM_Param *DelegateParam = InvokeMethod->GetFirstParam(); // first parameter in delegate signature
    BCSYM_Param *TargetParam = TargetParameters; // first parameter in target method signature

    if (TargetParam == NULL && DelegateParam != NULL)
    {
        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionAllArgumentsIgnored\n");
        MethodConversion |= MethodConversionAllArgumentsIgnored;
    }
    else
    {
        while ( TRUE )
        {
            if ( !DelegateParam || !TargetParam) break; // Done comparing parameters


            // Get the real parameter type of the target method & delegate
            Type *DelegateType = GetDataType(DelegateParam);
            DelegateType = ReplaceGenericParametersWithArguments(DelegateType, BindingForInvoke, *SymbolFactory);
            bool DelegateTypeIsByReference = false;
            if (DelegateType)
            {
                if (TypeHelpers::IsPointerType(DelegateType))
                {
                    // This occurs for references to symbols for Byref arguments.
                    DelegateType = TypeHelpers::GetReferencedType(DelegateType->PPointerType());
                    DelegateTypeIsByReference = true;
                }
            }

            Type *TargetType = GetDataType(TargetParam);
            TargetType = ReplaceGenericParametersWithArguments(TargetType, BindingForTarget, *SymbolFactory);
            bool TargetTypeIsByReference = false;
            if (TargetType)
            {
                if (TypeHelpers::IsPointerType(TargetType))
                {
                    // This occurs for references to symbols for Byref arguments.
                    TargetType = TypeHelpers::GetReferencedType(TargetType->PPointerType());
                    TargetTypeIsByReference = true;
                }

                // Devdiv Bug[22903]
                if (TargetMethodIsDllDeclare &&
                    !TargetParam->GetPWellKnownAttrVals()->GetMarshalAsData() &&
                    TargetType->GetVtype() == t_string)
                {
                    // PInvoke HACK: In VB, ByVal String reflects changes
                    // back through Declares. So under the covers make this
                    // a pointer to a string. We'll decorate the parameter when
                    // we output the parameter spec.
                    TargetTypeIsByReference = true;
                }
            }
            if (TargetTypeIsByReference != DelegateTypeIsByReference)
            {
                DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionByRefByValMismatch of '%S'\n", TargetParam->GetName());
                MethodConversion |= MethodConversionByRefByValMismatch;
            }

            ClassifyArgumentForMethodConversion(
                TargetType,
                DelegateType,
                TargetParam->IsParamArray(),
                BindingForTarget,
                SymbolFactory,
                FixedTargetType,
                MethodConversion);

            if (TargetTypeIsByReference)
            {
                VSASSERT(!TargetParam->IsParamArray(), "ByRef ParamArray is not Supported.");

                ClassifyArgumentForMethodConversion(
                    DelegateType,
                    TargetType,
                    false,
                    BindingForTarget,
                    SymbolFactory,
                    FixedTargetType,
                    MethodConversion);
            }

            // Move to the next parameter in the list
            if (DelegateParam) DelegateParam = DelegateParam->GetNext();
            if (TargetParam && !FixedTargetType) TargetParam = TargetParam->GetNext();
        } // End looping through the parameters

        if (DelegateParam)
        {
            VSASSERT(TargetParam == NULL, "Can't have parameters left on both input and output at the same time");
            if (!DelegateParam->IsParamArray())
            {
                // We have parameters left on the delegate. This is safe to ignore when we are building the sutb.
                // We don't need one if the parameter left is of type ParamArray.
                DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionExcessParamsArgumentOnDelegate of '%S'\n", DelegateParam->GetName());
                MethodConversion |= MethodConversionExcessParamsArgumentOnDelegate;
            }
        }
        else if (TargetParam)
        {
            VSASSERT(DelegateParam == NULL, "Can't have parameters left on both input and output at the same time");

            // We have extra parameter left on the target method. This is only allowed if those parameters are optional or paramarray.
            // This has an explicit requirement that the delegate itself cannot have these types of parameters.
            while ( TargetParam )
            {
                if (TargetParam->IsOptional() || TargetParam->IsParamArray())
                {
                    DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionExcessOptionalArgumentsOnTarget of '%S'\n", TargetParam->GetName());
                    MethodConversion |= MethodConversionExcessOptionalArgumentsOnTarget;
                }
                else
                {
                    // Invalid match, Can't generate a stub for this.
                    DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionError of '%S'\n", TargetParam->GetName());
                    MethodConversion |= MethodConversionError;
                }

                if (TargetParam) TargetParam = TargetParam->GetNext();
            }
        }
    }
    return MethodConversion;
}

/*=======================================================================================
ClassifyReturnTypeForMethodConversion
=======================================================================================*/
void
Semantics::ClassifyReturnTypeForMethodConversion
(
    Type *TargetRetType,
    Type *DelegateRetType,
    MethodConversionClass &MethodConversion
)
{
    // Unify the return types.
    DelegateRetType = DelegateRetType != NULL ? DelegateRetType : TypeHelpers::GetVoidType();
    TargetRetType = TargetRetType != NULL ? TargetRetType : TypeHelpers::GetVoidType();

#if DEBUG
        StringBuffer targetBuf;
        StringBuffer delegateBuf;

        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "  MethConv: ClassifyReturnType %S -> %S\n",
            ExtractErrorName(TargetRetType, targetBuf),
            ExtractErrorName(DelegateRetType, delegateBuf));
#endif

    // Verify return types conform to CLR rules for delegate signature matching
    ConversionClass Conversion = ConversionError;

    if ( (TypeHelpers::IsRootObjectType(DelegateRetType) && TypeHelpers::IsVoidType(TargetRetType)) ||
         (TypeHelpers::IsRootObjectType(TargetRetType) && TypeHelpers::IsVoidType(DelegateRetType)))
    {
        // Bug: 44858: object to void and vice versa are not error, but by default widening/narrowing. We need error to have func/sub logic.
        Conversion = ConversionError;
    }
    else
    {
        Procedure * OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool OperatorMethodIsLifted;

        Conversion = ClassifyConversion(DelegateRetType, TargetRetType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
    }

    if (Conversion == ConversionError)
    {
        if (DelegateRetType->IsVoidType() && !TargetRetType->IsVoidType())
        {
            // Special case to allow function foo to be hooked to delegate sub dropping the result.
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionReturnValueIsDropped\n");
            MethodConversion |= MethodConversionReturnValueIsDropped;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionError (ReturnVaule has ConversionError) \n");
            MethodConversion |= MethodConversionError;
        }
    }
    // Correct to check for widening since we are comparing from delegate to Target is what the stub will really does
    else if (Conversion == ConversionWidening)
    {
        ConversionClass ClrConversion = ClassifyPredefinedCLRConversion(DelegateRetType, TargetRetType, ConversionSemantics::Default);

        // Bug 73604. Generic Reference type needs to be treated specially.
        if (ClrConversion == ConversionWidening &&
            // Bug 114543: The CLR will not relax on value types, only reference types
            // so treat these as relaxations that needs a stub. For Arguments it works fine.
            TypeHelpers::IsReferenceType(TargetRetType) &&
            TypeHelpers::IsReferenceType(DelegateRetType))
        {
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionReturnIsClrNarrowing \n");
            MethodConversion |= MethodConversionReturnIsClrNarrowing;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionReturnIsIsVbOrBoxNarrowing \n");
            MethodConversion |= MethodConversionReturnIsIsVbOrBoxNarrowing;
        }
    }
    // Correct to check for narrowing since we are comparing from delegate to Target
    // is what the stub will really does
    else if (Conversion == ConversionNarrowing)
    {
        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionReturnIsWidening \n");
        MethodConversion |= MethodConversionReturnIsWidening;
    }
    // $
}

/*=======================================================================================
ClassifyArgumentForMethodConversion
=======================================================================================*/
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
)
{
    Procedure * OperatorMethod = NULL;
    GenericBinding *OperatorMethodGenericContext = NULL;
    bool OperatorMethodIsLifted;
    ConversionClass Conversion = ConversionError;

#if DEBUG
        StringBuffer targetBuf;
        StringBuffer delegateBuf;

        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "  MethConv: ClassifyArguments %S -> %S\n",
            ExtractErrorName(TargetType, targetBuf),
            ExtractErrorName(DelegateType, delegateBuf));
#endif

    if (FixedTargetType)
    {
        ThrowIfFalse(TargetIsParamArray);

        TargetType = FixedTargetType;
        Conversion = ClassifyConversion(TargetType, DelegateType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
    }
    else
    {
        // Compare the types
        Conversion = ClassifyConversion(TargetType, DelegateType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

        // if it failed and the target parameter is a ParamArray, get the type of the paramarray and match
        // keep looping over the remaining delegate parameters and maintain FixedTargetType as the
        // ArrayType of the ParamArray parameter.
        if (Conversion == ConversionError && TargetIsParamArray)
        {
            // Check if it can't match the argument directly to see if an expanded ParamArray will work.
            // From now on all DelegateParams must have a conversion to the type of the ParamArray.
            // We enforce this by not progressing the TargetParam. We do not do this with DelegateParam so
            // therefore we are guaranteed not to livelock.
            BCSYM_ArrayType* ArrayType = TargetType->PArrayType();
            VSASSERT(ArrayType, "ParamsArray Must ba an ArrayType");
            if (ArrayType)
            {
                TargetType = ArrayType->GetRoot ();
                TargetType = ReplaceGenericParametersWithArguments(TargetType, BindingForTarget, *SymbolFactory);
            }
            FixedTargetType = TargetType;

            OperatorMethod = NULL;
            OperatorMethodGenericContext = NULL;

            // Verify parameter conforms to CLR rules for delegate signature matching
            Conversion = ClassifyConversion(TargetType, DelegateType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);
        }
    }

    if (Conversion == ConversionError)
    {
        // Note: By design slight logic mismatch with returntype comparisson.
        // That is to handle subs vs function, it does need this for parameters.
        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionError (argument) \n");
        MethodConversion |= MethodConversionError;
    }
    else if (Conversion == ConversionNarrowing)
    {
        DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionOneArgumentIsNarrowing\n");
        MethodConversion |= MethodConversionOneArgumentIsNarrowing;
    }
    else if (Conversion == ConversionWidening)
    {
        // Bug 73604. Generic Reference type needs to be treated specially.
        ConversionClass ClrConversion = ClassifyPredefinedCLRConversion(TargetType, DelegateType, ConversionSemantics::Default);

        if (ClrConversion == ConversionWidening &&
            //!DelegateType->IsInterface() &&
            TypeHelpers::IsReferenceType(DelegateType))
        {
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionOneArgumentIsClrWidening\n");
            MethodConversion |= MethodConversionOneArgumentIsClrWidening;
        }
        else
        {
            DBG_SWITCH_PRINTF(fDumpRelaxedDel, "    MethConv: MethodConversionOneArgumentIsVbOrBoxWidening\n");
            MethodConversion |= MethodConversionOneArgumentIsVbOrBoxWidening;
        }
    }
    // $
}
/*=======================================================================================
IsStubRequiredForMethodConversion

Tells wheter the MethodConversionClass will need a stub to run or not.
This does NOT tell whether it is a supported conversion, please check with IsSupportedMethodConversion
=======================================================================================*/
bool
Semantics::IsStubRequiredForMethodConversion
(
    MethodConversionClass Conversion
)
{

    /*
    MethodConversionOneArgumentIsClrWidening            = 0x00000002,
    MethodConversionReturnIsClrNarrowing                = 0x00000010,
    MethodConversionLateBoundCall                       = 0x00000400,
    MethodConversionExtensionMethod                     = 0x00004000*/

    return
        ((Conversion & MethodConversionOneArgumentIsNarrowing) != 0) ||
        ((Conversion & MethodConversionOneArgumentIsVbOrBoxWidening) != 0) ||
        ((Conversion & MethodConversionReturnIsWidening) != 0) ||
        ((Conversion & MethodConversionReturnIsIsVbOrBoxNarrowing) != 0) ||
        ((Conversion & MethodConversionReturnValueIsDropped) != 0) ||
        ((Conversion & MethodConversionAllArgumentsIgnored) != 0) ||
        ((Conversion & MethodConversionExcessOptionalArgumentsOnTarget) != 0);
}


/*=======================================================================================
IsSupportedMethodConversion

Tells wheter the MethodConversionClass (with or without a stub) is possible.
=======================================================================================*/
bool
Semantics::IsSupportedMethodConversion
(
    bool OptionStrictOn,
    MethodConversionClass Conversion,
    bool* WouldHaveSucceededWithOptionStrictOff, //= NULL
    bool *pRequiresNarrowingConversion, // = NULL
    bool ConversionIsForAddressOf // = false
)
{
    if (WouldHaveSucceededWithOptionStrictOff)
    {
        *WouldHaveSucceededWithOptionStrictOff = false;
    }

    if(pRequiresNarrowingConversion)
    {
        *pRequiresNarrowingConversion = false;
    }

    bool isAllowed = ((Conversion & MethodConversionError) == 0) &&
        ((Conversion & MethodConversionExcessParamsArgumentOnDelegate) == 0) &&
        ((Conversion & MethodConversionByRefByValMismatch) == 0) &&
        ((Conversion & MethodConversionNotAMethod) == 0);

    if (isAllowed)
    {
        bool IsNarrowing = IsNarrowingMethodConversion(Conversion, ConversionIsForAddressOf);

        if(pRequiresNarrowingConversion)
        {
            *pRequiresNarrowingConversion = IsNarrowing;
        }

        if (OptionStrictOn)
        {
            isAllowed = !IsNarrowing;

            if (WouldHaveSucceededWithOptionStrictOff)
            {
                *WouldHaveSucceededWithOptionStrictOff = true;
            }
        }
    }

    return isAllowed;
}

/*=======================================================================================
IsNarrowingMethodConversion

Tells wheter the MethodConversionClass is considered to be narrowing or not.
=======================================================================================*/
bool
Semantics::IsNarrowingMethodConversion
(
    MethodConversionClass Conversion,
    bool ConversionIsForAddressOf
)
{
    return ((Conversion & MethodConversionOneArgumentIsNarrowing) != 0) ||
           ((Conversion & MethodConversionReturnIsWidening) != 0) ||
           (ConversionIsForAddressOf && (Conversion & MethodConversionAllArgumentsIgnored) != 0);
}

/*=======================================================================================
IsProcAllowedAsDelegate

Properties are not allowed to be used as targets for delegates.
=======================================================================================*/
bool
Semantics::IsProcAllowedAsDelegate
(
    BCSYM_Proc *TargetMethod
)
{
    return TargetMethod->IsMethodDecl() ||
        TargetMethod->IsMethodImpl() ||
        TargetMethod->IsSyntheticMethod() ||
        TargetMethod->IsDllDeclare();
}

/*=======================================================================================
DetermineDelegateRelaxationLevel

Helper to determine the relaxaction level of a given conversion. This will be used by
overload resolution in case of conflict. This is to prevent applications that compiled in VB8
to fail in VB9 because there are more matches. And the same for flipping strict On to Off.
=======================================================================================*/
DelegateRelaxationLevel
Semantics::DetermineDelegateRelaxationLevel
(
    MethodConversionClass MethodConversion
)
{
    if (MethodConversion ==  MethodConversionIdentity)
    {
        return DelegateRelaxationLevelNone;
    }
    else if (IsSupportedMethodConversion(true /* Strict On */, MethodConversion))
    {
        if ((MethodConversion & MethodConversionReturnValueIsDropped) == 0 &&
            (MethodConversion & MethodConversionAllArgumentsIgnored) == 0)
        {
            return DelegateRelaxationLevelWidening;
        }
        else
        {
            return DelegateRelaxationLevelWideningDropReturnOrArgs;
        }
    }
    else if (IsSupportedMethodConversion(false /* Strict Off */, MethodConversion))
    {
        return DelegateRelaxationLevelNarrowing;
    }
    else
    {
        return DelegateRelaxationLevelInvalid;
    }
}

/*=======================================================================================
ClassifyTryCastConversion

This function classifies all intrinsic CLR conversions, such as inheritance,
implementation, and array covariance. But besides these, it will also allow
some possible conversions involving generic parameters that are specific
to trycast.
=======================================================================================*/
ConversionClass
Semantics::ClassifyTryCastConversion
(
    Type *TargetType,
    Type *SourceType,
    bool IgnoreProjectEquivalence,              // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType      // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
)
{
    ConversionClass ConversionClassification =
        ClassifyPredefinedCLRConversion(
            TargetType,
            SourceType,
            ConversionSemantics::Default,
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType);

    if (ConversionClassification != ConversionError)
    {
        return ConversionClassification;
    }

    return
        ClassifyTryCastForGenericParams(
            TargetType,
            SourceType,
            IgnoreProjectEquivalence,
            ProjectForTargetType,
            ProjectForSourceType);
}

/*=======================================================================================
ClassifyTryCastForGenericParams

This function classifies conversions involving generic type parameters that are
specific to trycast.
=======================================================================================*/
ConversionClass
Semantics::ClassifyTryCastForGenericParams
(
    Type *TargetType,
    Type *SourceType,
    bool IgnoreProjectEquivalence,              // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForTargetType,     // The project containing a component of type TargetType that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForSourceType      // The project containing a component of type SourceType that was considered
                                                // for a successful match ignoring project equivalence.
)
{
    if (TypeHelpers::IsArrayType(TargetType) && TypeHelpers::IsArrayType(SourceType))
    {
        Type *SourceElementType = TypeHelpers::GetElementType(SourceType->PArrayType());
        Type *TargetElementType = TypeHelpers::GetElementType(TargetType->PArrayType());

        if ((TypeHelpers::IsReferenceType(SourceElementType) && TypeHelpers::IsValueType(TargetElementType)) ||
            (TypeHelpers::IsValueType(SourceElementType) && TypeHelpers::IsReferenceType(TargetElementType)))
        {
            return ConversionError;
        }

        return
            ClassifyTryCastForGenericParams(
                TargetElementType,
                SourceElementType,
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType);
    }

    if (!TypeHelpers::IsGenericParameter(SourceType) &&
        !TypeHelpers::IsGenericParameter(TargetType))
    {
        return ConversionError;
    }

    // Notation:
    // T - non class constrained type parameter
    // TC - class constrained type parameter
    // CC - Class constraint - eg: CC1 is class constraint of TC1
    // C - class
    // NC - Notinheritable class
    //
    // A - Conversions between Type parameters:
    //
    // 1. T1 -> T2      Conversion possible because T1 and T2 could be any types and thus be potentially related
    //
    // 2. TC1 -> T2     Conversion possible because T2 could be any type and thus be potentially related to TC1
    //
    // 3. T1 -> TC2     Conversion possible because T1 could be any type and thus be potentially related to TC2
    //
    // 4. TC1 -> TC2    Conversion possible only when CC1 and CC2 are related through inheritance, else
    //                  TC1 and TC2 would always be guaranteed to be classes along 2 completely unrelated
    //                  inheritance hierarchies.
    //
    //
    // B - Conversions between Type parameter and a non-type parameter type:
    //
    // 1. T1 -> C2      Conversion possible because T1 could be any type and thus potentially related to C2
    //
    // 2. C1 -> T2      Conversion possible because T2 could be any type and thus potentially related to C1
    //
    // 3. TC1 -> C2     Conversion possible only when CC1 and C2 are related through inheritance, else
    //                  TC1 and C2 would always be guaranteed to be classes along unrelated inheritance
    //                  hierarchies.
    //
    // 4. C1 -> TC2     Conversion possible only when C1 and CC2 are related through inheritance, else
    //                  C1 and CC2 would always be guaranteed to be classes along unrelated inheritance
    //                  hierarchies.
    //
    // 5. NC1 -> TC2    Conversion possible only when one of NC1 or its bases satisfies constraints of TC2
    //                  because those are the only types related to NC1 that could ever be passed to TC2.
    //
    // 6. TC1 -> NC2    Conversion possible only when one of NC2 or its bases satisfies constraints of TC1
    //                  because those are the only types related to NC1 that could ever be passed to TC1.
    //
    //
    // Both A and B above are unified conceptually by treating C1 and C2 to be type parameters constrained
    // to class constraints C1 and C2 respectively.
    //
    // Note that type params with the value type constraint i.e "Structure" is treated as a param with a class
    // constraint of System.ValueType.
    //
    // Note that the reference type constraint does not affect the try cast conversions because it does not prevent
    // System.ValueType and System.Enum and so value types can still be related to type params with such constraints.
    //

    Type *Type1 =
        TypeHelpers::IsGenericParameter(SourceType) ?
            (SourceType->PGenericParam()->HasValueConstraint() ?
                GetFXSymbolProvider()->GetType(FX::ValueTypeType) :
                GetClassConstraint(SourceType->PGenericParam(),m_CompilerHost, &m_TreeStorage, false, false)) :  // don't ReturnArraysAs"System.Array" and don't ReturnValuesAs"System.ValueType"or"System.Enum"
            SourceType;

    Type *Type2 =
        TypeHelpers::IsGenericParameter(TargetType) ?
            (TargetType->PGenericParam()->HasValueConstraint() ?
                GetFXSymbolProvider()->GetType(FX::ValueTypeType) :
                GetClassConstraint(TargetType->PGenericParam(), m_CompilerHost, &m_TreeStorage, false, false)) : // don't ReturnArraysAs"System.Array" and don't ReturnValuesAs"System.ValueType"or"System.Enum"
            TargetType;

    // If the class constraint is NULL, then conversion is possible because the non-class constrained
    // type parameter can be any type related to the other type or type parameter.
    //
    // Handles cases: A.1, A.2, A.3, B.1 and B.2
    //
    if (!Type1 ||
        !Type2)
    {
        return ConversionNarrowing;
    }


    // partially handles cases: A.4, B.3, B.4, B.5 and B.6
    //
    ConversionClass Conversion =
        ::ClassifyPredefinedCLRConversion(
                Type2,
                Type1,
                m_SymbolCreator,
                m_CompilerHost,
                ConversionSemantics::Default,
                0, // calling from outside so RecursionCount=0
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType,
                NULL
                );

    if (Conversion == ConversionIdentity ||
        Conversion == ConversionWidening)
    {
        // Since NotInheritable classes cannot be inherited, more conversion possibilites
        // could be ruled out at compiler time if the NotInheritable class or any of its
        // base can never be used as a type argument for this type parameter during type
        // instantiation.
        //
        // partially handles cases: B.5 and B.6
        //
        if (!(Type1->IsNamedRoot() && IsClassOnly(Type1->PNamedRoot())) ||
            Type1->PClass()->IsNotInheritable())
        {
            if (TypeHelpers::IsGenericParameter(TargetType) &&
                !CanClassOrBasesSatisfyConstraints(
                        Type1,
                        TargetType->PGenericParam(),
                        IgnoreProjectEquivalence,
                        ProjectForSourceType,
                        ProjectForTargetType))
            {
                return ConversionError;
            }
        }

        return ConversionNarrowing;
    }

    // partially handles cases: A.4, B.3 and B.4
    //
    Conversion =
        ::ClassifyPredefinedCLRConversion(
                Type1,
                Type2,
                m_SymbolCreator,
                m_CompilerHost,
                ConversionSemantics::Default,
                0, // RecursionCount=0 because we're calling from outside
                IgnoreProjectEquivalence,
                ProjectForSourceType,
                ProjectForTargetType,
                NULL
                );

    if (Conversion == ConversionIdentity ||
        Conversion == ConversionWidening)
    {
        // Since NotInheritable classes cannot be inherited, more conversion possibilites
        // could be rules out at compiler time if the NotInheritable class or any of its
        // base can never be used as a type argument for this type parameter during type
        // instantiation.
        //
        // partially handles cases: B.5 and B.6
        //
        if (!(Type2->IsNamedRoot() && IsClassOnly(Type2->PNamedRoot())) ||
            Type2->PClass()->IsNotInheritable())
        {
            if (TypeHelpers::IsGenericParameter(SourceType) &&
                !CanClassOrBasesSatisfyConstraints(
                        Type2,
                        SourceType->PGenericParam(),
                        IgnoreProjectEquivalence,
                        ProjectForTargetType,
                        ProjectForSourceType))
            {
                return ConversionError;
            }
        }

        return ConversionNarrowing;
    }

    // No conversion ever possible
    //
    return ConversionError;
}

/*=======================================================================================
CanClassOrBasesSatisfyConstraints

Given a class and a generic param, this function determines if the given class
or any of it bases satisfy all the constraints for this given generic param.
=======================================================================================*/
bool
Semantics::CanClassOrBasesSatisfyConstraints
(
    Type *Class,
    GenericParameter *GenericParam,
    bool IgnoreProjectEquivalence,              // Consider types as equivalent ignoring their projects' equivalence.
    CompilerProject **ProjectForClass,          // The project containing a component of type "Class" that was considered
                                                // for a successful match ignoring project equivalence.
    CompilerProject **ProjectForGenericParam    // The project containing a component of type "GenericParam" that was considered
                                                // for a successful match ignoring project equivalence.
)
{
    VSASSERT(Class->IsClass(), "Non-class unexpected!!!");

    // 

    for (Type *CurrentClass = Class;
         CurrentClass;
         CurrentClass =
            (CurrentClass->IsArrayType() ? GetFXSymbolProvider()->GetType(FX::ArrayType) : GetBaseClass(CurrentClass)))
    {
        if (Bindable::DoesTypeSatisfyGenericConstraints(
                CurrentClass,
                GenericParam,
                NULL,
                m_CompilerHost,
                m_Compiler,
                &m_SymbolCreator,
                IgnoreProjectEquivalence,
                ProjectForClass,
                ProjectForGenericParam))
        {
            return true;
        }
    }

    return false;
}

/*=======================================================================================
Encompasses

Definition: LARGER is said to encompass SMALLER if SMALLER widens to or is LARGER.
=======================================================================================*/
bool
Semantics::Encompasses
(
    Type *Larger,
    Type *Smaller,
    bool considerConversionsOnNullableBool,
    bool IgnoreIdentity
)
{
    // 



    ConversionClass Result = ClassifyPredefinedConversion(Larger, Smaller, false, NULL, NULL, false, considerConversionsOnNullableBool);

    return Result == ConversionWidening ||
           (!IgnoreIdentity && Result == ConversionIdentity);
}

/*=======================================================================================
NotEncompasses

Definition: LARGER is said to not encompass SMALLER if SMALLER narrows to or is LARGER.
=======================================================================================*/
bool
Semantics::NotEncompasses
(
    Type *Larger,
    Type *Smaller,
    bool considerConversionsOnNullableBool
)
{
    ConversionClass Result = ClassifyPredefinedConversion(Larger, Smaller, false, NULL, NULL, false, considerConversionsOnNullableBool);

    return Result == ConversionNarrowing || Result == ConversionIdentity;
}


/*=======================================================================================
MostEncompassing

Given a set TYPES, determine the set RESULTS of most encompassing types. An element
CANDIDATE of TYPES is said to be most encompassing, and thus a member of RESULTS, if no
other element of TYPES encompasses CANDIDATE.
=======================================================================================*/
Type *
Semantics::MostEncompassing
(
    TypeSet &Types,
    TypeSet &Results
)
{
    Type *LastResult = NULL;

    TypeSetIter Contestants(&Types);
    while (Type *Candidate = Contestants.Next())
    {
        bool IsEncompassed = false;

        TypeSetIter Challengers(&Types);
        while (Type *Challenger = Challengers.Next())
        {
            if (Candidate != Challenger &&  // redundant, but short circuit
                Encompasses(Challenger, Candidate, true, true /* Ignore identity */))
            {
                IsEncompassed = true;
                break;
            }
        }

        if (!IsEncompassed)
        {
            Results.Add(Candidate);
            LastResult = Candidate;
        }
    }

    // The only way all types can be encompassed is if W<-->W predefined conversions exist.
    // But W<-->W predefined conversions don't exist, so the result set cannot be empty.
    VSASSERT(Results.Count() > 0, "suprisingly, everyone is encompassed by someone");
    return LastResult;
}

/*=======================================================================================
MostEncompassed

Given a set TYPES, determine the set RESULTS of most encompassed types. An element
CANDIDATE of TYPES is said to be most encompassed, and thus a member of RESULTS, if
CANDIDATE encompasses no other element of TYPES.
=======================================================================================*/
Type *
Semantics::MostEncompassed
(
    TypeSet &Types,
    TypeSet &Results
)
{
    Type *LastResult = NULL;

    TypeSetIter Contestants(&Types);
    while (Type *Candidate = Contestants.Next())
    {
        bool IsEncompassing = false;

        TypeSetIter Challengers(&Types);
        while (Type *Challenger = Challengers.Next())
        {
            if (Candidate != Challenger &&  // redundant, but short circuit
                Encompasses(Candidate, Challenger, true /*consider conversions defined in Boolean? */, true /* Ignore identity */))
            {
                IsEncompassing = true;
                break;
            }
        }

        if (!IsEncompassing)
        {
            Results.Add(Candidate);
            LastResult = Candidate;
        }
    }

    // The only way all types can encompass is if W<-->W predefined conversions exist.
    // But W<-->W predefined conversions don't exist, so the result set cannot be empty.
    VSASSERT(Results.Count() > 0, "suprisingly, everyone is encompassing someone");
    return LastResult;
}

/*=======================================================================================
FindBestMatch

Given a set of conversion operators which convert from INPUT to RESULT, return the set
of operators for which INPUT is SOURCE and RESULT is TARGET.
=======================================================================================*/
void
Semantics::FindBestMatch
(
    Type *SourceType,
    Type *TargetType,
    CDoubleList<OperatorInfo> &SearchList,
    CDoubleList<OperatorInfo> &ResultList,
    bool &GenericOperatorsExistInResultList
)
{
    CDoubleListForwardIter<OperatorInfo> Iter(&SearchList);

    while (OperatorInfo *Current = Iter.Next())
    {
        Type *InputType =
            TypeInGenericContext(Current->Symbol->GetFirstParam()->GetType(), Current->GenericBindingContext);

        Type *ResultType =
            TypeInGenericContext(Current->Symbol->GetType(), Current->GenericBindingContext);

        if (TypeHelpers::EquivalentTypes(InputType, SourceType) &&
            TypeHelpers::EquivalentTypes(ResultType, TargetType))
        {
            // 


            SearchList.Remove(Current);
            InsertInOperatorListIfLessGenericThanExisting(Current, ResultList, GenericOperatorsExistInResultList);
        }
    }
}

void
Semantics::InsertInOperatorListIfLessGenericThanExisting
(
    OperatorInfo *Operator,
    CDoubleList<OperatorInfo> &OperatorList,
    bool &GenericMembersExistInList
)
{
    if (Operator->GenericBindingContext != NULL)
    {
        GenericMembersExistInList = true;
    }

    if (GenericMembersExistInList)
    {
        CDoubleListForwardIter<OperatorInfo> Iter(&OperatorList);
        while (OperatorInfo *Existing = Iter.Next())
        {
            Procedure *LeastGeneric =
                LeastGenericProcedure(Existing->Symbol, Existing->GenericBindingContext, Operator->Symbol, Operator->GenericBindingContext, true, false, false);

            if (LeastGeneric == Existing->Symbol)
            {
                // An existing one is less generic than the current operator being
                // considered, so skip adding the current operator to the operator
                // list.
                //
                return;
            }
            else if (LeastGeneric != NULL)
            {
                // The current operator is less generic than an existing operator,
                // so remove the existing operator from the list and continue to
                // check if any other exisiting operator can be removed from the
                // result set.
                //
                OperatorList.Remove(Existing);
            }
        }
    }

    OperatorList.InsertFirst(Operator);
}


bool Semantics::ConversionMaybeLifted
(
    Type * SourceType,
    Type * TargetType,
    Type * InputType,
    Type * ResultType
)
{
    return
        TypeHelpers::IsNullableType(SourceType, m_CompilerHost) &&
        TypeHelpers::IsNullableType(TargetType, m_CompilerHost) &&
        !TypeHelpers::IsNullableType(InputType, m_CompilerHost) &&
        !TypeHelpers::IsNullableType(ResultType, m_CompilerHost);
}

/*=======================================================================================
ResolveConversion

This function resolves which user-defined conversion operator contained in the input set
can be used to perform the conversion from source type S to target type T.

The algorithm defies succinct explaination, but roughly:

Conversions of the form S-->T use only one user-defined conversion at a time, i.e.,
user-defined conversions are not chained together.  It may be necessary to convert to and
from intermediate types using predefined conversions to match the signature of the
user-defined conversion exactly, so the conversion "path" is comprised of at most three
parts:

    1) [ predefined conversion  S-->Sx ]
    2) User-defined conversion Sx-->Tx
    3) [ predefined conversion Tx-->T  ]

    Where Sx is the intermediate source type
      and Tx is the intermediate target type

    Steps 1 and 3 are optional given S == Sx or Tx == T.

Much of the algorithm below concerns itself with finding Sx and Tx.  The rules are:

    - If a conversion operator in the set converts from S, then Sx is S.
    - If a conversion operator in the set converts to T, then Tx is T.
    - Otherwise Sx and Tx are the "closest" types to S and T.  If multiple types are
      equally close, the conversion is ambiguous.

Each operator presents a possibility for Sx (the parameter type of the operator).  Given
these choices, the "closest" type to S is the smallest (most encompassed) type that S
widens to.  If S widens to none of the possible types, then the "closest" type to S is
the largest (most encompassing) type that widens to S.  In this way, the algorithm
always prefers widening from S over narrowing from S.

Similarily, each operator presents a possibility for Tx (the return type of the operator).
Given these choices, the "closest" type to T is the largest (most encompassing) type that
widens to T.  If none of the possible types widen to T, then the "closest" type to T is
the smallest (most encompassed) type that T widens to.  In this way, the algorithm
always prefers widening to T over narrowing to T.

Upon deciding Sx and Tx, if one operator's operands exactly match Sx and Tx, then that
operator is chosen.  If no operators match, or if multiple operators match, the conversion
is impossible.

Refer to the language specification as it covers all details of the algorithm.
=======================================================================================*/
ConversionClass
Semantics::ResolveConversion
(
    Type *TargetType,
    Type *SourceType,
    CDoubleList<OperatorInfo> &OperatorSet,
    CDoubleList<OperatorInfo> &OperatorChoices,
    bool WideningOnly,
    NorlsAllocator &Scratch,
    bool &ResolutionIsAmbiguous,
    bool considerConversionsOnNullableBool,
    bool * pConversionRequiresUnliftedAccessToNullableValue
)
{
    // 





    ResolutionIsAmbiguous = false;

    Type *MostSpecificSourceType = NULL;
    Type *MostSpecificTargetType = NULL;
    CDoubleList<OperatorInfo> Candidates;
    bool GenericOperatorChoicesFound = false;

    // This loop is not currently required since the operator choices list is passed in empty, but
    // in case in the future if the implementation of the callers changed and if it came from a
    // previous iteration considering only widening, etc., then there not be any problem here.
    //
    CDoubleListForwardIter<OperatorInfo> OperatorChoicesIter(&OperatorSet);
    while (OperatorInfo *Existing = OperatorChoicesIter.Next())
    {
        if (Existing->GenericBindingContext)
        {
            GenericOperatorChoicesFound = true;
            break;
        }
    }

    // 






    TypeSet SourceBases(OperatorSet.NumberOfEntries(), Scratch);
    TypeSet TargetBases(OperatorSet.NumberOfEntries(), Scratch);
    TypeSet SourceDeriveds(OperatorSet.NumberOfEntries(), Scratch);
    TypeSet TargetDeriveds(OperatorSet.NumberOfEntries(), Scratch);

    // To minimize the number of calls to Encompasses, we categorize conversions
    // into three flavors:
    //
    //    1) Base of Source to Derived of Target (only flavor that can be completely widening)
    //    2) Base of Source to Base of Target
    //    3) Derived of Source to Base of Target
    //
    // For each flavor, we place the input and result type into the corresponding
    // type set. Then we calculate most encompassing/encompassed using the type sets.

    CDoubleListForwardIter<OperatorInfo> Iter(&OperatorSet);
    while (OperatorInfo *Current = Iter.Next())
    {
        // Performance trick: the operators are grouped by widening and then narrowing
        // conversions. If we are iterating over just widening conversions, we are done
        // once we find a narrowing conversion.
        if (WideningOnly && Current->Symbol->GetOperator() == OperatorNarrow) break;

        Type *InputType = TypeInGenericContext(Current->Symbol->GetFirstParam()->GetType(), Current->GenericBindingContext);
        Type *ResultType = TypeInGenericContext(Current->Symbol->GetType(), Current->GenericBindingContext);

        // Ignore user defined conversions between types that already have intrinsic conversions.
        // This could happen for generics after generic param substituion.
        if (IsGenericOrHasGenericParent(Current->Symbol) &&
            ClassifyPredefinedConversion(ResultType, InputType,false, NULL, NULL, false, considerConversionsOnNullableBool) != ConversionError)
        {
            continue;
        }

        if (TypeHelpers::EquivalentTypes(InputType, SourceType) &&
            TypeHelpers::EquivalentTypes(ResultType, TargetType))
        {
            // 


            OperatorSet.Remove(Current);
            InsertInOperatorListIfLessGenericThanExisting(Current, OperatorChoices, GenericOperatorChoicesFound);

            // 


        }
        else if (OperatorChoices.NumberOfEntries() == 0)
        {
            // Check SourceBase->TargetDerived flavor.
            if
            (
                Encompasses
                (
                    InputType,
                    SourceType,
                    considerConversionsOnNullableBool
                ) &&
                Encompasses
                (
                    TargetType,
                    ResultType,
                    considerConversionsOnNullableBool
                )
            )
            {
                OperatorSet.Remove(Current);
                Candidates.InsertFirst(Current);

                if (TypeHelpers::EquivalentTypes(InputType, SourceType))
                {
                    MostSpecificSourceType = InputType;
                }
                else
                {
                    // 

                    SourceBases.Add(InputType);
                }

                if (TypeHelpers::EquivalentTypes(ResultType, TargetType))
                {
                    MostSpecificTargetType = ResultType;
                }
                else
                {
                    TargetDeriveds.Add(ResultType);
                }
            }
            // Check SourceBase->TargetBase flavor.
            else if
            (
                !WideningOnly &&
                Encompasses
                (
                    InputType,
                    SourceType,
                    considerConversionsOnNullableBool
                ) &&
                NotEncompasses
                (
                    TargetType,
                    ResultType,
                    considerConversionsOnNullableBool
                )
            )
            {
                OperatorSet.Remove(Current);
                Candidates.InsertFirst(Current);

                if (TypeHelpers::EquivalentTypes(InputType, SourceType))
                {
                    MostSpecificSourceType = InputType;
                }
                else
                {
                    // 

                    SourceBases.Add(InputType);
                }

                if (TypeHelpers::EquivalentTypes(ResultType, TargetType))
                {
                    MostSpecificTargetType = ResultType;
                }
                else
                {
                    TargetBases.Add(ResultType);
                }
            }
            // Check SourceDerived->TargetBase flavor.
            else if
            (
                !WideningOnly &&
                NotEncompasses(InputType, SourceType, considerConversionsOnNullableBool) &&
                NotEncompasses(TargetType, ResultType, considerConversionsOnNullableBool)
            )
            {
                OperatorSet.Remove(Current);
                Candidates.InsertFirst(Current);

                if (TypeHelpers::EquivalentTypes(InputType, SourceType))
                {
                    MostSpecificSourceType = InputType;
                }
                else
                {
                    // 

                    SourceDeriveds.Add(InputType);
                }

                if (TypeHelpers::EquivalentTypes(ResultType, TargetType))
                {
                    MostSpecificTargetType = ResultType;
                }
                else
                {
                    TargetBases.Add(ResultType);
                }
            }
        }
    }  // while

    // Now attempt to find the most specific types Sx and Tx by analyzing the type sets
    // we built up in the code above.

    if (OperatorChoices.NumberOfEntries() == 0 && Candidates.NumberOfEntries() > 0)
    {
        if (MostSpecificSourceType == NULL)
        {
            // 

            if (SourceBases.Count() > 0)
            {
                // 

                TypeSet Results(SourceBases.Count(), Scratch);
                MostSpecificSourceType = MostEncompassed(SourceBases, Results);

                if (Results.Count() != 1)
                {
                    // 
                    ResolutionIsAmbiguous = true;
                    return ConversionError;
                }
            }
            else
            {
                VSASSERT(!WideningOnly && SourceDeriveds.Count() != 0, "how can this be?");
                TypeSet Results(SourceDeriveds.Count(), Scratch);
                MostSpecificSourceType = MostEncompassing(SourceDeriveds, Results);

                if (Results.Count() != 1)
                {
                    // 
                    ResolutionIsAmbiguous = true;
                    return ConversionError;
                }
            }
        }

        if (MostSpecificTargetType == NULL)
        {
            if (TargetDeriveds.Count() > 0)
            {
                TypeSet Results(TargetDeriveds.Count(), Scratch);
                MostSpecificTargetType = MostEncompassing(TargetDeriveds, Results);

                if (Results.Count() != 1)
                {
                    // 
                    ResolutionIsAmbiguous = true;
                    return ConversionError;
                }
            }
            else
            {
                VSASSERT(!WideningOnly && TargetBases.Count() != 0, "how can this be?");

                TypeSet Results(TargetBases.Count(), Scratch);
                MostSpecificTargetType = MostEncompassed(TargetBases, Results);

                if (Results.Count() != 1)
                {
                    // 
                    ResolutionIsAmbiguous = true;
                    return ConversionError;
                }
            }
        }

        FindBestMatch(
            MostSpecificSourceType,
            MostSpecificTargetType,
            Candidates,
            OperatorChoices,
            GenericOperatorChoicesFound);

        if (OperatorChoices.NumberOfEntries() != 1)
        {
            // 
            ResolutionIsAmbiguous = true;
            return ConversionError;
        }
    }

    if (OperatorChoices.NumberOfEntries() == 1)
    {
        return WideningOnly ? ConversionWidening : ConversionNarrowing;
    }
    else if (OperatorChoices.NumberOfEntries() == 0)
    {
        // No conversion possible.
        return ConversionError;
    }

    // 
    ResolutionIsAmbiguous = true;
    return ConversionError;
}

/*=======================================================================================
ClassifyUserDefinedConversion

Classifies the conversion from Source to Target using user-defined conversion operators.
If such a conversion exists, it will be supplied as an out parameter.

The result is a widening conversion from Source to Target if such a conversion exists.
Otherwise the result is a narrowing conversion if such a conversion exists.  Otherwise
no conversion is possible.  We perform this two pass process because the conversion
"path" is not affected by the user implicitly or explicitly specifying the conversion.

In other words, a safe (widening) conversion is always taken regardless of whether
Option Strict is on or off.
=======================================================================================*/
ConversionClass
Semantics::ClassifyUserDefinedConversion
(
    Type *TargetType,
    Type *SourceType,
    Procedure *&OperatorMethod,
    GenericBinding *&OperatorMethodGenericContext,
    bool *OperatorMethodIsLifted,
    bool considerConversionsOnNullableBool,
    bool * pConversionRequiresUnliftedAccessToNullableValue
    // 
)
{
    if (pConversionRequiresUnliftedAccessToNullableValue)
    {
        *pConversionRequiresUnliftedAccessToNullableValue = false;
    }

    // It would be nice if this function had two modes - resolve and then give error messages.  Maybe a wrapper which turns off error reporting?

    // 

    bool targetIsNullable = TypeHelpers::IsNullableType(TargetType, m_CompilerHost);
    bool sourceIsNullable = TypeHelpers::IsNullableType(SourceType, m_CompilerHost);

    if (!considerConversionsOnNullableBool && (targetIsNullable || sourceIsNullable) && !(targetIsNullable && sourceIsNullable))
    {
        Type * sourceElementType = TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost);
        Type * targetElementType = TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost);

        if
        (
            BCSYM::AreTypesEqual
            (
                targetElementType,
                sourceElementType
            ) &&
            BCSYM::AreTypesEqual
            (
                targetElementType,
                GetFXSymbolProvider()->GetBooleanType()
            )
        )
        {
            return ConversionError;
        }
    }

    bool ResolutionIsAmbiguous = false;

    if (OperatorMethodIsLifted)
    {
        *OperatorMethodIsLifted = false;
    }

    VSASSERT(ClassifyPredefinedConversion(TargetType, SourceType, false, NULL, NULL, false, considerConversionsOnNullableBool) == ConversionError,
             "predefined conversion is possible, so why try user-defined?");

    NorlsAllocator Scratch(NORLSLOC);
    CDoubleList<OperatorInfo> OperatorSet;
    OperatorMethod = NULL;
    OperatorMethodGenericContext = NULL;

    CollectConversionOperators(
        TargetType,
        SourceType,
        OperatorSet,
        Scratch);

    if (OperatorSet.NumberOfEntries() == 0)
    {
        // No conversion operators, so no conversion is possible.
        return ConversionError;
    }

    CDoubleList<OperatorInfo> OperatorChoices;

    // First pass: attempt to find a widening conversion.
    ResolveConversion
    (
        TargetType,
        SourceType,
        OperatorSet,
        OperatorChoices,
        true,  // widening only
        Scratch,
        ResolutionIsAmbiguous,
        considerConversionsOnNullableBool
    );

    if (OperatorChoices.NumberOfEntries() == 1)
    {
        OperatorMethod = OperatorChoices.GetFirst()->Symbol->GetOperatorMethod();
        OperatorMethodGenericContext = OperatorChoices.GetFirst()->GenericBindingContext;
        // The result from the first pass is necessarily widening.
        return ConversionWidening;
    }
    else if (OperatorChoices.NumberOfEntries() == 0 && !ResolutionIsAmbiguous)
    {
        if (OperatorSet.NumberOfEntries() == 0)
        {
            // 


            goto CheckLiftedNullable;   // try to lift for nullables before returning ConversionError
        }

        // Second pass: if the first pass failed, attempt to find a conversion
        // considering BOTH widening and narrowing.
        ResolveConversion
        (
            TargetType,
            SourceType,
            OperatorSet,
            OperatorChoices,
            false,  // widening and narrowing
            Scratch,
            ResolutionIsAmbiguous,
            considerConversionsOnNullableBool,
            pConversionRequiresUnliftedAccessToNullableValue
        );

        if (OperatorChoices.NumberOfEntries() == 1)
        {
            OperatorMethod = OperatorChoices.GetFirst()->Symbol->GetOperatorMethod();
            OperatorMethodGenericContext = OperatorChoices.GetFirst()->GenericBindingContext;

            targetIsNullable = TypeHelpers::IsNullableType(TargetType, m_CompilerHost);
            sourceIsNullable = TypeHelpers::IsNullableType(SourceType, m_CompilerHost);

            bool operatorSourceIsNullable = TypeHelpers::IsNullableType(TypeInGenericContext(OperatorMethod->GetFirstParam()->GetType(), OperatorMethodGenericContext), m_CompilerHost);
            bool operatorTargetIsNullable = TypeHelpers::IsNullableType(TypeInGenericContext(OperatorMethod->GetRawType(), OperatorMethodGenericContext), m_CompilerHost);

            if
            (
                pConversionRequiresUnliftedAccessToNullableValue &&
                !(
                    !operatorSourceIsNullable &&
                    !operatorTargetIsNullable &&
                    sourceIsNullable &&
                    targetIsNullable
                ) &&
                (
                    (sourceIsNullable && !operatorSourceIsNullable) ||
                    (operatorTargetIsNullable && ! targetIsNullable)
                )
            )
            {
                *pConversionRequiresUnliftedAccessToNullableValue = true;
            }

            // The result from the second pass is necessarily narrowing.
            return ConversionNarrowing;
        }
        else if (OperatorChoices.NumberOfEntries() == 0)
        {
            // No conversion possible.
            goto CheckLiftedNullable; // try to lift for nullables before returning ConversionError
        }
    }

    // Conversion is ambiguous.
    // 

    return ConversionError;

CheckLiftedNullable:
    // Warning: there is the potential case this code could never be reached when there is a narrowing user defined conversion from S to T.
    // It is the case S?->S-->T->T?. S? narrows to S, which narrows to T, which widens to T. Right now this case doesn't clasify as
    // a narrowing conversion, it is not a valid conversion scenario. It is for a reason: for the reference types case the corresponding
    // chain looks like BaseSource->DerivedSource-->DerivedTarget->BaseTarget. It is an imposible chain, because the user defined conversion
    // must be defined either BaseSource or BaseTarget. The user defined op is never found because it is searched up on inheritance
    // relation, not on derived. The existing algorithm is based on the assumption encompassing needs at least one widdening via inheritance.
    // And,  this is the only reason the lifting works.
    // If scenario above is ever going to change into a legal narrowing, then the regular narrowing would take over lifting.
    // The fix would be to go and disable narrowing for nullables.
    if ( OperatorMethodIsLifted && TypeHelpers::IsNullableType(SourceType, m_CompilerHost) && TypeHelpers::IsNullableType(TargetType, m_CompilerHost))
    {
        Type * TargetElementType = TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost);
        Type * SourceElementType = TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost);

        *OperatorMethodIsLifted = true;
        ConversionClass ret = ClassifyUserDefinedConversion
        (
            TargetElementType,
            SourceElementType,
            OperatorMethod,
            OperatorMethodGenericContext,
            NULL,
            true
        ); // no way to have double lifting, '??' is illegal.

        if
        (
            considerConversionsOnNullableBool ||
            (
                !BCSYM::AreTypesEqual
                (
                    TargetElementType,
                    GetFXSymbolProvider()->GetBooleanType()
                ) &&
                !BCSYM::AreTypesEqual
                (
                    SourceElementType,
                    GetFXSymbolProvider()->GetBooleanType()
                )
            ) ||
            (
                OperatorMethod &&
                OperatorMethod->GetType() &&
                ! TypeHelpers::IsNullableType(OperatorMethod->GetType(), m_CompilerHost) &&
                OperatorMethod->GetFirstParam()  &&
                OperatorMethod->GetFirstParam()->GetType() &&
                !TypeHelpers::IsNullableType(OperatorMethod->GetFirstParam()->GetType(), m_CompilerHost)
            )
        )
        {
            return ret;
        }
        else
        {
            OperatorMethod = NULL;
            OperatorMethodGenericContext = NULL;
            return ConversionError;
        }
    }
    return ConversionError;
}

void 
Semantics::TypeInferenceCollection::AddType(Type *pSym, ConversionRequiredEnum ConversionRequired, _In_opt_ ILTree::Expression *SourceExpression)
{
    ThrowIfNull(pSym);
    if (pSym->IsVoidType())
    {
        VSFAIL("Please do not put Void types into the dominant type algorithm. That doesn't make sense.");
        return;
    }

    DominantTypeData* typeData = new(*m_allocator) DominantTypeData(m_allocatorWrapper);
    typeData->ResultType = pSym;
    typeData->InferenceRestrictions = ConversionRequired;
    typeData->SourceExpressions.Add(SourceExpression);

    // We will add only unique types into this collection. Otherwise, say, if we added two types
    // "Integer" and "Integer", the dominant type routine would say that the two candidates are
    // ambiguous! This of course means we'll have to combine the restrictions of the two hints types.
    bool foundInList = false;
    DominantTypeDataListIterator iterForDuplicates(&m_dominantTypeDataList);
    while (iterForDuplicates.MoveNext())
    {
        DominantTypeData* competitor = (DominantTypeData*)iterForDuplicates.Current();
        if (competitor->ResultType && BCSYM::AreTypesEqual(pSym, competitor->ResultType))
        {
            competitor->InferenceRestrictions = CombineConversionRequirements(
                                                    competitor->InferenceRestrictions,
                                                    typeData->InferenceRestrictions);
            competitor->SourceExpressions.Add(SourceExpression);
            VSASSERT(!foundInList, "List is supposed to be unique: how can we already find two of the same type in this list.");
            foundInList = true;
        }
    }
    if (!foundInList)
    {
        m_dominantTypeDataList.Add(typeData);
    }
}



DominantTypeDataList*
Semantics::TypeInferenceCollection::GetTypeDataList()
{
    return &m_dominantTypeDataList;
}



Semantics::TypeInferenceCollection::HintSatisfactionEnum
Semantics::TypeInferenceCollection::CheckHintSatisfaction
(
    DominantTypeData* candidateData,
    DominantTypeData* hintData,
    ConversionRequiredEnum hintRestrictions
)
{
    // This algorithm is used in type inference (where we examine whether a candidate works against a set of hints)
    // It is also used in inferring a dominant type out of a collection e.g. for array literals:
    // in that case, "candidate" is the one we're currently examining against all the others, and "hint" is
    // one of the others with the "TypeInferenceArgumentAnyConversion" flag.

    VSASSERT(ConversionRequired::Count==8, "If you've updated the type argument inference restrictions, then please also update CheckHintSatisfaction()");

    // Microsoft 2008.June.27: in Orcas we kept a separate flag "ByRef" that could be used in conjunction with the
    // other ConversionRequiredEnum. What Orcas required was that hint->Restriction be
    // satisfied when seeing if the argument could be copied into the parameter, and candidate->Restriction
    // to see if the parameter could be copied back into the argument. It was clearly wrong because
    // the candidate restrictions had nothing to do whether whether the candidate satisfied the hint.
    // However, I couldn't find any bugs that arose from it; every possibility ended up requiring
    // identity for unrelated language reasons.
    //
    // In any case it's immaterial. I changed the code so that ByRef is now incorporated into the InferenceRestrictions
    // earlier and in a more controlled manner. "ByRef T" produces TypeArgumentInterenceVBConversionAndReverse
    // to ensure that the argument can be copied into parameter and copied back. And "ByRef T()" 
    // or "ByRef I(Of T)" produce ConversionRequired::Identity to ensure that T is identical
    // to the hint.

    Type *candidate = candidateData->ResultType;
    Type *hint = hintData->ResultType;

    ConversionClass conversion;

    if (candidate == NULL)
    {
        // This covers the case when the "candidate" is dominant-typeless array literal or Nothing or a lambda. Obviously
        // it's not a real candidate. So we skip over it.
        conversion = ConversionError;
    }

    else if (hintRestrictions == ConversionRequired::None)
    {
        conversion = ConversionIdentity;
    }

    else if (hintRestrictions == ConversionRequired::Identity)
    {
        conversion = m_pSemantics->ClassifyPredefinedCLRConversion(candidate, hint, ConversionSemantics::Default);
        if (conversion != ConversionIdentity)
        {
            conversion = ConversionError;
        }
    }

    else if (hintRestrictions == ConversionRequired::Any)
    {
        // three dummy arguments needed as out parameters for ClassifyConversion.
        Procedure * OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool OperatorMethodIsLifted;

        conversion = m_pSemantics->ClassifyConversion(
            candidate,  // copy TO the candidate for the parameter type T
            hint,       // FROM the hint (i.e. argument)
            OperatorMethod,
            OperatorMethodGenericContext,
            OperatorMethodIsLifted);
    }

    else if (hintRestrictions == ConversionRequired::AnyReverse)
    {
        // three dummy arguments needed as out parameters for ClassifyConversion.
        Procedure * OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool OperatorMethodIsLifted;

        conversion = m_pSemantics->ClassifyConversion(
            hint,       // copyback TO the hint (i.e. argument)
            candidate,  // FROM the candidate for parameter type T
            OperatorMethod,
            OperatorMethodGenericContext,
            OperatorMethodIsLifted);
    }

    else if (hintRestrictions == ConversionRequired::AnyAndReverse)
    {
        // three dummy arguments needed as out parameters for ClassifyConversion.
        Procedure * OperatorMethod = NULL;
        GenericBinding *OperatorMethodGenericContext = NULL;
        bool OperatorMethodIsLifted;

        // forwards copy of argument into parameter
        ConversionClass inConversion = m_pSemantics->ClassifyConversion(
            candidate,  // copy TO the candidate for the parameter type T
            hint,       // FROM the hint (i.e. argument)
            OperatorMethod,
            OperatorMethodGenericContext,
            OperatorMethodIsLifted);

        // back copy from the parameter back into the argument
        ConversionClass outConversion = conversion = m_pSemantics->ClassifyConversion(
            hint,       // copy back TO hint (i.e. argument)
            candidate,  // FROM the candidate parameter type T
            OperatorMethod,
            OperatorMethodGenericContext,
            OperatorMethodIsLifted);

        // Pick the lowest for our classification of identity/widening/narrowing/error.
        if (inConversion == ConversionError || outConversion == ConversionError)
        {
            conversion = ConversionError;
        }
        else if (inConversion == ConversionNarrowing || outConversion == ConversionNarrowing)
        {
            conversion = ConversionNarrowing;
        }
        else if (inConversion == ConversionWidening || outConversion == ConversionWidening)
        {
            conversion = ConversionWidening;
        }
        else
        {
            VSASSERT(inConversion == ConversionIdentity && outConversion == ConversionIdentity, "Can only be identity left");
            conversion = ConversionIdentity;
        }
    }

    else if (hintRestrictions == ConversionRequired::ArrayElement)
    {
        conversion = ClassifyCLRConversionForArrayElementTypes(
            candidate,
            hint,
            *m_pSemantics->GetSymbols(),
            m_pSemantics->GetCompilerHost(),
            ConversionSemantics::Default,
            0, // RecursionCount=0 because we're calling it from outside
            false, //IgnoreProjectEquivalence,
            NULL, //ProjectForTargetType,
            NULL); //ProjectForSource
    }

    else if (hintRestrictions == ConversionRequired::Reference)
    {
        conversion = m_pSemantics->ClassifyPredefinedCLRConversion(
            candidate,
            hint,
            ConversionSemantics::ReferenceConversions);
        
        // Dev10#595234: to preserve backwards-compatability with Orcas, a narrowing
        // counts as type inference failure if it happens inside a generic type parameter.
        // e.g. type inference will never infer T:Object for parameter IComparable(Of T)
        // when given argument IComparable(Of String), since it String->Object is narrowing,
        // and hence the candidate T:Object is deemed an error candidate.
        conversion = (conversion == ConversionNarrowing) ? ConversionError : conversion;
    }

    else if (hintRestrictions == ConversionRequired::ReverseReference)
    {
        conversion = m_pSemantics->ClassifyPredefinedCLRConversion(
            hint, // in reverse
            candidate,
            ConversionSemantics::ReferenceConversions);

        // Dev10#595234: as above, if there's narrowing inside a generic type parameter context is unforgivable.
        conversion = (conversion == ConversionNarrowing) ? ConversionError : conversion;

    }
    else 
    {
        VSFAIL("code logic error: inferenceRestrictions; we should have dealt with all of them already");
        conversion = ConversionError;
    }

    switch (conversion)
    {
        case ConversionError:
            return HintSatisfaction::Unsatisfied;
        case ConversionNarrowing:
            return HintSatisfaction::ThroughNarrowing;
        case ConversionWidening:
            return HintSatisfaction::ThroughWidening;
        case ConversionIdentity:
            return HintSatisfaction::ThroughIdentity;
        default:
            VSFAIL("code logic error: ConversionClass; we should have dealt with them already");
            return HintSatisfaction::Unsatisfied;
    }
}


// This method, given a set of types and constraints, attempts to find the
// best type that's in the set.
//
void
Semantics::TypeInferenceCollection::FindDominantType
(
    DominantTypeDataList& resultList,
    InferenceErrorReasons *inferenceErrorReasons,
    bool ignoreOptionStrict // = false
)
{

    // THE RULES FOR DOMINANT TYPE ARE THESE:
    //
    // Say we have hints T:{A+, B+, C-, D-}. Then calculate the following table...
    // For each hint+ we check "Hint<=Candidate", i.e. that Hint widens to Candidate
    // For each hint- we check "Candidate<=Hint", i.e. that Candidate widens to Hint.
    //
    //                hint:A+  hint:B+  hint:C-  hint:D-
    // candidate:A    A <= A   B <= A   A <= C   A <= D
    // candidate:B    A <= B   B <= B   B <= C   B <= D
    // candidate:C    A <= C   B <= C   C <= C   C <= D
    // candidate:D    A <= D   B <= D   D <= C   D <= D
    //
    // 1. If there is a unique "strict" candidate for which every check is ID/Widening, then return it.
    // 2. If there are multiple "strict" candidates for which each each check is ID/Widening, but these
    //    candidates have a unique widest ("dominant") type, then pick it.
    // 3. If there are no "strict" candidates for which each check is ID/Widening, but there is
    //    one "unstrict" candidate for which each check is ID/Widening/Narrowing, then pick it.
    // 4. Otherwise, fail.
    //
    // The motive for rule "2" was for cases like T:{Mammal+,Animal-} which would be ambiguous
    // otherwise, but for which C# picks Animal.
    //
    // Rule "2" might seem recursive ("to calculate dominant type we need to calculate dominant type
    // of a subset") but it's not really. NB. we might be tempted to just say that
    // if candidate A satisfies hint:B+ through widening, then A is okay as a dominant type for B; and
    // if candidate A satisfies hint:C- through widening, then A is not okay as a dominant type for C.
    // The first is true (and would let us re-use results that we've already calculated).
    // But the second is NOT true, because convertibility isn't symmetric.
    // So we need to calculate this table. Actually, we can't even re-use results from above.
    // That's because the first sweep through dominant would compare whether *expressions* could
    // be widened to the candidate type (for cases where a hint had an expression, e.g. for array
    // literals). The test we do for dominant type will compare candidate types only against
    // other candidate types, and won't check expressions.
    //
    //               hint:A+  hint:B+  hint:C+
    // candidate:A   .        .        C <= A
    // candidate:B   .        .        C <= B
    // candidate:C   .        .        C <= C
    //
    // e.g. T:{Mammal+, Animal-},
    // candidate "Mammal": ID, Widening     <-- a strict candidate
    // candidate "Animal": Widening, ID     <-- a strict candidate; also the unique widest one
    //
    // e.g. T:{Tiger+, Mammal-, Animal-},
    // candidate "Tiger":  ID, Widening, Widening  <-- a strict candidate
    // candidate "Mammal": Widening, ID, Widening  <-- a strict candidate; also the unique widest one
    // candidate "Animal": Widening, Narowing, ID  <-- an unstrict candidate
    // 


    unsigned int numberOfNodes = m_dominantTypeDataList.Count();
    bool optionStrictOff = !m_pSemantics->OptionStrictOn() || ignoreOptionStrict;
    ThrowIfNull(inferenceErrorReasons);

#if DEBUG
    bool shouldLog = numberOfNodes > 1;
    if (shouldLog)
    {
        DBG_SWITCH_PRINTF(fDumpInference, "    - Determining Dominant type between %d nodes:\n", numberOfNodes);
    }
#endif

    // The following four variables are so that, if we find an unambiguous answer,
    // we'll be able to return it immediately, rather than doing extra work.
    DominantTypeData *OneStrictCandidate = NULL;
    DominantTypeData *OneUnstrictCandidate = NULL;  // Note: we count strict candidates as unstrict candidates also
    unsigned int NumberOfStrictCandidates = 0;
    unsigned int NumberOfUnstrictCandidates = 0;

    // Now check each candidate against all its hints.
    DominantTypeDataListIterator candidateIter(&m_dominantTypeDataList);
    while (candidateIter.MoveNext())
    {
        DominantTypeData* candidateTypeData = candidateIter.Current();

        // expression-only nodes aren't real candidates for dominant type; they're here solely as hints:
        if (candidateTypeData->ResultType == NULL)
        {
            candidateTypeData->IsStrictCandidate = false;
            candidateTypeData->IsUnstrictCandidate = false;
            continue;
        }



        unsigned int NumberSatisfied[HintSatisfaction::Count] = {}; // sets all elements of array to 0

        DominantTypeDataListIterator hintIter(&m_dominantTypeDataList);
        while (hintIter.MoveNext()) 
        {
            DominantTypeData* hintTypeData = hintIter.Current();

            HintSatisfactionEnum hintSatisfaction = CheckHintSatisfaction(
                candidateTypeData,
                hintTypeData,
                hintTypeData->InferenceRestrictions);

            NumberSatisfied[hintSatisfaction] ++;

#if DEBUG
            const wchar_t *ConversionString = L"???";
            if (shouldLog)
            {
                const wchar_t *hintRestrictionString = L"???";
                switch (hintTypeData->InferenceRestrictions)
                {
                    case ConversionRequired::Identity:
                        hintRestrictionString = L"=";
                        break;
                    case ConversionRequired::Any:
                        hintRestrictionString = L"+any";
                        break;
                    case ConversionRequired::AnyReverse:
                        hintRestrictionString = L"-any";
                        break;
                    case ConversionRequired::AnyAndReverse:
                        hintRestrictionString = L"+-any";
                        break;
                    case ConversionRequired::ArrayElement:
                        hintRestrictionString = L"+arr";
                        break;
                    case ConversionRequired::Reference:
                        hintRestrictionString = L"+r";
                        break;
                    case ConversionRequired::ReverseReference:
                        hintRestrictionString = L"-r";
                        break;
                    case ConversionRequired::None:
                        hintRestrictionString = L"none";
                        break;
                }
                //
                const wchar_t *hintSatisfactionString = L"???";
                switch (hintSatisfaction)
                {
                    case HintSatisfaction::ThroughIdentity:
                        hintSatisfactionString = L"satisfied through identity";
                        break;
                    case HintSatisfaction::ThroughWidening:
                        hintSatisfactionString = L"satisfied through widening";
                        break;
                    case HintSatisfaction::ThroughNarrowing:
                        hintSatisfactionString = L"satisfied through narrowing";
                        break;
                    case HintSatisfaction::Unsatisfied:
                        hintSatisfactionString = L"unsatisfied";
                        break;
                }
                //
                StringBuffer CandidateTextBuffer, HintTextBuffer;
                //
                DBG_SWITCH_PRINTF(fDumpInference, L"      try '%s' against hint {%s %s}... %s\n",
                                  candidateTypeData->ResultType!=NULL
                                     ? m_pSemantics->ExtractErrorName(candidateTypeData->ResultType, CandidateTextBuffer)
                                     : L"<expr>",
                                  hintTypeData->ResultType!=NULL
                                     ? m_pSemantics->ExtractErrorName(hintTypeData->ResultType, HintTextBuffer)
                                     : L"<expr>",
                                  hintRestrictionString,
                                  hintSatisfactionString);

            }
#endif
        }

        // At this stage, for a candidate, we've gone through all the hints and made a count of how
        // well it satisfied them. So we store that information.
        // Incidentally, when we restart the type inference algorithm, the "candidateTypeData" object
        // persists. That's not a problem because we're overriding its boolean fields.
        candidateTypeData->IsStrictCandidate = (NumberSatisfied[HintSatisfaction::Unsatisfied]==0 && NumberSatisfied[HintSatisfaction::ThroughNarrowing]==0);
        candidateTypeData->IsUnstrictCandidate = (NumberSatisfied[HintSatisfaction::Unsatisfied]==0);

        // We might save the current candidate, as a shortcut: if in the end it turns out that there
        // was only one strict candidate, or only one unstrict candidate, then we can return that immediately.
        if (candidateTypeData->IsStrictCandidate)
        {
            NumberOfStrictCandidates++;
            OneStrictCandidate = candidateTypeData;
        }
        if (candidateTypeData->IsUnstrictCandidate)
        {
            NumberOfUnstrictCandidates++;
            OneUnstrictCandidate = candidateTypeData;
        }

#if DEBUG
        if (shouldLog)
        {
            StringBuffer CandidateTextBuffer;
            const WCHAR *candidacy = candidateTypeData->IsStrictCandidate ? L"a strict candidate." :
                                      (candidateTypeData->IsUnstrictCandidate ? L"an unstrict candidate." :
                                       L"not a candidate.");
            DBG_SWITCH_PRINTF(fDumpInference, L"      - candidate %s: %s\n",
                              candidateTypeData->ResultType==NULL
                                ? L"<expr>"
                                : m_pSemantics->ExtractErrorName(candidateTypeData->ResultType, CandidateTextBuffer),
                              candidacy);
        }
#endif
    }

    // NOTE: we count strict candidates as unstrict candidates also

    // Rule 1. "If there is a unique candidate for which every check is ID/Widening, then return it."
    if (NumberOfStrictCandidates == 1)
    {
        resultList.Add(OneStrictCandidate);
        return;
    }

    // Rule 3. "If there are no candidates for which each check is ID/Widening, but there is
    // one candidate for which each check is ID/Widening/Narrowing, then pick it."
    if (NumberOfStrictCandidates == 0 && NumberOfUnstrictCandidates == 1)
    {
        resultList.Add(OneUnstrictCandidate);
        return;
    }

    // Rule 4. "Otherwise, fail."
    if (NumberOfUnstrictCandidates == 0)
    {
        VSASSERT(NumberOfStrictCandidates == 0, "code logic error: since every strict candidate is also an unstrict candidate.");
        *inferenceErrorReasons |= InferenceErrorReasonsNoBest;
        return;
    }

    // If there were no strict candidates, but several unstrict ones, then we list them all and return ambiguity:
    if (NumberOfStrictCandidates == 0)
    {
        VSASSERT(NumberOfUnstrictCandidates>1, "code logic error: we should already have covered this case");
        DominantTypeDataListIterator i(&m_dominantTypeDataList);
        while (i.MoveNext())
        {
            if (i.Current()->IsUnstrictCandidate)
            {
                resultList.Add(i.Current());
            }
        }
        VSASSERT(resultList.Count() == NumberOfUnstrictCandidates, "code logic error: we should have >1 unstrict candidates, like we calculated earlier");
        *inferenceErrorReasons |= InferenceErrorReasonsAmbiguous;
        return;
    }

    // The only possibility remaining is that there were several widening candidates.
    VSASSERT(NumberOfStrictCandidates > 1, "code logic error: we should have >1 widening candidates; all other possibilities have already been covered");

    // Rule 2. "If there are multiple candidates for which each each check is ID/Widening, but these
    // candidates have a unique dominant type, then pick it."
    //
    // Note that we're only now looking for a unique dominant type amongst the IsStrictCandidates;
    // not amongst all candidates. Note also that this loop CANNOT have been already done inside the
    // candidate/hint loop above. That's because this loop requires knowledge of the IsStrictCandidate
    // flag for every candidate, and these flags were only complete at the end of the previous loop.
    //
    // Note that we could have reused some of the previous calculations, e.g. if a candidate satisfied "+r"
    // for every hint then it must also be a (possibly-joint-) widest candidate amongst just those hints that
    // are strict candidates. But we won't bother with this optimization, since it would break the modularity
    // of the "CheckHintSatisfaction" routine, and is a rare case anyway.

    DominantTypeDataListIterator widestOuterIter(&m_dominantTypeDataList);
    while (widestOuterIter.MoveNext())
    {
        DominantTypeData *outer = widestOuterIter.Current();
        
        // We're only now looking for widest candidates amongst the strict candidates;
        // so we're not concerned about conversions to candidates that weren't strict.
        if (!outer->IsStrictCandidate)
        {
            continue;
        }

        // we'll assume it is a (possibly-joint-)widest candidate, and only put "false" if it turns out not to be.
        bool IsOuterAWidestCandidate = true;

        DominantTypeDataListIterator widestInnerIter(&m_dominantTypeDataList);
        while (widestInnerIter.MoveNext())
        {
            DominantTypeData *inner = widestInnerIter.Current();
            // We're only now looking for widest candidates amongst the strict candidates;
            // so we're not concerned about conversions from candidates that weren't strict.
            if (!inner->IsStrictCandidate)
            {
                continue;
            }

            // No need to check against self: this will always work!
            if (outer == inner)
            {
                continue;
            }

            // A node was only ever a candidate if it had a type. e.g. "AddressOf" was never a candidate.
            if (outer->ResultType==NULL || inner->ResultType==NULL)
            {
                VSFAIL("How can a typeless hint be a candidate?");
                continue;
            }

            // Following is the same test as is done in CheckHintSatisfaction / TypeArgumentAnyConversion
            Procedure * OperatorMethod = NULL;
            GenericBinding *OperatorMethodGenericContext = NULL;
            bool OperatorMethodIsLifted;
            //
            ConversionClass conversion = m_pSemantics->ClassifyConversion(
                outer->ResultType, // convert TO outer
                inner->ResultType, // FROM inner
                OperatorMethod,
                OperatorMethodGenericContext,
                OperatorMethodIsLifted);
            
            if (conversion != ConversionIdentity && conversion != ConversionWidening)
            {
                IsOuterAWidestCandidate = false;
            }

#if DEBUG   
            if (shouldLog)
            {
                StringBuffer OuterTextBuffer, InnerTextBuffer;
                const WCHAR *widest = (conversion != ConversionIdentity && conversion != ConversionWidening)
                                           ? L"is not widest due to"
                                           : L"is not prevented from being widest through comparison to";
                DBG_SWITCH_PRINTF(fDumpInference, L"      - %s %s %s\n",
                                  outer->ResultType==NULL
                                     ? L"<expr>"
                                     : m_pSemantics->ExtractErrorName(outer->ResultType,OuterTextBuffer),
                                  widest,
                                  inner->ResultType==NULL
                                     ? L"<expr>"
                                     : m_pSemantics->ExtractErrorName(inner->ResultType,InnerTextBuffer));
            }
#endif

        }

        if (IsOuterAWidestCandidate)
        {
            resultList.Add(outer);
        }
    }

    // Rule 2. "If there are multiple candidates for which each each check is ID/Widening, but these
    // candidates have a unique dominant type, then pick it."
    if (resultList.Count() == 1)
    {
        return;
    }

    // If there were multiple dominant types out of that set, then return them all and say "ambiguous"
    if (resultList.Count() > 1)
    {
        *inferenceErrorReasons |= InferenceErrorReasonsAmbiguous;
        return;
    }

    // The only other possibility is that there were multiple strict candidates (and no widest ones).
    // So we'll return them all and say "ambiguous"

    VSFAIL("unexpected: how can there be multiple strict candidates and no widest ones??? please tell Microsoft if you find such a case.");
    // Actually, I believe this case to be impossible, but I can't figure out how to prove it.
    // So I'll leave the code in for now.

    DominantTypeDataListIterator returnAllStrictCandidatesIter(&m_dominantTypeDataList);
    while (returnAllStrictCandidatesIter.MoveNext())
    {
        if (returnAllStrictCandidatesIter.Current()->IsStrictCandidate)
        {
            resultList.Add(returnAllStrictCandidatesIter.Current());
        }
    }
    VSASSERT(resultList.Count() > 0, "code logic error: we already said there were multiple strict candidates");
    *inferenceErrorReasons |= InferenceErrorReasonsAmbiguous;
    return;
}




// ATTENTION!!!
// This function pays no regard to whether the type argument of a Nullable type satisfies its constraints. 
// It simply follows the spec to determine whether conversion is available, assuming the type is valid. 
// It is possible to get a non-ConversionError answer for conversion that cannot happen because participating 
// Nullable type uses an invalid type argument.
// Currently this doesn’t seem to cause a problem because an error about an invalid type argument 
// is reported elsewhere.
ConversionClass
Semantics::CheckNullableConversion
(
    bool &IsNullableInvolved,
    Type *TargetType,
    Type *SourceType,
    bool IgnoreProjectEquivalence,
    CompilerProject **ProjectForTargetType,
    CompilerProject **ProjectForSourceType,
    bool considerConversionsOnNullableBool,
    bool * pConversionRequiresUnliftedAccessToNullableValue
)
{
    if (pConversionRequiresUnliftedAccessToNullableValue)
    {
        *pConversionRequiresUnliftedAccessToNullableValue = false;
    }

    IsNullableInvolved = true;
    bool TargetIsNullable = TypeHelpers::IsNullableType(TargetType, m_CompilerHost); //Nullable( Of T) or Null reference T
    bool SourceIsNullable = TypeHelpers::IsNullableType(SourceType, m_CompilerHost);

    // start Nullable conversions only if no other predefined conversion exists for the types
    if((TargetIsNullable || SourceIsNullable) &&
        (ConversionError ==ClassifyPredefinedConversion(
                                TargetType,
                                SourceType,
                                IgnoreProjectEquivalence,
                                ProjectForTargetType,
                                ProjectForSourceType,
                                true /*ignore Nullable conversions*/,
                                considerConversionsOnNullableBool,
                                pConversionRequiresUnliftedAccessToNullableValue)))

    {
        // Dev10 #765086: Don't have predefined conversions from an Array literal to a Nullable type.
        // However, code below actually let's conversion to Nullable(Of Object) to slip through. 
        // Even though Nullable(Of Object) is an invalid type, it can be passed as a target type, 
        // the error is reported elsewhere.
        if (TargetIsNullable && SourceType->IsArrayLiteralType())
        {
            IsNullableInvolved = false;
            return ConversionError;
        }

    
        Type    *ElementTargetType = TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost);
        Type    *ElementSourceType = TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost);

        if (BCSYM::AreTypesEqual(ElementTargetType, ElementSourceType))
        {
            if
            (
                !considerConversionsOnNullableBool &&
                BCSYM::AreTypesEqual(ElementTargetType, GetFXSymbolProvider()->GetBooleanType())
            )
            {
                IsNullableInvolved = false;
                return ConversionError;
            }
            else
            {
                // conversion (T to T?) or (T? to T).
                // the identity cases (T to T) and (T? to T?) are treated before calling this function "if (TargetType == SourceType)"
                if ( TargetIsNullable )
                {
                    //widening conversion from T to T?
                    return ConversionWidening;
                }
                else
                {
                    if (pConversionRequiresUnliftedAccessToNullableValue)
                    {
                        *pConversionRequiresUnliftedAccessToNullableValue= true;
                    }
                    //narrowing conversion from T? to T
                    return ConversionNarrowing;
                }
            }
        }

        ConversionClass ElementConversionClassification =
            ClassifyPredefinedConversion
            (
                ElementTargetType,
                ElementSourceType,
                IgnoreProjectEquivalence,
                ProjectForTargetType,
                ProjectForSourceType,
                false
            );

        if (ConversionError == ElementConversionClassification)
        {
            // if no conversion S to T, then there is no conversion S? to T?
            return ConversionError;
        }

        if ( TargetIsNullable ) // case (S?, T?) or (S, T?)
        {
            if (considerConversionsOnNullableBool ||
                ! BCSYM::AreTypesEqual(
                    ElementTargetType,
                    GetFXSymbolProvider()->GetBooleanType()) ||
                SourceIsNullable)
            {
                // if (S converts(widening/narrowing) to T)  then (S? converts(widening/narrowing) to T?)
                // also, (S converts(widdening/narrowing) to T?)
                return ElementConversionClassification;
            }
            else
            {
                return ConversionError;
            }
        }
        else // case(S?, T)
        {
            // Dev10 #512957 Conversion from T? to an interface implemented by T is widening.
            // 'VB.NET specification says:
            //
            // '8.8 Widening Conversions
            //
            // '...
            //
            // 'Nullable Value Type conversions
            //
            // '...
            //
            // '- From a type T? to an interface type that the type T implements.
            if (ElementConversionClassification == ConversionWidening &&
                TypeHelpers::IsInterfaceType(TargetType))
            {
                return ConversionWidening;
            }
                    
            // S? converts narrowing to T. If there is a predefined conversion T to S

            // For now I do not see a case when there is a predefined conversion from S to T but no
            // predefined conversion T to S. The following condition might be superfluous.
            // Lets play safe for the future changes.
            if
            (
                ConversionError ==
                ClassifyPredefinedConversion
                (
                    ElementSourceType,
                    ElementTargetType,
                    IgnoreProjectEquivalence,
                    ProjectForSourceType,
                    ProjectForTargetType,
                    false,
                    false
                )
            )
            {
                return ConversionError;
            }
            else
            {
                if (pConversionRequiresUnliftedAccessToNullableValue)
                {
                    *pConversionRequiresUnliftedAccessToNullableValue = true;
                }
                return ConversionNarrowing;
            }
        }
    }
    IsNullableInvolved = false;
    return ConversionError; // the result is irrelevant

}
