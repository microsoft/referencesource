// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Errors;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal partial class ExpressionBinder
    {

        // ----------------------------------------------------------------------------
        // BindImplicitConversion
        // ----------------------------------------------------------------------------

        private class ImplicitConversion
        {
            public ImplicitConversion(ExpressionBinder binder, EXPR exprSrc, CType typeSrc, EXPRTYPEORNAMESPACE typeDest, bool needsExprDest, CONVERTTYPE flags)
            {
                this.binder = binder;
                this.exprSrc = exprSrc;
                this.typeSrc = typeSrc;
                this.typeDest = typeDest.TypeOrNamespace.AsType();
                this.exprTypeDest = typeDest;
                this.needsExprDest = needsExprDest;
                this.flags = flags;
                this.exprDest = null;
            }
            public EXPR ExprDest { get { return exprDest; } }
            private EXPR exprDest;
            private ExpressionBinder binder;
            private EXPR exprSrc;
            private CType typeSrc;
            private CType typeDest;
            private EXPRTYPEORNAMESPACE exprTypeDest;
            private bool needsExprDest;
            private CONVERTTYPE flags;

            /*
             * BindImplicitConversion
             *
             * This is a complex routine with complex parameters. Generally, this should
             * be called through one of the helper methods that insulates you
             * from the complexity of the interface. This routine handles all the logic
             * associated with implicit conversions.
             *
             * exprSrc - the expression being converted. Can be null if only type conversion
             *           info is being supplied.
             * typeSrc - type of the source
             * typeDest - type of the destination
             * exprDest - returns an expression of the src converted to the dest. If null, we
             *            only care about whether the conversion can be attempted, not the
             *            expression tree.
             * flags    - flags possibly customizing the conversions allowed. E.g., can suppress
             *            user-defined conversions.
             *
             * returns true if the conversion can be made, false if not.
             */
            public bool Bind()
            {
                // 13.1 Implicit conversions
                // 
                // The following conversions are classified as implicit conversions:
                // 
                // *   Identity conversions
                // *   Implicit numeric conversions
                // *   Implicit enumeration conversions
                // *   Implicit reference conversions
                // *   Boxing conversions
                // *   Implicit type parameter conversions
                // *   Implicit constant expression conversions
                // *   User-defined implicit conversions
                // *   Implicit conversions from an anonymous method expression to a compatible delegate type
                // *   Implicit conversion from a method group to a compatible delegate type
                // *   Conversions from the null type (11.2.7) to any nullable type
                // *   Implicit nullable conversions
                // *   Lifted user-defined implicit conversions
                // 
                // Implicit conversions can occur in a variety of situations, including function member invocations
                // (14.4.3), cast expressions (14.6.6), and assignments (14.14).

                // Can't convert to or from the error type.
                if (typeSrc == null || typeDest == null || typeDest.IsNeverSameType())
                {
                    return false;
                }

                Debug.Assert(typeSrc != null && typeDest != null);            // types must be supplied.
                Debug.Assert(exprSrc == null || typeSrc == exprSrc.type);    // type of source should be correct if source supplied
                Debug.Assert(!needsExprDest || exprSrc != null);           // need source expr to create dest expr

                switch (typeDest.GetTypeKind())
                {
                    case TypeKind.TK_ErrorType:
                        Debug.Assert(typeDest.AsErrorType().HasTypeParent() || typeDest.AsErrorType().HasNSParent());
                        if (typeSrc != typeDest)
                        {
                            return false;
                        }
                        if (needsExprDest)
                        {
                            exprDest = exprSrc;
                        }
                        return true;
                    case TypeKind.TK_NullType:
                        // Can only convert to the null type if src is null.
                        if (!typeSrc.IsNullType())
                        {
                            return false;
                        }
                        if (needsExprDest)
                        {
                            exprDest = exprSrc;
                        }
                        return true;
                    case TypeKind.TK_MethodGroupType:
                        VSFAIL("Something is wrong with Type.IsNeverSameType()");
                        return false;
                    case TypeKind.TK_NaturalIntegerType:
                    case TypeKind.TK_ArgumentListType:
                        return typeSrc == typeDest;
                    case TypeKind.TK_VoidType:
                        return false;
                    default:
                        break;
                }

                if (typeSrc.IsErrorType())
                {
                    Debug.Assert(!typeDest.IsErrorType());
                    return false;
                }

                // 13.1.1 Identity conversion
                //
                // An identity conversion converts from any type to the same type. This conversion exists only 
                // such that an entity that already has a required type can be said to be convertible to that type.

                if (typeSrc == typeDest &&
                    ((flags & CONVERTTYPE.ISEXPLICIT) == 0 || (!typeSrc.isPredefType(PredefinedType.PT_FLOAT) && !typeSrc.isPredefType(PredefinedType.PT_DOUBLE))))
                {
                    if (needsExprDest)
                    {
                        exprDest = exprSrc;
                    }
                    return true;
                }

                if (typeDest.IsNullableType())
                {
                    return BindNubConversion(typeDest.AsNullableType());
                }

                if (typeSrc.IsNullableType())
                {
                    return bindImplicitConversionFromNullable(typeSrc.AsNullableType());
                }

                if ((flags & CONVERTTYPE.ISEXPLICIT) != 0)
                {
                    flags |= CONVERTTYPE.NOUDC;
                }

                // Get the fundamental types of destination.
                FUNDTYPE ftDest = typeDest.fundType();
                Debug.Assert(ftDest != FUNDTYPE.FT_NONE || typeDest.IsParameterModifierType());

                switch (typeSrc.GetTypeKind())
                {
                    default:
                        VSFAIL("Bad type symbol kind");
                        break;
                    case TypeKind.TK_MethodGroupType:
                        if (exprSrc.isMEMGRP())
                        {
                            EXPRCALL outExpr;
                            bool retVal = binder.BindGrpConversion(exprSrc.asMEMGRP(), typeDest, needsExprDest, out outExpr, false);
                            exprDest = outExpr;
                            return retVal;
                        }
                        return false;
                    case TypeKind.TK_VoidType:
                    case TypeKind.TK_ErrorType:
                    case TypeKind.TK_ParameterModifierType:
                    case TypeKind.TK_ArgumentListType:
                        return false;
                    case TypeKind.TK_NullType:
                        if (bindImplicitConversionFromNull())
                        {
                            return true;
                        }
                        // If not, try user defined implicit conversions.
                        break;
                    case TypeKind.TK_ArrayType:
                        if (bindImplicitConversionFromArray())
                        {
                            return true;
                        }
                        // If not, try user defined implicit conversions.
                        break;
                    case TypeKind.TK_PointerType:
                        if (bindImplicitConversionFromPointer())
                        {
                            return true;
                        }
                        // If not, try user defined implicit conversions.
                        break;
                    case TypeKind.TK_TypeParameterType:
                        if (bindImplicitConversionFromTypeVar(typeSrc.AsTypeParameterType()))
                        {
                            return true;
                        }
                        // If not, try user defined implicit conversions.
                        break;
                    case TypeKind.TK_AggregateType:
                        // TypeReference and ArgIterator can't be boxed (or converted to anything else)
                        if (typeSrc.isSpecialByRefType())
                        {
                            return false;
                        }
                        if (bindImplicitConversionFromAgg(typeSrc.AsAggregateType()))
                        {
                            return true;
                        }
                        // If not, try user defined implicit conversions.
                        break;
                }

                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // RUNTIME BINDER ONLY CHANGE
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                //
                // Every incoming dynamic operand should be implicitly convertible
                // to any type that it is an instance of.

                if (exprSrc != null
                    && exprSrc.RuntimeObject != null
                    && typeDest.AssociatedSystemType.IsInstanceOfType(exprSrc.RuntimeObject)
                    && binder.GetSemanticChecker().CheckTypeAccess(typeDest, binder.Context.ContextForMemberLookup()))
                {
                    if (needsExprDest)
                    {
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, exprSrc.flags & EXPRFLAG.EXF_CANTBENULL);
                    }
                    return true;
                }

                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // END RUNTIME BINDER ONLY CHANGE
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                // 13.1.8 User-defined implicit conversions
                //
                // A user-defined implicit conversion consists of an optional standard implicit conversion, 
                // followed by execution of a user-defined implicit conversion operator, followed by another
                // optional standard implicit conversion. The exact rules for evaluating user-defined
                // conversions are described in 13.4.3.

                if (0 == (flags & CONVERTTYPE.NOUDC))
                {
                    return binder.bindUserDefinedConversion(exprSrc, typeSrc, typeDest, needsExprDest, out exprDest, true);
                }
                
                // No conversion was found.

                return false;
            }


            /***************************************************************************************************
                Called by BindImplicitConversion when the destination type is Nullable<T>. The following
                conversions are handled by this method:
             
                * For S in { object, ValueType, interfaces implemented by underlying type} there is an explicit
                  unboxing conversion S => T?
                * System.Enum => T? there is an unboxing conversion if T is an enum type
                * null => T? implemented as default(T?)
             
                * Implicit T?* => T?+ implemented by either wrapping or calling GetValueOrDefault the
                  appropriate number of times.
                * If imp/exp S => T then imp/exp S => T?+ implemented by converting to T then wrapping the
                  appropriate number of times.
                * If imp/exp S => T then imp/exp S?+ => T?+ implemented by calling GetValueOrDefault (m-1) times
                  then calling HasValue, producing a null if it returns false, otherwise calling Value,
                  converting to T then wrapping the appropriate number of times.
             
                The 3 rules above can be summarized with the following recursive rules:
             
                * If imp/exp S => T? then imp/exp S? => T? implemented as
                  qs.HasValue ? (T?)(qs.Value) : default(T?)
                * If imp/exp S => T then imp/exp S => T? implemented as new T?((T)s)
             
                This method also handles calling bindUserDefinedConverion. This method does NOT handle
                the following conversions:
             
                * Implicit boxing conversion from S? to { object, ValueType, Enum, ifaces implemented by S }. (Handled by BindImplicitConversion.)
                * If imp/exp S => T then explicit S?+ => T implemented by calling Value the appropriate number
                  of times. (Handled by BindExplicitConversion.)
             
                The recursive equivalent is:
             
                * If imp/exp S => T and T is not nullable then explicit S? => T implemented as qs.Value
             
                Some nullable conversion are NOT standard conversions. In particular, if S => T is implicit
                then S? => T is not standard. Similarly if S => T is not implicit then S => T? is not standard.
            ***************************************************************************************************/
            private bool BindNubConversion(NullableType nubDst)
            {
                // This code assumes that STANDARD and ISEXPLICIT are never both set.
                // bindUserDefinedConversion should ensure this!
                Debug.Assert(0 != (~flags & (CONVERTTYPE.STANDARD | CONVERTTYPE.ISEXPLICIT)));
                Debug.Assert(exprSrc == null || exprSrc.type == typeSrc);
                Debug.Assert(!needsExprDest || exprSrc != null);
                Debug.Assert(typeSrc != nubDst); // BindImplicitConversion should have taken care of this already.
                AggregateType atsDst = nubDst.GetAts(GetErrorContext());
                if (atsDst == null)
                    return false;

                // Check for the unboxing conversion. This takes precedence over the wrapping conversions.
                if (GetSymbolLoader().HasBaseConversion(nubDst.GetUnderlyingType(), typeSrc) && !CConversions.FWrappingConv(typeSrc, nubDst))
                {
                    // These should be different! Fix the caller if typeSrc is an AggregateType of Nullable.
                    Debug.Assert(atsDst != typeSrc);

                    // typeSrc is a base type of the destination nullable type so there is an explicit
                    // unboxing conversion.
                    if (0 == (flags & CONVERTTYPE.ISEXPLICIT))
                    {
                        return false;
                    }

                    if (needsExprDest)
                    {
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_UNBOX);
                    }
                    return true;
                }

                int cnubDst;
                int cnubSrc;
                CType typeDstBase = nubDst.StripNubs(out cnubDst);
                EXPRCLASS exprTypeDstBase = GetExprFactory().MakeClass(typeDstBase);
                CType typeSrcBase = typeSrc.StripNubs(out cnubSrc);

                ConversionFunc pfn = (flags & CONVERTTYPE.ISEXPLICIT) != 0 ?
                    (ConversionFunc)binder.BindExplicitConversion :
                    (ConversionFunc)binder.BindImplicitConversion;

                if (cnubSrc == 0)
                {
                    Debug.Assert(typeSrc == typeSrcBase);

                    // The null type can be implicitly converted to T? as the default value.
                    if (typeSrc.IsNullType())
                    {
                        // If we have the constant null, generate it as a default value of T?.  If we have 
                        // some crazy expression which has been determined to be always null, like (null??null)
                        // keep it in its expression form and transform it in the nullable rewrite pass.
                        if (needsExprDest)
                        {
                            if (exprSrc.isCONSTANT_OK())
                            {
                                exprDest = GetExprFactory().CreateZeroInit(nubDst);
                            }
                            else
                            {
                                exprDest = GetExprFactory().CreateCast(0x00, typeDest, exprSrc);
                            }
                        }
                        return true;
                    }

                    EXPR exprTmp = exprSrc;

                    // If there is an implicit/explicit S => T then there is an implicit/explicit S => T?
                    if (typeSrc == typeDstBase || pfn(exprSrc, typeSrc, exprTypeDstBase, nubDst, needsExprDest, out exprTmp, flags | CONVERTTYPE.NOUDC))
                    {
                        if (needsExprDest)
                        {
                            // UNDONE: This is a premature realization of the nullable conversion as 
                            // UNDONE: a constructor.  Rather than flagging this, can we simply emit it
                            // UNDONE: as a cast node and have the operator rewrite pass turn it into
                            // UNDONE: a constructor call?
                            EXPRUSERDEFINEDCONVERSION exprUDC = exprTmp.kind == ExpressionKind.EK_USERDEFINEDCONVERSION ? exprTmp.asUSERDEFINEDCONVERSION() : null;
                            if (exprUDC != null)
                            {
                                exprTmp = exprUDC.UserDefinedCall;
                            }

                            // This logic is left over from the days when T?? was legal. However there are error/LAF cases that necessitates the loop.
                            // typeSrc is not nullable so just wrap the required number of times. For legal code (cnubDst <= 0).

                            for (int i = 0; i < cnubDst; i++)
                            {
                                exprTmp = binder.BindNubNew(exprTmp);
                                exprTmp.asCALL().nubLiftKind = NullableCallLiftKind.NullableConversionConstructor;
                            }
                            if (exprUDC != null)
                            {
                                exprUDC.UserDefinedCall = exprTmp;
                                exprUDC.setType((CType)exprTmp.type);
                                exprTmp = exprUDC;
                            }
                            Debug.Assert(exprTmp.type == nubDst);
                            exprDest = exprTmp;
                        }
                        return true;
                    }

                    // No builtin conversion. Maybe there is a user defined conversion....
                    return 0 == (flags & CONVERTTYPE.NOUDC) && binder.bindUserDefinedConversion(exprSrc, typeSrc, nubDst, needsExprDest, out exprDest, 0 == (flags & CONVERTTYPE.ISEXPLICIT));
                }

                // Both are Nullable so there is only a conversion if there is a conversion between the base types.
                // That is, if there is an implicit/explicit S => T then there is an implicit/explicit S?+ => T?+.
                if (typeSrcBase != typeDstBase && !pfn(null, typeSrcBase, exprTypeDstBase, nubDst, false, out exprDest, flags | CONVERTTYPE.NOUDC))
                {
                    // No builtin conversion. Maybe there is a user defined conversion....
                    return 0 == (flags & CONVERTTYPE.NOUDC) && binder.bindUserDefinedConversion(exprSrc, typeSrc, nubDst, needsExprDest, out exprDest, 0 == (flags & CONVERTTYPE.ISEXPLICIT));
                }

                if (needsExprDest)
                {
                    MethWithInst mwi = new MethWithInst(null, null);
                    EXPRMEMGRP pMemGroup = GetExprFactory().CreateMemGroup(null, mwi);
                    EXPRCALL exprDst = GetExprFactory().CreateCall(0, nubDst, exprSrc, pMemGroup, null);

                    // Here we want to first check whether or not the conversions work on the base types.

                    EXPR arg1 = binder.mustCast(exprSrc, typeSrcBase);
                    EXPRCLASS arg2 = GetExprFactory().MakeClass(typeDstBase);

                    bool convertible;
                    if (0 != (flags & CONVERTTYPE.ISEXPLICIT))
                    {
                        convertible = binder.BindExplicitConversion(arg1, arg1.type, arg2, typeDstBase, out arg1, flags | CONVERTTYPE.NOUDC);
                    }
                    else
                    {
                        convertible = binder.BindImplicitConversion(arg1, arg1.type, arg2, typeDstBase, out arg1, flags | CONVERTTYPE.NOUDC);
                    }
                    if (!convertible)
                    {
                        VSFAIL("bind(Im|Ex)plicitConversion failed unexpectedly");
                        return false;
                    }

                    exprDst.castOfNonLiftedResultToLiftedType = binder.mustCast(arg1, nubDst, 0);
                    exprDst.nubLiftKind = NullableCallLiftKind.NullableConversion;
                    exprDst.pConversions = exprDst.castOfNonLiftedResultToLiftedType;
                    exprDest = exprDst;
                }

                return true;
            }

            private bool bindImplicitConversionFromNull()
            {
                // null type can be implicitly converted to any reference type or pointer type or type
                // variable with reference-type constraint.

                FUNDTYPE ftDest = typeDest.fundType();
                if (ftDest != FUNDTYPE.FT_REF && ftDest != FUNDTYPE.FT_PTR &&
                    (ftDest != FUNDTYPE.FT_VAR || !typeDest.AsTypeParameterType().IsReferenceType()) &&
                    // null is convertible to System.Nullable<T>.
                    !typeDest.isPredefType(PredefinedType.PT_G_OPTIONAL))
                {
                    return false;
                }
                if (needsExprDest)
                {
                    // If the conversion argument is a constant null then return a ZEROINIT.   
                    // Otherwise, bind this as a cast to the destination type. In a later 
                    // rewrite pass we will rewrite the cast as SEQ(side effects, ZEROINIT).
                    if (exprSrc.isCONSTANT_OK())
                    {
                        exprDest = GetExprFactory().CreateZeroInit(typeDest);
                    }
                    else
                    {
                        exprDest = GetExprFactory().CreateCast(0x00, typeDest, exprSrc);
                    }
                }
                return true;
            }

            private bool bindImplicitConversionFromNullable(NullableType nubSrc)
            {
                // We can convert T? using a boxing conversion, we can convert it to ValueType, and
                // we can convert it to any interface implemented by T.
                //    
                // 13.1.5 Boxing Conversions
                //
                // A nullable-type has a boxing conversion to the same set of types to which the nullable-type's 
                // underlying type has boxing conversions. A boxing conversion applied to a value of a nullable-type
                // proceeds as follows:
                //
                // *   If the HasValue property of the nullable value evaluates to false, then the result of the
                //     boxing conversion is the null reference of the appropriate type.
                //
                // Otherwise, the result is obtained by boxing the result of evaluating the Value property on
                // the nullable value.

                AggregateType atsNub = nubSrc.GetAts(GetErrorContext());
                if (atsNub == null)
                {
                    return false;
                }
                if (atsNub == typeDest)
                {
                    // REVIEW : Should this ASSERT?
                    if (needsExprDest)
                    {
                        exprDest = exprSrc;
                    }
                    return true;
                }
                if (GetSymbolLoader().HasBaseConversion(nubSrc.GetUnderlyingType(), typeDest) && !CConversions.FUnwrappingConv(nubSrc, typeDest))
                {
                    if (needsExprDest)
                    {
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_BOX);
                        if (!typeDest.isPredefType(PredefinedType.PT_OBJECT))
                        {

                            // DevDiv Bugs 125348: The base type of a nullable is always a non-nullable value type, 
                            // therefore so is typeDest unless typeDest is PT_OBJECT. In this case the conversion 
                            // needs to be unboxed. We only need this if we actually will use the result. 
                            binder.bindSimpleCast(exprDest, exprTypeDest, out exprDest, EXPRFLAG.EXF_FORCE_UNBOX);
                        }
                    }
                    return true;
                }
                return 0 == (flags & CONVERTTYPE.NOUDC) && binder.bindUserDefinedConversion(exprSrc, nubSrc, typeDest, needsExprDest, out exprDest, true);
            }

            private bool bindImplicitConversionFromArray()
            {
                // 13.1.4 
                // 
                // The implicit reference conversions are:
                // 
                // *   From an array-type S with an element type SE to an array-type T with an element 
                //     type TE, provided all of the following are true:
                //     *   S and T differ only in element type. In other words, S and T have the same number of dimensions.
                //     *   An implicit reference conversion exists from SE to TE.
                // *   From a one-dimensional array-type S[] to System.Collections.Generic.IList<S>, 
                //     System.Collections.Generic.IReadOnlyList<S> and their base interfaces 
                // *   From a one-dimensional array-type S[] to System.Collections.Generic.IList<T>, System.Collections.Generic.IReadOnlyList<T>
                //     and their base interfaces, provided there is an implicit reference conversion from S to T.
                // *   From any array-type to System.Array.
                // *   From any array-type to any interface implemented by System.Array.

                if (!GetSymbolLoader().HasBaseConversion(typeSrc, typeDest))
                {
                    return false;
                }

                EXPRFLAG grfex = 0;
                // The above if checks for dest==Array, object or an interface the array implements,
                // including IList<T>, ICollection<T>, IEnumerable<T>, IReadOnlyList<T>, IReadOnlyCollection<T>
                // and the non-generic versions.
                // REVIEW : Determine when we need EXF_REFCHECK!

                if ((typeDest.IsArrayType() ||
                     (typeDest.isInterfaceType() &&
                      typeDest.AsAggregateType().GetTypeArgsAll().Size == 1 &&
                      ((typeDest.AsAggregateType().GetTypeArgsAll().Item(0) != typeSrc.AsArrayType().GetElementType()) ||
                       0 != (flags & CONVERTTYPE.FORCECAST))))
                    &&
                    (0 != (flags & CONVERTTYPE.FORCECAST) ||
                     TypeManager.TypeContainsTyVars(typeSrc, null) ||
                     TypeManager.TypeContainsTyVars(typeDest, null)))
                {
                    grfex = EXPRFLAG.EXF_REFCHECK;
                }
                if (needsExprDest)
                {
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, grfex);
                }
                return true;
            }

            private bool bindImplicitConversionFromPointer()
            {

                // 27.4 Pointer conversions
                //
                // In an unsafe context, the set of available implicit conversions (13.1) is extended to include
                // the following implicit pointer conversions:
                //
                // * From any pointer-type to the type void*.

                if (typeDest.IsPointerType() && typeDest.AsPointerType().GetReferentType() == binder.getVoidType())
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                    return true;
                }
                return false;
            }

            private bool bindImplicitConversionFromAgg(AggregateType aggTypeSrc)
            {
                // GENERICS: The case for constructed types is very similar to types with
                // no parameters. The parameters are irrelevant for most of the conversions
                // below. They could be relevant if we had user-defined conversions on
                // generic types.

                AggregateSymbol aggSrc = aggTypeSrc.getAggregate();
                if (aggSrc.IsEnum())
                {
                    return bindImplicitConversionFromEnum(aggTypeSrc);
                }

                if (typeDest.isEnumType())
                {
                    if (bindImplicitConversionToEnum(aggTypeSrc))
                    {
                        return true;
                    }
                    // Even though enum is sealed, a class can derive from enum in LAF scenarios -- 
                    // continue testing for derived to base conversions below.
                }
                else if (aggSrc.getThisType().isSimpleType() && typeDest.isSimpleType())
                {
                    if (bindImplicitConversionBetweenSimpleTypes(aggTypeSrc))
                    {
                        return true;
                    }
                    // No simple conversion -- continue testing for derived to base conversions below.
                }

                return bindImplicitConversionToBase(aggTypeSrc);
            }

            private bool bindImplicitConversionToBase(AggregateType pSource)
            {
                // 13.1.4 Implicit reference conversions
                // 
                // *   From any reference-type to object.
                // *   From any class-type S to any class-type T, provided S is derived from T.
                // *   From any class-type S to any interface-type T, provided S implements T.
                // *   From any interface-type S to any interface-type T, provided S is derived from T.
                // *   From any delegate-type to System.Delegate.
                // *   From any delegate-type to System.ICloneable.

                if (!typeDest.IsAggregateType() || !GetSymbolLoader().HasBaseConversion(pSource, typeDest))
                {
                    return false;
                }
                EXPRFLAG flags = 0x00;
                if (pSource.getAggregate().IsStruct() && typeDest.fundType() == FUNDTYPE.FT_REF)
                {
                    flags = EXPRFLAG.EXF_BOX | EXPRFLAG.EXF_CANTBENULL;
                }
                else if (exprSrc != null)
                {
                    flags = exprSrc.flags & EXPRFLAG.EXF_CANTBENULL;
                }
                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, flags);
                return true;
            }

            private bool bindImplicitConversionFromEnum(AggregateType aggTypeSrc)
            {

                // 13.1.5 Boxing conversions
                // 
                // A boxing conversion permits any non-nullable-value-type to be implicitly converted to the type
                // object or System.ValueType or to any interface-type implemented by the value-type, and any enum
                // type to be implicitly converted to System.Enum as well. Boxing a value of a 
                // non-nullable-value-type consists of allocating an object instance and copying the value-type 
                // value into that instance. An enum can be boxed to the type System.Enum, since that is the direct
                // base class for all enums (21.4). A struct or enum can be boxed to the type System.ValueType, 
                // since that is the direct base class for all structs (18.3.2) and a base class for all enums.

                if (typeDest.IsAggregateType() && GetSymbolLoader().HasBaseConversion(aggTypeSrc, typeDest.AsAggregateType()))
                {
                    if (needsExprDest)
                        binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_BOX | EXPRFLAG.EXF_CANTBENULL);
                    return true;
                }
                return false;
            }

            private bool bindImplicitConversionToEnum(AggregateType aggTypeSrc)
            {
                // The spec states:
                // *****************
                // 13.1.3 Implicit enumeration conversions
                //
                // An implicit enumeration conversion permits the decimal-integer-literal 0 to be converted to any
                // enum-type.
                // *****************
                // However, we actually allow any constant zero, not just the integer literal zero, to be converted
                // to enum.  The reason for this is for backwards compatibility with a premature optimization
                // that used to be in the binding layer.  We would optimize away expressions such as 0 | blah to be
                // just 0, but not erase the "is literal" bit.  This meant that expression such as 0 | 0 | E.X
                // would succeed rather than correctly producing an error pointing out that 0 | 0 is not a literal
                // zero and therefore does not convert to any enum.
                //
                // We have removed the premature optimization but want old code to continue to compile. Rather than
                // try to emulate the somewhat complex behaviour of the previous optimizer, it is easier to simply
                // say that any compile time constant zero is convertible to any enum.  This means unfortunately
                // expressions such as (7-7) * 12 are convertible to enum, but frankly, that's better than having
                // some terribly complex rule about what constitutes a legal zero and what doesn't.

                // Note: Don't use GetConst here since the conversion only applies to bona-fide compile time constants.
                if (
                    aggTypeSrc.getAggregate().GetPredefType() != PredefinedType.PT_BOOL &&
                    exprSrc != null &&
                    exprSrc.isZero() &&
                    exprSrc.type.isNumericType() &&
                    /*(exprSrc.flags & EXF_LITERALCONST) &&*/
                    0 == (flags & CONVERTTYPE.STANDARD))
                {
                    // NOTE: This allows conversions from uint, long, ulong, float, double, and hexadecimal int
                    // NOTE: This is for backwards compatibility with Everett

                    // REVIEW: : This is another place where we lose EXPR fidelity. We shouldn't fold this
                    // into a constant here - we should move this to a later pass.
                    if (needsExprDest)
                    {
                        exprDest = GetExprFactory().CreateConstant(typeDest, ConstValFactory.GetDefaultValue(typeDest.constValKind()));
                    }
                    return true;
                }
                return false;
            }

            private bool bindImplicitConversionBetweenSimpleTypes(AggregateType aggTypeSrc)
            {
                AggregateSymbol aggSrc = aggTypeSrc.getAggregate();
                Debug.Assert(aggSrc.getThisType().isSimpleType());
                Debug.Assert(typeDest.isSimpleType());

                Debug.Assert(aggSrc.IsPredefined() && typeDest.isPredefined());
                PredefinedType ptSrc = aggSrc.GetPredefType();
                PredefinedType ptDest = typeDest.getPredefType();
                ConvKind convertKind;
                bool fConstShrinkCast = false;

                Debug.Assert((int)ptSrc < NUM_SIMPLE_TYPES && (int)ptDest < NUM_SIMPLE_TYPES);

                // 13.1.7 Implicit constant expression conversions
                // 
                // An implicit constant expression conversion permits the following conversions:
                // *   A constant-expression (14.16) of type int can be converted to type sbyte,  byte,  short,  
                //     ushort,  uint, or ulong, provided the value of the constant-expression is within the range 
                //     of the destination type.
                // *   A constant-expression of type long can be converted to type ulong, provided the value of
                //     the constant-expression is not negative.
                // Note: Don't use GetConst here since the conversion only applies to bona-fide compile time constants.
                if (exprSrc != null && exprSrc.isCONSTANT_OK() &&
                    ((ptSrc == PredefinedType.PT_INT && ptDest != PredefinedType.PT_BOOL && ptDest != PredefinedType.PT_CHAR) ||
                    (ptSrc == PredefinedType.PT_LONG && ptDest == PredefinedType.PT_ULONG)) &&
                    isConstantInRange(exprSrc.asCONSTANT(), typeDest))
                {
                    // Special case (CLR 6.1.6): if integral constant is in range, the conversion is a legal implicit conversion.
                    convertKind = ConvKind.Implicit;
                    fConstShrinkCast = needsExprDest && (GetConvKind(ptSrc, ptDest) != ConvKind.Implicit);
                }
                else if (ptSrc == ptDest)
                {
                    // Special case: precision limiting casts to float or double
                    Debug.Assert(ptSrc == PredefinedType.PT_FLOAT || ptSrc == PredefinedType.PT_DOUBLE);
                    Debug.Assert(0 != (flags & CONVERTTYPE.ISEXPLICIT));
                    convertKind = ConvKind.Implicit;
                }
                else
                {
                    convertKind = GetConvKind(ptSrc, ptDest);
                    Debug.Assert(convertKind != ConvKind.Identity);
                    // identity conversion should have been handled at first.
                }

                if (convertKind != ConvKind.Implicit)
                {
                    return false;
                }

                // An implicit conversion exists. Do the conversion.
                if (exprSrc.GetConst() != null)
                {
                    // Fold the constant cast if possible.
                    ConstCastResult result = binder.bindConstantCast(exprSrc, exprTypeDest, needsExprDest, out exprDest, false);
                    if (result == ConstCastResult.Success)
                    {
                        return true;  // else, don't fold and use a regular cast, below.
                    }
                    // REVIEW: I don't think this can ever be hit.  If exprSrc is a constant then
                    // it's either a floating point number (which always succeeds), it's a numeric implicit
                    // conversion (which always succeeds), or it's a constant numeric conversion (which
                    // has already been checked by isConstantInRange, so should always succeed)
                }

                if (isUserDefinedConversion(ptSrc, ptDest))
                {
                    if (!needsExprDest)
                    {
                        return true;
                    }
                    // According the language, this is a standard conversion, but it is implemented
                    // through a user-defined conversion. Because it's a standard conversion, we don't
                    // test the NOUDC flag here.
                    return binder.bindUserDefinedConversion(exprSrc, aggTypeSrc, typeDest, needsExprDest, out exprDest, true);
                }
                if (needsExprDest)
                    binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest);
                return true;
            }

            private bool bindImplicitConversionFromTypeVar(TypeParameterType tyVarSrc)
            {
                // 13.1.4
                // 
                // For a type-parameter T that is known to be a reference type (25.7), the following implicit
                // reference conversions exist:
                // 
                // *   From T to its effective base class C, from T to any base class of C, and from T to any 
                //     interface implemented by C.
                // *   From T to an interface-type I in T's effective interface set and from T to any base 
                //     interface of I.
                // *   From T to a type parameter U provided that T depends on U (25.7). [Note: Since T is known
                //     to be a reference type, within the scope of T, the run-time type of U will always be a 
                //     reference type, even if U is not known to be a reference type at compile-time.]
                // *   From the null type (11.2.7) to T.
                //
                // 13.1.5
                //
                // For a type-parameter T that is not known to be a reference type (25.7), the following conversions
                // involving T are considered to be boxing conversions at compile-time. At run-time, if T is a value
                // type, the conversion is executed as a boxing conversion. At run-time, if T is a reference type,
                // the conversion is executed as an implicit reference conversion or identity conversion.
                // 
                // *   From T to its effective base class C, from T to any base class of C, and from T to any 
                //     interface implemented by C. [Note: C will be one of the types System.Object, System.ValueType,
                //     or System.Enum (otherwise T would be known to be a reference type and 13.1.4 would apply
                //     instead of this clause).]
                // *   From T to an interface-type I in T's effective interface set and from T to any base
                //     interface of I.
                //
                // 13.1.6 Implicit type parameter conversions
                // 
                // This clause details implicit conversions involving type parameters that are not classified as 
                // implicit reference conversions or implicit boxing conversions.
                // 
                // For a type-parameter T that is not known to be a reference type, there is an implicit conversion 
                // from T to a type parameter U provided T depends on U. At run-time, if T is a value type and U is
                // a reference type, the conversion is executed as a boxing conversion. At run-time, if both T and U
                // are value types, then T and U are necessarily the same type and no conversion is performed. At 
                // run-time, if T is a reference type, then U is necessarily also a reference type and the conversion
                // is executed as an implicit reference conversion or identity conversion (25.7).

                CType typeTmp = tyVarSrc.GetEffectiveBaseClass();
                TypeArray bnds = tyVarSrc.GetBounds();
                int itype = -1;
                for (; ; )
                {
                    if (binder.canConvert(typeTmp, typeDest, flags | CONVERTTYPE.NOUDC))
                    {
                        if (!needsExprDest)
                        {
                            return true;
                        }
                        if (typeDest.IsTypeParameterType())
                        {
                            // For a type var destination we need to cast to object then to the other type var.
                            EXPR exprT;
                            EXPRCLASS exprObj = GetExprFactory().MakeClass(binder.GetReqPDT(PredefinedType.PT_OBJECT));
                            binder.bindSimpleCast(exprSrc, exprObj, out exprT, EXPRFLAG.EXF_FORCE_BOX);
                            binder.bindSimpleCast(exprT, exprTypeDest, out exprDest, EXPRFLAG.EXF_FORCE_UNBOX);
                        }
                        else
                        {
                            binder.bindSimpleCast(exprSrc, exprTypeDest, out exprDest, EXPRFLAG.EXF_FORCE_BOX);
                        }
                        return true;
                    }
                    do
                    {
                        if (++itype >= bnds.Size)
                        {
                            return false;
                        }
                        typeTmp = bnds.Item(itype);
                    }
                    while (!typeTmp.isInterfaceType() && !typeTmp.IsTypeParameterType());
                }
            }
            private SymbolLoader GetSymbolLoader()
            {
                return binder.GetSymbolLoader();
            }
            private ExprFactory GetExprFactory()
            {
                return binder.GetExprFactory();
            }
            private ErrorHandling GetErrorContext()
            {
                return binder.GetErrorContext();
            }
        }
    }
}
