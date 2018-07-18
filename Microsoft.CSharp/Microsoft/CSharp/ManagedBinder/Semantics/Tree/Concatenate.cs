// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // UNDONE:  This should probably be made into just another EXPRBINOP flavour
    internal class EXPRCONCAT : EXPR
    {
        public EXPR FirstArgument;
        public EXPR GetFirstArgument() { return FirstArgument; }
        public void SetFirstArgument(EXPR value) { FirstArgument = value; }
        public EXPR SecondArgument;
        public EXPR GetSecondArgument() { return SecondArgument; }
        public void SetSecondArgument(EXPR value) { SecondArgument = value; }
    }
}
