// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    class PointerType : CType
    {
        public CType GetReferentType() { return m_pReferentType; }
        public void SetReferentType(CType pType) { m_pReferentType = pType; }
        private CType m_pReferentType;
    }
}
