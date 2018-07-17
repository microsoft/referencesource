// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    class TypeParameterSymbol : Symbol
    {
        private bool m_bIsMethodTypeParameter;
        private bool m_bHasRefBound;
        private bool m_bHasValBound;
        private SpecCons m_constraints;

        private TypeParameterType m_pTypeParameterType;

        private int m_nIndexInOwnParameters;
        private int m_nIndexInTotalParameters;

        private TypeArray m_pBounds;
        private TypeArray m_pInterfaceBounds;

        private AggregateType m_pEffectiveBaseClass;
        private CType m_pDeducedBaseClass; // This may be a NullableType or an ArrayType etc, for error reporting.

        public bool Covariant;
        public bool Invariant { get { return !Covariant && !Contravariant; } }
        public bool Contravariant;

        public void SetTypeParameterType(TypeParameterType pType)
        {
            m_pTypeParameterType = pType;
        }

        public TypeParameterType GetTypeParameterType()
        {
            return m_pTypeParameterType;
        }

        public bool IsMethodTypeParameter()
        {
            return m_bIsMethodTypeParameter;
        }

        public void SetIsMethodTypeParameter(bool b)
        {
            m_bIsMethodTypeParameter = b;
        }

        public int GetIndexInOwnParameters()
        {
            return m_nIndexInOwnParameters;
        }

        public void SetIndexInOwnParameters(int index)
        {
            m_nIndexInOwnParameters = index;
        }

        public int GetIndexInTotalParameters()
        {
            return m_nIndexInTotalParameters;
        }

        public void SetIndexInTotalParameters(int index)
        {
            Debug.Assert(index >= m_nIndexInOwnParameters);
            m_nIndexInTotalParameters = index;
        }

        public TypeArray GetInterfaceBounds()
        {
            return m_pInterfaceBounds;
        }

        public void SetBounds(TypeArray pBounds)
        {
            m_pBounds = pBounds;
            m_pInterfaceBounds = null;
            m_pEffectiveBaseClass = null;
            m_pDeducedBaseClass = null;
            m_bHasRefBound = false;
            m_bHasValBound = false;
        }

        public TypeArray GetBounds()
        {
            return m_pBounds;
        }

        public void SetConstraints(SpecCons constraints)
        {
            m_constraints = constraints;
        }

        public AggregateType GetEffectiveBaseClass()
        {
            return m_pEffectiveBaseClass;
        }

        public bool IsValueType()
        {
            return (m_constraints & SpecCons.Val) > 0 || m_bHasValBound;
        }

        public bool IsReferenceType()
        {
            return (m_constraints & SpecCons.Ref) > 0 || m_bHasRefBound;
        }

        public bool IsNonNullableValueType()
        {
            return (m_constraints & SpecCons.Val) > 0 || m_bHasValBound && !m_pDeducedBaseClass.IsNullableType();
        }

        public bool HasNewConstraint()
        {
            return (m_constraints & SpecCons.New) > 0;
        }

        public bool HasRefConstraint()
        {
            return (m_constraints & SpecCons.Ref) > 0;
        }

        public bool HasValConstraint()
        {
            return (m_constraints & SpecCons.Val) > 0;
        }
    }
}
