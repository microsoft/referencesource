// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    internal class EXPRLIST : EXPR
    {
        // UNDONE: DevDivBugs 49559 - Make the EXPRLIST actually store an ATLList<EXPR*> instead
        // of using this funny chaining thing. Note that the very last LIST node will have its
        // NextListNode be the last element.
        public EXPR OptionalElement;
        public EXPR GetOptionalElement() { return OptionalElement; }
        public void SetOptionalElement(EXPR value) { OptionalElement = value; }
        public EXPR OptionalNextListNode;
        public EXPR GetOptionalNextListNode() { return OptionalNextListNode; }
        public void SetOptionalNextListNode(EXPR value) { OptionalNextListNode = value; }
    }
}
