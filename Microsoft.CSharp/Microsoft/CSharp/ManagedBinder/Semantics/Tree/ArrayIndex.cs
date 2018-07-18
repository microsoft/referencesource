// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRARRAYINDEX : EXPR
    {
        private EXPR Array;
        public EXPR GetArray() { return Array; }
        public void SetArray(EXPR value) { Array = value; }

        private EXPR Index;
        public EXPR GetIndex() { return Index; }
        public void SetIndex(EXPR value) { Index = value; }
    }
}
