// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /*************************************************************************************************
        This wraps the type arguments for a class. It contains the TypeArray which is
        associated with the AggregateType for the instantiation of the class. 
    *************************************************************************************************/

    internal class EXPRTYPEARGUMENTS : EXPR
    {
        public EXPR OptionalElements;
        public EXPR GetOptionalElements() { return OptionalElements; }
        public void SetOptionalElements(EXPR value) { OptionalElements = value; }
    }
}
