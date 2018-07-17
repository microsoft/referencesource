// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRFUNCPTR : EXPR
    {
        public MethWithInst mwi;
        public EXPR OptionalObject;
        public void SetOptionalObject(EXPR value) { OptionalObject = value; }
    }
}
