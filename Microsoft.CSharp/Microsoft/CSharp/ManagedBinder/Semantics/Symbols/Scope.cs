// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    class Scope : ParentSymbol
    {
        public uint nestingOrder;  // the nesting order of this scopes. outermost == 0
    }
}
