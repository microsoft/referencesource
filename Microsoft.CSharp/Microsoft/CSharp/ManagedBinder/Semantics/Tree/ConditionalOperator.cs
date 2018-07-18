// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRQUESTIONMARK : EXPR
    {
        // UNDONE: Refactor so that the condition, consequence and alternative
        // UNDONE: are all in one place. This business of representing the colon
        // UNDONE: as a binop is goofy.
        public EXPR TestExpression;
        public EXPR GetTestExpression() { return TestExpression; }
        public void SetTestExpression(EXPR value) { TestExpression = value; }
        public EXPRBINOP Consequence;
        public EXPRBINOP GetConsequence() { return Consequence; }
        public void SetConsequence(EXPRBINOP value) { Consequence = value; }
    }
}
