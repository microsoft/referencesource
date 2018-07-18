// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRBINOP : EXPR
    {
        private EXPR OptionalLeftChild;
        public EXPR GetOptionalLeftChild() { return OptionalLeftChild; }
        public void SetOptionalLeftChild(EXPR value) { OptionalLeftChild = value; }

        private EXPR OptionalRightChild;
        public EXPR GetOptionalRightChild() { return OptionalRightChild; }
        public void SetOptionalRightChild(EXPR value) { OptionalRightChild = value; }

        private EXPR OptionalUserDefinedCall;
        public EXPR GetOptionalUserDefinedCall() { return OptionalUserDefinedCall; }
        public void SetOptionalUserDefinedCall(EXPR value) { OptionalUserDefinedCall = value; }

        public MethWithInst predefinedMethodToCall;
        public bool isLifted;
        
        private MethPropWithInst UserDefinedCallMethod;
        public MethPropWithInst GetUserDefinedCallMethod() { return UserDefinedCallMethod; }
        public void SetUserDefinedCallMethod(MethPropWithInst value) { UserDefinedCallMethod = value; }
    }
}
