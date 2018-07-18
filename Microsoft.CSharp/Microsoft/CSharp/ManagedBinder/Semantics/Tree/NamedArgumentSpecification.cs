// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRNamedArgumentSpecification : EXPR
    {
        public Microsoft.CSharp.RuntimeBinder.Syntax.Name Name;
        public EXPR Value;
    }
}
