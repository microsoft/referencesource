// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    class LocalVariableSymbol : VariableSymbol
    {
        // UNDONE: To do expression tree rewriting we need to keep a map between a
        // UNDONE: local in an expression tree and the result of a ParameterExpression
        // UNDONE: creation.  We really ought to build a table to do the mapping in the
        // UNDONE: rewriter, but in the interests of expediency I've just put the mapping here
        // UNDONE: for now.

        public EXPRWRAP wrap;

        public bool isThis;           // Is this the one and only <this> pointer?
        // movedToField should have iIteratorLocal set appropriately
        public bool fUsedInAnonMeth;   // Set if the local is ever used in an anon method

        public void SetType(CType pType)
        {
            type = pType;
        }

        public new CType GetType()
        {
            return type;
        }
    }
}
