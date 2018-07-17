// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

namespace Microsoft.CSharp.RuntimeBinder.Semantics
{
    /////////////////////////////////////////////////////////////////////////////////
    // This is the base interface that Type and Namespace symbol inherit.

    interface ITypeOrNamespace
    {
        bool IsType();
        bool IsNamespace();

        AssemblyQualifiedNamespaceSymbol AsNamespace();
        CType AsType();
    }
}
