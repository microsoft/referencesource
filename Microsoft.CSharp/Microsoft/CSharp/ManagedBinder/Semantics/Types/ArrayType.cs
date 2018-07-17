// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    // ArrayType - a symbol representing an array.
    // ----------------------------------------------------------------------------

    class ArrayType : CType
    {
        // rank of the array. zero means unknown rank int [?].
        public int rank;

        public CType GetElementType() { return m_pElementType; }
        public void SetElementType(CType pType) { m_pElementType = pType; }

        // Returns the first non-array type in the parent chain.
        public CType GetBaseElementType()
        {
            CType type;
            for (type = GetElementType(); type.IsArrayType(); type = type.AsArrayType().GetElementType()) ;
            return type;
        }

        private CType m_pElementType;
    }
}

