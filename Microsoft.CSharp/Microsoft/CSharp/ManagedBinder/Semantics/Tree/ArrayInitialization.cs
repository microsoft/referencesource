// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRARRINIT : EXPR
    {
        private EXPR OptionalArguments;
        public EXPR GetOptionalArguments() { return OptionalArguments; }
        public void SetOptionalArguments(EXPR value) { OptionalArguments = value; }

        private EXPR OptionalArgumentDimensions;
        public EXPR GetOptionalArgumentDimensions() { return OptionalArgumentDimensions; }
        public void SetOptionalArgumentDimensions(EXPR value) { OptionalArgumentDimensions = value; }

        // The EXPRs bound as the size of the array.
        public int[] dimSizes;
        public int dimSize;
        public bool GeneratedForParamArray;
    }

}
