// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /////////////////////////////////////////////////////////////////////////////////

    class TypeParameterType : CType
    {
        public TypeParameterSymbol GetTypeParameterSymbol() { return m_pTypeParameterSymbol; }
        public void SetTypeParameterSymbol(TypeParameterSymbol pTypePArameterSymbol) { m_pTypeParameterSymbol = pTypePArameterSymbol; }

        public ParentSymbol GetOwningSymbol() { return m_pTypeParameterSymbol.parent; }


        public bool DependsOn(TypeParameterType pType)
        {
            Debug.Assert(pType != null);

            // * If a type parameter T is used as a constraint for type parameter S
            //   then S depends on T.
            // * If a type parameter S depends on a type parameter T and T depends on
            //   U then S depends on U.

            TypeArray pConstraints = GetBounds();
            for (int iConstraint = 0; iConstraint < pConstraints.size; ++iConstraint)
            {
                CType pConstraint = pConstraints.Item(iConstraint);
                if (pConstraint == pType)
                {
                    return true;
                }
                if (pConstraint.IsTypeParameterType() &&
                    pConstraint.AsTypeParameterType().DependsOn(pType))
                {
                    return true;
                }
            }
            return false;
        }

        // Forward calls into the symbol.
        public bool Covariant { get { return m_pTypeParameterSymbol.Covariant; } }
        public bool Invariant { get { return m_pTypeParameterSymbol.Invariant; } }
        public bool Contravariant { get { return m_pTypeParameterSymbol.Contravariant; } }
        public bool IsValueType() { return m_pTypeParameterSymbol.IsValueType(); }
        public bool IsReferenceType() { return m_pTypeParameterSymbol.IsReferenceType(); }
        public bool IsNonNullableValueType() { return m_pTypeParameterSymbol.IsNonNullableValueType(); }
        public bool HasNewConstraint() { return m_pTypeParameterSymbol.HasNewConstraint(); }
        public bool HasRefConstraint() { return m_pTypeParameterSymbol.HasRefConstraint(); }
        public bool HasValConstraint() { return m_pTypeParameterSymbol.HasValConstraint(); }
        public bool IsMethodTypeParameter() { return m_pTypeParameterSymbol.IsMethodTypeParameter(); }
        public int GetIndexInOwnParameters() { return m_pTypeParameterSymbol.GetIndexInOwnParameters(); }
        public int GetIndexInTotalParameters() { return m_pTypeParameterSymbol.GetIndexInTotalParameters(); }
        public TypeArray GetBounds() { return m_pTypeParameterSymbol.GetBounds(); }
        public TypeArray GetInterfaceBounds() { return m_pTypeParameterSymbol.GetInterfaceBounds(); }
        public AggregateType GetEffectiveBaseClass() { return m_pTypeParameterSymbol.GetEffectiveBaseClass(); }

        private TypeParameterSymbol m_pTypeParameterSymbol;
    }
}
