// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // ----------------------------------------------------------------------------
    //
    // UnresolvedAggregateSymbol
    //
    // A fabricated AggregateSymbol to represent an imported type that we couldn't resolve.
    // Used for error reporting.
    // In the EE this is used as a place holder until the real AggregateSymbol is created.
    //
    // ----------------------------------------------------------------------------

    class UnresolvedAggregateSymbol : AggregateSymbol
    {
    }
}