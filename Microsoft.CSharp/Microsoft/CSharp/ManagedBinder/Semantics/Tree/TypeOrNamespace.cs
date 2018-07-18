// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /*************************************************************************************************
         This is the base class for this set of EXPRs. When binding a type, the result
         must be a type or a namespace. This EXPR encapsulates that fact. The lhs member is the EXPR 
         tree that was bound to resolve the type or namespace.
     *************************************************************************************************/
    internal class EXPRTYPEORNAMESPACE : EXPR
    {
        public ITypeOrNamespace TypeOrNamespace;
    }
}
