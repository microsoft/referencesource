// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using System.Reflection;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // PropertySymbol
    //
    // PropertySymbol - a symbol representing a property. Parent is a struct, interface
    // or class (aggregate). No children.
    // ----------------------------------------------------------------------------

    class PropertySymbol : MethodOrPropertySymbol
    {
        public MethodSymbol methGet;            // Getter method (always has same parent)
        public MethodSymbol methSet;            // Setter method (always has same parent)
        public PropertyInfo AssociatedPropertyInfo;

        public bool isIndexer()
        {
            return isOperator;
        }

        public IndexerSymbol AsIndexerSymbol()
        {
            Debug.Assert(isIndexer());
            return (IndexerSymbol)this;
        }
    }
}