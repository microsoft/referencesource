// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal partial class ExpressionBinder
    {

        // ----------------------------------------------------------------------------
        // BindExplicitConversion
        // ----------------------------------------------------------------------------

        private class ExplicitConversion
        {
            private ExpressionBinder binder;
            private EXPR exprSrc;
            private CType typeSrc;
            private CType typeDest;
            private EXPRTYPEORNAMESPACE exprTypeDest;

            // This is for lambda error reporting. The reason we have this is because we 
            // store errors for lambda conversions, and then we dont bind the conversion
            // again to report errors. Consider the following case:
            //
            // int? x = () => null;
            //
            // When we try to convert the lambda to the nullable type int?, we first 
            // attempt the conversion to int. If that fails, then we know there is no
            // conversion to int?, since int is a predef type. We then look for UserDefined
            // conversions, and fail. When we report the errors, we ask the lambda for its
            // conversion errors. But since we attempted its conversion to int and not int?, 
            // we report the wrong error. This field is to keep track of the right type 
            // to report the error on, so that when the lambda conversion fails, it reports
            // errors on the correct type.

            private CType m_pDestinationTypeForLambdaErrorReporting;
            private EXPR exprDest;
            private bool needsExprDest;
            private CONVERTTYPE flags;

            // ----------------------------------------------------------------------------
            // BindExplicitConversion
            // ----------------------------------------------------------------------------

            public ExplicitConversion(ExpressionBinder binder, EXPR exprSrc, CType typeSrc, EXPRTYPEORNAMESPACE typeDest, CType pDestinationTypeForLambdaErrorReporting, bool needsExprDest, CONVERTTYPE flags)
            {
                this.binder = binder;
                this.exprSrc = exprSrc;
                this.typeSrc = typeSrc;
                this.typeDest = typeDest.TypeOrNamespace.AsType();
                this.m_pDestinationTypeForLambdaErrorReporting = pDestinationTypeForLambdaErrorReporting;
                this.exprTypeDest = typeDest;
                this.needsExprDest = needsExprDest;
                this.flags = flags;
                this.exprDest = null;
            }
            public EXPR ExprDest { get { return exprDest; } }
            /*
             * BindExplicitConversion
             *
             * This is a complex routine with complex parameter. Generally, this should
             * be called through one of the helper methods that insulates you
             * from the complexity of the interface. This routine handles all the logic
             * associated with explicit conversions.
             *
             * Note that this function calls BindImplicitConversion first, so the main
             * logic is only concerned with conversions that can be made explicitly, but
             * not implicitly.
             */
            public bool Bind()
            {
                // To test for a standard conversion, call canConvert(exprSrc, typeDest, STANDARDANDCONVERTTYPE.NOUDC) and
                // canConvert(typeDest, typeSrc, STANDARDANDCONVERTTYPE.NOUDC).
                Debug.Assert((flags & CONVERTTYPE.STANDARD) == 0);

                // 13.2 Explicit conversions
                // 
                // The following conversions are classified as explicit conversions: 
                // 
                // * All implicit conversions
                // * Explicit numeric conversions
                // * Explicit enumeration conversions
                // * Explicit reference conversions
                // * Explicit interface conversions
                // * Unboxing conversions
                // * Explicit type parameter conversions
                // * User-defined explicit conversions
                // * Explicit nullable conversions
                // * Lifted user-defined explicit conversions
                //
                // Explicit conversions can occur in cast expressions (14.6.6).
                //
                // The explicit conversions that are not implicit conversions are conversions that cannot be
                // proven always to succeed, conversions that are known possibly to lose information, and
                // conversions across domains of types sufficiently different to merit explicit notation.

                // The set of explicit conversions includes all implicit conversions.

                // Don't try user-defined conversions now because we'll try them again later.
                if (binder.BindImplicitConversion(exprSrc, typeSrc, exprTypeDest, m_pDestinationTypeForLambdaErrorReporting, needsExprDest, out exprDest, flags | CONVERTTYPE.ISEXPLICIT))
                {
                    return true;
                }

                if (typeSrc == null || typeDest == null || typeSrc.IsErrorType() ||
                    typeDest.IsErrorType() || typeDest.IsNeverSameType())
                {
                    return false;
                }

                if (typeDest.IsNullableType())
                {
                    // This is handled completely by BindImplicitConversion.
                    return false;
                }

                if (typeSrc.IsNullableType())
                {
                    return bindExplicitConversionFromNub();
                }

                if (bindExplicitConversionFromArrayToIList())
                {
                    return true;
                }

                // if we were casting an integral constant to another constant type,
                // then, if the constant were in range, then the above call would have succeeded.

                // But it failed, and so we know that the constant is not in range

                switch (typeDest.GetTypeKind())
                {
                    default:
                        VSFAIL("Bad type kind");
                        return false;
                    case TypeKind.TK_VoidType:
                        return false; // Can't convert to a method group or anon method.
                    case TypeKind.TK_NullType:
                        return false;  // Can never convert TO the null type.
                    case TypeKind.TK_TypeParameterType:
                        if (bindExplicitConversionToTypeVar())
                        {
                            return true;
                        }
                        break;
                    case TypeKind.TK_ArrayType:
                        if (bindExplicitConversionToArray(typeDest.AsArrayType()))
                        {
                            return true;
                        }
                        break;
                    case TypeKind.TK_PointerType:
                        if (bindExplicitConversionToPointer())
                        {
                            return true;
                        }
                        break;
                    case TypeKind.TK_AggregateType:
                        {
                            AggCastResult result = bindExplicitConversionToAggregate(typeDest.AsAggregateType());

                            if (result == AggCastResult.Success)
                            {
                                return true;
                            }
                            if (result == AggCastResult.Abort)
                            {
                                return false;
                            }
                            break;
                        }
                }

                // No built-in conversion was found. Maybe a user-defined conversion?
                if (0 == (flags & CONVERTTYPE.NOUDC))
                {
                    return binder.bindUserDefinedConversion(exprSrc, typeSrc, typeDest, needsExprDest, out exprDest, false);
                }
                return false;
            }

            private bool bindExplicitConversionFromNub()
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(typeDest != null);

                // If S and T are value types and there is a builtin conversion from S => T then there is an
                // explicit conversion from S? => T that throws on null.
                if (typeDest.IsValType() && binder.BindExplicitConversion(null, typeSrc.StripNubs(), exprTypeDest, m_pDestinationTypeForLambdaErrorReporting, flags | CONVERTTYPE.NOUDC))
                {
                    if (needsExprDest)
                    {
                        EXPR valueSrc = exprSrc;
                        // UNDONE: This is a holdover from the days when you could have nullable of nullable.
                        // UNDONE: Can we remove this loop?
                        while (valueSrc.type.IsNullableType())
                        {
                            valueSrc = binder.BindNubValue(valueSrc);
                        }
                        Debug.Assert(valueSrc.type == typeSrc.StripNubs());
                        if (!binder.BindExplicitConversion(valueSrc, valueSrc.type, exprTypeDest, m_pDestinationTypeForLambdaErrorReporting, needsExprDest, out exprDest, flags | CONVERTTYPE.NOUDC))
                        {
                            VSFAIL("BindExplicitConversion failed unexpectedly");
                            return false;
                        }
                        if (exprDest.kind == ExpressionKind.EK_USERDEFINEDCONVERSION)
                        {
                            exprDest.asUSERDEFINEDCONVERSION().Argument = exprSrc;
                        }
                    }
                    return true;
                }

                if ((flags & CONVERTTYPE.NOUDC) == 0)
                {
                    return binder.bindUserDefinedConversion(exprSrc, typeSrc, typeDest, needsExprDest, out exprDest, false);
                }
                return false;
            }

            private bool bindExplicitConversionFromArrayToIList()
            {
                // 13.2.2
                //
                // The explicit reference conversions are:
                //
                // * From a one-dimensional array-type S[] to System.Collections.Generic.IList<T>, System.Collections.Generic.IReadOnlyList<T> and
                //   their base interfaces, provided there is an explicit reference conversion from S to T.

                Debug.Assert(typeSrc != null);
                Debug.Assert(typeDest != null);

                if (!typeSrc.IsArrayType() || typeSrc.AsArrayType().rank != 1 ||
                    !typeDest.isInterfaceType() || typeDest.AsAggregateType().GetTypeArgsAll().Size != 1)
                {
                    return false;
                }

                AggregateSymbol aggIList = GetSymbolLoader().GetOptPredefAgg(PredefinedType.PT_G_ILIST);
                AggregateSymbol aggIReadOnlyList = GetSymbolLoader().GetOptPredefAgg(PredefinedType.PT_G_IREADONLYLIST);
                
                if ((aggIList == null ||
                    !GetSymbolLoader().IsBaseAggregate(aggIList, typeDest.AsAggregateType().getAggregate())) &&
                    (aggIReadOnlyList == null ||
                    !GetSymbolLoader().IsBaseAggregate(aggIReadOnlyList, typeDest.AsAggregateType().getAggregate())))
                {
                    return false;
                }

                CType typeArr = typeSrc.AsArrayType().GetElementType();
                CType typeLst = typeDest.AsAggregateType().GetTypeArgsAll().Item(0);

                if (!CConversions.FExpRefConv(GetSymbolLoader(), typeArr, typeLst))
                {
                    return false;
                }

                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK);
                return true;
            }

            private bool bindExplicitConversionToTypeVar()
            {

                // 13.2.3 Explicit reference conversions
                //
                // For a type-parameter T that is known to be a reference type (25.7), the following
                // explicit reference conversions exist:
                //
                // * From the effective base class C of T to T and from any base class of C to T.
                // * From any interface-type to T.
                // * From a type-parameter U to T provided that T depends on U (25.7).

                Debug.Assert(typeSrc != null);
                Debug.Assert(typeDest != null);

                // NOTE: for the flags, we have to use EXPRFLAG.EXF_FORCE_UNBOX (not EXPRFLAG.EXF_REFCHECK) even when
                // we know that the type is a reference type. The verifier expects all code for
                // type parameters to behave as if the type parameter is a value type.
                // The jitter should be smart about it....
                if (typeSrc.isInterfaceType() || binder.canConvert(typeDest, typeSrc, CONVERTTYPE.NOUDC))
                {
                    if (!needsExprDest)
                    {
                        return true;
                    }

                    // There is an explicit, possibly unboxing, conversion from Object or any interface to
                    // a type variable. This will involve a type check and possibly an unbox.
                    // There is an explicit conversion from non-interface X to the type var iff there is an
                    // implicit conversion from the type var to X.
                    if (typeSrc.IsTypeParameterType())
                    {
                        // Need to box first before unboxing.
                        EXPR exprT;
                        EXPRCLASS exprObj = GetExprFactory().MakeClass(binder.GetReqPDT(PredefinedType.PT_OBJECT));
                        binder.bindSimpleCast(exprSrc, exprObj, out exprT, EXPRFLAG.EXF_FORCE_BOX);
                        exprSrc = exprT;
                    }
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_FORCE_UNBOX);
                    return true;
                }
                return false;
            }

            private bool bindExplicitConversionFromIListToArray(ArrayType arrayDest)
            {
                // 13.2.2
                //
                // The explicit reference conversions are:
                //
                // * From System.Collections.Generic.IList<T>, System.Collections.Generic.IReadOnlyList<T> and their base interfaces 
                //   to a one-dimensional array-type S[], provided there is an implicit or explicit reference conversion from
                //   S[] to System.Collections.Generic.IList<T> or System.Collections.Generic.IReadOnlyList<T>. This is precisely when either S and T
                //   are the same type or there is an implicit or explicit reference conversion from S to T.

                if (arrayDest.rank != 1 || !typeSrc.isInterfaceType() ||
                    typeSrc.AsAggregateType().GetTypeArgsAll().Size != 1)
                {
                    return false;
                }

                AggregateSymbol aggIList = GetSymbolLoader().GetOptPredefAgg(PredefinedType.PT_G_ILIST);
                AggregateSymbol aggIReadOnlyList = GetSymbolLoader().GetOptPredefAgg(PredefinedType.PT_G_IREADONLYLIST);
                
                if ((aggIList == null ||
                    !GetSymbolLoader().IsBaseAggregate(aggIList, typeSrc.AsAggregateType().getAggregate())) &&
                    (aggIReadOnlyList == null ||
                    !GetSymbolLoader().IsBaseAggregate(aggIReadOnlyList, typeSrc.AsAggregateType().getAggregate())))
                {
                    return false;
                }

                CType typeArr = arrayDest.GetElementType();
                CType typeLst = typeSrc.AsAggregateType().GetTypeArgsAll().Item(0);

                Debug.Assert(!typeArr.IsNeverSameType());
                if (typeArr != typeLst && !CConversions.FExpRefConv(GetSymbolLoader(), typeArr, typeLst))
                {
                    return false;
                }
                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK);
                return true;
            }

            private bool bindExplicitConversionFromArrayToArray(ArrayType arraySrc, ArrayType arrayDest)
            {
                // 13.2.2
                //
                // The explicit reference conversions are:
                //
                // * From an array-type S with an element type SE to an array-type T with an element type
                //   TE, provided all of the following are true:
                //
                //   * S and T differ only in element type. (In other words, S and T have the same number
                //     of dimensions.)
                //
                //   * An explicit reference conversion exists from SE to TE.

                if (arraySrc.rank != arrayDest.rank)
                {
                    return false;  // Ranks do not match.
                }

                if (CConversions.FExpRefConv(GetSymbolLoader(), arraySrc.GetElementType(), arrayDest.GetElementType()))
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK);
                    return true;
                }

                return false;
            }

            private bool bindExplicitConversionToArray(ArrayType arrayDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(arrayDest != null);

                if (typeSrc.IsArrayType())
                {
                    return bindExplicitConversionFromArrayToArray(typeSrc.AsArrayType(), arrayDest);
                }

                if (bindExplicitConversionFromIListToArray(arrayDest))
                {
                    return true;
                }

                // 13.2.2
                //
                // The explicit reference conversions are:
                //
                // * From System.Array and the interfaces it implements, to any array-type.

                if (binder.canConvert(binder.GetReqPDT(PredefinedType.PT_ARRAY), typeSrc, CONVERTTYPE.NOUDC))
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK);
                    return true;
                }
                return false;
            }

            private bool bindExplicitConversionToPointer()
            {

                // 27.4 Pointer conversions
                //
                // in an unsafe context, the set of available explicit conversions (13.2) is extended to
                // include the following explicit pointer conversions:
                //
                // * From any pointer-type to any other pointer-type.
                // * From sbyte, byte, short, ushort, int, uint, long, or ulong to any pointer-type.

                if (typeSrc.IsPointerType() || typeSrc.fundType() <= FUNDTYPE.FT_LASTINTEGRAL && typeSrc.isNumericType())
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                    return true;
                }
                return false;
            }

            // 13.2.2 Explicit enumeration conversions
            //
            // The explicit enumeration conversions are:
            //
            // * From sbyte, byte, short, ushort, int, uint, long, ulong, char, float, double, or
            //   decimal to any enum-type.
            //
            // * From any enum-type to sbyte, byte, short, ushort, int, uint, long, ulong, char,
            //   float, double, or decimal.
            //
            // * From any enum-type to any other enum-type.
            //
            // * An explicit enumeration conversion between two types is processed by treating any
            //   participating enum-type as the underlying type of that enum-type, and then performing
            //   an implicit or explicit numeric conversion between the resulting types.

            private AggCastResult bindExplicitConversionFromEnumToAggregate(AggregateType aggTypeDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);

                if (!typeSrc.isEnumType())
                {
                    return AggCastResult.Failure;
                }

                AggregateSymbol aggDest = aggTypeDest.getAggregate();
                if (aggDest.isPredefAgg(PredefinedType.PT_DECIMAL))
                {
                    return bindExplicitConversionFromEnumToDecimal(aggTypeDest);
                }


                if (!aggDest.getThisType().isNumericType() &&
                    !aggDest.IsEnum() &&
                    !(aggDest.IsPredefined() && aggDest.GetPredefType() == PredefinedType.PT_CHAR))
                {
                    return AggCastResult.Failure;
                }

                if (exprSrc.GetConst() != null)
                {
                    ConstCastResult result = binder.bindConstantCast(exprSrc, exprTypeDest, needsExprDest, out exprDest, true);
                    if (result == ConstCastResult.Success)
                    {
                        return AggCastResult.Success;
                    }
                    else if (result == ConstCastResult.CheckFailure)
                    {
                        return AggCastResult.Abort;
                    }
                }

                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                return AggCastResult.Success;
            }

            private AggCastResult bindExplicitConversionFromDecimalToEnum(AggregateType aggTypeDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(typeSrc.isPredefType(PredefinedType.PT_DECIMAL));

                // There is an explicit conversion from decimal to all integral types.
                if (exprSrc.GetConst() != null)
                {
                    // Fold the constant cast if possible.
                    ConstCastResult result = binder.bindConstantCast(exprSrc, exprTypeDest, needsExprDest, out exprDest, true);
                    if (result == ConstCastResult.Success)
                    {
                        return AggCastResult.Success;  // else, don't fold and use a regular cast, below.
                    }
                    if (result == ConstCastResult.CheckFailure && 0 == (flags & CONVERTTYPE.CHECKOVERFLOW))
                    {
                        return AggCastResult.Abort;
                    }
                }

                // All casts from decimal to integer types are bound as user-defined conversions.

                bool bIsConversionOK = true;
                if (needsExprDest)
                {
                    // According the language, this is a standard conversion, but it is implemented
                    // through a user-defined conversion. Because it's a standard conversion, we don't
                    // test the CONVERTTYPE.NOUDC flag here.
                    CType underlyingType = aggTypeDest.underlyingType();
                    bIsConversionOK = binder.bindUserDefinedConversion(exprSrc, typeSrc, underlyingType, needsExprDest, out exprDest, false);

                    if (bIsConversionOK)
                    {
                        // upcast to the Enum type
                        binder.bindSimpleCast(exprDest, exprTypeDest, out exprDest);
                    }
                }
                return bIsConversionOK ? AggCastResult.Success : AggCastResult.Failure;
            }

            private AggCastResult bindExplicitConversionFromEnumToDecimal(AggregateType aggTypeDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);
                Debug.Assert(aggTypeDest.isPredefType(PredefinedType.PT_DECIMAL));

                AggregateType underlyingType = typeSrc.underlyingType().AsAggregateType();

                // Need to first cast the source expr to its underlying type.

                EXPR exprCast;

                if (exprSrc == null)
                {
                    exprCast = null;
                }
                else
                {
                    EXPRCLASS underlyingExpr = GetExprFactory().MakeClass(underlyingType);
                    binder.bindSimpleCast(exprSrc, underlyingExpr, out exprCast);
                }

                // There is always an implicit conversion from any integral type to decimal.

                if (exprCast.GetConst() != null)
                {
                    // Fold the constant cast if possible.
                    ConstCastResult result = binder.bindConstantCast(exprCast, exprTypeDest, needsExprDest, out exprDest, true);
                    if (result == ConstCastResult.Success)
                    {
                        return AggCastResult.Success;  // else, don't fold and use a regular cast, below.
                    }
                    if (result == ConstCastResult.CheckFailure && 0 == (flags & CONVERTTYPE.CHECKOVERFLOW))
                    {
                        return AggCastResult.Abort;
                    }
                }

                // Conversions from integral types to decimal are always bound as a user-defined conversion.

                if (needsExprDest)
                {
                    // According the language, this is a standard conversion, but it is implemented
                    // through a user-defined conversion. Because it's a standard conversion, we don't
                    // test the CONVERTTYPE.NOUDC flag here.

                    bool ok = binder.bindUserDefinedConversion(exprCast, underlyingType, aggTypeDest, needsExprDest, out exprDest, false);
                    Debug.Assert(ok);
                }

                return AggCastResult.Success;
            }

            private AggCastResult bindExplicitConversionToEnum(AggregateType aggTypeDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);

                AggregateSymbol aggDest = aggTypeDest.getAggregate();
                if (!aggDest.IsEnum())
                {
                    return AggCastResult.Failure;
                }

                if (typeSrc.isPredefType(PredefinedType.PT_DECIMAL))
                {
                    return bindExplicitConversionFromDecimalToEnum(aggTypeDest);
                }

                if (typeSrc.isNumericType() || (typeSrc.isPredefined() && typeSrc.getPredefType() == PredefinedType.PT_CHAR))
                {
                    // Transform constant to constant.
                    if (exprSrc.GetConst() != null)
                    {
                        ConstCastResult result = binder.bindConstantCast(exprSrc, exprTypeDest, needsExprDest, out exprDest, true);
                        if (result == ConstCastResult.Success)
                        {
                            return AggCastResult.Success;
                        }
                        if (result == ConstCastResult.CheckFailure)
                        {
                            return AggCastResult.Abort;
                        }
                    }
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                    return AggCastResult.Success;
                }
                else if (typeSrc.isPredefined() &&
                         (typeSrc.isPredefType(PredefinedType.PT_OBJECT) || typeSrc.isPredefType(PredefinedType.PT_VALUE) || typeSrc.isPredefType(PredefinedType.PT_ENUM)))
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_UNBOX);
                    return AggCastResult.Success;
                }
                return AggCastResult.Failure;
            }

            private AggCastResult bindExplicitConversionBetweenSimpleTypes(AggregateType aggTypeDest)
            {

                // 13.2.1
                //
                // Because the explicit conversions include all implicit and explicit numeric conversions,
                // it is always possible to convert from any numeric-type to any other numeric-type using
                // a cast expression (14.6.6).

                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);

                if (!typeSrc.isSimpleType() || !aggTypeDest.isSimpleType())
                {
                    return AggCastResult.Failure;
                }

                AggregateSymbol aggDest = aggTypeDest.getAggregate();

                Debug.Assert(typeSrc.isPredefined() && aggDest.IsPredefined());

                PredefinedType ptSrc = typeSrc.getPredefType();
                PredefinedType ptDest = aggDest.GetPredefType();

                Debug.Assert((int)ptSrc < NUM_SIMPLE_TYPES && (int)ptDest < NUM_SIMPLE_TYPES);

                ConvKind convertKind = GetConvKind(ptSrc, ptDest);
                // Identity and implicit conversions should already have been handled.
                Debug.Assert(convertKind != ConvKind.Implicit);
                Debug.Assert(convertKind != ConvKind.Identity);

                if (convertKind != ConvKind.Explicit)
                {
                    return AggCastResult.Failure;
                }

                if (exprSrc.GetConst() != null)
                {
                    // Fold the constant cast if possible.
                    ConstCastResult result = binder.bindConstantCast(exprSrc, exprTypeDest, needsExprDest, out exprDest, true);
                    if (result == ConstCastResult.Success)
                    {
                        return AggCastResult.Success;  // else, don't fold and use a regular cast, below.
                    }
                    if (result == ConstCastResult.CheckFailure && 0 == (flags & CONVERTTYPE.CHECKOVERFLOW))
                    {
                        return AggCastResult.Abort;
                    }
                }

                bool bConversionOk = true;
                if (needsExprDest)
                {
                    // Explicit conversions involving decimals are bound as user-defined conversions.
                    if (isUserDefinedConversion(ptSrc, ptDest))
                    {
                        // According the language, this is a standard conversion, but it is implemented
                        // through a user-defined conversion. Because it's a standard conversion, we don't
                        // test the CONVERTTYPE.NOUDC flag here.
                        bConversionOk = binder.bindUserDefinedConversion(exprSrc, typeSrc, aggTypeDest, needsExprDest, out exprDest, false);
                    }
                    else
                    {
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, (flags & CONVERTTYPE.CHECKOVERFLOW) != 0 ? EXPRFLAG.EXF_CHECKOVERFLOW : 0);
                    }
                }
                return bConversionOk ? AggCastResult.Success : AggCastResult.Failure;
            }

            private AggCastResult bindExplicitConversionBetweenAggregates(AggregateType aggTypeDest)
            {

                // 13.2.3 
                //
                // The explicit reference conversions are:
                //
                // * From object to any reference-type.
                // * From any class-type S to any class-type T, provided S is a base class of T.
                // * From any class-type S to any interface-type T, provided S is not sealed and
                //   provided S does not implement T.
                // * From any interface-type S to any class-type T, provided T is not sealed or provided
                //   T implements S.
                // * From any interface-type S to any interface-type T, provided S is not derived from T.

                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);

                if (!typeSrc.IsAggregateType())
                {
                    return AggCastResult.Failure;
                }

                AggregateSymbol aggSrc = typeSrc.AsAggregateType().getAggregate();
                AggregateSymbol aggDest = aggTypeDest.getAggregate();

                if (GetSymbolLoader().HasBaseConversion(aggTypeDest, typeSrc.AsAggregateType()))
                {
                    if (needsExprDest)
                    {
                        if (aggDest.IsValueType() && aggSrc.getThisType().fundType() == FUNDTYPE.FT_REF)
                        {
                            binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_UNBOX);
                        }
                        else
                        {
                            binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK | (exprSrc != null ? (exprSrc.flags & EXPRFLAG.EXF_CANTBENULL) : 0));
                        }
                    }
                    return AggCastResult.Success;
                }

                if ((aggSrc.IsClass() && !aggSrc.IsSealed() && aggDest.IsInterface()) ||
                    (aggSrc.IsInterface() && aggDest.IsClass() && !aggDest.IsSealed()) ||
                    (aggSrc.IsInterface() && aggDest.IsInterface()) ||
                    CConversions.HasGenericDelegateExplicitReferenceConversion(GetSymbolLoader(), typeSrc, aggTypeDest))
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_REFCHECK | (exprSrc != null ? (exprSrc.flags & EXPRFLAG.EXF_CANTBENULL) : 0));
                    return AggCastResult.Success;
                }
                return AggCastResult.Failure;
            }

            private AggCastResult bindExplicitConversionFromPointerToInt(AggregateType aggTypeDest)
            {

                // 27.4 Pointer conversions
                // in an unsafe context, the set of available explicit conversions (13.2) is extended to include
                // the following explicit pointer conversions:
                //
                // * From any pointer-type to sbyte, byte, short, ushort, int, uint, long, or ulong.

                if (!typeSrc.IsPointerType() || aggTypeDest.fundType() > FUNDTYPE.FT_LASTINTEGRAL || !aggTypeDest.isNumericType())
                {
                    return AggCastResult.Failure;
                }
                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                return AggCastResult.Success;
            }

            private AggCastResult bindExplicitConversionFromTypeVarToAggregate(AggregateType aggTypeDest)
            {

                // 13.2.3 Explicit reference conversions
                //
                // For a type-parameter T that is known to be a reference type (25.7), the following
                // explicit reference conversions exist:
                //
                // * From T to any interface-type I provided there isn't already an implicit reference
                //   conversion from T to I.

                if (!typeSrc.IsTypeParameterType())
                {
                    return AggCastResult.Failure;
                }
#if ! CSEE
                if (aggTypeDest.getAggregate().IsInterface())
#else
                if ((exprSrc != null && !exprSrc.eeValue.substType.IsNullableType()) || aggTypeDest.getAggregate().IsInterface())
#endif
                {
                    // Explicit conversion of type variables to interfaces.
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_FORCE_BOX | EXPRFLAG.EXF_REFCHECK);
                    return AggCastResult.Success;
                }
                return AggCastResult.Failure;
            }

            private AggCastResult bindExplicitConversionToAggregate(AggregateType aggTypeDest)
            {
                Debug.Assert(typeSrc != null);
                Debug.Assert(aggTypeDest != null);

                // TypeReference and ArgIterator can't be boxed (or converted to anything else)
                if (typeSrc.isSpecialByRefType())
                {
                    return AggCastResult.Abort;
                }

                AggCastResult result;

                result = bindExplicitConversionFromEnumToAggregate(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                result = bindExplicitConversionToEnum(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                result = bindExplicitConversionBetweenSimpleTypes(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                result = bindExplicitConversionBetweenAggregates(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                result = bindExplicitConversionFromPointerToInt(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                if (typeSrc.IsVoidType())
                {
                    // No conversion is allowed to or from a void type (user defined or otherwise)
                    // This is most likely the result of a failed anonymous method or member group conversion
                    return AggCastResult.Abort;
                }

                result = bindExplicitConversionFromTypeVarToAggregate(aggTypeDest);
                if (result != AggCastResult.Failure)
                {
                    return result;
                }

                return AggCastResult.Failure;
            }

            private SymbolLoader GetSymbolLoader()
            {
                return binder.GetSymbolLoader();
            }

            private ExprFactory GetExprFactory()
            {
                return binder.GetExprFactory();
            }
        }
    }
}
