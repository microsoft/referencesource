// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRRETURN : EXPRSTMT
    {
        // Return object is optional because of void returns.
        public EXPR OptionalObject;
        public EXPR GetOptionalObject() { return OptionalObject; }
        public void SetOptionalObject(EXPR value) { OptionalObject = value; }
    }
}
