// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // A symbol table is a helper class used by the symbol manager. There are
    // two symbol tables; a global and a local.

    class SYMTBL
    {
        /////////////////////////////////////////////////////////////////////////////////
        // Public

        public SYMTBL()
        {
            dictionary = new Dictionary<Key, Symbol>();
        }

        public Symbol LookupSym(Name name, ParentSymbol parent, symbmask_t kindmask)
        {
            Key k = new Key(name, parent);
            Symbol sym;

            if (dictionary.TryGetValue(k, out sym))
            {
                return FindCorrectKind(sym, kindmask);
            }

            return null;
        }

        public void InsertChild(ParentSymbol parent, Symbol child)
        {
            Debug.Assert(child.nextSameName == null);
            Debug.Assert(child.parent == null || child.parent == parent);
            child.parent = parent;

            // Place the child into the hash table.
            InsertChildNoGrow(child);
        }

        private void InsertChildNoGrow(Symbol child)
        {
            Key k = new Key(child.name, child.parent);
            Symbol sym;

            if (dictionary.TryGetValue(k, out sym))
            {
                // Link onto the end of the symbol chain here.
                while (sym != null && sym.nextSameName != null)
                {
                    sym = sym.nextSameName;
                }

                Debug.Assert(sym != null && sym.nextSameName == null);
                sym.nextSameName = child;
                return;
            }
            else
            {
                dictionary.Add(k, child);
            }
        }

        private static Symbol FindCorrectKind(Symbol sym, symbmask_t kindmask)
        {
            do
            {
                if ((kindmask & sym.mask()) != 0)
                {
                    return sym;
                }
                sym = sym.nextSameName;
            } while (sym != null);

            return null;
        }

        private Dictionary<Key, Symbol> dictionary;

        sealed class Key
        {
            private readonly Name name;
            private readonly ParentSymbol parent;

            public Key(Name name, ParentSymbol parent)
            {
                this.name = name;
                this.parent = parent;
            }

            public override bool Equals(object obj)
            {
                Key k = obj as Key;
                return k != null && name.Equals(k.name) && parent.Equals(k.parent);
            }

            public override int GetHashCode()
            {
                return name.GetHashCode() ^ parent.GetHashCode();
            }
        }
    }
}
