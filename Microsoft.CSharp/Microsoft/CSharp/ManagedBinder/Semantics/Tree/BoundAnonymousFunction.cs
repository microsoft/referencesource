// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

using System.Diagnostics;
using Microsoft.CSharp.RuntimeBinder.Syntax;

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    // UNDONE: Rename BoundAnonymousFunction
    internal class EXPRBOUNDLAMBDA : EXPR
    {
        public EXPRBLOCK OptionalBody;
        private Scope argumentScope;            // The scope containing the names of the parameters
        // The scope that will hold this anonymous function. This starts off as the outer scope and is then
        // ratcheted down to the correct scope after the containing method is fully bound.

        public void Initialize(Scope argScope)
        {
            Debug.Assert(argScope != null);
            argumentScope = argScope;
        }

        public AggregateType DelegateType()
        {
            return type.AsAggregateType();
        }
        
        public Scope ArgumentScope()
        {
            Debug.Assert(argumentScope != null);
            return argumentScope;
        }
    }
}
