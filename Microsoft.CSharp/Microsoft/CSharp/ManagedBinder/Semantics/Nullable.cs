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
    internal class CNullable
    {
        private SymbolLoader m_pSymbolLoader;
        private ExprFactory m_exprFactory;
        private ErrorHandling m_pErrorContext;

        private SymbolLoader GetSymbolLoader()
        {
            return m_pSymbolLoader;
        }
        private ExprFactory GetExprFactory()
        {
            return m_exprFactory;
        }
        private ErrorHandling GetErrorContext()
        {
            return m_pErrorContext;
        }
        public static bool IsNullableConstructor(EXPR expr)
        {
            Debug.Assert(expr != null);

            if (!expr.isCALL())
            {
                return false;
            }

            EXPRCALL pCall = expr.asCALL();
            if (pCall.GetMemberGroup().GetOptionalObject() != null)
            {
                return false;
            }

            MethodSymbol meth = pCall.mwi.Meth();
            if (meth == null)
            {
                return false;
            }
            return meth.IsNullableConstructor();
        }
        public static EXPR StripNullableConstructor(EXPR pExpr)
        {
            while (IsNullableConstructor(pExpr))
            {
                Debug.Assert(pExpr.isCALL());
                pExpr = pExpr.asCALL().GetOptionalArguments();
                Debug.Assert(pExpr != null && !pExpr.isLIST());
            }
            return pExpr;
        }

        // Value
        public EXPR BindValue(EXPR exprSrc)
        {
            Debug.Assert(exprSrc != null && exprSrc.type.IsNullableType());

            // For new T?(x), the answer is x.
            if (CNullable.IsNullableConstructor(exprSrc))
            {
                Debug.Assert(exprSrc.asCALL().GetOptionalArguments() != null && !exprSrc.asCALL().GetOptionalArguments().isLIST());
                return exprSrc.asCALL().GetOptionalArguments();
            }

            CType typeBase = exprSrc.type.AsNullableType().GetUnderlyingType();
            AggregateType ats = exprSrc.type.AsNullableType().GetAts(GetErrorContext());
            if (ats == null)
            {
                EXPRPROP rval = GetExprFactory().CreateProperty(typeBase, exprSrc);
                rval.SetError();
                return rval;
            }

            // UNDONE: move this to transform pass ...
            PropertySymbol prop = GetSymbolLoader().getBSymmgr().propNubValue;
            if (prop == null)
            {
                prop = GetSymbolLoader().getPredefinedMembers().GetProperty(PREDEFPROP.PP_G_OPTIONAL_VALUE);
                GetSymbolLoader().getBSymmgr().propNubValue = prop;
            }

            PropWithType pwt = new PropWithType(prop, ats);
            MethWithType mwt = new MethWithType(prop != null ? prop.methGet : null, ats);
            MethPropWithInst mpwi = new MethPropWithInst(prop, ats);
            EXPRMEMGRP pMemGroup = GetExprFactory().CreateMemGroup(exprSrc, mpwi);
            EXPRPROP exprRes = GetExprFactory().CreateProperty(typeBase, null, null, pMemGroup, pwt, mwt, null);

            if (prop == null)
            {
                exprRes.SetError();
            }

            return exprRes;
        }

        public EXPRCALL BindNew(EXPR pExprSrc)
        {
            Debug.Assert(pExprSrc != null);

            NullableType pNubSourceType = GetSymbolLoader().GetTypeManager().GetNullable(pExprSrc.type);

            AggregateType pSourceType = pNubSourceType.GetAts(GetErrorContext());
            if (pSourceType == null)
            {
                MethWithInst mwi = new MethWithInst(null, null);
                EXPRMEMGRP pMemGroup = GetExprFactory().CreateMemGroup(pExprSrc, mwi);
                EXPRCALL rval = GetExprFactory().CreateCall(0, pNubSourceType, null, pMemGroup, null);
                rval.SetError();
                return rval;
            }

            // UNDONE: move this to transform pass
            MethodSymbol meth = GetSymbolLoader().getBSymmgr().methNubCtor;
            if (meth == null)
            {
                meth = GetSymbolLoader().getPredefinedMembers().GetMethod(PREDEFMETH.PM_G_OPTIONAL_CTOR);
                GetSymbolLoader().getBSymmgr().methNubCtor = meth;
            }

            MethWithInst methwithinst = new MethWithInst(meth, pSourceType, BSYMMGR.EmptyTypeArray());
            EXPRMEMGRP memgroup = GetExprFactory().CreateMemGroup(null, methwithinst);
            EXPRCALL pExprRes = GetExprFactory().CreateCall(EXPRFLAG.EXF_NEWOBJCALL | EXPRFLAG.EXF_CANTBENULL, pNubSourceType, pExprSrc, memgroup, methwithinst);

            if (meth == null)
            {
                pExprRes.SetError();
            }

            return pExprRes;
        }
        public CNullable(SymbolLoader symbolLoader, ErrorHandling errorContext, ExprFactory exprFactory)
        {
            m_pSymbolLoader = symbolLoader;
            m_pErrorContext = errorContext;
            m_exprFactory = exprFactory;
        }
    }

    internal partial class ExpressionBinder
    {
        // Create an expr for exprSrc.Value where exprSrc.type is a NullableType.
        internal EXPR BindNubValue(EXPR exprSrc)
        {
            return m_nullable.BindValue(exprSrc);
        }

        // Create an expr for new T?(exprSrc) where T is exprSrc.type.
        private EXPRCALL BindNubNew(EXPR exprSrc)
        {
            return m_nullable.BindNew(exprSrc);
        }

    }
}
