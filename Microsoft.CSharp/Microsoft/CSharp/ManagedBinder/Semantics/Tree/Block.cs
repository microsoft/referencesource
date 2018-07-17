// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRBLOCK : EXPRSTMT
    {
        private EXPRSTMT OptionalStatements;
        public EXPRSTMT GetOptionalStatements() { return OptionalStatements; }
        public void SetOptionalStatements(EXPRSTMT value) { OptionalStatements = value; }

        public Scope OptionalScopeSymbol;
    }
}
