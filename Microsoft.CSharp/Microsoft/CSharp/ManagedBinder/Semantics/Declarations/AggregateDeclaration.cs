// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Reflection;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // AggregateDeclaration
    //
    // AggregateDeclaration - represents a declaration of a aggregate type. With partial classes,
    // an aggregate type might be declared in multiple places.  This symbol represents
    // on of the declarations.
    //
    // parent is the containing Declaration.
    // ----------------------------------------------------------------------------

    // Either a ClassNode or a DelegateNode
    class AggregateDeclaration : Declaration
    {
        public AggregateSymbol Agg()
        {
            return bag.AsAggregateSymbol();
        }

        public new InputFile getInputFile()
        {
            return null;
        }

        public new Assembly GetAssembly()
        {
            return Agg().AssociatedAssembly;
        }
    }
}
