// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRUSERDEFINEDCONVERSION : EXPR
    {
        public EXPR Argument;
        public EXPR UserDefinedCall;
        public MethWithInst UserDefinedCallMethod;
    }
}
