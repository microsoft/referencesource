// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Errors;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    // This class takes an EXPRMEMGRP and a set of arguments and binds the arguments
    // to the best applicable method in the group.
    // ----------------------------------------------------------------------------

    internal partial class ExpressionBinder
    {
        internal class GroupToArgsBinder
        {
            private enum Result
            {
                Success,
                Failure_SearchForExpanded,
                Failure_NoSearchForExpanded
            }

            private ExpressionBinder m_pExprBinder;
            private bool m_fCandidatesUnsupported;
            private BindingFlag m_fBindFlags;
            private EXPRMEMGRP m_pGroup;
            private ArgInfos m_pArguments;
            private ArgInfos m_pOriginalArguments;
            private bool m_bHasNamedArguments;
            private AggregateType m_pDelegate;
            private AggregateType m_pCurrentType;
            private MethodOrPropertySymbol m_pCurrentSym;
            private TypeArray m_pCurrentTypeArgs;
            private TypeArray m_pCurrentParameters;
            private TypeArray m_pBestParameters;
            private int m_nArgBest;
            // Keep track of the first 20 or so syms with the wrong arg count.
            private SymWithType[] m_swtWrongCount = new SymWithType[20];
            private int m_nWrongCount;
            private bool m_bIterateToEndOfNsList;               // we have found an appliacable extension method only itereate to 
            // end of current namespaces extension method list
            private bool m_bBindingCollectionAddArgs;           // Report parameter modifiers as error 
            private GroupToArgsBinderResult m_results;
            private List<CandidateFunctionMember> m_methList;
            private MethPropWithInst m_mpwiParamTypeConstraints;
            private MethPropWithInst m_mpwiBogus;
            private MethPropWithInst m_mpwiCantInferInstArg;
            private MethWithType m_mwtBadArity;
            private Name m_pInvalidSpecifiedName;
            private Name m_pNameUsedInPositionalArgument;
            private Name m_pDuplicateSpecifiedName;
            // When we find a type with an interface, then we want to mark all other interfaces that it 
            // implements as being hidden. We also want to mark object as being hidden. So stick them
            // all in this list, and then for subsequent types, if they're in this list, then we
            // ignore them.
            private List<CType> m_HiddenTypes;
            private bool m_bArgumentsChangedForNamedOrOptionalArguments;

            public GroupToArgsBinder(ExpressionBinder exprBinder, BindingFlag bindFlags, EXPRMEMGRP grp, ArgInfos args, ArgInfos originalArgs, bool bHasNamedArguments, AggregateType atsDelegate)
            {
                Debug.Assert(grp != null);
                Debug.Assert(exprBinder != null);
                Debug.Assert(args != null);

                m_pExprBinder = exprBinder;
                m_fCandidatesUnsupported = false;
                m_fBindFlags = bindFlags;
                m_pGroup = grp;
                m_pArguments = args;
                m_pOriginalArguments = originalArgs;
                m_bHasNamedArguments = bHasNamedArguments;
                m_pDelegate = atsDelegate;
                m_pCurrentType = null;
                m_pCurrentSym = null;
                m_pCurrentTypeArgs = null;
                m_pCurrentParameters = null;
                m_pBestParameters = null;
                m_nArgBest = -1;
                m_nWrongCount = 0;
                m_bIterateToEndOfNsList = false;
                m_bBindingCollectionAddArgs = false;
                m_results = new GroupToArgsBinderResult();
                m_methList = new List<CandidateFunctionMember>();
                m_mpwiParamTypeConstraints = new MethPropWithInst();
                m_mpwiBogus = new MethPropWithInst();
                m_mpwiCantInferInstArg = new MethPropWithInst();
                m_mwtBadArity = new MethWithType();
                m_HiddenTypes = new List<CType>();
            }

            // ----------------------------------------------------------------------------
            // This method does the actual binding.
            // ----------------------------------------------------------------------------

            public bool Bind(bool bReportErrors)
            {
                Debug.Assert(m_pGroup.sk == SYMKIND.SK_MethodSymbol || m_pGroup.sk == SYMKIND.SK_PropertySymbol && 0 != (m_pGroup.flags & EXPRFLAG.EXF_INDEXER));

                // We need the EXPRs for error reporting for non-delegates
                Debug.Assert(m_pDelegate != null || m_pArguments.fHasExprs);

                LookForCandidates();
                if (!GetResultOfBind(bReportErrors))
                {
                    if (bReportErrors)
                    {
                        ReportErrorsOnFailure();
                    }
                    return false;
                }
                return true;
            }

            public GroupToArgsBinderResult GetResultsOfBind()
            {
                return m_results;
            }

            public bool BindCollectionAddArgs()
            {
                m_bBindingCollectionAddArgs = true;
                return Bind(true /* bReportErrors */);
            }
            private SymbolLoader GetSymbolLoader()
            {
                return m_pExprBinder.GetSymbolLoader();
            }
            private CSemanticChecker GetSemanticChecker()
            {
                return m_pExprBinder.GetSemanticChecker();
            }
            private ErrorHandling GetErrorContext()
            {
                return m_pExprBinder.GetErrorContext();
            }
            public static CType GetTypeQualifier(EXPRMEMGRP pGroup)
            {
                Debug.Assert(pGroup != null);

                CType rval = null;

                if (0 != (pGroup.flags & EXPRFLAG.EXF_BASECALL))
                {
                    rval = null;
                }
                else if (0 != (pGroup.flags & EXPRFLAG.EXF_CTOR))
                {
                    rval = pGroup.GetParentType();
                }
                else if (pGroup.GetOptionalObject() != null)
                {
                    rval = pGroup.GetOptionalObject().type;
                }
                else
                {
                    rval = null;
                }
                return rval;
            }

            private void LookForCandidates()
            {
                bool fExpanded = false;
                bool bSearchForExpanded = true;
                int cswtMaxWrongCount = m_swtWrongCount.Length;
                bool allCandidatesUnsupported = true;
                bool lookedAtCandidates = false;

                // Calculate the mask based on the type of the sym we've found so far.  This
                // is to ensure that if we found a propsym (or methsym, or whatever) the 
                // iterator will only return propsyms (or methsyms, or whatever)
                symbmask_t mask = (symbmask_t)(1 << (int)m_pGroup.sk);

                CType pTypeThrough = m_pGroup.GetOptionalObject() != null ? m_pGroup.GetOptionalObject().type : null;
                CMemberLookupResults.CMethodIterator iterator = m_pGroup.GetMemberLookupResults().GetMethodIterator(GetSemanticChecker(), GetSymbolLoader(), pTypeThrough, GetTypeQualifier(m_pGroup), m_pExprBinder.ContextForMemberLookup(), true, // AllowBogusAndInaccessible
                    false, m_pGroup.typeArgs.size, m_pGroup.flags, mask);
                while (true)
                {
                    bool bFoundExpanded;
                    Result currentTypeArgsResult;

                    bFoundExpanded = false;
                    if (bSearchForExpanded && !fExpanded)
                    {
                        bFoundExpanded = fExpanded = ConstructExpandedParameters();
                    }

                    // Get the next sym to search for.
                    if (!bFoundExpanded)
                    {
                        fExpanded = false;

                        if (!GetNextSym(iterator))
                        {
                            break;
                        }

                        // Get the parameters.
                        m_pCurrentParameters = m_pCurrentSym.Params;
                        bSearchForExpanded = true;
                    }

                    if (m_bArgumentsChangedForNamedOrOptionalArguments)
                    {
                        // If we changed them last time, then we need to reset them.
                        m_bArgumentsChangedForNamedOrOptionalArguments = false;
                        CopyArgInfos(m_pOriginalArguments, m_pArguments);
                    }

                    // If we have named arguments, reorder them for this method.
                    if (m_pArguments.fHasExprs)
                    {
                        // If we dont have EXPRs, its because we're doing a method group conversion.
                        // In those scenarios, we never want to add named arguments or optional arguments.
                        if (m_bHasNamedArguments)
                        {
                            if (!ReOrderArgsForNamedArguments())
                            {
                                continue;
                            }
                        }
                        else if (HasOptionalParameters())
                        {
                            if (!AddArgumentsForOptionalParameters())
                            {
                                continue;
                            }
                        }
                    }

                    if (!bFoundExpanded)
                    {
                        lookedAtCandidates = true;
                        allCandidatesUnsupported &= m_pCurrentSym.getBogus();

                        // If we have the wrong number of arguments and still have room in our cache of 20 (: this needs
                        // to get fixed... why 20?), then store it in our cache and go to the next sym.
                        if (m_pCurrentParameters.size != m_pArguments.carg)
                        {
                            if (m_nWrongCount < cswtMaxWrongCount &&
                                    (!m_pCurrentSym.isParamArray || m_pArguments.carg < m_pCurrentParameters.size - 1))
                            {
                                m_swtWrongCount[m_nWrongCount++] = new SymWithType(m_pCurrentSym, m_pCurrentType);
                            }
                            bSearchForExpanded = true;
                            continue;
                        }
                    }

                    // If we cant use the current symbol, then we've filtered it, so get the next one.

                    if (!iterator.CanUseCurrentSymbol())
                    {
                        continue;
                    }

                    // Get the current type args.
                    currentTypeArgsResult = DetermineCurrentTypeArgs();
                    if (currentTypeArgsResult != Result.Success)
                    {
                        bSearchForExpanded = (currentTypeArgsResult == Result.Failure_SearchForExpanded);
                        continue;
                    }

                    // Check access.
                    bool fCanAccess = !iterator.IsCurrentSymbolInaccessible();
                    if (!fCanAccess && (!m_methList.IsEmpty() || m_results.GetInaccessibleResult()))
                    {
                        // We'll never use this one for error reporting anyway, so just skip it.
                        bSearchForExpanded = false;
                        continue;
                    }

                    // Check bogus.
                    bool fBogus = fCanAccess && iterator.IsCurrentSymbolBogus();
                    if (fBogus && (!m_methList.IsEmpty() || m_results.GetInaccessibleResult() || m_mpwiBogus))
                    {
                        // We'll never use this one for error reporting anyway, so just skip it.
                        bSearchForExpanded = false;
                        continue;
                    }

                    // Check convertibility of arguments.
                    if (!ArgumentsAreConvertible())
                    {
                        bSearchForExpanded = true;
                        continue;
                    }

                    // We know we have the right number of arguments and they are all convertible.
                    if (!fCanAccess)
                    {
                        // In case we never get an accessible method, this will allow us to give
                        // a better error...
                        Debug.Assert(!m_results.GetInaccessibleResult());
                        m_results.GetInaccessibleResult().Set(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs);
                    }
                    else if (fBogus)
                    {
                        // In case we never get a good method, this will allow us to give
                        // a better error...
                        Debug.Assert(!m_mpwiBogus);
                        m_mpwiBogus.Set(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs);
                    }
                    else
                    {
                        // This is a plausible method / property to call.
                        // Link it in at the end of the list.
                        m_methList.Add(new CandidateFunctionMember(
                                    new MethPropWithInst(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs),
                                    m_pCurrentParameters,
                                    0,
                                    fExpanded));

                        // When we find a method, we check if the type has interfaces. If so, mark the other interfaces
                        // as hidden, and object as well.

                        if (m_pCurrentType.isInterfaceType())
                        {
                            TypeArray ifaces = m_pCurrentType.GetIfacesAll();
                            for (int i = 0; i < ifaces.size; i++)
                            {
                                AggregateType type = ifaces.Item(i).AsAggregateType();

                                Debug.Assert(type.isInterfaceType());
                                m_HiddenTypes.Add(type);
                            }

                            // Mark object.
                            AggregateType typeObject = GetSymbolLoader().GetReqPredefType(PredefinedType.PT_OBJECT, true);
                            m_HiddenTypes.Add(typeObject);
                        }
                    }

                    // Don't look at the expanded form.
                    bSearchForExpanded = false;
                }
                m_fCandidatesUnsupported = allCandidatesUnsupported && lookedAtCandidates;

                // Restore the arguments to their original state if we changed them for named/optional arguments.
                // ILGen will take care of putting the real arguments in there.
                if (m_bArgumentsChangedForNamedOrOptionalArguments)
                {
                    // If we changed them last time, then we need to reset them.
                    CopyArgInfos(m_pOriginalArguments, m_pArguments);
                }
            }

            private void CopyArgInfos(ArgInfos src, ArgInfos dst)
            {
                dst.carg = src.carg;
                dst.types = src.types;
                dst.fHasExprs = src.fHasExprs;

                dst.prgexpr.Clear();
                for (int i = 0; i < src.prgexpr.Count; i++)
                {
                    dst.prgexpr.Add(src.prgexpr[i]);
                }
            }

            private bool GetResultOfBind(bool bReportErrors)
            {
                // We looked at all the evidence, and we come to render the verdict:
                CandidateFunctionMember pmethBest;

                if (!m_methList.IsEmpty())
                {
                    if (m_methList.Count == 1)
                    {
                        // We found the single best method to call.
                        pmethBest = m_methList.Head();
                    }
                    else
                    {
                        // We have some ambiguities, lets sort them out.
                        CandidateFunctionMember pAmbig1 = null;
                        CandidateFunctionMember pAmbig2 = null;

                        CType pTypeThrough = m_pGroup.GetOptionalObject() != null ? m_pGroup.GetOptionalObject().type : null;
                        pmethBest = m_pExprBinder.FindBestMethod(m_methList, pTypeThrough, m_pArguments, out pAmbig1, out pAmbig2);

                        if (null == pmethBest)
                        {
                            // Arbitrarily use the first one, but make sure to report errors or give the ambiguous one
                            // back to the caller.
                            pmethBest = pAmbig1;
                            m_results.AmbiguousResult = pAmbig2.mpwi;

                            if (bReportErrors)
                            {
                                if (pAmbig1.@params != pAmbig2.@params ||
                                    pAmbig1.mpwi.MethProp().Params.size != pAmbig2.mpwi.MethProp().Params.size ||
                                    pAmbig1.mpwi.TypeArgs != pAmbig2.mpwi.TypeArgs ||
                                    pAmbig1.mpwi.GetType() != pAmbig2.mpwi.GetType() ||
                                    pAmbig1.mpwi.MethProp().Params == pAmbig2.mpwi.MethProp().Params)
                                {
                                    GetErrorContext().Error(ErrorCode.ERR_AmbigCall, pAmbig1.mpwi, pAmbig2.mpwi);
                                }
                                else
                                {
                                    // The two signatures are identical so don't use the type args in the error message.
                                    GetErrorContext().Error(ErrorCode.ERR_AmbigCall, pAmbig1.mpwi.MethProp(), pAmbig2.mpwi.MethProp());
                                }
                            }
                        }
                    }

                    // This is the "success" exit path.
                    Debug.Assert(pmethBest != null);
                    m_results.BestResult = pmethBest.mpwi;

                    // Record our best match in the memgroup as well. This is temporary.

                    if (bReportErrors)
                    {
                        ReportErrorsOnSuccess();
                    }
                    return true;
                }

                return false;
            }

            /////////////////////////////////////////////////////////////////////////////////
            // This method returns true if we're able to match arguments to their names.
            // If we either have too many arguments, or we cannot match their names, then
            // we return false. 
            //
            // Note that if we have not enough arguments, we still return true as long as
            // we can find matching parameters for each named arguments, and all parameters
            // that do not have a matching argument are optional parameters.

            private bool ReOrderArgsForNamedArguments()
            {
                // First we need to find the method that we're actually trying to call. 
                MethodOrPropertySymbol methprop = FindMostDerivedMethod(m_pCurrentSym, m_pGroup.GetOptionalObject());
                if (methprop == null)
                {
                    return false;
                }

                int numParameters = m_pCurrentParameters.size;

                // If we have no parameters, or fewer parameters than we have arguments, bail.
                if (numParameters == 0 || numParameters < m_pArguments.carg)
                {
                    return false;
                }

                // Make sure all the names we specified are in the list and we dont have duplicates.
                if (!NamedArgumentNamesAppearInParameterList(methprop))
                {
                    return false;
                }

                m_bArgumentsChangedForNamedOrOptionalArguments = ReOrderArgsForNamedArguments(
                        methprop,
                        m_pCurrentParameters,
                        m_pCurrentType,
                        m_pGroup,
                        m_pArguments,
                        m_pExprBinder.GetTypes(),
                        m_pExprBinder.GetExprFactory(),
                        GetSymbolLoader());
                return m_bArgumentsChangedForNamedOrOptionalArguments;
            }

            internal static bool ReOrderArgsForNamedArguments(
                    MethodOrPropertySymbol methprop,
                    TypeArray pCurrentParameters,
                    AggregateType pCurrentType,
                    EXPRMEMGRP pGroup,
                    ArgInfos pArguments,
                    TypeManager typeManager,
                    ExprFactory exprFactory,
                    SymbolLoader symbolLoader)
            {
                // We use the param count from pCurrentParameters because they may have been resized 
                // for param arrays.
                int numParameters = pCurrentParameters.size;

                EXPR[] pExprArguments = new EXPR[numParameters];

                // Now go through the parameters. First set all positional arguments in the new argument
                // set, then for the remainder, look for a named argument with a matching name.
                int index = 0;
                EXPR paramArrayArgument = null;
                TypeArray @params = typeManager.SubstTypeArray(
                    pCurrentParameters,
                    pCurrentType,
                    pGroup.typeArgs);
                foreach (Name name in methprop.ParameterNames)
                {
                    // This can happen if we had expanded our param array to size 0.
                    if (index >= pCurrentParameters.size)
                    {
                        break;
                    }

                    // If:
                    // (1) we have a param array method
                    // (2) we're on the last arg
                    // (3) the thing we have is an array init thats generated for param array
                    // then let us through.
                    if (methprop.isParamArray &&
                        index < pArguments.carg &&
                        pArguments.prgexpr[index].isARRINIT() && pArguments.prgexpr[index].asARRINIT().GeneratedForParamArray)
                    {
                        paramArrayArgument = pArguments.prgexpr[index];
                    }

                    // Positional.
                    if (index < pArguments.carg &&
                        !pArguments.prgexpr[index].isNamedArgumentSpecification() &&
                        !(pArguments.prgexpr[index].isARRINIT() && pArguments.prgexpr[index].asARRINIT().GeneratedForParamArray))
                    {
                        pExprArguments[index] = pArguments.prgexpr[index++];
                        continue;
                    }

                    // Look for names.
                    EXPR pNewArg = FindArgumentWithName(pArguments, name);
                    if (pNewArg == null)
                    {
                        if (methprop.IsParameterOptional(index))
                        {
                            pNewArg = GenerateOptionalArgument(symbolLoader, exprFactory, methprop, @params.Item(index), index);
                        }
                        else if (paramArrayArgument != null && index == methprop.Params.Count - 1)
                        {
                            // If we have a param array argument and we're on the last one, then use it.
                            pNewArg = paramArrayArgument;
                        }
                        else
                        {
                            // No name and no default value.
                            return false;
                        }
                    }
                    pExprArguments[index++] = pNewArg;
                }

                // Here we've found all the arguments, or have default values for them.
                CType[] prgTypes = new CType[pCurrentParameters.size];
                for (int i = 0; i < numParameters; i++)
                {
                    if (i < pArguments.prgexpr.Count)
                    {
                        pArguments.prgexpr[i] = pExprArguments[i];
                    }
                    else
                    {
                        pArguments.prgexpr.Add(pExprArguments[i]);
                    }
                    prgTypes[i] = pArguments.prgexpr[i].type;
                }
                pArguments.carg = pCurrentParameters.size;
                pArguments.types = symbolLoader.getBSymmgr().AllocParams(pCurrentParameters.size, prgTypes);
                return true;
            }

            /////////////////////////////////////////////////////////////////////////////////

            private static EXPR GenerateOptionalArgument(
                    SymbolLoader symbolLoader,
                    ExprFactory exprFactory,
                    MethodOrPropertySymbol methprop,
                    CType type,
                    int index)
            {
                CType pParamType = type;
                CType pRawParamType = type.IsNullableType() ? type.AsNullableType().GetUnderlyingType() : type;

                EXPR optionalArgument = null;
                if (methprop.HasDefaultParameterValue(index))
                {
                    CType pConstValType = methprop.GetDefaultParameterValueConstValType(index);
                    CONSTVAL cv = methprop.GetDefaultParameterValue(index);

                    if (pConstValType.isPredefType(PredefinedType.PT_DATETIME) &&
                        (pRawParamType.isPredefType(PredefinedType.PT_DATETIME) || pRawParamType.isPredefType(PredefinedType.PT_OBJECT) || pRawParamType.isPredefType(PredefinedType.PT_VALUE)))
                    {
                        // This is the specific case where we want to create a DateTime
                        // but the constval that stores it is a long.

                        AggregateType dateTimeType = symbolLoader.GetReqPredefType(PredefinedType.PT_DATETIME);
                        optionalArgument = exprFactory.CreateConstant(dateTimeType, new CONSTVAL(DateTime.FromBinary(cv.longVal)));
                    }
                    else if (pConstValType.isSimpleOrEnumOrString())
                    {
                        // In this case, the constval is a simple type (all the numerics, including
                        // decimal), or an enum or a string. This covers all the substantial values,
                        // and everything else that can be encoded is just null or default(something).

                        // For enum parameters, we create a constant of the enum type. For everything
                        // else, we create the appropriate constant.

                        if (pRawParamType.isEnumType() && pConstValType == pRawParamType.underlyingType())
                        {
                            optionalArgument = exprFactory.CreateConstant(pRawParamType, cv);
                        }
                        else
                        {
                            optionalArgument = exprFactory.CreateConstant(pConstValType, cv);
                        }
                    }
                    else if ((pParamType.IsRefType() || pParamType.IsNullableType()) && cv.IsNullRef())
                    {
                        // We have an "= null" default value with a reference type or a nullable type.

                        optionalArgument = exprFactory.CreateNull();
                    }
                    else
                    {
                        // We have a default value that is encoded as a nullref, and that nullref is
                        // interpreted as default(something). For instance, the pParamType could be
                        // a type parameter type or a non-simple value type.

                        optionalArgument = exprFactory.CreateZeroInit(pParamType);
                    }
                }
                else
                {
                    // There was no default parameter specified, so generally use default(T),
                    // except for some cases when the parameter type in metatdata is object.

                    if (pParamType.isPredefType(PredefinedType.PT_OBJECT))
                    {
                        if (methprop.MarshalAsObject(index))
                        {
                            // For [opt] parameters of type object, if we have marshal(iunknown),
                            // marshal(idispatch), or marshal(interface), then we emit a null.

                            optionalArgument = exprFactory.CreateNull();
                        }
#if !SILVERLIGHT
                        else if (methprop.IsDispatchConstantParameter(index)
                            || methprop.IsUnknownConstantParameter(index))
                        {
                            // Otherwise, if we have an [IUnknownConstant] or [IDispatchConstant], 
                            // then we emit the appropriate wrapper type constructed with a null

                            if (methprop.IsUnknownConstantParameter(index))
                            {
                                AggregateType unknownWrapperType = symbolLoader.GetOptPredefType(PredefinedType.PT_UNKNOWNWRAPPER);
                                optionalArgument = exprFactory.CreateConstant(unknownWrapperType, new CONSTVAL(new System.Runtime.InteropServices.UnknownWrapper(null)));
                            }
                            else
                            {
                                AggregateType dispatchWrapperType = symbolLoader.GetOptPredefType(PredefinedType.PT_DISPATCHWRAPPER);
                                optionalArgument = exprFactory.CreateConstant(dispatchWrapperType, new CONSTVAL(new System.Runtime.InteropServices.DispatchWrapper(null)));
                            }
                        }
#endif
                        else
                        {
                            // Otherwise, we generate Type.Missing

                            AggregateSymbol agg = symbolLoader.GetOptPredefAgg(PredefinedType.PT_MISSING);
                            Name name = symbolLoader.GetNameManager().GetPredefinedName(PredefinedName.PN_CAP_VALUE);
                            FieldSymbol field = symbolLoader.LookupAggMember(name, agg, symbmask_t.MASK_FieldSymbol).AsFieldSymbol();
                            FieldWithType fwt = new FieldWithType(field, agg.getThisType());
                            EXPRFIELD exprField = exprFactory.CreateField(0, agg.getThisType(), null, 0, fwt, null);

                            if (agg.getThisType() != type)
                            {
                                optionalArgument = exprFactory.CreateCast(0, type, exprField);
                            }
                            else
                            {
                                optionalArgument = exprField;
                            }
                        }
                    }
                    else
                    {
                        // Every type aside from object that doesn't have a default value gets
                        // its default value.

                        optionalArgument = exprFactory.CreateZeroInit(pParamType);
                    }
                }

                Debug.Assert(optionalArgument != null);
                optionalArgument.IsOptionalArgument = true;
                return optionalArgument;
            }

            /////////////////////////////////////////////////////////////////////////////////

            private MethodOrPropertySymbol FindMostDerivedMethod(
                    MethodOrPropertySymbol pMethProp,
                    EXPR pObject)
            {
                return FindMostDerivedMethod(GetSymbolLoader(), pMethProp, pObject != null ? pObject.type : null);
            }

            /////////////////////////////////////////////////////////////////////////////////

            public static MethodOrPropertySymbol FindMostDerivedMethod(
                    SymbolLoader symbolLoader,
                    MethodOrPropertySymbol pMethProp,
                    CType pType)
            {
                MethodSymbol method;
                bool bIsIndexer = false;

                if (pMethProp.IsMethodSymbol())
                {
                    method = pMethProp.AsMethodSymbol();
                }
                else
                {
                    PropertySymbol prop = pMethProp.AsPropertySymbol();
                    method = prop.methGet != null ? prop.methGet : prop.methSet;
                    if (method == null)
                    {
                        return null;
                    }
                    bIsIndexer = prop.isIndexer();
                }

                if (!method.isVirtual)
                {
                    return method;
                }

                if (pType == null)
                {
                    // This must be a static call.
                    return method;
                }

                // Now get the slot method.
                if (method.swtSlot != null && method.swtSlot.Meth() != null)
                {
                    method = method.swtSlot.Meth();
                }

                if (!pType.IsAggregateType())
                {
                    // Not something that can have overrides anyway.
                    return method;
                }

                for (AggregateSymbol pAggregate = pType.AsAggregateType().GetOwningAggregate();
                        pAggregate != null && pAggregate.GetBaseAgg() != null;
                        pAggregate = pAggregate.GetBaseAgg())
                {
                    for (MethodOrPropertySymbol meth = symbolLoader.LookupAggMember(method.name, pAggregate, symbmask_t.MASK_MethodSymbol | symbmask_t.MASK_PropertySymbol).AsMethodOrPropertySymbol();
                            meth != null;
                            meth = symbolLoader.LookupNextSym(meth, pAggregate, symbmask_t.MASK_MethodSymbol | symbmask_t.MASK_PropertySymbol).AsMethodOrPropertySymbol())
                    {
                        if (!meth.isOverride)
                        {
                            continue;
                        }
                        if (meth.swtSlot.Sym != null && meth.swtSlot.Sym == method)
                        {
                            if (bIsIndexer)
                            {
                                Debug.Assert(meth.IsMethodSymbol());
                                return meth.AsMethodSymbol().getProperty();
                            }
                            else
                            {
                                return meth;
                            }
                        }
                    }
                }

                // If we get here, it means we can have two cases: one is that we have 
                // a delegate. This is because the delegate invoke method is virtual and is 
                // an override, but we wont have the slots set up correctly, and will 
                // not find the base type in the inheritance hierarchy. The second is that
                // we're calling off of the base itself.
                Debug.Assert(method.parent.IsAggregateSymbol());
                return method;
            }


            /////////////////////////////////////////////////////////////////////////////////

            private bool HasOptionalParameters()
            {
                MethodOrPropertySymbol methprop = FindMostDerivedMethod(m_pCurrentSym, m_pGroup.GetOptionalObject());
                return methprop != null ? methprop.HasOptionalParameters() : false;
            }

            /////////////////////////////////////////////////////////////////////////////////
            // Returns true if we can either add enough optional parameters to make the 
            // argument list match, or if we dont need to at all.

            private bool AddArgumentsForOptionalParameters()
            {
                if (m_pCurrentParameters.size <= m_pArguments.carg)
                {
                    // If we have enough arguments, or too many, no need to add any optionals here.
                    return true;
                }

                // First we need to find the method that we're actually trying to call. 
                MethodOrPropertySymbol methprop = FindMostDerivedMethod(m_pCurrentSym, m_pGroup.GetOptionalObject());
                if (methprop == null)
                {
                    return false;
                }

                // If we're here, we know we're not in a named argument case. As such, we can
                // just generate defaults for every missing argument.
                int i = m_pArguments.carg;
                int index = 0;
                TypeArray @params = m_pExprBinder.GetTypes().SubstTypeArray(
                    m_pCurrentParameters,
                    m_pCurrentType,
                    m_pGroup.typeArgs);
                EXPR[] pArguments = new EXPR[m_pCurrentParameters.size - i];
                for (; i < @params.size; i++, index++)
                {
                    if (!methprop.IsParameterOptional(i))
                    {
                        // We dont have an optional here, but we need to fill it in.
                        return false;
                    }

                    pArguments[index] = GenerateOptionalArgument(GetSymbolLoader(), m_pExprBinder.GetExprFactory(), methprop, @params.Item(i), i);
                }

                // Success. Lets copy them in now.
                for (int n = 0; n < index; n++)
                {
                    m_pArguments.prgexpr.Add(pArguments[n]);
                }
                CType[] prgTypes = new CType[@params.size];
                for (int n = 0; n < @params.size; n++)
                {
                    prgTypes[n] = m_pArguments.prgexpr[n].type;
                }
                m_pArguments.types = GetSymbolLoader().getBSymmgr().AllocParams(@params.size, prgTypes);
                m_pArguments.carg = @params.size;
                m_bArgumentsChangedForNamedOrOptionalArguments = true;
                return true;
            }

            /////////////////////////////////////////////////////////////////////////////////

            private static EXPR FindArgumentWithName(ArgInfos pArguments, Name pName)
            {
                for (int i = 0; i < pArguments.carg; i++)
                {
                    if (pArguments.prgexpr[i].isNamedArgumentSpecification() &&
                            pArguments.prgexpr[i].asNamedArgumentSpecification().Name == pName)
                    {
                        return pArguments.prgexpr[i];
                    }
                }
                return null;
            }

            /////////////////////////////////////////////////////////////////////////////////

            private bool NamedArgumentNamesAppearInParameterList(
                    MethodOrPropertySymbol methprop)
            {
                // Keep track of the current position in the parameter list so that we can check
                // containment from this point onwards as well as complete containment. This is 
                // for error reporting. The user cannot specify a named argument for a parameter
                // that has a fixed argument value.
                List<Name> currentPosition = methprop.ParameterNames;
                HashSet<Name> names = new HashSet<Name>();
                for (int i = 0; i < m_pArguments.carg; i++)
                {
                    if (!m_pArguments.prgexpr[i].isNamedArgumentSpecification())
                    {
                        if (!currentPosition.IsEmpty())
                        {
                            currentPosition = currentPosition.Tail();
                        }
                        continue;
                    }

                    Name name = m_pArguments.prgexpr[i].asNamedArgumentSpecification().Name;
                    if (!methprop.ParameterNames.Contains(name))
                    {
                        if (m_pInvalidSpecifiedName == null)
                        {
                            m_pInvalidSpecifiedName = name;
                        }
                        return false;
                    }
                    else if (!currentPosition.Contains(name))
                    {
                        if (m_pNameUsedInPositionalArgument == null)
                        {
                            m_pNameUsedInPositionalArgument = name;
                        }
                        return false;
                    }
                    if (names.Contains(name))
                    {
                        if (m_pDuplicateSpecifiedName == null)
                        {
                            m_pDuplicateSpecifiedName = name;
                        }
                        return false;
                    }
                    names.Add(name);
                }
                return true;
            }

            // This method returns true if we have another sym to consider.
            // If we've found a match in the current type, and have no more syms to consider in this type, then we
            // return false.
            private bool GetNextSym(CMemberLookupResults.CMethodIterator iterator)
            {
                if (!iterator.MoveNext(m_methList.IsEmpty(), m_bIterateToEndOfNsList))
                {
                    return false;
                }
                m_pCurrentSym = iterator.GetCurrentSymbol();
                AggregateType type = iterator.GetCurrentType();

                // If our current type is null, this is our first iteration, so set the type.
                // If our current type is not null, and we've got a new type now, and we've already matched
                // a symbol, then bail out.

                if (m_pCurrentType != type &&
                        m_pCurrentType != null &&
                        !m_methList.IsEmpty() &&
                        !m_methList.Head().mpwi.GetType().isInterfaceType() &&
                        (!m_methList.Head().mpwi.Sym.IsMethodSymbol() || !m_methList.Head().mpwi.Meth().IsExtension()))
                {
                    return false;
                }
                else if (m_pCurrentType != type &&
                        m_pCurrentType != null &&
                        !m_methList.IsEmpty() &&
                        !m_methList.Head().mpwi.GetType().isInterfaceType() &&
                        m_methList.Head().mpwi.Sym.IsMethodSymbol() &&
                        m_methList.Head().mpwi.Meth().IsExtension())
                {
                    // we have found a applicable method that is an extension now we must move to the end of the NS list before quiting
                    if (m_pGroup.GetOptionalObject() != null)
                    {
                        // if we find this while looking for static methods we should ignore it
                        m_bIterateToEndOfNsList = true;
                    }
                }

                m_pCurrentType = type;

                // We have a new type. If this type is hidden, we need another type.

                while (m_HiddenTypes.Contains(m_pCurrentType))
                {
                    // Move through this type and get the next one.
                    for (; iterator.GetCurrentType() == m_pCurrentType; iterator.MoveNext(m_methList.IsEmpty(), m_bIterateToEndOfNsList)) ;
                    m_pCurrentSym = iterator.GetCurrentSymbol();
                    m_pCurrentType = iterator.GetCurrentType();

                    if (iterator.AtEnd())
                    {
                        return false;
                    }
                }
                return true;
            }

            private bool ConstructExpandedParameters()
            {
                // Deal with params.
                if (m_pCurrentSym == null || m_pArguments == null || m_pCurrentParameters == null)
                {
                    return false;
                }
                if (0 != (m_fBindFlags & BindingFlag.BIND_NOPARAMS))
                {
                    return false;
                }
                if (!m_pCurrentSym.isParamArray)
                {
                    return false;
                }

                // Count the number of optionals in the method. If there are enough optionals
                // and actual arguments, then proceed.
                {
                    int numOptionals = 0;
                    for (int i = m_pArguments.carg; i < m_pCurrentSym.Params.size; i++)
                    {
                        if (m_pCurrentSym.IsParameterOptional(i))
                        {
                            numOptionals++;
                        }
                    }
                    if (m_pArguments.carg + numOptionals < m_pCurrentParameters.size - 1)
                    {
                        return false;
                    }
                }

                Debug.Assert(m_methList.IsEmpty() || m_methList.Head().mpwi.MethProp() != m_pCurrentSym);
                // Construct the expanded params.
                return m_pExprBinder.TryGetExpandedParams(m_pCurrentSym.Params, m_pArguments.carg, out m_pCurrentParameters);
            }

            private Result DetermineCurrentTypeArgs()
            {
                TypeArray typeArgs = m_pGroup.typeArgs;

                // Get the type args.
                if (m_pCurrentSym.IsMethodSymbol() && m_pCurrentSym.AsMethodSymbol().typeVars.size != typeArgs.size)
                {
                    MethodSymbol methSym = m_pCurrentSym.AsMethodSymbol();
                    // Can't infer if some type args are specified.
                    if (typeArgs.size > 0)
                    {
                        if (!m_mwtBadArity)
                        {
                            m_mwtBadArity.Set(methSym, m_pCurrentType);
                        }
                        return Result.Failure_NoSearchForExpanded;
                    }
                    Debug.Assert(methSym.typeVars.size > 0);

                    // Try to infer. If we have an errorsym in the type arguments, we know we cant infer,
                    // but we want to attempt it anyway. We'll mark this as "cant infer" so that we can
                    // report the appropriate error, but we'll continue inferring, since we want 
                    // error sym to go to any type.

                    // UNDONE: Do we know that m_pCurrentType is always the parent of the method symbol?

                    bool inferenceSucceeded;

                    inferenceSucceeded = MethodTypeInferrer.Infer(
                                m_pExprBinder, GetSymbolLoader(),
                                methSym, m_pCurrentType.GetTypeArgsAll(), m_pCurrentParameters,
                                m_pArguments, out m_pCurrentTypeArgs);

                    if (!inferenceSucceeded)
                    {
                        if (m_results.IsBetterUninferrableResult(m_pCurrentTypeArgs))
                        {
                            TypeArray pTypeVars = methSym.typeVars;
                            if (pTypeVars != null && m_pCurrentTypeArgs != null && pTypeVars.size == m_pCurrentTypeArgs.size)
                            {
                                m_mpwiCantInferInstArg.Set(m_pCurrentSym.AsMethodSymbol(), m_pCurrentType, m_pCurrentTypeArgs);
                            }
                            else
                            {
                                m_mpwiCantInferInstArg.Set(m_pCurrentSym.AsMethodSymbol(), m_pCurrentType, pTypeVars);
                            }
                        }
                        return Result.Failure_SearchForExpanded;
                    }
                }
                else
                {
                    m_pCurrentTypeArgs = typeArgs;
                }
                return Result.Success;
            }

            private bool ArgumentsAreConvertible()
            {
                bool containsErrorSym = false;
                bool bIsInstanceParameterConvertible = false;
                if (m_pArguments.carg != 0)
                {
                    UpdateArguments();
                    for (int ivar = 0; ivar < m_pArguments.carg; ivar++)
                    {
                        CType var = m_pCurrentParameters.Item(ivar);
                        bool constraintErrors = !TypeBind.CheckConstraints(GetSemanticChecker(), GetErrorContext(), var, CheckConstraintsFlags.NoErrors);
                        if (constraintErrors && !DoesTypeArgumentsContainErrorSym(var))
                        {
                            m_mpwiParamTypeConstraints.Set(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs);
                            return false;
                        }
                    }

                    for (int ivar = 0; ivar < m_pArguments.carg; ivar++)
                    {
                        CType var = m_pCurrentParameters.Item(ivar);
                        containsErrorSym |= DoesTypeArgumentsContainErrorSym(var);
                        bool fresult;

                        if (m_pArguments.fHasExprs)
                        {
                            EXPR pArgument = m_pArguments.prgexpr[ivar];

                            // If we have a named argument, strip it to do the conversion.
                            if (pArgument.isNamedArgumentSpecification())
                            {
                                pArgument = pArgument.asNamedArgumentSpecification().Value;
                            }

                            fresult = m_pExprBinder.canConvert(pArgument, var);
                        }
                        else
                        {
                            fresult = m_pExprBinder.canConvert(m_pArguments.types.Item(ivar), var);
                        }

                        // Mark this as a legitimate error if we didn't have any error syms.
                        if (!fresult && !containsErrorSym)
                        {
                            if (ivar > m_nArgBest)
                            {
                                m_nArgBest = ivar;

                                // If we already have best method for instance methods don't overwrite with extensions
                                if (!m_results.GetBestResult())
                                {
                                    m_results.GetBestResult().Set(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs);
                                    m_pBestParameters = m_pCurrentParameters;
                                }
                            }
                            else if (ivar == m_nArgBest && m_pArguments.types.Item(ivar) != var)
                            {
                                // this is to eliminate the paranoid case of types that are equal but can't convert 
                                // (think ErrorType != ErrorType)
                                // See if they just differ in out / ref.
                                CType argStripped = m_pArguments.types.Item(ivar).IsParameterModifierType() ?
                                    m_pArguments.types.Item(ivar).AsParameterModifierType().GetParameterType() : m_pArguments.types.Item(ivar);
                                CType varStripped = var.IsParameterModifierType() ? var.AsParameterModifierType().GetParameterType() : var;

                                if (argStripped == varStripped)
                                {
                                    // If we already have best method for instance methods don't overwrite with extensions
                                    if (!m_results.GetBestResult())
                                    {
                                        m_results.GetBestResult().Set(m_pCurrentSym, m_pCurrentType, m_pCurrentTypeArgs);
                                        m_pBestParameters = m_pCurrentParameters;
                                    }
                                }
                            }

                            if (m_pCurrentSym.IsMethodSymbol())
                            {
                                // Do not store the result if we have an extension method and the instance 
                                // parameter isn't convertible.

                                if (!m_pCurrentSym.AsMethodSymbol().IsExtension() || bIsInstanceParameterConvertible)
                                {
                                    m_results.AddInconvertibleResult(
                                        m_pCurrentSym.AsMethodSymbol(),
                                        m_pCurrentType,
                                        m_pCurrentTypeArgs);
                                }
                            }
                            return false;
                        }
                    }
                }

                if (containsErrorSym)
                {
                    if (m_results.IsBetterUninferrableResult(m_pCurrentTypeArgs) && m_pCurrentSym.IsMethodSymbol())
                    {
                        // If we're an instance method or we're an extension that has an inferrable instance argument,
                        // then mark us down. Note that the extension may not need to infer type args,
                        // so check if we have any type variables at all to begin with.
                        if (!m_pCurrentSym.AsMethodSymbol().IsExtension() ||
                            m_pCurrentSym.AsMethodSymbol().typeVars.size == 0 ||
                                MethodTypeInferrer.CanObjectOfExtensionBeInferred(
                                    m_pExprBinder,
                                    GetSymbolLoader(),
                                    m_pCurrentSym.AsMethodSymbol(),
                                    m_pCurrentType.GetTypeArgsAll(),
                                    m_pCurrentSym.AsMethodSymbol().Params,
                                    m_pArguments))
                        {
                            m_results.GetUninferrableResult().Set(
                                    m_pCurrentSym.AsMethodSymbol(),
                                    m_pCurrentType,
                                    m_pCurrentTypeArgs);
                        }
                    }
                }
                else
                {
                    if (m_pCurrentSym.IsMethodSymbol())
                    {
                        // Do not store the result if we have an extension method and the instance 
                        // parameter isn't convertible.

                        if (!m_pCurrentSym.AsMethodSymbol().IsExtension() || bIsInstanceParameterConvertible)
                        {
                            m_results.AddInconvertibleResult(
                                    m_pCurrentSym.AsMethodSymbol(),
                                    m_pCurrentType,
                                    m_pCurrentTypeArgs);
                        }
                    }
                }
                return !containsErrorSym;
            }

            private void UpdateArguments()
            {
                // Parameter types might have changed as a result of
                // method type inference. 

                m_pCurrentParameters = m_pExprBinder.GetTypes().SubstTypeArray(
                        m_pCurrentParameters, m_pCurrentType, m_pCurrentTypeArgs);

                // It is also possible that an optional argument has changed its value
                // as a result of method type inference. For example, when inferring
                // from Foo(10) to Foo<T>(T t1, T t2 = default(T)), the fabricated
                // argument list starts off as being (10, default(T)). After type
                // inference has successfully inferred T as int, it needs to be 
                // transformed into (10, default(int)) before applicability checking
                // notices that default(T) is not assignable to int.

                if (m_pArguments.prgexpr == null || m_pArguments.prgexpr.Count == 0)
                {
                    return;
                }

                MethodOrPropertySymbol pMethod = null; 
                for(int iParam = 0 ; iParam < m_pCurrentParameters.size; ++iParam)
                {
                    EXPR pArgument = m_pArguments.prgexpr[iParam];
                    if (!pArgument.IsOptionalArgument)
                    {
                        continue;
                    }
                    CType pType = m_pCurrentParameters.Item(iParam);

                    if (pType == pArgument.type)
                    {
                        continue;
                    }

                    // Argument has changed its type because of method type inference. Recompute it.
                    if (pMethod == null)
                    {
                        pMethod = FindMostDerivedMethod(m_pCurrentSym, m_pGroup.GetOptionalObject());
                        Debug.Assert(pMethod != null);
                    }
                    Debug.Assert(pMethod.IsParameterOptional(iParam));
                    EXPR pArgumentNew = GenerateOptionalArgument(GetSymbolLoader(), m_pExprBinder.GetExprFactory(), pMethod, m_pCurrentParameters[iParam], iParam);
                    m_pArguments.prgexpr[iParam] = pArgumentNew;
                }
            }

            private bool DoesTypeArgumentsContainErrorSym(CType var)
            {
                if (!var.IsAggregateType())
                {
                    return false;
                }

                TypeArray typeVars = var.AsAggregateType().GetTypeArgsAll();
                for (int i = 0; i < typeVars.size; i++)
                {
                    CType type = typeVars.Item(i);
                    if (type.IsErrorType())
                    {
                        return true;
                    }
                    else if (type.IsAggregateType())
                    {
                        // If we have an agg type sym, check if its type args have errors.
                        if (DoesTypeArgumentsContainErrorSym(type))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }

            // ----------------------------------------------------------------------------

            private void ReportErrorsOnSuccess()
            {
                // used for Methods and Indexers
                Debug.Assert(m_pGroup.sk == SYMKIND.SK_MethodSymbol || m_pGroup.sk == SYMKIND.SK_PropertySymbol && 0 != (m_pGroup.flags & EXPRFLAG.EXF_INDEXER));
                Debug.Assert(m_pGroup.typeArgs.size == 0 || m_pGroup.sk == SYMKIND.SK_MethodSymbol);

                // if this is a binding to finalize on object, then complain:
                if (m_results.GetBestResult().MethProp().name == GetSymbolLoader().GetNameManager().GetPredefName(PredefinedName.PN_DTOR) &&
                    m_results.GetBestResult().MethProp().getClass().isPredefAgg(PredefinedType.PT_OBJECT))
                {
                    if (0 != (m_pGroup.flags & EXPRFLAG.EXF_BASECALL))
                    {
                        GetErrorContext().Error(ErrorCode.ERR_CallingBaseFinalizeDeprecated);
                    }
                    else
                    {
                        GetErrorContext().Error(ErrorCode.ERR_CallingFinalizeDepracated);
                    }
                }

                Debug.Assert(0 == (m_pGroup.flags & EXPRFLAG.EXF_USERCALLABLE) || m_results.GetBestResult().MethProp().isUserCallable());

                if (m_pGroup.sk == SYMKIND.SK_MethodSymbol)
                {
                    Debug.Assert(m_results.GetBestResult().MethProp().IsMethodSymbol());

                    if (m_results.GetBestResult().TypeArgs.size > 0)
                    {
                        // Check method type variable constraints.
                        TypeBind.CheckMethConstraints(GetSemanticChecker(), GetErrorContext(), new MethWithInst(m_results.GetBestResult()));
                    }
                }
            }

            private void ReportErrorsOnFailure()
            {
                // First and foremost, report if the user specified a name more than once.
                if (m_pDuplicateSpecifiedName != null)
                {
                    GetErrorContext().Error(ErrorCode.ERR_DuplicateNamedArgument, m_pDuplicateSpecifiedName);
                    return;
                }

                Debug.Assert(m_methList.IsEmpty());
                // Report inaccessible.
                if (m_results.GetInaccessibleResult())
                {
                    // We might have called this, but it is inaccesable...
                    GetSemanticChecker().ReportAccessError(m_results.GetInaccessibleResult(), m_pExprBinder.ContextForMemberLookup(), GetTypeQualifier(m_pGroup));
                    return;
                }

                // Report bogus.
                if (m_mpwiBogus)
                {
                    // We might have called this, but it is bogus...
                    GetErrorContext().ErrorRef(ErrorCode.ERR_BindToBogus, m_mpwiBogus);
                    return;
                }

                bool bUseDelegateErrors = false;
                Name nameErr = m_pGroup.name;

                // Check for an invoke.
                if (m_pGroup.GetOptionalObject() != null &&
                        m_pGroup.GetOptionalObject().type != null &&
                        m_pGroup.GetOptionalObject().type.isDelegateType() &&
                        m_pGroup.name == GetSymbolLoader().GetNameManager().GetPredefName(PredefinedName.PN_INVOKE))
                {
                    Debug.Assert(!m_results.GetBestResult() || m_results.GetBestResult().MethProp().getClass().IsDelegate());
                    Debug.Assert(!m_results.GetBestResult() || m_results.GetBestResult().GetType().getAggregate().IsDelegate());
                    bUseDelegateErrors = true;
                    nameErr = m_pGroup.GetOptionalObject().type.getAggregate().name;
                }

                if (m_results.GetBestResult())
                {
                    // If we had some invalid arguments for best matching.
                    ReportErrorsForBestMatching(bUseDelegateErrors, nameErr);
                }
                else if (m_results.GetUninferrableResult() || m_mpwiCantInferInstArg)
                {
                    if (!m_results.GetUninferrableResult())
                    {
                        //copy the extension method for which instacne argument type inference failed
                        m_results.GetUninferrableResult().Set(m_mpwiCantInferInstArg.Sym.AsMethodSymbol(), m_mpwiCantInferInstArg.GetType(), m_mpwiCantInferInstArg.TypeArgs);
                    }
                    Debug.Assert(m_results.GetUninferrableResult().Sym.IsMethodSymbol());

                    MethodSymbol sym = m_results.GetUninferrableResult().Meth();
                    TypeArray pCurrentParameters = sym.Params;
                    // if we tried to bind to an extensionmethod and the instance argument Type Inference failed then the method does not exist
                    // on the type at all. this is treated as a lookup error
                    CType type = null;
                    if (m_pGroup.GetOptionalObject() != null)
                    {
                        type = m_pGroup.GetOptionalObject().type;
                    }
                    else if (m_pGroup.GetOptionalLHS() != null)
                    {
                        type = m_pGroup.GetOptionalLHS().type;
                    }

                    MethWithType mwtCantInfer = new MethWithType();
                    mwtCantInfer.Set(m_results.GetUninferrableResult().Meth(), m_results.GetUninferrableResult().GetType());
                    GetErrorContext().Error(ErrorCode.ERR_CantInferMethTypeArgs, mwtCantInfer);
                }
                else if (m_mwtBadArity)
                {
                    int cvar = m_mwtBadArity.Meth().typeVars.size;
                    GetErrorContext().ErrorRef(cvar > 0 ? ErrorCode.ERR_BadArity : ErrorCode.ERR_HasNoTypeVars, m_mwtBadArity, new ErrArgSymKind(m_mwtBadArity.Meth()), m_pArguments.carg);
                }
                else if (m_mpwiParamTypeConstraints)
                {
                    // This will always report an error
                    TypeBind.CheckMethConstraints(GetSemanticChecker(), GetErrorContext(), new MethWithInst(m_mpwiParamTypeConstraints));
                }
                else if (m_pInvalidSpecifiedName != null)
                {
                    // Give a better message for delegate invoke.
                    if (m_pGroup.GetOptionalObject() != null &&
                            m_pGroup.GetOptionalObject().type.IsAggregateType() &&
                            m_pGroup.GetOptionalObject().type.AsAggregateType().GetOwningAggregate().IsDelegate())
                    {
                        GetErrorContext().Error(ErrorCode.ERR_BadNamedArgumentForDelegateInvoke, m_pGroup.GetOptionalObject().type.AsAggregateType().GetOwningAggregate().name, m_pInvalidSpecifiedName);
                    }
                    else
                    {
                        GetErrorContext().Error(ErrorCode.ERR_BadNamedArgument, m_pGroup.name, m_pInvalidSpecifiedName);
                    }
                }
                else if (m_pNameUsedInPositionalArgument != null)
                {
                    GetErrorContext().Error(ErrorCode.ERR_NamedArgumentUsedInPositional, m_pNameUsedInPositionalArgument);
                }
                else
                {
                    CParameterizedError error;

                    if (m_pDelegate != null)
                    {
                        GetErrorContext().MakeError(out error, ErrorCode.ERR_MethDelegateMismatch, nameErr, m_pDelegate);
                        GetErrorContext().AddRelatedTypeLoc(error, m_pDelegate);
                    }
                    else
                    {
                        // The number of arguments must be wrong.

                        if (m_fCandidatesUnsupported)
                        {
                            GetErrorContext().MakeError(out error, ErrorCode.ERR_BindToBogus, nameErr);
                        }
                        else if (bUseDelegateErrors)
                        {
                            Debug.Assert(0 == (m_pGroup.flags & EXPRFLAG.EXF_CTOR));
                            GetErrorContext().MakeError(out error, ErrorCode.ERR_BadDelArgCount, nameErr, m_pArguments.carg);
                        }
                        else
                        {
                            if (0 != (m_pGroup.flags & EXPRFLAG.EXF_CTOR))
                            {
                                Debug.Assert(!m_pGroup.GetParentType().IsTypeParameterType());
                                GetErrorContext().MakeError(out error, ErrorCode.ERR_BadCtorArgCount, m_pGroup.GetParentType(), m_pArguments.carg);
                            }
                            else
                            {
                                GetErrorContext().MakeError(out error, ErrorCode.ERR_BadArgCount, nameErr, m_pArguments.carg);
                            }
                        }
                    }

                    // Report possible matches (same name and is accesible). We stored these in m_swtWrongCount.
                    for (int i = 0; i < m_nWrongCount; i++)
                    {
                        if (GetSemanticChecker().CheckAccess(
                                    m_swtWrongCount[i].Sym,
                                    m_swtWrongCount[i].GetType(),
                                    m_pExprBinder.ContextForMemberLookup(),
                                    GetTypeQualifier(m_pGroup)))
                        {
                            GetErrorContext().AddRelatedSymLoc(error, m_swtWrongCount[i].Sym);
                        }
                    }
                    GetErrorContext().SubmitError(error);
                }
            }
            private void ReportErrorsForBestMatching(bool bUseDelegateErrors, Name nameErr)
            {
                // Best matching overloaded method 'name' had some invalid arguments.
                if (m_pDelegate != null)
                {
                    GetErrorContext().ErrorRef(ErrorCode.ERR_MethDelegateMismatch, nameErr, m_pDelegate, m_results.GetBestResult());
                    return;
                }

                if (m_bBindingCollectionAddArgs)
                {
                    if (ReportErrorsForCollectionAdd())
                    {
                        return;
                    }
                }

                if (bUseDelegateErrors)
                {
                    // Point to the Delegate, not the Invoke method
                    GetErrorContext().Error(ErrorCode.ERR_BadDelArgTypes, m_results.GetBestResult().GetType());
                }
                else
                {
                    if (m_results.GetBestResult().Sym.IsMethodSymbol() && m_results.GetBestResult().Sym.AsMethodSymbol().IsExtension() && m_pGroup.GetOptionalObject() != null)
                    {
                        GetErrorContext().Error(ErrorCode.ERR_BadExtensionArgTypes, m_pGroup.GetOptionalObject().type, m_pGroup.name, m_results.GetBestResult().Sym);
                    }
                    else if (m_bBindingCollectionAddArgs)
                    {
                        GetErrorContext().Error(ErrorCode.ERR_BadArgTypesForCollectionAdd, m_results.GetBestResult());
                    }
                    else
                    {
                        GetErrorContext().Error(ErrorCode.ERR_BadArgTypes, m_results.GetBestResult());
                    }
                }

                // Argument X: cannot convert type 'Y' to type 'Z'
                for (int ivar = 0; ivar < m_pArguments.carg; ivar++)
                {
                    CType var = m_pBestParameters.Item(ivar);

                    if (!m_pExprBinder.canConvert(m_pArguments.prgexpr[ivar], var))
                    {
                        // See if they just differ in out / ref.
                        CType argStripped = m_pArguments.types.Item(ivar).IsParameterModifierType() ?
                            m_pArguments.types.Item(ivar).AsParameterModifierType().GetParameterType() : m_pArguments.types.Item(ivar);
                        CType varStripped = var.IsParameterModifierType() ? var.AsParameterModifierType().GetParameterType() : var;
                        if (argStripped == varStripped)
                        {
                            if (varStripped != var)
                            {
                                // The argument is wrong in ref / out-ness.
                                GetErrorContext().Error(ErrorCode.ERR_BadArgRef, ivar + 1, (var.IsParameterModifierType() && var.AsParameterModifierType().isOut) ? "out" : "ref");
                            }
                            else
                            {
                                CType argument = m_pArguments.types.Item(ivar);

                                // the argument is decorated, but doesn't needs a 'ref' or 'out'
                                GetErrorContext().Error(ErrorCode.ERR_BadArgExtraRef, ivar + 1, (argument.IsParameterModifierType() && argument.AsParameterModifierType().isOut) ? "out" : "ref");
                            }
                        }
                        else
                        {
                            // if we tried to bind to an extensionmethod and the instance argument conversion failed then the method does not exist
                            // on the type at all. 
                            Symbol sym = m_results.GetBestResult().Sym;
                            if (ivar == 0 && sym.IsMethodSymbol() && sym.AsMethodSymbol().IsExtension() && m_pGroup.GetOptionalObject() != null &&
                                !m_pExprBinder.canConvertInstanceParamForExtension(m_pGroup.GetOptionalObject(), sym.AsMethodSymbol().Params.Item(0)))
                            {
                                if (!m_pGroup.GetOptionalObject().type.getBogus())
                                {
                                    GetErrorContext().Error(ErrorCode.ERR_BadInstanceArgType, m_pGroup.GetOptionalObject().type, var);
                                }
                            }
                            else
                            {
                                GetErrorContext().Error(ErrorCode.ERR_BadArgType, ivar + 1, new ErrArg(m_pArguments.types.Item(ivar), ErrArgFlags.Unique), new ErrArg(var, ErrArgFlags.Unique));
                            }
                        }
                    }
                }
            }

            private bool ReportErrorsForCollectionAdd()
            {
                for (int ivar = 0; ivar < m_pArguments.carg; ivar++)
                {
                    CType var = m_pBestParameters.Item(ivar);
                    if (var.IsParameterModifierType())
                    {
                        GetErrorContext().ErrorRef(ErrorCode.ERR_InitializerAddHasParamModifiers, m_results.GetBestResult());
                        return true;
                    }
                }
                return false;
            }
        }
    }
}
