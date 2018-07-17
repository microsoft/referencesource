// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // ErrorType
    //
    // ErrorType - a symbol representing an error that has been reported.
    // ----------------------------------------------------------------------------

    class ErrorType : CType
    {
        public Name nameText;
        public TypeArray typeArgs;

        public bool HasParent() { return m_pParentType != null || m_pParentNS != null; }

        public bool HasTypeParent() { return m_pParentType != null; }
        public CType GetTypeParent() { return m_pParentType; }
        public void SetTypeParent(CType pType) { m_pParentType = pType; }

        public bool HasNSParent() { return m_pParentNS != null; }
        public AssemblyQualifiedNamespaceSymbol GetNSParent() { return m_pParentNS; }
        public void SetNSParent(AssemblyQualifiedNamespaceSymbol pNS) { m_pParentNS = pNS; }

        private CType m_pParentType;
        private AssemblyQualifiedNamespaceSymbol m_pParentNS;
    }
}
