// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRUSERLOGOP : EXPR
    {
        public EXPR TrueFalseCall;
        public EXPRCALL OperatorCall;
        public EXPR FirstOperandToExamine;
    }
}
