// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // Declaration
    //
    // Base class for NamespaceDeclaration and AggregateDeclaration. Parent is another DECL.
    // Children are DECLs.
    // ----------------------------------------------------------------------------

    class Declaration : ParentSymbol
    {
        public NamespaceOrAggregateSymbol bag;
        public Declaration declNext;
    }
}
