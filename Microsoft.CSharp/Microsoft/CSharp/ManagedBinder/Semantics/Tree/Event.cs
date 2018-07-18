// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPREVENT : EXPR
    {
        public EXPR OptionalObject;
        public EventWithType ewt;
    }
}
