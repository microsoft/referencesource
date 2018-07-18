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
    // ParentSymbol - a symbol that can contain other symbols as children.
    //
    // ----------------------------------------------------------------------------

    class ParentSymbol : Symbol
    {
        public Symbol firstChild;       // List of all children of this symbol
        private Symbol lastChild;

        // This adds the sym to the child list but doesn't associate it
        // in the symbol table.

        public void AddToChildList(Symbol sym)
        {
            Debug.Assert(sym != null /*&& this != null */);

            // If parent is set it should be set to us!
            Debug.Assert(sym.parent == null || sym.parent == this);
            // There shouldn't be a nextChild.
            Debug.Assert(sym.nextChild == null);

            if (lastChild == null)
            {
                Debug.Assert(firstChild == null);
                firstChild = lastChild = sym;
            }
            else
            {
                this.lastChild.nextChild = sym;
                this.lastChild = sym;
                sym.nextChild = null;

#if DEBUG
                // Validate our chain.
                Symbol psym;
                int count = 400; // Limited the length of chain that we'll run - so debug perf doesn't stink too badly.
                for (psym = this.firstChild; psym != null && psym.nextChild != null && --count > 0; )
                    psym = psym.nextChild;
                Debug.Assert(this.lastChild == psym || count == 0);
#endif
            }

            sym.parent = this;
        }
    }
}