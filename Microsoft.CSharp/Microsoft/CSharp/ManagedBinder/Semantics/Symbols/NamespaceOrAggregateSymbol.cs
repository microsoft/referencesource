// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // NamespaceOrAggregateSymbol
    //
    // Base class for NamespaceSymbol and AggregateSymbol. Bags have DECLSYMs.
    // Parent is another BAG. Children are other BAGs, members, type vars, etc.
    // ----------------------------------------------------------------------------

    abstract class NamespaceOrAggregateSymbol : ParentSymbol
    {
        Declaration declFirst;
        Declaration declLast;

        public NamespaceOrAggregateSymbol()
        {
        }

        // ----------------------------------------------------------------------------
        // NamespaceOrAggregateSymbol
        // ----------------------------------------------------------------------------

        public Declaration DeclFirst()
        {
            return this.declFirst;
        }

        // Compare to ParentSymbol::AddToChildList
        public void AddDecl(Declaration decl)
        {
            Debug.Assert(decl != null);
            Debug.Assert(this.IsNamespaceSymbol() || this.IsAggregateSymbol());
            Debug.Assert(decl.IsNamespaceDeclaration() || decl.IsAggregateDeclaration());
            Debug.Assert(!this.IsNamespaceSymbol() == !decl.IsNamespaceDeclaration());

            // If parent is set it should be set to us!
            Debug.Assert(decl.bag == null || decl.bag == this);
            // There shouldn't be a declNext.
            Debug.Assert(decl.declNext == null);

            if (this.declLast == null)
            {
                Debug.Assert(declFirst == null);
                this.declFirst = this.declLast = decl;
            }
            else
            {
                this.declLast.declNext = decl;
                this.declLast = decl;

#if DEBUG
                // Validate our chain.
                Declaration pdecl;
                for (pdecl = declFirst; pdecl != null && pdecl.declNext != null; pdecl = pdecl.declNext)
                { }
                Debug.Assert(pdecl == null || (pdecl == declLast && pdecl.declNext == null));
#endif
            }

            decl.declNext = null;
            decl.bag = this;

            if (decl.IsNamespaceDeclaration())
                decl.AsNamespaceDeclaration().Bag().DeclAdded(decl.AsNamespaceDeclaration());
        }
    }
}
