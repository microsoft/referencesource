// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRASSIGNMENT : EXPR
    {
        private EXPR LHS;
        public EXPR GetLHS() { return LHS; }
        public void SetLHS(EXPR value) { LHS = value; }

        private EXPR RHS;
        public EXPR GetRHS() { return RHS; }
        public void SetRHS(EXPR value) { RHS = value; }
    }
}
