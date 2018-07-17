// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRFIELD : EXPR
    {
        public EXPR OptionalObject;
        public EXPR GetOptionalObject() { return OptionalObject; }
        public void SetOptionalObject(EXPR value) { OptionalObject = value; }
        public FieldWithType fwt;
    }
}
