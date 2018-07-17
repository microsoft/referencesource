// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using Microsoft.CSharp.RuntimeBinder;
using Microsoft.CSharp.RuntimeBinder.Errors;
using Microsoft.CSharp.RuntimeBinder.Syntax;

#if FEATURE_NETCORE
using System.Security;
#endif
namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class TypeManager
    {
        BSYMMGR m_BSymmgr;
        PredefinedTypes m_predefTypes;

        TypeFactory m_typeFactory;
        TypeTable m_typeTable;
        SymbolTable m_symbolTable;

        // Special types
        VoidType voidType;
        NullType nullType;
        OpenTypePlaceholderType typeUnit;
        BoundLambdaType typeAnonMeth;
        MethodGroupType typeMethGrp;
        ArgumentListType argListType;
        ErrorType errorType;

        StdTypeVarColl stvcMethod;
        StdTypeVarColl stvcClass;

        public TypeManager()
        {
            this.m_predefTypes = null; // Initialized via the Init call.
            this.m_BSymmgr = null; // Initialized via the Init call.
            this.m_typeFactory = new TypeFactory();
            this.m_typeTable = new TypeTable();

            // special types with their own symbol kind.
            errorType = m_typeFactory.CreateError(null, null, null, null, null);
            voidType = m_typeFactory.CreateVoid();
            nullType = m_typeFactory.CreateNull();
            typeUnit = m_typeFactory.CreateUnit();
            typeAnonMeth = m_typeFactory.CreateAnonMethod();
            typeMethGrp = m_typeFactory.CreateMethodGroup();
            argListType = m_typeFactory.CreateArgList();

            InitType(errorType);
            errorType.SetErrors(true);

            InitType(voidType);
            InitType(nullType);
            InitType(typeUnit);
            InitType(typeAnonMeth);
            InitType(typeMethGrp);

            stvcMethod = new StdTypeVarColl();
            stvcClass = new StdTypeVarColl();
        }

        public void InitTypeFactory(SymbolTable table)
        {
            m_symbolTable = table;
        }

        private void InitType(CType at)
        {
        }

        public static bool TypeContainsAnonymousTypes(CType type)
        {
            CType ctype = (CType)type;

        LRecurse:  // Label used for "tail" recursion.
            switch (ctype.GetTypeKind())
            {
                default:
                    Debug.Assert(false, "Bad Symbol kind in TypeContainsAnonymousTypes");
                    return false;

                case TypeKind.TK_NullType:
                case TypeKind.TK_VoidType:
                case TypeKind.TK_NullableType:
                case TypeKind.TK_TypeParameterType:
                case TypeKind.TK_UnboundLambdaType:
                case TypeKind.TK_MethodGroupType:
                    return false;

                case TypeKind.TK_ArrayType:
                case TypeKind.TK_ParameterModifierType:
                case TypeKind.TK_PointerType:
                    ctype = (CType)ctype.GetBaseOrParameterOrElementType();
                    goto LRecurse;

                case TypeKind.TK_AggregateType:
                    if (ctype.AsAggregateType().getAggregate().IsAnonymousType())
                    {
                        return true;
                    }

                    TypeArray typeArgsAll = ctype.AsAggregateType().GetTypeArgsAll();
                    for (int i = 0; i < typeArgsAll.size; i++)
                    {
                        CType typeArg = typeArgsAll.Item(i);

                        if (TypeContainsAnonymousTypes(typeArg))
                        {
                            return true;
                        }
                    }
                    return false;

                case TypeKind.TK_ErrorType:
                    if (ctype.AsErrorType().HasTypeParent())
                    {
                        ctype = ctype.AsErrorType().GetTypeParent();
                        goto LRecurse;
                    }
                    return false;
            }
        }

        class StdTypeVarColl
        {
            public List<TypeParameterType> prgptvs;

            public StdTypeVarColl()
            {
                prgptvs = new List<TypeParameterType>();
            }

            ////////////////////////////////////////////////////////////////////////////////
            // Get the standard type variable (eg, !0, !1, or !!0, !!1).
            //
            //      iv is the index.
            //      pbsm is the containing symbol manager
            //      fMeth designates whether this is a method type var or class type var
            //
            // The standard class type variables are useful during emit, but not for type 
            // comparison when binding. The standard method type variables are useful during
            // binding for signature comparison.

            public TypeParameterType GetTypeVarSym(int iv, TypeManager pTypeManager, bool fMeth)
            {
                Debug.Assert(iv >= 0);

                TypeParameterType tpt = null;
                if (iv >= this.prgptvs.Count)
                {
                    TypeParameterSymbol pTypeParameter = new TypeParameterSymbol();
                    pTypeParameter.SetIsMethodTypeParameter(fMeth);
                    pTypeParameter.SetIndexInOwnParameters(iv);
                    pTypeParameter.SetIndexInTotalParameters(iv);
                    pTypeParameter.SetAccess(ACCESS.ACC_PRIVATE);
                    tpt = pTypeManager.GetTypeParameter(pTypeParameter);
                    this.prgptvs.Add(tpt);
                }
                else
                {
                    tpt = this.prgptvs[iv];
                }
                Debug.Assert(tpt != null);
                return tpt;
            }
        }

        public ArrayType GetArray(CType elementType, int args)
        {
            Name name;
            ArrayType pArray;

            Debug.Assert(args > 0 && args < 32767);

            switch (args)
            {
                case 1:
                case 2:
                    name = m_BSymmgr.GetNameManager().GetPredefinedName(PredefinedName.PN_ARRAY0 + args);
                    break;
                default:
                    name = m_BSymmgr.GetNameManager().Add("[X" + args + 1);
                    break;
            }

            // See if we already have an array type of this element type and rank.
            pArray = m_typeTable.LookupArray(name, elementType);
            if (pArray == null)
            {
                // No existing array symbol. Create a new one.
                pArray = m_typeFactory.CreateArray(name, elementType, args);
                pArray.InitFromParent();

                m_typeTable.InsertArray(name, elementType, pArray);
            }
            else
            {
                Debug.Assert(pArray.HasErrors() == elementType.HasErrors());
                Debug.Assert(pArray.IsUnresolved() == elementType.IsUnresolved());
            }

            Debug.Assert(pArray.rank == args);
            Debug.Assert(pArray.GetElementType() == elementType);

            return pArray;
        }

        public AggregateType GetAggregate(AggregateSymbol agg, AggregateType atsOuter, TypeArray typeArgs)
        {
            Debug.Assert(agg.GetTypeManager() == this);
            Debug.Assert(atsOuter == null || atsOuter.getAggregate() == agg.Parent, "");

            if (typeArgs == null)
            {
                typeArgs = BSYMMGR.EmptyTypeArray();
            }

            Debug.Assert(agg.GetTypeVars().Size == typeArgs.Size);

            Name name = m_BSymmgr.GetNameFromPtrs(typeArgs, atsOuter);
            Debug.Assert(name != null);

            AggregateType pAggregate = m_typeTable.LookupAggregate(name, agg);
            if (pAggregate == null)
            {
                pAggregate = m_typeFactory.CreateAggregateType(
                          name,
                          agg,
                          typeArgs,
                          atsOuter
                      );

                Debug.Assert(!pAggregate.fConstraintsChecked && !pAggregate.fConstraintError);

                pAggregate.SetErrors(pAggregate.GetTypeArgsAll().HasErrors());
#if CSEE
        
                SpecializedSymbolCreationEE* pSymCreate = static_cast<SpecializedSymbolCreationEE*>(m_BSymmgr.GetSymFactory().m_pSpecializedSymbolCreation);
                AggregateSymbolExtra* pExtra = pSymCreate.GetHashTable().GetElement(agg).AsAggregateSymbolExtra();
                pAggregate.typeRes = pAggregate;
                if (!pAggregate.IsUnresolved())
                {
                    pAggregate.tsRes = ktsImportMax;
                }
                pAggregate.fDirty = pExtra.IsDirty() || pAggregate.IsUnresolved();
                pAggregate.tsDirty = pExtra.GetLastComputedDirtyBit();
#endif // CSEE

                m_typeTable.InsertAggregate(name, agg, pAggregate);

                // If we have a generic type definition, then we need to set the
                // base class to be our current base type, and use that to calculate 
                // our agg type and its base, then set it to be the generic version of the
                // base type. This is because:
                //
                // Suppose we have Foo<T> : IFoo<T>
                //
                // Initially, the BaseType will be IFoo<Foo.T>, which gives us the substitution
                // that we want to use for our agg type's base type. However, in the Symbol chain,
                // we want the base type to be IFoo<IFoo.T>. Thats why we need to do this little trick.
                //
                // If we dont have a generic type definition, then we just need to set our base
                // class. This is so that if we have a base type that's generic, we'll be
                // getting the correctly instantiated base type.

                if (pAggregate.AssociatedSystemType != null &&
                    pAggregate.AssociatedSystemType.BaseType != null)
                {
                    // Store the old base class.

                    AggregateType oldBaseType = agg.GetBaseClass();
                    agg.SetBaseClass(m_symbolTable.GetCTypeFromType(pAggregate.AssociatedSystemType.BaseType).AsAggregateType());
                    pAggregate.GetBaseClass(); // Get the base type for the new agg type we're making.

                    agg.SetBaseClass(oldBaseType);
                }
            }
            else
            {
                Debug.Assert(pAggregate.HasErrors() == pAggregate.GetTypeArgsAll().HasErrors());
            }

            Debug.Assert(pAggregate.getAggregate() == agg);
            Debug.Assert(pAggregate.GetTypeArgsThis() != null && pAggregate.GetTypeArgsAll() != null);
            Debug.Assert(pAggregate.GetTypeArgsThis() == typeArgs);

            return pAggregate;
        }

        public AggregateType GetAggregate(AggregateSymbol agg, TypeArray typeArgsAll)
        {
            Debug.Assert(typeArgsAll != null && typeArgsAll.Size == agg.GetTypeVarsAll().Size);

            if (typeArgsAll.size == 0)
                return agg.getThisType();

            AggregateSymbol aggOuter = agg.GetOuterAgg();

            if (aggOuter == null)
                return GetAggregate(agg, null, typeArgsAll);

            int cvarOuter = aggOuter.GetTypeVarsAll().Size;
            Debug.Assert(cvarOuter <= typeArgsAll.Size);

            TypeArray typeArgsOuter = m_BSymmgr.AllocParams(cvarOuter, typeArgsAll, 0);
            TypeArray typeArgsInner = m_BSymmgr.AllocParams(agg.GetTypeVars().Size, typeArgsAll, cvarOuter);
            AggregateType atsOuter = GetAggregate(aggOuter, typeArgsOuter);

            return GetAggregate(agg, atsOuter, typeArgsInner);
        }

        public PointerType GetPointer(CType baseType)
        {
            PointerType pPointer = m_typeTable.LookupPointer(baseType);
            if (pPointer == null)
            {
                // No existing type. Create a new one.
                Name namePtr = m_BSymmgr.GetNameManager().GetPredefName(PredefinedName.PN_PTR);

                pPointer = m_typeFactory.CreatePointer(namePtr, baseType);
                pPointer.InitFromParent();

                m_typeTable.InsertPointer(baseType, pPointer);
            }
            else
            {
                Debug.Assert(pPointer.HasErrors() == baseType.HasErrors());
                Debug.Assert(pPointer.IsUnresolved() == baseType.IsUnresolved());
            }

            Debug.Assert(pPointer.GetReferentType() == baseType);

            return pPointer;
        }

        public NullableType GetNullable(CType pUnderlyingType)
        {
            NullableType pNullableType = m_typeTable.LookupNullable(pUnderlyingType);
            if (pNullableType == null)
            {
                Name pName = m_BSymmgr.GetNameManager().GetPredefName(PredefinedName.PN_NUB);

                pNullableType = m_typeFactory.CreateNullable(pName, pUnderlyingType, m_BSymmgr, this);
                pNullableType.InitFromParent();

                m_typeTable.InsertNullable(pUnderlyingType, pNullableType);
            }

            return pNullableType;
        }

        public NullableType GetNubFromNullable(AggregateType ats)
        {
            Debug.Assert(ats.isPredefType(PredefinedType.PT_G_OPTIONAL));
            return GetNullable(ats.GetTypeArgsAll().Item(0));
        }

        public ParameterModifierType GetParameterModifier(CType paramType, bool isOut)
        {
            Name name = m_BSymmgr.GetNameManager().GetPredefName(isOut ? PredefinedName.PN_OUTPARAM : PredefinedName.PN_REFPARAM);
            ParameterModifierType pParamModifier = m_typeTable.LookupParameterModifier(name, paramType);

            if (pParamModifier == null)
            {
                // No existing parammod symbol. Create a new one.
                pParamModifier = m_typeFactory.CreateParameterModifier(name, paramType);
                pParamModifier.isOut = isOut;
                pParamModifier.InitFromParent();

                m_typeTable.InsertParameterModifier(name, paramType, pParamModifier);
            }
            else
            {
                Debug.Assert(pParamModifier.HasErrors() == paramType.HasErrors());
                Debug.Assert(pParamModifier.IsUnresolved() == paramType.IsUnresolved());
            }

            Debug.Assert(pParamModifier.GetParameterType() == paramType);

            return pParamModifier;
        }

        public ErrorType GetErrorType(
                CType pParentType,
                AssemblyQualifiedNamespaceSymbol pParentNS,
                Name nameText,
                TypeArray typeArgs)
        {
            Debug.Assert(nameText != null);
            Debug.Assert(pParentType == null || pParentNS == null);
            if (pParentType == null && pParentNS == null)
            {
                // Use the root namespace as the parent.
                pParentNS = m_BSymmgr.GetRootNsAid(KAID.kaidGlobal);
            }
            if (typeArgs == null)
            {
                typeArgs = BSYMMGR.EmptyTypeArray();
            }

            Name name = m_BSymmgr.GetNameFromPtrs(nameText, typeArgs);
            Debug.Assert(name != null);

            ErrorType pError = null;
            if (pParentType != null)
            {
                pError = m_typeTable.LookupError(name, pParentType);
            }
            else
            {
                Debug.Assert(pParentNS != null);
                pError = m_typeTable.LookupError(name, pParentNS);
            }

            if (pError == null)
            {
                // No existing error symbol. Create a new one.
                pError = m_typeFactory.CreateError(name, pParentType, pParentNS, nameText, typeArgs);
                pError.SetErrors(true);
                if (pParentType != null)
                {
                    m_typeTable.InsertError(name, pParentType, pError);
                }
                else
                {
                    m_typeTable.InsertError(name, pParentNS, pError);
                }
            }
            else
            {
                Debug.Assert(pError.HasErrors());
                Debug.Assert(pError.nameText == nameText);
                Debug.Assert(pError.typeArgs == typeArgs);
            }
            Debug.Assert(!pError.IsUnresolved());

            return pError;
        }

        public VoidType GetVoid()
        {
            return this.voidType;
        }

        public NullType GetNullType()
        {
            return this.nullType;
        }

        public OpenTypePlaceholderType GetUnitType()
        {
            return this.typeUnit;
        }

        public BoundLambdaType GetAnonMethType()
        {
            return this.typeAnonMeth;
        }

        public MethodGroupType GetMethGrpType()
        {
            return this.typeMethGrp;
        }

        public ArgumentListType GetArgListType()
        {
            return this.argListType;
        }

        public ErrorType GetErrorSym()
        {
            return this.errorType;
        }

        public AggregateSymbol GetNullable()
        {
            return this.GetOptPredefAgg(PredefinedType.PT_G_OPTIONAL);
        }

        public CType SubstType(CType typeSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth, SubstTypeFlags grfst)
        {
            if (typeSrc == null)
                return null;

            var ctx = new SubstContext(typeArgsCls, typeArgsMeth, grfst);
            return ctx.FNop() ? typeSrc : SubstTypeCore(typeSrc, ctx);
        }

        public CType SubstType(CType typeSrc, TypeArray typeArgsCls)
        {
            return SubstType(typeSrc, typeArgsCls, null, SubstTypeFlags.NormNone);
        }

        public CType SubstType(CType typeSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth)
        {
            return SubstType(typeSrc, typeArgsCls, typeArgsMeth, SubstTypeFlags.NormNone);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, SubstContext pctx)
        {
            if (taSrc == null || taSrc.Size == 0 || pctx == null || pctx.FNop())
                return taSrc;

            CType[] prgpts = new CType[taSrc.Size];
            for (int ipts = 0; ipts < taSrc.Size; ipts++)
            {
                prgpts[ipts] = this.SubstTypeCore(taSrc.Item(ipts), pctx);
            }
            return m_BSymmgr.AllocParams(taSrc.size, prgpts);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth, SubstTypeFlags grfst)
        {
            if (taSrc == null || taSrc.Size == 0)
                return taSrc;

            var ctx = new SubstContext(typeArgsCls, typeArgsMeth, grfst);

            if (ctx.FNop())
                return taSrc;

            CType[] prgpts = new CType[taSrc.Size];
            for (int ipts = 0; ipts < taSrc.Size; ipts++)
            {
                prgpts[ipts] = SubstTypeCore(taSrc.Item(ipts), ctx);
            }
            return m_BSymmgr.AllocParams(taSrc.Size, prgpts);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth)
        {
            return this.SubstTypeArray(taSrc, typeArgsCls, typeArgsMeth, SubstTypeFlags.NormNone);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, TypeArray typeArgsCls)
        {
            return this.SubstTypeArray(taSrc, typeArgsCls, (TypeArray)null, SubstTypeFlags.NormNone);
        }

        private CType SubstTypeCore(CType type, SubstContext pctx)
        {
            CType typeSrc;
            CType typeDst;

            switch (type.GetTypeKind())
            {
                default:
                    Debug.Assert(false);
                    return type;

                case TypeKind.TK_NullType:
                case TypeKind.TK_VoidType:
                case TypeKind.TK_OpenTypePlaceholderType:
                case TypeKind.TK_MethodGroupType:
                case TypeKind.TK_BoundLambdaType:
                case TypeKind.TK_UnboundLambdaType:
                case TypeKind.TK_NaturalIntegerType:
                case TypeKind.TK_ArgumentListType:
                    return type;

                case TypeKind.TK_ParameterModifierType:
                    typeDst = SubstTypeCore(typeSrc = type.AsParameterModifierType().GetParameterType(), pctx);
                    return (typeDst == typeSrc) ? type : GetParameterModifier(typeDst, type.AsParameterModifierType().isOut);

                case TypeKind.TK_ArrayType:
                    typeDst = SubstTypeCore(typeSrc = type.AsArrayType().GetElementType(), pctx);
                    return (typeDst == typeSrc) ? type : GetArray(typeDst, type.AsArrayType().rank);

                case TypeKind.TK_PointerType:
                    typeDst = SubstTypeCore(typeSrc = type.AsPointerType().GetReferentType(), pctx);
                    return (typeDst == typeSrc) ? type : GetPointer(typeDst);

                case TypeKind.TK_NullableType:
                    typeDst = SubstTypeCore(typeSrc = type.AsNullableType().GetUnderlyingType(), pctx);
                    return (typeDst == typeSrc) ? type : GetNullable(typeDst);

                case TypeKind.TK_AggregateType:
                    if (type.AsAggregateType().GetTypeArgsAll().size > 0)
                    {
                        AggregateType ats = type.AsAggregateType();

                        TypeArray typeArgs = SubstTypeArray(ats.GetTypeArgsAll(), pctx);
                        if (ats.GetTypeArgsAll() != typeArgs)
                            return GetAggregate(ats.getAggregate(), typeArgs);
                    }
                    return type;

                case TypeKind.TK_ErrorType:
                    if (type.AsErrorType().HasParent())
                    {
                        ErrorType err = type.AsErrorType();
                        Debug.Assert(err.nameText != null && err.typeArgs != null);

                        CType pParentType = null;
                        if (err.HasTypeParent())
                        {
                            pParentType = SubstTypeCore(err.GetTypeParent(), pctx);
                        }

                        TypeArray typeArgs = SubstTypeArray(err.typeArgs, pctx);
                        if (typeArgs != err.typeArgs || (err.HasTypeParent() && pParentType != err.GetTypeParent()))
                        {
                            return GetErrorType(pParentType, err.GetNSParent(), err.nameText, typeArgs);
                        }
                    }
                    return type;

                case TypeKind.TK_TypeParameterType:
                    {
                        TypeParameterSymbol tvs = type.AsTypeParameterType().GetTypeParameterSymbol();
                        int index = tvs.GetIndexInTotalParameters();
                        if (tvs.IsMethodTypeParameter())
                        {
                            if ((pctx.grfst & SubstTypeFlags.DenormMeth) != 0 && tvs.parent != null)
                                return type;
                            Debug.Assert(tvs.GetIndexInOwnParameters() == tvs.GetIndexInTotalParameters());
                            if (index < pctx.ctypeMeth)
                            {
                                Debug.Assert(pctx.prgtypeMeth != null);
                                return pctx.prgtypeMeth[index];
                            }
                            else
                            {
                                return ((pctx.grfst & SubstTypeFlags.NormMeth) != 0 ? GetStdMethTypeVar(index) : type);
                            }
                        }
                        if ((pctx.grfst & SubstTypeFlags.DenormClass) != 0 && tvs.parent != null)
                            return type;
                        return index < pctx.ctypeCls ? pctx.prgtypeCls[index] :
                               ((pctx.grfst & SubstTypeFlags.NormClass) != 0 ? GetStdClsTypeVar(index) : type);
                    }
            }
        }

        public bool SubstEqualTypes(CType typeDst, CType typeSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth, SubstTypeFlags grfst)
        {
            if (typeDst.Equals(typeSrc))
            {
                Debug.Assert(typeDst.Equals(SubstType(typeSrc, typeArgsCls, typeArgsMeth, grfst)));
                return true;
            }

            var ctx = new SubstContext(typeArgsCls, typeArgsMeth, grfst);

            return !ctx.FNop() && SubstEqualTypesCore(typeDst, typeSrc, ctx);
        }

        public bool SubstEqualTypeArrays(TypeArray taDst, TypeArray taSrc, TypeArray typeArgsCls, TypeArray typeArgsMeth, SubstTypeFlags grfst)
        {
            // Handle the simple common cases first.
            if (taDst == taSrc || (taDst != null && taDst.Equals(taSrc)))
            {
                // The following assertion is not always true and indicates a problem where
                // the signature of override method does not match the one inherited from
                // the base class. The method match we have found does not take the type 
                // arguments of the base class into account. So actually we are not overriding
                // the method that we "intend" to. This overload resolution problem in nested
                // generic types is tracked by DevDiv Bugs 152636.
                // Debug.Assert(taDst == SubstTypeArray(taSrc, typeArgsCls, typeArgsMeth, grfst));
                return true;
            }
            if (taDst.Size != taSrc.Size)
                return false;
            if (taDst.Size == 0)
                return true;

            var ctx = new SubstContext(typeArgsCls, typeArgsMeth, grfst);

            if (ctx.FNop())
                return false;

            for (int i = 0; i < taDst.size; i++)
            {
                if (!SubstEqualTypesCore(taDst.Item(i), taSrc.Item(i), ctx))
                    return false;
            }

            return true;
        }

        public bool SubstEqualTypesCore(CType typeDst, CType typeSrc, SubstContext pctx)
        {
        LRecurse:  // Label used for "tail" recursion.

            if (typeDst == typeSrc || typeDst.Equals(typeSrc))
            {
                return true;
            }

            switch (typeSrc.GetTypeKind())
            {
                default:
                    Debug.Assert(false, "Bad Symbol kind in SubstEqualTypesCore");
                    return false;

                case TypeKind.TK_NullType:
                case TypeKind.TK_VoidType:
                case TypeKind.TK_OpenTypePlaceholderType:
                    // There should only be a single instance of these.
                    Debug.Assert(typeDst.GetTypeKind() != typeSrc.GetTypeKind());
                    return false;

                case TypeKind.TK_ArrayType:
                    if (typeDst.GetTypeKind() != TypeKind.TK_ArrayType || typeDst.AsArrayType().rank != typeSrc.AsArrayType().rank)
                        return false;
                    goto LCheckBases;

                case TypeKind.TK_ParameterModifierType:
                    if (typeDst.GetTypeKind() != TypeKind.TK_ParameterModifierType ||
                        ((pctx.grfst & SubstTypeFlags.NoRefOutDifference) == 0 &&
                         typeDst.AsParameterModifierType().isOut != typeSrc.AsParameterModifierType().isOut))
                        return false;
                    goto LCheckBases;

                case TypeKind.TK_PointerType:
                case TypeKind.TK_NullableType:
                    if (typeDst.GetTypeKind() != typeSrc.GetTypeKind())
                        return false;
                LCheckBases:
                    typeSrc = typeSrc.GetBaseOrParameterOrElementType();
                typeDst = typeDst.GetBaseOrParameterOrElementType();
                goto LRecurse;

                case TypeKind.TK_AggregateType:
                if (typeDst.GetTypeKind() != TypeKind.TK_AggregateType)
                    return false;
                { // BLOCK
                    AggregateType atsSrc = typeSrc.AsAggregateType();
                    AggregateType atsDst = typeDst.AsAggregateType();

                    if (atsSrc.getAggregate() != atsDst.getAggregate())
                        return false;

                    Debug.Assert(atsSrc.GetTypeArgsAll().Size == atsDst.GetTypeArgsAll().Size);

                    // All the args must unify.
                    for (int i = 0; i < atsSrc.GetTypeArgsAll().Size; i++)
                    {
                        if (!SubstEqualTypesCore(atsDst.GetTypeArgsAll().Item(i), atsSrc.GetTypeArgsAll().Item(i), pctx))
                            return false;
                    }
                }
                return true;

                case TypeKind.TK_ErrorType:
                if (!typeDst.IsErrorType() || !typeSrc.AsErrorType().HasParent() || !typeDst.AsErrorType().HasParent())
                    return false;
                {
                    ErrorType errSrc = typeSrc.AsErrorType();
                    ErrorType errDst = typeDst.AsErrorType();
                    Debug.Assert(errSrc.nameText != null && errSrc.typeArgs != null);
                    Debug.Assert(errDst.nameText != null && errDst.typeArgs != null);

                    if (errSrc.nameText != errDst.nameText || errSrc.typeArgs.Size != errDst.typeArgs.Size)
                        return false;

                    if (errSrc.HasTypeParent() != errDst.HasTypeParent())
                    {
                        return false;
                    }
                    if (errSrc.HasTypeParent())
                    {
                        if (errSrc.GetTypeParent() != errDst.GetTypeParent())
                        {
                            return false;
                        }
                        if (!SubstEqualTypesCore(errDst.GetTypeParent(), errSrc.GetTypeParent(), pctx))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (errSrc.GetNSParent() != errDst.GetNSParent())
                        {
                            return false;
                        }
                    }

                    // All the args must unify.
                    for (int i = 0; i < errSrc.typeArgs.Size; i++)
                    {
                        if (!SubstEqualTypesCore(errDst.typeArgs.Item(i), errSrc.typeArgs.Item(i), pctx))
                            return false;
                    }
                }
                return true;

                case TypeKind.TK_TypeParameterType:
                { // BLOCK
                    TypeParameterSymbol tvs = typeSrc.AsTypeParameterType().GetTypeParameterSymbol();
                    int index = tvs.GetIndexInTotalParameters();

                    if (tvs.IsMethodTypeParameter())
                    {
                        if ((pctx.grfst & SubstTypeFlags.DenormMeth) != 0 && tvs.parent != null)
                        {
                            // typeDst == typeSrc was handled above.
                            Debug.Assert(typeDst != typeSrc);
                            return false;
                        }
                        Debug.Assert(tvs.GetIndexInOwnParameters() == tvs.GetIndexInTotalParameters());
                        Debug.Assert(pctx.prgtypeMeth == null || tvs.GetIndexInTotalParameters() < pctx.ctypeMeth);
                        if (index < pctx.ctypeMeth && pctx.prgtypeMeth != null)
                        {
                            return typeDst == pctx.prgtypeMeth[index];
                        }
                        if ((pctx.grfst & SubstTypeFlags.NormMeth) != 0)
                        {
                            return typeDst == GetStdMethTypeVar(index);
                        }
                    }
                    else
                    {
                        if ((pctx.grfst & SubstTypeFlags.DenormClass) != 0 && tvs.parent != null)
                        {
                            // typeDst == typeSrc was handled above.
                            Debug.Assert(typeDst != typeSrc);
                            return false;
                        }
                        Debug.Assert(pctx.prgtypeCls == null || tvs.GetIndexInTotalParameters() < pctx.ctypeCls);
                        if (index < pctx.ctypeCls)
                            return typeDst == pctx.prgtypeCls[index];
                        if ((pctx.grfst & SubstTypeFlags.NormClass) != 0)
                            return typeDst == GetStdClsTypeVar(index);
                    }
                }
                return false;
            }
        }

        public void ReportMissingPredefTypeError(ErrorHandling errorContext, PredefinedType pt)
        {
            m_predefTypes.ReportMissingPredefTypeError(errorContext, pt);
        }

        public static bool TypeContainsType(CType type, CType typeFind)
        {
        LRecurse:  // Label used for "tail" recursion.

            if (type == typeFind || type.Equals(typeFind))
                return true;

            switch (type.GetTypeKind())
            {
                default:
                    Debug.Assert(false, "Bad Symbol kind in TypeContainsType");
                    return false;

                case TypeKind.TK_NullType:
                case TypeKind.TK_VoidType:
                case TypeKind.TK_OpenTypePlaceholderType:
                    // There should only be a single instance of these.
                    Debug.Assert(typeFind.GetTypeKind() != type.GetTypeKind());
                    return false;

                case TypeKind.TK_ArrayType:
                case TypeKind.TK_NullableType:
                case TypeKind.TK_ParameterModifierType:
                case TypeKind.TK_PointerType:
                    type = type.GetBaseOrParameterOrElementType();
                    goto LRecurse;

                case TypeKind.TK_AggregateType:
                    { // BLOCK
                        AggregateType ats = type.AsAggregateType();

                        for (int i = 0; i < ats.GetTypeArgsAll().Size; i++)
                        {
                            if (TypeContainsType(ats.GetTypeArgsAll().Item(i), typeFind))
                                return true;
                        }
                    }
                    return false;

                case TypeKind.TK_ErrorType:
                    if (type.AsErrorType().HasParent())
                    {
                        ErrorType err = type.AsErrorType();
                        Debug.Assert(err.nameText != null && err.typeArgs != null);

                        for (int i = 0; i < err.typeArgs.Size; i++)
                        {
                            if (TypeContainsType(err.typeArgs.Item(i), typeFind))
                                return true;
                        }
                        if (err.HasTypeParent())
                        {
                            type = err.GetTypeParent();
                            goto LRecurse;
                        }
                    }
                    return false;

                case TypeKind.TK_TypeParameterType:
                    return false;
            }
        }

        public static bool TypeContainsTyVars(CType type, TypeArray typeVars)
        {
        LRecurse:  // Label used for "tail" recursion.
            switch (type.GetTypeKind())
            {
                default:
                    Debug.Assert(false, "Bad Symbol kind in TypeContainsTyVars");
                    return false;

                case TypeKind.TK_UnboundLambdaType:
                case TypeKind.TK_BoundLambdaType:
                case TypeKind.TK_NullType:
                case TypeKind.TK_VoidType:
                case TypeKind.TK_OpenTypePlaceholderType:
                case TypeKind.TK_MethodGroupType:
                    return false;

                case TypeKind.TK_ArrayType:
                case TypeKind.TK_NullableType:
                case TypeKind.TK_ParameterModifierType:
                case TypeKind.TK_PointerType:
                    type = type.GetBaseOrParameterOrElementType();
                    goto LRecurse;

                case TypeKind.TK_AggregateType:
                    { // BLOCK
                        AggregateType ats = type.AsAggregateType();

                        for (int i = 0; i < ats.GetTypeArgsAll().Size; i++)
                        {
                            if (TypeContainsTyVars(ats.GetTypeArgsAll().Item(i), typeVars))
                            {
                                return true;
                            }
                        }
                    }
                    return false;

                case TypeKind.TK_ErrorType:
                    if (type.AsErrorType().HasParent())
                    {
                        ErrorType err = type.AsErrorType();
                        Debug.Assert(err.nameText != null && err.typeArgs != null);

                        for (int i = 0; i < err.typeArgs.Size; i++)
                        {
                            if (TypeContainsTyVars(err.typeArgs.Item(i), typeVars))
                            {
                                return true;
                            }
                        }
                        if (err.HasTypeParent())
                        {
                            type = err.GetTypeParent();
                            goto LRecurse;
                        }
                    }
                    return false;

                case TypeKind.TK_TypeParameterType:
                    if (typeVars != null && typeVars.Size > 0)
                    {
                        int ivar = type.AsTypeParameterType().GetIndexInTotalParameters();
                        return ivar < typeVars.Size && type == typeVars.Item(ivar);
                    }
                    return true;
            }
        }

        public static bool ParametersContainTyVar(TypeArray @params, TypeParameterType typeFind)
        {
            Debug.Assert(@params != null);
            Debug.Assert(typeFind != null);
            for (int p = 0; p < @params.size; p++)
            {
                CType sym = @params[p];
                if (TypeContainsType(sym, typeFind))
                {
                    return true;
                }
            }
            return false;
        }

        public AggregateSymbol GetReqPredefAgg(PredefinedType pt)
        {
            return m_predefTypes.GetReqPredefAgg(pt);
        }

        public AggregateSymbol GetOptPredefAgg(PredefinedType pt)
        {
            return m_predefTypes.GetOptPredefAgg(pt);
        }

        public TypeArray CreateArrayOfUnitTypes(int cSize)
        {
            CType[] ppArray = new CType[cSize];
            for (int i = 0; i < cSize; i++)
            {
                ppArray[i] = GetUnitType();
            }
            return m_BSymmgr.AllocParams(cSize, ppArray);
        }

        public TypeArray ConcatenateTypeArrays(TypeArray pTypeArray1, TypeArray pTypeArray2)
        {
            return m_BSymmgr.ConcatParams(pTypeArray1, pTypeArray2);
        }

        public TypeArray GetStdMethTyVarArray(int cTyVars)
        {
            TypeParameterType[] prgvar = new TypeParameterType[cTyVars];

            for (int ivar = 0; ivar < cTyVars; ivar++)
            {
                prgvar[ivar] = GetStdMethTypeVar(ivar);
            }

            return m_BSymmgr.AllocParams(cTyVars, (CType[])prgvar);
        }

        public CType SubstType(CType typeSrc, SubstContext pctx)
        {
            return (pctx == null || pctx.FNop()) ? typeSrc : SubstTypeCore(typeSrc, pctx);
        }

        public CType SubstType(CType typeSrc, AggregateType atsCls)
        {
            return SubstType(typeSrc, atsCls, (TypeArray)null);
        }

        public CType SubstType(CType typeSrc, AggregateType atsCls, TypeArray typeArgsMeth)
        {
            return SubstType(typeSrc, atsCls != null ? atsCls.GetTypeArgsAll() : null, typeArgsMeth);
        }

        public CType SubstType(CType typeSrc, CType typeCls, TypeArray typeArgsMeth)
        {
            return SubstType(typeSrc, typeCls.IsAggregateType() ? typeCls.AsAggregateType().GetTypeArgsAll() : null, typeArgsMeth);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, AggregateType atsCls, TypeArray typeArgsMeth)
        {
            return SubstTypeArray(taSrc, atsCls != null ? atsCls.GetTypeArgsAll() : null, typeArgsMeth);
        }

        public TypeArray SubstTypeArray(TypeArray taSrc, AggregateType atsCls)
        {
            return this.SubstTypeArray(taSrc, atsCls, (TypeArray)null);
        }

        public bool SubstEqualTypes(CType typeDst, CType typeSrc, CType typeCls, TypeArray typeArgsMeth)
        {
            return SubstEqualTypes(typeDst, typeSrc, typeCls.IsAggregateType() ? typeCls.AsAggregateType().GetTypeArgsAll() : null, typeArgsMeth, SubstTypeFlags.NormNone);
        }

        public bool SubstEqualTypes(CType typeDst, CType typeSrc, CType typeCls)
        {
            return SubstEqualTypes(typeDst, typeSrc, typeCls, (TypeArray)null);
        }

        //public bool SubstEqualTypeArrays(TypeArray taDst, TypeArray taSrc, AggregateType atsCls, TypeArray typeArgsMeth)
        //{
        //    return SubstEqualTypeArrays(taDst, taSrc, atsCls != null ? atsCls.GetTypeArgsAll() : (TypeArray)null, typeArgsMeth, SubstTypeFlags.NormNone);
        //}

        public TypeParameterType GetStdMethTypeVar(int iv)
        {
            return stvcMethod.GetTypeVarSym(iv, this, true);
        }

        public TypeParameterType GetStdClsTypeVar(int iv)
        {
            return stvcClass.GetTypeVarSym(iv, this, false);
        }

        public TypeParameterType GetTypeParameter(TypeParameterSymbol pSymbol)
        {
            // These guys should be singletons for each.

            TypeParameterType pTypeParameter = m_typeTable.LookupTypeParameter(pSymbol);
            if (pTypeParameter == null)
            {
                pTypeParameter = m_typeFactory.CreateTypeParameter(pSymbol);
                m_typeTable.InsertTypeParameter(pSymbol, pTypeParameter);
            }

            return pTypeParameter;
        }

        internal void Init(BSYMMGR bsymmgr, PredefinedTypes predefTypes)
        {
            m_BSymmgr = bsymmgr;
            m_predefTypes = predefTypes;
        }

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // RUNTIME BINDER ONLY CHANGE
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        internal bool GetBestAccessibleType(CSemanticChecker semanticChecker, BindingContext bindingContext, CType typeSrc, out CType typeDst)
        {
            // This method implements the "best accessible type" algorithm for determining the type
            // of untyped arguments in the runtime binder. It is also used in method type inference
            // to fix type arguments to types that are accessible.

            // The new type is returned in an out parameter. The result will be true (and the out param
            // non-null) only when the algorithm could find a suitable accessible type.

            Debug.Assert(semanticChecker != null);
            Debug.Assert(bindingContext != null);
            Debug.Assert(typeSrc != null);

            typeDst = null;

            if (semanticChecker.CheckTypeAccess(typeSrc, bindingContext.ContextForMemberLookup()))
            {
                // If we already have an accessible type, then use it. This is the terminal point of the recursion.
                typeDst = typeSrc;
                return true;
            }

            // These guys have no accessibility concerns.
            Debug.Assert(!typeSrc.IsVoidType() && !typeSrc.IsErrorType() && !typeSrc.IsTypeParameterType());

            if (typeSrc.IsParameterModifierType() || typeSrc.IsPointerType())
            {
                // We cannot vary these.
                return false;
            }

            CType intermediateType;
            if ((typeSrc.isInterfaceType() || typeSrc.isDelegateType()) && TryVarianceAdjustmentToGetAccessibleType(semanticChecker, bindingContext, typeSrc.AsAggregateType(), out intermediateType))
            {
                // If we have an interface or delegate type, then it can potentially be varied by its type arguments
                // to produce an accessible type, and if that's the case, then return that.
                // Example: IEnumerable<PrivateConcreteFoo> --> IEnumerable<PublicAbstractFoo>
                typeDst = intermediateType;

                Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
                return true;
            }
            
            if (typeSrc.IsArrayType() && TryArrayVarianceAdjustmentToGetAccessibleType(semanticChecker, bindingContext, typeSrc.AsArrayType(), out intermediateType))
            {
                // Similarly to the interface and delegate case, arrays are covariant in their element type and
                // so we can potentially produce an array type that is accessible.
                // Example: PrivateConcreteFoo[] --> PublicAbstractFoo[]
                typeDst = intermediateType;

                Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
                return true;
            }

            if (typeSrc.IsNullableType())
            {
                // We have an inaccessible nullable type, which means that the best we can do is System.ValueType.
                typeDst = this.GetOptPredefAgg(PredefinedType.PT_VALUE).getThisType();

                Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
                return true;
            }

            if (typeSrc.IsArrayType())
            {
                // We have an inaccessible array type for which we could not earlier find a better array type
                // with a covariant conversion, so the best we can do is System.Array.
                typeDst = this.GetReqPredefAgg(PredefinedType.PT_ARRAY).getThisType();

                Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
                return true;
            }

            Debug.Assert(typeSrc.IsAggregateType());

            if (typeSrc.IsAggregateType())
            {
                // We have an AggregateType, so recurse on its base class.
                AggregateType aggType = typeSrc.AsAggregateType();
                AggregateType baseType = aggType.GetBaseClass();

                if (baseType == null)
                {
                    // This happens with interfaces, for instance. But in that case, the
                    // conversion to object does exist, is an implicit reference conversion,
                    // and so we will use it.
                    baseType = this.GetReqPredefAgg(PredefinedType.PT_OBJECT).getThisType();
                }

                return GetBestAccessibleType(semanticChecker, bindingContext, baseType, out typeDst);
            }

            return false;
        }

        private bool TryVarianceAdjustmentToGetAccessibleType(CSemanticChecker semanticChecker, BindingContext bindingContext, AggregateType typeSrc, out CType typeDst)
        {
            Debug.Assert(typeSrc != null);
            Debug.Assert(typeSrc.isInterfaceType() || typeSrc.isDelegateType());

            typeDst = null;

            AggregateSymbol aggSym = typeSrc.GetOwningAggregate();
            AggregateType aggOpenType = aggSym.getThisType();

            if (!semanticChecker.CheckTypeAccess(aggOpenType, bindingContext.ContextForMemberLookup()))
            {
                // if the aggregate symbol itself is not accessible, then forget it, there is no
                // variance that will help us arrive at an accessible type.
                return false;
            }

            TypeArray typeArgs = typeSrc.GetTypeArgsThis();
            TypeArray typeParams = aggOpenType.GetTypeArgsThis();
            CType[] newTypeArgsTemp = new CType[typeArgs.size];

            for (int i = 0; i < typeArgs.size; i++)
            {
                if (semanticChecker.CheckTypeAccess(typeArgs.Item(i), bindingContext.ContextForMemberLookup()))
                {
                    // we have an accessible argument, this position is not a problem.
                    newTypeArgsTemp[i] = typeArgs.Item(i);
                    continue;
                }

                if (!typeArgs.Item(i).IsRefType() || !typeParams.Item(i).AsTypeParameterType().Covariant)
                {
                    // This guy is inaccessible, and we are not going to be able to vary him, so we need to fail.
                    return false;
                }

                CType intermediateTypeArg;
                if (GetBestAccessibleType(semanticChecker, bindingContext, typeArgs.Item(i), out intermediateTypeArg))
                {
                    // now we either have a value type (which must be accessible due to the above
                    // check, OR we have an inaccessible type (which must be a ref type). In either
                    // case, the recursion worked out and we are OK to vary this argument.
                    newTypeArgsTemp[i] = intermediateTypeArg;
                    continue;
                }
                else
                {
                    Debug.Assert(false, "GetBestAccessibleType unexpectedly failed on a type that was used as a type parameter");
                    return false;
                }
            }

            TypeArray newTypeArgs = semanticChecker.getBSymmgr().AllocParams(typeArgs.size, newTypeArgsTemp);
            CType intermediateType = this.GetAggregate(aggSym, typeSrc.outerType, newTypeArgs);

            // All type arguments were varied successfully, which means now we must be accessible. But we could
            // have violated constraints. Let's check that out.

            if (!TypeBind.CheckConstraints(semanticChecker, null/*ErrorHandling*/, intermediateType, CheckConstraintsFlags.NoErrors))
            {
                // Uh oh, we messed up the type constraints.
                return false;
            }

            typeDst = intermediateType;
            Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
            return true;
        }

        private bool TryArrayVarianceAdjustmentToGetAccessibleType(CSemanticChecker semanticChecker, BindingContext bindingContext, ArrayType typeSrc, out CType typeDst)
        {
            Debug.Assert(typeSrc != null);

            typeDst = null;

            // We are here because we have an array type with an inaccessible element type. If possible,
            // we should create a new array type that has an accessible element type for which a
            // conversion exists.

            CType elementType = typeSrc.GetElementType();
            if (!elementType.IsRefType())
            {
                // Covariant array conversions exist for reference types only.
                return false;
            }

            CType intermediateType;
            if (GetBestAccessibleType(semanticChecker, bindingContext, elementType, out intermediateType))
            {
                typeDst = this.GetArray(intermediateType, typeSrc.rank);

                Debug.Assert(semanticChecker.CheckTypeAccess(typeDst, bindingContext.ContextForMemberLookup()));
                return true;
            }

            return false;
        }
        
        private Dictionary<Tuple<Assembly, Assembly>, bool> internalsVisibleToCalculated
            = new Dictionary<Tuple<Assembly, Assembly>, bool>();

#if FEATURE_NETCORE
        [SecuritySafeCritical]
#endif
        internal bool InternalsVisibleTo(Assembly assemblyThatDefinesAttribute, Assembly assemblyToCheck)
        {
            bool result;

            var key = Tuple.Create(assemblyThatDefinesAttribute, assemblyToCheck);
            if (!internalsVisibleToCalculated.TryGetValue(key, out result))
            {
#if !SILVERLIGHT || FEATURE_NETCORE
                AssemblyName assyName = null;

                // Assembly.GetName() requires FileIOPermission to FileIOPermissionAccess.PathDiscovery.
                // If we don't have that (we're in low trust), then we are going to effectively turn off
                // InternalsVisibleTo. The alternative is to crash when this happens. (TODO: can I ask
                // the security system up front without taking an exception?)

                try
                {
                    assyName = assemblyToCheck.GetName();
                }
                catch (System.Security.SecurityException)
                {
                    result = false;
                    goto SetMemo;
                }

                result = assemblyThatDefinesAttribute.GetCustomAttributes(true)
                    .OfType<InternalsVisibleToAttribute>()
                    .Select(ivta => new AssemblyName(ivta.AssemblyName))
                    .Any(an => AssemblyName.ReferenceMatchesDefinition(an, assyName));

            SetMemo:
#endif
                internalsVisibleToCalculated[key] = result;
            }
            
            return result;
        }
        
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // END RUNTIME BINDER ONLY CHANGE
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    }
}
