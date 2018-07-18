// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRARRAYLENGTH : EXPR
    {
        private EXPR Array;
        public EXPR GetArray() { return Array; }
        public void SetArray(EXPR value) { Array = value; }
    }
}
