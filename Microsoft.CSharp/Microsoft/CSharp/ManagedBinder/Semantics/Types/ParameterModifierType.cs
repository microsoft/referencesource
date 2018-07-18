// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // ParameterModifierType
    //
    // ParameterModifierType - a symbol representing parameter modifier -- either
    // out or ref.
    //
    // ----------------------------------------------------------------------------

    class ParameterModifierType : CType
    {
        public bool isOut;            // True for out parameter, false for ref parameter.

        public CType GetParameterType() { return m_pParameterType; }
        public void SetParameterType(CType pType) { m_pParameterType = pType; }

        private CType m_pParameterType;
    }
}
